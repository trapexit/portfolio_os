/* $Id: international_folio.c,v 1.15 1994/12/19 21:53:57 vertex Exp $ */

/* #define TRACING */

#include "types.h"
#include "folio.h"
#include "task.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "item.h"
#include "operror.h"
#include "super.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "locales.h"
#include "usermodeserver.h"
#include "international_folio.h"


/****************************************************************************/


#ifdef DEVELOPMENT
uint32 FolioErrorItem;
#endif


/****************************************************************************/


/* User functions for this folio */
static void *(*FolioUserFuncs[])() =
{
    (void *(*)())intlGetCharAttrs,         /* -6 */
    (void *(*)())intlConvertString,        /* -5 */
    (void *(*)())intlCompareStrings,       /* -4 */
    (void *(*)())intlTransliterateString,  /* -3 */
    (void *(*)())intlFormatDate,           /* -2 */
    (void *(*)())intlFormatNumber          /* -1 */
};
#define NUM_USERFUNCS (sizeof(FolioUserFuncs)/sizeof(void *))


/****************************************************************************/


/* Info on nodes maintained by this folio */
static NodeData FolioNodeData[] =
{
    { 0, 0 },
    { 0, NODE_ITEMVALID | NODE_SIZELOCKED }
};
#define NUM_NODEDATA (sizeof(FolioNodeData)/sizeof(NodeData))


/****************************************************************************/


static int32 InitFolio(InternationalFolio *internationalBase);
static int32 DeleteFolio(InternationalFolio *internationalBase);


/****************************************************************************/


/* Tags used when creating the Folio */
static TagArg FolioTags[] =
{
    { TAG_ITEM_NAME,                (void *) INTL_FOLIONAME},              /* name of folio              */
    { CREATEFOLIO_TAG_ITEM,         (void *) NST_INTL},
    { CREATEFOLIO_TAG_DATASIZE,     (void *) sizeof(InternationalFolio)},  /* size of folio          */
    { CREATEFOLIO_TAG_NUSERVECS,    (void *) NUM_USERFUNCS },          /* number of user functions   */
    { CREATEFOLIO_TAG_USERFUNCS,    (void *) FolioUserFuncs },         /* list of user functions     */
    { CREATEFOLIO_TAG_INIT,         (void *) ((int32)InitFolio) },   /* called when creating folio */
    { CREATEFOLIO_TAG_DELETEF,      (void *) ((int32)DeleteFolio) },   /* called when deleting folio */
    { CREATEFOLIO_TAG_NODEDATABASE, (void *) FolioNodeData },          /* node database   */
    { CREATEFOLIO_TAG_MAXNODETYPE,  (void *) NUM_NODEDATA },           /* number of nodes */
    { TAG_END,                      (void *) 0 },                      /* end of tag list */
};


/****************************************************************************/


#ifdef DEVELOPMENT

static char *FolioErrors[] =
{
    "no error",

    /* INTL_ERR_BADRESULTBUFFER */
    "The supplied result buffer pointer is invalid",

    /* INTL_ERR_BUFFERTOOSMALL */
    "The result buffer was not large enough to hold the output",

    /* INTL_ERR_BADNUMERICSPEC */
    "The supplied NumericSpec structure pointer is invalid",

    /* INTL_ERR_FRACTOOLARGE */
    "The frac parameter is greater than 999999999",

    /* INTL_ERR_IMPOSSIBLEDATE */
    "The supplied GregorianDate structure contained an impossible date",

    /* INTL_ERR_BADGREGORIANDATE */
    "The supplied GregorianDate structure pointer is invalid",

    /* INTL_ERR_BADDATESPEC */
    "The supplied DateSpec pointer is invalid",

    /* INTL_ERR_CANTFINDCOUNTRY */
    "Unable to access the country database",

    /* INTL_ERR_BADCHARACTERSET */
    "An unknown character set was specified for intlTransliterateString()",

    /* INTL_ERR_BADSOURCEBUFFER */
    "The supplied source buffer pointer is invalid",

    /* INTL_ERR_BADFLAGS */
    "The supplied flags parameters has some undefined bits that are set"
};
#define MAX_FOLIO_ERR_SIZE 70  /* length of longest string... */

