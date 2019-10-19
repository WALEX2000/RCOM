#pragma once

typedef enum {
	START_STATE,
  FLAG_RCV_STATE,
  A_RCV_STATE,
  C_RCV_STATE,
  BCC_OK_STATE,
  STOP_STATE,
  WAIT_DATA_STATE,
  WAIT_DATA_NOBCC_STATE,
  WAIT_DATA_ESC_STATE,
  WAIT_FLAG_STATE
} ReadState;

typedef enum {
	SET = 0x03,
  UA = 0x07,
  RR = 0x05,
  REJ = 0x01,
  DISC = 0x0B
} CommandControlField;

typedef enum {
  COMMAND,
  INFORMATION,
  INVALID
} MessageType;

typedef struct {
  MessageType type;
  int address;
  CommandControlField c_field;
  unsigned char* bytes;
  int length;
} MessagePacket;

MessagePacket read_frame(int fd, int expected_address);
MessagePacket read_frame_timeout(int fd, int expected_address, int expected_c, int timeout_s);

