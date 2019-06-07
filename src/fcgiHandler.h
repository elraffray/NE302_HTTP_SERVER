#ifndef FCGIHANDLER_H
#define FCGIHANDLER_H

char *execute(char *filename, int *l);
char *readStdout(int socket, int *l);
void extractHeader(char *header, char **name, char **value);
void readNBytes(int socket, unsigned int n, char* buffer);

#endif