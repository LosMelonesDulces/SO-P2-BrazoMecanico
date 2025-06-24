# Makefile para compilar

obj-m += robotic_hand_driver.o

# Variable que apunta al directorio de los fuentes del kernel actual
KDIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean