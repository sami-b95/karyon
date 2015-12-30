DASH_VER=0.5.7

include ../envtest

all:
	@if [ ! -e dash-$(DASH_VER) ]; then \
		@echo "Extracting sources..."; \
		tar xfv dash-$(DASH_VER).tar.gz; \
	fi

	@echo "Creating build directory..."
	mkdir -p build-dash

	@echo "Building..."
	cd build-dash; \
	if [ ! -e config.log ]; then \
		../dash-$(DASH_VER)/configure --host=i586-pc-karyon \
		--prefix=$(KARYON_SYSROOT); \
	fi; \
	echo "#undef _GNU_SOURCE" >> config.h; \
	make; \
	make install

clean:
	-cd build-dash; \
	make uninstall
	rm -rf build-dash
