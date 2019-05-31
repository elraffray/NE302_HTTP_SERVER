#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "api.h"
#include "request.h"
#include "semantic.h"

/// Incluez ici les fichiers d'entête nécessaires pour l'execution de ce programme.
/// La fonction parseur doit être dans un autre fichier .c
/// N'ajouter aucun autre code dans ce fichier.

#define REPONSE1 "HTTP/1.0 200 OK\r\n"
#define REPONSE2 "\r\n"

int main(int argc, char *argv[])
{
    message *requete; 
    char *reponse;


	while ( 1 ) {
		// on attend la reception d'une requete HTTP requete pointera vers une ressource allouée par librequest. 
		if ((requete=getRequest(8080)) == NULL ) return -1; 

		// Affichage de debug 
		printf("#########################################\nDemande recue depuis le client %d\n",requete->clientId); 
		printf("Client [%d] [%s:%d]\n",requete->clientId,inet_ntoa(requete->clientAddress->sin_addr),htons(requete->clientAddress->sin_port));
		printf("Contenu de la demande %.*s\n\n",requete->len,requete->buf);  


        if (parseur(requete->buf, requete->len))
        {
            if (semanticCheck(requete->buf))
            {
                printf("Request valid.\n");
            }
            else
            {
                printf("Request not valid.\n");
            }
        }
        else
        {
            printf("Couldn't parse request.\n");
        }
        reponse = createResponse();
		writeDirectClient(requete->clientId,reponse,strlen(reponse)); 
		//writeDirectClient(requete->clientId,REPONSE2,strlen(REPONSE2)); 
		endWriteDirectClient(requete->clientId); 
        if (getKeepAlive() == 0)
		    requestShutdownSocket(requete->clientId); 
        // on ne se sert plus de requete a partir de maintenant, on peut donc liberer... 
        freeRequest(requete); 
        free(reponse);
	}
	return (1);
}

// int main_old(int argc, char *argv[])
// {
//     //char req[]="GET / HTTP/1.0\r\nReferer: https://developer.mozilla.org/en-US/docs/Web/JavaScript\r\nCookie: yolo=\"swag\"\r\nAccept: text/html, application/xhtml+xml, application/xml;q=0.9, */*;q=0.8\r\nExpect: 100-continue\r\nConnection: keep-alive\r\nContent-Length: 1337\r\nContent-Type: text/html; charset=utf-8\r\nTransfer-Encoding: gzip\r\nHost: developer.cdn.mozilla.net:1337\r\nAccept-Charset: utf-8, iso-8859-1;q=0.5, *;q=0.1\r\nAccept-Language: fr-CH, fr;q=0.9, en;q=0.8, de;q=0.7, *;q=0.5\r\nAccept-Encoding: deflate, gzip;q=1.0, *;q=0.5\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:47.0) Gecko/20100101 Firefox/47.0\r\n\r\n";
//     char *req = getFileContent(argv[2]);
//     //if (argc != 2 ) { printf("usage: %s <rulename>\n",argv[0]); return 0; }
    
//     if (parseur(req,strlen(req))) {
//         _Token *r,*tok;
//         void *root ;
//         root=getRootTree();
//         r=searchTree(root,argv[1]);
//         tok=r;
//         while (tok) {
//             int l;
//             char *s;
//             s=getElementValue(tok->node,&l);
//             printf("FOUND [%.*s]\n",l,s);
//             tok=tok->next;
//         }
//         purgeElement(&r);

//         purgeTree(root) ;
//         printf("Valid\n");

//     }
//     else {
//         printf("Not valid\n");
//         return 0;
//     }
//     free(req);
//     return 1;
// }
