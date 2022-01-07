/* $Id: items.c,v 1.46 1994/11/19 02:54:28 vertex Exp $ */

/**
|||	AUTODOC PUBLIC tpg/shell/items
|||	items - display lists of active items
|||
|||	  Format
|||
|||	    items [<item number>]
|||	          [-name <item name>]
|||	          [-type <item type>]
|||	          [-owner <task name>]
|||	          [-full]
|||
|||	  Description
|||
|||	    This command displays information about the items that
|||	    currently exist in the system. The information is sent
|||	    out to the debugging terminal.
|||
|||	  Arguments
|||
|||	    <item number>               Requests that only information on the
|||	                                item supplied be displayed. The item
|||	                                number can be in decimal, or in
|||	                                hexadecimal starting with 0x or $.
|||
|||	    -name <item name>           Requests that only items with the
|||	                                supplied name be listed.
|||
|||	    -type <item type>           Requests that only items with the
|||	                                supplied type be listed. The
|||	                                currently supported types are:
|||
|||	                                  Folio
|||	                                  Task
|||	                                  FIRQ
|||	                                  Sema4
|||	                                  Message
|||	                                  MsgPort
|||	                                  Driver
|||	                                  IOReq
|||	                                  Device
|||	                                  Timer
|||	                                  ErrorText
|||	                                  HLInt
|||	                                  ScreenGroup
|||	                                  Screen
|||	                                  Bitmap
|||	                                  VDL
|||	                                  DisplayInfo
|||	                                  Overlay
|||	                                  FileSystem
|||	                                  File
|||	                                  Alias
|||	                                  Locale
|||	                                  Template
|||	                                  Instrument
|||	                                  Knob
|||	                                  Sample
|||	                                  Cue
|||	                                  Envelope
|||	                                  Attachment
|||	                                  Tuning
|||	                                  Probe
|||
|||	    -owner <task name>          Requests that only items owned by the
|||	                                supplied task be displayed.
|||
|||	    -full                       Requests that any extra information
|||	                                available on the items being listed
|||	                                be displayed.
|||
|||	  Implementation
|||
|||	    Command implemented in V20.
|||
|||	  Location
|||
|||	    $c/items
**/

/* This program display information about items currently alive in a system.
 *
 * To add new item types, simply find the pertinent string table, and add
 * the new item name string at the end.
 *
 * To add new subsys types, define a new name table for the items in the
 * subsystem (that is, a table similar to kernelNodes or audioNodes), and
 * add an entry for this table in the nameTables array.
 */

#include "types.h"
#include "nodes.h"
#include "list.h"
#include "kernel.h"
#include "task.h"
#include "kernelnodes.h"
#include "strings.h"
#include "device.h"
#include "io.h"
#include "semaphore.h"
#include "graphics.h"
#include "stdio.h"
#include "debug.h"
#include "filesystem.h"


/*****************************************************************************/


static void DumpExtraFolio(Folio *f);
static void DumpExtraTask(Task *t);
static void DumpExtraIOReq(IOReq *ioReq);
static void DumpExtraSemaphore(Semaphore *sem);
static void DumpExtraMessage(Message *msg);
static void DumpExtraMsgPort(MsgPort *port);
static void DumpExtraErrorText(ErrorText *errText);
static void DumpExtraScreenGroup(ScreenGroup *sg);
static void DumpExtraAlias(Alias *alias);


/*****************************************************************************/


typedef void (* DumpExtraFunc)(void *data);

typedef struct ItemMap
{
    char          *im_Name;
    DumpExtraFunc  im_DumpExtra;
} ItemMap;

static ItemMap kernelNodes[] =
{
    {"<unknown>",	NULL},
    {NULL,		NULL},
    {NULL,		NULL},
    {NULL,		NULL},
    {"Folio",		(DumpExtraFunc)DumpExtraFolio},
    {"Task",		(DumpExtraFunc)DumpExtraTask},
    {"FIRQ",		NULL},
    {"Sema4",		(DumpExtraFunc)DumpExtraSemaphore},
    {NULL,		NULL},
    {"Message",		(DumpExtraFunc)DumpExtraMessage},
    {"MsgPort",		(DumpExtraFunc)DumpExtraMsgPort},
    {NULL,		NULL},
    {NULL,		NULL},
    {"Driver",		NULL},
    {"IOReq",		(DumpExtraFunc)DumpExtraIOReq},
    {"Device",		NULL},
    {"Timer",		NULL},
    {"ErrorText",	(DumpExtraFunc)DumpExtraErrorText},
    {"HLInt",		NULL},
};

static ItemMap graphicsNodes[] =
{
    {"<unknown>",	NULL},
    {"ScreenGroup",	(DumpExtraFunc)DumpExtraScreenGroup},
    {"Screen",		NULL},
    {"Bitmap",		NULL},
    {"VDL",		NULL},
    {"DisplayInfo",	NULL},
    {"Overlay",		NULL},
};

static ItemMap fileNodes[] =
{
    {"<unknown>",	NULL},
    {"FileSystem",	NULL},
    {"File",		NULL},
    {"Alias",		(DumpExtraFunc)DumpExtraAlias},
};

static ItemMap intlNodes[] =
{
    {"<unknown>",	NULL},
    {"Locale",		NULL},
};

static ItemMap audioNodes[] =
{
    {"<unknown>",	NULL},
    {"Template",	NULL},
    {"Instrument",	NULL},
    {"Knob",		NULL},
    {"Sample",		NULL},
    {"Cue",		NULL},
    {"Envelope",	NULL},
    {"Attachment",	NULL},
    {"Tuning",		NULL},
    {"Probe",           NULL},
};


/*****************************************************************************/


typedef struct Subsystem
{
    char    *ss_Name;
    uint32   ss_SubsysType;
    ItemMap *ss_ItemMap;
    uint32   ss_NumItems;
} Subsystem;

static Subsystem subsystems[] =
{
    {"Kernel",        NST_KERNEL,   kernelNodes,   sizeof(kernelNodes) / sizeof(ItemMap)},
    {"Graphics",      NST_GRAPHICS, graphicsNodes, sizeof(graphicsNodes) / sizeof(ItemMap)},
    {"FileSystem",    NST_FILESYS,  fileNodes,     sizeof(fileNodes) / sizeof(ItemMap)},
    {"Audio",         NST_AUDIO,    audioNodes,    sizeof(audioNodes) / sizeof(ItemMap)},
    {"International", NST_INTL,     intlNodes,     sizeof(intlNodes) / sizeof(ItemMap)},
};

#define NUM_SUBSYSTEMS (sizeof(subsystems) / sizeof(Subsystem))


/*****************************************************************************/


static char *timerCommands[] =
{
    "CMD_WRITE",
    "CMD_READ",
    "CMD_STATUS",
    "TIMERCMD_DELAY",
    "TIMERCMD_DELAYUNTIL",
};

static char *cdromCommands[] =
{
    "CMD_WRITE",
    "CMD_READ",
    "CMD_STATUS",
    "CDROMCMD_PASSTHROUGH",
    "CDROMCMD_DISCDATA",
    "CDROMCMD_SETDEFAULTS",
    "CDROMCMD_READ_SUBQ",
    "CDROMCMD_READ_DISC_CODE",
    "CDROMCMD_OPEN_DRAWER",
    "CDROMCMD_CLOSE_DRAWER",
};

static char *controlPortCommands[] =
{
    "CMD_WRITE",
    "CMD_READ",
    "CMD_STATUS",
    "CONTROLPORTCMD_CONFIGURE",
};

static char *maccdromCommands[] =
{
    "CMD_WRITE",
    "CMD_READ",
    "CMD_STATUS",
    "CDROMCMD_SETUNITNAME",
};

static char *lomacCommands[] =
{
    "CMD_WRITE",
    "CMD_READ",
    "CMD_STATUS",
    "LOMAC_DEBUGTRIGGER",
};

static char *macCommands[] =
{
    "CMD_WRITE",
    "CMD_READ",
    "CMD_STATUS",
    "MACCMD_PRINT",
    "MACCMD_ASK",
    "MACCMD_APPEND",
    "MACCMD_FILELEN",
    "MACCMD_WRITECR",
    NULL,
    "MACCMD_READDIR",
    "MACCMD_READCDDELAY",
};

static char *xbusCommands[] =
{
    "CMD_WRITE",
    "CMD_READ",
    "CMD_STATUS",
    "XBUSCMD_Command",
    "XBUSCMD_DrainDataFifo",
    "XBUSCMD_CommandSyncStat",
    "XBUSCMD_SetXferSpeed",
    "XBUSCMD_SetBurstSize",
    "XBUSCMD_SetHogTime",
    "XBUSCMD_WaitDipirStart",
    "XBUSCMD_WaitDipirEnd",
    "XBUSCMD_ResetUnit",
};

static char *sportCommands[] =
{
    "CMD_WRITE",
    "CMD_READ",
    "CMD_STATUS",
    NULL,
    "SPORTCMD_CLONE",
    "SPORTCMD_COPY",
    "FLASHWRITE_CMD",
};

static char *filesystemCommands[] =
{
    "CMD_WRITE",
    "CMD_READ",
    "CMD_STATUS",
    "FILECMD_READDIR",
    "FILECMD_GETPATH",
    "FILECMD_READENTRY",
    "FILECMD_ALLOCBLOCKS",
    "FILECMD_SETEOF",
    "FILECMD_ADDENTRY",
    "FILECMD_DELETEENTRY",
    "FILECMD_SETTYPE",
    "FILECMD_OPENENTRY",
    "FILECMD_FSSTAT",
};

static char *timerUnits[] =
{
    "TIMER_UNIT_VBLANK",
    "TIMER_UNIT_USEC",
};


/*****************************************************************************/


typedef struct CommandMap
{
    char    *cm_DeviceName;
    char   **cm_CommandNames;
    uint32   cm_NumCommands;
    char   **cm_UnitNames;
    uint32   cm_NumUnits;
} CommandMap;

static CommandMap commandMaps[] =
{
    {"timer",       timerCommands,       sizeof(timerCommands) / sizeof(char *), timerUnits, sizeof(timerUnits) / sizeof(char *)},
    {"xbus",        xbusCommands,        sizeof(xbusCommands) / sizeof(char *), },
    {"CD-ROM",      cdromCommands,       sizeof(cdromCommands) / sizeof(char *), },
    {"ControlPort", controlPortCommands, sizeof(controlPortCommands) / sizeof(char *), },
    {"lomac",       lomacCommands,       sizeof(lomacCommands) / sizeof(char *), },
    {"mac",         macCommands,         sizeof(macCommands) / sizeof(char *), },
    {"SPORT",       sportCommands,       sizeof(sportCommands) / sizeof(char *), },
    {"maccdrom",    maccdromCommands,    sizeof(maccdromCommands) / sizeof(char *), },
    {"filesystem",  filesystemCommands,  sizeof(filesystemCommands) / sizeof(char *), },
};

#define NUM_COMMANDMAPS (sizeof(commandMaps) / sizeof(CommandMap))


/*****************************************************************************/


typedef struct FlagMap
{
    uint32  fm_Flag;
    char   *fm_Name;
} FlagMap;


static const FlagMap nodeFlags[] =
{
    {NODE_RSRV1,        "NODE_RSRV1"},
    {NODE_SIZELOCKED,   "NODE_SIZELOCKED"},
    {NODE_ITEMVALID,    "NODE_ITEMVALID"},
    {NODE_NAMEVALID,    "NODE_NAMEVALID"},
    {0,                 NULL}
};

static const FlagMap messageFlags[] =
{
    {NODE_RSRV1,            "NODE_RSRV1"},
    {NODE_SIZELOCKED,       "NODE_SIZELOCKED"},
    {NODE_ITEMVALID,        "NODE_ITEMVALID"},
    {NODE_NAMEVALID,        "NODE_NAMEVALID"},
    {MESSAGE_SENT,          "MESSAGE_SENT"},
    {MESSAGE_REPLIED,       "MESSAGE_REPLIED"},
    {MESSAGE_SMALL,         "MESSAGE_SMALL"},
    {MESSAGE_PASS_BY_VALUE, "MESSAGE_PASS_BY_VALUE"},
    {0,                     NULL}
};

static const FlagMap itemFlags[] =
{
    {ITEMNODE_NOTREADY,         "ITEMNODE_NOTREADY"},
    {ITEMNODE_CONSTANT_NAME,    "ITEMNODE_CONSTANT_NAME"},
    {ITEMNODE_PRIVILEGED,       "ITEMNODE_PRIVILEGED"},
    {ITEMNODE_UNIQUE_NAME,      "ITEMNODE_UNIQUE_NAME"},
    {0,                         NULL}
};

static const FlagMap folioFlags[] =
{
    {FF_DEBUG1, "FF_DEBUG1"},
    {FF_DEBUG2, "FF_DEBUG2"},
    {0,         NULL}
};

static const FlagMap ioFlags[] =
{
    {IO_DONE,     "IO_DONE"},
    {IO_QUICK,    "IO_QUICK"},
    {IO_INTERNAL, "IO_INTERNAL"},
    {0, NULL}
};


/*****************************************************************************/


static void DumpFlags(const FlagMap *fm, uint32 flags)
{
uint32 i,j;
bool   first;

    kprintf("$%08x",flags);
    if (flags)
    {
        first = TRUE;
        for (i = 0; i < 31; i++)
        {
            if ((1 << i) & flags)
            {
                j = 0;
                while (fm[j].fm_Name)
                {
                    if (fm[j].fm_Flag & (1 << i))
                    {
                        if (first)
                        {
                            kprintf(" (");
                            first = FALSE;
                        }
                        else
                        {
                            kprintf(" | ");
                        }

                        kprintf("%s",fm[j].fm_Name);
                        break;
                    }

                    j++;
                }
            }
        }

        if (!first)
            kprintf(")");
    }
}


/*****************************************************************************/


static void DumpExtraFolio(Folio *f)
{
    kprintf("  f_OpenCount        %d\n",f->f_OpenCount);
    kprintf("  f_TaskDataIndex    %u\n",f->f_TaskDataIndex);
    kprintf("  f_MaxSwiFunctions  %u\n",f->f_MaxSwiFunctions);
    kprintf("  f_MaxUserFunctions %u\n",f->f_MaxUserFunctions);
    kprintf("  f_MaxNodeType      %u\n",f->f_MaxNodeType);
    kprintf("  f_DebugTable       $%06x\n",f->f_DebugTable);
    kprintf("  f_DemandLoad       $%06x\n",f->f_DemandLoad);
}


/*****************************************************************************/


