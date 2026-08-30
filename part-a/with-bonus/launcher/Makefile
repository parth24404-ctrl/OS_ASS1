# Compile launch.c by linking it against lib_simpleloader.so present in ../bin

CC     = gcc
BIN    = ../bin
TARGET = $(BIN)/launch

# Same 32-bit toolchain configuration as loader/Makefile (see comments there).
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

CFLAGS = $(M32FLAGS) -Wall -I../loader

all: $(TARGET)

$(TARGET): launch.c ../loader/loader.h $(BIN)/lib_simpleloader.so
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) -o $(TARGET) launch.c -L$(BIN) -l_simpleloader -Wl,-rpath,'$$ORIGIN'

clean:
	-@rm -f $(TARGET)
