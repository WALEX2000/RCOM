CC=gcc
CFLAGS=-I. -D_REENTRANT -lpthread
DEPS = url.h ftp.h
OBJ = download.c url.c ftp.c

clientDL: $(OBJ)
	$(CC) -Wall -o $@ $^ $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean: 
	rm -f clientDL