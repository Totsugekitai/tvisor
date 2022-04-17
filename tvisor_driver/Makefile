obj-m += tvisor.o
tvisor-objs := tvisor_main.o cpu_asm.o cpu.o page_table.o

PWD := $(CURDIR)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean