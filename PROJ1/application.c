#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "data-link.h"
#include "application.h"

extern Statistics data_link_statistics;

void printStatistics() {
    printf("\nFrames sent: %d\n", data_link_statistics.sentFrames);
    printf("Frames received: %d\n", data_link_statistics.receivedFrames);
    printf("ACKS: %d\n", data_link_statistics.noRR);
    printf("NACKS: %d\n", data_link_statistics.noREJ);
    printf("Timeouts: %d\n", data_link_statistics.noTimeouts);
    printf("Total time: %.03fms\n", data_link_statistics.timeSpent);
}

int main(int argc, char*argv[]) {
  srand(time(NULL));
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

    printStatistics();

    return 0;
}

int sendFile(int fd, char* inputFileName) {

    FILE* file = fopen(inputFileName, "r");
    if(file == NULL) {
        printf("Couldn't find file: %s\n", inputFileName);
        return 1;
    }

    fseek(file, 0L, SEEK_END);
    long int fileSize = ftell(file);
    rewind(file);

    if(sendControlPacket(fd, CONTROL_START, inputFileName, fileSize) != 0) {
        printf("Error sending Start Control Packet\n");
        return 1;
    }

    double elapsedTime = 0;
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    if(sendFileData(fd, file, fileSize) != 0) {
        printf("Error sending Data Packet\n");
        return 1;
    }

    if(sendControlPacket(fd, CONTROL_END, inputFileName, fileSize) != 0) {
        printf("Error sending End Control Packet\n");
        return 1;
    }

    clock_gettime(CLOCK_REALTIME, &end);
    elapsedTime = (end.tv_sec - start.tv_sec)*1000;
    elapsedTime += (end.tv_nsec - start.tv_nsec) / 1000000.0;
    data_link_statistics.timeSpent = elapsedTime;

    return 0;
}

int sendControlPacket(int fd, ControlPacketType type, char* fileName, long int fileSize) {
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


    //DEBUG
    //struct controlPacket paqueta = parseControlPacket(controlPacket, controlPacketSize);
    //displayControlPacket(paqueta);

    int written = llwrite(fd, controlPacket, controlPacketSize);
    if(written != controlPacketSize) return 1;

    free(controlPacket);
    return 0;
}

int sendFileData(int fd, FILE* file, long int fileSize) {
    const int nPackets = (fileSize/(MAX_DATA_PACKET_SIZE-4))+1;
    char* data = malloc(fileSize);
    double totalWritten = 0;
    char progressBar[11] = "          ";
    fread(data, fileSize, 1, file);
    for(unsigned int i = 0; i < nPackets; i++) {
        int realSize = MAX_DATA_PACKET_SIZE;
        if(i == (nPackets-1)) //in case the last packet has less bytes to send than the other ones
            realSize = fileSize - (MAX_DATA_PACKET_SIZE-4)*i + 4;

        unsigned char* dataPacket = malloc(realSize);

        dataPacket[0] = DATA; //control field
        dataPacket[1] = i; //serial number
        dataPacket[2] = (realSize & 0xff00) >> 8; //size of packet data
        dataPacket[3] = realSize & 0xff; //size of packet data
        memcpy(dataPacket + 4, data + i*(MAX_DATA_PACKET_SIZE-4), realSize-4); //packet data

        //Send dataPacket Data
        int writtenData = llwrite(fd, dataPacket, realSize);
        if(writtenData != realSize) {
            printf("ERROR: Couldn't write everything in dataPacket\n");
            return 1;
        }

        if(i != (nPackets-1))
            totalWritten += MAX_DATA_PACKET_SIZE-4;
        else
            totalWritten += fileSize % (MAX_DATA_PACKET_SIZE-4);
        for(int j = 0; j < 10; j++) {
            if(totalWritten/fileSize*10 > j)
                progressBar[j] = '#';
        }
        printf("[%s] (%.2f%%)", progressBar, totalWritten/fileSize*100);
        if(totalWritten/fileSize != 1)
            printf("\r");
        else
            printf("\n");
        fflush(stdout);

        free(dataPacket);
    }

    free(data);
    return 0;
}

int assignControlTypeValue(int type, int length, unsigned char* value, struct controlPacket* control) {
    long int size = 0;

    switch (type)
    {
    case FILE_SIZE:
        for(unsigned int i = 0; i < length; i++) {
            size += (value[i] & 0xff) << 8*i;
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
        unsigned char* value = (unsigned char*) malloc(length);
        memcpy(value, packet + i + 2, length);
        assignControlTypeValue(type, length, value, &control);
        i += length + 2;

        free(value);
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
    /*switch (packet.control_field)
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
    }*/

    printf("\nFile name: %s\n", packet.file_name);
    printf("File size: %lu\n\n", packet.file_size);
}

int receiveFile(int fd, char* saveFolderPath) {
    unsigned char* readControlPacket = malloc(100);
    int controlPacketSize = llread(fd, readControlPacket);

    double elapsedTime = 0;
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    struct controlPacket controlStart = parseControlPacket(readControlPacket, controlPacketSize);
    displayControlPacket(controlStart);

    double bytesRead = 0;
    unsigned char* finalFileData = (unsigned char*) malloc(controlStart.file_size);
    char progressBar[11] = "          ";
    while(bytesRead < controlStart.file_size) {

        //Read data
        unsigned char* packet = malloc(MAX_DATA_PACKET_SIZE);
        int nRead = llread(fd, packet);
        struct dataHead* header = parseDataHead(packet);
        if(header == NULL) {
            printf("ERROR reading data header, type is not data.\n");
            printf("bytesRead: %lu\n", (long unsigned int)bytesRead);
            printf("file size: %lu\n", controlStart.file_size);
            return 1;
        }

        memcpy(finalFileData + (int)bytesRead, packet + 4, nRead - 4);

        bytesRead += nRead - 4;

        for(int j = 0; j < 10; j++) {
            if(bytesRead/controlStart.file_size*10 > j)
                progressBar[j] = '#';
        }
        printf("[%s] (%.2f%%)", progressBar, bytesRead/controlStart.file_size*100);
        if(bytesRead/controlStart.file_size != 1)
            printf("\r");
        else
            printf("\n");

        fflush(stdout);

        free(header);
        free(packet);
    }

    unsigned char* endControlPacket = malloc(100);
    int endControlPacketSize = llread(fd, endControlPacket);

    clock_gettime(CLOCK_REALTIME, &end);
    elapsedTime = (end.tv_sec - start.tv_sec)*1000;
    elapsedTime += (end.tv_nsec - start.tv_nsec) / 1000000.0;
    data_link_statistics.timeSpent = elapsedTime;

    struct controlPacket controlEnd = parseControlPacket(endControlPacket, endControlPacketSize);
    //displayControlPacket(controlEnd);
    //check that control end is the same as control Start to be sure there were no errors

    //save file into a new gif
    char newFileName[100];
    sprintf(newFileName, "%s/%s", saveFolderPath, controlEnd.file_name);
    FILE* newFile = fopen(newFileName, "w");

    if(newFile == NULL) {
        printf("Unable to create file: %s\n", newFileName);
        return 1;
    }

    fwrite(finalFileData, sizeof(char), controlEnd.file_size, newFile);
    free(finalFileData);
    fclose(newFile);

    free(readControlPacket);
    free(endControlPacket);

    return 0;
}
