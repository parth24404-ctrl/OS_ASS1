/*
 * loader.c : SimpleLoader implementation (standalone version, without bonus)
 *
 * Loads a statically linked, 32-bit ELF executable without using any
 * ELF-manipulating library API, and transfers control to its entrypoint.
 *
 * Exposed APIs (declared in loader.h):
 *      void load_and_run_elf(char** exe);
 *      void loader_cleanup(void);
 */

#include "loader.h"

/* ---- Global loader state (needed by loader_cleanup) ---- */
Elf32_Ehdr *ehdr = NULL;   /* pointer to ELF header inside file_buffer   */
Elf32_Phdr *phdr = NULL;   /* pointer to PHDR table inside file_buffer   */
int fd = -1;               /* file descriptor of the ELF file            */

static void  *file_buffer  = NULL; /* heap copy of the entire ELF binary */
static void  *mapped_seg   = NULL; /* mmap'ed segment holding entrypoint */
static size_t mapped_size  = 0;    /* size of the above mapping          */

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  /* unmap the segment we mapped for execution */
  if (mapped_seg != NULL && mapped_seg != MAP_FAILED) {
    if (munmap(mapped_seg, mapped_size) < 0)
      perror("loader_cleanup: munmap failed");
    mapped_seg  = NULL;
    mapped_size = 0;
  }

  /* free the heap buffer that holds the whole binary */
  if (file_buffer != NULL) {
    free(file_buffer);
    file_buffer = NULL;
  }
  ehdr = NULL;
  phdr = NULL;

  /* close the ELF file */
  if (fd >= 0) {
    close(fd);
    fd = -1;               /* mark descriptor invalid */
  }
}

/*
 * Internal helper: print an error, clean up whatever was acquired and exit.
 */
static void fatal(const char *msg) {
  fprintf(stderr, "SimpleLoader error: %s\n", msg);
  loader_cleanup();
  exit(EXIT_FAILURE);
}

/*
 * Load and run the ELF executable file.
 * exe points to the argv slot holding the path of the executable.
 */
void load_and_run_elf(char **exe) {
  if (exe == NULL || exe[0] == NULL)
    fatal("no executable path supplied to the loader");

  /* ---------- 1. Open the ELF file ---------- */
  fd = open(exe[0], O_RDONLY);
  if (fd < 0) {
    perror("open");
    fatal("couldn't open the ELF file");
  }

  /* find the size of the file so that we can slurp it into the heap */
  off_t fsize = lseek(fd, 0, SEEK_END);
  if (fsize < 0) {
    perror("lseek");
    fatal("couldn't determine file size");
  }
  if ((size_t)fsize < sizeof(Elf32_Ehdr))
    fatal("file is too small to be a valid 32-bit ELF");
  if (lseek(fd, 0, SEEK_SET) < 0)
    fatal("couldn't rewind the ELF file");

  /* ---------- 1(b). Read the whole binary into heap memory ---------- */
  file_buffer = malloc((size_t)fsize);
  if (file_buffer == NULL)
    fatal("malloc failed while allocating buffer for the binary");

  size_t total = 0;
  while (total < (size_t)fsize) {
    ssize_t n = read(fd, (char *)file_buffer + total, (size_t)fsize - total);
    if (n < 0) {
      perror("read");
      fatal("failed while reading the ELF file");
    }
    if (n == 0) break;                       /* unexpected EOF */
    total += (size_t)n;
  }
  if (total != (size_t)fsize)
    fatal("short read: could not read the complete ELF file");

  /* ---------- 2. Validate the ELF header ---------- */
  ehdr = (Elf32_Ehdr *)file_buffer;

  if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
      ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3)
    fatal("not an ELF file (bad magic number)");

  if (ehdr->e_ident[EI_CLASS] != ELFCLASS32)
    fatal("not a 32-bit ELF file (compile the test case with -m32)");

  if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    fatal("unsupported ELF data encoding (little endian expected)");

  if (ehdr->e_type != ET_EXEC)
    fatal("not an executable ELF (compile the test case with -no-pie)");

  if (ehdr->e_machine != EM_386)
    fatal("unsupported machine type (i386 expected)");

  if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0)
    fatal("no program header table present in the ELF file");

  if (ehdr->e_phentsize != sizeof(Elf32_Phdr))
    fatal("unexpected program header entry size");

  /* make sure the PHDR table really lies inside the file */
  if ((size_t)ehdr->e_phoff + (size_t)ehdr->e_phnum * sizeof(Elf32_Phdr) >
      (size_t)fsize)
    fatal("program header table lies outside the file");

  /* ---------- 3. Walk the PHDR table for the PT_LOAD holding e_entry ---- */
  phdr = (Elf32_Phdr *)((char *)file_buffer + ehdr->e_phoff);

  Elf32_Addr  entry  = ehdr->e_entry;
  Elf32_Phdr *target = NULL;

  for (int i = 0; i < ehdr->e_phnum; i++) {
    if (phdr[i].p_type != PT_LOAD)
      continue;
    Elf32_Addr vstart = phdr[i].p_vaddr;
    Elf32_Addr vend   = phdr[i].p_vaddr + phdr[i].p_memsz;
    if (entry >= vstart && entry < vend) {
      target = &phdr[i];
      break;
    }
  }

  if (target == NULL)
    fatal("no PT_LOAD segment contains the entrypoint address");

  if (target->p_filesz > target->p_memsz)
    fatal("corrupt program header: p_filesz greater than p_memsz");

  if ((size_t)target->p_offset + (size_t)target->p_filesz > (size_t)fsize)
    fatal("segment contents lie outside the file");

  /* ---------- 4. mmap memory for the segment and copy its content ------- */
  mapped_size = target->p_memsz;
  mapped_seg  = mmap(NULL, mapped_size,
                     PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mapped_seg == MAP_FAILED) {
    perror("mmap");
    mapped_seg = NULL;
    fatal("failed to allocate memory for the segment");
  }

  /* copy p_filesz bytes from the heap copy of the binary ... */
  memcpy(mapped_seg, (char *)file_buffer + target->p_offset, target->p_filesz);
  /* ... and zero-fill the remaining (.bss) part of the segment */
  if (target->p_memsz > target->p_filesz)
    memset((char *)mapped_seg + target->p_filesz, 0,
           target->p_memsz - target->p_filesz);

  /* ---------- 5. Walk to the entrypoint inside the mapped segment ------- */
  /* e_entry need not be equal to p_vaddr, so add the offset within it. */
  void *entry_ptr = (char *)mapped_seg + (entry - target->p_vaddr);

  /* typecast the address to a function pointer matching "_start" */
  int (*_start)(void) = (int (*)(void))entry_ptr;

  /* ---------- 6. Call _start and report the returned value -------------- */
  int result = _start();
  printf("User _start return value = %d\n", result);
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    printf("Usage: %s <ELF Executable>\n", argv[0]);
    exit(1);
  }
  load_and_run_elf(&argv[1]);
  loader_cleanup();
  return 0;
}
