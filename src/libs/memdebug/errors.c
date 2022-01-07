
#include "types.h"
#include "item.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "operror.h"


/*****************************************************************************/


static char *Errors[] =
{
    "no error",

    /* MEMDEBUG_ERR_BADFLAGS */
    "A bad flag was set in the controlFlag parameter of CreateMemDebug()"
};
#define MAX_ERROR_LEN 68

static TagArg ErrorTags[] =
{
    TAG_ITEM_NAME,	(void *)"MemDebug",
    ERRTEXT_TAG_OBJID,	(void *)((ER_LINKLIB<<ERR_IDSIZE)|(ER_MEMDEBUG)),
    ERRTEXT_TAG_MAXERR,	(void *)(sizeof(Errors)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,	(void *)Errors,
    ERRTEXT_TAG_MAXSTR,	(void *)MAX_ERROR_LEN,
    TAG_END,		0
};

Err InstallMemDebugErrors(void)
{
    return CreateItem(MKNODEID(KERNELNODE,ERRORTEXTNODE),ErrorTags);
}