/* BackTrace: peel apart APCS3 stack frames.
 *
 * subroutine entry sequence:
 *	SUBR	MOV	IP,SP
 *		STMFD	SP!,{...,FP,IP,LR,PC}	; or STMDB
 *		SUB	FP,IP,#4
 *		CMP	SP,SL
 *		BLLT	__rt_stkovf_split_small
 *
 * subroutine exit sequence:
 *		LDMDB	FP,{...,FP,SP,PC}
 *
 * NOTE: if the FP is "1" then we have reached swi_ret, and we need to
 * use the stack pointer to examine its frame, set up with:
 *
 *		STMFD	SP!,{r8,r9,r10,r11,r14}
 */

static char *CheckAddr(uint32 a)
{
    if (a == 0)
	return "is null";

    if (a & 3)
	return "is badly aligned";

    if (a < 0x200)
	return "points to low memory";

    if (a > 0x2FFFFF)
	return "points above RAM";

    return NULL;
}


/*****************************************************************************/


#define	ASIZE(a) (sizeof(a) / sizeof(a[0]))

static char *kernelSWIs[] =
{
    "CreateSizedItem",
    "WaitSignal",
    "SendSignal",
    "DeleteItem",
    "FindItem",
    "OpenItem",
    "UnlockItem",
    "LockItem",
    "CloseItem",
    "Yield",
    "SetItemPriority",
    "Forbid",
    "Permit",
    "AllocMemBlocks",
    "printf",
    "GetThisMsg",
    "PutMsg",
    "ReadHardwareRandomNumber",
    "ReplyMsg",
    "GetMsg",
    "ControlMem",
    "AllocSignal",
    "FreeSignal",
    "SetFunction",
    "SendIO",
    "AbortIO",
    "RSACheck",
    "wcopy",
    "SetItemOwner",
    "callbacksuper",
    "MayGetChar",
    "illegal",
    "illegal",
    "SystemScavengeMem",
    "CompleteIO",
    "DiscOsVersion",
    "FindAndOpenItem",
    "DoIO"
};

static char *graphicsSWIs[] =
{
    "GrafInit",
    "SetReadAddress",
    "ResetReadAddress",
    "SetClipOrigin",
    "WaitForLine",
    "EnableVAVG",
    "DisableVAVG",
    "EnableHAVG",
    "DisableHAVG",
    "SetScreenColor",
    "ResetScreenColors",
    "ResetSystemGraphics",
    "illegal_12",
    "SetScreenColors",
    "ResetCurrentFont",
    "illegal_15",
    "illegal_16",
    "AddScreenGroup",
    "RemoveScreenGroup",
    "SetClipWidth",
    "SetClipHeight",
    "illegal_21",
    "illegal_22",
    "DrawScreenCels",
    "FillEllipse",
    "OpenFileFont",
    "SetFileFontCacheSize",
    "OpenRAMFont",
    "OpenFileFont",
    "DrawText16",
    "CloseFont",
    "DrawChar",
    "illegal_32",
    "DrawTo",
    "illegal_34",
    "FillRect",
    "SetCurrentFontCCB",
    "GetCurrentFont",
    "DrawText8",
    "DrawCels",
    "illegal_40",
    "SetCEControl",
    "SetCEWatchDog",
    "CopyRect",
    "DeleteScreenGroup",
    "DisplayScreen",
    "DeleteVDL",
    "SetVDL",
    "SubmitVDL",
    "illegal_49",
    "realCreateScreenGroup",
    "ModifyVDL"
};

static char *fileSWIs[] =
{
    "OpenFile",
    "CloseOpenFile",
    "FileDaemonInternals",
    "Unimplemented",
    "MountFileSystem",
    "OpenFileInDir",
    "MountMacFileSystem",
    "ChangeDirectory",
    "GetDirectory",
    "CreateFile",
    "DeleteFile",
    "CreateAlias",
    "LoadOverlay",
    "DismountFileSystem",
    "CreateLink"
};

static char *audioSWIs[] =
{
    "SetAudioFolioInfo",
    "EnableAudioInput",
    "AbortTimerCue",
    "BendInstrumentPitch",
    "IncrementGlobalIndex",
    "WhereAttachment",
    "ResumeInstrument",
    "PauseInstrument",
    "SetAudioItemInfo",
    "ScavengeInstrument",
    "AdoptInstrument",
    "AbandonInstrument",
    "SetMasterTuning",
    "MonitorAttachment",
    "LinkAttachments",
    "StopAttachment",
    "ReleaseAttachment",
    "StartAttachment",
    "TweakRawKnob",
    "SetAudioDuration",
    "SetAudioRate",
    "RunAudioSignalTask",
    "SignalAtTime",
    "DisconnectInstruments",
    "FreeAmplitude",
    "AllocAmplitude",
    "TraceAudio",
    "ConnectInstruments",
    "TestHack",
    "PutSampleInfo",
    "TuneInstrument",
    "TuneInsTemplate",
    "StopInstrument",
    "ReleaseInstrument",
    "StartInstrument",
    "TweakKnob"
};