TagArg FolioErrorTags[] =
{
    TAG_ITEM_NAME,	(void *)"International Folio",
    ERRTEXT_TAG_OBJID,	(void *)((ER_FOLI<<ERR_IDSIZE)|(ER_INTL)),
    ERRTEXT_TAG_MAXERR,	(void *)(sizeof(FolioErrors)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,	(void *)FolioErrors,
    ERRTEXT_TAG_MAXSTR,	(void *)MAX_FOLIO_ERR_SIZE,
    TAG_END,		0
};

#endif /* DEVELOPMENT */


/*****************************************************************************/


/* stub to keep junk being pulled in from stdlib.o */
FILE *stdout;
int32 putc(char ch, FILE *stream)
{
    kprintf("%c", ch);
    return (ch);
}


/****************************************************************************/


extern char BuildDate[];


/*****************************************************************************/


int main(int argc, char *argv[])
{
    TRACE(("MAIN: entering International folio\n"));

    if (argc != DEMANDLOAD_MAIN_CREATE)
    {
        TRACE(("MAIN: called with argc = %d\n",argc));
        return 0;
    }

#ifdef DEVELOPMENT
    print_vinfo();
#endif

    return (int)CreateItem(MKNODEID(KERNELNODE,FOLIONODE),FolioTags);
}


/****************************************************************************/


static Item CreateFolioItem(void *node, uint8 nodeType, void *args)
{
    switch (nodeType)
    {
        case INTL_LOCALE_NODE : return (CreateLocaleItem((Locale *)node,(TagArg *)args));
        default               : return (INTL_ERR_BADSUBTYPE);
    }
}


/****************************************************************************/


static int32 DeleteFolioItem(Item it, Task *task)
{
Node *node;

    node = (Node *)LookupItem(it);

    switch (node->n_Type)
    {
        case INTL_LOCALE_NODE : return (DeleteLocaleItem((Locale *)node));
        default               : return (INTL_ERR_BADITEM);
    }
}


/******************************************************************/


static Item FindFolioItem(int32 ntype, TagArg *args)
{
    SUPERTRACE(("FINDFOLIOITEM: entering with ntype = %ld\n",ntype));

    switch (ntype)
    {
        case INTL_LOCALE_NODE : return (FindLocaleItem(args));
        default               : return (INTL_ERR_BADITEM);
    }
}


/******************************************************************/


static Item OpenFolioItem(Node *node, void *args)
{
    switch (node->n_Type)
    {
        case INTL_LOCALE_NODE : return (OpenLocaleItem((Locale *)node,(TagArg *)args));
        default               : return (INTL_ERR_BADITEM);
    }
}


/******************************************************************/


static int32 CloseFolioItem(Item it, Task *task)
{
Node *node;

    node = (Node *) LookupItem(it);

    switch (node->n_Type)
    {
        case INTL_LOCALE_NODE : return (CloseLocaleItem((Locale *)node));
        default               : return (INTL_ERR_BADITEM);
    }
}


/******************************************************************/


static Err SetOwnerFolioItem(ItemNode *n, Item newOwner, Task *task)
{
    return 0;
}


/****************************************************************************/


/* This is called by the kernel when the folio item is created */
static int32 InitFolio(InternationalFolio *internationalBase)
{
ItemRoutines *itr;
int32         result;

    SUPERTRACE(("INITFOLIO: entering\n"));

    /* Set pointers to required Folio functions. */
    itr              = internationalBase->fb_Folio.f_ItemRoutines;
    itr->ir_Delete   = DeleteFolioItem;
    itr->ir_Create   = CreateFolioItem;
    itr->ir_Find     = FindFolioItem;
    itr->ir_Open     = OpenFolioItem;
    itr->ir_Close    = CloseFolioItem;
    itr->ir_SetOwner = SetOwnerFolioItem;

    SUPERTRACE(("INITFOLIO: about to create user mode server\n"));

    result = CreateUserModeServer();

#ifdef DEVELOPMENT
    if (result >= 0)
    {
        FolioErrorItem = SuperCreateItem(MKNODEID(KERNELNODE,ERRORTEXTNODE),FolioErrorTags);
    }
#endif

    SUPERTRACE(("INITFOLIO: exiting with %d\n",result));

    return (result);
}


/****************************************************************************/


/* This is called by the kernel when the folio item is deleted */
static int32 DeleteFolio(InternationalFolio *internationalBase)
{
    SUPERTRACE(("DELETEFOLIO: entering\n"));

#ifdef DEVELOPMENT
    SuperDeleteItem(FolioErrorItem);
#endif

    DeleteUserModeServer();

    SUPERTRACE(("DELETEFOLIO: exiting with 0\n"));

    return (0);
}
