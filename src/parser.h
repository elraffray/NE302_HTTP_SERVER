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

extern char headers[][32];
extern int nbHeaders;



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
int validateBrothers(char **req, Node *n, int *len);
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
int validateOws(char **req, Node *n);
int validateToken(char **req, Node *n);

/* vérifie si le char en argument est un tchar */
int isTchar(char c);
int isUnreserved(char c);
int isSubDelims(char c);
/* tente de lire un pchar dans la requete */
int readPchar(char **req, int *len);

int readString(char **req, char *s);


// CHARLY //////////////////////////////////////////::
int validateUri(char **req, Node *n);  ////
int validateUriReference(char **req, Node *n);
int validateRelativeRef(char **req, Node *n); ////
int validatePath(char **req, Node *n);
int validateFragment(char **req, Node *n);
int validateHierPart(char **req, Node *n);


//tous les header
int validateHeaderField(char **req, Node *n);
int validateContentTypeHeader(char **req, Node * n);
int validateContentTypeHeader(char **req, Node * n);
int validateTrailerHeader(char **req, Node * n);
int validateTransferEncodingHeader(char **req, Node *n );
int validateUpgradeHeader(char **req, Node * n);
int validateViaHeader(char **req, Node * n);
int validateAgeHeader(char **req, Node * n);
int validateExpiresHeader(char **req, Node *n );
int validateDateHeader(char **req, Node * n);
int validateLocationHeader(char **req, Node *n );
int validateRetryAfterHeader(char **req, Node *n );
int validateVaryHeader(char **req, Node * n);
int validateWarningHeader(char **req, Node *n );
int validateCacheControlHeader(char **req, Node *n );
int validateMaxForwardsHeader(char **req, Node *n );
int validatePragmaHeader(char **req, Node * n);
int validateRangeHeader(char **req, Node * n);
int validateTEheader(char **req, Node * n);
int validateIfMatchHeader(char **req, Node *n );
int validateIfNoneMatchHeader(char **req, Node *n );
int validateIfModifiedSinceHeader(char **req, Node *n );
int validateIfUnmodifiedSinceHeader(char **req, Node *n );
int validateIfRangeHeader(char **req, Node *n );
int validateAcceptCharsetHeader(char **req, Node *n );
int validateAcceptEncodingHeader(char **req, Node *n );
int validateAcceptLanguageHeader(char **req, Node *n );
int validateAuthorizationHeader(char **req, Node *n );
int validateProxyAuthorizationHeader(char **req, Node *n);
int validateUserAgentHeader(char **req, Node *n );
int validateCookieHeader(char **req, Node *n );

//les fonctions qui vérifient le nom des headers
int validateContentTypeStr(char **req, Node * n);
int validateContentTypeStr(char **req, Node * n);
int validateTrailerStr(char **req, Node * n);
int validateTransferEncodingStr(char **req, Node *n );
int validateUpgradeStr(char **req, Node * n);
int validateViaStr(char **req, Node * n);
int validateAgeStr(char **req, Node * n);
int validateExpiresStr(char **req, Node *n );
int validateDateStr(char **req, Node * n);
int validateLocationStr(char **req, Node *n );
int validateRetryAfterStr(char **req, Node *n );
int validateVaryStr(char **req, Node * n);
int validateWarningStr(char **req, Node *n );
int validateCacheControlStr(char **req, Node *n );
int validateMaxForwardsStr(char **req, Node *n );
int validatePragmaStr(char **req, Node * n);
int validateRangeStr(char **req, Node * n);
int validateTEStr(char **req, Node * n);
int validateIfMatchStr(char **req, Node *n );
int validateIfNoneMatchStr(char **req, Node *n );
int validateIfModifiedSinceStr(char **req, Node *n );
int validateIfUnmodifiedSinceStr(char **req, Node *n );
int validateIfRangeStr(char **req, Node *n );
int validateAcceptCharsetStr(char **req, Node *n );
int validateAcceptEncodingStr(char **req, Node *n );
int validateAcceptLanguageStr(char **req, Node *n );
int validateAuthorizationStr(char **req, Node *n );
int validateProxyAuthorizationStr(char **req, Node *n);
int validateUserAgentStr(char **req, Node *n );

int validateCookieStr(char **req, Node *n ); // "Cookie"
int validateCookieString(char **req, Node *n ); // cookie-string
int validateCookiePair(char **req, Node *n);
int validateCookieName(char **req, Node *n);
int validateCookieOctet(char **req, Node *n);

