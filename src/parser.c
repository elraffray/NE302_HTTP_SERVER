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

char headers[][32] = {"Cookie-header", "Referer-header", "Accept-header", "Expect-header", "Connection-header", "Content-Length-header", "Content-Type-header", "Transfer-Encoding-header"};
int nbHeaders = 8;

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
        if (it == NULL)
        {
            printf("mauvais index de suppression.\n");
            exit(0);
        }
        itOld = it;
        it = it->brother;
    }
    if (itOld != NULL) //si on ne supprime pas a partir du début, il faut casser le lien de n-1 a n avant de supprimer n
        itOld->brother = NULL;
    while (it != NULL)
    {
        //n->len -= it->len;
        deleteChildren(it);
        itOld = it;
        it = it->brother;
        free(itOld->ruleName);
        free(itOld->value);
        free(itOld);    
    }
    if (k == 0)
        n->child = NULL;
}

void setValues(Node *start)
{
    Node *it = start->child;
    start->value = (char *)malloc(start->len+1);

    //printf("set value of %s, length %d\n", start->ruleName, start->len);
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

/*  start-line * ( header-field CRLF ) CRLF [ message-body ] */
int validateHttpMessage(char **req, Node *n)
{
    int ind = 0;
    n->start = *req;

    addChild(n, "start-line");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    ind++;
    addChild(n, "header-field");
    addChild(n, "CRLF");
    while (validateChildrenStartingFrom(req, n, ind))
    {
        ind+=2;
        addChild(n, "header-field");
        addChild(n, "CRLF");
    }
    deleteChildrenFromIndex(n, ind);

    addChild(n, "CRLF");
    if (!validateChildrenStartingFrom(req, n, ind))
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
    int ind = 0;
    n->start = *req;

    addChild(n, "/");
    addChild(n, "segment");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    ind+=2;
    
    addChild(n, "/");
    addChild(n, "segment");
    while(validateChildrenStartingFrom(req, n, ind))
    {
        ind+=2;
        addChild(n, "/");
        addChild(n, "segment");
    }
    
    deleteChildrenFromIndex(n, ind);

    return 1;
}



/* * ( pchar / "/" / "?" ) */
int validateQuery(char **req, Node *n)
{
    n->start = *req;
    int found = 0, len;
    do
    {
        if (**req == '/' || **req == '?')
        {
            (*req)++;
            n->len++;
            found = 1;
        }
        else if (readPchar(req, &len))
        {
            found = 1;
            n->len += len;
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


int validateHeaderField(char **req, Node *n)
{
    int i;
    n->start = *req;
    for (i = 0; i < nbHeaders; i++)
    {
        addChild(n, headers[i]);
        if (validateChildren(req, n))
            return 1;
        deleteChildren(n);
    }
    return 0;
}


int validateCookieHeader(char **req, Node *n)
{
    n->start = *req;
    addChild(n, "cookie-str");
    addChild(n, ":");
    addChild(n, "OWS");
    addChild(n, "cookie-string");
    addChild(n, "OWS");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}


int validateCookieStr(char **req, Node *n)
{
    n->start = *req;
    int i;
    char *str = "Cookie";
    
    for (i = 0; i < strlen(str); i++)
    {
        if (**req != str[i])
        {
            *req = n->start;
            n->len = 0;
            return 0;
        }
        (*req)++;
    }
    n->len = strlen(str);
    return 1;

}


/* cookie-pair * ( ";" SP cookie-pair ) */
int validateCookieString(char **req, Node *n )
{
    n->start = *req;
    int ind = 0;
    addChild(n, "cookie-pair");
    
    if (validateChildren(req, n))
    {
        do
        {
            if (ind == 0) ind+= 1;
            else ind += 3;
            addChild(n, ";");
            addChild(n, "SP");
            addChild(n, "cookie-pair");
        } while (validateChildrenStartingFrom(req, n, ind));
        deleteChildrenFromIndex(n, ind);
        return 1;
    }
    deleteChildren(n);
    return 0;
}

/* cookie-name "=" cookie-value */
int validateCookiePair(char **req, Node *n)
{
    n->start = *req;
    addChild(n, "cookie-name");
    addChild(n, "=");
    addChild(n, "cookie-value");
    
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

/* 1* tchar */ /* token */
int validateCookieName(char **req, Node *n)
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

/*  ( DQUOTE * cookie-octet DQUOTE ) / * cookie-octet */
int validateCookieValue(char **req, Node *n)
{
    n->start = *req;
    int ind = -1, dquote = 0;
    Node *it;
    // on lit dquote
    addChild(n, "\"");
    if (!validateChildren(req, n))
        deleteChildren(n);
    else
    {
        ind++;
        dquote = 1;
    }
    
    // on lit n cookie-octet 
    do
    {
        addChild(n, "cookie-octet");
        ind++;
    } while (validateChildrenStartingFrom(req, n, ind));
    
    it = n->child;
    while (it != NULL)
    {
        it = it->brother;
    }
    deleteChildrenFromIndex(n, ind);
    
    //on relit dquote si on l'avait lu au début
    if (dquote)
    {
        addChild(n, "\"");
        it = n->child;
        while (it != NULL)
        {
            it = it->brother;
        }
        if (!validateChildrenStartingFrom(req, n, ind))
        {
            deleteChildren(n);
            return 0;
        }
    }
    return 1;
}

/*  %x21 / %x23-2B / %x2D-3A / %x3C-5B / %x5D-7E */
int validateCookieOctet(char **req, Node *n)
{
    n->start = *req;
    if (**req == 21)
    {
        (*req)++;
        n->len++;
        return 1;
    }
    if (**req >= 35 && **req <= 43)
    {
        (*req)++;
        n->len++;
        return 1;
    }
    if (**req >= 45 && **req <= 58)
    {
        (*req)++;
        n->len++;
        return 1;
    }
    if (**req >= 60 && **req <= 91)
    {
        (*req)++;
        n->len++;
        return 1;
    }
    if (**req >= 93 && **req <= 126)
    {
        (*req)++;
        n->len++;
        return 1;
    }
    return 0;
}


int validateRefererHeader(char **req, Node *n )
{
    n->start = *req;
    addChild(n, "referer-str");
    addChild(n, ":");
    addChild(n, "OWS");
    addChild(n, "Referer");
    addChild(n, "OWS");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}


int validateRefererStr(char **req, Node *n )
{
    n->start = *req;
    int i;
    char *str = "Referer";
    
    for (i = 0; i < strlen(str); i++)
    {
        if (**req != str[i])
        {
            *req = n->start;
            n->len = 0;
            return 0;
        }
        (*req)++;
    }
    n->len = strlen(str);
    return 1;
}

/* absolute-uri / partial-uri */
int validateReferer(char **req, Node *n)
{
    n->start = *req;
    addChild(n, "absolute-URI");
    if (validateChildren(req, n))
        return 1;
    deleteChildren(n);
    addChild(n, "partial-URI");
    if (validateChildren(req, n))
        return 1;
    deleteChildren(n);
    return 0;
}

// absolute-URI = scheme ":" hier-part [ "?" query ]
int validateAbsoluteUri(char **req, Node *n)
{
    n->start = *req;
    addChild(n, "scheme");
    addChild(n, ":");
    addChild(n, "hier-part");

    if (validateChildren(req, n))
    {
        addChild(n, "?");
        addChild(n, "query");
        if (!validateChildrenStartingFrom(req, n, 3))
            deleteChildrenFromIndex(n, 3);
        return 1;
    }
    deleteChildren(n);
    return 0;
}

/* ALPHA * ( ALPHA / DIGIT / "+" / "-" / "." ) */
int validateScheme(char **req, Node *n)
{
    n->start = *req;
    if (!isalpha(**req))
        return 0;
    (*req)++;
    n->len++;
    while (isalpha(**req) || isdigit(**req) || **req == '+' || **req == '-' || **req == '.')
    {
        (*req)++;
        n->len++;
    }

    return 1;
}

// hier-part = "//" authority path-abempty / path-absolute / path-rootless / path-empty
int validateHierPart(char **req, Node *n)
{ 
    n->start = *req;
    addChild(n, "/");
    addChild(n, "/");
    addChild(n, "authority");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }

    addChild(n, "path-abempty");
    if (validateChildrenStartingFrom(req, n, 3))
        return 1;
    deleteChildrenFromIndex(n, 3);

    addChild(n, "path-absolute");
    if (validateChildrenStartingFrom(req, n, 3))
        return 1;
    deleteChildrenFromIndex(n, 3);
    
    addChild(n, "path-rootless");
    if (validateChildrenStartingFrom(req, n, 3))
        return 1;
    deleteChildrenFromIndex(n, 3);
    
    addChild(n, "path-empty");
    if (validateChildrenStartingFrom(req, n, 3))
        return 1;
    deleteChildrenFromIndex(n, 3);

    return 0;

}

/* [ userinfo "@" ] host [ ":" port ] */
int validateAuthority(char **req, Node *n)
{
    int ind = 0;
    n->start = *req;
    addChild(n, "userinfo");
    addChild(n, "@");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        *req = n->start;
    }
    else
        ind = 2;

    addChild(n, "host");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildren(n);
        return 0;
    }
    ind++;
    addChild(n, ":");
    addChild(n, "port");
    //printf("AUTH 2 %d\n", n->len); //MAGIC PRINTF
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildrenFromIndex(n, ind);
        return 1;
    }
    printf("AUTH 3 %d\n", n->len);

    return 1;
}

//  * ( unreserved / pct-encoded / sub-delims / ":" )
int validateUserInfo(char **req, Node *n)
{
    int len = 0;
    n->start = *req;
    while (**req != '@') // userinfo est composé de characteres similaire a des pchar, mais sans l'arobase
    {
        if (readPchar(req, &len))
        {

            n->len += len;
            len = 0;
        }
        else
            break;        
    }
    return 1;
}

// host = IP-literal / IPv4address / reg-name
int validateHost(char **req, Node *n){
    n->start = *req;
    addChild(n, "IP-literal");
    if (validateChildren(req, n))
        return 1;
    deleteChildren(n);

    addChild(n, "IPv4address");
    if (validateChildren(req, n))
        return 1;
    deleteChildren(n);

    addChild(n, "reg-name");
    if (validateChildren(req, n))
        return 1;
    deleteChildren(n);

    return 0;
}

//IP-literal =  "[" ( IPv6address / IPvFuture ) "]"
int validateIPliteral(char **req, Node *n)
{
    n->start = *req;
    addChild(n, "[");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    addChild(n, "IPv6address");
    if (!validateChildrenStartingFrom(req, n, 1))
    {
        deleteChildrenFromIndex(n, 1);
        addChild(n, "IPvFuture");
        if (!validateChildrenStartingFrom(req, n, 1))
        {
            deleteChildren(n);
            return 0;
        }
        addChild(n, "]");
        if (validateChildrenStartingFrom(req, n, 2))
            return 1;
        deleteChildren(n);
        return 0;        
    }
    return 1;
}

/*  6 ( h16 ":" ) ls32 / "::" 5 ( h16 ":" ) ls32 / [ h16 ] "::" 4 ( h16 ":" ) ls32 / [ h16 *1 ( ":" h16 ) ] "::" 3 ( h16 ":" ) ls32 / [ h16 *2 ( ":" h16 ) ] "::" 2 ( h16 ":" ) ls32 / [ h16 *3 ( ":"
h16 ) ] "::" h16 ":" ls32 / [ h16 *4 ( ":" h16 ) ] "::" ls32 / [ h16 *5 ( ":" h16 ) ] "::" h16 / [ h16 *6 ( ":" h16 ) ] "::" */
int validateIPv6address(char **req, Node *n)
{
    n->start = *req;
    int i, ind = 0;

    // 6 ( h16 ":" )
    for (i = 0; i < 6; i++)
    {
        addChild(n, "h16");
        addChild(n, ":");
    }
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    ind = 12;

    // ls32 / "::"
    addChild(n, "ls32");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildrenFromIndex(n, ind);
        addChild(n, ":");
        addChild(n, ":");
        if (!validateChildrenStartingFrom(req, n, ind))
        {
            deleteChildren(n);
            return 0;
        }
        ind += 2;
    }
    else
        ind += 1;

    // 5 ( h16 ":" )
    for (i = 0; i < 5; i++)
    {
        addChild(n, "h16");
        addChild(n, ":");
    }
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildren(n);
        return 0;
    }
    ind += 10;

    // ls32 / [ h16 ]
    addChild(n, "ls32");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildrenFromIndex(n, ind);
        addChild(n, "h16");
        if (validateChildrenStartingFrom(req, n, ind))
            ind += 1;
    }
    else
        ind++;

    // "::"
    addChild(n, ":");
    addChild(n, ":");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildren(n);
        return 0;
    }
    ind += 2;

    // 4 ( h16 ":" )
    for (i = 0; i < 4; i++)
    {
        addChild(n, "h16");
        addChild(n, ":");
    }
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildren(n);
        return 0;
    }
    ind += 8;

    // ls32 / [ h16 *1 ( ":" h16 ) ]
    addChild(n, "ls32");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildrenFromIndex(n, ind);
        addChild(n, "h16");
        if (!validateChildrenStartingFrom(req, n, ind))
        {
            deleteChildrenFromIndex(n, ind);
        }
        else
        {
            ind++;
            addChild(n, ":");
            addChild(n, "h16");
            if (!validateChildrenStartingFrom(req, n, ind))
            {
                deleteChildrenFromIndex(n, ind);
            }
            else
                ind+= 2;
        }
    }
    else
        ind++;

    // "::"
    addChild(n, ":");
    addChild(n, ":");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildren(n);
        return 0;
    }
    ind += 2;

    // 3 ( h16 ":" )
    for (i = 0; i < 3; i++)
    {
        addChild(n, "h16");
        addChild(n, ":");
    }
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildren(n);
        return 0;
    }
    ind += 6;

    // ls32 / [ h16 *2 ( ":" h16 ) ]
    addChild(n, "ls32");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildrenFromIndex(n, ind);
        addChild(n, "h16");
        if (!validateChildrenStartingFrom(req, n, ind))
        {
            deleteChildrenFromIndex(n, ind);
        }
        else
        {
            ind++;
            for (i = 0; i < 2; i++)
            {
                addChild(n, ":");
                addChild(n, "h16");
                if (!validateChildrenStartingFrom(req, n, ind))
                {
                    deleteChildrenFromIndex(n, ind);
                    break;
                }
                else
                    ind+= 2;
            }
        }
    }
    else
        ind++;
    
    // "::"
    addChild(n, ":");
    addChild(n, ":");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildren(n);
        return 0;
    }
    ind += 2;

    // 2 ( h16 ":" )
    for (i = 0; i < 2; i++)
    {
        addChild(n, "h16");
        addChild(n, ":");
    }
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildren(n);
        return 0;
    }
    ind += 6;

    // ls32 / [ h16 *3 ( ":" h16 ) ]
    addChild(n, "ls32");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildrenFromIndex(n, ind);
        addChild(n, "h16");
        if (!validateChildrenStartingFrom(req, n, ind))
        {
            deleteChildrenFromIndex(n, ind);
        }
        else
        {
            ind++;
            for (i = 0; i < 3; i++)
            {
                addChild(n, ":");
                addChild(n, "h16");
                if (!validateChildrenStartingFrom(req, n, ind))
                {
                    deleteChildrenFromIndex(n, ind);
                    break;
                }
                else
                    ind+= 2;
            }
        }
    }
    else
        ind++;

    // "::" h16 ":" 
    addChild(n, ":");
    addChild(n, ":");
    addChild(n, "h16");
    addChild(n, ":");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildren(n);
        return 0;
    }
    else
        ind += 4;
    
    // ls32 / [ h16 *4 ( ":" h16 ) ]
    addChild(n, "ls32");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildrenFromIndex(n, ind);
        addChild(n, "h16");
        if (!validateChildrenStartingFrom(req, n, ind))
        {
            deleteChildrenFromIndex(n, ind);
        }
        else
        {
            ind++;
            for (i = 0; i < 4; i++)
            {
                addChild(n, ":");
                addChild(n, "h16");
                if (!validateChildrenStartingFrom(req, n, ind))
                {
                    deleteChildrenFromIndex(n, ind);
                    break;
                }
                else
                    ind+= 2;
            }
        }
    }
    else
        ind++;

    // "::"
    addChild(n, ":");
    addChild(n, ":");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildren(n);
        return 0;
    }
    ind += 2;

    // ls32 / [ h16 *5 ( ":" h16 ) ]
    addChild(n, "ls32");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildrenFromIndex(n, ind);
        addChild(n, "h16");
        if (!validateChildrenStartingFrom(req, n, ind))
        {
            deleteChildrenFromIndex(n, ind);
        }
        else
        {
            ind++;
            for (i = 0; i < 5; i++)
            {
                addChild(n, ":");
                addChild(n, "h16");
                if (!validateChildrenStartingFrom(req, n, ind))
                {
                    deleteChildrenFromIndex(n, ind);
                    break;
                }
                else
                    ind+= 2;
            }
        }
    }
    else
        ind++;

    // "::"
    addChild(n, ":");
    addChild(n, ":");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildren(n);
        return 0;
    }
    ind += 2;

    // h16 / [ h16 *6 ( ":" h16 ) ]
    addChild(n, "h16");
    if (validateChildrenStartingFrom(req, n, ind))
    {
        ind++;
        for (i = 0; i < 6; i++)
        {
            addChild(n, ":");
            addChild(n, "h16");
            if (!validateChildrenStartingFrom(req, n, ind))
            {
                deleteChildrenFromIndex(n, ind);
                break;
            }
            else
                ind+= 2;
        }
    }
    else
        deleteChildrenFromIndex(n, ind);
    

    // "::"
    addChild(n, ":");
    addChild(n, ":");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildren(n);
        return 0;
    }
    ind += 2;

    return 1;
}


