/*
 * launch.c : driver program for the SimpleLoader shared library.
 *
 * It takes the path of a 32-bit ELF executable as a command line argument,
 * performs sanity checks on it, hands it over to the loader library
 * (lib_simpleloader.so) for loading/execution and finally invokes the
 * loader's cleanup routine.
 */

#include "loader.h"

/*
 * Perform basic checks on the input file before handing it to the loader:
 *  - the file exists and is readable
 *  - it is at least as big as an ELF header
 *  - it carries the ELF magic number, is 32-bit, little endian, i386 and
 *    of type ET_EXEC (statically linked, non-PIE executable)
 * Returns 0 on success, -1 on failure.
 */
static int is_valid_elf32(const char *path) {
  int f = open(path, O_RDONLY);
  if (f < 0) {
    perror("launch: open");
    return -1;
  }

  Elf32_Ehdr hdr;
  ssize_t n = read(f, &hdr, sizeof(hdr));
  close(f);

  if (n != (ssize_t)sizeof(hdr)) {
    fprintf(stderr, "launch: '%s' is too small to be a valid ELF file\n", path);
    return -1;
  }

  if (hdr.e_ident[EI_MAG0] != ELFMAG0 || hdr.e_ident[EI_MAG1] != ELFMAG1 ||
      hdr.e_ident[EI_MAG2] != ELFMAG2 || hdr.e_ident[EI_MAG3] != ELFMAG3) {
    fprintf(stderr, "launch: '%s' is not an ELF file\n", path);
    return -1;
  }
  if (hdr.e_ident[EI_CLASS] != ELFCLASS32) {
    fprintf(stderr, "launch: '%s' is not a 32-bit ELF file\n", path);
    return -1;
  }
  if (hdr.e_ident[EI_DATA] != ELFDATA2LSB) {
    fprintf(stderr, "launch: '%s' is not little endian\n", path);
    return -1;
  }
  if (hdr.e_type != ET_EXEC) {
    fprintf(stderr, "launch: '%s' is not a non-PIE executable\n", path);
    return -1;
  }
  if (hdr.e_machine != EM_386) {
    fprintf(stderr, "launch: '%s' is not built for the i386 machine\n", path);
    return -1;
  }
  if (hdr.e_phoff == 0 || hdr.e_phnum == 0) {
    fprintf(stderr, "launch: '%s' has no program header table\n", path);
    return -1;
  }
  return 0;
}

int main(int argc, char** argv)
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }

  /* 1. carry out necessary checks on the input ELF file */
  if (is_valid_elf32(argv[1]) != 0)
    exit(1);

  /* 2. pass it to the loader for carrying out the loading/execution */
  load_and_run_elf(&argv[1]);

  /* 3. invoke the cleanup routine inside the loader */
  loader_cleanup();
  return 0;
}
