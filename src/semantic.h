#ifndef SEMANTIC_H
#define SEMANTIC_H


extern char methods[][10];
extern int nbMethods;

char *getUri();
char *getKeepAlive();


int semanticCheck(char *req);
int methodCheck(char *req);
void decodeUri();
char *getResponseVersion();
char *getFileContent(char *filename);

char *createResponse();

int checkHostHeader();
int checkConnectionHeader();



#endif