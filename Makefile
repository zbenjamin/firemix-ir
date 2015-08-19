CC=gcc
CFLAGS=
LDFLAGS=-lrt
DEPS=

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

flirmem: flirmem.o
	gcc -o flirmem flirmem.o $(LDFLAGS)
