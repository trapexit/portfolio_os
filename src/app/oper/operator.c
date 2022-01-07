/* $Id: operator.c,v 1.138.1.4 1995/01/17 19:41:38 jhw Exp $ */

#include "types.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "mem.h"
#include "task.h"
#include "kernel.h"
#include "strings.h"
#include "msgport.h"
#include "aif.h"
#include "operror.h"
#include "stdio.h"
#include "filesystem.h"
#include "filefunctions.h"
#include "super.h"
#include "debug.h"
#include "sherryvers.h"
#include "sysinfo.h"


/*****************************************************************************/


#define NEVERNEVERLAND WaitSignal(0)

#define ERR(x)	  kprintf x
#define DBUG(x)	  /* kprintf x */


/*****************************************************************************/


/* Store here the Sherry AIFHeader address passed thru <argc,argv> */
AIFHeader *SherryHeader = NULL;

extern AIFHeader __my_AIFHeader;
Item             serverPort;
int32            serverSignal;
Item             mathFolio;
Item             graphicsFolio;
Item             audioFolio;


/*****************************************************************************/


Item createKMacDriver(void);
Item createRamDriver(void);
Item createMacDriver(void);
Item createTimerDriver(void);
Item createMacCDRomDriver(void);
Item createExpansionDriver(void);
Item createSPORTDriver(void);
Item createEEPROMDriver(void);
Item createLCCDDriver(void);
Item createCDRomDriver(void);
Item createControlPortDriver(void);
void ProvideServices(Item serverPort); /* from kernel.lib */


/*****************************************************************************/


static void IdleThread(void)
{
volatile int32 foo;

    foo = 1;
    while (1)
    {
        foo <<= 1;    /* This seems to mess my stereo up less */
    }
}


/*****************************************************************************/


#define IDLE_STACK_SIZE 128

static Item createIdleThread(void)
{
void *stack;
Item  thread;

    stack = AllocMem(IDLE_STACK_SIZE,MEMTYPE_ANY);
    if (stack)
    {
        thread = CreateItemVA(MKNODEID(KERNELNODE,TASKNODE),
                              TAG_ITEM_PRI,             1,
                              TAG_ITEM_NAME,            "IdleTask",
                              CREATETASK_TAG_PC,        IdleThread,
                              CREATETASK_TAG_STACKSIZE, IDLE_STACK_SIZE,
                              CREATETASK_TAG_SP,        (uint32)stack + IDLE_STACK_SIZE,
                              CREATETASK_TAG_USERONLY,  0,
                              TAG_END);
    }
    else
    {
        thread = NOMEM;
    }

    if (thread < 0)
    {
	ERR(("oper: no idle task, err %d\n",thread));
	NEVERNEVERLAND;
    }

    DBUG(("IdleTask item $%x\n",thread));

    return thread;
}


/*****************************************************************************/


static Item createFileSystem(void)
{
AIFHeader *aif;
Item       fsTask;
void      *stack;
uint8      oldPri;
uint32     imageSize;
uint32     roundedSize;
uint32     numPages;
uint32     type;
uint32     pageSize;
uint32     alignMask;
uint32     stackSize;
_3DOBinHeader *binHdr;

    /*
     * The filesystem is loaded above the operator,
     * so look "after" the operator first. If that
     * fails, then search from the beginning.
     * see documentation in kernel/mem.c
     * for details.
     */

    aif = FindImage(&__my_AIFHeader, 0, "FileSystem");
    if (aif == NULL)
	aif = FindImage(0, 0, "FileSystem");

    if (aif == NULL)
    {
        ERR(("oper: no FS image\n"));
        NEVERNEVERLAND;
    }

    if ((aif->aif_blDecompress != 0xe1a00000) &&
        ((aif->aif_blDecompress & 0xff000000) != 0xeb000000))
    {
	ERR(("oper: no FS at $%x\n",aif));
	NEVERNEVERLAND;
    }

    /* Free the unused portion of memory that follows the file system.
     * This will let us use this space for other things...
     */
    type         = GetMemType(aif);
    pageSize     = GetPageSize(type);
    alignMask    = 16;   /* (1 << GetMemAllocAlignment(type)); This function doesn't currently have a stub for it... */
    imageSize    = aif->aif_ImageROsize + aif->aif_ImageRWsize + aif->aif_ZeroInitSize;
    roundedSize  = ((imageSize + alignMask - 1) / alignMask) * alignMask;
    numPages     = (roundedSize + pageSize - 1) / pageSize;
    FreeMem((void *)((uint32)aif + roundedSize),(numPages * pageSize) - roundedSize);

    binHdr = (_3DOBinHeader *)&aif[1];
    stackSize = binHdr->_3DO_Stack;

    stack = AllocMem(stackSize,MEMTYPE_ANY);
    if (!stack)
    {
        ERR(("oper: no FS thread stack\n"));
        NEVERNEVERLAND;
    }

    fsTask = CreateItemVA(MKNODEID(KERNELNODE,TASKNODE),
                          TAG_ITEM_NAME,            "FileSystem",
                          TAG_ITEM_VERSION,         binHdr->_3DO_Item.n_Version,
                          TAG_ITEM_REVISION,        binHdr->_3DO_Item.n_Revision,
                          CREATETASK_TAG_SUPER,     0,
                          CREATETASK_TAG_PC,        aif,
                          CREATETASK_TAG_SP,        (uint32)stack + stackSize,
                          CREATETASK_TAG_STACKSIZE, stackSize,
                          TAG_END);
    if (fsTask < 0)
    {
        ERR(("oper: no FS task, err %d\n",fsTask));
        NEVERNEVERLAND;
    }

    oldPri = CURRENTTASK->t.n_Priority;
    SetItemPri(CURRENTTASK->t.n_Item,10);
    while (FindNamedItem(MKNODEID(KERNELNODE,FOLIONODE),"File") < 0)
    {
        /* Wait for the file folio to be ready */
        Yield();
    }
    SetItemPri(CURRENTTASK->t.n_Item,oldPri);

    return fsTask;
}


