# VARIABLES GENERALES
CC      := gcc
AR      := ar
CFLAGS  := -Wall -Wextra -O2 -fPIC

# PARTE BIBLIOTECA
LIB_TARGET  := libbiblioteca.a
LIB_OBJS    := biblioteca.o
PREFIX      := /usr/local

# PARTE MÓDULO KERNEL
obj-m       := robotic_hand_driver.o
KDIR        := /lib/modules/$(shell uname -r)/build
PWD         := $(shell pwd)

# Objetivos por defecto
.PHONY: all library module clean install

all: library module

# ===== Biblioteca estática =====
library: $(LIB_TARGET)

$(LIB_TARGET): $(LIB_OBJS)
	$(AR) rcs $@ $^

biblioteca.o: biblioteca.c biblioteca.h
	$(CC) $(CFLAGS) -c $< -o $@

# ===== Módulo del kernel =====
module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# ===== Limpieza =====
clean: 
	# Limpia la biblioteca
	rm -f $(LIB_OBJS) $(LIB_TARGET)
	# Limpia el módulo
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# ===== Instalación de la biblioteca =====
install: library
	install -d $(PREFIX)/lib
	install -m 644 $(LIB_TARGET) $(PREFIX)/lib
	install -d $(PREFIX)/include
	install -m 644 biblioteca.h $(PREFIX)/include
