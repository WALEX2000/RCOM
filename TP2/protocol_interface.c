#include "protocol_interface.h"

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

    char path[11] = "/dev/pts/?";
    switch (port) {
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

    if (type == TRANSMITTER) {
        for (int i = 0; i < MAX_ATTEMPTS; i++) {
            write_control_frame(fd, A_SENDER, SET);
            int * expected_cs = malloc(1);
            expected_cs[0] = UA;
            if(!read_frame_timeout(fd, A_SENDER, expected_cs, 1, TIMEOUT_SECS).timed_out) {// sucesso
                printf("Successfully connected to receiver\n");
                free(expected_cs);
                return fd;
            }
            free(expected_cs);
        }
        printf("Connection timed out\n");
        return -1;
    }
    else if (type == RECEIVER) {
        int * expected_cs = malloc(1);
        expected_cs[0] = SET;
        read_frame(fd, A_SENDER, expected_cs, 1);
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

    if (globals.type == TRANSMITTER) {
        for (int i = 0; i < MAX_ATTEMPTS; i++) {
            write_control_frame(fd, A_SENDER, DISC);
            int * expected_cs = malloc(1);
            expected_cs[0] = DISC;
            if(!read_frame_timeout(fd, A_RCVR, expected_cs, 1, TIMEOUT_SECS).timed_out) {
                free(expected_cs);
                write_control_frame(fd, A_RCVR, UA);
                close(fd);
                globals.opened = false;
                printf("Successfully disconnected from receiver\n");
                return 0;
            }
            free(expected_cs);
        }
        printf("Disconnection timed out\n");
        return -1;
    }
    else if (globals.type == RECEIVER) {
        int * expected_cs = malloc(1);
        expected_cs[0] = DISC;
        read_frame(fd, A_SENDER, expected_cs, 1);
        write_control_frame(fd, A_RCVR, DISC);
        expected_cs[0] = UA;
        read_frame(fd, A_RCVR, expected_cs, 1);
        close(fd);
        globals.opened = false;
        printf("Successfully disconnected from transmitter\n");
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
        if(ack) {
            nr = !nr; //dps vemos
            ns = !ns;
            return length;
        }
    }
    return -1;
}

int llread(int fd, unsigned char * buffer) {
    int * expected_cs = malloc(3);
    expected_cs[0] = I_0;
    expected_cs[1] = I_1;
    expected_cs[2] = DISC;
    frame_content frame = read_frame(fd, A_SENDER, expected_cs, 3); //Temos de arranjar manneira de aceitar estes 3 c_fields
    free(expected_cs);

    //now check if read was successful
    if(frame.bytes == NULL && frame.c_field != DISC) {//if NACK
        int c_field;
        if (frame.c_field == I_0) c_field = REJ_0;
        else if(frame.c_field == I_1) c_field = REJ_1;

        write_control_frame(fd, A_SENDER, c_field);
        return -1;
    }
    else if(frame.bytes != NULL) { //if ACK
        int c_field;
        if (frame.c_field == I_0) {
            c_field = RR_1;
            nr = true; //??? ja estou a ficar confuso
        }
        else if(frame.c_field == I_1) {
             c_field = RR_0;
             nr = false;  //??? mesma confusão que a de cima
        }
        else return -1; //not supposed to happen?

        write_control_frame(fd, A_SENDER, c_field);
        buffer = frame.bytes;
        return frame.length;
    }
    else {//if DISC
        //Dunno how to Disconnect (só fazer exit(0)?)
        return -1; //not sure if correct return
    }
}
