# Create lib_simpleloader.so from loader.c and place it inside ../bin

CC     = gcc
BIN    = ../bin
TARGET = $(BIN)/lib_simpleloader.so

# ---- 32-bit toolchain configuration ------------------------------------
# If the system has gcc-multilib installed, plain "-m32" is enough.
# Otherwise we fall back to a locally built i386 sysroot.
# Override on the command line if your sysroot lives elsewhere, e.g.
#       make SYSROOT=/path/to/i386
SYSROOT ?= $(HOME)/local/i386
GCCVER  ?= 13

ifneq ($(wildcard $(SYSROOT)/usr/include/stdio.h),)
M32FLAGS = -m32 \
	--sysroot=$(SYSROOT) \
	-I$(SYSROOT)/usr/include/i386-linux-gnu \
	-I$(SYSROOT)/usr/include \
	-B$(SYSROOT)/usr/lib/i386-linux-gnu/ \
	-B$(SYSROOT)/usr/lib/gcc/i686-linux-gnu/$(GCCVER)/ \
	-L$(SYSROOT)/usr/lib/i386-linux-gnu \
	-L$(SYSROOT)/usr/lib/gcc/i686-linux-gnu/$(GCCVER)
else
M32FLAGS = -m32
endif

CFLAGS = $(M32FLAGS) -Wall -fPIC

all: $(TARGET)

$(TARGET): loader.c loader.h
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) -shared -o $(TARGET) loader.c

clean:
	-@rm -f $(TARGET)
