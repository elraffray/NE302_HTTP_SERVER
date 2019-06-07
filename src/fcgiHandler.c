#include "fcgiHandler.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <linux/limits.h>
#include <stdio.h>
#include <errno.h>
#include "socket.c"
#include "api.h"
#include "semantic.h"

#define HEADER_PREFIX "HTTP_"
#define PROXY_URL "proxy:fcgi://127.0.0.1:9000/"

char *execute(char *filename, int *l)
{
    FCGI_Header h;
    _Token *headers, *header, *methodToken, *bodyToken;
    char *name, *value, *fullname, *path, *script_filename, *fullpath, *method, *body, *res;
    int fd, len, i;
    
    
    //on créé le socket et on initialise la connection
    fd = createSocket(9000);
	sendBeginRequest(fd,10,FCGI_RESPONDER,FCGI_KEEP_CONN);

    h.type = FCGI_PARAMS;
    h.requestId = 10;
	h.version=FCGI_VERSION_1; 
    h.paddingLength = 0;

    headers= searchTree(NULL, "header-field");
    if (headers == NULL)
        exit(1);


    // extraction de tout les headers HTTP
    header = headers;
    while (header != NULL)
    {
        extractHeader(getElementValue(header->node, &len), &name, &value);
        if (strcmp(name, "Content-Type") == 0)
            addNameValuePair(&h, "CONTENT_TYPE", value);
        else if (strcmp(name, "Content-Length") == 0)
            addNameValuePair(&h, "CONTENT_LENGTH", value);
        else
        {
            fullname = (char *)malloc(strlen(HEADER_PREFIX) + strlen(name) +1);
            if (fullname == NULL)
                exit(1);
            strcpy(fullname, HEADER_PREFIX);
            strcat(fullname, name);
            for (i = 0; i < strlen(fullname); i++)
                fullname[i] = toupper(fullname[i]);

            addNameValuePair(&h, fullname, value);
            free(fullname);
        }
        free(name);
        free(value);
        header = header->next;
    }
    purgeElement(&headers);

    // SCRIPT_NAME
    path = (char *)malloc(strlen(filename) +2);
    if (path == NULL)
    {
        printf("erreur malloc");
        exit(1);
    }
    path[0] = '/';
    path[1] = '\0';
    strcat(path, filename);
    addNameValuePair(&h, "SCRIPT_NAME", path);
    
    //while (path[strlen(path) -1] != '/') path[strlen(path) -1] = '\0';
    addNameValuePair(&h, "REQUEST_URI", path);

    // SCRIPT_FILENAME
    fullpath = realpath(filename, NULL);
    if (fullpath == NULL)
    {
        printf("erreur realpath()\n");
        free(path);
        exit(1);
    }
    script_filename = (char *)malloc(strlen(fullpath) +2);
    script_filename[0] = '/';
    script_filename[1] = '\0';
    strcat(script_filename, fullpath);
    addNameValuePair(&h, "SCRIPT_FILENAME", script_filename);
    free(fullpath);
    free(script_filename);

    // REQUEST_METHOD
    methodToken = searchTree(NULL, "method");
    if (methodToken == NULL)
    {
        printf("no method found\n");
        free(path);
        exit(1);
    }
    method = getElementValue(methodToken->node, &len);
    free(methodToken);
    addNameValuePair(&h, "REQUEST_METHOD", method);
	writeSocket(fd,&h,FCGI_HEADER_SIZE+(h.contentLength)+(h.paddingLength)); 
    h.contentLength = 0;
	writeSocket(fd,&h,FCGI_HEADER_SIZE+(h.contentLength)+(h.paddingLength)); 

     



    bodyToken = searchTree(NULL, "message-body");
    if (bodyToken != NULL)
    {
        body = getElementValue(bodyToken->node, &len);
        sendStdin(fd, 10, body, len);
    }
    
    
    sendStdin(fd, 10, "", 0);

    free(path);
    res = readStdout(fd, l);
    sendAbortRequest(fd, 10);
    close(fd);
    return res;
}


void extractHeader(char *header, char **name, char **value)
{
    char *pch, ows = 0, *tmp;
    
    pch = strchr(header, ':');
    if (pch == NULL)
    {
        printf("Erreur strchr");
        exit(1);
    }
    *pch = '\0';
    *name = (char *)malloc(strlen(header) +1);
    if (*name == NULL)
    {
        printf("Erreur malloc extractHeader name\n");
        exit(1);
    }
    strcpy(*name, header);
    *pch = ':';
    tmp = pch;
    tmp++;
    while (isspace(*tmp)) tmp++;

    *value = (char *)malloc(strlen(tmp) + 1);
    if (*value == NULL)
    {
        printf("Erreur malloc extractHeader value");
        exit(1);
    }
    strcpy(*value, tmp);
}
char *readStdout(int socket, int *l)
{
    char tmp[9], *msg = NULL, *newMsg = NULL, *padding;
    int len = 0, msgLen = 0;

    //on lit l'entete du message pour extraire la taille des données.
    readNBytes(socket, 8, tmp);

    while (tmp[1] != FCGI_END_REQUEST)
    {
        //on vérifie le type du message
        printf("Fcgi type: %X\n", tmp[1]);
        len += tmp[4];
        len = len << 8;
        len += tmp[5];
        if (msg == NULL)
        {
            msg = (char *)malloc(len + 1);
            if (msg == NULL)
            {
                printf("erreur malloc readstdout msg.\n");
                exit(1);
            }
        }
        else
        {
            newMsg = (char *)realloc(msg, msgLen + len +1);
            if (newMsg == NULL)
            {
                printf("erreur realloc readstdout.\n");
                exit(1);
            }
            msg = newMsg;
        }
        printf("Content-length: %d\n", len);
        readNBytes(socket, len, msg+msgLen);
        msgLen += len;
        printf("Padding: %d\n", tmp[6]);
        if (tmp[6] > 0)
        {
            padding = (char *)malloc(tmp[6]);
            if (padding == NULL)
            {
                printf("erreur malloc readstdout padding.\n");
                exit(1);
            }
            readNBytes(socket, tmp[6], padding);

        }
        //on lit le header du prochain message
        readNBytes(socket, 8, tmp);
        free(padding);
    }
    *l = msgLen;
    printf("MSG: %s\n", msg);
    return msg;

}


void readNBytes(int socket, unsigned int n, char* buffer)
{
    int bytesRead = 0;
    int result;
    while (bytesRead < n)
    {
        result = read(socket, buffer + bytesRead, n - bytesRead);
        if (result == -1 )
        {
            printf("erreur read socket, errno = %d\n", errno);
            exit(1);
        }
        bytesRead += result;
    }
}