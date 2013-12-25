CC=gcc

all: rcx download

rcx: RCX_Request_Reply.c
	gcc RCX_Request_Reply.c -o rcx

download: RCX_Download.c
	gcc RCX_Download.c -o download

# Makefile for H8/300 cross translation of assembler programs.
# Path to assembler (as) and linker (ld).
BINDIR = /usr/bin/
AS = $(BINDIR)h8300-hms-as
LD = $(BINDIR)h8300-hms-ld

# Options for linker.
LFLAGS = -Trcx.lds

# Entries.
# $< and $@ expands to the actual file name that matches the 
# right hand side and the left hand side of : (colon).
%.o: %.s
	$(AS) --verbose $< -o $@

%.srec: %.o
	$(LD) $(LFLAGS) -o $@ $<