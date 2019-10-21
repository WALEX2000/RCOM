#include <stdio.h>
#include <stdlib.h>

#include "data-link.h"
#include "application.h"

int main(int argc, char*argv[]) {
    if (argc < 3) {
      printf("Usage:\tserialcom COM type filepath\n");
      exit(1);
    }

    int com = atoi(argv[1]);
    char* typeStr = argv[2];
    int type = 0;
    char* filepath = argv[3];

    if(strcasecmp(typeStr, "sender") == 0) type = TRANSMITTER;
    else if (strcasecmp(typeStr, "receiver") == 0) type = RECEIVER;
    else {
        printf("%s is not a valid type\n", typeStr);
        exit(1);
    }

    int fd = llopen(com, type);
    if (fd == -1) {
        printf("Error opening serial port\n");
        exit(1);
    }

    if(type == TRANSMITTER) {
        if(sendFile(fd, filepath) != 0) printf("Error sending file!\n");
    }
    else receiveFile(fd, filepath);
    
    if (llclose(fd) != 0) {
        printf("Error closing serial port\n");
        exit(1);
    }

    return 0;
}

int sendFile(int fd, char* inputFileName) {
    FILE* file = fopen(inputFileName, "r");
    if(file == NULL) {
        printf("Couldn't find file: %s\n", inputFileName);
        return 1;
    }
    
    fseek(file, 0L, SEEK_END);
    int fileSize = ftell(file);
    rewind(file);
    
    if(sendControlPacket(fd, CONTROL_START, inputFileName, fileSize) != 0) {
        printf("Error sending Control Packet\n");
        return 1;
    }

    sendFileData(fd, file, fileSize);

    return 0;
}

int sendControlPacket(int fd, ControlPacketType type, char* fileName, int fileSize) {
    int fileNameSize = strlen(fileName);

    int fileSizeBufferSize = 0;
    int temp = fileSize;
    do {
        fileSizeBufferSize++;
        temp /= 256;
    } while (temp > 0);

    int controlPacketSize = 5 + fileNameSize + fileSizeBufferSize;
    unsigned char* controlPacket = malloc(controlPacketSize);

    controlPacket[0] = CONTROL_START;
    controlPacket[1] = FILE_SIZE;
    controlPacket[2] = fileSizeBufferSize;

    for(int i = 0; i < fileSizeBufferSize; i++) {
        unsigned char byte = fileSize & 0xff;
        fileSize = fileSize >> 8;
        controlPacket[i+3] = byte;
    }

    controlPacket[fileSizeBufferSize+3] = FILE_NAME;
    controlPacket[fileSizeBufferSize+4] = fileNameSize;

    for(int i = 0; i < fileNameSize; i++) {
        controlPacket[fileSizeBufferSize+5+i] = fileName[i];
    }

    for(int i = 0; i < 5 + fileNameSize + fileSizeBufferSize; i++) {
        printf("%d: %X\n", i, controlPacket[i]);
    }

    int written = llwrite(fd, controlPacket, controlPacketSize);
    if(written != controlPacketSize) return 1;

    return 0;
}

int sendFileData(int fd, FILE* file, int fileSize) {
    const int nPackets = 10;
    const int nBytesPerPacket = fileSize/nPackets;
    for(unsigned int i = 0; i < nPackets; i++) {
        unsigned char* dataPacket = malloc(5);

        dataPacket[0] = DATA;
        dataPacket[0] = i;
        //enviar o numero de Tetos
        //enviar os tetos
    }
    return 0;
}

int receiveFile(int fd, char* outputFileName) {
    unsigned char* readControl = malloc(100);
    int nRead = llread(fd, readControl);

    for(int i = 0; i < nRead; i++) {
        printf("%d: %X\n", i, readControl[i]);
    }
    return 0;
}
