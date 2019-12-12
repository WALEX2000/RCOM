
#define MAX_ARG_LEN 256

// ftp://[user[:password]@]host[:port]/url-path
typedef struct Url {
	char user[MAX_ARG_LEN];
	char password[MAX_ARG_LEN];
	char host[MAX_ARG_LEN]; 
	char ip[MAX_ARG_LEN];
	char path[MAX_ARG_LEN]; 
	char filename[MAX_ARG_LEN];
	int port;
} Url;

void initUrl(Url *url);
int parseUrl(char* urlString, Url* url);
int getIpFromHost(Url *url);