// 1*4 HEXDIG 
int validateH16(char **req, Node *n)
{
    int i;
    n->start = *req;
    if (isdigit(**req) || **req == 'A' || **req == 'B' || **req == 'C' || **req == 'D' || **req == 'E' || **req == 'F')
    {
        (*req)++;
        n->len++;
        for (i = 1; i<4; i++)
        {
            if (isdigit(**req) || **req == 'A' || **req == 'B' || **req == 'C' || **req == 'D' || **req == 'E' || **req == 'F')
            {
                (*req)++;
                n->len++;
            }
            else
                break;
        }
        return 1;
    }
    return 0;
}


// ( h16 ":" h16 ) / IPv4address
int validateLS32(char **req, Node *n)
{
    n->start = *req;
    addChild(n, "h16");
    addChild(n, ":");
    addChild(n, "h16");
    if (validateChildren(req, n))
        return 1;
    deleteChildren(n);

    addChild(n, "IPv4address");
    if (validateChildren(req, n))
        return 1;
    deleteChildren(n);
    return 0;
}


//  "v" 1* HEXDIG "." 1* ( unreserved / sub-delims / ":" )
int validateIPvFuture(char **req, Node *n)
{
    n->start = *req;

    // "v"
    if (**req != 'v')
    {
        return 0;
    }
    n->len++;
    (*req)++;

    if (isdigit(**req) || **req == 'A' || **req == 'B' || **req == 'C' || **req == 'D' || **req == 'E' || **req == 'F')
    {
        n->len++;
        (*req)++;
        while (isdigit(**req) || **req == 'A' || **req == 'B' || **req == 'C' || **req == 'D' || **req == 'E' || **req == 'F')
        {
            n->len++;
            (*req)++;
        }
    }

    if (**req != '.')
    {
        *req = n->start;
        return 0;
    }
    n->len++;
    (*req)++;

    if (isUnreserved(**req) || isSubDelims(**req) || **req == ':')
    {
        n->len++;
        (*req)++;
        while (isUnreserved(**req) || isSubDelims(**req) || **req == ':')
        {
            n->len++;
            (*req)++;
        }
        return 1;
    }
    else
    {
        *req = n->start;
        return 0;
    }

}





//IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
int validateIPv4address(char **req, Node *n)
{
    n->start = *req;
    addChild(n, "dec-octet");
    addChild(n, ".");
    addChild(n, "dec-octet");
    addChild(n, ".");
    addChild(n, "dec-octet");
    addChild(n, ".");
    addChild(n, "dec-octet");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

// "25" %x30-35 / "2" %x30-34 DIGIT / "1" 2 DIGIT / %x31-39 DIGIT / DIGIT
int validateDecOctet(char **req, Node *n)
{
    n->start = *req;

    // "25" %x30-35
    if (**req == '2')
    {
        (*req)++;
        n->len++;
        if (**req == '5')
        {
            (*req)++;
            n->len++;
            if (**req >= 48 && **req <= 53)
            {
                (*req)++;
                n->len++;
                return 1;
            }
        }
    }
    *req = n->start;

    // "2" %x30-34 DIGIT
    if (**req =='2')
    {
        (*req)++;
        n->len++;
        if (**req >= 48 && **req <= 52)
        {
            (*req)++;
            n->len++;
            if (isdigit(**req))
            {
                (*req)++;
                n->len++;
                return 1;
            }
        }
    }
    *req = n->start;

    // "1" 2 DIGIT
    if (**req == '1')
    {
        (*req)++;
        n->len++;
        if (isdigit(**req))
        {
            (*req)++;
            n->len++;
            if (isdigit(**req))
            {
                (*req)++;
                n->len++;
                return 1;
            }    
        }
    }
    *req = n->start;

    // %x31-39 DIGIT
    if (**req >= 49 && **req <= 57)
    {
        (*req)++;
        n->len++;
        if (isdigit(**req))
        {
            (*req)++;
            n->len++;
            return 1;
        }
    }
    *req = n->start;

    if (isdigit(**req))
    {
        (*req)++;
        n->len++;
        return 1;
    }

    *req = n->start;
    return 0;    
}



// * ( unreserved / pct-encoded / sub-delims )
int validateRegName(char **req, Node *n) // basically * pchar without ":" and "@"
{
    int len = 0;
    n->start = *req;

    while (**req != ':' && **req != '@')
    {
        if (readPchar(req, &len))
        {
            n->len += len;
            len = 0;
        }
        else
            break;        
    }
    return 1;
}


// port = * DIGIT
int validatePort(char **req, Node *n)
{
    n->start = *req;
    while(isdigit(req))
        n->len++;
    return 1;
}

//  * ( "/" segment )
int validatePathAbempty(char **req, Node *n)
{
    int ind = 0;
    n->start = *req;
    addChild(n, "/");
    addChild(n, "segment");
    while (validateChildrenStartingFrom(req, n, ind))
    {
        ind+=2;
        addChild(n, "/");
        addChild(n, "segment");
    }
    deleteChildrenFromIndex(n, ind);
    return 1;
    
}

//  "/" [ segment-nz * ( "/" segment ) ]
int validatePathAbsolute(char **req, Node *n)
{
    int ind = 0;
    n->start = *req;
    addChild(n, "/");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    ind++;
    addChild(n, "segment-nz");
    if (validateChildrenStartingFrom(req, n, ind))
    {
        ind++;
        addChild(n, "/");
        addChild(n, "segment");
        while (validateChildrenStartingFrom(req, n, ind))
        {
            ind+=2;
            addChild(n, "/");
            addChild(n, "segment");
        }
        deleteChildrenFromIndex(n, ind);

    }
    return 1;

    
}

// 1* pchar
int validateSegmentNz(char **req, Node *n)
{
    int len;
    n->start = *req;

    if (!readPchar(req, &len))
        return 0;
    n->len += len;
    while (readPchar(req, &len))
        n->len += len;
    return 1;
}

// segment-nz * ( "/" segment )
int validatePathRootless(char **req, Node *n)
{
    int ind = 0;
    n->start = *req;

    addChild(n, "segment-nz");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    ind++;

    addChild(n, "/");
    addChild(n, "segment");
    while (validateChildrenStartingFrom(req, n, ind))
    {
        ind+=2;
        addChild(n, "/");
        addChild(n, "segment");
    }
    deleteChildrenFromIndex(n, ind);
    return 1;
}



int validatePathEmpty(char **req, Node *n)
{
    n->start = *req;
    return 1;
}


// relative-part [ "?" query ]
int validatePartialUri(char **req, Node *n)
{
    n->start = *req;

    addChild(n, "relative-part");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    addChild(n, "?"); 
    addChild(n, "query");
    if (!validateChildrenStartingFrom(req, n, 1))
        deleteChildrenFromIndex(n, 1);
    return 1;

}


// relative-part = "//" authority path-abempty / path-absolute / path-noscheme / path-empty
int validateRelativePart(char **req, Node *n)
{ 
    n->start = *req;
    addChild(n, "/");
    addChild(n, "/");
    addChild(n, "authority");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }

    addChild(n, "path-abempty");
    if (validateChildrenStartingFrom(req, n, 3))
        return 1;
    deleteChildrenFromIndex(n, 1);

    addChild(n, "path-absolute");
    if (validateChildrenStartingFrom(req, n, 3))
        return 1;
    deleteChildrenFromIndex(n, 1);
    
    addChild(n, "path-noscheme");
    if (validateChildrenStartingFrom(req, n, 3))
        return 1;
    deleteChildrenFromIndex(n, 1);
    
    addChild(n, "path-empty");
    if (validateChildrenStartingFrom(req, n, 3))
        return 1;
    deleteChildrenFromIndex(n, 1);

    return 0;

}

