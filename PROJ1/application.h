#pragma once

#define MAX_DATA_PACKET_SIZE 128

int sendFile(int fd, char* inputFileName);
int receiveFile(int fd, char* saveFolderPath);


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
    long int file_size;
};

struct dataHead{
    int serialNumber;
    int packet_size;
};

int sendControlPacket(int fd, ControlPacketType type, char* fileName, long int fileSize);
int sendFileData(int fd, FILE* file, long int fileSize);
struct controlPacket parseControlPacket(unsigned char* packet, int packetSize);
void displayControlPacket(struct controlPacket packet);
int assignControlTypeValue(int type, int length, unsigned char* value, struct controlPacket* control);
