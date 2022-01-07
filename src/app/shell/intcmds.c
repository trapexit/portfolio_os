/* $Id: intcmds.c,v 1.41 1994/09/15 17:22:01 vertex Exp $ */

#include "types.h"
#include "nodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "device.h"
#include "mem.h"
#include "string.h"
#include "io.h"
#include "stdlib.h"
#include "filefunctions.h"
#include "debug.h"
#include "stdio.h"
#include "shelldebug.h"
#include "intcmds.h"


/*****************************************************************************/


#ifdef DEVELOPMENT
#include "debug.h"
#define INFO(x) kprintf x
#define ERRORINFO(x) {kprintf("Shell: "); kprintf x;}
#else
#define INFO(x)
#define ERRORINFO(x)
#endif


/*****************************************************************************/


bool fromROM;
bool default_bg = TRUE;
bool verbose    = FALSE;

static bool minMem = FALSE;
static List stolenMem = INITLIST(stolenMem,NULL);


/* These constants are the maximum ram footprint of Portfolio
 * that we have guaranteed to our customers.
 */
#define DRAM_PORTFOLIO	(608<<10)
#define VRAM_PORTFOLIO	(16<<10)

/* These constants reflect the minimum amount of physical ram
 * in any standard implementation.
 */
#define DRAM_MINHW	(2<<20)
#define VRAM_MINHW	(1<<20)

/* These constants are calculated from the above, and reflect the
 * amount of memory that we are guaranteeing to the application.
 */
#define DRAM_FORAPP	(DRAM_MINHW-DRAM_PORTFOLIO)
#define VRAM_FORAPP	(VRAM_MINHW-VRAM_PORTFOLIO)


/*****************************************************************************/


uint32 ConvertNum(char *str)
{
    if (*str == '$')
    {
        str++;
        return strtoul(str,0,16);
    }

    return strtoul(str,0,0);
}


/*****************************************************************************/


#ifdef DEVELOPMENT
/* stub to keep junk being pulled in from stdlib.o */
FILE *stdout;
int32 putc(char ch, FILE *stream)
{
    kprintf("%c", ch);
    return (ch);
}
#endif


/*****************************************************************************/


static Task *SpecialFindTask(char *s)
{
Item  it;
int32 n;
Task *task;

    it   = FindNamedItem(MKNODEID(KERNELNODE,TASKNODE),s);
    task = (Task *)CheckItem(it,KERNELNODE,TASKNODE);

    if (!task)
    {
        n = ConvertNum(s);

        /* was input an Item? */
        task = (Task *)CheckItem((Item)n,KERNELNODE,TASKNODE);
    }

    return (task);
}


/*****************************************************************************/


#ifdef DEVELOPMENT
static void Help(char *args)
{
    INFO(("3DO Shell Built-In Commands:\n\n"));

    INFO(("alias <name> [str] : create a file-path alias\n"));
    INFO(("sleep <seconds>    : sleep for <seconds>\n"));
    INFO(("killkprintf        : stop kernel kprintf screen output\n"));
    INFO(("rom                : jump to rom emulation\n\n"));

    INFO(("setbg              : set shell's default to not wait for tasks (&)\n"));
    INFO(("setfg              : set shell's default to wait for tasks (#)\n"));
    INFO(("setpri             : set shell's priority\n"));
    INFO(("setcd              : change current directory\n"));
    INFO(("setverbose [on|off]: set shell's verbosity state\n"));
    INFO(("setminmem          : set free mem to minimum guaranteed\n"));
    INFO(("setmaxmem          : set free mem to maximum available\n\n"));

    INFO(("showshellvars      : display the values of shell variables\n"));
    INFO(("showavailmem       : display available memory in the system\n"));
    INFO(("showmemlist <list> : display the given memory list\n"));
    INFO(("showcd             : print current directory\n"));
    INFO(("showerror <error #>: print an error string from a system error code\n"));
    INFO(("showkernelbase     : print interesting bits of KernelBase\n"));
    INFO(("showmemmap [item|name] : display memory page owners\n"));
    INFO(("showtask [item|name]   : print task information\n\n"));

    INFO(("killtask <item|name>   : Kill a task\n\n"));
}
#endif


/*****************************************************************************/


#ifdef DEVELOPMENT
static void ShowError(char *args)
{
    PrintfSysErr(ConvertNum(args));
}
#endif


/*****************************************************************************/


#ifdef DEVELOPMENT
static void ShowKernelBase(char *args)
{
    INFO(("KernelBase         = $%08x\n",KernelBase));
    INFO(("  ->kb_TimerBits   = $%08x\n",KernelBase->kb_TimerBits));
    INFO(("  ->kb_CPUFlags    = $%08x\n",KernelBase->kb_CPUFlags));
    INFO(("  ->kb_numticks    = $%08x\n",KernelBase->kb_numticks));
    INFO(("  ->kb_DRAMSetSize = $%x\n",KernelBase->kb_DRAMSetSize));
    INFO(("  ->kb_VRAMSetSize = $%x\n",KernelBase->kb_VRAMSetSize));
    INFO(("  ->kb_MadamRev    = %d\n",KernelBase->kb_MadamRev));
    INFO(("  ->kb_ClioRev     = %d\n",KernelBase->kb_ClioRev));
}
#endif


/*****************************************************************************/


static void Sleep(char *args)
{
Item            timer;
Item            io;
IOReq          *ior;
IOInfo          ioInfo;
Err             ioerr;
struct timeval  tv;
int32           secs;

    secs       = ConvertNum(args);
    tv.tv_sec  = secs;
    tv.tv_usec = 0;

    timer = OpenNamedDevice("timer",0);
    if (timer >= 0)
    {
        io = CreateIOReq(0,0,timer,0);
        if (io >= 0)
        {
            ior = (IOReq *)LookupItem(io);

            /* initialize ioInfo */
            memset(&ioInfo,0,sizeof(ioInfo));
            ioInfo.ioi_Command         = TIMERCMD_DELAY;
            ioInfo.ioi_Unit            = TIMER_UNIT_USEC;
            ioInfo.ioi_Send.iob_Buffer = &tv;
            ioInfo.ioi_Send.iob_Len    = sizeof(tv);

            ioerr = DoIO(io,&ioInfo);

            if (ioerr < 0)
            {
                PrintError(NULL,"\\(Sleep) unable to perform DoIO() to","timer",ioerr);
            }
            else
            {
                ERRORINFO(("(Sleep) slept %d seconds\n",secs));
            }

            DeleteIOReq(io);
        }
        else
        {
            PrintError(NULL,"\\(Sleep) unable to create IO request for","timer",io);
        }
        CloseItem(timer);
    }
    else
    {
        PrintError(NULL,"\\(Sleep) unable to open device","timer",timer);
    }
}


/*****************************************************************************/


static void SetBGMode(char *args)
{
    /* set default to put task in background */
    default_bg = TRUE;
}


/*****************************************************************************/


static void SetFGMode(char *args)
{
    /* set default to wait for tasks */
    default_bg = FALSE;
}


/*****************************************************************************/


static void SetPri(char *args)
{
Task   *task = KernelBase->kb_CurrentTask;
uint32  newpri;

    if (args && args[0])
    {
        INFO(("Old priority: %d\n",task->t.n_Priority));

        newpri = ConvertNum(args);
        if ((newpri > 0) && (newpri < 250))
        {
            SetItemPri(task->t.n_Item,(uint8)newpri);
        }
        else
        {
            INFO(("Priority out of range 1..249\n"));
        }

        INFO(("New priority: %d\n",newpri));
    }
#ifdef DEVELOPMENT
    else
    {
        INFO(("Current priority: %d\n",task->t.n_Priority));
    }
#endif
}


/*****************************************************************************/


static void SetROM(char *args)
{
    fromROM = TRUE;
}


/*****************************************************************************/


#ifdef DEVELOPMENT
static void ShowShellVars(char *args)
{
    INFO(("Shell Priority  : %d\n",KernelBase->kb_CurrentTask->t.n_Priority));
    INFO(("Execution Mode  : %s\n",default_bg  ? "Background"   : "Foreground"));
    INFO(("Memory Amount   : %s\n",minMem      ? "Minimum"      : "Maximum"));
    INFO(("Shell Verbosity : %s\n",verbose     ? "ON"           : "OFF"));
}
#endif


/*****************************************************************************/


#ifdef DEVELOPMENT
static void SetVerbose(char *args)
{
    if (strcasecmp(args,"ON") == 0)
        verbose = TRUE;
    else if (strcasecmp(args,"OFF") == 0)
        verbose = FALSE;

    if (verbose)
        INFO(("Shell verbosity is on\n"));
    else
        INFO(("Shell verbosity is off\n"));
}
#endif


/*****************************************************************************/


#ifdef DEVELOPMENT
static void ShowCD(char *args)
{
int32 result;
char  path[256];

    result = GetDirectory(path, 256);
    if (result < 0)
    {
        PrintError(NULL,"\\(ShowCD) GetDirectory() failed",0,result);
    }
    else
    {
        INFO(("Current directory is '%s'\n", path));
    }
}
#endif


/*****************************************************************************/


static void SetCD(char *args)
{
int32 result;

    if (args[0] != 0)
    {
        result = ChangeDirectory(args);
        if (result < 0)
        {
            PrintError(NULL,"\\(SetCD) ChangeDirectory() to ",args,result);
        }
    }
#ifdef DEVELOPMENT
    else
    {
        ShowCD(NULL);
    }
#endif
}


/*****************************************************************************/


static void MakeAlias(char *args)
{
int32 result;
char *aliasname, *aliastext;

    aliasname = args;
    if (*aliasname == 0)
    {
        ERRORINFO(("(Alias) missing alias name\n"));
        return;
    }

    aliastext = aliasname+1;
    while (*aliastext)
    {
        if (*aliastext == ' ')
        {
            *aliastext++ = '\0';
            break;
        }
        aliastext++;
    }

    while (*aliastext)
    {
        if (*aliastext != ' ')
            break;

        aliastext++;
    }

    result = CreateAlias(aliasname, aliastext);
    if (result < 0)
    {
        PrintError(NULL,"\\(Alias) CreateAlias() failed for",aliastext,result);
    }
}


/*****************************************************************************/


static void KillKPrintf(char *args)
{
    kprintf("%c",4);
}


/*****************************************************************************/


static void KillTask(char *args)
{
Task *task;
Item  it;

    task = SpecialFindTask(args);
    if (!task)
    {
        ERRORINFO(("(KillTask) task not found\n"));
        return;
    }

    it = task->t.n_Item;
    DeleteItem(it);

    if (LookupItem(it) == 0)
    {
        ERRORINFO(("(KillTask) task %s now removed\n",args));
    }
    else
    {
        ERRORINFO(("(KillTask) task %s survived the assault!\n",args));
    }
}


/*****************************************************************************/


static void NOP(char *args)
{
    /* for obsolete commands... */
    ERRORINFO(("(NOP) this command is no longer supported\n"));
}

/*****************************************************************************/


#ifdef DEVELOPMENT
static void DumpTask(Task *task)
{
struct timeval *tv = &task->t_ElapsedTime;
    uint32	s = tv->tv_sec;
    uint32	u = tv->tv_usec;
    Task *p = task->t_ThreadTask;

    INFO((" $%05x", task->t.n_Item));
    INFO((" $%06x", task));
    INFO((" $%06x", p));
    INFO((" $%08x $%08x", task->t_WaitBits,task->t_SigBits));
    INFO((" %3d", task->t.n_Priority));
    INFO((" %3d:%02d", s/3600, (s/60)%60));
    INFO((":%02d.%03d", s%60, u/1000));
    INFO((" %c %c",
	  task->t.n_Flags & TASK_SUPER ? 'S' : 'U',
	  task->t.n_Flags & TASK_RUNNING ? 'X' :
	  task->t.n_Flags & TASK_READY ? 'R' :
	  task->t.n_Flags & TASK_WAITING ? 'W' : ' '));
    INFO((" %s\n",task->t.n_Name));
}
#endif


/*****************************************************************************/


#ifdef DEVELOPMENT

/* defines the maximum number of tasks/threads on a single task list
 * that DumpTaskList() will deal with. Warning: increasing this will
 * increase the amount of stack used by DumpTaskList()!
 */
#define MAX_TASK_LIST 128

static void DumpTaskList(List *l)
{
ItemNode *in;
Item      tasks[MAX_TASK_LIST];
uint32    taskCnt;
uint32    i;
Task     *task;

    taskCnt = 0;
    SCANLIST(l,in,ItemNode)
    {
        tasks[taskCnt++] = in->n_Item;
        if (taskCnt == MAX_TASK_LIST)
            break;
    }

    for (i = 0; i < taskCnt; i++)
    {
        task = (Task *)LookupItem(tasks[i]);
        if (task)
            DumpTask(task);
    }
}
#endif


/*****************************************************************************/


#ifdef DEVELOPMENT
static void DumpResources(Task *task)
{
int   i;
int   pi = 0;
Item *p = task->t_ResourceTable;

    for (i = 0; i < task->t_ResourceCnt; i++)
    {
        if (*p >= 0)
        {
            if (*p & ITEM_WAS_OPENED)
            {
                INFO(("($%05x) ",*p & ~ITEM_WAS_OPENED));
            }
            else
            {
                INFO((" $%05x  ",*p));
            }
            pi++;

            if (pi > 7)
            {
                pi = 0;
                INFO(("\n"));
            }
        }
        p++;
    }

    if (pi)
        INFO(("\n"));
}
#endif


/*****************************************************************************/


#ifdef DEVELOPMENT
static void DumpMemList(MemList *ml)
{
uint32 *p;
int32   unused;
int32   i;
int32   largest;
Node   *node;
int32   ones;

    p    = ml->meml_OwnBits;
    i    = ml->meml_OwnBitsSize;
    ones = 0;

    while (i--)
        ones += CountBits(*p++);

    INFO(("MemList: '%s', addr $%06x, num pages %d, ",ml->meml_n.n_Name,ml,ones));

    unused  = 0;
    largest = 0;

    SCANLIST(ml->meml_l,node,Node)
    {
        if (largest < node->n_Size)
            largest = node->n_Size;

        unused += node->n_Size;
    }
    INFO(("unused %d ($%x), ",unused,unused));
    INFO(("largest %d ($%x)\n",largest,largest));
}
#endif


/*****************************************************************************/


#ifdef DEVELOPMENT
static void ShowMemList(char *args)
{
MemList *ml;
Node    *node;

    ml = (MemList *)ConvertNum(args);
    if ((ml == 0)
     || ((uint32)ml & 3)
     || (ml->meml_n.n_SubsysType != NST_KERNEL)
     || (ml->meml_n.n_Type != MEMLISTNODE))
    {
        ERRORINFO(("(ShowMemList) invalid memlist address\n"));
	return;
    }

    DumpMemList(ml);

    SCANLIST(ml->meml_l,node,Node)
    {
        INFO(("  Free memory, range $%06x-$%06x, size %d\n",node,(uint32)node + (uint32)node->n_Size - 1,node->n_Size));
    }
}
#endif


/*****************************************************************************/


#ifdef DEVELOPMENT
static void PrintMemUsage(List *l)
{
MemList *m;

    SCANLIST(l,m,MemList)
    {
        DumpMemList(m);
    }
}
#endif


/*****************************************************************************/


#ifdef DEVELOPMENT
static char *aregn[] = {
    "a1", "a2", "a3", "a4",
    "v1", "v2", "v3", "v4", "v5",
    "sb", "sl", "fp", "ip",
    "sp", "lr", "pc"
};

static void ShowTask(char *args)
{
Task *task;

    if (args[0])
    {
	struct timeval *tv;
	uint32	s;
	uint32	u;
	Task *p;

        task = SpecialFindTask(args);
        if (!task)
        {
            ERRORINFO(("(ShowTask) task not found\n"));
            return;
        }

	tv = &task->t_ElapsedTime;
	s = tv->tv_sec;
	u = tv->tv_usec;
	p = task->t_ThreadTask;

	INFO(("Task $%06x: ", task));
	INFO(("pri=%3d ", task->t.n_Priority));
	INFO(("item=$%06x ", task->t.n_Item));
	INFO((" '%s'\n", task->t.n_Name));
	INFO(("\tFlags: %08x", task->t.n_Flags));
	if (task->t.n_Flags & TASK_READY) { INFO((" READY")); }
	if (task->t.n_Flags & TASK_WAITING) { INFO((" WAITING")); }
	if (task->t.n_Flags & TASK_RUNNING) { INFO((" RUNNING")); }
	if (task->t.n_Flags & TASK_SUPER) { INFO((" SUPER")); }
	INFO(("\n"));
	if (p) { INFO(("\tThread of task %06x '%s'\n", p, p->t.n_Name)); }
	INFO(("\tAccumulated time: %3d:%02d", s/3600, (s/60)%60));
	INFO((":%02d.%03d\n", s%60, u/1000));
	INFO(("\tTime Slice: %dms\n", task->t_MaxUSecs/1000));
	INFO(("\tSignals a=%08x w=%08x r=%08x\n",
	      task->t_AllocatedSigs,task->t_WaitBits,task->t_SigBits));

        INFO(("SuperStack Base $%06x, size %d, SSP $%06x\n",task->t_SuperStackBase,task->t_SuperStackSize,task->t_ssp));
        INFO(("UserStack  Base $%06x, size %d, ",task->t_StackBase,task->t_StackSize));

        if (task->t_psr & 0x7)     /* supermode? */
        {
            INFO(("USP $%06x, SUPERpc $%06x",task->t_Usersp,task->t_pc));
	    if (task->t_SuperStackBase)
	        INFO((" USERpc $%06x",*(int32*)((char*)task->t_SuperStackBase + task->t_SuperStackSize - 8)));
        }
        else
        {
            INFO(("USP $%06x, USERpc $%06x",task->t_sp,task->t_pc));
        }

        INFO(("\n\nTask Memory Lists:\n"));
        PrintMemUsage(task->t_FreeMemoryLists);

        INFO(("\nTask Item Table:\n"));
        DumpResources(task);

	INFO(("\nARM Registers:"));
	{
	    int i;
	    for (i=0; i<16; ++i)
		INFO(("%s%s=$%08x",
		      (i&3)?" ":"\n",
		      aregn[i],
		      task->t_regs[i]));
	}
	INFO(("\n"));
    }
    else
    {
        INFO(("  Item    Task   Parent  WaitSigs  RecvSigs Pri      Time     T S Name\n"));
        DumpTaskList(KernelBase->kb_TaskWaitQ);
        DumpTaskList(KernelBase->kb_TaskReadyQ);
        DumpTask(KernelBase->kb_CurrentTask);
    }
}
#endif


/*****************************************************************************/


static void StealMem(uint32 types)
{
int32  pagesize;
Node  *n;

    pagesize = GetPageSize(types|MEMTYPE_SYSTEMPAGESIZE);
    while ((n = (Node *)AllocMem(pagesize,types|MEMTYPE_STARTPAGE)) != 0)
    {
        n->n_Size = pagesize;
        AddTail(&stolenMem,n);
    }
}


/*****************************************************************************/


static void SetMinMem(char *args)
{
void *mem;

    /* ScavengeMem() has been called by shell.c before we got here... */

    mem = AllocMem(VRAM_FORAPP,MEMTYPE_VRAM|MEMTYPE_STARTPAGE);
    if (mem)
    {
        StealMem(MEMTYPE_VRAM);
        FreeMem(mem,VRAM_FORAPP);
        INFO(("(SetMinMem): %d bytes (%dK) VRAM available starting at $%06x\n",VRAM_FORAPP,VRAM_FORAPP/1024,mem));
    }
    else
    {
        ERRORINFO(("(SetMinMem) unable to obtain %d bytes (%dK) of contiguous VRAM\n",VRAM_FORAPP,VRAM_FORAPP/1024));
    }

    mem = AllocMem(DRAM_FORAPP,MEMTYPE_DRAM|MEMTYPE_STARTPAGE);
    if (mem)
    {
        StealMem(MEMTYPE_DRAM);
        FreeMem(mem,DRAM_FORAPP);
        INFO(("(SetMinMem): %d bytes (%dK) DRAM available starting at $%06x\n",DRAM_FORAPP,DRAM_FORAPP/1024,mem));
    }
    else
    {
        ERRORINFO(("(SetMinMem) unable to obtain %d bytes (%dK) of contiguous DRAM\n",DRAM_FORAPP,DRAM_FORAPP/1024));
    }

    ScavengeMem();

    minMem = TRUE;
}