// segment-nz-nc * ( "/" segment )
int validatePathNoscheme(char **req, Node *n)
{
    int ind = 0;
    n->start = *req;

    addChild(n, "segment-nz-nc");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    ind++;

    addChild(n, "/");
    addChild(n, "segment");
    while (validateChildrenStartingFrom(req, n, ind))
    {
        ind+=2;
        addChild(n, "/");
        addChild(n, "segment");
    }
    deleteChildrenFromIndex(n, ind);
    return 1;

}

//  1* ( unreserved / pct-encoded / sub-delims / "@" )
int validateSegmentNzNc(char **req, Node *n)
{
    int len;
    n->start = *req;
    if (**req != '@' && readPchar(req, &len))
    {
        n->len += len;
        while (**req != '@' && readPchar(req, &len))
            n->len += len;
        
        return 1;
    }
    return 0;
}


int validateAcceptHeader(char **req, Node *n )
{
    n->start = *req;
    addChild(n, "accept-str");
    addChild(n, ":");
    addChild(n, "OWS");
    addChild(n, "Accept");
    addChild(n, "OWS");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

/*  [ ( "," / ( media-range [ accept-params ] ) ) * ( OWS "," [ OWS ( media-range [ accept-params ] ) ] ) ] */
int validateAccept(char **req, Node *n )
{
    int ind = 0, res = 1;
    n->start = *req;

    // ( "," / ( media-range [ accept-params ] ) )
    addChild(n, ",");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        addChild(n, "media-range");
        if (!validateChildren(req, n))
        {
            deleteChildren(n);
            return 0;
        }
        ind++;
        addChild(n, "accept-params");
        if (!validateChildrenStartingFrom(req, n, ind))
        {
            deleteChildrenFromIndex(n, ind);
        }
        else
            ind++;
    }
    else
        ind++;

    // * ( OWS "," [ OWS ( media-range [ accept-params ] ) ] )
    do
    {
        addChild(n, "OWS");
        addChild(n, ",");
        if (!validateChildrenStartingFrom(req, n, ind))
        {
            deleteChildrenFromIndex(n, ind);
            res = 0;
        }
        else
        {
            res = 1;
            ind += 2;
            addChild(n, "OWS");
            addChild(n, "media-range");
            if (!validateChildrenStartingFrom(req, n, ind))
            {
                deleteChildrenFromIndex(n, ind);
            }
            else
            {
                ind += 2;
                addChild(n, "accept-params");
                if (!validateChildrenStartingFrom(req, n, ind))
                {
                    deleteChildrenFromIndex(n, ind);
                }
                else
                    ind+=1;
            }
        } 
    } while (res);

    return 1;
}


