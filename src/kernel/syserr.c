/* $Id: syserr.c,v 1.49 1995/01/24 21:27:58 vertex Exp $ */

/* Error text management */

/* table defining ascii to 6 bit encoding and back */

static char _SixToAscii[64] =
{
	' ',
	'0','1','2','3','4','5','6','7','8','9',
	'a','b','c','d','e','f','g','h','i','j','k','l','m','n',
	'o','p','q','r','s','t','u','v','w','x','y','z',
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N',
	'O','P','Q','R','S','T','U','V','W','X','Y','Z',
	'.'
};

#undef KERNEL
#define KERNEL

#include "types.h"
#include "nodes.h"
#include "list.h"
#include "listmacros.h"
#include "kernelnodes.h"
#include "strings.h"
#include "item.h"
#include "folio.h"
#include "kernel.h"
#include "semaphore.h"
#include "operror.h"
#include "stdio.h"
#include "internalf.h"

#define	MakeASCII(n) (_SixToAscii[(n)])
#define	MAKEASCII(n) MakeASCII(n)

extern void Panic(int halt,char *);

static char *err_severity[] =
{
	"Info","Warning","Severe","Fatal"
};
#define MAX_SEVERITY	9	/* number of chars in longest string */

static char *err_env[] =
{
	"System","Application","User","Reserved"
};
#define MAX_ENV		12	/* number of chars in longest string */

static char *err_class[] =
{
	"Standard","Extended"
};
#define MAX_CLASS	9	/* number of chars in longest string */

static char *err_stnd[] =
{
	"no error",
	"Bad Item",
	"undefined tag",
	"bad value in tagarg list",
	"NotPrivileged",
	"Item with that name not found",
	"insufficient memory to complete",
	"Bad SubType for this Folio/Cmd",
	"Evil system error, should not occur!",
	"ptr/range is illegal for this task",
	"operation aborted",	/* 10 */
	"bad unit in IOReq",
	"bad cmd in IOReq",
	"bad IOArgs in ioinfo",
	"bad Name for Item",
	"IO in progress for this IOReq",	/* 15 */
	"Operation not supported at this time", /* 16 */
	"IO incomplete for this IOReq",	/* 17 */
	"NotOwner",	/* 18 */
        "Device offline", /* 19 */
        "Device error", /* 20 */
        "Medium error", /* 21 */
        "End of medium", /* 22 */
        "Illegal parameter(s)", /* 23 */
        "No signals available", /* 24 */
        "Required hardware is not available", /* 25 */
};
#define MAX_STND	sizeof("Operation not supported at this time")

static char *krnl_tbl[] =
{
	"no error",
	"Bad Type Specifier",
	"Unknown SubType for this Folio",
	"Resource Table OverFlow",
	"DriverItem is not valid",
	"Item is not opened by you",
	"Item Table Full",
	"BadMemCmd",
	"Out of signals",
	"Msg already sent / Bad State",
	"NoReplyPort",
	"size less than minimum",
	"BadPriority",
	"illegal operation for thread",
	"BadQuanta",
	"BadStackSize",
	"NoTimer",
	"Can't Open",
	"RSA failed in CreateTask",
	"No devices on XBUS",
	">15 devices on XBUS",
	"Replyport required",
	"Illegal signal",
	"Bad Lock arg",
	"Bad AIF header",
	"Can't transfer item ownership",
	"Can't delete item since it is still open",
	"An item of that name already exists",
	"Bad exit status",
	"Task got killed"
};
#define MAX_KRNL	sizeof("Can't delete item since it is still open")

static List ErrorList;
static Item listSem = -1;     /* semaphore to protect above list */

