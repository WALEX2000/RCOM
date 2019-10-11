#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

#define CTRL_S BIT(6)
#define CTRL_R BIT(7)

#define FLAG        0x7e
#define A_SENDER    0x03
#define A_RCVR      0x01
#define SET         0x03
#define DISC        0x0B
#define UA          0x07
#define RR          0x05
#define REJ         0x01
#define I_CTRL      0x00

#define START_STATE     0
#define FLAG_RCV_STATE  1
#define A_RCV_STATE     2
#define C_RCV_STATE     3
#define BCC_OK_STATE    4
#define STOP_STATE      5

#define ESC_BYTE 0x7d
#define ESC_MASK 0x20

struct data_frame_content {
  unsigned char* bytes;
  int length;
};


int read_control_frame(int fd, int control_field, int address, bool enable_timeout, int timeout);

int write_control_frame(int fd, int control_field, int address);

void write_data_frame(int fd, struct data_frame_content content);

struct data_frame_content read_data_frame(int fd);