int validateAcceptStr(char **req, Node *n )
{
    n->start = *req;
    int i;
    char *str = "Accept";
    
    for (i = 0; i < strlen(str); i++)
    {
        if (**req != str[i])
        {
            *req = n->start;
            n->len = 0;
            return 0;
        }
        (*req)++;
    }
    n->len = strlen(str);
    return 1;

}

//  ( "*/*" / ( type "/" subtype ) / ( type "/*" ) ) * ( OWS ";" OWS parameter )
int validateMediaRange(char **req, Node *n)
{
    int ind = 0;
    n->start = *req;

    // "*/*"
    addChild(n, "*");
    addChild(n, "/");
    addChild(n, "*");
    if (!validateChildren(req, n))
    {
        // ( type "/" subtype )
        deleteChildren(n);
        addChild(n, "type");
        addChild(n, "/");
        addChild(n, "subtype");
        if (!validateChildren(req, n))
        {
            // ( type "/*" ) 
            deleteChildren(n);
            addChild(n, "type");
            addChild(n, "/");
            addChild(n, "*");
            if (!validateChildren(req, n))
            {
                deleteChildren(n);
                return 0;
            }
            else
            {
                ind += 3;
            }
        }
        else
            ind += 3;
    }
    else
        ind += 3;

    // * ( OWS ";" OWS parameter )
    addChild(n, "OWS");
    addChild(n, ";");
    addChild(n, "OWS");
    addChild(n, "parameter");
    while (validateChildrenStartingFrom(req, n, ind))
    {
        ind += 4;
        addChild(n, "OWS");
        addChild(n, ";");
        addChild(n, "OWS");
        addChild(n, "parameter");
    }
    return 1;
}

// token
int validateType(char **req, Node *n)
{
    n->start = *req;
    addChild(n, "token");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;

}

// token
int validateSubtype(char **req, Node *n)
{
    n->start = *req;
    addChild(n, "token");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

// token "=" ( token / quoted-string )
int validateParameter(char **req, Node *n)
{
    int ind = 0;
    n->start = *req;
    addChild(n, "token");
    addChild(n, "=");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    ind += 2;
    
    addChild(n, "token");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildrenFromIndex(n, ind);
        addChild(n, "quoted-string");
        if (!validateChildrenStartingFrom(req, n, ind))
        {
            deleteChildren(n);
            return 0;
        }
    }
    return 1;
}

