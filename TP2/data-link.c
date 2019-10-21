#include "data-link.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */

struct global_vars {
    int type;
    struct termios previous_tio;
    bool opened;
};

static bool ns = 0, nr = 0;
static struct global_vars globals;

int llopen(int port, int type) {
    struct termios oldtio, newtio;

    const int pathLen = 12;
    char path[pathLen] = "/dev/ttys00?";
    //const int pathLen = 10;
    //char path[10] = "/dev/pts/?";
    switch (port) {
        case COM0: path[pathLen-1] = '0'; break;
        case COM1: path[pathLen-1] = '1'; break;
        case COM2: path[pathLen-1] = '2'; break;
        case COM3: path[pathLen-1] = '3'; break;
        case COM4: path[pathLen-1] = '4'; break;    
        default: printf("Unknown port: %d\n", port); return -1;
    }
    //printf("PATH: %s\n", path);

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
    globals.opened = true;

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

    int expected_set[1] = { SET };
    int expected_ua[1] = { UA };

    if (type == TRANSMITTER) {
        for (int i = 0; i < MAX_ATTEMPTS; i++) {
            write_control_frame(fd, A_SENDER, SET);
            if(!read_frame_timeout(fd, A_SENDER, expected_ua, 1, TIMEOUT_SECS).timed_out) {// sucesso
                printf("Successfully connected to receiver\n");
                return fd;
            }
        }
        printf("Connection timed out\n");
        return -1;
    }
    else if (type == RECEIVER) {
        read_frame(fd, A_SENDER, expected_set, 1);
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
    if(globals.opened == false) {
        printf("No connection open at the moment\n");
        return -1;
    }

    if (tcsetattr(fd, TCSANOW, &globals.previous_tio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    int expected_disc[1] = { DISC };
    int expected_ua[1] = { UA };

    if (globals.type == TRANSMITTER) {
        for (int i = 0; i < MAX_ATTEMPTS; i++) {
            write_control_frame(fd, A_SENDER, DISC);
            if(!read_frame_timeout(fd, A_RCVR, expected_disc, 1, TIMEOUT_SECS).timed_out) {
                write_control_frame(fd, A_RCVR, UA);
                close(fd);
                globals.opened = false;
                printf("Successfully disconnected from receiver\n");
                return 0;
            }
        }
        printf("Disconnection timed out\n");
        return -1;
    }
    else if (globals.type == RECEIVER) {
        read_frame(fd, A_SENDER, expected_disc, 1);
        for (int i = 0; i < MAX_ATTEMPTS; i++) {
            write_control_frame(fd, A_RCVR, DISC);
            if(!read_frame_timeout(fd, A_RCVR, expected_ua, 1, TIMEOUT_SECS).timed_out) {
                close(fd);
                globals.opened = false;
                printf("Successfully disconnected from transmitter\n");
                return 0;
            }
        }

        printf("Disconnection timed out\n");
        return 0;
    }
    else return -1;  
}

int llwrite(int fd, unsigned char * buffer, int length) {
    frame_content content;
    content.address = A_SENDER;
    content.bytes = buffer;
    content.c_field = ns? I_1 : I_0;
    content.length = length;
    content.timed_out = false;
    for(int i = 0; i < MAX_ATTEMPTS; i++) {
        write_frame(fd, content);
        bool ack = read_ack_frame(fd, TIMEOUT_SECS, ns);
        content.c_field = ns? I_1 : I_0;
        if(ack) {
            ns = !ns;
            return length;
        }
    }

    return -1;
}

int llread(int fd, unsigned char * buffer) {
    int expected_cs[3] = { I_0, I_1, DISC };
    frame_content frame;
    do {
        frame = read_frame(fd, A_SENDER, expected_cs, 3);
        if (frame.c_field == DISC) {
            // Received Disconnect
            printf("Disconnect received during llread\n");
            return -1;
        }
        else if (frame.bytes == NULL) { // if NACK
            printf("Bad bad bad gonna send nack!\n");
            int nack;
            if (frame.c_field == I_0) nack = REJ_0;
            else if (frame.c_field == I_1) nack = REJ_1;
            write_control_frame(fd, A_SENDER, nack);
            continue;
        }
        else { // if ACK
            int ack;
            if (frame.c_field == I_0) ack = RR_1;
            else if(frame.c_field == I_1) ack = RR_0;
            write_control_frame(fd, A_SENDER, ack);
        }
    } while(frame.c_field >> 6 != nr); // verifica o bit Nr para saber se ja recebeu

    nr = !nr;

    for(unsigned int i = 0; i < frame.length; i++)
        buffer[i] = frame.bytes[i];

    return frame.length;
}
