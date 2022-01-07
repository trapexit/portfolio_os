/* $Id: iostress.c,v 1.10.1.3 1995/02/06 19:27:15 vertex Exp $ */

/**
|||	AUTODOC PUBLIC tpg/shell/iostress
|||	iostress - Put some stress of the IO subsystem to uncover title bugs
|||
|||	  Format
|||
|||	    iostress [-quit]
|||
|||	  Description
|||
|||	    This command installs a set of monitoring routines which
|||	    catch incorrect use of the IO subsystem.
|||
|||	    The monitoring routines ensure the following things:
|||
|||	      - When writing data, the routines guarantee that the buffer used
|||	        for the IO remains consistent and does not get altered while
|||	        the write operation occurs. This guarantees that a title is not
|||	        fiddling with data that is in the process of being fetched by
|||	        an IO module.
|||
|||	      - When reading data, the routines guarantee that it is not
|||	        possible to read any data associated with the IO request
|||	        until the IO request has completed. This guarantees that a
|||	        title is not reading data before it has been completely
|||	        transfered.
|||
|||	    The monitoring routines do their work by allocating temporary
|||	    buffers and using these for the IO transactions. Doing so
|||	    requires some extra memory, and requires some extra CPU time to
|||	    copy data between the buffers. This can affect the performance
|||	    of a title, possibly causing some jerky animations, but should
|||	    never cause a crash, or trashed sound or graphics. If these
|||	    things happen, then the title has a bug.
|||
|||	  Arguments
|||
|||	    -quit                       Tells a previously started version of
|||	                                iostress to exit. (Quiting is
|||	                                a tricky operation and may not always
|||	                                work. The safest way to "quit" is by
|||	                                resetting the system)
|||
|||	  Implementation
|||
|||	    Command implemented in V24.
|||
|||	  Location
|||
|||	    $c/iostress
**/

#include "types.h"
#include "folio.h"
#include "io.h"
#include "device.h"
#include "kernel.h"
#include "list.h"
#include "mem.h"
#include "operror.h"
#include "super.h"
#include "stdio.h"
#include "string.h"
#include "sherryvers.h"


/*****************************************************************************/


#define Error(x,err)      {printf(x); PrintfSysErr(err);}
#define IOSTRESS_PORTNAME  "IOStress"


/*****************************************************************************/


/* packet of options passed from main code to main loop */
typedef struct OptionPacket
{
    bool   op_Quit;
} OptionPacket;

typedef struct TransferInfo
{
    uint32  ti_Size;
    void   *ti_TempBuffer;
    void   *ti_RealBuffer;
} TransferInfo;

typedef struct Transaction
{
    MinNode       tr_Link;
    uint32        tr_SendBufferCRC;
    TransferInfo  tr_Recv;
} Transaction;


/*****************************************************************************/


static bool    quit;
static Item    optPort;
static Device *sportDevice;

/* cache for unused transaction structures */
static List   freeTransactions = INITLIST(freeTransactions,NULL);
static uint32 freeTransactionCount; /* count the number of caches structures   */


/*****************************************************************************/


static uint32 CalcCRC(char *data, uint32 numBytes)
{
uint32 i, w, crc, index;
static uint32 crc32table[256] =
{
    0x00000000,0x04c11db7,0x09823b6e,0x0d4326d9,0x130476dc,0x17c56b6b,0x1a864db2,0x1e475005,
    0x2608edb8,0x22c9f00f,0x2f8ad6d6,0x2b4bcb61,0x350c9b64,0x31cd86d3,0x3c8ea00a,0x384fbdbd,
    0x4c11db70,0x48d0c6c7,0x4593e01e,0x4152fda9,0x5f15adac,0x5bd4b01b,0x569796c2,0x52568b75,
    0x6a1936c8,0x6ed82b7f,0x639b0da6,0x675a1011,0x791d4014,0x7ddc5da3,0x709f7b7a,0x745e66cd,
    0x9823b6e0,0x9ce2ab57,0x91a18d8e,0x95609039,0x8b27c03c,0x8fe6dd8b,0x82a5fb52,0x8664e6e5,
    0xbe2b5b58,0xbaea46ef,0xb7a96036,0xb3687d81,0xad2f2d84,0xa9ee3033,0xa4ad16ea,0xa06c0b5d,
    0xd4326d90,0xd0f37027,0xddb056fe,0xd9714b49,0xc7361b4c,0xc3f706fb,0xceb42022,0xca753d95,
    0xf23a8028,0xf6fb9d9f,0xfbb8bb46,0xff79a6f1,0xe13ef6f4,0xe5ffeb43,0xe8bccd9a,0xec7dd02d,
    0x34867077,0x30476dc0,0x3d044b19,0x39c556ae,0x278206ab,0x23431b1c,0x2e003dc5,0x2ac12072,
    0x128e9dcf,0x164f8078,0x1b0ca6a1,0x1fcdbb16,0x018aeb13,0x054bf6a4,0x0808d07d,0x0cc9cdca,
    0x7897ab07,0x7c56b6b0,0x71159069,0x75d48dde,0x6b93dddb,0x6f52c06c,0x6211e6b5,0x66d0fb02,
    0x5e9f46bf,0x5a5e5b08,0x571d7dd1,0x53dc6066,0x4d9b3063,0x495a2dd4,0x44190b0d,0x40d816ba,
    0xaca5c697,0xa864db20,0xa527fdf9,0xa1e6e04e,0xbfa1b04b,0xbb60adfc,0xb6238b25,0xb2e29692,
    0x8aad2b2f,0x8e6c3698,0x832f1041,0x87ee0df6,0x99a95df3,0x9d684044,0x902b669d,0x94ea7b2a,
    0xe0b41de7,0xe4750050,0xe9362689,0xedf73b3e,0xf3b06b3b,0xf771768c,0xfa325055,0xfef34de2,
    0xc6bcf05f,0xc27dede8,0xcf3ecb31,0xcbffd686,0xd5b88683,0xd1799b34,0xdc3abded,0xd8fba05a,
    0x690ce0ee,0x6dcdfd59,0x608edb80,0x644fc637,0x7a089632,0x7ec98b85,0x738aad5c,0x774bb0eb,
    0x4f040d56,0x4bc510e1,0x46863638,0x42472b8f,0x5c007b8a,0x58c1663d,0x558240e4,0x51435d53,
    0x251d3b9e,0x21dc2629,0x2c9f00f0,0x285e1d47,0x36194d42,0x32d850f5,0x3f9b762c,0x3b5a6b9b,
    0x0315d626,0x07d4cb91,0x0a97ed48,0x0e56f0ff,0x1011a0fa,0x14d0bd4d,0x19939b94,0x1d528623,
    0xf12f560e,0xf5ee4bb9,0xf8ad6d60,0xfc6c70d7,0xe22b20d2,0xe6ea3d65,0xeba91bbc,0xef68060b,
    0xd727bbb6,0xd3e6a601,0xdea580d8,0xda649d6f,0xc423cd6a,0xc0e2d0dd,0xcda1f604,0xc960ebb3,
    0xbd3e8d7e,0xb9ff90c9,0xb4bcb610,0xb07daba7,0xae3afba2,0xaafbe615,0xa7b8c0cc,0xa379dd7b,
    0x9b3660c6,0x9ff77d71,0x92b45ba8,0x9675461f,0x8832161a,0x8cf30bad,0x81b02d74,0x857130c3,
    0x5d8a9099,0x594b8d2e,0x5408abf7,0x50c9b640,0x4e8ee645,0x4a4ffbf2,0x470cdd2b,0x43cdc09c,
    0x7b827d21,0x7f436096,0x7200464f,0x76c15bf8,0x68860bfd,0x6c47164a,0x61043093,0x65c52d24,
    0x119b4be9,0x155a565e,0x18197087,0x1cd86d30,0x029f3d35,0x065e2082,0x0b1d065b,0x0fdc1bec,
    0x3793a651,0x3352bbe6,0x3e119d3f,0x3ad08088,0x2497d08d,0x2056cd3a,0x2d15ebe3,0x29d4f654,
    0xc5a92679,0xc1683bce,0xcc2b1d17,0xc8ea00a0,0xd6ad50a5,0xd26c4d12,0xdf2f6bcb,0xdbee767c,
    0xe3a1cbc1,0xe760d676,0xea23f0af,0xeee2ed18,0xf0a5bd1d,0xf464a0aa,0xf9278673,0xfde69bc4,
    0x89b8fd09,0x8d79e0be,0x803ac667,0x84fbdbd0,0x9abc8bd5,0x9e7d9662,0x933eb0bb,0x97ffad0c,
    0xafb010b1,0xab710d06,0xa6322bdf,0xa2f33668,0xbcb4666d,0xb8757bda,0xb5365d03,0xb1f740b4
};

    crc = 0xffffffff;
    for (i = 0; i < numBytes; i++)
    {
	w = (uint32)*data++;
	index = (crc >> 24) ^ w;
	crc = (crc << 8) ^ crc32table[index & 0x000000ff];
    }

    return (crc);
}


/*****************************************************************************/