/*****************************************************************************/


static Item createStandardAliases(void)
{
#ifdef DEVELOPMENT
    CreateAlias("boot","/remote");

#else
    Err err;

    if (KernelBase->kb_CPUFlags & KB_ROMAPP)
    {
        PlatformID platform;
        /* we're booting into ROM app mode */
        CreateAlias("boot","/on.ram.1.0");

        /* Find which platform this box is running on */
        memset(&platform, 0, sizeof(PlatformID));
        if (SuperQuerySysInfo(SYSINFO_TAG_PLATFORMID, &platform, sizeof(PlatformID)) !=
                              SYSINFO_SUCCESS)
        {
            DBUG(("Operator couldn't locate platform ID from SysInfo\n"));
        }
        else
            if (platform.mfgr == SYSINFO_MFGR_SA)
                CreateLink("/flash/System","/on.ram.1.0/System", FSLK_PREFER_FIRST);
    }
    else
    {
    char buf[64];

        /* Set $boot to wherever we booted from. This is a run-of-the-mill
         * bootup from CD-based or cable-based OS
         */
        sprintf(buf,"/%s",KernelBase->kb_BootVolumeName);
        CreateAlias("boot",buf);
	if (KernelBase->kb_CPUFlags & KB_ROMOVERCD)
	{
	    /* We're doing ROM-over-CD... */
	    sprintf(buf, "/%s/System", KernelBase->kb_BootVolumeName);
            if ((err = CreateLink("/rom/System", buf, FSLK_PREFER_FIRST)) < 0)
            {
               ERR(("oper: failed to establish link 0x%x\n", err));
                 NEVERNEVERLAND;
            }
        }
    }
#endif

    CreateAlias("audio","$boot/System/Audio");
    CreateAlias("folios","$boot/System/Folios");
    CreateAlias("devices","$boot/System/Devices");
    CreateAlias("drivers","$boot/System/Drivers");
    CreateAlias("tasks","$boot/System/Tasks");
    CreateAlias("scripts","$boot/System/Scripts");

    return 0;
}


/*****************************************************************************/


#define FOLIOLOADER_STACK_SIZE 1600


/* The following three functions work together to load in the 3 standard
 * system folios. This is more complicated than it sounds, which is why
 * this code is so bizarre.
 *
 * The main task of the operator cannot simply call FindAndOpenFolio() to
 * get the folios loaded-in. This is because the kernel will send messages
 * to the operator in order to load the folios in memory. This would cause
 * a deadlock.
 *
 * To avoid a deadlock, the operator must spawn a thread to call
 * FindAndOpenFolio() on its behalf. This works great until the loading
 * thread dies. This causes the open count of the newly-loaded folios
 * to drop to 0, which is turn causes the kernel to unload the folios
 * from memory. Basically, this approach is a big slow NOP.
 *
 * The solution implemented here is to have the loader thread open the
 * folios, and then signal the main operator task that it has done so.
 * The thread then goes to sleep forever. Next, the main task operator
 * wakes up and proceeds to open the standard folios. This bumps the open
 * count of the folios by one. The loader thread is then deleted. Since
 * the open count of the folios is at that time 2, then nuking the
 * thread brings the open count back down to 1. Since the open count
 * is greater than 0, the folios are not unloaded by the kernel and so
 * stick around.
 */


static Item GetFolio(char *name)
{
Item result;

    result = FindAndOpenFolio(name);
    if (result < 0)
    {
        ERR(("oper: couldn't open '%s' folio, err %d\n",name,result));
    }

    return result;
}

