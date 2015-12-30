# Karyon

## Introducing the project

Karyon is a minimalist Unix-like operating system mainly written in C, targetted to x86 architectures. It aims at giving an insight into the general concepts of operating systems while keeping implementation as trivial as possible. Though the algorithms it uses are mostly simplistic, the code may serve as a convenient place for experimenting more sophisticated implementation strategies, be it with task scheduling, disk block caching or TCP packets processing. It comes complete with a dedicated C toolchain (including a standard library based on Newlib), making software porting relatively easy.

## General instructions

This is just a quick overview. For further details, please refer to the appropriate section below. The procedure described is valid for most Unix/Linux systems but can probably be easily adapted for other platforms.

The repository includes a ready-to-use version, containing both compiled kernel and compiled user programs.

If you want to compile by your own means, you may chose to build either the kernel or the user programs or both.

## Running the kernel

To run the OS, you need:
- The floppy disk image containing Grub and the kernel executable.
- The hard disk image containing the Karyon filesystem.

If you use the precompiled version (in `readytouse` subdirectory), they are respectively named `floppy` and `disk.img`. So for instance, if you use **qemu**, the command line would be:
```
qemu-system-i386 -boot a -fda floppy -hda disk.img
```
(replace `qemu-system-i386` by the name of the appropriate **qemu** binary installed on your machine, which might differ).

## Building the kernel

To build the kernel properly from scratch, you need a cross-compiler. To build the cross compiler:
- `cd` to `crossgcc` subdirectory.
- Set the `I686_ELF_GCC_ROOT `environment variable to the directory you want to install the compiler in.
- run `make`.

Once you have installed the cross compiler, do not forget to update your path:
```
export PATH=$I686_ELF_GCC_ROOT/bin:$PATH
```

Finally, switch to the `kern` subdirectory and simply run `make`.

## Installing the kernel

Simply mount the floppy disk image `floppy` (root of the repository) where you want and copy the file `kern/kernel` (it should have appeared after kernel compilation) to the root of the filesystem. The floppy is formatted to Ext2, so make sure you have the proper driver installed. On most Linux distros, you would run the commands (from the root of the workspace, and supposing you want to mount `floppy` to `/mnt`):
```
mount floppy /mnt
cp kern/kernel /mnt
umount /mnt
```

## Building the toolchain and the user programs

- First, set the `KARYON_TCHAIN_ROOT` and the `KARYON_SYSROOT` environment variables: the former is the installation directory of the toolchain, while the latter will be the root of Karyon filesystem.
- cd to `user`.
- `make`.

If you want to compile your own programs, you'll have to add `$KARYON_TCHAIN_ROOT/bin` to your path. The platform-specific GCC is `i586-pc-karyon-gcc`, the platform-specific LD is `i586-pc-karyon-ld`, etc.

## Installing the user programs

First, create a large empty file (several hundreds MB to be comfortable). Then, format this disk image file to Ext2, with inode size set to 128. Finally, mount the freshly created disk image and copy the content of `$KARYON_SYSROOT` to the root of the filesystem. On most Linux distros, the following commands will do:
```
dd if=/dev/zero of=disk.img bs=512 count=524288
mke2fs -I 128 disk.img
mount disk.img /mnt
cp -r $KARYON_SYSROOT/* /mnt/
umount /mnt
```
