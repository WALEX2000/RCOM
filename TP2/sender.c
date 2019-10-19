/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "frame_transfer_utils.h"
#include "protocol_interface.h"

int main(int argc, char **argv)
{


    int fd = llopen(COM1, TRANSMITTER);
    if (fd == -1) {
        printf("Error opening serial port\n");
        exit(1);
    }

    unsigned char * buffer = malloc(3);
    buffer[0] = 'o';
    buffer[1] = 'l';
    buffer[2] = 'a';
    llwrite(fd, buffer, 3);

    buffer = malloc(2);
    buffer[0] = 'x';
    buffer[1] = 'd';
    llwrite(fd, buffer, 2);

    buffer = malloc(5);
    buffer[0] = 'a';
    buffer[1] = 'a';
    buffer[2] = 'a';
    buffer[3] = 'a';
    buffer[4] = 'a';
    llwrite(fd, buffer, 5);

    if (llclose(fd) != 0) {
        printf("Error closing serial port\n");
        exit(1);
    }

    return 0;
}
