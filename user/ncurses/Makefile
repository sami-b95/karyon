NCURSES_VER=5.9
NCURSES_SUBVER=1
PREFIX=$(KARYON_TCHAIN_ROOT)/i586-pc-karyon
BUILD=$(shell gcc -dumpmachine)

all:
	if [ ! -e ncurses-$(NCURSES_VER) ]; then \
		@echo "Extracting sources..."; \
		tar xfv \
		ncurses-karyon-$(NCURSES_VER).$(NCURSES_SUBVER).tar.gz; \
	fi

	echo "Creating build directory..."
	mkdir -p build-ncurses

	@echo "Building..."
	cd build-ncurses; \
	if [ ! -e config.log ]; then \
		../ncurses-$(NCURSES_VER)/configure --prefix=$(PREFIX) \
		--host=i586-pc-karyon --build=$(BUILD) \
		--disable-database --with-fallbacks=vt100 --without-cxx \
		--with-termlib --enable-overwrite; \
	fi; \
	echo "Building ncurses..."; \
	make; \
	echo "Installing ncurses..."; \
	make install

clean:
	-cd build-ncurses; \
	make uninstall
	rm -rf build-ncurses
