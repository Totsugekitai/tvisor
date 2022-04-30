obj-m += tvisor.o
tvisor-objs := main.o cpu.o vmx.o ept.o vm.o util.o

ccflags-y += -g -Og -Wno-declaration-after-statement

PWD := $(CURDIR)

default:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

debug:
	DEBUG=1 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean