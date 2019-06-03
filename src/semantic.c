#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "semantic.h"
#include "api.h"
#include "parser.h"



char methods[][10] = {"GET", "HEAD", "POST", "PUT", "startETE", "CONNECT", "OPTIONS", "TRACE"};
int nbMethods = 8;
char max_version[10] = "";
int keep_alive = 1;

QStruct qvalues[MAX_QVALUES];
int nbQvalues;

char *uri;




int cmpMimetype(const void *a, const void *b)
{
    int res = (*(QStruct *)a).value - (*(QStruct *)b).value;
    if (res != 0) //si on a des valeurs de q différentes
        return res;

    //sinon on met les type le plus spécifique en premier (7231 5.3.2)
    return strlen((*(QStruct *)b).name) - strlen((*(QStruct *)a).name);
}



//compare les qname en fonction de leur valeur 
int cmpQvalue(const void *a, const void *b)
{
    return (*(QStruct *)a).value - (*(QStruct *)b).value;
}

int setQValuesFrom(char *rulename)
{
    _Token* res;
    int len, curr = 0, start = 0, i;
    char *list, *pch;
    res = searchTree(NULL, rulename);
    if (res == NULL)
        return 0;

    list = getElementValue(res->node, &len);
    pch = strtok(list, ";, ");
    purgeElement(&res);

    while (pch != NULL)
    {
        if (pch[0] == 'q' && pch[1] == '=')
        {
            for (i = start; i < curr; i++)
            {
                qvalues[i].value = atof(pch+2);
            }
            start = curr;
        }
        else
        {
            strcpy(qvalues[curr].name, pch);
            qvalues[curr].value = -1;
            curr++;
        }
        pch = strtok(NULL, ";, ");
    }

    qsort(qvalues, curr, sizeof(QStruct), cmpMimetype);
    nbQvalues = curr;

    return 1;
}


int setAcceptQValuesFrom(char *rulename)
{
    _Token* res;
    int len, curr = 0, start = 0, i;
    char *list, *pch;
    res = searchTree(NULL, rulename);
    if (res == NULL)
        return 0;

    list = getElementValue(res->node, &len);
    pch = strtok(list, ";, ");
    purgeElement(&res);


    while (pch != NULL)
    {
        if (pch[0] == 'q' && pch[1] == '=')
        {
            for (i = start; i < curr; i++)
            {
                qvalues[i].value = atof(pch+2);
            }
            start = curr;
        }
        else if (strchr(pch, '=') != NULL) //on regarde si on est dans un accept-format-extension
        {
            strcat(qvalues[curr-1].name, ";");
            strcat(qvalues[curr-1].name, pch);

        }
        else
        {
            strcpy(qvalues[curr].name, pch);
            qvalues[curr].value = -1;
            curr++;
        }
        pch = strtok(NULL, ";, ");
    }

    qsort(qvalues, curr, sizeof(QStruct), cmpMimetype);
    nbQvalues = curr;

    return 1;
}


int getKeepAlive()
{
    return keep_alive;
}


char *getUri()
{
    return uri;
}


char *decodeChunked(char *data)
{
    char *res;
    int chunkLen, i = 0;
    res = (char *)malloc(strlen(data));
    if (res == NULL)
    {
        exit(1);
    }

    sscanf(data, "%x\r\n", &chunkLen);
    while (chunkLen != 0)
    {   
        sscanf(data, "%c", &res[i++]);
        sscanf(data, "%x\r\n", &chunkLen);
    }

    return res;
}



char *decodeBody()
{
    _Token* res, *body;
    int len;
    char *pch, *val, *raw, *decoded;
    
    res = searchTree(NULL, "Transfer-Encoding");
    if (res == NULL)
        return 0;
    val = getElementValue(res->node, &len);

    body = searchTree(NULL, "message-body");
    if (body == NULL)
        return 0;
    raw = getElementValue(body->node, &len);
    free(body);

    pch = strtok(val, ", ");

    while (pch != NULL)
    {
        if (strcmp(pch, "chunked") == 0)
        {
            decoded = decodeChunked(raw);
            free(raw);
            raw = decoded;   
        }
        else if (strcmp(pch, "compress") == 0)
        {
            decoded = raw;
        }
        else if (strcmp(pch, "deflate") == 0)
        {
            decoded = raw;
        }
        else if (strcmp(pch, "gzip") == 0)
        {
            decoded = raw;
        }
        else if (strcmp(pch, "identity") == 0)
        {
            decoded = raw;
        }
        else
            decoded = raw;

        pch = strtok(NULL, ", ");
    }

    return decoded;
    
}