struct {
    char	       *fname;
    int			ncalls;
    char	      **cname;
}			swi_folio[] = {
    { "debugger",	0,			0 },
    { "sherry",		ASIZE(kernelSWIs),	kernelSWIs },
    { "graphics",	ASIZE(graphicsSWIs),	graphicsSWIs },
    { "filesystem",	ASIZE(fileSWIs),	fileSWIs },
    { "audio",		ASIZE(audioSWIs),	audioSWIs },
    { "math",		0,			0 },
    { "folio 6",	0,			0 },
    { "folio 7",	0,			0 },
    { "av",		0,			0 },
    { "international",	0,			0 },
    { "conn",		0,			0 },
    { "folio B",	0,			0 },
    { "folio C",	0,			0 },
    { "folio D",	0,			0 },
    { "folio E",	0,			0 },
    { "folio F",	0,			0 }
};

#define	SWI_FOLIOS	ASIZE(swi_folio)

#define	ARM_MASK_OPCODE	0x0F000000
#define	ARM_OPCODE_SWI	0x0F000000
#define	ARM_SWI_IMM	0x00FFFFFF
#define	SWI_FOLIO_SHIFT	16
#define	SWI_FOLIO_MASK	0x00FF
#define	SWI_ENTRY_MASK	0xFFFF

static void BackTrace(uint32 *p)
{
int     fc = 32;
char   *s;
uint32  fp;
uint32  sp;
uint32  lr;
uint32  pc;
uint32  i, f, c;

    kprintf("\n  Stack Backtrace:\n");

    fp = (uint32)p;
    while (TRUE)
    {
	if ((s = CheckAddr(fp)) != NULL)
        {
	    kprintf("  ending trace: fp ($%08x) %s\n", fp, s);
	    break;
	}

	kprintf("    fp=$%08x: ", fp);
	if (!--fc)
        {
	    kprintf("probable infinite loop (trace aborted)\n");
	    break;
	}

	p = (uint32 *)fp;
	fp = p[-3];
	sp = p[-2];
	lr = p[-1];
	pc = p[ 0];

	kprintf("$%08x called from $%08x\n", lr-4, pc-8);
	if ((s = CheckAddr(sp)) != NULL)
        {
	    kprintf("  ending trace: sp ($%08x) %s\n", sp, s);
	    break;
	}

	if (fp == 1)
        {
	    if ((s = CheckAddr(sp)) != NULL)
            {
		kprintf("    bad trap frame; sp ($%08x) %s\n", sp, s);
		break;
	    }

	    p = (uint32 *)sp;
	    fp = p[3];
	    lr = p[4];

	    kprintf("    trap from pc=$%08x: ", lr-4);
	    if ((s = CheckAddr(lr)) != NULL)
            {
		kprintf("%s\n", s);
		break;
	    }
            else
            {
		p = (uint32 *)lr;
		i = p[-1];
		if ((i & ARM_MASK_OPCODE) == ARM_OPCODE_SWI)
                {
		    kprintf("SWI ");
		    i = i & ARM_SWI_IMM;
		    if (i == 0)
                    {
			kprintf("INDIRECT\n");
		    }
                    else
                    {
			f = (i >> SWI_FOLIO_SHIFT) & SWI_FOLIO_MASK;
			c = i & SWI_ENTRY_MASK;
			if (f < SWI_FOLIOS)
                        {
			    kprintf("%s/", swi_folio[f].fname);
			    if (c < swi_folio[f].ncalls)
				kprintf("%s\n", swi_folio[f].cname[c]);
			    else
				kprintf("%d\n", c);

			}
                        else
                        {
			    kprintf("%d/%d\n", f, c);
			}
		    }
		}
                else
                {
		    kprintf("instr=$%08x\n", i);
		}
	    }
	}

	if ((s = CheckAddr(pc)) != NULL)
        {
	    kprintf("  ending trace: pc ($%08x) %s\n", pc, s);
	    break;
	}

	if ((s = CheckAddr(lr)) != NULL)
        {
	    kprintf("  ending trace: lr ($%08x) %s\n", lr, s);
	    break;
	}
    }
}


/*****************************************************************************/


static void DumpExtraTask(Task *t)
{
    BackTrace((uint32 *)t->t_regs[11]);
}


/*****************************************************************************/


