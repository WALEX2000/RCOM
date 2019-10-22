#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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
        printf("Error sending Start Control Packet\n");
        return 1;
    }

    if(sendFileData(fd, file, fileSize) != 0) {
        printf("Error sending Data Packet\n");
        return 1;
    }

    if(sendControlPacket(fd, CONTROL_END, inputFileName, fileSize) != 0) {
        printf("Error sending End Control Packet\n");
        return 1;
    }

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

    controlPacket[0] = type;
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

    struct controlPacket paqueta = parseControlPacket(controlPacket, controlPacketSize);
    displayControlPacket(paqueta);

    int written = llwrite(fd, controlPacket, controlPacketSize);
    if(written != controlPacketSize) return 1;

    return 0;
}

int sendFileData(int fd, FILE* file, int fileSize) {
    const int nPackets = 20;
    const int nBytesPerPacket = ceil(fileSize/nPackets);
    char* data = malloc(fileSize);
    fread(data, fileSize, 1, file);
    for(unsigned int i = 0; i < nPackets; i++) {
        int realSize = nBytesPerPacket;
        if(i == nPackets-1) //in case the last packet has less bytes to send than the other ones
            realSize = fileSize - nBytesPerPacket*i;

        unsigned char* dataPacket = malloc(4 + realSize);

        dataPacket[0] = DATA; //control field
        dataPacket[1] = i; //serial number
        dataPacket[2] = (realSize & 0xff00) >> 8; //size of packet data
        dataPacket[3] = realSize & 0xff; //size of packet data
        memcpy(dataPacket + 4, data + i*nBytesPerPacket, realSize); //packet data

        //Send dataPacket Head
        int writtenHead = llwrite(fd, dataPacket, 4);
        if(writtenHead != 4) {
            printf("ERROR: Couldn't write head of dataPacket\n");
            return 1;
        }
        
        //Send dataPacket Data
        int writtenData = llwrite(fd, dataPacket + 4, realSize);
        if(writtenData != realSize) {
            printf("ERROR: Couldn't write everything in dataPacket\n");
            return 1;
        }

        printf("DATA Packet %d:\n", i);
        printf("SIZE: %d\n", writtenData);
    }
    return 0;
}

int assignControlTypeValue(int type, int length, char* value, struct controlPacket* control) {
    int size = 0;
    
    switch (type)
    {
    case FILE_SIZE:
        for(unsigned int i = 0; i < length; i++) {
            size = value[i] << 8*i;
        }
        control->file_size = size;
        break;

    case FILE_NAME:
        control->file_name = (char*) malloc(length);
        memcpy(control->file_name, value, length);
        break;
    
    default:
        printf("ERROR: Unknown type in Control Packet: %d\n", type);
        return 1;
        break;
    }

    return 0;
}

struct controlPacket parseControlPacket(unsigned char* packet, int packetSize) {
    struct controlPacket control;
    control.control_field = packet[0];

    unsigned int i = 1;
    while(i < packetSize) {
        int type = packet[i];
        int length = packet[i+1];
        char* value = (char*) malloc(length);
        memcpy(value, packet + i + 2, length);
        assignControlTypeValue(type, length, value, &control);
        i += length + 2;
    }

    return control;
}

struct dataHead* parseDataHead(unsigned char* head) {
    struct dataHead* header = (struct dataHead*) malloc(sizeof(struct dataHead));
    if(head[0] != DATA) return NULL;
    header->serialNumber = head[1];
    header->packet_size = (((head[2] << 8) & 0xff00) | (head[3] & 0xff));

    return header;
}

void displayControlPacket(struct controlPacket packet) {
    switch (packet.control_field)
    {
    case CONTROL_START:
        printf("Control Field: START\n");
        break;
    case DATA:
        printf("Control Field: DATA\n");
        break;
    case CONTROL_END:
        printf("Control Field: END\n");
        break;
    
    default:
        printf("Control Field: UNKNOWN (%d)\n", packet.control_field);
        break;
    }

    printf("File name: %s\n", packet.file_name);
    printf("File size: %d\n", packet.file_size);
}

int receiveFile(int fd, char* outputFileName) {
    unsigned char* readControlPacket = malloc(100);
    int controlPacketSize = llread(fd, readControlPacket);

    struct controlPacket controlStart = parseControlPacket(readControlPacket, controlPacketSize);
    displayControlPacket(controlStart);

    int fileRead = 0;
    unsigned char* finalFileData = (unsigned char*) malloc(controlStart.file_size);
    while(fileRead < controlStart.file_size) {
        //Read header
        unsigned char* head = malloc(4);
        int headLength = llread(fd, head);
        if(headLength != 4) {
            printf("Couldn't read entire Head of data Packet! Only %d bytes read\n", headLength);
            return 1;
        }
        struct dataHead* header = parseDataHead(head);
        if(header == NULL) {
            printf("ERROR reading data header, type is not data.\n");
            return 1;
        }
        //printf("SN: %d\nPacket Size: %d\n", header->serialNumber, header->packet_size);

        //Read data
        unsigned char* file = malloc(header->packet_size);
        int nRead = llread(fd, file);
        memcpy(finalFileData + fileRead, file, nRead);

        printf("DATA Packet: %d\n", header->serialNumber);
        printf("Read: %d\n", nRead);
        /*for(int i = 0; i < nRead; i++) {
            printf("%d: %X\n", i, file[i]);
        }*/
        fileRead += nRead;
    }
    printf("EXITED READ FILE LOOP\n");
   
    unsigned char* endControlPacket = malloc(100);
    int endControlPacketSize = llread(fd, endControlPacket);
    struct controlPacket controlEnd = parseControlPacket(endControlPacket, endControlPacketSize);

    displayControlPacket(controlEnd);
    //check that control end is the same as control Start to be sure there were no errors

    //save file into a new gif
    char newFileName[100];
    sprintf(newFileName, "imagesRecieved/%s", controlEnd.file_name);
    FILE* newFile = fopen(newFileName, "w");

    if(newFile == NULL)
    {
        printf("Unable to create file.\n");
        exit(EXIT_FAILURE);
    }

    fwrite(finalFileData, sizeof(char), controlEnd.file_size, newFile);
    fclose(newFile);


    return 0;
}
