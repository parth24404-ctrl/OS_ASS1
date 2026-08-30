# Create the 32-bit executable for factorial.c using the flags mentioned in the PDF
# It is rebuilt only if the executable is missing or factorial.c has changed.

CC     = gcc
CFLAGS = -m32 -no-pie -nostdlib

all: factorial

factorial: factorial.c
	$(CC) $(CFLAGS) -o factorial factorial.c

clean:
	-@rm -f factorial