char *encodeBody(char *encoding, int *len)
{
    char *body;
    int i;

    body = getFileContent(uri+1, len);
    if (body == NULL)
        return NULL;

    if (!setQValuesFrom("Accept-Encoding"))
    {
        return body;
    }

    for (i = 0; i < nbQvalues; i++)
    {
        if (strcmp(qvalues[i].name, "chunked") == 0)
        {
            encoding = "chunked";
            return body;
        }
        else if (strcmp(qvalues[i].name, "compress") == 0)
        {
            encoding = "compress";
            return body;
        }
        else if (strcmp(qvalues[i].name, "deflate") == 0)
        {
            encoding = "deflate";
            return body;
        }
        else if (strcmp(qvalues[i].name, "gzip") == 0)
        {
            encoding = "gzip";
            return body;
        }
        else if (strcmp(qvalues[i].name, "identity") == 0)
        {
            encoding = "identity";
            return body;
        }
        else
        {
            encoding = "identity";
            return body;
        }
    }
    return body;
}


char *getContentType()
{
    int i;
    char *res;
    char *mimeType, *mimeSubType, *mimedata, *pch;
    char *goalType, *goalSubType;
    mimedata = getMimeData(uri+1);
    if (mimedata == NULL)
        return NULL;
    //on récupere type/soustype
    pch = strtok(mimedata, "/ ");
    if (pch != NULL)
    {
        mimeType = (char *)malloc(strlen(pch) + 1);
        if (mimeType == NULL)
        {
            free(mimedata);
            exit(1);
        }
        strcpy(mimeType, pch);

        pch = strtok(NULL, "/ ");
        if (pch != NULL)
        {
            mimeSubType = (char *)malloc(strlen(pch) + 1);
            if (mimeSubType == NULL)
            {
                free(mimedata);
                exit(1);
            }
            strcpy(mimeSubType, pch);
        }
    }
    else
    {
        free(mimedata);
        exit(1);
    }
    free(mimedata);
    
    
    if (setAcceptQValuesFrom("Accept"))
    {
        for (i = 0; i < nbQvalues; i++)
        {
            //on sépare type/soustype
            pch = strtok(qvalues[i].name, "/");
            if (pch != NULL)
            {
                goalType = (char *)malloc(strlen(pch) + 1);
                if (goalType == NULL)
                {
                    exit(1);
                }
                strcpy(goalType, pch);

                pch = strtok(NULL, "/");
                if (pch != NULL)
                {
                    goalSubType = (char *)malloc(strlen(pch) + 1);
                    if (goalSubType == NULL)
                    {
                        exit(1);
                    }
                    strcpy(goalSubType, pch);

                }
            }
            

            // correspondance mime
            printf("mimetype: %s/%s\ngoaltype: %s/%s\n", mimeType, mimeSubType, goalType, goalSubType);
            
            if ((strcmp(goalType, mimeType) == 0 || strcmp(goalType, "*") == 0) && (strcmp(goalSubType, mimeSubType) == 0 || strcmp(goalSubType, "*") == 0))
            {
                mimedata = getMimeData(uri+1);
                res = (char *)malloc(strlen("Content-Type: ") + strlen(mimedata) + 3);
                if (res == NULL)
                {
                    exit(1);
                }
                strcpy(res, "Content-Type: ");
                strcat(res, mimedata);
                strcat(res, "\r\n");
                free(mimeType);
                free(mimeSubType);
                free(goalType);
                free(goalSubType);
                printf("MIME trouvé: %s\n", res);
                free(mimedata);
                return res;
            }
            free(goalType);
            free(goalSubType);
        }
    }


    free(mimeType);
    free(mimeSubType);

    return NULL;
}

char *getMimeData(char *filename)
{
    FILE *fp;
    char path[1035];
    char *cmd, *pch, *res;
    
    if (access(filename, R_OK) == -1)
    {
        return NULL;
    }


    if (strcmp(filename+strlen(filename) - 3, "css") == 0)
    {
        res = (char *)malloc(strlen("text/css") + 1);
        if (res == NULL)
        {
            exit(1);
        }
        strcpy(res, "text/css");
        return res;
    }
    cmd = (char *)malloc(strlen(filename) + strlen("file -i ") + 1);
    if (cmd == NULL)
    {
        exit(1);
    }
    strcpy(cmd, "file -i ");
    strcat(cmd, filename);

    /* Open the command for reading. */
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        free(cmd);
        exit(1);
    }
    /* Read the output a line at a time - output it. */
    if (fgets(path, sizeof(path)-1, fp) == NULL) {
        pclose(fp);
        free(cmd);
        return NULL;
    }
    printf("mimedata: %s\n", path);
    free(cmd);

    pclose(fp);
    // exemple output: "www/postel.jpg: image/jpeg; charset=binary"
    pch = strtok(path, ":; "); // pointe sur www/postel.jpg
    if (pch != NULL)
    {
        pch = strtok(NULL, ":; "); // pointe sur le mimetype
        if (pch != NULL)
        {
            res = (char *)malloc(strlen(pch) + 1);
            if (res == NULL)
                exit(1);
            strcpy(res, pch);
            return res;
        }
    }

    return NULL;
}




