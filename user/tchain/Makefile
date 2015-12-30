BINUTILS_VER=2.24
GCC_VER=4.8.2
GMP_VER=5.1.3
MPFR_VER=3.1.2
MPC_VER=1.0.1
LIBICONV_VER=1.14
NEWLIB_VER=2.0.0
NEWLIB_SUBVER=11
export PATH:=$(PATH):$(KARYON_TCHAIN_ROOT)/bin

include ../envtest

all:
	@if [ ! -e binutils-$(BINUTILS_VER) ]; then \
		echo "Extracting Binutils..."; \
		tar xfv binutils-$(BINUTILS_VER)-karyon.tar.gz; \
	fi

	@if [ ! -e gcc-$(GCC_VER) ]; then \
		echo "Extracting GCC..."; \
		tar xfv gcc-$(GCC_VER)-karyon.tar.gz; \
		tar xfv gmp-$(GMP_VER).tar.bz2; \
		tar xfv mpfr-$(MPFR_VER).tar.bz2; \
		tar xfv mpc-$(MPC_VER).tar.gz; \
		tar xfv libiconv-$(LIBICONV_VER).tar.gz; \
		cp -r gmp-$(GMP_VER) gcc-$(GCC_VER)/gmp; \
		cp -r mpfr-$(MPFR_VER) gcc-$(GCC_VER)/mpfr; \
		cp -r mpc-$(MPC_VER) gcc-$(GCC_VER)/mpc; \
		cp -r libiconv-$(LIBICONV_VER) gcc-$(GCC_VER)/libiconv; \
	fi

	@if [ ! -e newlib-$(NEWLIB_VER) ]; then \
		echo "Extracting Newlib..."; \
		tar xfv newlib-$(NEWLIB_VER).$(NEWLIB_SUBVER)-karyon.tar.gz; \
	fi

	@if [ ! -e $(KARYON_SYSROOT)/usr/include/stdio.h ]; then \
		echo "Copying headers to system root..."; \
		mkdir -p $(KARYON_SYSROOT)/usr/include; \
		cp -r newlib-$(NEWLIB_VER)/newlib/libc/include/* \
		      $(KARYON_SYSROOT)/usr/include; \
		cp -r newlib-$(NEWLIB_VER)/newlib/libc/sys/karyon/include/* \
		      $(KARYON_SYSROOT)/usr/include; \
	fi

	@echo "Creating build directories..."
	@mkdir -p build-binutils build-gcc build-newlib

	@cd build-binutils; \
	if [ ! -e config.log ]; then \
		echo "Configuring Binutils..."; \
		../binutils-$(BINUTILS_VER)/configure \
		--prefix=$(KARYON_TCHAIN_ROOT) --target=i586-pc-karyon \
		--with-sysroot=$(KARYON_SYSROOT) \
		--disable-werror; \
	fi; \
	echo "Building Binutils..."; \
	make; \
	echo "Installing Binutils..."; \
	make install
	
	@cd build-gcc; \
	if [ ! -e config.log ]; then \
		echo "Configuring GCC..."; \
		../gcc-$(GCC_VER)/configure --prefix=$(KARYON_TCHAIN_ROOT) \
		--target=i586-pc-karyon --with-sysroot=$(KARYON_SYSROOT) \
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

	@cd build-newlib; \
	if [ ! -e config.log ]; then \
		echo "Configuring Newlib..."; \
		../newlib-$(NEWLIB_VER)/configure --prefix=$(KARYON_TCHAIN_ROOT) \
		--target=i586-pc-karyon --disable-werror \
		--enable-newlib-io-pos-args --enable-newlib-io-c99-formats \
		--enable-newlib-io-long-long --enable-newlib-io-long-double; \
	fi; \
	echo "Building Newlib..."; \
	make; \
	echo "Installing Newlib..."; \
	make install; \
	echo "Installing complementary headers..."; \
	cp -r ../newlib-$(NEWLIB_VER)/newlib/libc/sys/karyon/include/* \
	      $(KARYON_TCHAIN_ROOT)/i586-pc-karyon/include

clean:
	@echo "Cleaning Binutils..."
	@rm -rf build-binutils
	@echo "Cleaning GCC..."
	@rm -rf build-gcc
	@echo "Cleaning Newlib..."
	@rm -rf $(KARYON_SYSROOT)/usr/include
	@rm -rf build-newlib  
	@rm -rf $(KARYON_TCHAIN_ROOT)/*
