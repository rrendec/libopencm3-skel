include config.mk

.PHONY: all firmware libopencm3

all: firmware

# Add a hard dependency on libopencm3 to make sure that the libraries are built
# before we invoke the sub-make in firmware. Otherwise, genlink-config.mk, which
# is included from firmware/Makefile, will complain about the missing libraries.
firmware: libopencm3
	$(Q)$(MAKE) -C firmware

libopencm3:
	$(Q)$(MAKE) -C libopencm3
