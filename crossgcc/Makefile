BINUTILS_VER=2.24
GCC_VER=4.8.2
GMP_VER=5.1.3
MPFR_VER=3.1.2
MPC_VER=1.0.1
LIBICONV_VER=1.14
export TARGET=i686-elf

ifndef I686_ELF_GCC_ROOT
$(error Please define I686_ELF_GCC_ROOT)
endif

ifndef I686_ELF_GCC_ROOT
$(error Please define I686_ELF_GCC_ROOT)
endif

all:
	mkdir -p $(I686_ELF_GCC_ROOT)
	if [ ! -e binutils-$(BINUTILS_VER) ]; then \
		echo "Extracting Binutils..."; \
		tar xfv binutils-$(BINUTILS_VER).tar.gz; \
	fi

	if [ ! -e gcc-$(GCC_VER) ]; then \
		echo "Extracting GCC..."; \
		tar xfv gcc-$(GCC_VER).tar.gz; \
		tar xfv gmp-$(GMP_VER).tar.bz2; \
		tar xfv mpfr-$(MPFR_VER).tar.bz2; \
		tar xfv mpc-$(MPC_VER).tar.gz; \
		tar xfv libiconv-$(LIBICONV_VER).tar.gz; \
		cp -r gmp-$(GMP_VER) gcc-$(GCC_VER)/gmp; \
		cp -r mpfr-$(MPFR_VER) gcc-$(GCC_VER)/mpfr; \
		cp -r mpc-$(MPC_VER) gcc-$(GCC_VER)/mpc; \
		cp -r libiconv-$(LIBICONV_VER) gcc-$(GCC_VER)/libiconv; \
	fi

	echo "Creating build directories..."
	mkdir -p build-binutils build-gcc

	cd build-binutils; \
	if [ ! -e config.log ]; then \
		echo "Configuring Binutils..."; \
		../binutils-$(BINUTILS_VER)/configure \
		--prefix=$(I686_ELF_GCC_ROOT) --target=$(TARGET) \
		--disable-werror --disable-nls; \
	fi; \
	echo "Building Binutils..."; \
	make; \
	echo "Installing Binutils..."; \
	make install
	
	cd build-gcc; \
	if [ ! -e config.log ]; then \
		echo "Configuring GCC..."; \
		../gcc-$(GCC_VER)/configure --prefix=$(I686_ELF_GCC_ROOT) \
		--target=$(TARGET) --disable-nls \
		--enable-languages=c --disable-libssp \
		--disable-werror; \
	fi; \
	echo "Building GCC..."; \
	make all-gcc; \
	echo "Installing GCC..."; \
	make install-gcc; \
	echo "Building LIBGCC..."; \
	make all-target-libgcc; \
	echo "Installing LIBGCC..."; \
	make install-target-libgcc

clean:
	echo "Cleaning Binutils..."
	rm -rf build-binutils
	echo "Cleaning GCC..."
	rm -rf build-gcc
	rm -rf $(I686_ELF_GCC_ROOT)/*
