#include <stdio.h>
#include <stdlib.h>

#include "data-link.h"
#include "application.h"

int main(int argc, char*argv[]) {
    if (argc < 3) {
      printf("Usage:\tserialcom COM type filepath");
      exit(1);
    }

    int com = atoi(argv[1]);
    char* typeStr = argv[2];
    int type = 0;
    char* filepath = argv[3];

    if(strcmp(typeStr, "sender") == 0) type = TRANSMITTER;
    else if (strcmp(typeStr, "receiver") == 0) type = RECEIVER;
    else {
        printf("%s is not a valid type\n", typeStr);
        exit(1);
    }

    int fd = 1;//llopen(com, type);
    if (fd == -1) {
        printf("Error opening serial port\n");
        exit(1);
    }

    if(type == TRANSMITTER) sendFile(fd, filepath);
    else receiveFile(fd, filepath);

    /*if (llclose(fd) != 0) {
        printf("Error closing serial port\n");
        exit(1);
    }*/

    return 0;
}

int sendFile(int fd, char* inputFileName) {
    FILE* file = fopen(inputFileName, "r");
    //if(file == NULL) return 1;

    //fseek(file, 0L, SEEK_END);
    int fileSize = 273;//ftell(file);
    //rewind(file);

    sendControlPacket(fd, CONTROL_START, inputFileName, fileSize);

    return 0;
}

int sendControlPacket(int fd, ControlPacketType type, char* fileName, int fileSize) {
    int fileNameSize = strlen(fileName);

    int fileSizeBufferSize = 0;
    int temp = fileSize;
    do {
        printf("%d\n", temp);
        fileSizeBufferSize++;
        temp /= 256;
    } while (temp > 0);

    int* controlPacket = malloc(5 + fileNameSize + fileSizeBufferSize);

    controlPacket[0] = CONTROL_START;
    controlPacket[1] = FILE_SIZE;
    controlPacket[2] = fileSizeBufferSize;

    char* fileSizeString = malloc(fileSizeBufferSize);
    sprintf(fileSizeString, "%x", fileSize);
    for(int i = 0; i < fileSizeBufferSize; i++) {
        controlPacket[i+3] = fileSizeString[i];
    }

    controlPacket[fileSizeBufferSize+3] = FILE_NAME;
    controlPacket[fileSizeBufferSize+4] = fileNameSize;

    for(int i = 0; i < fileNameSize; i++) {
        controlPacket[fileSizeBufferSize+5+i] = fileName[i];
    }

    for(int i = 0; i < 5 + fileNameSize + fileSizeBufferSize; i++) {
        printf("%d: %X\n", i, controlPacket[i]);
    }

    return 0;
}

int receiveFile(int fd, char* outputFileName) {
    return 0;
}
