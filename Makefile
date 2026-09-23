DEVKIT  := /usr/local/xenon
CC      := $(DEVKIT)/bin/xenon-gcc
XEX     := $(DEVKIT)/bin/xenon-xex

CFLAGS  := -I$(DEVKIT)/include -O2 -Wall -std=c99
LDFLAGS := -L$(DEVKIT)/lib -lxenon -lfat -lusb -lm

all: file_manager_xb.xex

file_manager_xb.elf: main.c
	$(CC) $(CFLAGS) main.c -o $@ $(LDFLAGS)

file_manager_xb.xex: file_manager_xb.elf xex.xml
	$(XEX) xex.xml file_manager_xb.elf file_manager_xb.xex

clean:
	rm -f *.elf *.xex
