# without-bonus : builds the standalone loader and the fib test case.
#
# If the system has gcc-multilib installed, plain "-m32" is enough.
# Otherwise we fall back to a locally built i386 sysroot.  Override with:
#       make SYSROOT=/path/to/i386 GCCVER=13

CC      = gcc
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

all: loader fib

loader: loader.c loader.h
	$(CC) $(M32FLAGS) -Wall -o loader loader.c

fib: fib.c
	$(CC) -m32 -no-pie -nostdlib -o fib fib.c

clean:
	-@rm -f loader fib
