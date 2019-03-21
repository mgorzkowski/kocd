obj-m += kocd.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	sudo insmod kocd.ko size=32
	sudo chmod a+rw /dev/kocd

reinstall:
	sudo rmmod kocd
	sudo insmod kocd.ko size=32
	sudo chmod a+rw /dev/kocd