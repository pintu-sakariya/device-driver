CFLAGS_chdrv.o := -DDEBUG
ifneq ($(KERNELRELEASE),)
obj-m   := chdrv.o
else
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	$(CC) -o test_app test_app.c
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm -r test_app
endif
