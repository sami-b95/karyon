CC=i586-pc-karyon-gcc

include ../envtest

all: netconf
	cp netconf $(KARYON_SYSROOT)/bin/

netconf:
	$(CC) netconf.c -o netconf

clean:
	rm -f netconf
