obj-m += tvisor.o
tvisor-objs := main.o cpu.o vmx.o ept.o vm.o util.o

ccflags-y += -g -Og -Wno-declaration-after-statement

ifdef debug
	ccflags-y += -DDEBUG
endif

PWD := $(CURDIR)

default:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

debug:
	make debug=true -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean