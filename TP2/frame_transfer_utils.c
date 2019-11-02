
#include "frame_transfer_utils.h"

static bool alarm_rang = false;
static int curr_fd_read;

Statistics data_link_statistics;

static bool byteIsInArray(unsigned char byte, int *array, int arraySize)
{
  for (unsigned int i = 0; i < arraySize; i++)
  {
    if (array[i] == byte)
      return true;
  }
  return false;
}

static int fd_set_blocking(int fd, bool blocking)
{
  /* Save the current flags */
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    return 0;

  if (blocking)
    flags &= ~O_NONBLOCK;
  else
    flags |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags) != -1;
}

void alarm_handler()
{ // atende alarme
  printf("alarme tocou!\n");
  alarm_rang = true;
  fd_set_blocking(curr_fd_read, false);
}

void setup_timeout(int fd, int timeout_s)
{
  signal(SIGALRM, alarm_handler); // instala  rotina que atende interrupcao
  alarm(timeout_s);               // activa alarme
  alarm_rang = false;
  curr_fd_read = fd;
}

void disable_timeout()
{
  alarm(0);
}

bool verify_bcc(unsigned char bytes[], int length)
{
  unsigned char bcc2_check = 0;
  for (int i = 0; i < length; i++)
    bcc2_check = bcc2_check ^ bytes[i];

  return bcc2_check == 0;
}

frame_content create_frame_content()
{
  frame_content content;
  content.bytes = NULL;
  content.address = 0;
  content.length = 0;
  content.c_field = 0;
  content.timed_out = false;
  return content;
}

//static int screwupvar = 0;

void write_frame(int fd, frame_content content)
{
  data_link_statistics.sentFrames++;

  unsigned char flag = FLAG;
  unsigned char a = content.address;
  unsigned char c = content.c_field;
  unsigned char bcc = a ^ c;

  if (c == I_0 || c == I_1)
  {

    unsigned char bcc2 = 0;

    unsigned char *bytes = content.bytes;
    int numbytes = content.length;

    /*printf("BEFORE STUFFING\n");
        for(int i = 0; i < numbytes; i++) {
          printf("message[%d]: %x\n", i, bytes[i]);
        }*/

    int message_size = 6 + numbytes;
    for (int i = 0; i < numbytes; i++)
    {
      if (bytes[i] == FLAG || bytes[i] == ESC_BYTE)
        message_size++;
      bcc2 = bcc2 ^ bytes[i];
    }

    if (bcc2 == FLAG || bcc2 == ESC_BYTE) // stuffing ao bcc2
      message_size++;

    unsigned char *message = malloc(message_size);
    message[0] = flag;
    message[1] = a;
    message[2] = c;
    message[3] = bcc;

    for (int i = 0, msgIndex = 4; i < numbytes; i++, msgIndex++)
    {
      if (bytes[i] == FLAG || bytes[i] == ESC_BYTE)
      { // stuffing
        message[msgIndex] = ESC_BYTE;
        message[msgIndex + 1] = bytes[i] ^ ESC_MASK;
        msgIndex++;
      }
      else
        message[msgIndex] = bytes[i];
    }

    if (bcc2 == FLAG || bcc2 == ESC_BYTE)
    { // bcc2 stuffing
      message[message_size - 3] = ESC_BYTE;
      message[message_size - 2] = bcc2 ^ ESC_MASK;
    }
    else
      message[message_size - 2] = bcc2;

    message[message_size - 1] = flag;
    write(fd, message, message_size);

    /*printf("AFTER STUFFING\n");
        for (int i = 4; i < message_size - 2; i++) {
          printf("message[%d] = %x\n", i-4, message[i]);
        }*/

    free(message);
  }
  else
  {
    unsigned char frame[5] = {flag, a, c, bcc, flag};

    //printf("Written 5 bytes: FLAG = %x, A = %x, C = %x, BCC = %x, FLAG = %x\n", flag, a, c, bcc, flag);
    write(fd, frame, 5);
  }
}

void write_control_frame(int fd, int address, int c_field)
{
  frame_content content;
  content.bytes = NULL;
  content.address = address;
  content.length = 0;
  content.c_field = c_field;
  content.timed_out = false;
  write_frame(fd, content);
}

