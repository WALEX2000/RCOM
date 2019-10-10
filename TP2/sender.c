/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

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

volatile int STOP=FALSE;

int alarm_set=0, conta=1;
struct termios oldtio,newtio;
int fd,c, res;

int read_control_frame(int fd, int control_field, bool alarme) {
    if(alarme && !alarm_set){
        alarm(3);                 // activa alarme de 3s
        alarm_set=1;
    } 
    unsigned char byte;
    int state = START_STATE;
    unsigned char a;
    unsigned char c;

    while (state != STOP_STATE) {
      int nbytes = read(fd,&byte,1);   /* returns after 1 chars have been input */
      if(!alarm_set && alarme) return 1;
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

void atende()                   // atende alarme
{
  printf("alarme # %d\n", conta);
  alarm_set=0;
  conta++;
  newtio.c_cc[VMIN]     = 0; 
  if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
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

int main(int argc, char** argv)
{
    (void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao

    char buf[255];
    int i, sum = 0, speed = 0;
    
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
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    sleep(1);

    // Estabelecimento
    
  while(conta < 4){
  	write_control_frame(fd, SET);

    if(read_control_frame(fd, UA, true) == 0) break;
    
    newtio.c_cc[VMIN]     = 1;
    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    } 
  }

  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
   }
    

  close(fd);
  return 0;
}
