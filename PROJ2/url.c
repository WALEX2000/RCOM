#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "url.h"

static int readUntilChar(char **originalString, char *outputString, char delim)
{
    //Check if delimitating char exists
    if (strchr(*originalString, delim) == NULL)
    {
        return 1;
    }

    //Get index of delim, copy everything up to delim to outputString and advance it from originalString
    int index = strlen(*originalString) - strlen(strcpy(outputString, strchr(*originalString, delim)));
    outputString[index] = '\0';
    strncpy(outputString, *originalString, index);
    *originalString += strlen(outputString) + 1;

    return 0;
}

void initUrl(Url *url)
{
    memset(url->user, 0, sizeof(url->user));
    memset(url->password, 0, sizeof(url->password));
    memset(url->host, 0, sizeof(url->host));
    memset(url->ip, 0, sizeof(url->ip));
    memset(url->path, 0, sizeof(url->path));
    memset(url->filename, 0, sizeof(url->filename));
    url->port = 21;
}

// ftp://user:password@host/url-path
int parseUrl(char *urlString, Url *url)
{

    //Check for 'ftp://' prefix
    char ftpPrefix[7];
    strncpy(ftpPrefix, urlString, 6);
    ftpPrefix[6] = '\0';
    if (strcmp(ftpPrefix, "ftp://") != 0)
    {
        printf("Url missing prefix 'ftp://'\n");
        return 1;
    }
    urlString += 6;

    if (strchr(urlString, ':') == NULL)
    {
        //Anonymous mode
        strcpy(url->user, "anonymous");
        printf("User: %s\n", url->user);
        strcpy(url->password, "");
    } else {
        //User/password mode

        //Parse user
        if (readUntilChar(&urlString, url->user, ':') || strlen(url->user) == 0)
        {
            printf("Couldn't find user in url\n");
            return 1;
        }
        printf("User: %s\n", url->user);

        //Parse password
        if (readUntilChar(&urlString, url->password, '@') || strlen(url->password) == 0)
        {
            printf("Couldn't find password in url\n");
            return 1;
        }
        printf("Password: %s\n", url->password);
    }

    //Parse host
    if (readUntilChar(&urlString, url->host, '/') || strlen(url->host) == 0)
    {
        printf("Couldn't find host in url\n");
        return 1;
    }
    printf("Host: %s\n", url->host);

    //Parse path
    if (strchr(urlString, '/') == NULL)
    {
        strcpy(url->path, "");
    } else {
        memcpy(url->path, urlString, strrchr(urlString, '/')-urlString);
    }
    printf("Path: %s\n", url->path);

    //Parse filename
    if(strchr(urlString, '/') == NULL)
        strcpy(url->filename, urlString);
    else
        strcpy(url->filename, strrchr(urlString, '/') + 1);
    printf("Filename: %s\n", url->filename);

    return 0;
}

int getIpFromHost(Url *url)
{
    struct hostent *h;
    h = gethostbyname(url->host);
    if (h == NULL)
    {
        return 1;
    }

    strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr)));
    printf("IP: %s\n", url->ip);
    return 0;
}