static void DumpExtraIOReq(IOReq *ior)
{
uint32    table;
bool      found;
ItemNode *node;
char      errorBuffer[100];

    found = FALSE;
    for (table = 0; table < NUM_COMMANDMAPS; table++)
    {
        if (strcasecmp(commandMaps[table].cm_DeviceName,ior->io_Dev->dev.n_Name) == 0)
        {
            found = TRUE;
            break;
        }
    }

    kprintf("  io_Dev         $%06x ('%s', item $%06x)\n",ior->io_Dev,ior->io_Dev->dev.n_Name,ior->io_Dev->dev.n_Item);
    kprintf("  io_CallBack    $%06x\n",ior->io_CallBack);
    kprintf("  ioi_Command    %d",ior->io_Info.ioi_Command);

    if (found
     && (ior->io_Info.ioi_Command < (uint8)commandMaps[table].cm_NumCommands))
    {
        kprintf(" (%s)",commandMaps[table].cm_CommandNames[ior->io_Info.ioi_Command]);
    }
    kprintf("\n");

    kprintf("  ioi_Flags      "); DumpFlags(ioFlags,ior->io_Info.ioi_Flags); kprintf("\n");
    kprintf("  ioi_Unit       %d",ior->io_Info.ioi_Unit);

    if (found
     && commandMaps[table].cm_UnitNames
     && (ior->io_Info.ioi_Unit <= commandMaps[table].cm_NumUnits)
     && (commandMaps[table].cm_UnitNames[ior->io_Info.ioi_Unit]))
    {
        kprintf(" (%s)",commandMaps[table].cm_UnitNames[ior->io_Info.ioi_Unit]);
    }
    kprintf("\n");

    kprintf("  ioi_Flags2     $%02x\n",ior->io_Info.ioi_Flags2);
    kprintf("  ioi_CmdOptions $%08x\n",ior->io_Info.ioi_CmdOptions);
    kprintf("  ioi_User       $%08x\n",ior->io_Info.ioi_User);
    kprintf("  ioi_Offset     %d\n",ior->io_Info.ioi_Offset);
    kprintf("  ioi_Send       buf $%06x, len %d\n",ior->io_Info.ioi_Send.iob_Buffer,ior->io_Info.ioi_Send.iob_Len);
    kprintf("  ioi_Recv       buf $%06x, len %d\n",ior->io_Info.ioi_Recv.iob_Buffer,ior->io_Info.ioi_Recv.iob_Len);
    kprintf("  io_Actual      %d\n",ior->io_Actual);
    kprintf("  io_Flags       "); DumpFlags(ioFlags,ior->io_Flags); kprintf("\n");
    kprintf("  io_Error       $%08x",ior->io_Error);

    if (ior->io_Error < 0)
    {
        GetSysErr(errorBuffer,sizeof(errorBuffer),ior->io_Error);
        kprintf(" ('%s')",errorBuffer);
    }
    kprintf("\n");

    kprintf("  io_MsgItem     $%06x\n",ior->io_MsgItem);
    kprintf("  io_SigItem     $%06x",ior->io_SigItem);

    node = (ItemNode *)CheckItem(ior->io_SigItem,KERNELNODE,TASKNODE);
    if (node)
    {
        kprintf(" (task/thread '%s')",node->n_Name);
    }
    else
    {
        node = (ItemNode *)CheckItem(ior->io_SigItem,KERNELNODE,MESSAGENODE);
        if (node)
        {
            kprintf(" (message)");
        }
    }
    kprintf("\n");
}


/*****************************************************************************/


static void DumpExtraSemaphore(Semaphore *sem)
{
Task *task;

    kprintf("  sem_bit             $%08x\n",sem->sem_bit);
    kprintf("  sem_Owner           $%06x\n",sem->sem_Owner);
    kprintf("  sem_NestCnt         $%d\n",sem->sem_NestCnt);
    kprintf("  sem_TaskWaitingList ");

    SCANLIST(&sem->sem_TaskWaitingList,task,Task)
    {
        kprintf("$%06x ",task->t.n_Item);
    }

    kprintf("\n");
}


/*****************************************************************************/


static void DumpExtraMessage(Message *msg)
{
ItemNode *node;

    kprintf("  msg_ReplyPort   $%06x\n",msg->msg_ReplyPort);
    kprintf("  msg_Result      %d\n",msg->msg_Result);
    kprintf("  msg_DataPtr     $%08x\n",msg->msg_DataPtr);
    kprintf("  msg_DataSize    $%d\n",msg->msg_DataSize);
    kprintf("  msg_MsgPort     $%06x\n",msg->msg_MsgPort);
    kprintf("  msg_DataPtrSize %d\n",msg->msg_DataPtrSize);
    kprintf("  msg_SigItem     $%06x",msg->msg_SigItem);

    node = (ItemNode *)CheckItem(msg->msg_SigItem,KERNELNODE,TASKNODE);
    if (node)
        kprintf(" ('%s')",node->n_Name);

    kprintf("\n");
}


/*****************************************************************************/


static void DumpExtraMsgPort(MsgPort *port)
{
    kprintf("  mp_Signal   $%08x\n",port->mp_Signal);
    kprintf("  mp_UserData $%08x\n",port->mp_UserData);
}


/*****************************************************************************/


static void DumpExtraErrorText(ErrorText *errText)
{
    kprintf("  et_ObjID         $%08x\n",errText->et_ObjID);
    kprintf("  et_MaxErr        %d\n",errText->et_MaxErr);
    kprintf("  et_MaxStringSize %d\n",errText->et_MaxStringSize);
    kprintf("  et_ErrorTable    $%08x\n",errText->et_ErrorTable);
}


