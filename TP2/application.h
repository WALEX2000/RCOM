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

int sendControlPacket(int fd, ControlPacketType type, char* fileName, int fileSize);
int sendFileData(int fd, FILE* file, int fileSize);
