COREUTILS_VER=8.23
COREUTILS_SUBVER=1

include ../envtest

all:
	@if [ ! -e coreutils-$(COREUTILS_VER) ]; then \
		@echo "Extracting sources..."; \
		tar xfv coreutils-karyon-$(COREUTILS_VER).$(COREUTILS_SUBVER).tar.gz; \
	fi

	@echo "Creating build directory..."
	mkdir -p build-coreutils

	@echo "Building..."
	cd build-coreutils; \
	if [ ! -e config.log ]; then \
		../coreutils-$(COREUTILS_VER)/configure \
		--prefix=$(KARYON_SYSROOT) --host=i586-pc-karyon \
		--disable-nls --enable-no-install-program=stdbuf; \
	fi; \
	make; \
	make install

clean:
	-cd build-coreutils; \
	make uninstall
	rm -rf build-coreutils
