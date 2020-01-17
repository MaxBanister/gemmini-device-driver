obj-$(CONFIG_GEMMINI) := gemmini_driver.o

KDIR = /lib/modules/$(shell uname -r)/build

CC := $(CROSS_COMPILE)gcc

all:
	$(MAKE) -C $(KDIR) M=$(shell pwd) modules

clean:
	$(MAKE) -C $(KDIR) M=$(shell pwd) clean
