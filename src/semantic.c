#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "semantic.h"
#include "api.h"
#include "parser.h"

char methods[][10] = {"GET", "HEAD", "POST", "PUT", "DELETE", "CONNECT", "OPTIONS", "TRACE"};
int nbMethods = 8;

int keep_alive = 1;

char *uri;


char *getKeepAlive()
{
    return keep_alive;
}


char *getUri()
{
    return uri;
}

char *getResponseVersion()
{
    return "HTTP/1.1";
}

char *getStatusCode()
{
    return "200 OK";
}

char *getFileContent(char *filename)
{
    char * buffer = 0;
    long length;
    FILE * f = fopen (filename, "rb");
    if (f == NULL)
    {
        printf("fopen failed.\n");
        return NULL;
    }
    
    if (f)
    {
        fseek (f, 0, SEEK_END);
        length = ftell (f);
        fseek (f, 0, SEEK_SET);
        buffer = malloc (length);
        if (buffer)
        {
            fread (buffer, 1, length, f);
        }
        fclose (f);
    }

    if (buffer)
    {
        return buffer;
    }
    return NULL;
}



char *createResponse()
{
    char *response;
    char *statusCode = getStatusCode();
    char *version = getResponseVersion();
    char *body;
    char *connectionHeader = NULL;
    body = getFileContent(uri+1);
    if (body == NULL)
    {
        statusCode = "404 NOT FOUND";
        body = "";
    }
    if (keep_alive == 0)
    {
        connectionHeader = "Connection: close\r\n";
    }
    response = (char *)malloc(strlen(version) +
                              strlen(statusCode) +
                              strlen(body) +
                              strlen(connectionHeader) +
                              5);
     
    strcpy(response, version);
    strcat(response, " ");
    strcat(response, statusCode);
    strcat(response, "\r\n");

    // headers go here.
    if (connectionHeader)
        strcat(response, connectionHeader);


    strcat(response, "\r\n");
    strcat(response, body);

    if (strcmp(body, "") != 0)
        free(body);
    free(uri);
    return response;
    
}


int isHex(char c)
{
    return (c >= 48 && c <= 57) || (c >= 65 && c <= 70) || (c >= 97 && c <= 102); 
}


void decodeUri()
{
    _Token* res;
    int len, i, j = 0;
    res = searchTree(NULL, "request-target");
    char *original, *decoded, tmp, code[3];
    code[3] = '\0';
    original = getElementValue(res->node, &len);

    decoded = (char *)malloc(len + 1);
    if (decoded == NULL)
    {
        printf("Malloc failed.\n");
        exit(1);
    }


    // PERCENT ENCODING
    for (i = 0; i < len; i++)
    {
        if (i < len - 3 && original[i] == '%' && isHex(original[i+1]) && isHex(original[i+2]))
        {
            code[0] = original[i+1];
            code[1] = original[i+2];
            tmp =  (int)strtol(code, NULL, 16);
            decoded[j++] = tmp;
            i += 2;
        }
        else
        {
            decoded[j++] = original[i];
        }
    }
    decoded[j] = '\0';
    uri = decoded;

}

int methodCheck(char *req)
{
    _Token* res;
    int len, i, ind;
    res = searchTree(NULL, "method");

    for (i = 0; i < nbMethods; i++)
    {
        if (strcmp(getElementValue(res->node, &len), methods[i]) == 0)
        {
            ind = i;
            break;
        }
    }

    // Semantics checks
    switch (ind)
    {
        case 0: // if GET
            if (searchTree(NULL, "message-body") != NULL) //if request has message body
                return 0;
            break;
        
        case 1: // if HEAD
            if (searchTree(NULL, "message-body") != NULL) //if request has message body
                return 0;
            break;
        
        default:
            break;
    }
    return 1;
}

// RFC 7230 6
int checkConnectionHeader()
{
    _Token* res;
    int len;
    res = searchTree(NULL, "HTTP-version");
    if (res != NULL)
    {
        if (strcmp(getElementValue(res->node, &len), "HTTP/1.1") == 0)
        {
            if ((res = searchTree(NULL, "Connection")) == NULL)
            {
                if (strcmp(getElementValue(res->node, &len), "close") == 0)
                {
                    keep_alive = 0;
                }
            }
        }
        else if (strcmp(getElementValue(res->node, &len), "HTTP/1.0") == 0)
        {
            if ((res = searchTree(NULL, "Connection")) == NULL)
            {
                if (strcmp(getElementValue(res->node, &len), "keep-alive") == 0)
                {
                    keep_alive = 1;
                }
            }
        }
    }
    return 1;
}


// RFC 7230 5.4
int checkHostHeader()
{
    _Token* res;
    int len;
    res = searchTree(NULL, "HTTP-version");
    if (res != NULL)
    {
        if (strcmp(getElementValue(res->node, &len), "HTTP/1.1") == 0)
        {
            if (searchTree(NULL, "Host-header") == NULL)
            {
                printf("NO HOST HEADER IN HTTP/1.1\n");
                return 0;
            }
        }
    }
    return 1;
}


int semanticCheck(char *req)
{
    keep_alive = 1;
    decodeUri();
    if (!methodCheck(req))
        return 0;
    if (!checkHostHeader(req))
        return 0;
    if (!checkConnectionHeader(req))
        return 0;
    return 1;
}