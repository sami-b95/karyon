LESS_VER=451
BUILD=$(shell gcc -dumpmachine)

include ../envtest

all:
	if [ ! -e less-$(LESS_VER) ]; then \
		@echo "Extracting sources..."; \
		tar xfv less-$(LESS_VER).tar.gz; \
	fi

	mkdir -p build-less

	@echo "Building..."
	cd build-less; \
	if [ ! -e config.log ]; then \
		LIBS="-lncurses -ltinfo" ../less-$(LESS_VER)/configure \
		--prefix=$(KARYON_SYSROOT) --host=i586-pc-karyon \
		--build=$(BUILD); \
	fi; \
	make; \
	make install

clean:
	-cd build-less; \
	make uninstall
	rm -rf build-less
