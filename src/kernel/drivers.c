/* $Id: drivers.c,v 1.46 1994/12/13 00:21:50 limes Exp $ */
/* file: drivers.c */

#include "types.h"
#include "list.h"
#include "listmacros.h"
#include "item.h"
#include "driver.h"
#include "device.h"
#include "internalf.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "io.h"
#include "setjmp.h"
#include "operror.h"
#include "sherryvers.h"
#include "string.h"
#include "usermodeservices.h"
#include "stdio.h"
#include "aif.h"

#define DBUG(x)	/*printf x*/

int32
defaultDispatch(IOReq *ior)
{
uint8   cmd;
Driver *drv;

    cmd = ior->io_Info.ioi_Command;
    drv = ior->io_Dev->dev_Driver;

    if (cmd >= drv->drv_MaxCommands)
    {
	ior->io_Error = BADCOMMAND;
    }
    else
    {
	if ((drv->drv_CmdTable[cmd])(ior) == 0)
	    return 0;
    }

    if ( (ior->io_Flags & IO_INTERNAL) == 0)
        internalCompleteIO(ior);

    return 1;
}

static int32
icd_c(drv, p, tag, arg)
Driver *drv;
void *p;
uint32 tag;
uint32 arg;
{
    DBUG(("icd_c drv=%lx tag=%lx arg=%lx\n",drv,tag,arg));
    switch( tag )
    {
	case CREATEDRIVER_TAG_ABORTIO: drv->drv_AbortIO = Make_Func(void,arg);
					break;
	case CREATEDRIVER_TAG_MAXCMDS: drv->drv_MaxCommands = (uint8)arg;
					break;
	case CREATEDRIVER_TAG_CMDTABLE: drv->drv_CmdTable = (int32 (**)())arg;
					break;
	case CREATEDRIVER_TAG_MSGPORT: drv->drv_MsgPort = (MsgPort *)arg;
					break;
	case CREATEDRIVER_TAG_INIT: drv->drv_Init = Make_Func(int32,arg);
					break;
	case CREATEDRIVER_TAG_DISPATCH: drv->drv_DispatchIO = Make_Func(int32,arg);
					break;
	default:
	    return BADTAG;
    }
    return 0;
}

Item
internalCreateDriver(dev,a)
Driver *dev;
TagArg *a;
{
	Item ret;

	DBUG(("internalCreateDriver(%lx,%lx)\n",dev,a));

        if ((CURRENTTASK->t.n_Flags & TASK_SUPER) == 0) return BADPRIV;

	ret = TagProcessor(dev, a, icd_c, 0);
	if (ret < 0) goto now_ret;

	/* must have a name */
	DBUG(("validate driver tag args\n"));
	if ( (dev->drv.n_Name == 0) ||
	   (dev->drv_MaxCommands == 0) ||
	   (dev->drv_AbortIO == 0) )
	{
	   ret = BADTAGVAL;
	   goto now_ret;
	}

	if (dev->drv_DispatchIO == 0)
	{
		/* install default dispatch routine */
		dev->drv_DispatchIO = defaultDispatch;
	}

	DBUG(("drv_Init=%lx\n",(long)dev->drv_Init));
	if (dev->drv_Init) ret = (*dev->drv_Init)(dev);
	else ret = dev->drv.n_Item;

now_ret:
	DBUG(("now return ret=%lx\n",ret));
	if (ret >= 0)	TailInsertNode(KernelBase->kb_Drivers,(Node *)dev);
	return ret;
}

int32
internalDeleteDriver(drvr, t)
Driver *drvr;
struct Task *t;
{
	List *l = KernelBase->kb_Devices;
	Device *d;
	/* abort all pending ioreqs for this device */
	/* Delete all devices using this driver */
again:
	for (d = (Device *)FIRSTNODE(l) ; ISNODE(l,d);
		d = (Device *)NEXTNODE(d) )
	{
	    if (d->dev_Driver == drvr)
	    {
		Task *t = (Task *)LookupItem(d->dev.n_Owner);
		internalDeleteItem(d->dev.n_Item,t);
		goto again;
	    }
	}
	REMOVENODE((Node *)drvr);
	return 0;
}


/*****************************************************************************/


/* Load a driver from external storage */
Item internalLoadDriver(char *name)
{
void   *context;
Item    result;
Driver *drv;

    result = LoadModule("driver",name,MKNODEID(KERNELNODE,DRIVERNODE),&context);

    drv = (Driver *)LookupItem(result);
    if (drv)
        drv->drv_DemandLoad = context;

    return result;
}


/*****************************************************************************/


/* Unload a driver from memory */
Err internalUnloadDriver(Driver *d)
{
    return UnloadModule((ItemNode *)d,d->drv_DemandLoad);
}


/*****************************************************************************/


Item
OpenDriver(dev,a)
Driver *dev;
void *a;
{
	DBUG(("OpenDriver() dev=%lx\n",(uint32)dev));
	dev->drv_OpenCnt++;
	return dev->drv.n_Item;
}

int32
CloseDriver(drv,t)
Driver *drv;
struct Task *t;
{
	drv->drv_OpenCnt--;

	if (!drv->drv_OpenCnt)
	    return internalUnloadDriver(drv);

	return 0;
}

void *
DipirSlot(uint32 n)
{
	uint32 tablesize;
	uint32 *vectors = (uint32 *)0x200;

	/*
	 * Figure out how big the dipir vector table is.
	 * Newer dipirs have the table size in slot 6.
	 * Older ones just have a 0 after the last entry.
	 */
	if (vectors[3] == 0)
		tablesize = 3;
	else if (vectors[4] == 0)
		tablesize = 4;
	else if (vectors[5] == 0)
		tablesize = 5;
	else if (vectors[6] == 0)
		tablesize = 6;
	else
		tablesize = vectors[6];

	if (n >= tablesize) {
		/*
		 * This dipir doesn't have the requested entry point.
		 */
		return 0;
	}
	return (void*)(&vectors[n]);
}

int32
SectorECC(uint8 *buffer)
{
	func_t func;

	func = (func_t)DipirSlot(5);
	if (func == 0)
		return 0;
	return (*func)(buffer);
}

uint32	cachedDiscOsVersion = 0;

uint32
internalDiscOsVersion(uint32 set)
{
#ifndef	DEVELOPMENT
    func_t func;
#endif

    if (set == DiscOsReset)
	set = cachedDiscOsVersion = 0;

    if (set == 0)
    {
	set = cachedDiscOsVersion;
	if (set == 0)
	{
#ifndef	DEVELOPMENT
	    func = (func_t)DipirSlot(10);
	    if (func == (func_t)0)
#endif
		set =((__my_3DOBinHeader._3DO_Item.n_Version << 8) |
		      (__my_3DOBinHeader._3DO_Item.n_Revision));
#ifndef	DEVELOPMENT
	    else
		set = (*func)();	/* do not need to be supv */
	    if (set == 0)		/* some discs have vers.rev set to 0.0 */
		set = DiscOs_1_0;	/* ... they were cut with Release 1.0 */
#endif
	}
    }
    if (!isUser())
	cachedDiscOsVersion = set;
    return set;
}
