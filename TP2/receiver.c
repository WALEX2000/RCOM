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

#include "frame_transfer_utils.h"
#include "protocol_interface.h"

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

int main(int argc, char** argv)
{
  
    int fd = llopen(COM3, RECEIVER);
    if (fd == -1) {
        printf("Error opening blablabla\n");
        exit(1);
    }

    frame_content fc = read_frame(fd, A_SENDER, I_0);

    printf("----------------\n");
    for (int i = 0; i < fc.length ; i++) {
        printf("content[%d] = %x\n", i, fc.bytes[i]);
    }

    if (llclose(fd) != 0) {
        printf("Error closing serial port\n");
        exit(1);
    }

    return 0;
}
