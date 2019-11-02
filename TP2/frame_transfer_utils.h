#pragma once

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

#define FLAG      0x7e
#define A_SENDER  0x03
#define A_RCVR    0x01

#define SET       0x03
#define DISC      0x0B
#define UA        0x07

#define RR_0      0x05
#define RR_1      0x85
#define REJ_0     0x01
#define REJ_1     0x81

#define I_0       0x00
#define I_1       0x40

#define START_STATE     0
#define FLAG_RCV_STATE  1
#define A_RCV_STATE     2
#define C_RCV_STATE     3
#define BCC_OK_STATE    4
#define STOP_STATE      5

#define WAIT_DATA_STATE 6
#define WAIT_DATA_NOBCC_STATE 7
#define WAIT_DATA_ESC_STATE 8

#define ESC_BYTE 0x7d
#define ESC_MASK 0x20

typedef struct {
  int address;
  int c_field;
  unsigned char* bytes;
  int length;
  bool timed_out;
} frame_content;

typedef struct {
	int sentFrames;
	int receivedFrames;
  int noTimeouts;
	int noRR;
	int noREJ;
  double timeSpent;
} Statistics;


void write_control_frame(int fd, int address, int c_field);
void write_frame(int fd, frame_content content);

frame_content read_frame(int fd, int expected_address, int * expected_cs, int expected_cs_size);
frame_content read_frame_timeout(int fd, int expected_address, int * expected_cs, int expected_cs_size, int timeout_s);
bool read_ack_frame(int fd, int timeout_s, bool ns);
bool verify_bcc(unsigned char bytes[], int length);