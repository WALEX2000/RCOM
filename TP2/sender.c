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

    unsigned char bytes1[6] = {FLAG, FLAG, 0, 0x12, ESC_BYTE, ESC_BYTE};

    frame_content dfc1;
    dfc1.bytes = bytes1;
    dfc1.length = 6;
    dfc1.c_field = I_0;
    dfc1.address = A_SENDER;


    write_frame(fd, dfc1);

    if (llclose(fd) != 0) {
        printf("Error closing serial port\n");
        exit(1);
    }

    return 0;
}