/*****************************************************************************/


static void DumpExtraScreenGroup(ScreenGroup *sg)
{
Screen *scr;

    kprintf("  sg_Y             %d\n",sg->sg_Y);
    kprintf("  sg_ScreenHeight  %d\n",sg->sg_ScreenHeight);
    kprintf("  sg_DisplayHeight %d\n",sg->sg_DisplayHeight);
    kprintf("  ag_Add_SG_Called %d\n",sg->sg_Add_SG_Called);
    kprintf("  ag_ScreenList    ");

    SCANLIST(&sg->sg_ScreenList,scr,Screen)
    {
        kprintf("$%06x ",scr->scr.n_Item);
    }

    kprintf("\n");
}


/*****************************************************************************/


static void DumpExtraAlias(Alias *alias)
{
    kprintf("  a_Value      '%s'\n",alias->a_Value);
}


/*****************************************************************************/


static bool DumpItem(Item it, bool full)
{
ItemNode     *n;
bool          found;
uint32        table;
Item          owner;
ItemNode     *t;
DumpExtraFunc dumpExtra;
char          typeStr[80];
char          ownerStr[80];
char          nameStr[80];

    n = (ItemNode *)LookupItem(it);
    if (n)
    {
        found     = FALSE;
        dumpExtra = NULL;

        for (table = 0; table < NUM_SUBSYSTEMS; table++)
        {
            if (subsystems[table].ss_SubsysType == n->n_SubsysType)
            {
                found = TRUE;
                break;
            }
        }

        if (found)
        {
            if (n->n_Type < subsystems[table].ss_NumItems)
            {
                strcpy(typeStr,subsystems[table].ss_ItemMap[n->n_Type].im_Name);
                dumpExtra = subsystems[table].ss_ItemMap[n->n_Type].im_DumpExtra;
            }
            else
            {
                sprintf(typeStr,"<type %d from %s>,",n->n_Type,subsystems[table].ss_Name);
            }
        }
        else
        {
            /* unknown subsystem, try to find the responsible party and print its name  */
            Item folioI = n->n_SubsysType;
            Node *n = (Node *)LookupItem(folioI);
            sprintf(typeStr,"(%s's node) ",n->n_Name);
        }

        owner = n->n_Owner;
        t = (ItemNode *)LookupItem(owner);
        if (owner == 0)
        {
            strcpy(ownerStr,"'kernel'");
        }
        else if (t == 0)
        {
            strcpy(ownerStr,"<unknown>");
        }
        else
        {
            sprintf(ownerStr,"'%s'",t->n_Name);
        }

        if (n->n_Flags & NODE_NAMEVALID)
        {
            if (n->n_Name)
            {
                sprintf(nameStr,"'%s'",n->n_Name);
            }
            else
            {
                strcpy(nameStr,"<null>");
            }
        }
        else
        {
            strcpy(nameStr,"<unnamed>");
        }

        if (full)
        {
            kprintf("-----------\n");
            kprintf("  Item address $%06x\n",n);
            kprintf("  n_Type       %s\n",typeStr);
            kprintf("  n_Priority   %u\n",n->n_Priority);
            kprintf("  n_Flags      ");

            if ((n->n_SubsysType == KERNELNODE) && (n->n_Type == MESSAGENODE))
                DumpFlags(messageFlags,n->n_Flags);
            else
                DumpFlags(nodeFlags,n->n_Flags);

            kprintf("\n");
            kprintf("  n_Size       %d\n",n->n_Size);
            kprintf("  n_Name       %s\n",nameStr);
            kprintf("  n_Version    %u\n",n->n_Version);
            kprintf("  n_Revision   %u\n",n->n_Revision);
            kprintf("  n_FolioFlags "); DumpFlags(folioFlags,n->n_FolioFlags); kprintf("\n");
            kprintf("  n_ItemFlags  "); DumpFlags(itemFlags,n->n_ItemFlags); kprintf("\n");
            kprintf("  n_Item       $%08x\n",n->n_Item);
            kprintf("  n_Owner      $%08x (%s)\n",n->n_Owner,ownerStr);

            if (dumpExtra)
                (* dumpExtra)(n);
        }
        else
        {
            kprintf("$%06x, @ $%06x, pri %3d, ", it, n, n->n_Priority);
            strcat(typeStr,",");
            kprintf("type %-12s owner ",typeStr);

            if (n->n_Flags & NODE_NAMEVALID)
            {
                strcat(ownerStr,",");
                kprintf("%-14s ",ownerStr);
                kprintf("name %s\n",nameStr);
            }
            else
            {
                kprintf("%-12s\n",ownerStr);
            }
        }

        return (TRUE);
    }

    return (FALSE);
}


/*****************************************************************************/


#define ITEMS_PER_BLOCK	128