//  DQUOTE * ( qdtext / quoted-pair ) DQUOTE
int validateQuotedString(char **req, Node *n)
{
    int ind = 0, res = 1;
    n->start = *req;

    addChild(n, "\"");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    ind += 1;
    do
    {
        addChild(n, "qdtext");
        res = validateChildrenStartingFrom(req, n, ind);
        if (!res)
        {
            deleteChildrenFromIndex(n, ind);
            addChild(n, "quoted-pair");
            if (!validateChildrenStartingFrom(req, n, ind))
            {
                deleteChildrenFromIndex(n, ind);
                res = 0;
            }
            else
                ind++;
        }
        else
            ind++;

    } while (res);

    addChild(n, "\"");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

// HTAB / SP / "!" / %x23-5B / %x5D-7E / obs-text
int validateQdText(char **req, Node *n)
{
    n->start = *req;
    if (**req == '\t' || **req == ' ' || **req == '!' || (**req >= 35 && **req <= 91) || (**req >= 93 && **req <= 126) || (**req >= 128 && **req <= 255))
    {
        n->len++;
        return 1;
    }
    return 0;
}

//  "\" ( HTAB / SP / VCHAR / obs-text )
int validateQuotedPair(char **req, Node *n)
{
    n->start = *req;
    if (**req != '\\')
        return 0;
    n->len++;

    if (**req == '\t' || **req == ' ' || (**req >= 33 && **req <= 126) || (**req >= 128 && **req <= 255))
    {
        n->len++;
        return 1;
    }
    return 0;
}

// weight * accept-ext
int validateAcceptParams(char **req, Node *n)
{
    int ind = 0;
    n->start = *req;

    addChild(n, "weight");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    ind++;
    addChild(n, "accept-ext");
    while (validateChildrenStartingFrom(req, n, ind))
    {
        ind++;
        addChild(n, "accept-ext");
    }

    return 1;
}

// OWS ";" OWS "q=" qvalue
int validateWeight(char **req, Node *n)
{
    n->start = *req;
    
    addChild(n, "OWS");
    addChild(n, ";");
    addChild(n, "OWS");
    addChild(n, "q");
    addChild(n, "=");
    addChild(n, "qvalue");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

// ( "0" [ "." *3 DIGIT ] ) / ( "1" [ "." *3 "0" ] )
int validateQvalue(char **req, Node *n)
{
    int i;
    n->start = *req;
    if (**req == '0')
    {
        (*req)++;
        n->len++;
        if (**req == '.')
        {
            (*req)++;
            n->len++;
            for (i = 0; i < 3; i++)
            {
                if (isdigit(**req))
                {
                    (*req)++;
                    n->len++;
                }
            }
            return 1;
        }
    }
    else if (**req == '1')
    {
        (*req)++;
        n->len++;
        if (**req == '.')
        {
            (*req)++;
            n->len++;
            for (i = 0; i < 3; i++)
            {
                if (**req == '0')
                {
                    (*req)++;
                    n->len++;
                }
            }
            return 1;
        }
    }
    return 0;
}

// OWS ";" OWS token [ "=" ( token / quoted-string ) ]
int validateAcceptExt(char **req, Node *n)
{
    int ind = 0;
    n->start = *req;

    addChild(n, "OWS");
    addChild(n, ";");
    addChild(n, "OWS");
    addChild(n, "token");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    ind += 4;

    addChild(n, "=");
    if (validateChildrenStartingFrom(req, n, ind))
    {
        ind++;
        addChild(n, "token");
        if (validateChildrenStartingFrom(req, n, ind))
            return 1;
        deleteChildrenFromIndex(n, ind);
        addChild(n, "quoted-string");
        if (validateChildrenStartingFrom(req, n, ind))
            return 1;
        deleteChildrenFromIndex(n, ind);
        return 0;
    }
    deleteChildrenFromIndex(n, ind);
    return 1;

}


int validateExpectHeader(char **req, Node * n)
{
    n->start = *req;
    addChild(n, "expect-str");
    addChild(n, ":");
    addChild(n, "OWS");
    addChild(n, "Expect");
    addChild(n, "OWS");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}


int validateExpectStr(char **req, Node * n)
{
    n->start = *req;
    int i;
    char *str = "Expect";
    
    for (i = 0; i < strlen(str); i++)
    {
        if (**req != str[i])
        {
            *req = n->start;
            n->len = 0;
            return 0;
        }
        (*req)++;
    }
    n->len = strlen(str);
    return 1;
}

int validateExpect(char **req, Node * n)
{
    n->start = *req;
    int i;
    char *str = "100-continue";
    
    for (i = 0; i < strlen(str); i++)
    {
        if (**req != str[i])
        {
            *req = n->start;
            n->len = 0;
            return 0;
        }
        (*req)++;
    }
    n->len = strlen(str);
    return 1;
}



int validateConnectionHeader(char **req, Node * n)
{
    n->start = *req;
    addChild(n, "connection-str");
    addChild(n, ":");
    addChild(n, "OWS");
    addChild(n, "Connection");
    addChild(n, "OWS");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

int validateConnectionStr(char **req, Node * n)
{
    n->start = *req;
    int i;
    char *str = "Connection";
    
    for (i = 0; i < strlen(str); i++)
    {
        if (**req != str[i])
        {
            *req = n->start;
            n->len = 0;
            return 0;
        }
        (*req)++;
    }
    n->len = strlen(str);
    return 1;
}

// * ( "," OWS ) connection-option * ( OWS "," [ OWS connection-option ] )
int validateConnection(char **req, Node * n)
{
    int ind = 0, res = 1;
    n->start = *req;
    
    addChild(n, ",");
    addChild(n, "OWS");
    while (validateChildrenStartingFrom(req, n, ind))
    {
        ind += 2;
        addChild(n, ",");
        addChild(n, "OWS");
    }
    deleteChildrenFromIndex(n, ind);

    addChild(n, "connection-option");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildrenFromIndex(n, ind);
        return 0;
    }
    ind++;

    do
    {
        addChild(n, "OWS");
        addChild(n, ",");
        if (!validateChildrenStartingFrom(req, n, ind))
        {
            deleteChildrenFromIndex(n, ind);
            res = 0;
        }
        else
        {
            ind+= 2;
            addChild(n, "OWS");
            addChild(n, "connection-option");
            if (!validateChildrenStartingFrom(req, n, ind))
                deleteChildrenFromIndex(n, ind);
        }


    } while (res);
    
    return 1;

}

int validateConnectionOption(char **req, Node * n)
{
    n->start = *req;
    addChild(n, "token");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}


int validateContentLengthHeader(char **req, Node * n)
{
    n->start = *req;
    addChild(n, "content-length-str");
    addChild(n, ":");
    addChild(n, "OWS");
    addChild(n, "Content-Length");
    addChild(n, "OWS");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

int validateContentLengthStr(char **req, Node * n)
{
    n->start = *req;
    int i;
    char *str = "Content-Length";
    for (i = 0; i < strlen(str); i++)
    {
        if (**req != str[i])
        {
            *req = n->start;
            n->len = 0;
            return 0;
        }
        (*req)++;
    }
    n->len = strlen(str);
    return 1;
}

// 1* digit
int validateContentLength(char **req, Node * n)
{
    n->start = *req;
    if (isdigit(**req))
    {
        (*req)++;
        n->len++;
        while (isdigit(**req))
        {
            (*req)++;
            n->len++;
        }
        return 1;
    }
        printf("here\n");
    return 0;
}


int validateContentTypeHeader(char **req, Node * n)
{
    n->start = *req;
    addChild(n, "content-type-str");
    addChild(n, ":");
    addChild(n, "OWS");
    addChild(n, "Content-Type");
    addChild(n, "OWS");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

int validateContentTypeStr(char **req, Node * n)
{
    n->start = *req;
    int i;
    char *str = "Content-Type";
    for (i = 0; i < strlen(str); i++)
    {
        if (**req != str[i])
        {
            *req = n->start;
            n->len = 0;
            return 0;
        }
        (*req)++;
    }
    n->len = strlen(str);
    return 1;
}

// media-type
int validateContentType(char **req, Node * n)
{
    n->start = *req;

    addChild(n, "media-type");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

//  type "/" subtype * ( OWS ";" OWS parameter )
int validateMediaType(char **req, Node * n)
{
    int ind = 0;
    n->start = *req;

    addChild(n, "type");
    addChild(n, "/");
    addChild(n, "subtype");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    ind += 3;

    addChild(n, "OWS");
    addChild(n, ";");
    addChild(n, "OWS");
    addChild(n, "parameter");
    while (validateChildrenStartingFrom(req, n, ind))
    {
        ind += 4;
        addChild(n, "OWS");
        addChild(n, ";");
        addChild(n, "OWS");
        addChild(n, "parameter");
    }
    deleteChildrenFromIndex(n, ind);
    return 1;
    
}

int validateTransferEncodingHeader(char **req, Node * n)
{
    n->start = *req;
    addChild(n, "transfer-encoding-str");
    addChild(n, ":");
    addChild(n, "OWS");
    addChild(n, "Transfer-Encoding");
    addChild(n, "OWS");

    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

int validateTransferEncodingStr(char **req, Node * n)
{
    n->start = *req;
    int i;
    char *str = "Transfer-Encoding";
    for (i = 0; i < strlen(str); i++)
    {
        if (**req != str[i])
        {
            *req = n->start;
            n->len = 0;
            return 0;
        }
        (*req)++;
    }
    n->len = strlen(str);
    return 1;
}

// * ( "," OWS ) transfer-coding * ( OWS "," [ OWS transfer-coding ] )
int validateTransferEncoding(char **req, Node * n)
{
    int ind = 0, res = 1;
    n->start = *req;

    addChild(n, ",");
    addChild(n, "OWS");
    while (validateChildrenStartingFrom(req, n, ind))
    {
        ind += 2;
        addChild(n, ",");
        addChild(n, "OWS");
    }
    deleteChildrenFromIndex(n, ind);

    addChild(n, "transfer-coding");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildren(n);
        return 0;
    }
    ind++;
    do
    {
        addChild(n, "OWS");
        addChild(n, ",");
        if (!validateChildrenStartingFrom(req, n, ind))
        {
            deleteChildrenFromIndex(n, ind);
            res = 0;
        }
        else
        {
            res = 1;
            ind += 2;
            addChild(n, "OWS");
            addChild(n, "transfer-coding");
            if (!validateChildrenStartingFrom(req, n, ind))
                deleteChildrenFromIndex(n, ind);
            else
                ind += 2;
        }
    } while (res);
    
    return 1;
}


// "chunked" / "compress" / "deflate" / "gzip" / transfer-extension
int validateTransferCoding(char **req, Node * n)
{
    n->start = *req;

    if (!readString(req, "chunked"))
    {
        n->len = 0;
    }
    else
    {
        n->len = strlen("chunked");
        return 1;
    }

    if (!readString(req, "compress"))
    {
        n->len = 0;
    }
    else
    {
        n->len = strlen("compress");
        return 1;
    }
    
    if (!readString(req, "deflate"))
    {
        n->len = 0;
    }
    else
    {
        n->len = strlen("deflate");
        return 1;
    }
    
    if (!readString(req, "gzip"))
    {
        n->len = 0;
    }
    else
    {
        n->len = strlen("gzip");
        return 1;
    }
    
    addChild(n, "transfer-extension");
    if (!validateChildrenStartingFrom(req, n, 0))
    {
        deleteChildren(n);
        return 0;
    }
    return 1;
}

// token * ( OWS ";" OWS transfer-parameter )
int validateTransferExtension(char **req, Node * n)
{
    int ind = 0;
    n->start = *req;

    addChild(n, "token");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    ind++;
    addChild(n, "OWS");
    addChild(n, ";");
    addChild(n, "OWS");
    addChild(n, "transfer-parameter");
    while (validateChildrenStartingFrom(req, n, ind))
    {
        ind += 4;
        addChild(n, "OWS");
        addChild(n, ";");
        addChild(n, "OWS");
        addChild(n, "transfer-parameter");
    }
    deleteChildrenFromIndex(n, ind);
    return 1;
}

//  token BWS "=" BWS ( token / quoted-string )
int validateTransferParameter(char **req, Node * n)
{
    int ind = 0;
    n->start = *req;
    
    addChild(n, "token");
    addChild(n, "OWS");
    addChild(n, "=");
    addChild(n, "OWS");
    if (!validateChildren(req, n))
    {
        deleteChildren(n);
        return 0;
    }
    ind+=4;

    addChild(n, "token");
    if (!validateChildrenStartingFrom(req, n, ind))
    {
        deleteChildrenFromIndex(n, ind);
        addChild(n, "quoted-string");
        if (!validateChildrenStartingFrom(req, n, ind))
        {
            deleteChildrenFromIndex(n, ind);
            return 0;
        }
        return 1;
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
    int len;
    n->start = *req;
    while (readPchar(req, &len))
        n->len += len;
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

// *( SP /  HTAB ) 
int validateOws(char **req, Node *n)
{
    n->start = *req;
    while (**req == ' ' || **req == '\t')
    {
        (*req)++;
        n->len++;
    }
    return 1;
}

// 1*tchar
int validateToken(char **req, Node *n)
{
    n->start = *req;
    if (!isTchar(**req))
        return 0;

    (*req)++;
    n->len++;
    while(isTchar(**req))
    {
        (*req)++;
        n->len++;
    }

    return 1;
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

int isUnreserved(char c)
{
    if (isalpha(c) || isdigit(c) || c == '-' || c == '.' || c == '_' || c == '~')
        return 1;
    return 0;
}

int isSubDelims(char c)
{
    int i;
    for (i = 0; i < nSubDelims; i++)
    {
        if (c == subDelims[i])
            return 1;
    }
    return 0;
}

int readString(char **req, char *s)
{
    int i;
    for (i = 0; i < strlen(s); i++)
    {
        if (**req != s[i])
        {
            return 0;
        }
        (*req)++;
    }
    return 1;
}

/* pchar = unreserved / pct-encoded / sub-delims / ":" / "@" */
int readPchar(char **req, int *len)
{
    *len = 0;
    /* unreserved */
    if (isalpha(**req) || isdigit(**req) || **req == '-' || **req == '.' || **req == '_' || **req == '~')
    {
        (*req)++;
        (*len)++;
        return 1;
    }
    /* pct-encoded */
    if (**req == '%')
    {
        (*req)++;
        (*len)++;
        if (isdigit(**req) || **req == 'A' || **req == 'B' || **req == 'C' || **req == 'D' || **req == 'E' || **req == 'F')
        {
            (*req)++;
            (*len)++;
            if (isdigit(**req) || **req == 'A' || **req == 'B' || **req == 'C' || **req == 'D' || **req == 'E' || **req == 'F')
            {
                (*req)++;
                (*len)++;
                return 1;
            }
            (*req)--;
            (*len)--;
        }
        (*req)--;
        (*len)--;
    }

    if (isSubDelims(**req))
    {
        (*req)++;
        (*len)++;
        return 1;
    }
    if (**req == ':' || **req == '@')
    {
        (*req)++;
        (*len)++;
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
int validateBrothers(char **req, Node *n, int *len)
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
            *len = 0;
            return 0;
        }
        it = it->brother;
    }
    *len = lenRead;
    return 1;
}

int validateChildren(char **req, Node *n)
{
    return validateChildrenStartingFrom(req, n, 0);
}

int validateChildrenStartingFrom(char **req, Node *n, int start)
{
    int i, l, res;
    Node *it = n->child;
    for (i = 0; i < start; i++)
    {
        if (it->brother == NULL)
        {
            printf("Mauvais index de fils %d\n", start);
            exit(0);
        }
        it = it->brother;
    }

    res = validateBrothers(req, it, &l);
    n->len += l;
    if (res == 0) /* si la validation échoue */
    {
        return 0;
    }    
    return 1;
}



int(*getValidationFunction(Node *n))(char **req, Node *n)
{
    char *name = n->ruleName;
    //printf("validating: %s\n", n->ruleName);
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
    else if (strcmp(name, "OWS") == 0)
    {
        return validateOws;
    }
    else if (strcmp(name, "header-field") == 0)
    {
        return validateHeaderField;
    }
    else if (strcmp(name, "Cookie-header") == 0)
    {
        return validateCookieHeader;
    }
    else if (strcmp(name, "cookie-string") == 0)
    {
        return validateCookieString;
    }
    else if (strcmp(name, "cookie-str") == 0)
    {
        return validateCookieStr;
    }
    else if (strcmp(name, "cookie-pair") == 0)
    {
        return validateCookiePair;
    }
    else if (strcmp(name, "cookie-name") == 0)
    {
        return validateCookieName;
    }
    else if (strcmp(name, "cookie-value") == 0)
    {
        return validateCookieValue;
    }
    else if (strcmp(name, "cookie-octet") == 0)
    {
        return validateCookieOctet;
    }
    else if (strcmp(name, "Referer-header") == 0)
    {
        return validateRefererHeader;
    }
    else if (strcmp(name, "referer-str") == 0)
    {
        return validateRefererStr;
    }
    else if (strcmp(name, "Referer") == 0)
    {
        return validateReferer;
    }
    else if (strcmp(name, "absolute-URI") == 0)
    {
        return validateAbsoluteUri;
    }
    else if (strcmp(name, "authority") == 0)
    {
        return validateAuthority;
    }
    else if (strcmp(name, "userinfo") == 0)
    {
        return validateUserInfo;
    }
    else if (strcmp(name, "host") == 0)
    {
        return validateHost;
    }
    else if (strcmp(name, "IP-literal") == 0)
    {
        return validateIPliteral;
    }
    else if (strcmp(name, "IPv6address") == 0)
    {
        return validateIPv6address;
    }
    else if (strcmp(name, "h16") == 0)
    {
        return validateH16;
    }
    else if (strcmp(name, "ls32") == 0)
    {
        return validateLS32;
    }
    else if (strcmp(name, "IPvFuture") == 0)
    {
        return validateIPvFuture;
    }
    else if (strcmp(name, "IPv4address") == 0)
    {
        return validateIPv4address;
    }
    else if (strcmp(name, "dec-octet") == 0)
    {
        return validateDecOctet;
    }
    else if (strcmp(name, "reg-name") == 0)
    {
        return validateRegName;
    }
    else if (strcmp(name, "port") == 0)
    {
        return validatePort;
    }
    else if (strcmp(name, "path-abempty") == 0)
    {
        return validatePathAbempty;
    }
    else if (strcmp(name, "path-absolute") == 0)
    {
        return validatePathAbsolute;
    }
    else if (strcmp(name, "path-rootless") == 0)
    {
        return validatePathRootless;
    }
    else if (strcmp(name, "segment-nz") == 0)
    {
        return validateSegmentNz;
    }
    else if (strcmp(name, "segment-nz-nc") == 0)
    {
        return validateSegmentNzNc;
    }
    else if (strcmp(name, "path-empty") == 0)
    {
        return validatePathEmpty;
    }
    else if (strcmp(name, "partial-URI") == 0)
    {
        return validatePartialUri;
    }
    else if (strcmp(name, "path-noscheme") == 0)
    {
        return validatePathNoscheme;
    }
    else if (strcmp(name, "scheme") == 0)
    {
        return validateScheme;
    }
    else if (strcmp(name, "hier-part") == 0)
    {
        return validateHierPart;
    }
    else if (strcmp(name, "relative-part") == 0)
    {
        return validateRelativePart;
    }
    else if (strcmp(name, "Accept-header") == 0)
    {
        return validateAcceptHeader;
    }
    else if (strcmp(name, "accept-str") == 0)
    {
        return validateAcceptStr;
    }
    else if (strcmp(name, "Accept") == 0)
    {
        return validateAccept;
    }
    else if (strcmp(name, "media-range") == 0)
    {
        return validateMediaRange;
    }
    else if (strcmp(name, "type") == 0)
    {
        return validateType;
    }
    else if (strcmp(name, "subtype") == 0)
    {
        return validateSubtype;
    }
    else if (strcmp(name, "parameter") == 0)
    {
        return validateParameter;
    }
    else if (strcmp(name, "quoted-string") == 0)
    {
        return validateQuotedString;
    }
    else if (strcmp(name, "qdtext") == 0)
    {
        return validateQdText;
    }
    else if (strcmp(name, "quoted-pair") == 0)
    {
        return validateQuotedPair;
    }
    else if (strcmp(name, "accept-params") == 0)
    {
        return validateAcceptParams;
    }
    else if (strcmp(name, "weight") == 0)
    {
        return validateWeight;
    }
    else if (strcmp(name, "qvalue") == 0)
    {
        return validateQvalue;
    }
    else if (strcmp(name, "accept-ext") == 0)
    {
        return validateAcceptExt;
    }
    else if (strcmp(name, "token") == 0)
    {
        return validateToken;
    }
    else if (strcmp(name, "Expect-header") == 0)
    {
        return validateExpectHeader;
    }
    else if (strcmp(name, "expect-str") == 0)
    {
        return validateExpectStr;
    }
    else if (strcmp(name, "Expect") == 0)
    {
        return validateExpect;
    }
    else if (strcmp(name, "Connection-header") == 0)
    {
        return validateConnectionHeader;
    }
    else if (strcmp(name, "connection-str") == 0)
    {
        return validateConnectionStr;
    }
    else if (strcmp(name, "Connection") == 0)
    {
        return validateConnection;
    }
    else if (strcmp(name, "connection-option") == 0)
    {
        return validateConnectionOption;
    }
    else if (strcmp(name, "Content-Length-header") == 0)
    {
        return validateContentLengthHeader;
    }
    else if (strcmp(name, "content-length-str") == 0)
    {
        return validateContentLengthStr;
    }
    else if (strcmp(name, "Content-Length") == 0)
    {
        return validateContentLength;
    }
    else if (strcmp(name, "Content-Type-header") == 0)
    {
        return validateContentTypeHeader;
    }
    else if (strcmp(name, "content-type-str") == 0)
    {
        return validateContentTypeStr;
    }
    else if (strcmp(name, "Content-Type") == 0)
    {
        return validateContentType;
    }
    else if (strcmp(name, "media-type") == 0)
    {
        return validateMediaType;
    }
    else if (strcmp(name, "Transfer-Encoding-header") == 0)
    {
        return validateTransferEncodingHeader;
    }
    else if (strcmp(name, "transfer-encoding-str") == 0)
    {
        return validateTransferEncodingStr;
    }
    else if (strcmp(name, "Transfer-Encoding") == 0)
    {
        return validateTransferEncoding;
    }
    else if (strcmp(name, "transfer-coding") == 0)
    {
        return validateTransferCoding;
    }
    else if (strcmp(name, "transfer-extension") == 0)
    {
        return validateTransferExtension;
    }
    else if (strcmp(name, "transfer-parameter") == 0)
    {
        return validateTransferParameter;
    }

    printf("validation function for %s not found.\n", n->ruleName);
    return NULL;
}