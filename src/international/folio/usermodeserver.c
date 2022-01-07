/* $Id: usermodeserver.c,v 1.10 1994/12/22 20:13:42 vertex Exp $ */

/* #define TRACING */

#include "types.h"
#include "kernel.h"
#include "msgport.h"
#include "super.h"
#include "mem.h"
#include "clib.h"
#include "international_folio.h"
#include "usermodeserver.h"
#include "debug.h"


/*****************************************************************************/


#define SERVER_STACK_SIZE    1600
#define BASE_SERVER_PRIORITY 199

static Item serverPort;
static char serverStack[SERVER_STACK_SIZE];
Item        serverThread;


/*****************************************************************************/


static void UserModeServerThread(int32 signal, Task *signalTask)
{
Item          msgItem;
Message      *msg;
Err           result;
USERMODEFUNC  func;
void         *data;

    TRACE(("USERMODESERVERTHREAD: entering with signal $08%x and signalTask $%08x\n",signal,signalTask));

    serverPort = CreateMsgPort(NULL,0,0);
    SendSignal(signalTask->t.n_Item,signal);
    if (serverPort >= 0)
    {
        TRACE(("USERMODESERVERTHREAD: entering infinite loop\n"));

        while (TRUE)
        {
            msgItem = WaitPort(serverPort,0);

            TRACE(("USERMODESERVERTHREAD: got a message\n"));

            msg = (Message *)LookupItem(msgItem);
            if (msg)
            {
                /* switch to the same priority as the client */
                SetItemPri(CURRENTTASK->t.n_Item,((Task *)LookupItem(msg->msg.n_Owner))->t.n_Priority);

                func = (USERMODEFUNC)msg->msg_DataPtr;
                data = (void *)msg->msg_DataSize;

                result = (*func)(data);
                SetItemPri(CURRENTTASK->t.n_Item,BASE_SERVER_PRIORITY);
            }
            else
            {
                result = -1;
            }

            TRACE(("USERMODESERVERTHREAD: replying message with result=%d\n",result));

            ReplySmallMsg(msgItem,result,0,0);
        }
    }

    TRACE(("USERMODESERVERTHREAD: unexpected termination\n"));
}


/*****************************************************************************/


static Item SuperCreateItemVA(int32 ctype, uint32 tag, ...)
{
    return SuperCreateSizedItem(ctype, (TagArg *)&tag, 0);
}


/*****************************************************************************/


Err CreateUserModeServer(void)
{
int32 signal;
Err   result;

    SUPERTRACE(("CREATEUSERMODESERVER: entering\n"));

    signal = SuperAllocSignal(0);
    if (signal > 0)
    {
        SUPERTRACE(("CREATEUSERMODESERVER: about to launch server thread\n"));

        result = SuperCreateItemVA(MKNODEID(KERNELNODE,TASKNODE),
                                   TAG_ITEM_PRI,                  BASE_SERVER_PRIORITY,
                                   TAG_ITEM_NAME,                 "International Folio Server",
                                   CREATETASK_TAG_PC,             UserModeServerThread,
                                   CREATETASK_TAG_STACKSIZE,      SERVER_STACK_SIZE,
                                   CREATETASK_TAG_SP,             (uint32)serverStack + SERVER_STACK_SIZE,
                                   CREATETASK_TAG_ARGP,           CURRENTTASK,
                                   CREATETASK_TAG_ARGC,           signal,
                                   CREATETASK_TAG_SUPER,          0,
                                   TAG_END);
        if (result >= 0)
        {
            serverThread = result;

            SUPERTRACE(("CREATEUSERMODESERVER: waiting for server thread sync signal\n"));

            SuperWaitSignal(signal);
            result = serverPort;
        }
        SuperFreeSignal(signal);
    }
    else
    {
        result = INTL_ERR_NOSIGNALS;
    }

    SUPERTRACE(("CREATEUSERMODESERVER: exiting with %d\n",result));

    return (result);
}


/*****************************************************************************/


void DeleteUserModeServer(void)
{
    SuperInternalDeleteItem(defaultLocaleItem);
    SuperInternalDeleteItem(serverThread);
}


/****************************************************************************/


Err CallUserModeFunc(USERMODEFUNC func, void *data)
{
Err  result;
Item msg;
Item port;

    if (isUser())
    {
        TRACE(("CALLUSERMODEFUNC: entering in user-mode\n"));

        port = CreateMsgPort(NULL,0,0);
        msg = CreateSmallMsg(NULL,0,port);

        result = SendSmallMsg(serverPort,msg,(uint32)func,(uint32)data);
        if (result == 0)
        {
            TRACE(("CALLUSERMODEFUNC: waiting for user server to return message\n"));

            WaitPort(port,0);

            TRACE(("CALLUSERMODEFUNC: server returned message\n"));

            result = ((Message *)LookupItem(msg))->msg_Result;
        }
        else
        {
            TRACE(("CALLUSERMODEFUNC: unable to initiate user mode function call, error $%lx\n",result));
        }

        DeleteMsg(msg);
        DeleteMsgPort(port);

        TRACE(("CALLUSERMODEFUNC: exiting with $%lx\n",result));
    }
    else
    {
        SUPERTRACE(("CALLUSERMODEFUNC: entering in supervisor-mode\n"));

        port = SuperCreateMsgPort(NULL,0,0);
        msg = SuperCreateSmallMsg(NULL,0,port);

        result = SuperSendSmallMsg(serverPort,msg,(uint32)func,(uint32)data);
        if (result == 0)
        {
            SUPERTRACE(("CALLUSERMODEFUNC: waiting for user server to return message\n"));

            SuperWaitPort(port,0);

            SUPERTRACE(("CALLUSERMODEFUNC: server returned message\n"));

            result = ((Message *)LookupItem(msg))->msg_Result;
        }
        else
        {
            SUPERTRACE(("CALLUSERMODEFUNC: unable to initiate user mode function call, error $%lx\n",result));
        }

        SuperDeleteMsg(msg);
        SuperDeleteMsgPort(port);

        SUPERTRACE(("CALLUSERMODEFUNC: exiting with $%lx\n",result));
    }

    return (result);
}
