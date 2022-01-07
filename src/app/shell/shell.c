/* $Id: shell.c,v 1.152 1994/10/04 00:46:24 vertex Exp $ */

/* #define TRACING */

#include "types.h"
#include "nodes.h"
#include "list.h"
#include "task.h"
#include "device.h"
#include "mem.h"
#include "string.h"
#include "io.h"
#include "aif.h"
#include "operror.h"
#include "cdrom.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "filesystem.h"
#include "filefunctions.h"
#include "stdio.h"
#include "graphics.h"
#include "shelldebug.h"
#include "errors.h"
#include "intcmds.h"
#include "debug.h"
#include "launchapp.h"
#include "shell.h"


/*****************************************************************************/


typedef struct ThreadNode
{
    struct ThreadNode *tn_Next;
    uint32             tn_NodeSize;
    CodeHandle         tn_Code;
    Item               tn_ThreadItem;
    uint32             tn_ArgvSize;
    char             **tn_Argv;
    char               tn_ArgBuffer[1];  /* actually variable-sized */
} ThreadNode;

typedef void (* ServiceProvider)(Item msgPort);


/*****************************************************************************/


#define FILENOTFOUND MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_NoFile)

#ifdef DEVELOPMENT
static Item macItem;
#endif

static char            prompt[] = "-> ";
static uint32          waitMask;
#define	MAX_INPUT_LINE	127
static char            inputBuffer[MAX_INPUT_LINE+1];
static char	       lastCommand[MAX_INPUT_LINE+1];
static ThreadNode     *threadList;
static Item            accessPortItem;
static Item            serverPortItem;
static bool            cancelSleep;
static ServiceProvider servicer;


/*****************************************************************************/


static void WipeThreads(void)
{
ThreadNode **previous;
ThreadNode  *threadNode;

    TRACE(("WT: entering\n"));

    /* free resources used up by any dead threads */

    previous = &threadList;
    while (TRUE)
    {
        threadNode = *previous;
        if (!threadNode)
            break;

        if (CheckItem(threadNode->tn_ThreadItem,KERNELNODE,TASKNODE) == NULL)
        {
            VERBOSE(("Found a dead thread (item = $%lx), cleaning up...\n",threadNode->tn_ThreadItem));

            /* remove the node from the list */
            *previous = threadNode->tn_Next;

            UnloadCode(threadNode->tn_Code);

            if (threadNode->tn_Argv)
                FreeMem(threadNode->tn_Argv,threadNode->tn_ArgvSize);

            FreeMem(threadNode,threadNode->tn_NodeSize);
        }
        else
        {
            previous = &((*previous)->tn_Next);
        }
    }

    TRACE(("WT: exiting\n"));
}


/*****************************************************************************/


static void ProcessAsyncEvents(void)
{
Item       msgItem;
CodeHandle code;
Err        result;

    TRACE(("PAE: checking for Access requests\n"));

    code = NULL;
    result = 0;

    while (TRUE)
    {
        msgItem = GetMsg(accessPortItem);
        if (msgItem <= 0)
            break;

        if (!code)
            result = LoadCode("$tuners/access.tuner",&code);

        if (code)
            ExecuteAsSubroutine(code,msgItem,NULL);
        else
            ReplyMsg(msgItem,result,NULL,0);
    }

    UnloadCode(code);

    if (servicer)
        (*servicer)(serverPortItem);

    WipeThreads();

    TRACE(("PAE: exiting\n"));
}


/*****************************************************************************/


static void WaitForChild(Item childItem)
{
Task *task;

    TRACE(("WFC: waiting for child\n"));

    while (TRUE)
    {
        /* is the child still around? */
        task = (Task *)CheckItem(childItem,KERNELNODE,TASKNODE);
        if (!task)
        {
            TRACE(("WFC: task/thread has died\n"));
            break;
        }

        /* did we loose ownership of the task? */
        if (task->t.n_Owner != KernelBase->kb_CurrentTask->t.n_Item)
        {
            TRACE(("WFC: lost ownership of task/thread\n"));
            break;
        }

        WaitSignal(waitMask);

        ProcessAsyncEvents();
    }

    ProcessAsyncEvents();

    TRACE(("WFC: exiting, child is done for\n"));
}