static bool AllocTransferInfo(TransferInfo *ti, IOBuf *iob)
{
    ti->ti_RealBuffer = iob->iob_Buffer;
    ti->ti_Size       = iob->iob_Len;

    if (!ti->ti_Size)
        return TRUE;

    ti->ti_TempBuffer = SuperAllocMem(ti->ti_Size,MEMTYPE_ANY);
    if (!ti->ti_TempBuffer)
        return FALSE;

    iob->iob_Buffer = ti->ti_TempBuffer;

    return TRUE;
}


/*****************************************************************************/


static void FreeTransferInfo(TransferInfo *ti)
{
    if (ti->ti_TempBuffer)
    {
        SuperFreeMem(ti->ti_TempBuffer,ti->ti_Size);
        ti->ti_TempBuffer = NULL;
    }
}


/*****************************************************************************/


static Transaction *AllocTransaction(void)
{
Transaction *tr;

    if (freeTransactionCount)
    {
        freeTransactionCount--;
        tr = (Transaction *)RemHead(&freeTransactions);
    }
    else
    {
        tr = (Transaction *)SuperAllocMem(sizeof(Transaction),MEMTYPE_ANY);
    }

    return tr;
}


/*****************************************************************************/


static void FreeTransaction(Transaction *tr)
{
    if (freeTransactionCount > 32)
    {
        /* don't cache more than 32 transactions */
        SuperFreeMem(tr,sizeof(Transaction));
    }
    else
    {
        freeTransactionCount++;
        AddHead(&freeTransactions,(Node *)tr);
    }
}


/*****************************************************************************/


static void CleanupIO(IOReq *ior)
{
Transaction *tr;

    /* WARNING: we use this private pointer to find the parent node quickly */
    tr = (Transaction *)ior->io.n_ReservedP;
    if (tr)
    {
        if (tr->tr_Recv.ti_TempBuffer)
        {
            /* copy from the temporary buffer to the real target buffer */
            memcpy(tr->tr_Recv.ti_RealBuffer,tr->tr_Recv.ti_TempBuffer,tr->tr_Recv.ti_Size);
            FreeTransferInfo(&tr->tr_Recv);
            ior->io_Info.ioi_Recv.iob_Buffer = tr->tr_Recv.ti_RealBuffer;
        }

        if (ior->io_Info.ioi_Send.iob_Len)
        {
            /* make sure the buffer being written out hasn't changed illegally */
            if (CalcCRC((char *)ior->io_Info.ioi_Send.iob_Buffer,ior->io_Info.ioi_Send.iob_Len) != tr->tr_SendBufferCRC)
            {
                Superkprintf("WARNING: Send buffer contents were altered before IO completed!\n");
                Superkprintf("         IOReq Item $%05x, buffer $%x\n",ior->io.n_Item,ior->io_Info.ioi_Send.iob_Buffer);
            }
        }
        FreeTransaction(tr);
        ior->io.n_ReservedP = NULL;

        ScavengeMem();
    }
}


/*****************************************************************************/


