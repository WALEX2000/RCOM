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

    unsigned char buffer1[] = "ola";
    unsigned char buffer2[] = "xd";
    unsigned char buffer3[] = "aaaaa";

    llwrite(fd, buffer1, 3);
    printf("Sending %s \n", buffer1);

    llwrite(fd, buffer2, 2);    
    printf("Sending %s \n", buffer2);

    llwrite(fd, buffer3, 5);
    printf("Sending %s \n", buffer3);

    if (llclose(fd) != 0) {
        printf("Error closing serial port\n");
        exit(1);
    }

    return 0;
}
