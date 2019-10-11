
#include "frame_transfer_utils.h"

static bool alarm_rang = false;
static int alarm_num = 1;
static int curr_fd_read;

static int fd_set_blocking(int fd, bool blocking) {
    /* Save the current flags */
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return 0;

    if (blocking)
        flags &= ~O_NONBLOCK;
    else flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) != -1;
}

static void alarm_handler() {                // atende alarme
    printf("alarme # %d\n", alarm_num);
    alarm_rang = true;
    alarm_num++;
    fd_set_blocking(curr_fd_read, false);   
}

int read_control_frame(int fd, int control_field, int address, bool enable_timeout, int timeout) {
    if(enable_timeout){
        signal(SIGALRM, alarm_handler);  // instala  rotina que atende interrupcao
        alarm(timeout);                 // activa alarme
        alarm_rang = false;
        curr_fd_read = fd;
    } 

    int state = START_STATE;
    unsigned char byte;
    unsigned char a;
    unsigned char c;

    while (state != STOP_STATE) {
      int nbytes = read(fd, &byte, 1);   /* returns after 1 chars have been input */
      if(enable_timeout && alarm_rang) 
        break;
      printf("Read %d bytes: %x\n", nbytes, byte);
      switch(state) {
        case START_STATE:
          if (byte == FLAG)
            state = FLAG_RCV_STATE;
          break;
        case FLAG_RCV_STATE:
          if (byte == address) {
            a = byte;
            state = A_RCV_STATE;
          }
          else if (byte != FLAG)
            state = START_STATE;
          break;
        case A_RCV_STATE:
          if (byte == control_field) {
            c = byte;
            state = C_RCV_STATE;
          }
          else if (byte == FLAG)
            state = FLAG_RCV_STATE;
          else state = START_STATE;
          break;
        case C_RCV_STATE:
          if ((a ^ c) == byte)
            state = BCC_OK_STATE;
          else if (byte == FLAG)
            state = FLAG_RCV_STATE;
          else state = START_STATE;
          break;
        case BCC_OK_STATE:
          if (byte == FLAG)
            state = STOP_STATE;
          else state = START_STATE;
          break;   
      }
    }

    if (enable_timeout && alarm_rang) {
      fd_set_blocking(fd, true);   
      alarm_rang = false;
      return 1;
    }

    alarm(0);
    return 0;
}

int write_control_frame(int fd, int control_field, int address) {
    unsigned char flag =  FLAG;
    unsigned char a = address;
    unsigned char c = control_field;
    unsigned char bcc =  a ^ c;

    unsigned char frame[5] = {flag, a, c, bcc, flag};
    write(fd, frame, 5);
    return 0;
}

void write_data_frame(int fd, struct data_frame_content content) {
    unsigned char flag =  FLAG;
    unsigned char a = A_SENDER;
    unsigned char c = I_CTRL; // Nao sei o que se poe em N(s)
    unsigned char bcc1 =  a ^ c;

    unsigned char bcc2 =  0;
    
    unsigned char * bytes = content.bytes;
    int numbytes = content.length;

    int message_size = 6 + numbytes;
    for (int i = 0; i < numbytes; i++) {
      if (bytes[i] == FLAG || bytes[i] == ESC_BYTE)
        message_size++;
      bcc2 = bcc2 ^ bytes[i];
    }

    unsigned char* message = malloc(message_size);


    message[0] = flag;
    message[1] = a;
    message[2] = c;
    message[3] = bcc1;

    for (int i = 0, msgIndex = 4; i < numbytes; i++, msgIndex++) {
      if (bytes[i] == FLAG || bytes[i] == ESC_BYTE) {
        message[msgIndex] = ESC_BYTE;
        message[msgIndex + 1] = bytes[i] ^ ESC_MASK;
        msgIndex++;
      }
      else message[msgIndex] = bytes[i];
    }

    message[message_size - 2] = bcc2;
    message[message_size - 1] = flag;

    write(fd, message, message_size);

    for (int i = 0; i < message_size; i++) {
      printf("message[%d] = %x\n", i, message[i]);
    }

    free(message);
}

struct data_frame_content read_data_frame(int fd) {
  struct data_frame_content ret;
  ret.bytes = NULL;
  ret.length = 0;
  return ret;
}