/*****************************************************************************/


static Item LaunchThread(char *cmdName, char *cmdArgs)
{
int32       argc;
char       *ptr;
uint32      nodeSize;
ThreadNode *threadNode;
Item        subTask;
int32       i;
char       *threadName;

    TRACE(("LT: launching '%s' as a thread\n",cmdName));

    argc = 1;
    ptr  = cmdArgs;
    while (*ptr)
    {
        argc++;
        while (*ptr && (*ptr != ' '))
            ptr++;

        while (*ptr == ' ')
            ptr++;
    }

    ptr = cmdName;
    threadName = ptr;
    while (*ptr)
    {
        if (*ptr == '/')
            threadName = &ptr[1];

        ptr++;
    }

    TRACE(("LT: thread's argc = %ld\n",argc));

    nodeSize = sizeof(ThreadNode) + strlen(cmdName) + strlen(cmdArgs) + 1;

    threadNode = (ThreadNode *)AllocMem(nodeSize,MEMTYPE_ANY);
    if (threadNode)
    {
        threadNode->tn_NodeSize = nodeSize;
        threadNode->tn_ArgvSize = (argc+1) * sizeof(char *);
        threadNode->tn_Argv     = NULL;
        threadNode->tn_Next     = threadList;
        threadList              = threadNode;

        subTask = LoadCode(cmdName,&threadNode->tn_Code);
        if (subTask >= 0)
        {
            ptr = threadNode->tn_ArgBuffer;
            sprintf(ptr,"%s %s",cmdName,cmdArgs);

            threadNode->tn_Argv = (char **)AllocMem(threadNode->tn_ArgvSize,MEMTYPE_ANY | MEMTYPE_FILL);
            if (threadNode->tn_Argv)
            {
                i = 0;

                while (*ptr)
                {
                    threadNode->tn_Argv[i++] = ptr;
                    while (*ptr && (*ptr != ' '))
                        ptr++;

                    if (!*ptr)
                        break;

                    *ptr++ = 0;

                    while (*ptr == ' ')
                        ptr++;
                }

                subTask = ExecuteAsThread(threadNode->tn_Code,argc,threadNode->tn_Argv,threadName,-1);
            }
        }
        threadNode->tn_ThreadItem = subTask;
    }
    else
    {
        PrintError(NULL,"allocate memory to launch",cmdName,NOMEM);
        subTask = NOMEM;
    }

    TRACE(("LT: exiting with $%lx\n",subTask));

    return (subTask);
}


/*****************************************************************************/


