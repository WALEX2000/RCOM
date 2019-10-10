/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x07
#define A 0x03
#define SET 0x03
#define UA 0x07


#define START_STATE 0
#define FLAG_RCV_STATE 1
#define A_RCV_STATE 2
#define C_RCV_STATE 3
#define BCC_OK_STATE 4
#define STOP_STATE 5

int read_control_frame(int fd, int control_field) {
    unsigned char byte;
    int state = START_STATE;
    unsigned char a;
    unsigned char c;

    while (state != STOP_STATE) {
      int nbytes = read(fd,&byte,1);   /* returns after 1 chars have been input */
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
          if (a ^ c == byte)
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

    return 0;
}

int write_control_frame(int fd, int control_field) {
    unsigned char flag =  FLAG;
    a = A;
    c = control_field;
    unsigned char bcc =  a ^ c;
    write(fd, &flag, 1);
    write(fd, &a, 1);
    write(fd, &c, 1);
    write(fd, &bcc, 1);
    write(fd, &flag, 1);
    return 0;
}

int main(int argc, char** argv)
{
    int fd;
    struct termios oldtio,newtio;

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) &&
          (strcmp("/dev/ttys001", argv[1])!=0) &&
          (strcmp("/dev/ttys002", argv[1])!=0))) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    read_control_frame(fd, SET);
    write_control_frame(fd, UA);
    printf("Vou terminar\n");

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
