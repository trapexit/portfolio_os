/* $Id: devices.c,v 1.50 1994/12/12 21:22:37 vertex Exp $ */
/* file: devices.c */

#include "types.h"
#include "list.h"
#include "listmacros.h"
#include "item.h"
#include "driver.h"
#include "device.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "io.h"
#include "task.h"
#include "usermodeservices.h"
#include "string.h"
#include "mem.h"
#include "stdio.h"


#include "operror.h"

#define DBUG(x)	/*printf x*/

#define SEMI_DEVICE

#ifdef	SEMI_DEVICE
#include "semaphore.h"
extern Semaphore *DevSemaphore;
#endif

#include "internalf.h"

static int32
icd_c(dev, p, tag, arg)
Device *dev;
void *p;
uint32 tag;
uint32 arg;
{
    DBUG(("icd_c dev=%lx tag=%lx arg=%lx\n",dev,tag,arg));
    switch (tag)
    {
	case CREATEDEVICE_TAG_DRVR:
	{
	    Driver *drvr = (Driver *)CheckItem((Item)arg,KERNELNODE,DRIVERNODE);
	    if (!drvr)	return BADTAGVAL;
	    dev->dev_Driver = drvr;
	    break;
	}
	case CREATEDEVICE_TAG_CRIO: dev->dev_CreateIOReq = Make_Func(int32,arg);
					break;
	case CREATEDEVICE_TAG_DLIO: dev->dev_DeleteIOReq = Make_Func(int32,arg);
					break;
	case CREATEDEVICE_TAG_OPEN: dev->dev_Open = Make_Func(int32,arg);
					break;
	case CREATEDEVICE_TAG_CLOSE: dev->dev_Close = Make_Func(void,arg);
					break;
	case CREATEDEVICE_TAG_INIT: dev->dev_Init = Make_Func(int32,arg);
					break;
	case CREATEDEVICE_TAG_IOREQSZ:
	{
	    int32 size = (int32)arg;
	    if (size < sizeof(IOReq) )	return BADTAGVAL;
	    dev->dev_IOReqSize = size;
	}
	break;

	default:
		return BADTAG;
    }
    return 0;
}

/**
|||	AUTODOC PRIVATE spg/kernel/createdevice
|||	CreateDevice               Creates a device
|||
|||	  SYNOPSIS
|||
|||	     Item CreateDevice(const char *name, uint8 pri, Item driver)
|||
|||	  DESCRIPTION
|||
|||	     This procedure creates a device item.  It can only be
|||	     called by system tasks.
|||
|||	  ARGUMENTS
|||
|||	     name         Input: The name of the device (see "Notes")
|||
|||	     pri          Input: The priority of the device.  This
|||	                  determines the position of the device in
|||	                  the device list; devices with higher
|||	                  priorities are found earlier and are
|||	                  slightly more efficient as a result.
|||
|||	     driver       Input: The driver to be used for the device
|||
|||	  RETURN VALUE
|||
|||	     The procedure returns 0 if it is successful or an error
|||	     code if an error occurred.
|||
|||	  IMPLEMENTATION
|||
|||	     Convenience call implemented in clib.lib V20.
|||
|||	  ASSOCIATED FILES
|||
|||	     device.h     clib.lib
|||
|||	  NOTES
|||
|||	     When you no longer need a device, use DeleteDevice() to
|||	     delete it.
|||
|||	     You can use FindDevice() to find a device by name.  When
|||	     naming devices, you should assign unique names whenever
|||	     possible.
|||
|||	     You must open a device before using it.
|||
|||	     The Kernel may change the priority of a device to help
|||	     optimize throughput.
|||
|||	  CAVEATS
|||
|||	     You cannot delete a device once it has been created.
|||
|||	  SEE ALSO
|||
|||	     DeleteDevice(), OpenDevice(), CloseDevice()
**/

Item
internalCreateDevice(dev,a)
Device *dev;
TagArg *a;
{
    Task *ct = KernelBase->kb_CurrentTask;
    Item ret;

    DBUG(("internalCreateDevice\n"));

    if ((ct->t.n_Flags & TASK_SUPER) == 0) return BADPRIV;

    dev->dev_Extension = (DeviceExtension *)AllocMem(sizeof(DeviceExtension),MEMTYPE_FILL);
    if (!dev->dev_Extension)
        return NOMEM;

    dev->dev_Extension->de_StructSize = sizeof(DeviceExtension);

    DBUG(("process devicetag args\n"));

    ret = TagProcessor(dev,a,icd_c,0);
    DBUG(("TagProc returns:%lx\n",ret));

    if (ret >= 0)
    {
	if (dev->dev_IOReqSize == 0)	dev->dev_IOReqSize = sizeof(IOReq);

	if (dev->dev_Init)
	{
	    ret = (*dev->dev_Init)(dev);
	    if (ret < 0)	goto now_ret;
	}

	InitList(&dev->dev_IOReqs,"Device ioreqs");
#ifdef	SEMI_DEVICE
	internalLockSemaphore(DevSemaphore,1);
#endif
	TailInsertNode(KernelBase->kb_Devices,(Node *)dev);
#ifdef	SEMI_DEVICE
	internalUnlockSemaphore(DevSemaphore);
#endif
	ret = dev->dev.n_Item;
    }
now_ret:

    if (ret < 0)
    {
        FreeMem(dev->dev_Extension,sizeof(DeviceExtension));
        dev->dev_Extension = NULL;
    }

    return	ret;
}

