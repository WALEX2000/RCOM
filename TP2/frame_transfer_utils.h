#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

#define FLAG 0x07
#define A 0x03
#define SET 0x03
#define DISC 0x0B
#define UA 0x07
#define RR 0x05
#define REJ 0x01

#define START_STATE 0
#define FLAG_RCV_STATE 1
#define A_RCV_STATE 2
#define C_RCV_STATE 3
#define BCC_OK_STATE 4
#define STOP_STATE 5

int read_control_frame(int fd, int control_field, bool enable_timeout, int timeout);

int write_control_frame(int fd, int control_field);