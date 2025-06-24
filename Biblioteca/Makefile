CC      := gcc
AR      := ar
CFLAGS  := -Wall -Wextra -O2 -fPIC
TARGET  := libbiblioteca.a
OBJS    := biblioteca.o
PREFIX  := /usr/local

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

biblioteca.o: biblioteca.c biblioteca.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	install -d $(PREFIX)/lib
	install -m 644 $(TARGET) $(PREFIX)/lib
	install -d $(PREFIX)/include
	install -m 644 biblioteca.h $(PREFIX)/include

.PHONY: all clean install
