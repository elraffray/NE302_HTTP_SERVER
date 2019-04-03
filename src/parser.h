#ifndef PARSER_H
#define PARSER_H

typedef struct node
{
    char *ruleName;
    struct node *child;
    struct node *brother;
    char * start;
    int len;
    char *value;
} Node;



/* racine de l'arbre */
extern Node *tree;
/* tableau des charactères spéciaux de tchar */
extern char tchar[];
/* taille du tableau tchar */
extern int nTchar;
/* tableau des charactères de sub-delims */
extern char subDelims[];
/* taille du tableau subDelims */
extern int nSubDelims;



/* initalise les attributs du node */
void initNode(Node *slot, char *name);

/* ajoute le node en queue de la liste chainee de fils de n */
void addChild(Node *n, char *name);

void deleteChildren(Node *n);
void deleteChildrenFromIndex(Node *n, int k);

/* retourne la fonction de validation du noeud passé en argument (en fonction de sa rulename) */
int(* getValidationFunction(Node *n))(char **req, Node *n);

/* alloue les valeurs des noeuds */
void setValues(Node *start);

/********** FONCTIONS DE VALIDATION **********/

/* valide tous les fils du node n */
int validateChildren(char **req, Node *n);
/* valide tous les freres du node n (inclu) */
int validateBrothers(char **req, Node *n);
/* valide tous les fils du node n, a partir du start-ieme (avec 0 on valide tout les fils) */
int validateChildrenStartingFrom(char **req, Node *n, int start);

int validateRequest(char *req, int len);
int validateHttpMessage(char **req, Node *n);
int validateStartLine(char **req, Node *n);
int validateRequestLine(char **req, Node *n);
int validateMethod(char **req, Node *n);
int validateRequestTarget(char **req, Node *n);
int validateOriginForm(char **req, Node *n);
int validateAbsolutePath(char **req, Node *n);
int validateSegment(char **req, Node *n);
int validateQuery(char **req, Node *n);
int validateHttpVersion(char **req, Node *n);
int validateSp(char **req, Node *n);
int validateCrlf(char **req, Node *n);
int validateDigit(char **req, Node *n);
int validateCharacter(char **req, Node *n, char c);

/* vérifie si le char en argument est un tchar */
int isTchar(char c);
/* tente de lire un pchar dans la requete */
int readPchar(char **req);

/*********************************************/

#endif