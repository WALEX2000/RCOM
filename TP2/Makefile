CC=gcc
CFLAGS=-I. -D_REENTRANT -lpthread
DEPS = frame_transfer_utils.h
OBJ_SENDER = frame_transfer_utils.c sender.c
OBJ_RECEIVER = frame_transfer_utils.c receiver.c

all: sender receiver

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sender: $(OBJ_SENDER)
	$(CC) -Wall -o $@ $^ $(CFLAGS)

receiver: $(OBJ_RECEIVER)
	$(CC) -Wall -o $@ $^ $(CFLAGS)

clean: rm -f sender receiver