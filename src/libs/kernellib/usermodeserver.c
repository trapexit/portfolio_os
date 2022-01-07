/* $Id: usermodeserver.c,v 1.23 1994/12/20 00:20:51 vertex Exp $ */

/* This module provides user-mode services for supervisor mode code. The
 * services currently include loading and running a module, as well as
 * unloading a previously loaded module.
 */

#include "types.h"
#include "msgport.h"
#include "mem.h"
#include "io.h"
#include "string.h"
#include "filesystem.h"
#include "filestream.h"
#include "filefunctions.h"
#include "filestreamfunctions.h"
#include "operror.h"
#include "aif.h"
#include "super.h"
#include "stdio.h"
#include "usermodeservices.h"
#include "debug.h"


/*****************************************************************************/


typedef struct ModuleData
{
    CodeHandle md_Code;
    uint32     md_NumItems;
    Item      *md_Items;     /* array */
    uint32     md_TableSize;
} ModuleData;


/*****************************************************************************/


static bool IsCodeRSASafe(CodeHandle code, _3DOBinHeader *hdr)
{
bool result;

    result = TRUE;
    if (hdr->_3DO_SignatureLen)
    {
        result = RSACheck((char *)code,hdr->_3DO_SignatureLen
                                     + hdr->_3DO_Signature);
    }

    return result;
}


/*****************************************************************************/


static _3DOBinHeader *ExtractBinHeader(CodeHandle code)
{
AIFHeader *aif;

    aif = (AIFHeader *)code;
    return (_3DOBinHeader *)&aif[1];
}


/*****************************************************************************/


#define LOADERTHREAD_STACK_SIZE 1600

/* Jump into a loaded code module. Once that's done, transfer ownership of
 * all items created by the module to the parent of the thread
 */
static void LoaderThread(Message *msg, ModuleData *md)
{
Err                result;
Item              *ip;
uint32             i;
Item               it;
uint32             numItems;
Item              *items;
Err                err;
UMSLoadModuleArgs  args;
_3DOBinHeader     *binHdr;

    memcpy(&args,msg->msg_DataPtr,msg->msg_DataSize);
    args.ums_DemandLoad = md;

    /* The code is loaded, so jump into it to let it initialize itself */
    result = ExecuteAsSubroutine(md->md_Code,args.ums_Argc,args.ums_Argv);

    /* If result >= 0, its value must be the return value of the service
     * message. This is typically the item number of the main item
     * created by the subroutine call, and is expected by the
     * caller of this service.
     */

    if (result >= 0)
    {
        /* return some values to the caller */
        binHdr                  = ExtractBinHeader(md->md_Code);
        args.ums_ItemVersion    = binHdr->_3DO_Item.n_Version;
        args.ums_ItemRevision   = binHdr->_3DO_Item.n_Revision;
        args.ums_ItemPrivileged = (binHdr->_3DO_Flags & _3DO_PRIVILEGE) ? TRUE : FALSE;

        /* Now here's the tricky part. Transfer
         * the ownership of all items in our
         * resource table to the parent task.
         * This is so that any items created
         * in ExecuteAsSub() remain in existence
         * even when this current thread dies.
         */

        numItems = CURRENTTASK->t_ResourceCnt;
        items = (Item *)AllocMem(numItems * sizeof(Item),MEMTYPE_ANY);
        if (items)
        {
            md->md_TableSize = numItems * sizeof(Item);
            md->md_Items     = items;

            ip  = CURRENTTASK->t_ResourceTable;
            ip += CURRENTTASK->t_ResourceCnt;       /* go to last entry */

            numItems = 0;
            for (i = 0; i < CURRENTTASK->t_ResourceCnt; i++)
            {
                it                     = *--ip;
                md->md_Items[numItems] = it;
                if (it >= 0)
                {
                ItemNode *in;

                    in = (ItemNode *)LookupItem(it);
                    if (in)
                    {
                        numItems++;

                        if (it & ITEM_WAS_OPENED)
                        {
                            printf("WARNING: A demand-loaded component's main() opened an item.\n");
                            printf("         This is currently not supported.\n");

                            /* !!! we need a way to transfer this opened item
                             *     to the operator.
                             */
                            err = NOSUPPORT;
                        }
                        else
                        {
                            err = SetItemOwner(it,CURRENTTASK->t.n_Owner);
                        }
#ifdef DEVELOPMENT
                        if (err < 0)
                        {
                            printf("WARNING: Unable to transfer ownership of item $%06x\n",it);
                            printf("         (type %d, subtype %d) to '%s', err %d\n",in->n_Type,in->n_SubsysType,((Task *)LookupItem(CURRENTTASK->t.n_Owner))->t.n_Name,err);
                        }
#endif
                    }
                }
            }
            md->md_NumItems = numItems;

            /* ignore failures of SetItemOwner() */
            ReplyMsg(msg->msg.n_Item,result,&args,msg->msg_DataSize);
            return;
        }
        else
        {
            /* not enough memory to create item tracking table */
            result = NOMEM;
        }
    }

    /* nuke everything.... */
    UnloadCode(md->md_Code);
    FreeMem(md,sizeof(ModuleData));
    ReplyMsg(msg->msg.n_Item,result,NULL,0);
}