/**
|||	AUTODOC PRIVATE spg/kernel/deletedevice
|||	DeleteDevice               Deletes a device
|||
|||	  SYNOPSIS
|||
|||	     Err DeleteDevice( Item x )
|||
|||	  DESCRIPTION
|||
|||	     This macro deletes the specified device.  It can only be
|||	     called by system tasks.
|||
|||	  ARGUMENTS
|||
|||	     x             Input: The item number of the device to be
|||	                  deleted
|||
|||	  RETURN VALUE
|||
|||	     The procedure returns 0 if successful or an error code (a
|||	     negative value) if an error occurred.
|||
|||	  IMPLEMENTATION
|||
|||	     Macro implemented in device.h V20.
|||
|||	  ASSOCIATED FILES
|||
|||	     device.h
|||
|||	  CAVEATS
|||
|||	     This function does not work. What more can I say?
|||
|||	  SEE ALSO
|||
|||	     CreateDevice()
**/

int32
internalDeleteDevice(dev, t)
Device *dev;
struct Task *t;
{
	int32	err;

        if (dev->dev_OpenCnt)
        {
            return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_ItemStillOpened);
        }

        err = 0;

	if (dev->dev_DeleteDev)	 err = (*dev->dev_DeleteDev)(dev);

        if (err >= 0)
        {
#ifdef	SEMI_DEVICE
	    internalLockSemaphore(DevSemaphore,1);
#endif
	    REMOVENODE((Node *)dev);
#ifdef	SEMI_DEVICE
	    internalUnlockSemaphore(DevSemaphore);
#endif

            FreeMem(dev->dev_Extension,sizeof(DeviceExtension));
	}
	return err;
}


/*****************************************************************************/


#define FMVDEVICEKLUDGE

/* Load a device from external storage */
Item internalLoadDevice(char *name)
{
void   *context;
Item    result;
Device *dev;

    result = LoadModule("device",name,MKNODEID(KERNELNODE,DEVICENODE),&context);

#ifdef FMVDEVICEKLUDGE
    if (result < 0)
    {
        /* The device was not found. See if it is the extra-special,
         * extra-kludgy FMV device thing...
         *
         * What's happening here is that the demand-loader is trying to
         * find a device by the name "FMVAudioDevice". This file could not
         * be loaded from disk. It turns out that in the first version of the
         * FMV support code, the FMVAudioDevice is actually contained in the
         * same binary as the FMVVideoDevice. Therefore, if we couldn't locate
         * a stand-alone FMVAudioDevice file, we try to load the FMVVideoDevice
         * file, and launch it, in the hopes that it will start the
         * FMVAudioDevice that we need.
         *
         * This horrible kludge can be taken out as soon as the FMV device
         * is split into two separate binaries on disk.
         */

        if (strcasecmp(name,"FMVAudioDevice") == 0)
        {
            result = LoadModule("device","FMVVideoDevice",MKNODEID(KERNELNODE,DEVICENODE),&context);
            if (result > 0)
            {
                dev = (Device *)NodeToItem(FindNamedNode(KernelBase->kb_Devices,"FMVAudioDevice"));
                if (dev)
                {
                    result = dev->dev.n_Item;
                }
                else
                {
                    UnloadModule((ItemNode *)LookupItem(result),context);
                    result = MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);
                }
            }
        }
    }
#endif

    dev = (Device *)LookupItem(result);
    if (dev)
        dev->dev_Extension->de_DemandLoad = context;

    return result;
}


/*****************************************************************************/


/* Unload a device from memory */
Err internalUnloadDevice(Device *d)
{
    return UnloadModule((ItemNode *)d,d->dev_Extension->de_DemandLoad);
}


/*****************************************************************************/


