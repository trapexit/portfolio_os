/* $Id: usermodeservices.c,v 1.13 1994/09/30 18:36:26 vertex Exp $ */

#include "types.h"
#include "item.h"
#include "msgport.h"
#include "internalf.h"
#include "kernel.h"
#include "string.h"
#include "stdio.h"
#include "operror.h"
#include "usermodeservices.h"


/*****************************************************************************/


Item privServerPort    = -1;
Item nonprivServerPort = -1;


/*****************************************************************************/


Err UserModeService(void *umsArgs, uint32 umsArgSize, bool privilegedServer)
{
Item    msgItem;
Msg    *msg;
Item    replyPort;
TagArg  tags[3];
Err     result;
Item    serverPort;

    if (privilegedServer)
    {
        if (privServerPort == -1)
            privServerPort = NodeToItem(FindNamedNode(KernelBase->kb_MsgPorts,"PrivServer"));

        serverPort = privServerPort;
    }
    else
    {
        if (nonprivServerPort == -1)
            nonprivServerPort = NodeToItem(FindNamedNode(KernelBase->kb_MsgPorts,"NonPrivServer"));

        serverPort = nonprivServerPort;
    }

    if (serverPort >= 0)
    {
        replyPort = internalCreateSizedItem(MKNODEID(KERNELNODE,MSGPORTNODE),NULL,0);
        if (replyPort >= 0)
        {
            tags[0].ta_Tag = CREATEMSG_TAG_REPLYPORT;
            tags[0].ta_Arg = (void *)replyPort;
            tags[1].ta_Tag = CREATEMSG_TAG_DATA_SIZE;
            tags[1].ta_Arg = (void *)umsArgSize;
            tags[2].ta_Tag = 0;
            msgItem = internalCreateSizedItem(MKNODEID(KERNELNODE,MESSAGENODE),tags,0);
            if (msgItem >= 0)
            {
                msg = (Msg *)LookupItem(msgItem);

                /* tell the recipient that it really came from the kernel... */
                msg->msg.n_ItemFlags |= ITEMNODE_PRIVILEGED;

                result = externalPutMsg(serverPort, msgItem, umsArgs, umsArgSize);
                if (result >= 0)
                {
                    externalWaitPort(replyPort, msgItem);
                    result = msg->msg_Result;
                    if (result >= 0)
                        memcpy(umsArgs,msg->msg_DataPtr,umsArgSize);
                }

                externalDeleteItem(msgItem);
            }
            else
            {
                result = msgItem;
            }
            externalDeleteItem(replyPort);
        }
        else
        {
            result = replyPort;
        }
    }
    else
    {
        result = MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);
    }

    return (result);
}


/*****************************************************************************/


/* Load a module from external storage */
Item LoadModule(const char *typeName, const char *itemName, int32 cntype,
                void **context)
{
UMSLoadModuleArgs lmod;
Err               result;
char              path[FILESYSTEM_PART_PATH_LEN + 30];
ItemNode         *it;

    /* is the name short enough? */
    if (strlen(itemName) > FILESYSTEM_PART_PATH_LEN)
        return BADNAME;

    /* init a service request */
    lmod.ums_Hdr.ums_Service = UMS_LOADMODULE;
    lmod.ums_Argc            = DEMANDLOAD_MAIN_CREATE;
    lmod.ums_Argv            = NULL;
    lmod.ums_DemandLoad      = NULL;
    lmod.ums_ItemPath        = path;
    lmod.ums_ItemName        = (char *)itemName;
    lmod.ums_ItemType        = NODEPART(cntype);
    lmod.ums_ItemSubsysType  = SUBSYSPART(cntype);

    /* try from the primary path with an extension of .priv<typeName> */
    sprintf(path,"$%ss/%s.priv%s",typeName,itemName,typeName);
    result = UserModeService(&lmod,sizeof(lmod),TRUE);
    if (result == MAKEFERR(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile))
    {
        /* try from the primary path with an extension of .<typeName> */
        sprintf(path,"$%ss/%s.%s",typeName,itemName,typeName);
        result = UserModeService(&lmod,sizeof(lmod),FALSE);
        if (result == MAKEFERR(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile))
        {
            /* try from the secondary path with an extension of .<typeName> */
            sprintf(path,"$app/%ss/%s.%s",typeName,itemName,typeName);
            result = UserModeService(&lmod,sizeof(lmod),FALSE);
        }
    }

    if (result >= 0)
    {
        *context       = lmod.ums_DemandLoad;
        it             = (ItemNode *)LookupItem(result);
        it->n_Version  = lmod.ums_ItemVersion;
        it->n_Revision = lmod.ums_ItemRevision;

        if (lmod.ums_ItemPrivileged)
            it->n_ItemFlags |= ITEMNODE_PRIVILEGED;
    }
    else
    {
        *context = NULL;
    }

    return result;
}


/*****************************************************************************/


/* Unload a module from memory */
Err UnloadModule(ItemNode *it, void *context)
{
UMSUnloadModuleArgs umod;
Err                 result;

    result = 0;
    if (context)
    {
       /* if there are no more openers, and this was demand-loaded, then
        * remove it from memory
        */
        umod.ums_Hdr.ums_Service = UMS_UNLOADMODULE;
        umod.ums_Argc            = DEMANDLOAD_MAIN_DELETE;
        umod.ums_Argv            = NULL;
        umod.ums_DemandLoad      = context;
        result = UserModeService(&umod,sizeof(umod),(it->n_ItemFlags & ITEMNODE_PRIVILEGED) != 0);

#ifdef DEVELOPMENT
	if (result < 0)
	    printf("UnloadModule: unable to unload the module '%s', error %d!\n",it->n_Name,result);
#endif
    }

    return result;
}