/*****************************************************************************/


/* Load code into memory */
static void LoadCodeModule(Message *msg)
{
ModuleData        *md;
Err                result;
void              *stack;
UMSLoadModuleArgs *args;
_3DOBinHeader     *binHdr;

    args = (UMSLoadModuleArgs *)msg->msg_DataPtr;

    md = (ModuleData *)AllocMem(sizeof(ModuleData),MEMTYPE_FILL);
    if (md)
    {
        result = LoadCode(args->ums_ItemPath, &md->md_Code);
        if (result >= 0)
        {
            binHdr = ExtractBinHeader(md->md_Code);

            /* The privileged state of the loaded code must
             * match that of the current task. This is to prevent
             * privileged code from running in user space, or user code
             * running in privileged space.
             */

            if (binHdr->_3DO_Flags & _3DO_PRIVILEGE)
            {
                if (!(CURRENTTASK->t.n_Flags & TASK_SUPER))
                {
                    result = BADPRIV;
                }
                else if (!IsCodeRSASafe(md->md_Code,binHdr))
                {
                    result = MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_Kr_RSAFail);
                }
            }
            else
            {
                if (CURRENTTASK->t.n_Flags & TASK_SUPER)
                    result = BADPRIV;
            }

            if (strcasecmp(binHdr->_3DO_Name,args->ums_ItemName) != 0)
                result = BADNAME;

            if (binHdr->_3DO_Item.n_Type != args->ums_ItemType)
                result = MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_BadType);

            if (binHdr->_3DO_Item.n_SubsysType != args->ums_ItemSubsysType)
                result = BADSUBTYPE;

            if (result >= 0)
            {
                /* spawn a thread to handle the rest of the service */

                stack = AllocMem(LOADERTHREAD_STACK_SIZE,MEMTYPE_ANY);
                if (stack)
                {
                    result = CreateItemVA(MKNODEID(KERNELNODE,TASKNODE),
                                          TAG_ITEM_NAME,                 "LoaderThread",
                                          CREATETASK_TAG_PC,             LoaderThread,
                                          CREATETASK_TAG_STACKSIZE,      LOADERTHREAD_STACK_SIZE,
                                          CREATETASK_TAG_SP,             (uint32)stack + LOADERTHREAD_STACK_SIZE,
                                          CREATETASK_TAG_ARGC,           msg,
                                          CREATETASK_TAG_ARGP,           md,
                                          CREATETASK_TAG_ALLOCDTHREADSP, 0,
                                          (CURRENTTASK->t.n_Flags & TASK_SUPER ? CREATETASK_TAG_SUPER : TAG_NOP), 0,
                                          TAG_END);

                    if (result >= 0)
                        return;

                    FreeMem(stack,LOADERTHREAD_STACK_SIZE);
                }
                else
                {
                    /* no memory for stack */
                    result = NOMEM;
                }
            }
            UnloadCode(md->md_Code);
        }
        FreeMem(md,sizeof(ModuleData));
    }
    else
    {
        /* no memory for the module data we need */
        result = NOMEM;
    }

    ScavengeMem();

    ReplyMsg(msg->msg.n_Item,result,NULL,0);
}


/*****************************************************************************/


typedef struct UnloaderThreadParms
{
    int32       utp_ParentSig;
    int32       utp_ChildSig;
    ModuleData *utp_ModuleData;
} UnloaderThreadParms;


#define UNLOADERTHREAD_STACK_SIZE 1024

static void UnloaderThread(Message *msg, UnloaderThreadParms *utp)
{
ModuleData          *md;
Err                  result;
uint32               numItems;
UMSUnloadModuleArgs *args;

    md   = utp->utp_ModuleData;
    args = (UMSUnloadModuleArgs *)msg->msg_DataPtr;

    /* sync with the parent */
    utp->utp_ChildSig = AllocSignal(0);
    SendSignal(CURRENTTASK->t.n_Owner,utp->utp_ParentSig);
    WaitSignal(utp->utp_ChildSig);

    /* At this point, this thread has ownership of all items that the
     * module being unloaded has created.
     */

    /* now give a chance to the module itself to clean things up  */
    result = ExecuteAsSubroutine(md->md_Code,args->ums_Argc,args->ums_Argv);
    if (result >= 0)
    {
        /* if the module said it could go away, nuke its items, its code, and
         * other data we maintain for it.
         */

        numItems = md->md_NumItems;
        while (numItems--)
        {
            if (md->md_Items[numItems] & ITEM_WAS_OPENED)
                CloseItem(md->md_Items[numItems]);
            else
                DeleteItem(md->md_Items[numItems]);
        }

        UnloadCode(md->md_Code);
        FreeMem(md->md_Items,md->md_TableSize);
        FreeMem(md,sizeof(ModuleData));
    }
    else
    {
        /* Oh no! The module won't unload itself! So return the ownership
         * of the items back to our parent so that when we die we don't
         * end up freeing them.
         */

        numItems = md->md_NumItems;
        while (numItems--)
        {
            if (md->md_Items[numItems] & ITEM_WAS_OPENED)
            {
                /* !!! need to transfer the opened item, but there's no
                 * way to do that for now, so complain vehemently
                 */
                printf("Demand-loader is confused!\n");
            }
            else
            {
                SetItemOwner(md->md_Items[numItems],CURRENTTASK->t.n_Owner);
            }
        }
    }

    ScavengeMem();

    /* return this to the caller of the service */
    ReplyMsg(msg->msg.n_Item,result,NULL,0);
}


