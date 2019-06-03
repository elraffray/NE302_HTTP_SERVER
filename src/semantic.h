#ifndef SEMANTIC_H
#define SEMANTIC_H


extern char methods[][10];
extern int nbMethods;

#define MAX_MAJOR_VERSION 1
#define MAX_MINOR_VERSION 1
#define MAX_QVALUES 10

typedef struct qStruct {
    char name[32];
    float value;
} QStruct;



// compare deux QStruct
int cmpQvalue(const void *a, const void *b);

// remplie et trie le tableau de qstruct depuis la valeur de la rulename;
int setQValuesFrom(char *rulename);

// remplie et trie le tableau de qstruct en respectand la grammaire de Accept
int setAcceptQValuesFrom(char *rulename);

int getKeepAlive();
char *getUri();


char *decodeChunked(char *data);


// utilise le transfer-encoding pour decoder le body de la request
char *decodeBody();

//utilise accept-encoding de la requete pour encoder le body de la réponse
char *encodeBody(char *encoding, int *len);

// renvoie le header content-type approprié
char *getContentType();

//renvoie le mimetype/mimesubtype du fichier passé en parametre
char *getMimeData(char *filename);

//renvoie la version HTTP
char *getResponseVersion();

//renvoie le code de statut de la réponse
char *getStatusCode();

//renvoie le contenu de filename
char *getFileContent(char *filename, int *len);

//renvoie la réponse
char *createResponse();

int isHex(char c);

//décode l'uri (percent encoding, dot segment removal)
void decodeUri();

//renvoie 1 pour keep alive, 0 pour close
int checkConnectionHeader();


#endif