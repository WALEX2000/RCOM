#include <stdio.h>
#include <string.h>

#include "url.h"
#include "ftp.h"

//ftp://[username]:[password]@[servidor]:[porta]
int main(int argc, char **argv) {

    if(argc!=2) {
        printf("Usage: download ftp://[user]:[password]@[host]/[path]\n");
        return 1;
    }

    Url url;
    initUrl(&url);
    if(parseUrl(argv[1], &url)) {
        printf("Usage: download ftp://[user]:[password]@[host]/[path]\n");
        return 1;
    }

    if(getIpFromHost(&url)) {
        printf("Couldn't resolve ip from host\n");
        return 1;
    }

    FTP_sockets ftp;
    if(ftpConnect(&ftp, url.ip, url.port))
        return 1;

    if(ftpLogin(&ftp, url.user, url.password))
        return 1;

    if(strcmp(url.path, ""))
        if(ftpChangeWorkingDirectory(&ftp, url.path))
            return 1;

    if(ftpSetPassiveMode(&ftp))
        return 1;

    if(ftpRetrieve(&ftp, url.filename))
        return 1;

    if(ftpDownload(&ftp, url.filename))
        return 1;
    
    if(ftpDisconnect(&ftp))
        return 1;

    return 0;
}