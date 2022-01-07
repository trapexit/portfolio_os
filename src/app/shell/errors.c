
#include "types.h"
#include "music.h"
#include "item.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "mem.h"
#include "operror.h"


/*****************************************************************************/


#ifdef DEVELOPMENT
static char *Errors[] =
{
    "no error",

    /* ACCERR_BAD_REQUEST */
    "Unknown request argument",

    /* ACCERR_NO_SCREEN */
    "No screen specified",

    /* ACCERR_BAD_ARGS */
    "Something about the args was bad",
};
#define MAX_ERROR_LEN 35

static TagArg ErrorTags[] =
{
    TAG_ITEM_NAME,	(void *)"Access",
    ERRTEXT_TAG_OBJID,	(void *)((ER_TASK<<ERR_IDSIZE)|(ER_ACCESS)),
    ERRTEXT_TAG_MAXERR,	(void *)(sizeof(Errors)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,	(void *)Errors,
    ERRTEXT_TAG_MAXSTR,	(void *)MAX_ERROR_LEN,
    TAG_END,		0
};

static Err InstallAccessErrors(void)
{
    return CreateItem(MKNODEID(KERNELNODE,ERRORTEXTNODE),ErrorTags);
}
#endif


/*****************************************************************************/


typedef Err (* AddErrorFunc)(void);


#ifdef DEVELOPMENT
static const AddErrorFunc addErrFuncs[] =
{
    InstallMusicLibErrors,
    InstallMemDebugErrors,
    InstallAccessErrors,
    NULL
};


void AttachErrors(void)
{
uint32 i;

    i = 0;
    while (addErrFuncs[i])
    {
        (* addErrFuncs[i])();
        i++;
    }
}
#endif


/*****************************************************************************/


/* This code is not being used right now. This code lets you add a new
 * command to the shell, call it adderror. You can invoke the command
 * using "adderror <error number> <error string>". The code then proceeds
 * to create an error node to hold this error and attaches it to the system.
 *
 * Using this scheme, you could have a script file invoked by the startup
 * script which would add a bunch of error strings to the system.
 *
 * This code wasn't fully tested, so beware!
 */

#if 0
#include "types.h"
#include "list.h"
#include "operror.h"
#include "nodes.h"
#include "string.h"
#include "mem.h"
#include "errors.h"


/****************************************************************************/


typedef struct Error
{
    MinNode err_Link;
    uint32  err_Number;
    char    err_Text[1];
} Error;

typedef struct ErrorObj
{
    MinNode  eo_Link;
    uint32   eo_ErrorObjCode;
    List     eo_Errors;
    char   **eo_ErrorTable;
    uint32   eo_ErrorTableSize;
    Item     eo_ErrorItem;
} ErrorObj;


/****************************************************************************/


static List objList;
static bool firstTime  = TRUE;
static bool workNeeded = FALSE;


#define GETERROROBJ(err) ((uint32)(err>>ERR_OBJSHIFT)&0x3f)
#define GETERRORNUM(err) ((err) & 0xff)


/*****************************************************************************/


static uint32 ConvertNum(char *str, char **remainder)
{
    if (*str == '$')
    {
        str++;
        return strtoul(str,remainder,16);
    }

    return strtoul(str,remainder,0);
}


/****************************************************************************/


static void RemoveError(uint32 errorNum)
{
ErrorObj *obj;
Error    *err;

    SCANLIST(&objList,obj,ErrorObj)
    {
        if (obj->eo_ErrorObjCode == GETERROROBJ(errorNum))
        {
            SCANLIST(&obj->eo_Errors,err,Error)
            {
                if (err->err_Number == GETERRORNUM(errorNum))
                {
                    RemNode((Node *)err);
                    FreeMem(err,sizeof(Error) + strlen(err->err_Text));
                    break;
                }
            }
            return;
        }
    }
}


/****************************************************************************/


void AddError(char *args)
{
Error    *err;
ErrorObj *obj;
uint32    errorNum;
char     *errorText;

    errorNum = ConvertNum(args,&errorText);
    while (*errorText == ' ')
        errorText++;

    printf("error num $%x, text '%s'\n",errorNum,errorText);

    if (firstTime)
    {
        InitList(&objList,NULL);
        firstTime = FALSE;
    }

    RemoveError(errorNum);

    err = (Error *)AllocMem(sizeof(Error) + strlen(errorText),MEMTYPE_ANY);
    if (err)
    {
printf("allocated error node\n");
        err->err_Number = GETERRORNUM(errorNum);
        strcpy(err->err_Text,errorText);

        SCANLIST(&objList,obj,ErrorObj)
        {
            if (obj->eo_ErrorObjCode == GETERROROBJ(errorNum))
            {
printf("found a matching node id, adding error node to the list\n");
                AddTail(&obj->eo_Errors,(Node *)err);
                workNeeded = TRUE;
                return;
            }
        }

printf("no matching id found, creating a new one\n");
        obj = (ErrorObj *)AllocMem(sizeof(ErrorObj),MEMTYPE_ANY|MEMTYPE_FILL);
        if (obj)
        {
            InitList(&obj->eo_Errors,NULL);
            AddTail(&obj->eo_Errors,(Node *)err);
            obj->eo_ErrorObjCode = GETERROROBJ(errorNum);
            obj->eo_ErrorItem    = -1;
            AddTail(&objList,(Node *)obj);
printf("added new obj id node\n");

            workNeeded = TRUE;
        }
    }
}


/****************************************************************************/


void AttachErrors(void)
{
uint32    maxNum;
uint32    maxLen;
uint32    len;
TagArg    tags[6];
ErrorObj *obj;
Error    *err;

    if (workNeeded)
    {
printf("doing work\n");
        SCANLIST(&objList,obj,ErrorObj)
        {
            DeleteItem(obj->eo_ErrorItem);
            if (obj->eo_ErrorTable)
            {
                FreeMem(obj->eo_ErrorTable,obj->eo_ErrorTableSize);
                obj->eo_ErrorTable = NULL;
            }

            maxNum = 0;
            maxLen = 0;
            SCANLIST(&obj->eo_Errors,err,Error)
            {
                len = strlen(err->err_Text);
                if (len > maxLen)
                    maxLen = len;

                if (err->err_Number > maxNum)
                    maxNum = err->err_Number;
            }
printf("maxNum = %ld, maxLen %d\n",maxNum,maxLen);
            if (maxNum)
            {
                obj->eo_ErrorTable = (char **)AllocMem((maxNum + 1) * sizeof(char *),MEMTYPE_ANY|MEMTYPE_FILL);
                if (obj->eo_ErrorTable)
                {
                    obj->eo_ErrorTableSize = (maxNum + 1) * sizeof(char *);

                    SCANLIST(&obj->eo_Errors,err,Error)
                    {
                        obj->eo_ErrorTable[err->err_Number] = err->err_Text;
                    }

                    tags[0].ta_Tag = TAG_ITEM_NAME;
                    tags[0].ta_Arg = (void *)"Shell Sponsored Error Codes";
                    tags[1].ta_Tag = ERRTEXT_TAG_OBJID;
                    tags[1].ta_Arg = (void *)obj->eo_ErrorObjCode;
                    tags[2].ta_Tag = ERRTEXT_TAG_MAXERR;
                    tags[2].ta_Arg = (void *)maxNum;
                    tags[3].ta_Tag = ERRTEXT_TAG_TABLE;
                    tags[3].ta_Arg = (void *)obj->eo_ErrorTable;
                    tags[4].ta_Tag = ERRTEXT_TAG_MAXSTR;
                    tags[4].ta_Arg = (void *)maxLen;
                    tags[5].ta_Tag = TAG_END;

                    obj->eo_ErrorItem = CreateItem(MKNODEID(KERNELNODE,ERRORTEXTNODE),tags);
printf("new item $%x\n",obj->eo_ErrorItem);
                }
            }
        }

        workNeeded = FALSE;
    }
}
#endif
