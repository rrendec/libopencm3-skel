# See libopencm3/mk/README

DEVICE		= stm32f103c8t6
OPENCM3_DIR	= ../libopencm3
TARGET		= firmware
OBJS		+= main.o

CFLAGS		+= -Os -ggdb3
CPPFLAGS	+= -MD
LDFLAGS		+= -static -nostartfiles -specs=nano.specs -Wl,--gc-sections
LDLIBS		+= -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

include ../config.mk

include $(OPENCM3_DIR)/mk/genlink-config.mk
include $(OPENCM3_DIR)/mk/gcc-config.mk

.PHONY: all clean

all: $(TARGET).elf $(TARGET).hex

clean:
	$(Q)$(RM) -rf $(TARGET).* *.o

include $(OPENCM3_DIR)/mk/genlink-rules.mk
include $(OPENCM3_DIR)/mk/gcc-rules.mk