static frame_content read_frame_general(int fd, int expected_address, int *expected_c_values, int c_values_array_size, bool timeout_enabled)
{
  int state = START_STATE;
  unsigned char byte;
  int num_bytes_read = 0, bytesArraySize = 2;
  unsigned char *bytes = malloc(bytesArraySize);

  frame_content content = create_frame_content();

  while (state != STOP_STATE)
  {
    /* int nbytes = */ read(fd, &byte, 1); /* returns after 1 chars have been input */
    if (timeout_enabled && alarm_rang)
      break;
    //printf("Read %d bytes: %x | %d \n", nbytes, byte, state);
    switch (state)
    {
    case START_STATE:
      if (byte == FLAG)
        state = FLAG_RCV_STATE;
      break;
    case FLAG_RCV_STATE:
      if (byte == expected_address)
        state = A_RCV_STATE;
      else if (byte != FLAG)
        state = START_STATE;
      break;
    case A_RCV_STATE:
      if (byteIsInArray(byte, expected_c_values, c_values_array_size))
      {
        content.c_field = byte;
        state = C_RCV_STATE;
      }
      else if (byte == FLAG)
        state = FLAG_RCV_STATE;
      else
        state = START_STATE;
      break;
    case C_RCV_STATE:
      if ((expected_address ^ content.c_field) == byte)
      {
        if (content.c_field == I_0 || content.c_field == I_1)
          state = WAIT_DATA_NOBCC_STATE;
        else
          state = BCC_OK_STATE;
      }
      else if (byte == FLAG)
        state = FLAG_RCV_STATE;
      else
        state = START_STATE;
      break;
    case WAIT_DATA_STATE:
      if (byte == FLAG)
        state = STOP_STATE;
      else if (byte == ESC_BYTE)
        state = WAIT_DATA_ESC_STATE;
      else
      {

        if (num_bytes_read + 1 > bytesArraySize)
        {
          bytesArraySize *= 2;
          bytes = realloc(bytes, bytesArraySize);
        }
        bytes[num_bytes_read] = byte;
        num_bytes_read++;
      }
      break;
    case WAIT_DATA_NOBCC_STATE:
      if (byte == FLAG)
        return content;
      else if (byte == ESC_BYTE)
        state = WAIT_DATA_ESC_STATE;
      else
      {
        if (num_bytes_read + 1 > bytesArraySize)
        {
          bytesArraySize *= 2;
          bytes = realloc(bytes, bytesArraySize);
        }
        bytes[num_bytes_read] = byte;
        num_bytes_read++;
        state = WAIT_DATA_STATE;
      }
      break;
    case WAIT_DATA_ESC_STATE:
      if (byte == (ESC_MASK ^ FLAG) || byte == (ESC_MASK ^ ESC_BYTE))
        state = WAIT_DATA_STATE;
      else
        return content;
      if (num_bytes_read + 1 > bytesArraySize)
      {
        bytesArraySize *= 2;
        bytes = realloc(bytes, bytesArraySize);
      }
      bytes[num_bytes_read] = byte ^ ESC_MASK;
      num_bytes_read++;
      break;
    case BCC_OK_STATE:
      if (byte == FLAG)
        return content;
      else
        state = START_STATE;
      break;
    default:
      return content;
    }
  }

  if (timeout_enabled && alarm_rang)
  {
    data_link_statistics.noTimeouts++;
    fd_set_blocking(fd, true);
    alarm_rang = false;
    content.timed_out = true;
    return content;
  }

  data_link_statistics.receivedFrames++;

  if (!verify_bcc(bytes, num_bytes_read))
    return content;

  content.bytes = bytes;
  content.length = num_bytes_read - 1;

  return content;
}

frame_content read_frame(int fd, int expected_address, int *expected_cs, int expected_cs_size)
{
  return read_frame_general(fd, expected_address, expected_cs, expected_cs_size, false);
}

frame_content read_frame_timeout(int fd, int expected_address, int *expected_cs, int expected_cs_size, int timeout_s)
{
  setup_timeout(fd, timeout_s);
  frame_content frame = read_frame_general(fd, expected_address, expected_cs, expected_cs_size, true);
  disable_timeout();
  return frame;
}

bool read_ack_frame(int fd, int timeout_s, bool ns)
{
  int ack, nack;
  if (ns)
  {
    ack = RR_0;
    nack = REJ_1;
  }
  else
  {
    ack = RR_1;
    nack = REJ_0;
  }

  int *expected_cs = malloc(2);
  expected_cs[0] = ack;
  expected_cs[1] = nack;
  setup_timeout(fd, timeout_s);
  frame_content ret = read_frame_general(fd, A_SENDER, expected_cs, 2, true);
  disable_timeout();
  return (ret.c_field == ack);
}