/*****************************************************************************/


static void SetMaxMem(char *args)
{
Node *n;

    while (TRUE)
    {
        n = RemHead(&stolenMem);
        if (!n)
            break;

        FreeMem(n,n->n_Size);
    }

    ScavengeMem();

    minMem = FALSE;
}


/*****************************************************************************/


#ifdef DEVELOPMENT
static void ShowAvailMem(char *args)
{
List   *l = KernelBase->kb_MemHdrList;
MemHdr *m;

    for (m=(MemHdr *)FIRSTNODE(l); ISNODE(l,m); m = (MemHdr *)NEXTNODE(m))
    {
        ulong *p = m->memh_FreePageBits;
        int ones = 0;
        int32 size;
        int i =  m->memh_FreePageBitsSize;
        while (i--) ones += CountBits(*p++);
        size = ones*m->memh_PageSize;
        INFO(("MemHdr: '%s', addr $%06x, type $%08x, ",m->memh_n.n_Name,m,m->memh_Types));
        INFO(("page size %d, available %d ($%x)\n",m->memh_PageSize,size,size));
    }

    l = KernelBase->kb_MemFreeLists;

    INFO(("\nSupervisor Memory Lists:\n"));

    PrintMemUsage(l);
}
#endif


/*****************************************************************************/


#ifdef DEVELOPMENT

#define MAX_TASKS 26

static void ShowMemMap(char *args)
{
uint32   i;
uint32   j;
uint32   k;
bool     found;
MemList *systemML;
MemList *ml;
Task    *task;
MemHdr  *mh;
uint32   pageNum;
Item     tasks[MAX_TASKS];
uint32   taskCnt;
uint32   lineLen;
Item     specialItem;
void    *ptr;

    specialItem = -1;
    if (args[0])
    {
        task = SpecialFindTask(args);
        if (!task)
        {
            ERRORINFO(("(ShowMemMap): unable to find task '%s'\n",args));
            return;
        }
        specialItem = task->t.n_Item;
    }

    taskCnt = 0;
    SCANLIST(KernelBase->kb_Tasks,ptr,void)
    {
        task = (Task *)((uint32)ptr - offsetof(Task,t_TasksLinkNode));
        if (!task->t_ThreadTask)
        {
            if (taskCnt >= MAX_TASKS)
                break;

            tasks[taskCnt++] = task->t.n_Item;
        }
    }

    INFO(("Legend: .=Free, S=Supervisor, K=Kernel"));

    lineLen = 38;
    for (k = 0; k < taskCnt; k++)
    {
        task = (Task *)LookupItem(tasks[k]);
        if (task)
        {
            if (lineLen > 70)
            {
                INFO((",\n        "));
                lineLen = 8;
            }
            else
            {
                INFO((", "));
                lineLen += 2;
            }

            if (tasks[k] == specialItem)
                INFO(("*=%s",task->t.n_Name));
            else
                INFO(("%c=%s",k + 'a',task->t.n_Name));

            lineLen += 2 + strlen(task->t.n_Name);
        }
    }

    INFO(("\n"));

    SCANLIST(KernelBase->kb_MemHdrList,mh,MemHdr)
    {
	INFO(("\n"));

	INFO(("MemHdr: '%s', addr $%06x, page size %d, ",mh->memh_n.n_Name,mh,mh->memh_PageSize));
	INFO(("range $%06x-$%06x\n",mh->memh_MemBase,mh->memh_MemTop));

	/* Find the system's MemList for this MemHdr */
	SCANLIST(KernelBase->kb_MemFreeLists,systemML,MemList)
	{
	    if (systemML->meml_MemHdr == mh)
         	break;
	}

	for (i = 0; i < mh->memh_FreePageBitsSize; i++)
	{
	    for (j = 0; j < 32; j++)
	    {
		pageNum = i*32 + j;

		if (PD_IsSet(pageNum,(pd_set *)mh->memh_FreePageBits))
                {
                    /* memory page is free */

		    INFO(("."));
		}
		else if (PD_IsSet(pageNum,(pd_set *)systemML->meml_OwnBits))
		{
		    /* memory page belongs to the system */

		    INFO(("S"));
		}
		else
		{
		    /* memory page belongs to a task, find which one... */

		    found = FALSE;
                    for (k = 0; k < taskCnt; k++)
		    {
                        task = (Task *)CheckItem(tasks[k],KERNELNODE,TASKNODE);
                        if (task)
                        {
                            SCANLIST(task->t_FreeMemoryLists,ml,MemList)
                            {
                                if (ml->meml_MemHdr == mh)
                                {
                                    if (PD_IsSet(pageNum,(pd_set *)ml->meml_OwnBits))
                                    {
                                        if (tasks[k] == specialItem)
                                            INFO(("*"));
                                        else
                                            INFO(("%c",k + 'a'));
                                        found = TRUE;
                                    }
                                    break;
                                }
                            }
                        }
		    }

		    if (!found)
		        INFO(("K"));
		}
	    }
	    INFO(("\n"));
	}
    }
}
#endif


