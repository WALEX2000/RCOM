typedef struct FTP_sockets {
    int control_fd;
    int data_fd;
} FTP_sockets;

int ftpSend(FTP_sockets * ftp_sockets, const char* str, int size);
int ftpRead(FTP_sockets * ftp_sockets, char * buffer, int size);
int ftpConnect(FTP_sockets * ftp_sockets, const char * ip, int port);
int ftpDisconnect(FTP_sockets * ftp_sockets);
int ftpLogin(FTP_sockets * ftp_sockets, const char * user, const char * password);
int ftpChangeWorkingDirectory(FTP_sockets * ftp_sockets, const char* path);
int ftpSetPassiveMode(FTP_sockets * ftp_sockets);
int ftpRetrieve(FTP_sockets * ftp_sockets, const char* filename);
int ftpDownload(FTP_sockets * ftp_sockets, const char* filename);