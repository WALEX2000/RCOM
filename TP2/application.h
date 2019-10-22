#pragma once

int sendFile(int fd, char* inputFileName);
int receiveFile(int fd, char* outputFileName);


typedef enum {
    DATA = 1,
    CONTROL_START = 2,
    CONTROL_END = 3
} ControlPacketType;

typedef enum {
    FILE_SIZE,
    FILE_NAME
} ControlPacketAttributeType;

struct controlPacket{
    int control_field;
    char* file_name;
    int file_size;
};

struct dataHead{
    int serialNumber;
    int packet_size;
};

int sendControlPacket(int fd, ControlPacketType type, char* fileName, int fileSize);
int sendFileData(int fd, FILE* file, int fileSize);
struct controlPacket parseControlPacket(unsigned char* packet, int packetSize);
void displayControlPacket(struct controlPacket packet);