static void ExecuteInputBuffer(void)
{
char   *cp;
bool    background;
bool    thread;
Item    subTask;
bool    script;
bool    found;
int32   fileSize;
char   *current;
char   *end;
Stream *stream;
uint32  i;
int32   ret;
char   *scriptMemory;
char    ch;
int32   pokeSpace;
char   *cmdLine;
char   *cmdArgs;

    VERBOSE(("%s\n",inputBuffer));

    background = default_bg;
    thread     = FALSE;
    script     = FALSE;
    found      = FALSE;

    /* skip leading spaces and tabs on the command-line */
    i = 0;
    while ((inputBuffer[i] == ' ') || (inputBuffer[i] == '\t'))
        i++;

    cmdLine = &inputBuffer[i];

    /* remove comments... # also forces foreground execution */
    cp = strchr(cmdLine,'#');
    if (cp)
    {
        *cp = 0;
        background = FALSE;
    }

    /* should we force background launching? */
    cp = strchr(cmdLine,'&');
    if (cp)
    {
        *cp = 0;
        background = TRUE;
    }

    /* should we launch this thing as a thread? */
    cp = strchr(cmdLine,'@');
    if (cp)
    {
        *cp = 0;
        thread = TRUE;
    }

    /* should we not bother to try to run this as an AIF? */
    cp = strchr(cmdLine,'%');
    if (cp)
    {
        *cp = 0;
        script = TRUE;
    }

    i = 0;
    while (cmdLine[i] && (cmdLine[i] != ' '))
        i++;

    if (i == 0)
    {
        /* empty command-line, just return */
        return;
    }

    pokeSpace = -1;
    if (cmdLine[i] == ' ')
    {
        cmdLine[i] = 0;
        pokeSpace = i;
        do
        {
            i++;
        }
        while (cmdLine[i] == ' ');
    }

    cmdArgs = &cmdLine[i];

    TRACE(("EIB: cmdLine = '%s', cmdArgs = '%s'\n",cmdLine,cmdArgs));

    ScavengeMem();

    if (cancelSleep)
    {
        /* we want to ignore any "sleep" commands that come as the first
         * thing in AppStartup
         */

        cancelSleep = FALSE;

        if (strcasecmp(cmdLine,"sleep") == 0)
        {
            VERBOSE(("Skipped useless sleep command in AppStartup\n"));
            return;
        }
    }

    /* is it a built-in command? */
    if (ExecBuiltIn(cmdLine,cmdArgs))
        return;

    if (!script)
    {
        if (thread)
        {
            TRACE(("EIB: before LaunchThread(\"%s\")\n",cmdLine));

            subTask = LaunchThread(cmdLine,cmdArgs);

            TRACE(("EIB: after LaunchThread(), return was $%lx\n",subTask));
        }
        else
        {
            if (pokeSpace >= 0)
                cmdLine[pokeSpace] = ' ';

            TRACE(("EIB: before LoadProgram(\"%s\")\n",cmdLine));

            subTask = LoadProgram(cmdLine);

            TRACE(("EIB: after LoadProgram(), return was $%x\n",subTask));

            if (pokeSpace >= 0)
                cmdLine[pokeSpace] = 0;
        }

        WipeThreads();

        if (subTask >= 0)
        {
            VERBOSE(("Launched task/thread for '%s', item $%lx, addr $%lx\n",cmdLine,subTask,LookupItem(subTask)));

            found = TRUE;
            if (!background)  /* wait until child is done before proceeding */
                WaitForChild(subTask);
        }
        else
        {
            if (subTask != FILENOTFOUND)  /* don't consider this a fatal error... */
            {
                if (subTask == BADAIF)
                {
                    script = TRUE;
                    found  = TRUE;
                }
                else
                {
                    PrintError(NULL,"spawn",cmdLine,subTask);
                    found = TRUE;
                }
            }
        }
    }

    if (script)
    {
        TRACE(("EIB: executing '%s' as a script\n",cmdLine));

        /* if we're running AppStartup, we want to cancel any sleep command
         * that we encounter before any other command is executed.
         */

        cancelSleep = (strncasecmp(cmdLine,"$boot/AppStartup",16) == 0);

        stream = OpenDiskStream(cmdLine,0);
        if (stream)
        {
            found = TRUE;

            fileSize = stream->st_FileLength;

            /* we put the allocation of the script in VRAM in order to
             * avoid fragging the shell's memory space...
             */
            scriptMemory = (char *)AllocMem(fileSize, MEMTYPE_FROMTOP | MEMTYPE_VRAM);
            if (scriptMemory)
            {
                ret = ReadDiskStream(stream,scriptMemory,fileSize);
                CloseDiskStream(stream);
                stream = NULL;

                if (ret == fileSize)
                {
                    current = scriptMemory;
                    end     = &scriptMemory[fileSize];
                    while (current < end)
                    {
                        /* The global "inputBuffer" is used here instead
                         * of a local stack based variable, which saves
                         * on stack space. Nice...
                         *
                         * But wait, this function is recursive, so
                         * inputBuffer might end up being used multiple
                         * times simulatenously!
                         *
                         * But wait, we carefully do not touch the contents
                         * of inputBuffer after we recursively call
                         * ExecuteInputBuffer(). This lets the global
                         * be safely used in this recursive manner.
                         *
                         * Ickky, but functional.
                         */

                        i = 0;
                        while ((current < end) && (i < sizeof(inputBuffer)-1))
                        {
                            ch = *current++;
                            if ((ch == '\n') || (ch == '\r'))
                                break;

                            inputBuffer[i++] = ch;
                        }
                        inputBuffer[i] = 0;

                        ExecuteInputBuffer();
                    }
                }
                else
                {
                    PrintError(NULL,"ReadDiskStream() script",cmdLine,0);
                }
                FreeMem(scriptMemory,fileSize);
            }
            else
            {
                PrintError(NULL,"allocate memory to hold script",cmdLine,NOMEM);
            }

            if (stream)
                CloseDiskStream(stream);

            TRACE(("EIB: executing program '%s'\n",cmdLine));
        }
    }

    if (!found)
        PrintError(NULL,"find",cmdLine,0);
}