static ItemEntry *GetItemEntryPtr(uint32 i)
{
ItemEntry  *p;
ItemEntry **ipt;
uint32      j;

    ipt = KernelBase->kb_ItemTable;
    if (KernelBase->kb_MaxItem <= i)	return 0;
    j = i/ITEMS_PER_BLOCK;	/* which block */
    p = ipt[j];
    i -= j*ITEMS_PER_BLOCK;	/* which one in this block? */

    return p + i;
}


/*****************************************************************************/


static bool GetType(const char *name, int32 *subsystype, int32 *type)
{
uint32  i;
uint32  table;
char   *subname;

    /* try to match string to our tables */
    for (table = 0; table < NUM_SUBSYSTEMS; table++)
    {
        for (i = 0; i < subsystems[table].ss_NumItems; i++)
        {
            subname = subsystems[table].ss_ItemMap[i].im_Name;
            if (subname)
            {
                if (strcasecmp(name,subname) == 0)
                {
                    *type       = i;
                    *subsystype = subsystems[table].ss_SubsysType;

                    return (TRUE);
                }
            }
        }
    }

    return (FALSE);
}


/*****************************************************************************/


static void PrintUsage(void)
{
    kprintf("items - prints out list of all active items\n");
    kprintf("  <item number>      - display info on the item with this number\n");
    kprintf("  -name <item name>  - display info on items with this name\n");
    kprintf("  -type <item type>  - display info on items with this type\n");
    kprintf("  -owner <task name> - display info on items belonging to this task\n");
    kprintf("  -full              - display more details if available\n");
}


/*****************************************************************************/


static int32  itemType   = -1;
static int32  subsysType = -1;
static Item   itemOwner  = -1;
static char  *itemName   = NULL;


/*****************************************************************************/


static bool MatchItem(ItemNode *ip)
{
    /* Does this item match criteria for dumping contents? */

    if (itemType >= 0)
    {
        if ((ip->n_SubsysType != subsysType) || (ip->n_Type != itemType))
            return (FALSE);
    }

    if (itemOwner >= 0)
    {
        if (ip->n_Owner != itemOwner)
            return FALSE;
    }

    if (itemName)
    {
        if (!ip->n_Name)
            return (FALSE);

        if (!(ip->n_Flags & NODE_NAMEVALID))
            return (FALSE);

        if (strcasecmp(itemName,ip->n_Name))
            return (FALSE);
    }

    return (TRUE);
}


/*****************************************************************************/


static uint32 ConvertNum(char *str)
{
    if (*str == '$')
    {
        str++;
        return strtoul(str,0,16);
    }

    return strtoul(str,0,0);
}


/*****************************************************************************/


int main(int argc, char **argv)
{
int32      parm;
bool       full;
ItemEntry *ie;
Item       gen;
uint32     i;
ItemNode  *ip;
Item       itm;

    full = FALSE;

    for (parm = 1; parm < argc; parm++)
    {
        if ((strcasecmp("-help",argv[parm]) == 0)
         || (strcasecmp("-?",argv[parm]) == 0)
         || (strcasecmp("help",argv[parm]) == 0)
         || (strcasecmp("?",argv[parm]) == 0))
        {
            PrintUsage();
            return (0);
        }

        if (strcasecmp("-owner",argv[parm]) == 0)
        {
            /* do owner match also */
            parm++;
            if (parm == argc)
            {
                kprintf("No task name specified for '-owner' option\n");
                return (1);
            }

            itemOwner = FindTask(argv[parm]);
            if (itemOwner < 0)
            {
                kprintf("Could not find owner task '%s'\n",argv[parm]);
                return (1);
            }
        }
        else if (strcasecmp("-name",argv[parm]) == 0)
        {
            parm++;
            if (parm == argc)
            {
                kprintf("No item name specified for '-name' option\n");
                return (1);
            }

            itemName = argv[parm];
        }
        else if (strcasecmp("-full",argv[parm]) == 0)
        {
            full = TRUE;
        }
        else if (strcasecmp("-type",argv[parm]) == 0)
        {
            parm++;
            if (parm == argc)
            {
                kprintf("No type name specified for '-type' option\n");
                return (1);
            }

            if (!GetType(argv[parm],&subsysType,&itemType))
            {
                kprintf("Could not find item type '%s'\n",argv[parm]);
                return (1);
            }
        }
        else
        {
            /* interpret parameter as item number */
            Item t = (Item)(ConvertNum(argv[parm]));

            if (LookupItem(t))
            {
                DumpItem(t,TRUE);
                return (0);
            }

            if (!GetType(argv[parm],&subsysType,&itemType))
                itemName = argv[parm];
        }
    }

    for (i = 0; i < KernelBase->kb_MaxItem; i++)
    {
        ie  = GetItemEntryPtr(i);
        gen = (int)(ie->ie_ItemInfo & ITEM_GEN_MASK);
        itm = (Item)(gen+i);

        ip = (ItemNode *)LookupItem(itm);
        if (ip)
        {
            if (MatchItem(ip))
            {
                DumpItem(itm,full);
            }
        }
    }
}
