
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

int read_control_frame(int fd, int control_field, bool enable_timeout, int timeout) {
    if(enable_timeout){
        curr_fd_read = fd;
        signal(SIGALRM, alarm_handler);  // instala  rotina que atende interrupcao
        alarm(timeout);                 // activa alarme de 3s
        alarm_rang = false;
    } 
    unsigned char byte;
    int state = START_STATE;
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
          if (byte == A) {
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
      //printf("State: %d\n", state);
    }

    if (enable_timeout && alarm_rang) {
      fd_set_blocking(fd, true);   
      alarm_rang = false;
      return 1;
    }

    alarm(0);
    return 0;
}

int write_control_frame(int fd, int control_field) {
    unsigned char flag =  FLAG;
    unsigned char a = A;
    unsigned char c = control_field;
    unsigned char bcc =  a ^ c;
    write(fd, &flag, 1);
    write(fd, &a, 1);
    write(fd, &c, 1);
    write(fd, &bcc, 1);
    write(fd, &flag, 1);
    return 0;
}