/*****************************************************************************/


static char *CommandNames[] =
{
    "alias",
    "cd,setcd",
    "bg,setbg",
    "fg,setfg",
    "shellpri,setpri",
    "rom",
    "pwd,showcd",
    "avail,showavailmem",
    "memmap,showmemmap",
    "memlist,showmemlist",
    "kbprint,showkernelbase",
    "ps,showtask",
    "kill,killtask",
    "killkprintf",
    "sleep",
    "minmem,setminmem",
    "maxmem,setmaxmem",
    "verbose,setverbose",
    "help,?",
    "showshellvars",
    "err,showerror",
    "spawnpri,input,stack,dram,vram",
    NULL
};

static void (*CommandDispatch[])(char *) =
#ifdef DEVELOPMENT
{
    MakeAlias,
    SetCD,
    SetBGMode,
    SetFGMode,
    SetPri,
    SetROM,
    ShowCD,
    ShowAvailMem,
    ShowMemMap,
    ShowMemList,
    ShowKernelBase,
    ShowTask,
    KillTask,
    KillKPrintf,
    Sleep,
    SetMinMem,
    SetMaxMem,
    SetVerbose,
    Help,
    ShowShellVars,
    ShowError,
    NOP
};
#else
{
    MakeAlias,
    SetCD,
    SetBGMode,
    SetFGMode,
    SetPri,
    SetROM,
    NOP,
    NOP,
    NOP,
    NOP,
    NOP,
    NOP,
    KillTask,
    KillKPrintf,
    Sleep,
    SetMinMem,
    SetMaxMem,
    NOP,
    NOP,
    NOP,
    NOP,
    NOP
};
#endif


/*****************************************************************************/


bool ExecBuiltIn(char *command, char *cmdLine)
{
uint32  index;
char    local[32];
uint32  i,j;
char   *names;

    TRACE(("EBI: command '%s', cmdLine '%s'\n",command,cmdLine));

    index = 0;
    while (TRUE)
    {
        names = CommandNames[index];
        if (!names)
            break;

        i = 0;
        while (names[i])
        {
            j = 0;
            while (names[i] && (names[i] != ','))
                local[j++] = names[i++];

            if (names[i])
                i++;

            local[j] = 0;

            if (strcasecmp(local,command) == 0)
            {
                ScavengeMem();
                (*CommandDispatch[index])(cmdLine);
                return (TRUE);
            }
        }

        index++;
    }

    return (FALSE);
}
