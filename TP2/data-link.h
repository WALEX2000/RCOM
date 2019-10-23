#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "frame_transfer_utils.h"

#define TIMEOUT_SECS 3
#define MAX_ATTEMPTS 50

#define TRANSMITTER 0
#define RECEIVER 1

#define COM0 0
#define COM1 1
#define COM2 2
#define COM3 3
#define COM4 4

int llopen(int port, int type);
int llclose(int fd);

int llwrite(int fd, unsigned char * buffer, int length);
int llread(int fd, unsigned char * buffer);