static Err StartIO(IOReq *ior, IOInfo *ioInfo, bool wait)
{
Transaction *tr;
Err          ret;

    if (!ior)
        return BADITEM;

    if (!SuperIsRamAddr(ioInfo,sizeof(IOInfo)))
    {
        /* IOInfo not readable */
        return BADPTR;
    }

    if ((uint32)ioInfo & 3)
    {
        /* IOInfo is not suitably aligned */
        return BADPTR;
    }

    if (ior->io.n_Owner != CURRENTTASK->t.n_Item)
    {
        /* current task is not owner of IO */
        return NOTOWNER;
    }

    if (!(ior->io_Flags & IO_DONE))
    {
        /* IO is still being used */
        return MAKEKERR(ER_SEVER,ER_C_STND,ER_IONotDone);
    }

    if (ioInfo->ioi_Flags2)
    {
        /* all bits must be 0 for now */
        return BADIOARG;
    }

    if (ioInfo->ioi_Flags & ~IO_QUICK)
    {
        /* only IO_QUICK can be set for now */
        return BADIOARG;
    }

    if (ioInfo->ioi_Unit > ior->io_Dev->dev_MaxUnitNum)
    {
        /* out-of-range unit number */
        return BADUNIT;
    }

    ior->io.n_ReservedP = 0;
    ior->io_Info        = *ioInfo;
    ior->io_CallBack    = NULL;

    if (wait)
        ior->io_Info.ioi_Flags |= IO_QUICK;

    if (ior->io_Info.ioi_Recv.iob_Len)
    {
        if (SuperValidateMem(CURRENTTASK,ior->io_Info.ioi_Recv.iob_Buffer,ior->io_Info.ioi_Recv.iob_Len) < 0)
        {
            /* receive buffer not writable */
            return BADPTR;
        }
    }

    if (ior->io_Info.ioi_Send.iob_Len)
    {
        if (!SuperIsRamAddr(ior->io_Info.ioi_Send.iob_Buffer,ior->io_Info.ioi_Send.iob_Len))
        {
            /* send buffer not readable */
            return BADPTR;
        }
    }

    if (ior->io_Dev != sportDevice)
    {
        /* get a transaction structure for the IO request */
        tr = AllocTransaction();
        if (tr)
        {
            if (AllocTransferInfo(&tr->tr_Recv,&ior->io_Info.ioi_Recv))
            {
                if (ior->io_Info.ioi_Send.iob_Len)
                {
                    /* calculate a CRC on the output buffer, so we verify it
                     * doesn't change during the write operation.
                     */
                    tr->tr_SendBufferCRC = CalcCRC((char *)ior->io_Info.ioi_Send.iob_Buffer,ior->io_Info.ioi_Send.iob_Len);
                }

                /* set the real buffer to garbage to avoid all confusion */
                if (tr->tr_Recv.ti_RealBuffer)
                    memset(tr->tr_Recv.ti_RealBuffer,0xf7,tr->tr_Recv.ti_Size);

                /* WARNING: we're using the reserved pointer for private storage! */
                ior->io.n_ReservedP = tr;

                /* perform the IO for real, and wait for completion if we need to */
                ret = SuperinternalSendIO(ior);
                if (ret != 0)
                {
                    /* If the IO is not deferred, we must clean up now.
                     * If the IO is deferred, clean up will happen when
                     * CompleteIO() is called by the driver.
                     */
                    CleanupIO(ior);
                }

                return ret;
            }
            FreeTransaction(tr);
        }
        ret = NOMEM;
    }
    else
    {
        /* Perform the IO without any special magic if we're talking to the
         * sport device.
         */
        ret = SuperinternalSendIO(ior);
    }

    return ret;
}


/*****************************************************************************/


static void internalCompleteIOPatch(IOReq *ior)
{
again:
    ior->io_Flags |= IO_DONE;
    if (ior->io_CallBack)
    {
	IOReq *newior = (*ior->io_CallBack)(ior);
	if (newior)
	{
	    int32 ret;
	    newior->io_Flags |= IO_INTERNAL;
	    ret = SuperInternalSendIO(newior);
	    newior->io_Flags &= ~IO_INTERNAL;
	    if (ret)
	    {	/* Done with this one too! */
		ior = newior;
		goto again;
	    }
	}
    }
    else if ((ior->io_Flags & IO_QUICK) == 0)
    {
	Msg *msg = (Msg *)LookupItem(ior->io_MsgItem);
	Task *t;

	if (msg->msg.n_Type == MESSAGENODE)
	{
	    if (SuperDiscOsVersion(0) <= DiscOs_1_4)
	    {
	        /* For old titles from Digital Pictures (like NightTrap), we
	         * must return the result field and data size fields set to 0.
	         */
                SuperReplyMsg(msg->msg.n_Item,0,(void *)ior->io.n_Item,0);
            }
            else
            {
                SuperReplyMsg(msg->msg.n_Item,ior->io_Error,(void *)ior->io.n_Item,ior->io_Info.ioi_User);
            }
	}
	else
	{
	    t = (Task *)LookupItem(ior->io.n_Owner);
	    SuperInternalSignal(t,SIGF_IODONE);
	}

	if (ior->io_SigItem)
	{
	    t = (Task *)CheckItem(ior->io_SigItem, KERNELNODE, TASKNODE);
	    if (t) SuperInternalSignal(t, SIGF_ONESHOT);
	}
    }

    CleanupIO(ior);
}


/*****************************************************************************/


static void externalCompleteIOPatch(IOReq *ior)
{
    if (!(CURRENTTASK->t.n_Flags & TASK_SUPER))
    {
        /* only privileged tasks can call this */
        return;
    }

    internalCompleteIOPatch(ior);
}


/*****************************************************************************/


static Err externalDoIOPatch(Item iorItem, IOInfo *ioInfo)
{
Err    ret;
IOReq *ior;

    ior = (IOReq *)CheckItem(iorItem,KERNELNODE,IOREQNODE);
    ret = StartIO(ior, ioInfo, TRUE);

    /* Wait only if the IO was deferred (done asynchronously) */
    if (ret == 0)
        ret = SuperInternalWaitIO(ior);

    if (ret >= 0)
        ret = ior->io_Error;

    return ret;
}


