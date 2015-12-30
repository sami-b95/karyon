include envtest

all:
	@echo "Creating system and toolchain root..."
	mkdir -p $(KARYON_SYSROOT) $(KARYON_TCHAIN_ROOT)
	@echo "Making toolchain..."
	make -C tchain
	@export PATH=$(KARYON_TCHAIN_ROOT)/bin:$(PATH); \
	echo "Making Dash..."; \
	make -C dash; \
	echo "Making Coreutils..."; \
	make -C coreutils; \
	echo "Making Ncurses..."; \
	make -C ncurses; \
	echo "Making Nano..."; \
	make -C nano; \
	echo "Making Links..."; \
	make -C links; \
	echo "Making Less..."; \
	make -C less; \
	echo "Making Netconf..."; \
	make -C netconf

clean:
	@echo "Cleaning Netconf..."
	make -C netconf clean
	@echo "Cleaning Less..."
	make -C less clean
	@echo "Cleaning Links..."
	make -C links clean
	@echo "Cleaning Nano..."
	make -C nano clean
	@echo "Cleaning Ncurses..."
	make -C ncurses clean
	@echo "Cleaning Coreutils..."
	make -C coreutils clean
	@echo "Cleaning Dash..."
	make -C dash clean
	@echo "Cleaning toolchain..."
	make -C tchain clean
