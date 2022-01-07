/* $Id: jstring_folio.c,v 1.4 1994/11/04 18:24:35 vertex Exp $ */

#include "types.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "debug.h"
#include "folio.h"
#include "jstring.h"


/*****************************************************************************/


/* User functions for this folio */
static void *(*FolioUserFuncs[])() =
{
    /* add new functions at the top */
    (void *(*)())ConvertHalfKana2Romaji,     /* -16 */
    (void *(*)())ConvertHalfKana2FullKana,   /* -15 */
    (void *(*)())ConvertHalfKana2Hiragana,   /* -14 */
    (void *(*)())ConvertFullKana2Romaji,     /* -13 */
    (void *(*)())ConvertFullKana2HalfKana,   /* -12 */
    (void *(*)())ConvertFullKana2Hiragana,   /* -11 */
    (void *(*)())ConvertHiragana2HalfKana,   /* -10 */
    (void *(*)())ConvertHiragana2FullKana,   /* -9 */
    (void *(*)())ConvertHiragana2Romaji,     /* -8 */
    (void *(*)())ConvertRomaji2HalfKana,     /* -7 */
    (void *(*)())ConvertRomaji2FullKana,     /* -6 */
    (void *(*)())ConvertRomaji2Hiragana,     /* -5 */
    (void *(*)())ConvertASCII2ShiftJIS,      /* -4 */
    (void *(*)())ConvertShiftJIS2ASCII,      /* -3 */
    (void *(*)())ConvertUniCode2ShiftJIS,    /* -2 */
    (void *(*)())ConvertShiftJIS2UniCode     /* -1 */
};
#define NUM_USERFUNCS (sizeof(FolioUserFuncs)/sizeof(void *))


/*****************************************************************************/


/* Tags used when creating the Folio */
static TagArg FolioTags[] =
{
    { TAG_ITEM_NAME,                (void *) JSTR_FOLIONAME },   /* name of folio              */
    { CREATEFOLIO_TAG_NUSERVECS,    (void *) NUM_USERFUNCS },    /* number of user functions   */
    { CREATEFOLIO_TAG_USERFUNCS,    (void *) FolioUserFuncs },   /* list of user functions     */
    { TAG_END,                      (void *) 0 },                /* end of tag list */
};


/****************************************************************************/


#ifdef DEVELOPMENT

static char *FolioErrors[] =
{
    "no error",

    /* JSTR_ERR_BUFFERTOOSMALL */
    "The result buffer was not large enough to hold the output"
};
#define MAX_FOLIO_ERR_SIZE 57  /* length of longest string... */

static TagArg FolioErrorTags[] =
{
    TAG_ITEM_NAME,	(void *)"JString Folio",
    ERRTEXT_TAG_OBJID,	(void *)((ER_FOLI<<ERR_IDSIZE)|(ER_JSTR)),
    ERRTEXT_TAG_MAXERR,	(void *)(sizeof(FolioErrors)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,	(void *)FolioErrors,
    ERRTEXT_TAG_MAXSTR,	(void *)MAX_FOLIO_ERR_SIZE,
    TAG_END,		0
};

#endif /* DEVELOPMENT */


/*****************************************************************************/


int main(int32 argc, char **argv)
{
Item result;

    if (argc != DEMANDLOAD_MAIN_CREATE)
        return 0;

#ifdef DEVELOPMENT
    print_vinfo();
#endif

    result = CreateItem(MKNODEID(KERNELNODE,FOLIONODE), FolioTags);

#ifdef DEVELOPMENT
    if (result >= 0)
    {
        /* ignore failures */
        CreateItem(MKNODEID(KERNELNODE,ERRORTEXTNODE),FolioErrorTags);
    }
#endif

    return (int)result;
}