char *getResponseVersion()
{
    _Token* res;
    int len;
    char *val, tmp[2], *pch;

    if (strlen(max_version) <= 1)
    {
        strcat(max_version, "HTTP/");
        sprintf(tmp,"%d",MAX_MAJOR_VERSION);
        strcat(max_version, tmp);
        sprintf(tmp,"%d",MAX_MINOR_VERSION);
        strcat(max_version, tmp);
    }

    res = searchTree(NULL, "HTTP-version");
    if (res != NULL)
    {
        val = getElementValue(res->node, &len);
        purgeElement(&res);

        if (strcmp(val, "HTTP/1.1") == 0)
        {
            return "HTTP/1.1";
        }
        else if (strcmp(val, "HTTP/1.0") == 0)
        {
            return "HTTP/1.0";
        }
        pch = strtok(val, "HTTP/.");
        if (atoi(pch) > MAX_MAJOR_VERSION) // on regarde la version majeure de la requete
        {
            return max_version;
        }
        pch = strtok(NULL, "HTTP/.");
        if (atoi(pch) > MAX_MINOR_VERSION) // on regarde la version mineure de la requete
        {
            return max_version;
        }
    }
    return max_version;
}

char *getStatusCode()
{
    _Token* res, *it;
    int len, i ,ind;
    char *val, *pch, *tmp, *tmp2;

    // requete HTTP/1.1 doit avoir un Host-header
    res = searchTree(NULL, "HTTP-version");
    if (res != NULL)
    {
        val = getElementValue(res->node, &len);
        printf("version HTTP: %s\n", val);
        purgeElement(&res);

        if (strcmp(val, "HTTP/1.1") == 0)
        {
            if ((res = searchTree(NULL, "Host-header")) == NULL)
            {
                purgeElement(&res);
                return "400 Bad Request";
            }
            purgeElement(&res);
        }
        pch = strtok(val, "HTTP/.");
        if (atoi(pch) > MAX_MAJOR_VERSION) // on regarde la version majeure de la requete
        {
            return "505 HTTP Version Not Supported";
        }
    }

    res = searchTree(NULL, "method");
    for (i = 0; i < nbMethods; i++)
    {
        if (strcmp(getElementValue(res->node, &len), methods[i]) == 0)
        {
            ind = i;
            break;
        }
    }
    purgeElement(&res);

    res = searchTree(NULL, "message-body");
    switch (ind)
    {
        case 0: // requete GET ne doit pas avoir de message-body
            if (res != NULL)
            {
                purgeElement(&res);
                return "400 Bad Request";
            }
            break;
        
        case 1: // requete HEAD ne doit pas avoir de message-body
            if (res != NULL)
            {
                purgeElement(&res);
                return "400 Bad Request";
            }
            break;
        
        default:
            break;
    }


    if ((res = searchTree(NULL, "Transfer-Encoding-header")) != NULL)
    {
        purgeElement(&res);
        if ((res = searchTree(NULL, "Content-Length-header")) != NULL)
        {
            purgeElement(&res);
           return "400 Bad Request";
        }
    }

    //RFC 7230 3.3.3 4. => si plusieurs content-length != => 400
    if ((res = searchTree(NULL, "Transfer-Encoding-header")) == NULL)
    {
        purgeElement(&res);
        if ((res = searchTree(NULL, "Content-Length")) != NULL)
        {
            it = res;
            tmp = getElementValue(res->node, &len);
            while (it->next != NULL)
            {
                tmp2 = getElementValue(it->next->node, &len);
                if (strcmp(tmp, tmp2) != 0)
                {
                    purgeElement(&res);
                    return "400 Bad Request";
                }
            }
        }
    }
    purgeElement(&res);




    // Si uri non trouvée
    if (access(uri+1, R_OK) == -1)
    {
        return "404 Not Found";
    }




    return "200 OK";
}

char *getFileContent(char *filename, int *len)
{
    char * buffer = 0;
    long length;
    printf("target-file: %s\n", filename);

    if (access(filename, R_OK) == -1)
    {
        return NULL;
    }
    
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
        *len = length;
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
        printf("URI FOUND: %s\n", filename);
        return buffer;
    }

    return NULL;
}



