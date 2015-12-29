PWD=$(shell pwd)
CC=i686-elf-gcc
CFLAGS=-O0 -Wall -nostdlib -fno-builtin "-I$(PWD)" -fno-zero-initialized-in-bss
LD=i686-elf-ld
LDFLAGS=-T $(LDSCRIPT) -e kmain
LDSCRIPT=kernel.lds
OBJ=	grubmb.o \
	kernel/kernel.o \
	kernel/gdt.o \
	kernel/idt.o \
	kernel/isr.o \
	kernel/isr_wrap.o \
	kernel/libc.o \
	kernel/pic.o \
	kernel/printk.o \
	kernel/process.o \
	kernel/schedule.o \
	kernel/screen.o \
	kernel/signal.o \
	kernel/syscall.o \
	kernel/syscalls/accept.o \
	kernel/syscalls/bind.o \
	kernel/syscalls/chdir.o \
	kernel/syscalls/close.o \
	kernel/syscalls/connect.o \
	kernel/syscalls/dup2.o \
	kernel/syscalls/execve.o \
	kernel/syscalls/exit.o \
	kernel/syscalls/fcntl.o \
	kernel/syscalls/fork.o \
	kernel/syscalls/fstat.o \
	kernel/syscalls/fstatfs.o \
	kernel/syscalls/ftruncate.o \
	kernel/syscalls/getcwd.o \
	kernel/syscalls/gsocknam.o \
	kernel/syscalls/gsockopt.o \
	kernel/syscalls/isatty.o \
	kernel/syscalls/kill.o \
	kernel/syscalls/killpg.o \
	kernel/syscalls/listen.o \
	kernel/syscalls/link.o \
	kernel/syscalls/lseek.o \
	kernel/syscalls/mkdir.o \
	kernel/syscalls/open.o \
	kernel/syscalls/pipe2.o \
	kernel/syscalls/read.o \
	kernel/syscalls/recv.o \
	kernel/syscalls/rmdir.o \
	kernel/syscalls/select.o \
	kernel/syscalls/send.o \
	kernel/syscalls/sigact.o \
	kernel/syscalls/sigpmask.o \
	kernel/syscalls/sigsusp.o \
	kernel/syscalls/sigret.o \
	kernel/syscalls/socket.o \
	kernel/syscalls/ssockopt.o \
	kernel/syscalls/tcflush.o \
	kernel/syscalls/tcgattr.o \
	kernel/syscalls/tcgpgrp.o \
	kernel/syscalls/tcsattr.o \
	kernel/syscalls/tcspgrp.o \
	kernel/syscalls/unlink.o \
	kernel/syscalls/wait4.o \
	kernel/syscalls/write.o \
	mm/paging.o \
	fs/ata.o \
	fs/cache.o \
	fs/disk.o \
	fs/ext2.o \
	fs/fifo.o \
	fs/file.o \
	fs/path.o \
	fs/tty.o \
	fs/vt100.o \
	net/arp.o \
	net/pci.o \
	net/rtl8139.o \
	net/ether.o \
	net/ip.o \
	net/tcp.o
KERNEL=kernel.bin

all: $(KERNEL)

$(KERNEL): $(OBJ)
	$(LD) $(LDFLAGS) -o $(KERNEL) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/isr_wrap.o: kernel/isr_wrap.asm
	nasm -f elf $< -o $@

grubmb.o: grubmb.asm
	nasm -f elf $< -o $@

clean:
	rm -f *~
	rm -f kernel/*.o kernel/*~
	rm -f kernel/syscalls/*.o kernel/syscalls/*~
	rm -f fs/*.o fs/*~
	rm -f mm/*.o mm/*~
	rm -f net/*.o net/*~
	rm -f $(KERNEL)