/*****************************************************************************/


static Err externalSendIOPatch(Item iorItem, IOInfo *ioInfo)
{
    return StartIO((IOReq *)CheckItem(iorItem, KERNELNODE, IOREQNODE), ioInfo, FALSE);
}


/*****************************************************************************/


static void ApplyOP(OptionPacket *op)
{
    quit = op->op_Quit;
}


/*****************************************************************************/


static void MainLoop(OptionPacket *op)
{
Item     msgItem;
Message *msg;
void    *originalexternalDoIO;
void    *originalexternalSendIO;
void    *originalinternalCompleteIO;
void    *originalexternalCompleteIO;
bool     patched;

    freeTransactionCount = 0;

    sportDevice = (Device *)LookupItem(FindDevice("SPORT"));

    quit    = FALSE;
    patched = FALSE;
    originalexternalDoIO       = NULL;
    originalexternalSendIO     = NULL;
    originalexternalCompleteIO = NULL;
    originalinternalCompleteIO = NULL;

    ApplyOP(op);

    while (!quit)
    {
        if (!quit)
        {
            if (!patched)
            {
                originalexternalDoIO       = SetFunction(1,37,VTYPE_SWI,(void *)externalDoIOPatch);
                originalexternalSendIO     = SetFunction(1,24,VTYPE_SWI,(void *)externalSendIOPatch);
                originalexternalCompleteIO = SetFunction(1,34,VTYPE_SWI,(void *)externalCompleteIOPatch);
                originalinternalCompleteIO = SetFunction(1,-8,VTYPE_SUPER,(void *)internalCompleteIOPatch);
                patched                    = TRUE;
                printf("iostress: now stressing IO subsystem\n");
            }
        }

        msgItem = WaitPort(optPort,0);
        msg     = (Message *)LookupItem(msgItem);
        ApplyOP((OptionPacket *)msg->msg_DataPtr);
        ReplyMsg(msgItem,0,NULL,0);
    }

    if (patched)
    {
        /* !!! Ouch! We should check for outstanding IOs, and do something
         *     intelligent.
         */

        SetFunction(1,37,VTYPE_SWI,originalexternalDoIO);
        SetFunction(1,24,VTYPE_SWI,originalexternalSendIO);
        SetFunction(1,34,VTYPE_SWI,originalexternalCompleteIO);
        SetFunction(1,-8,VTYPE_SUPER,originalinternalCompleteIO);
        printf("iostress: no longer stressing IO subsystem\n");
    }
}


/*****************************************************************************/


int main(int32 argc, char **argv)
{
int          parm;
OptionPacket op;
Item         repPort;
Item         msg;
Item         err;

    memset(&op,0,sizeof(op));

    for (parm = 1; parm < argc; parm++)
    {
        if ((strcmp("-help",argv[parm]) == 0)
         || (strcmp("-?",argv[parm]) == 0)
         || (strcmp("help",argv[parm]) == 0)
         || (strcmp("?",argv[parm]) == 0))
        {
            printf("iostress: stress the IO subsystem to find bugs\n");
            printf("  -quit - quit the program\n");
            return (0);
        }

        if (strcmp("-quit",argv[parm]) == 0)
        {
            op.op_Quit = TRUE;
        }
    }

    /* Now that the options are parsed, we need to figure out what to do
     * with them.
     *
     * This code checks to see if another version of IOStress is already
     * running. If it is already running, then a message is sent to it with the
     * new options. If IOStress is not already running, we initialize
     * the universe, and jump into the metering loop.
     */

    optPort = FindMsgPort(IOSTRESS_PORTNAME);
    if (optPort >= 0)
    {
        repPort = CreateMsgPort(NULL,0,0);
        if (repPort >= 0)
        {
            msg = CreateMsg(NULL,0,repPort);
            if (msg >= 0)
            {
                err = SendMsg(optPort,msg,&op,sizeof(OptionPacket));
                if (err >= 0)
                {
                    WaitPort(repPort,msg);
                }
                else
                {
                    Error("Unable to send a message: ",err);
                }
            }
            else
            {
                Error("Unable to create a message: ",msg);
            }
            DeleteMsg(msg);
        }
        else
        {
            Error("Unable to create a message port: ",repPort);
        }
        DeleteMsgPort(repPort);
    }
    else
    {
        optPort = CreateMsgPort(IOSTRESS_PORTNAME,0,0);
        if (optPort >= 0)
        {
            MainLoop(&op);
            DeleteMsgPort(optPort);
        }
        else
        {
            Error("Unable to create message port: ",optPort);
        }
    }
}