char *createResponse(int *length)
{
    char *response, *version, *statusCode, headers[256] = "", *body, tmp[16];
    char *contenttype, encoding[16];
    int len = 0, bodyLen = 0;

    decodeUri();

    body = encodeBody(encoding, &bodyLen);
    version = getResponseVersion();
    statusCode = getStatusCode();

    if (checkConnectionHeader() == 0)
    {
        strcat(headers , "Connection: close\r\n");
    }

    

    contenttype = getContentType();
    if (contenttype != NULL)
    {
        strcat(headers, contenttype);
        free(contenttype);
    }


    if (body != NULL)
    {
        strcat(headers , "Content-Length: ");
        sprintf(tmp,"%d", bodyLen);
        strcat(headers , tmp);
        strcat(headers, "\r\n");
    }

    len += (strlen(version) +
           strlen(statusCode) +
           strlen(headers) +
           10);
    if (body != NULL)
        len += bodyLen;
    response = (char *)malloc(len);
    if (response == NULL)
    {
        exit(1);
    }
     
    strcpy(response, version);
    strcat(response, " ");
    strcat(response, statusCode);
    strcat(response, "\r\n");

    // headers go here.
    if (strlen(headers) > 1)
        strcat(response, headers);


    strcat(response, "\r\n");
    *length = strlen(response) + bodyLen;
    if (body != NULL)
    {
        //strcat(response, body);
        memcpy(response + strlen(response) ,body, bodyLen);
        free(body);
    }

    free(uri);

    //printf("\n############## REPONSE ################\n%s\n\n", response);
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
    if (res == NULL)
    {
        printf("no request-target");
        exit(1);
    }
    char *original, *decoded, *sanitized, tmp, code[3];
    code[3] = '\0';
    original = getElementValue(res->node, &len);
    purgeElement(&res);
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
    len = strlen(decoded);
    j = 0;


    // DOT SEGMENT REMOVAL (RFC 3986 5.2.4)
    sanitized = (char *)malloc(len + 1);
    if (sanitized == NULL)
    {
        printf("Malloc failed.\n");
        exit(1);
    }
    for (i = 0; i < len; i++)
    {
        // A
        if ((i < len - 3 && decoded[i] == '.' && decoded[i+1] == '.' && decoded[i+2] == '/') ||
            (i < len - 2 && decoded[i] == '.' && decoded[i+1] == '/' ))
        {
            while (decoded[i] != '/') i++;
            i++;
        }
        // B
        if ((i < len - 3 && decoded[i] == '/' && decoded[i+1] == '.' && decoded[i+2] == '/') ||
            (i < len - 2 && decoded[i] == '/' && decoded[i+1] == '.' ))
        {
            if (i < len - 3 && decoded[i+2] == '/')
                i+=1;
            else
            {
                decoded[i+1] = '/';
            }
        }
        // C
        if ((i < len - 4 && decoded[i] == '/' && decoded[i+1] == '.' && decoded[i+2] == '.' && decoded[i+3] == '/') ||
            (i < len - 3 && decoded[i] == '/' && decoded[i+1] == '.' && decoded[i+2] == '.'))
        {
            while (decoded[i] != '/') i++;
            if (i < len - 4 && decoded[i+3] == '/')
                i+=2;
            else
            {
                i+=1;
                decoded[i+2] = '/';
            }
            while (sanitized[j] != '/') j--;
            j--;

        }
        // D
        if (strcmp(decoded, ".") == 0)
            i++;
        if (strcmp(decoded, "..") == 0)
            i+=2;
        // E
        if (i < len)
        {
            if (decoded[i++] == '/') sanitized[j++] = '/';
            while (i < len && decoded[i] != '/')
            {
                sanitized[j++] = decoded[i++];
            }
            i--;
        }

    }

    free(decoded);
    sanitized[j] = '\0';
    uri = sanitized;


}


// RFC 7230 6
// retourne 1 pour keep-alive, 0 pour close
int checkConnectionHeader()
{
    _Token* res;
    int len;
    res = searchTree(NULL, "HTTP-version");
    if (res != NULL)
    {
        if (strcmp(getElementValue(res->node, &len), "HTTP/1.1") == 0)
        {
            purgeElement(&res);
            if ((res = searchTree(NULL, "Connection")) == NULL)
            {
                if (strcmp(getElementValue(res->node, &len), "close") == 0)
                {
                    keep_alive = 0;
                    purgeElement(&res);
                    return 0;
                }
                purgeElement(&res);
            }
        }
        else if (strcmp(getElementValue(res->node, &len), "HTTP/1.0") == 0)
        {
            purgeElement(&res);
            if ((res = searchTree(NULL, "Connection")) == NULL)
            {
                if (strcmp(getElementValue(res->node, &len), "keep-alive") == 0)
                {
                    keep_alive = 1;
                    purgeElement(&res);
                    return 1;
                }
                purgeElement(&res);
            }
        }
        purgeElement(&res);

    }
    keep_alive = 1;
    return 1;
}