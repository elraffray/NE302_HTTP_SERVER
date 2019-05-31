#include <stdio.h>
#include <stdlib.h>
#include <string.h> 





char *readFileReq(char *fn){
    char c;
    char *dt;  
    FILE *f=fopen(fn,"r"); 
    long l; 

    if(f==NULL) {
        printf("File error: %s\n", fn);
        exit(0);
    } 
    fseek(f, 0, SEEK_END);
    l = ftell (f);
    fseek (f, 0, SEEK_SET);
    dt = (char *) malloc (l+1);
    if(dt){
        fread (dt, 1, l, f);
    }
    dt[l]='\0';
    
    fclose(f);
    return dt;
}


int LMT=1;

int main(int argc, char *argv[])
{
    char *req;
    char file[100]; 

    char root1[]="/home/ndiaye/stests/get%d";
    char root2[]="/home/ndiaye/stests/testFile/test%d.txt";

    int start=1;
    int stop=start+LMT;
    int score=0;
    int st=1;

    if (argc != 2 ) { printf("usage: %s <rulename>\n",argv[0]); return 0; }
 
    
    for(int i=start; i<stop; i++){
        sprintf(file,root1,i);
        req = readFileReq(file);  
        printf("### Test %i :",i);  
        if (parseur(req,strlen(req))) { 
            if(!(st && i>5 && i%2==0)) score++;
            printf("Successful parsing \n");
            _Token *r,*tok;
            void *root ;
            root=getRootTree();
            r=searchTree(root,argv[1]);
            tok=r;
            printf("---Results---\n");
            while (tok) {
            int l;
            char *s;
            s=getElementValue(tok->node,&l);
            printf("FOUND [%.*s]\n",l,s);
            tok=tok->next;
            }
            printf("-----End-----\n");
            purgeElement(&r);
            purgeTree(root) ;
        }
        else {
            if(st && i>5 && i%2==0) score++;
            printf("Failed to parse, bad HTTP Request\n\n");
        } 
    
    }
    printf("----------------------------------\n score : %d / %d \n ",score,LMT);
    
    return 1;
}