/*****************************************************************************/


/* Unload a chunk of code that was previously loaded */
static void UnloadCodeModule(Message *msg)
{
ModuleData           *md;
Err                  result;
uint32               numItems;
void                *stack;
UnloaderThreadParms  utp;
UMSUnloadModuleArgs *args;

    result = 0;

    /* find the universe */
    args = (UMSUnloadModuleArgs *)msg->msg_DataPtr;
    md   = (ModuleData *)args->ums_DemandLoad;

    if (md)
    {
        utp.utp_ModuleData = md;

        utp.utp_ParentSig = AllocSignal(0);
        if (utp.utp_ParentSig > 0)
        {
            stack = AllocMem(UNLOADERTHREAD_STACK_SIZE,MEMTYPE_ANY);
            if (stack)
            {
                /* spawn a thread to tear things down asynchronously */
                result = CreateItemVA(MKNODEID(KERNELNODE,TASKNODE),
                                      TAG_ITEM_NAME,                 "UnloaderThread",
                                      CREATETASK_TAG_PC,             UnloaderThread,
                                      CREATETASK_TAG_STACKSIZE,      UNLOADERTHREAD_STACK_SIZE,
                                      CREATETASK_TAG_SP,             (uint32)stack + UNLOADERTHREAD_STACK_SIZE,
                                      CREATETASK_TAG_ARGC,           msg,
                                      CREATETASK_TAG_ARGP,           &utp,
                                      CREATETASK_TAG_ALLOCDTHREADSP, 0,
                                      (CURRENTTASK->t.n_Flags & TASK_SUPER ? CREATETASK_TAG_SUPER : TAG_NOP), 0,
                                      TAG_END);
                if (result >= 0)
                {
                    /* Transfer the ownership of all module items to the thread,
                     * so they will be deleted or closed when the thread dies
                     */

                    numItems = md->md_NumItems;
                    while (numItems--)
                    {
                        if (md->md_Items[numItems] & ITEM_WAS_OPENED)
                        {
                            /* !!! need to transfer the opened item, but there's no
                             * way to do that for now, so complain vehemently
                             */
                            printf("Demand-loader is confused!\n");
                        }
                        else
                        {
                            SetItemOwner(md->md_Items[numItems],result);
                        }
                    }

                    /* wait for our child to be ready */
                    WaitSignal(utp.utp_ParentSig);

                    /* let the child go... */
                    SendSignal(result,utp.utp_ChildSig);

                    FreeSignal(utp.utp_ParentSig);
                    return;
                }
                else
                {
                    FreeMem(stack,UNLOADERTHREAD_STACK_SIZE);
                }
            }
            else
            {
                result = NOMEM;
            }
            FreeSignal(utp.utp_ParentSig);
        }
        else
        {
            result = MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_NoSigs);;
        }
    }

    ReplyMsg(msg->msg.n_Item,result,NULL,0);
}


/*****************************************************************************/


typedef void (* ServiceFunc)(Message *);

static ServiceFunc serviceFuncs[] =
{
    LoadCodeModule,
    UnloadCodeModule
};


void ProvideServices(Item serverPort)
{
Item       msgItem;
Message   *msg;
Err        result;
UMSHeader *hdr;

    while (TRUE)
    {
        ScavengeMem();

        /* extract a command from the server port */

        msgItem = GetMsg(serverPort);
        if (msgItem <= 0)
            return;

        /* validate the message */

        msg = (Message *)LookupItem(msgItem);
        if (msg->msg.n_ItemFlags & ITEMNODE_PRIVILEGED)
        {
            if (msg->msg_DataSize >= sizeof(UMSHeader))
            {
                hdr = (UMSHeader *)msg->msg_DataPtr;
                if (hdr->ums_Service <= UMS_UNLOADMODULE)
                {
                    /* Call the service function. It'll reply the message */
                    (*serviceFuncs[hdr->ums_Service])(msg);
                    continue;
                }
                else
                {
                    /* being asked for a service we don't know about */
                    result = NOSUPPORT;
                }
            }
            else
            {
                /* not a data size that is expected */
                result = MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadSize);
            }
        }
        else
        {
            /* message wasn't sent by kernel, something smells fishy... */
            result = BADPRIV;
        }
        ReplyMsg(msgItem,result,NULL,0);
    }
}
