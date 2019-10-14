#include "protocol_interface.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */

struct global_vars {
    int type;
    struct termios previous_tio;
};

static struct global_vars globals;

int llopen(int port, int type) {
    struct termios oldtio, newtio;

    char path[11] = "/dev/pts/?";
    switch (port) {
        // eu duvido que seja isto que os COMs sejam mas olha
        case COM1: path[9] = '0'; break;
        case COM2: path[9] = '1'; break;
        case COM3: path[9] = '2'; break;
        case COM4: path[9] = '3'; break;    
        default: printf("Unknown port: %d\n", port); return -1;
    }


    /*
        Open serial port device for reading and writing and not as controlling tty
        because we don't want to get killed if linenoise sends CTRL-C.
    */

    int fd = open(path, O_RDWR | O_NOCTTY );
    if (fd <0) {
        perror(path); return -1; 
    }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }
    globals.type = type;
    globals.previous_tio = oldtio;

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

    if (type == TRANSMITTER) {
        for (int i = 0; i < MAX_ATTEMPTS; i++) {
            write_control_frame(fd, A_SENDER, SET);
            if(!read_frame_timeout(fd, A_SENDER, UA, TIMEOUT_SECS).timed_out) {// sucesso
                printf("Successfully connected to receiver\n");
                return fd;
            }
        }
        printf("Connection timed out\n");
        return -1;
    }
    else if (type == RECEIVER) {
        read_frame(fd, A_SENDER, SET);
        write_control_frame(fd, A_SENDER, UA);
        printf("Successfully connected to transmitter\n");
        return fd;
    }
    else {
        printf("type must be %d or %d \n", TRANSMITTER, RECEIVER);
        return -1;
    }

}

int llclose(int fd) {
    // Arranjar maneira fiavel e nao jabarda de guardar oldtio se possivel <-- nao foi possivel
    
    if (tcsetattr(fd, TCSANOW, &globals.previous_tio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    if (globals.type == TRANSMITTER) {
        for (int i = 0; i < MAX_ATTEMPTS; i++) {
            write_control_frame(fd, A_SENDER, DISC);
            if(!read_frame_timeout(fd, A_RCVR, DISC, TIMEOUT_SECS).timed_out) {
                write_control_frame(fd, A_RCVR, UA);
                printf("Successfully disconnected from receiver\n");
                close(fd);
                return 0;
            }
        }
        printf("Connection timed out\n"); // Disconnection timed out xd
        return -1;
    }
    else if (globals.type == RECEIVER) {
        read_frame(fd, A_SENDER, DISC);
        write_control_frame(fd, A_RCVR, DISC);
        read_frame(fd, A_RCVR, UA);
        printf("Successfully disconnected from transmitter\n");
        close(fd);
        return 0;
    }
    else {
        printf("type must be %d or %d \n", TRANSMITTER, RECEIVER);
        return -1;
    }}