int validateRefererHeader(char **req, Node *n );
int validateRefererStr(char **req, Node *n );
int validateReferer(char **req, Node *n);
int validateAbsoluteUri(char **req, Node *n);
int validateAuthority(char **req, Node *n);
int validateUserInfo(char **req, Node *n);
int validateHost(char **req, Node *n);
int validateIPliteral(char **req, Node *n);
int validateIPv6address(char **req, Node *n);
int validateH16(char **req, Node *n);
int validateLS32(char **req, Node *n);
int validateIPvFuture(char **req, Node *n);
int validateIPv4address(char **req, Node *n);
int validateDecOctet(char **req, Node *n);
int validateRegName(char **req, Node *n);
int validatePort(char **req, Node *n);
int validatePathAbempty(char **req, Node *n);
int validatePathAbsolute(char **req, Node *n);
int validatePathRootless(char **req, Node *n);
int validateSegmentNz(char **req, Node *n);
int validateSegmentNzNc(char **req, Node *n);
int validatePathEmpty(char **req, Node *n);
int validatePartialUri(char **req, Node *n);
int validateRelativePart(char **req, Node *n);
int validatePathNoscheme(char **req, Node *n);

int validateConnectionHeader(char **req, Node * n);
int validateConnectionStr(char **req, Node * n);
int validateConnection(char **req, Node * n);
int validateConnectionOption(char **req, Node * n);

int validateAcceptHeader(char **req, Node *n );
int validateAcceptStr(char **req, Node *n );
int validateAccept(char **req, Node *n );
int validateMediaRange(char **req, Node *n);
int validateType(char **req, Node *n);
int validateSubtype(char **req, Node *n);
int validateParameter(char **req, Node *n);
int validateQuotedString(char **req, Node *n);
int validateQdText(char **req, Node *n);
int validateQuotedPair(char **req, Node *n);
int validateAcceptParams(char **req, Node *n);
int validateWeight(char **req, Node *n);
int validateQvalue(char **req, Node *n);
int validateAcceptExt(char **req, Node *n);

int validateExpectHeader(char **req, Node * n);
int validateExpectStr(char **req, Node * n);
int validateExpect(char **req, Node * n);

int validateContentLengthHeader(char **req, Node * n);
int validateContentLengthStr(char **req, Node * n);
int validateContentLength(char **req, Node * n);

int validateContentTypeHeader(char **req, Node * n);
int validateContentTypeStr(char **req, Node * n);
int validateContentType(char **req, Node * n);
int validateMediaType(char **req, Node * n);

int validateTransferEncodingHeader(char **req, Node * n);
int validateTransferEncodingStr(char **req, Node * n);
int validateTransferEncoding(char **req, Node * n);
int validateTransferCoding(char **req, Node * n);
int validateTransferExtension(char **req, Node * n);
int validateTransferParameter(char **req, Node * n);

int validateHostHeader(char **req, Node * n);
int validateHostStr(char **req, Node * n);
int validateHost2(char **req, Node * n);
int validateUriHost(char **req, Node * n);

int validateAcceptCharsetHeader(char **req, Node * n);
int validateAcceptCharsetStr(char **req, Node * n);
int validateAcceptCharset(char **req, Node * n);
int validateCharset(char **req, Node * n);

int validateAcceptLanguageHeader(char **req, Node * n);
int validateAcceptLanguageStr(char **req, Node * n);
int validateAcceptLanguage(char **req, Node * n);
int validateAcceptLanguageRange(char **req, Node * n);

int validateAcceptEncodingHeader(char **req, Node * n);
int validateAcceptEncodingStr(char **req, Node * n);
int validateAcceptEncoding(char **req, Node * n);
int validateCodings(char **req, Node * n);
int validateContentCoding(char **req, Node * n);
int validateIdentityStr(char **req, Node * n);

int validateUserAgentHeader(char **req, Node *n);
int validateUserAgent(char **req, Node *n);
int validateUserAgentStr(char **req, Node *n);
int validateComment(char **req, Node *n);
int validateCtext(char **req, Node *n);
int validateProduct(char **req, Node *n);
int validateProductVersion(char **req, Node *n);
int validateRWS(char **req, Node *n);


/*********************************************/

#endif