Item
OpenDevice(dev,a)
Device *dev;
void *a;
{
	Item ret;
	DBUG(("OpenDevice() dev=%lx\n",(uint32)dev));
	if (dev->dev_Open) ret = (*dev->dev_Open)(dev);
	else {
		DBUG(("OpenDevice: '%s' has null dev_Open pointer\n",
		       dev->dev.n_Name));
		ret = 0;		/* sometimes we need this. */
	}
	DBUG(("OpenDevice ret=%lx\n",ret));
	if (ret < 0)	return ret;
	dev->dev_OpenCnt++;
	return dev->dev.n_Item;
}

/*****************************************************************************/


static void ReleaseDeviceIOReqs(Task *task, Device *dev)
{
Item   *ip;
int32   i;
Item    it;
uint32  cnt;
IOReq  *ior;

    if (task->t_Flags & TASK_EXITING)
    {
        /* don't bother if the task is going away */
        return;
    }

    /* first determine if the current task has more than one open on the
     * given device
     */

    ip  = task->t_ResourceTable;
    ip += task->t_ResourceCnt; /* go to last entry */
    cnt = 0;

    for (i = 0; i < task->t_ResourceCnt; i++)
    {
        it = *--ip;
        if (it == (dev->dev.n_Item | ITEM_WAS_OPENED))
            cnt++;
    }

    /* If there's only one open, it means the task is in the process of
     * closing the device for the last time. We must therefore throw away
     * any IOReqs for that device that the task might still have.
     */

    if (cnt == 1)
    {
        ip  = task->t_ResourceTable;
        ip += task->t_ResourceCnt; /* go to last entry */

        for (i = 0; i < task->t_ResourceCnt; i++)
        {
            it = *--ip;
            if (it >= 0)
            {
                ior = (IOReq *)CheckItem(it,KERNELNODE,IOREQNODE);
                if (ior && (ior->io_Dev == dev))
                {
#ifdef DEVELOPMENT
		    if (task->t_ThreadTask)
                        printf("WARNING: thread");
                    else
                        printf("WARNING: task");

                    printf(" '%s' is closing the '%s' device w/o having deleted ioreq $%06x\n",task->t.n_Name,dev->dev.n_Name,it);
                    if ((ior->io.n_Flags & NODE_NAMEVALID) && (ior->io.n_Name))
                        printf("         (name '%s')\n",ior->io.n_Name);
#endif
                    internalDeleteItem(it,task);
                }
            }
        }
    }
}


/*****************************************************************************/


int32
CloseDevice(dev,t)
Device *dev;
struct Task *t;
{
	if (dev->dev_Close)	(*dev->dev_Close)(dev);
	dev->dev_OpenCnt--;

        ReleaseDeviceIOReqs(CURRENTTASK,dev);

        if (!dev->dev_OpenCnt)
        {
            return internalUnloadDevice(dev);
        }

	return 0;
}


#ifdef undef
Item
fooIOReqUsingMemory(t,p,len)
Task *t;
char *p;
int32 len;
{
	Device *d;
	List *l = KernelBase->kb_Devices;
	for (d = (Device *)FIRSTNODE(l); ISNODE(l,d) ; d = (Device *)NEXTNODE(d) )
	{
	    int32 oldints;
	    List *iol = &d->dev_IOReqs;
	    IOReq *ior;
	    oldints =  Disable();
	    for (ior = (IOReq *)FIRSTNODE(iol); ISNODE(iol,ior);
			ior = (IOReq *)NEXTNODE(ior) )
	    {
		if ((ior->io_Flags & IO_DONE) == 0)
		{	/* IO in progress, check it */
		    /* We only need to check the writes */
		    IOInfo *ioi = &ior->io_Info;
		    if (ioi->ioi_Recv.iob_Len)
		    {
			/* recv buffer start later? */
			if ((int32)p >
			    ioi->ioi_Recv.iob_Len + (int32)ioi->ioi_Recv.iob_Buffer )
				continue;
			if ((int32)p + len < (int32)ioi->ioi_Recv.iob_Buffer )
				continue;
			Enable(oldints);
			return ior->io.n_Item;
		    }
		}
	    }
	    Enable(oldints);
	}
	return -1;
}
#endif


#ifdef	SEMI_DEVICE
TagArg dtags[2] =
{	TAG_ITEM_NAME, (void *)"DevList",
	TAG_END,	0,
};
#endif

int32 init_dev_sem(void) {
#ifdef	SEMI_DEVICE
    /* allocate device list semaphore */

    DevSemaphore = (Semaphore *)AllocateNode((Folio *)KernelBase,SEMA4NODE);
    if (!DevSemaphore)return -1;
    KernelBase->kb_DevSemaphore = internalCreateSemaphore(DevSemaphore,dtags);
    return KernelBase->kb_DevSemaphore;
#else
    return 0;
#endif
}