/*****************************************************************************/


void ExecuteString(const char *name)
{
    TRACE(("ES: entering, name = '%s'",name));

    strcpy(inputBuffer,name);
    ExecuteInputBuffer();

    TRACE(("ES: exiting\n"));
}


/*****************************************************************************/


#define	CTL(x)	((x)&0x1F)
#define	KEY_DELC	CTL('H')
#define	KEY_DELL	CTL('X')
#define	KEY_REDRAW	CTL('R')
#define	KEY_NEWL	CTL('M')
#define	KEY_REPL	CTL('G')


int main(int argc, char **argv)
{
Item	VBLior = 0;
int32   i;
bool    serialPort;
#ifdef DEVELOPMENT
Item    macIOReqItem = 0;
IOReq  *macior = 0;
IOInfo  ioInfo;
#endif

#ifdef DEVELOPMENT
    print_vinfo();
#endif

    if (argc == 2)
    {
        /* On startup, the shell gets passed the address of a function
         * to call whenever messages arrive at the service port it
         * creates. This function lives in the operator, and pass the
         * function pointer around to save space. We can then have the
         * code in memory only once. If there is no parameter supplied,
         * then the shell doesn't create its service message port.
         */
        servicer = (ServiceProvider)ConvertNum(argv[1]);
    }

    /* try to CD to the boot volume ... don't wait forever, though */
    for (i = 0; i < 30000; i++)
        if (ChangeDirectory("$boot") >= 0)
            break;

    if (KernelBase->kb_CPUFlags & KB_NODBGR)
        fromROM = TRUE;

    if (KernelBase->kb_CPUFlags & KB_SERIALPORT)
    {
        serialPort = TRUE;
        fromROM    = FALSE;
    }
    else
    {
        serialPort = FALSE;
#ifdef PRODUCTION
        /* If we're in production mode, and there isn't a serial port
         * attached, but there is a debugger, we wish to force ROM-mode.
         * This is to help the layout optimization process.
         */
        fromROM    = TRUE;
#endif
    }

#ifdef DEVELOPMENT
    TRACE(("MAIN: calling OpenItem() for Mac\n"));

    macItem = OpenNamedDevice("mac",0);
    if (macItem > 0)
    {
        macIOReqItem = CreateIOReq(0,0,macItem,0);
        if (macIOReqItem < 0)
        {
            PrintError(NULL,"CreateIOReq()","mac",macIOReqItem);
            return (0);
        }

        macior = (IOReq *)LookupItem(macIOReqItem);
    }
    else
    {
        PrintError(NULL,"OpenNamedDevice()","mac",macItem);
    }

    /* demand-load the debugger folio, ignore failures */
    FindAndOpenFolio("debugger");

#endif

#ifdef DEVELOPMENT
    strcpy(inputBuffer,"/remote/system/scripts/startopera%");
#else
    strcpy(inputBuffer,"^/system/scripts/startopera%");
#endif

#ifdef DEVELOPMENT
    AttachErrors();
#endif

    accessPortItem = CreateMsgPort("access",0,0);

    if (servicer)
        serverPortItem = CreateMsgPort("NonPrivServer",200,0);

    if ((accessPortItem < 0) || (serverPortItem < 0))
    {
        PrintError(NULL,"CreateMsgPort()","Access or UserServer port",serverPortItem);
        return (0);
    }

    waitMask = (SIGF_IODONE | SIGF_DEADTASK |
                ((MsgPort *)LookupItem(serverPortItem))->mp_Signal |
                ((MsgPort *)LookupItem(accessPortItem))->mp_Signal);

    TRACE(("MAIN: automatically executing '%s'\n",inputBuffer));

    if (serialPort)
	VBLior = GetVBLIOReq();

    while (TRUE)
    {
        ExecuteInputBuffer();
        inputBuffer[0] = 0;

        if (fromROM)
        {
            /* from this point on, our default must be foreground mode */
            default_bg = FALSE;

            LaunchApp();
            /* NEVER RETURNS */
        }

        TRACE(("MAIN: waiting for user input from the Mac, or for Access reqs\n"));

	if (serialPort)
	{
            int   ch;
            int32 charCnt;
	    int   deleting;

            charCnt        = 0;
            inputBuffer[0] = 0;
            kprintf(prompt);
	    deleting = 0;
	    while (1)		/* yucchh a serial polling loop! */
            {
                ch = MayGetChar();
		if (ch < 0) {
		    /* yucch another poll! */
		    if (GetCurrentSignals() & waitMask)
                    {
			WaitSignal(waitMask);
			ProcessAsyncEvents();
		    } else
			WaitVBL(VBLior, 1);
		    continue;
		}
		if (ch == KEY_DELC) {
		    if (charCnt) {
			if (!deleting) {
			    kprintf("/");
			    deleting = 1;
			}
			charCnt--;
			kprintf("%c",inputBuffer[charCnt]);
			if (!charCnt) {
			    kprintf("/");
			    deleting = 0;
			}
                    }
		} else {
		    if (deleting) {
			kprintf("/");
			deleting = 0;
		    }
		    if (ch == KEY_DELL) {
			kprintf("\n%s", prompt);
			charCnt = 0;
		    } else if (ch == KEY_REDRAW) {
			kprintf("\n%s%s", prompt,inputBuffer);
			continue;
		    } else if (ch == KEY_NEWL) {
			strcpy(lastCommand, inputBuffer);
			break;
		    } else if (ch == KEY_REPL) {
			strcpy(inputBuffer, lastCommand);
			kprintf("\n%s%s", prompt, inputBuffer);
			break;
		    } else if (charCnt < MAX_INPUT_LINE) {
			kprintf("%c",ch);
                        inputBuffer[charCnt] = ch;
                        charCnt++;
		    } else {
                        break;
		    }
		}
		inputBuffer[charCnt] = 0;
	    }
	    kprintf("\n");
	}
#ifdef DEVELOPMENT
	else
	{
            memset(&ioInfo,0,sizeof(ioInfo));
            ioInfo.ioi_Command         = MACCMD_ASK;
            ioInfo.ioi_Send.iob_Buffer = prompt;
            ioInfo.ioi_Recv.iob_Buffer = inputBuffer;
            ioInfo.ioi_Recv.iob_Len    = sizeof(inputBuffer);
            ioInfo.ioi_Send.iob_Len    = strlen(prompt)+1;

            SendIO(macIOReqItem,&ioInfo);

            while (TRUE)
            {
                /* wait for something to happen */
                WaitSignal(waitMask);

                if (CheckIO(macIOReqItem))
                {
                    kprintf("\n");

                    WaitIO(macIOReqItem);
                    if (macior->io_Error == 0)
                    {
                        /* make sure it is zero terminated */
                        inputBuffer[macior->io_Actual] = 0;
                        TRACE(("MAIN: Mac user input = '%s'\n",inputBuffer));
                    }
                    else
                    {
                        PrintError(NULL,"read from","Mac",macior->io_Error);
                        inputBuffer[0] = 0;
                    }
                    break;
                }

                ProcessAsyncEvents();
            }
	}
#endif

        ProcessAsyncEvents();
    }
}
