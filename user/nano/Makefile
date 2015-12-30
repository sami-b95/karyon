include ../envtest

NANO_VER=2.2.6
NANO_SUBVER=1
BUILD=$(shell gcc -dumpmachine)

all:
	if [ ! -e nano-$(NANO_VER) ]; then \
		@echo "Extracting sources..."; \
		tar xfv nano-karyon-$(NANO_VER).$(NANO_SUBVER).tar.gz; \
	fi

	mkdir -p build-nano

	@echo "Building..."
	cd build-nano; \
	if [ ! -e config.log ]; then \
		LIBS="-lncurses -ltinfo" ../nano-$(NANO_VER)/configure \
		--prefix=$(KARYON_SYSROOT) --host=i586-pc-karyon \
		--build=$(BUILD) --enable-tiny; \
	fi; \
	make; \
	make install

clean:
	-cd build-nano; \
	make uninstall
	rm -rf build-nano