static void FolioLoader(int32 signal)
{
    mathFolio     = GetFolio("Operamath");
    graphicsFolio = GetFolio("graphics");
    audioFolio    = GetFolio("audio");
    SendSignal(CURRENTTASK->t_ThreadTask->t.n_Item,signal);
    WaitSignal(0);
}

static Item createStandardFolios(void)
{
void *stack;
Item  thread;
int32 signal;
int32 sigs;

    signal = AllocSignal(0);
    if (signal > 0)
    {
        /* allocate the stack in an out-of-the-way location, so we can load
         * things up without causing some fragmentation
         */
        stack = AllocMem(FOLIOLOADER_STACK_SIZE,MEMTYPE_VRAM);
        if (stack)
        {
            thread = CreateItemVA(MKNODEID(KERNELNODE,TASKNODE),
                                  TAG_ITEM_PRI,             CURRENTTASK->t.n_Priority+1,
                                  TAG_ITEM_NAME,            "FolioLoader",
                                  CREATETASK_TAG_PC,        FolioLoader,
                                  CREATETASK_TAG_STACKSIZE, FOLIOLOADER_STACK_SIZE,
                                  CREATETASK_TAG_SP,        (uint32)stack + FOLIOLOADER_STACK_SIZE,
                                  CREATETASK_TAG_SUPER,     TRUE,
                                  CREATETASK_TAG_ARGC,      signal,
                                  TAG_END);
            if (thread >= 0)
            {
                while (TRUE)
                {
                    ProvideServices(serverPort);
                    sigs = WaitSignal(signal | serverSignal);

                    if (sigs & signal)
                        break;
                }

                OpenItem(mathFolio,NULL);
                OpenItem(graphicsFolio,NULL);
                OpenItem(audioFolio,NULL);
                DeleteItem(thread);

            }
            else
            {
                ERR(("oper: could not launch folio loader thread, err %d\n",thread));
                NEVERNEVERLAND;
            }

            FreeMem(stack,FOLIOLOADER_STACK_SIZE);
        }
        FreeSignal(signal);
    }

    return 0;
}


/*****************************************************************************/


static Item createShell(void)
{
Item shellTask;
char buf[100];

    sprintf(buf,"$tasks/shell $%x\n",ProvideServices);

    shellTask = LoadProgram(buf);

    DBUG(("shellTask item $%x\n",shellTask));
    if (shellTask < 0)
    {
        ERR(("oper: no shell task, err %d\n",shellTask));
        NEVERNEVERLAND;
    }

    return shellTask;
}


/****************************************************************************/


typedef Item (* CreateFunc)(void);

/* a bunch of things to start */
static CreateFunc createFuncs[] =
{
    createIdleThread,
    createRamDriver,
    createTimerDriver,
    createExpansionDriver,
    createLCCDDriver,
    createCDRomDriver,
    createControlPortDriver,

#ifdef DEVELOPMENT
    createMacDriver,
    createMacCDRomDriver,
#endif

#ifdef EEPROM
    createEEPROMDriver,
#endif

    createSPORTDriver,
    createFileSystem,

    createStandardAliases,
    createStandardFolios,

    createShell,

    NULL /* indicates end of array */
};


/*****************************************************************************/


/* This runs in user mode!
 * However all of memory is writable!
 * Therefore, the Operator can't call any routines must be careful
 * not to use any memory that is writable by other processes
 */

int main(int32 argc, char **argv)
{
AIFHeader *aif;
uint32     i;

    DBUG(("Operator started\n"));

    aif          = (AIFHeader *)&__my_AIFHeader;
    SherryHeader = (AIFHeader *)argv[1];

    DBUG(("aif=%lx SherryHeader=%lx\n",aif, SherryHeader));
    DBUG(("RO=%d RW=%d ZI=%d",aif->aif_ImageROsize,aif->aif_ImageRWsize,aif->aif_ZeroInitSize));
    DBUG((" totalsize = %d\n",aif->aif_ImageROsize + aif->aif_ImageRWsize + aif->aif_ZeroInitSize;));

    print_vinfo();

    serverPort = CreateMsgPort("PrivServer",200,0);
    if (serverPort < 0)
    {
        ERR(("oper: no server port\n"));
        NEVERNEVERLAND;
    }
    serverSignal = ((MsgPort *)LookupItem(serverPort))->mp_Signal;

    i = 0;
    while (createFuncs[i])
    {
        DBUG(("Invoking creation function %d at $%x\n",i,createFuncs[i]));
        (* createFuncs[i++])();
    }

    ScavengeMem();

    while (TRUE)
    {
        ProvideServices(serverPort);
        WaitSignal(serverSignal);
    }
}
