CC=gcc
CFLAGS=-I. -D_REENTRANT -lpthread
DEPS = frame_transfer_utils.h data-link.h application.h
OBJ = frame_transfer_utils.c data-link.c application.c

serialcom: $(OBJ)
	$(CC) -Wall -o $@ $^ $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean: rm -f serialcom