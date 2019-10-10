/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

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

volatile int STOP=FALSE;

int flag=1, conta=1;
struct termios oldtio,newtio;
int fd,c, res;

void atende()                   // atende alarme
{
  printf("alarme # %d\n", conta);
  flag=1;
  conta++;
  newtio.c_cc[VMIN]     = 0; 
  if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
}

int main(int argc, char** argv)
{
    (void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao

    char buf[255];
    int i, sum = 0, speed = 0;
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
          (strcmp("/dev/ttyS1", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS4", argv[1])!=0) )) {
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
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */



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

    char BCC = A^SET;
    char info[5] = {FLAG, A, SET, BCC, FLAG};
    
  while(conta < 4){
  	for(int i = 0; i < 5; i++) {
      		res = write(fd,&info[i],1);

     	 	printf("written: %02x\n", info[i]);
    	}

    if(flag){
<<<<<<< HEAD
      alarm(3);                 // activa alarme de 3s
      flag=0;
    }

    for(int i = 0; i < 5; i++) {
      res = write(fd,&info[i],1);

      printf("written: %02x\n", info[i]);
    }

    char echo[255];

    read(fd, echo, 5);
    printf("Echoing message: %s\n", echo);

    // 
    
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
=======
        alarm(3);                 // activa alarme de 3s
        flag=0;
    } 

    char echo[255];
    int bytes_lidos = read(fd, echo, 5);
    if(bytes_lidos == 5) {
    	printf("Echoing message: %02x\n", echo);
    }
    newtio.c_cc[VMIN]     = 5;
    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
>>>>>>> e4ad0e945c3ea2fe4c2fb16ef26825d3b2a6ec30
      perror("tcsetattr");
      exit(-1);
    } 

    if(!flag) break;
  }

  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
   }
    

  close(fd);
  return 0;
}
