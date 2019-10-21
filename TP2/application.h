#pragma once

int sendFile(int fd, char* inputFileName);
int receiveFile(int fd, char* outputFileName);

typedef enum {
    CONTROL_START = 2,
    CONTROL_END = 3
} ControlPacketType;

typedef enum {
    FILE_SIZE,
    FILE_NAME
} ControlPacketAttributeType;