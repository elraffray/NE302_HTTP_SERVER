#include "api.h"
#include "parser.h"
#include "string.h"
#include <stdlib.h>
#include <stdio.h>

int parseur(char *req, int len)
{
    return validateRequest(req, len);
}

void *getRootTree()
{
    return (void *)tree;
}


_Token *searchTree(void *start,char *name)
{
    Node *tmp, *n = (Node *)start;
    _Token *tok = NULL, *it, *end;
    /* on regarde le noeud courant */
    if (strcmp(n->ruleName, name) == 0)
    {
        printf("FOUND %s of lenght %d\n", name, n->len);
        tok = (_Token *)malloc(sizeof(_Token));
        if (tok == NULL) { printf("erreur malloc token\n"); exit(0); }
        tok->node = start;
        tok->next = NULL;
    }
    end = tok;

    /* on parcourt les fils et leur freres */
    tmp = n->child;
    while (tmp != NULL)
    {
        it = searchTree(tmp, name);
        /* on insere les resultats en queue*/
        if (tok != NULL && it != NULL)
        {
            end->next = it;
            end = it;
        }
        /* on cree la liste si elle est vide */
        else if (it != NULL)
        {
            tok = it;
            end = it;
            while (end->next != NULL)
            {
                end = end->next;
            }
        }
        tmp = tmp->brother;
    }
    
    return tok;    
}

char *getElementTag(void *node,int *len)
{
    Node *n = (Node *)node;
    *len = strlen(n->ruleName);
    return n->ruleName;
}


char *getElementValue(void *node,int *len)
{
    Node *n = (Node *)node;
    *len = n->len;
    return n->value;
}

void purgeElement(_Token **r)
{
    if (*r == NULL)
        return;
    if ((*r)->next != NULL)
    {
        purgeElement(&(*r)->next);
    }
    free(*r);
}

void purgeTree(void *root)
{
    Node *n = (Node *)root;
    deleteChildren(n);
    free(n->ruleName);
    free(n->value);
    free(n);
    root = NULL;
}