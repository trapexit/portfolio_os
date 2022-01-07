/* $Id: compression_folio.c,v 1.5 1994/08/25 22:06:38 vertex Exp $ */

#include "types.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "debug.h"
#include "compression.h"


/****************************************************************************/


/* User functions for this folio */
static void *(*FolioUserFuncs[])() =
{
    (void *(*)())GetDecompressorWorkBufferSize, /* -8 */
    (void *(*)())FeedDecompressor,              /* -7 */
    (void *(*)())DeleteDecompressor,            /* -6 */
    (void *(*)())CreateDecompressor,            /* -5 */
    (void *(*)())GetCompressorWorkBufferSize,   /* -4 */
    (void *(*)())FeedCompressor,                /* -3 */
    (void *(*)())DeleteCompressor,              /* -2 */
    (void *(*)())CreateCompressor               /* -1 */
};
#define NUM_USERFUNCS (sizeof(FolioUserFuncs)/sizeof(void *))


/*****************************************************************************/


/* Tags used when creating the Folio */
static TagArg FolioTags[] =
{
    { TAG_ITEM_NAME,                (void *) COMP_FOLIONAME },   /* name of folio              */
    { CREATEFOLIO_TAG_NUSERVECS,    (void *) NUM_USERFUNCS },    /* number of user functions   */
    { CREATEFOLIO_TAG_USERFUNCS,    (void *) FolioUserFuncs },   /* list of user functions     */
    { TAG_END,                      (void *) 0 },                /* end of tag list */
};


/****************************************************************************/


#ifdef DEVELOPMENT

static char *FolioErrors[] =
{
    "no error",

    /* COMP_ERR_DATAREMAINS */
    "There is more source data than the decompressor expects",

    /* COMP_ERR_DATAMISSING */
    "There is not enough data for the decompressor",

    /* COMP_ERR_OVERFLOW */
    "Result buffer is too small for output data"
};
#define MAX_FOLIO_ERR_SIZE 55  /* length of longest string... */

static TagArg FolioErrorTags[] =
{
    TAG_ITEM_NAME,	(void *)"Compression Folio",
    ERRTEXT_TAG_OBJID,	(void *)((ER_FOLI<<ERR_IDSIZE)|(ER_COMP)),
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
