#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>



/* racine de l'arbre */
Node *tree;

/* tableau des charactères spéciaux de tchar */
char tchar[] = { '!', '#', '$', '%', '&', '\'', '*', '+', '-', '.', '^', '_', '`', '|', '~'};
/* taille du tableau tchar */
int nTchar = 15;

/* tableau des charactères de sub-delims */
char subDelims[] = { '!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '='};
/* taille du tableau subDelims */
int nSubDelims = 11;


void initNode(Node *slot, char *name)
{
    slot->ruleName = (char *)malloc((strlen(name) + 1) * sizeof(char));
    strcpy(slot->ruleName, name);

    slot->child = NULL;
    slot->brother = NULL;
    slot->len = 0;
    slot->start = NULL;
    slot->value = NULL;
}

void addChild(Node *n, char *name)
{
    Node *it;
    if (n->child == NULL)
    {
        n->child = (Node *)malloc(sizeof(Node));
        initNode(n->child, name);
        return;
    }
    it = n->child;
    while (it->brother != NULL)
    {
        it = it->brother;
    }

    it->brother = (Node *)malloc(sizeof(Node));
    initNode(it->brother, name);
    return;
}

void deleteChildren(Node *n)
{
    deleteChildrenFromIndex(n, 0);
}

void deleteChildrenFromIndex(Node *n, int k)
{
    int i = 0;
    Node *it = n->child, *itOld = NULL;
    for (i = 0; i < k; i++)
    {
        itOld = it;
        it = it->brother;
    }
    if (itOld != NULL)
        itOld->brother = NULL;
    if(it != NULL)
    {
        while (it != NULL)
        {
            deleteChildren(it);
            itOld = it;
            it = it->brother;
            free(itOld->ruleName);
            free(itOld->value);
            free(itOld);    
        }
    }
}

void setValues(Node *start)
{
    int i;
    Node *it = start->child;
    start->value = (char *)malloc(start->len+1);

    strncpy(start->value, start->start, start->len);
    
    start->value[start->len] = '\0';
    while (it != NULL)
    {
        setValues(it);
        it = it->brother;
    }
}

int validateRequest(char *req, int len)
{
    int res;
    tree = (Node *)malloc(sizeof(Node));
    initNode(tree, "HTTP-message");
    res = validateHttpMessage(&req, tree);
    setValues(tree);
    if (!res)
        deleteChildren(tree);

    return res;
}

int validateHttpMessage(char **req, Node *n)
{
    n->start = *req;
    addChild(n, "start-line");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

int validateStartLine(char **req, Node *n)
{
    n->start = *req;
    addChild(n,"request-line");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }    
    return 1;
}

int validateRequestLine(char **req, Node *n)
{
    n->start = *req;
    addChild(n,"method");
    addChild(n,"SP");
    addChild(n,"request-target");
    addChild(n,"SP");
    addChild(n,"HTTP-version");
    addChild(n,"CRLF");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

int validateMethod(char **req, Node *n)
{
    n->start = *req;
    char *it = *req;

    /* on vérifie que le 1er charactere est bien un tchar */
    if (!isTchar(*it))
        return 0;
    
    it++;
    n->len++;
    while (*it != '\0')
    {
        if (!isTchar(*it))
        {
            *req = it;
            return 1;
        }
        it++;
        n->len++;
    }
    return 1;
    
}

int validateRequestTarget(char **req, Node *n)
{
    n->start = *req;
    addChild(n, "origin-form");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

/* absolute-path [ "?" query ] */
int validateOriginForm(char **req, Node *n)
{
    n->start = *req;
    addChild(n, "absolute-path");
    if (validateChildren(req, n))
    {
    
        addChild(n, "?"); 
        addChild(n, "query");
        if (!validateChildrenStartingFrom(req, n, 1))
            deleteChildrenFromIndex(n, 1);
        return 1;
    }
    deleteChildren(n);
    return 0;
}

/* 1* ( "/" segment ) */
int validateAbsolutePath(char **req, Node *n)
{
    int deleteFrom = 0, lenRead = 0;
    Node *startingPoint;
    n->start = *req;
    addChild(n, "/");
    addChild(n, "segment");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    startingPoint = n->child;
    do 
    {
        n->len += lenRead;
        addChild(n, "/");
        addChild(n, "segment");
        startingPoint = startingPoint->brother->brother;
        deleteFrom+=2;
    } while ((lenRead = validateBrothers(req, startingPoint) > 0));
    deleteChildrenFromIndex(n, deleteFrom);
    return 1;
}



/* * ( pchar / "/" / "?" ) */
int validateQuery(char **req, Node *n)
{
    n->start = *req;
    int found = 0;
    do
    {
        if (**req == '/' || **req == '?')
        {
            (*req)++;
            n->len++;
            found = 1;
        }
        else if (readPchar(req))
        {
            found = 1;
            n->len++;
        }
        else
            found = 0;
    } while (found == 1);
    return 1;    
}

/* HTTP-name "/" DIGIT "." DIGIT */
int validateHttpVersion(char **req, Node *n)
{
    n->start = *req;
    addChild(n, "HTTP-name");
    addChild(n, "/");
    addChild(n, "DIGIT");
    addChild(n, ".");
    addChild(n, "DIGIT");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

/* %x48.54.54.50 */
int validateHttpName(char **req, Node *n)
{
    n->start = *req;
    addChild(n, "H");
    addChild(n, "T");
    addChild(n, "T");
    addChild(n, "P");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

int validateSp(char **req, Node *n)
{
    n->start = *req;
    if (**req == ' ')
    {
        n->len++;
        (*req)++;
        return 1;
    }
    return 0;
}

/* * pchar */
int validateSegment(char **req, Node *n)
{
    n->start = *req;
    while (readPchar(req))
        n->len++;
    return 1;
}


int validateCrlf(char **req, Node *n)
{
    n->start = *req;
    addChild(n, "\r");
    addChild(n, "\n");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}



int validateDigit(char **req, Node *n)
{
    n->start = *req;
    if (isdigit(**req))
    {
        (*req)++;
        n->len++;
        return 1;
    }
    return 0;
}



int isTchar(char c)
{
    int i;
    if (isalpha(c) || isdigit(c))
    {
        return 1;
    }
    for (i = 0; i < nTchar; i++)
    {
        if (c == tchar[i])
            return 1;
    }
    return 0;
}

/* pchar = unreserved / pct-encoded / sub-delims / ":" / "@" */
int readPchar(char **req)
{
    char *backupReq = *req;
    int i;

    /* unreserved */
    if (isalpha(**req) || isdigit(**req) || **req == '-' || **req == '.' || **req == '_' || **req == '~')
    {
        (*req)++;
        return 1;
    }
    /* pct-encoded */
    if (**req == '%')
    {
        (*req)++;
        if (isdigit(**req) || **req == 'A' || **req == 'B' || **req == 'C' || **req == 'D' || **req == 'E' || **req == 'F')
        {
            (*req)++;
            if (isdigit(**req) || **req == 'A' || **req == 'B' || **req == 'C' || **req == 'D' || **req == 'E' || **req == 'F')
            {
                (*req)++;
                return 1;
            }
            (*req)--;
        }
        (*req)--;
    }
    /* sub-delims */
    for (i = 0; i < nSubDelims; i++)
    {
        if (**req == subDelims[i])
        {
            (*req)++;
            return 1;
        }
    }
    if (**req == ':' || **req == '@')
    {
        (*req)++;
        return 1;
    }
    return 0;
}

int validateCharacter(char **req,Node *n, char c)
{
    n->start = *req;
    if (**req == c)
    {
        (*req)++;
        n->len = 1;

        return 1;
    }
    return 0;
}

/* valide la liste de freres commencant a n, retourne le nombre de charactere lus lors de la validation (ou -1) */
int validateBrothers(char **req, Node *n)
{
    int lenRead = 0, tmp;
    Node *it = n;

    while (it != NULL)
    {
        if (strlen(it->ruleName) == 1) /* si la rulename est composé d'un seul charactere */
            tmp = validateCharacter(req, it, it->ruleName[0]); /* on tente de lire ce charactere */
        else
            tmp = (getValidationFunction(it)(req, it)); /* sinon on appelle une fonction de validation */
        if (tmp == 1)
        {
            lenRead += it->len;
        }
        if (tmp == 0)
        {
            return -1;
        }
        it = it->brother;
    }
    return lenRead;
}

int validateChildren(char **req, Node *n)
{
    validateChildrenStartingFrom(req, n, 0);
}

int validateChildrenStartingFrom(char **req, Node *n, int start)
{
    int i, l;
    Node *it = n->child;
    for (i = 0; i < start; i++)
    {
        if (it->brother == NULL)
        {
            printf("Mauvais index de fils %d\n", start);
            return 0;
        }
        it = it->brother;
    }

    l = validateBrothers(req, it);
    if (l == -1) /* si la validation échoue */
    {
        return 0;
    }    
    n->len = l;
    return 1;
}



int(*getValidationFunction(Node *n))(char **req, Node *n)
{
    int i;
    char *name = n->ruleName;
    if (strcmp(name, "HTTP-message") == 0)
    {
        return validateHttpMessage;
    }
    else if (strcmp(name, "start-line") == 0)
    {
        return validateStartLine;
    }
    else if (strcmp(name, "request-line") == 0)
    {
        return validateRequestLine;
    }
    else if (strcmp(name, "method") == 0)
    {
        return validateMethod;
    }
    else if (strcmp(name, "SP") == 0)
    {
        return validateSp;
    }
    else if (strcmp(name, "request-target") == 0)
    {
        return validateRequestTarget;
    }
    else if (strcmp(name, "origin-form") == 0)
    {
        return validateOriginForm;
    }
    else if (strcmp(name, "absolute-path") == 0)
    {
        return validateAbsolutePath;
    }
    else if (strcmp(name, "segment") == 0)
    {
        return validateSegment;
    }
    else if (strcmp(name, "query") == 0)
    {
        return validateQuery;
    }
    else if (strcmp(name, "HTTP-version") == 0)
    {
        return validateHttpVersion;
    }
    else if (strcmp(name, "HTTP-name") == 0)
    {
        return validateHttpName;
    }
    else if (strcmp(name, "CRLF") == 0)
    {
        return validateCrlf;
    }
    else if (strcmp(name, "DIGIT") == 0)
    {
        return validateDigit;
    }

    printf("validation function of %s not found.", n->ruleName);
    return NULL;
}