TagArg ErrTA[] =
{
    TAG_ITEM_NAME,	(void *)"Kernel Errors",
    ERRTEXT_TAG_OBJID,	0,
    ERRTEXT_TAG_MAXERR,	(void *)(sizeof(krnl_tbl)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,	(void *)krnl_tbl,
    ERRTEXT_TAG_MAXSTR,	(void *)MAX_KRNL,
    TAG_END,		0
};

static TagArg semaphoreTags[2] =
{
	TAG_ITEM_NAME, (void *) "ErrorText",
	TAG_END, 0,
};

void
ErrorInit(void)
{
	ErrorText *et;
	Item eti;
	Semaphore *s;

	InitList(&ErrorList,0);

        s = (Semaphore *)AllocateNode ((Folio *) KernelBase, SEMA4NODE);
	if (s)
            listSem = internalCreateSemaphore (s, semaphoreTags);

	et = (ErrorText *)AllocateNode((Folio *)KernelBase,ERRORTEXTNODE);
	if (et)
	{
            ErrTA[1].ta_Arg = (void *)((ER_FOLI<<ERR_IDSIZE)|(ER_KRNL));
            eti = internalCreateErrorText(et,ErrTA);
        }
}

static int32
icet_c(et, p, tag, arg)
ErrorText *et;
void *p;
uint32 tag;
uint32 arg;
{
    switch (tag)
    {
	case ERRTEXT_TAG_OBJID:
			et->et_ObjID = arg;
			break;
	case ERRTEXT_TAG_MAXERR:
			et->et_MaxErr = (uint8)arg;
			break;
	case ERRTEXT_TAG_MAXSTR:
			et->et_MaxStringSize = (uint8)arg;
			break;
	case ERRTEXT_TAG_TABLE:
			et->et_ErrorTable = (char **)arg;
			break;
	default:	return BADTAG;
    }
    return 0;
}


Item
internalCreateErrorText(ErrorText *et,TagArg *a)
{
	Item ret;
	ErrorText *node;

	ret = TagProcessor(et, a, icet_c, 0);
	if (ret < 0) return ret;

        /* These values are required */
	if ((et->et_ObjID == 0)
	 || (et->et_MaxErr == 0)
	 || (et->et_ErrorTable == NULL)
	 || (et->et_MaxStringSize == 0))
	{
	    return BADTAGVAL;
	}

        /* the error table must be in RAM */
	if (!IsRamAddr(et->et_ErrorTable,et->et_MaxErr * sizeof(char *)))
	    return BADPTR;

        if (CURRENTTASK)
            externalLockSemaphore(listSem,TRUE);

        /* make sure we don't already have errors for this object id */
	SCANLIST(&ErrorList, node, ErrorText)
	{
	    if (node->et_ObjID == et->et_ObjID)
	    {
                externalUnlockSemaphore(listSem);
	        return BADTAGVAL;
	    }
	}

	ADDTAIL(&ErrorList,(Node *)et);

        if (CURRENTTASK)
            externalUnlockSemaphore(listSem);

	return et->et.n_Item;
}

int32
internalDeleteErrorText(struct ErrorText *et,struct  Task *t)
{
        externalLockSemaphore(listSem,TRUE);
	REMOVENODE((Node *)et);
        externalUnlockSemaphore(listSem);
	return 0;
}

/**
|||	AUTODOC PUBLIC spg/kernel/getsyserr
|||	GetSysErr - Get the error string for an error.
|||
|||	  Synopsis
|||
|||	    int32 GetSysErr( char *buff, int32 buffsize, Err err )
|||
|||	  Description
|||
|||	    This procedure returns a character string that describes an error
|||	    code.  The resulting string is placed in a buffer.
|||
|||	    The procedure interprets all the fields in the err argument and
|||	    returns corresponding text strings for all of them.  If an
|||	    error-text table is not found for the specific error, the routine
|||	    uses numbers for the final error code value.
|||
|||	  Arguments
|||
|||	    buff                        A pointer to a buffer to hold the
|||	                                error string.
|||
|||	    buffsize                    The size of the error-text buffer, in
|||	                                bytes.  This should be 128.
|||
|||	    err                         The error code whose error string to
|||	                                get.
|||
|||	  Return Value
|||
|||	    The procedure returns the number of bytes in the description
|||	    string, or a negative error code if a bad buffer pointer is
|||	    supplied.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    operror.h                   ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  See Also
|||
|||	    PrintfSysErr()
|||
**/

struct KernelBase *getKernelBase(void);

int32 GetSysErr(char *ubuff, int32 n, Err i)
{
uint32     obj;
uint32     id;
uint32     severity;
uint32     env;
uint32     errclass;
uint32     errnum;
uint32     maxerr;
char     **errtbl;
char       local[128];
ErrorText *et;
uint32     maxsize;
struct KernelBase *oldR9;

    if (n == 0)
    {
        /* buffer has got no room! */
        return 0;
    }

    /* When this function is called, R9 does not contain KernelBase. Since
     * we're calling other kernel functions, we have to set up R9
     * appropriately. So, we save the old R9 and load in KernelBase. Before
     * returning, we must restore the previous value of R9.
     * (BTW, "KernelBase" is an alias for R9)
     */

    oldR9 = KernelBase;
    KernelBase = getKernelBase();

    if (isUser())
    {
        if (ValidateMem(CURRENTTASK,ubuff,n) != 0)
        {
            /* trying to pass a bad buffer, are we? */
            KernelBase = oldR9;
            return BADPTR;
        }
    }

    if (i >= 0)
    {
        /* not an error code */
	sprintf(local,"<%d, not an error>",i);
    }
    else if (i == -1)
    {
        /* -1 is the catch-all error code */
	strcpy(local,"<-1, generic error>");
    }
    else
    {
        /* decompose the error code */
        obj      = ((uint32)i >> ERR_OBJSHIFT)    & ((1 << ERR_OBJSIZE) - 1);
        id       = ((uint32)i >> ERR_IDSHIFT)     & ((1 << ERR_IDSIZE) - 1);
        severity = ((uint32)i >> ERR_SEVERESHIFT) & ((1 << ERR_SEVERESIZE) - 1);
        env      = ((uint32)i >> ERR_ENVSHIFT)    & ((1 << ERR_ENVSIZE) - 1);
        errclass = ((uint32)i >> ERR_CLASHIFT)    & ((1 << ERR_CLASSIZE) - 1);
        errnum   = ((uint32)i >> ERR_ERRSHIFT)    & ((1 << ERR_ERRSIZE) - 1);

	sprintf(local,"%c%c%c-%s-%s-%s-",MAKEASCII(obj),
	                                 MAKEASCII((id >> 6) & 0x3f),
	                                 MAKEASCII(id & 0x3f),
	                                 err_severity[severity],
                                         err_env[env],
                                         err_class[errclass]);

        if (isUser())
            LockSemaphore(listSem,TRUE);
        else
            externalLockSemaphore(listSem,TRUE);

	if (errclass == ER_C_STND)
	{
	    errtbl  = err_stnd;
	    maxerr  = sizeof(err_stnd)/sizeof(char *);
	    maxsize = sizeof(local);  /* a white lie */
	}
	else
	{
	    errtbl  = NULL;
	    maxerr  = 0;
	    maxsize = 0;

            SCANLIST(&ErrorList,et,ErrorText)
	    {
		if (et->et_ObjID == ((obj << ERR_IDSIZE) | id))
		{
		    errtbl  = et->et_ErrorTable;
		    maxerr  = et->et_MaxErr;
		    maxsize = et->et_MaxStringSize;
		    break;
		}
	    }
	}

	if ((errnum < maxerr) && IsRamAddr(errtbl[errnum],maxsize))
        {
            if (maxsize > sizeof(local))
                maxsize = sizeof(local);

            strncat(local,errtbl[errnum],maxsize);
            local[sizeof(local) - 1] = 0;
        }
        else
        {
	    sprintf(&local[strlen(local)],"<%03u>",errnum);
        }

        if (isUser())
            UnlockSemaphore(listSem);
        else
            externalUnlockSemaphore(listSem);
    }

    strncpy(ubuff,local,n);
    ubuff[n - 1] = 0;

    KernelBase = oldR9;

    return strlen(ubuff);
}
