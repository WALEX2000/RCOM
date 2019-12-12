#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "ftp.h"

//FTP Commands are in the format "COMMAND <args>"
int ftpSend(FTP_sockets * ftp_sockets, const char* buffer, int size) {
    printf("Sending: %s", buffer);
	if (write(ftp_sockets->control_fd, buffer, size) <= 0) {
		printf("Error writing to FTP server\n");
		return 1;
	}
	return 0;
}

//FTP Replies are in the format "123 text"
int ftpRead(FTP_sockets * ftp_sockets, char * buffer, int size) {
	FILE* fp = fdopen(ftp_sockets->control_fd, "r");

	buffer = fgets(buffer, size, fp);
    printf("Received: %s", buffer);

	if(!(buffer[0]>='1' && buffer[0] <= '5') || (buffer[3] != ' ' && buffer[3] != '-')) {
        printf("Error reading FTP server reply\n");
        return 1;
    }
	
	if(buffer[3] == '-') {
		char code[4];
		memcpy(code, buffer, 3);
		code[3] = '\0';

		memset(buffer, 0, size);
		char newCode[4];
		newCode[3]= '\0';

		while(strcmp(code, newCode) || buffer[3] == '-') {
			buffer = fgets(buffer, size, fp);
    		printf("Received: %s", buffer);
			memcpy(newCode, buffer, 3);
		}
	}
	return 0;
}

static int openSocket(const char* ip, int port) {
	int sockfd;
	struct sockaddr_in server_addr;

	// server address handling
	explicit_bzero((char*) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip); /*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port); /*server TCP port must be network byte ordered */

	// open an TCP socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		return -1;
	}

	// connect to the server
	if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		perror("connect()");
		return -1;
	}

	return sockfd;
}

int ftpConnect(FTP_sockets * ftp_sockets, const char * ip, int port) {

	ftp_sockets->control_fd = openSocket(ip, port);
    if (ftp_sockets->control_fd < 0) {
		printf("Error opening control socket.\n");
		return 1;
	}

    char buffer[256];
	if (ftpRead(ftp_sockets, buffer, sizeof(buffer))) {
		printf("Error estabilishing connection to FTP server.\n");
		return 1;
	}

	return 0;
}

int ftpDisconnect(FTP_sockets * ftp_sockets) {

	char buffer[256];
	sprintf(buffer, "QUIT\r\n");
	if (ftpSend(ftp_sockets, buffer, strlen(buffer))) {
		printf("Error sending QUIT command.\n");
		return 1;
	}

	memset(buffer, 0, 256);
	if (ftpRead(ftp_sockets, buffer, sizeof(buffer))) {
		printf("Error estabilishing connection to FTP server.\n");
		return 1;
	}

	close(ftp_sockets->control_fd);
	return 0;
}

int ftpLogin(FTP_sockets * ftp_sockets, const char * user, const char * password) {
    char buffer[256];

	// Send USER command
	sprintf(buffer, "USER %s\r\n", user);
	if (ftpSend(ftp_sockets, buffer, strlen(buffer))) {
		printf("Error sending USER command\n");
		return 1;
	}

	if (ftpRead(ftp_sockets, buffer, sizeof(buffer))) {
		printf("Error receiving USER reply\n");
		return 1;
	}

	// cleaning buffer
	memset(buffer, 0, sizeof(buffer));

	// password
	sprintf(buffer, "PASS %s\r\n", password);
	if (ftpSend(ftp_sockets, buffer, strlen(buffer))) {
		printf("Error sending PASS command\n");
		return 1;
	}

	if (ftpRead(ftp_sockets, buffer, sizeof(buffer))) {
		printf("Error receiving PASS reply\n");
		return 1;
	}

	return 0;
}

int ftpChangeWorkingDirectory(FTP_sockets * ftp_sockets, const char* path) {
	char buffer[256];
	sprintf(buffer, "CWD %s\r\n", path);
	if (ftpSend(ftp_sockets, buffer, strlen(buffer))) {
		printf("Error sending CWD command.\n");
		return 1;
	}

	if (ftpRead(ftp_sockets, buffer, sizeof(buffer))) {
		printf("Error receiving CWD reply.\n");
		return 1;
	}

	return 0;
}

int ftpSetPassiveMode(FTP_sockets * ftp_sockets) {

	char buffer[256] = "PASV\r\n";
	if (ftpSend(ftp_sockets, buffer, strlen(buffer))) {
		printf("Error sending PASV command.\n");
		return 1;
	}

	memset(buffer, 0, 256);
	if (ftpRead(ftp_sockets, buffer, sizeof(buffer))) {
		printf("Error receiving PASV reply.\n");
		return 1;
	}
	
	int ip32, ip24, ip16, ip8;
	int port16, port8;
	if ((sscanf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).", &ip32, &ip24, &ip16, &ip8, &port16, &port8)) < 6) {
		printf("Couldn't read IP/port from PASV reply.\n");
		return 1;
	}

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d.%d.%d.%d", ip32, ip24, ip16, ip8);
	int port = (port16 << 8) + port8;

	if ((ftp_sockets->data_fd = openSocket(buffer, port)) < 0) {
		printf("Couldn't open data socket.\n");
		return 1;
	}
	return 0;
}

int ftpRetrieve(FTP_sockets * ftp_sockets, const char* filename) {
	char buffer[256];
	sprintf(buffer, "RETR %s\r\n", filename);
	if (ftpSend(ftp_sockets, buffer, strlen(buffer))) {
		printf("Error sending RETR command.\n");
		return 1;
	}

	memset(buffer, 0, sizeof(buffer));
	if (ftpRead(ftp_sockets, buffer, sizeof(buffer))) {
		printf("Error reading RETR reply.\n");
		return 1;
	}

	char code[4];
	memcpy(code, buffer, 3);
	code[3] = '\0';

	if(strcmp(code, "150"))
		return 1;

	return 0;
}

int ftpDownload(FTP_sockets * ftp_sockets, const char* filename) {
	FILE* file;
	int bytes;

	if (!(file = fopen(filename, "w+"))) {
		printf("Error creating file.\n");
		return 1;
	}

	char buf[1024];
	while ((bytes = read(ftp_sockets->data_fd, buf, sizeof(buf)))) {
		if (bytes < 0) {
			printf("ERROR: Nothing was received from data socket fd.\n");
			return 1;
		}

		if ((bytes = fwrite(buf, bytes, 1, file)) < 0) {
			printf("ERROR: Cannot write data in file.\n");
			return 1;
		}
	}

	fclose(file);
	close(ftp_sockets->data_fd);

	return 0;
}
