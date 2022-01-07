/* $Id: audio_folio.c,v 1.116 1994/12/14 23:36:09 phil Exp $ */
/****************************************************************
**
** Audio Folio
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/

/*
** 921207 PLB Add TraceAudio.
** 921208 PLB Add TRACE. Changed name to "audio"
** 930126 PLB Change to use .dsp files instead of .ofx
**            Use relative pathname if none specified.  (Not /ad)
** 930127 PLB Proper error returns for allocating functions.
** 930208 PLB Add support for AudioTimer and AudioCue
** 930311 PLB Add UnloadInstrument that unloads template
** 930315 PLB Extensive API changes, added Envelope & Attachment
** 930415 PLB Track connections between Templates/Instruments/Samples/Attachments/Knobs
** 930519 PLB Print OperaVersion.
** 930521 PLB Allow arbitrary DAC Control words to be passed with a -c option.
** 930524 PLB Add LoadSampleHere
** 930602 PLB Began Envelopes
** 930612 PLB Envelopes Working, tuning too
** 930713 PLB Add copyright. Add romhack.
** 930728 PLB Do not play note when no sample in range.
** 930809 PLB Envelopes have RELEASEJUMP
** 940203 GLL Revise Folio Greeting to use "print_vinfo"
** 940309 PLB Add semaphore to prevent race condition on startup.
** 940314 PLB Lock semaphore in InitAudioFolio() to completely prevent
**            race condition.
** 940407 PLB Remove BETATEST and some other messages.
** 940411 WJB Patched to prevent dereferencing NULL pointer in swiSetAudioItemInfo()
**            and GetAudioItemInfo() when fed a bad item.
** 940427 PLB Added EnableAudioInput().
** 940506 PLB Added SetAudioFolioInfo(), gHeadAmpKnob
** 940608 PLB SetAudioFolioInfo() caller must be priveledged. Adjust ticks available.
** 940617 PLB Add support for SetItemOwner()
** 940622 WJB Added AUDIO_TUNING_NODE to GetAudioItemInfo() so that it returns
**            the correct error code.
** 940802 WJB Made demand loadable (but not unloadable).
** 940809 WJB Removed redundant PrintError() calls.
** 940831 PLB Some magic requested by Martin Taillefer for demand loading support.
** 940901 WJB Fixed typo in DEMANDLOAD_MAIN_CREATE comparison.
** 940907 WJB Changed audio folio's thread name to just 'audio'.
** 940920 WJB Implemented GetAudioFolioInfo().
** 940921 PLB Use |NODE_SIZELOCKED in node database.
** 941010 WJB Tweaked SetAudioFolioInfo() autodocs.
** 941011 WJB Replaced FindAudioDevice(), ControlAudioDevice(), and GetNumInstruments() with AFReserved().
** 941128 PLB Restore valid sample values if SetInfo fails.
** 941209 PLB Added support for DuckAndCover, removed some dead code.
*/

#include "audio_internal.h"
#include "filefunctions.h"
#include <tags.h>           /* tag iteration */

/* Macros for debugging. */
#define DBUG(x)     /* PRT(x) */
#define BREAKPOINT  Debug()

/* Change this to zero to disable tracing on startup. */
#define TRACEMASK (0)

/* Prototypes */
int32 InitAudioFolio (AudioFolio *sb);
int32 TermAudioFolio (AudioFolio *sb);
int32 OpenAudioBase (AudioFolio *sb);
/* int32 CloseAudioBase (AudioFolio *ab); @@@ not used */
Item internalFindAudioItem (int32 ntype, TagArg *tp);
Item internalOpenAudioItem (Node *n, void *args);
int32 internalCloseAudioItem (Item it, Task *t);
static Err internalSetItemOwner(ItemNode *n, Item NewOwner, struct Task *t);
static int32 internalSetItemPriority(ItemNode *n, uint8 pri, struct Task *t);
void DeleteAudioTaskHook(Task *theTask);
AudioFolioTaskData *SetupAudioFolioTaskDataFor(Task *theTask);
int32 CreateAudioTaskHook(Task *t, TagArg *tags);
int32 swiSetAudioFolioInfo( const TagArg *tp );
int32 GetAudioFolioInfo(  TagArg *tp );

/* from audio_attachments.c */
extern int32 internalSetAttachmentInfo (AudioAttachment *aatt, TagArg *args);
extern int32 internalGetAttachmentInfo (AudioAttachment *aatt, TagArg *args);

/* from handy_tools.c */
extern void SuperMemFree(void *ptr, uint32 size);
extern void *SuperMemAlloc(uint32 size, uint32 type);

/* other internal prototypes */
int32 StartAudioFolio( void *startupdata );
static Err AFReserved (void);

/***************************************************************\
* Static Data & necessary structures                                   *
\***************************************************************/

AudioFolio *AudioBase = NULL;    /* Pointer to Folio base. */
char *AudioStartupDirectory = "$audio";

/* DSPP Externals that need to be called from User Mode, set in main. */
static Item OscInstrumentItem = 0;
static Item gHeadInstrumentItem = 0;

Item gHeadAmpKnob = 0;
Item gTailInstrumentItem = 0;
Item gSplitExecTemplate = 0;

/*
** SWI - Software Interrupt Functions.
*/
void *(*AudioSWIFuncs[])() = {
  (void *(*)())dsphGetAudioCyclesUsed,   /* 39 */
  (void *(*)())dsphGetAudioFrameCount,   /* 38 */
  (void *(*)())swiNotImplemented,        /* 37 */
  (void *(*)())swiReadProbe,             /* 36 */
  (void *(*)())swiSetAudioFolioInfo,     /* 35 */
  (void *(*)())swiEnableAudioInput,      /* 34 */
  (void *(*)())swiAbortTimerCue,         /* 33 */
  (void *(*)())swiBendInstrumentPitch,   /* 32 */
  (void *(*)())swiIncrementGlobalIndex,  /* 31 */
  (void *(*)())swiWhereAttachment,       /* 30 */
  (void *(*)())swiResumeInstrument,      /* 29 */
  (void *(*)())swiPauseInstrument,       /* 28 */
  (void *(*)())swiSetAudioItemInfo,      /* 27 */
  (void *(*)())swiScavengeInstrument,    /* 26 */
  (void *(*)())swiAdoptInstrument,       /* 25 */
  (void *(*)())swiAbandonInstrument,     /* 24 */
  (void *(*)())swiSetMasterTuning,       /* 23 */
  (void *(*)())swiMonitorAttachment,     /* 22 */
  (void *(*)())swiLinkAttachments,       /* 21 */
  (void *(*)())swiStopAttachment,        /* 20 */
  (void *(*)())swiReleaseAttachment,     /* 19 */
  (void *(*)())swiStartAttachment,       /* 18 */
  (void *(*)())swiTweakRawKnob,          /* 17 */
  (void *(*)())swiSetAudioDuration,      /* 16 */
  (void *(*)())swiSetAudioRate,          /* 15 */
  (void *(*)())swiRunAudioSignalTask,    /* 14 , private */
  (void *(*)())swiSignalAtTime,          /* 13 */
  (void *(*)())swiDisconnectInstruments, /* 12 */
  (void *(*)())swiFreeAmplitude,          /* 11 */
  (void *(*)())swiAllocAmplitude,         /* 10 */
  (void *(*)())swiTraceAudio,            /* 9 */
  (void *(*)())swiConnectInstruments,    /* 8 */
  (void *(*)())swiTestHack,              /* 7 */
  (void *(*)())swiPutSampleInfo,         /* 6 obsolete */
  (void *(*)())swiTuneInstrument,        /* 5 */
  (void *(*)())swiTuneInsTemplate,       /* 4 */
  (void *(*)())swiStopInstrument,        /* 3 */
  (void *(*)())swiReleaseInstrument,     /* 2 */
  (void *(*)())swiStartInstrument,       /* 1 */
  (void *(*)())swiTweakKnob,             /* 0 */
};
#define NUM_AUDIOSWIFUNCS (sizeof(AudioSWIFuncs)/sizeof(void *))

/* User functions for Audio folio */
void *(*AudioUserFuncs[])() = {
  (void *(*)())DeleteProbe,         /* -50 */
  (void *(*)())CreateProbe,         /* -49 */
  (void *(*)())DeleteInstrument,    /* -48 */
  (void *(*)())CreateInstrument,    /* -47 */
  (void *(*)())GetAudioFolioInfo,   /* -46 */
  (void *(*)())CreateInsTemplate,   /* -45 */
  (void *(*)())CreateSample,        /* -44 */
  (void *(*)())Convert12TET_F16,    /* -43 */
  (void *(*)())GetAudioTime,        /* -42 */
  (void *(*)())DefineSampleHere,    /* -41 */
  (void *(*)())DeleteDelayLine,     /* -40 */
  (void *(*)())CreateDelayLine,     /* -39 */
  (void *(*)())LoadSampleHere,      /* -38 */
  (void *(*)())DetachSample,        /* -37 */
  (void *(*)())AttachSample,        /* -36 */
  (void *(*)())DeleteEnvelope,      /* -35 */
  (void *(*)())CreateEnvelope,      /* -34 */
  (void *(*)())DetachEnvelope,      /* -33 */
  (void *(*)())AttachEnvelope,      /* -32 */
  (void *(*)())DeleteTuning,        /* -31 */
  (void *(*)())CreateTuning,        /* -30 */
  (void *(*)())GetMasterTuning,     /* -29 */
  (void *(*)())AFReserved,          /* -28 */
  (void *(*)())AFReserved,          /* -27 */
  (void *(*)())DefineInsTemplate,   /* -26 */
  (void *(*)())AFReserved,          /* -25 */
  (void *(*)())UnloadInstrument,    /* -24 */
  (void *(*)())UnloadInsTemplate,   /* -23 */
  (void *(*)())DSPGetTotalRsrcUsed, /* -22 */
  (void *(*)())DSPGetInsRsrcUsed,   /* -21 */
  (void *(*)())DisownAudioClock,    /* -20 */
  (void *(*)())OwnAudioClock,       /* -19 */
  (void *(*)())GetCueSignal,        /* -18 */
  (void *(*)())SleepUntilTime,      /* -17 */
  (void *(*)())GetAudioDuration,    /* -16 */
  (void *(*)())GetAudioRate,        /* -15 */
  (void *(*)())MakeSample,          /* -14 */
  (void *(*)())DSPGetRsrcAlloc,     /* -13 */
  (void *(*)())ScanSample,          /* -12 */
  (void *(*)())UnloadSample,        /* -11 */
  (void *(*)())LoadInstrument,      /* -10 */
  (void *(*)())GetAudioItemInfo,    /* -9 */
  (void *(*)())GetKnobName,         /* -8 */
  (void *(*)())GetNumKnobs,         /* -7 */
  (void *(*)())DebugSample,         /* -6 */
  (void *(*)())SleepAudioTicks,     /* -5 */
  (void *(*)())GrabKnob,            /* -4 */
  (void *(*)())LoadSample,          /* -3 */
  (void *(*)())AllocInstrument,     /* -2 */
  (void *(*)())LoadInsTemplate,     /* -1 */
};
#define NUM_AUDIOUSERFUNCS (sizeof(AudioUserFuncs)/sizeof(void *))

/* Item ID matches the order of the entries in this database. */
struct NodeData AudioNodeData[] = {
  { 0, 0 },
  { sizeof(AudioInsTemplate), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
  { sizeof(AudioInstrument), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
  { sizeof(AudioKnob), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
  { sizeof(AudioSample), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
  { sizeof(AudioCue), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
  { sizeof(AudioEnvelope), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
  { sizeof(AudioAttachment), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
  { sizeof(AudioTuning), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
  { sizeof(AudioProbe), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED }
};
#define AUDIONODECOUNT (sizeof(AudioNodeData)/sizeof(NodeData))

/* Tags used when creating the Folio */
TagArg AudioFolioTags[] = {
  { TAG_ITEM_NAME,       (void *) AUDIOFOLIONAME },              /* name of audio folio */
  { CREATEFOLIO_TAG_DATASIZE,   (void *) sizeof (AudioFolio) },  /* size of audio folio */
  { CREATEFOLIO_TAG_NSWIS,      (void *) NUM_AUDIOSWIFUNCS },    /* number of SWI functions */
  { CREATEFOLIO_TAG_NUSERVECS,  (void *) NUM_AUDIOUSERFUNCS },   /* number of user functions */
  { CREATEFOLIO_TAG_SWIS,       (void *) AudioSWIFuncs },        /* list of swi functions */
  { CREATEFOLIO_TAG_USERFUNCS,  (void *) AudioUserFuncs },       /* list of user functions */
  { CREATEFOLIO_TAG_INIT,       (void *) ((int32)InitAudioFolio) }, /* initialization code */
  { CREATEFOLIO_TAG_DELETEF,    (void *) ((int32)TermAudioFolio) }, /* deletion code */
  { CREATEFOLIO_TAG_ITEM,       (void *) AUDIONODE },
  { CREATEFOLIO_TAG_OPENF,      (void *) ((int32)OpenAudioBase) },
/*{ CREATEFOLIO_TAG_CLOSEF,     (TagData) ((int32)CloseAudioBase) }, @@@ only need this if we are going to try to unload */
  { CREATEFOLIO_TAG_NODEDATABASE,  (void *) AudioNodeData },     /* Audio node database */
  { CREATEFOLIO_TAG_MAXNODETYPE,   (void *) AUDIONODECOUNT },    /* number of nodes */
  { CREATEFOLIO_TAG_TASKDATA,	(void *) sizeof (AudioFolioTaskData) },
  { TAG_END,                     (void *) 0 },                   /* end of tag list */
};


/***************************************************************\
* Code                                                          *
\***************************************************************/

/* -------------------- Open/Close docs */

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/openaudiofolio
 |||	OpenAudioFolio - Opens the audio folio.
 |||
 |||	  Synopsis
 |||
 |||	    Err OpenAudioFolio (void)
 |||
 |||	  Description
 |||
 |||	    This procedure connects a task to the audio folio and must be executed by
 |||	    each task before it makes any audio folio calls. If a task makes an audio
 |||	    folio call before it executes OpenAudioFolio(), the task fails.
 |||
 |||	    Call CloseAudioFolio() to relinquish a task's access to the audio folio.
 |||
 |||	  Arguments
 |||
 |||	    None
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in audio.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    CloseAudioFolio()
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/closeaudiofolio
 |||	CloseAudioFolio - Closes the audio folio.
 |||
 |||	  Synopsis
 |||
 |||	    Err CloseAudioFolio (void)
 |||
 |||	  Description
 |||
 |||	    This procedure closes a task's connection with the audio folio. Call
 |||	    it when your application finishes using audio calls.
 |||
 |||	  Arguments
 |||
 |||	    None
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in audio.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    OpenAudioFolio()
 |||
 **/


/* -------------------- main() */

struct afStartupData {
    Task   *ReplyTask;
    int32   ReplySignal;
    Item    Result;     /* success: audiofolio item.  failure: an error code */
};

static Item StartAudioFolioThread (void);
static void AudioFolioThread (struct afStartupData *startupdata);
static void NotifyStartupFailure (struct afStartupData *startupdata, Err errcode);

static char ThreadStack[5000];      /* cheap trick to get a stack for the AudioFolioThread.
                                       !!! maybe could be smaller?
                                       !!! could be computed from file?
                                       !!! might move some initialization to main() instead of
                                           thread if only initialization requires this big a stack
                                    */

int main (int argc, char *argv[])
{
    Item result;

	DBUG(("Attempt to load AUDIO FOLIO.\n"));

/* Some magic requested by Martin Taillefer for demand loading support. 940831 */
	if( argc != DEMANDLOAD_MAIN_CREATE ) return 0;

	print_vinfo();
  #ifdef PARANOID
	PRT(("AudioFolio: PARANOID error checking turned on!\n"));
  #endif

        /* Load error codes so we can use them during failure */
	if ((result = InitAudioErrors()) < 0) goto clean;

	    /* start thread (on success, returns item # of folio) */
	if ((result = StartAudioFolioThread()) < 0) goto clean;

clean:
        /* print error message on failure */
    if (result < 0) PrintError ("audio folio", "startup", NULL, result);

    return (int)result;     /* @@@ this is an annoying dependency on 32-bit ints */
}

/*
    Start audio folio thread, which also creates the folio.
    Returns item # of folio on success, negative error code on failure.
*/
static Item StartAudioFolioThread (void)
{
    struct afStartupData startupdata;
    int32 result;

        /* set up startup data (cannot permit failure before here) */
    memset (&startupdata, 0, sizeof startupdata);
    startupdata.ReplyTask = CURRENTTASK;
    if ((result = AllocSignal (0)) <= 0) {
        if (!result) result = AF_ERR_NOSIGNAL;
        goto clean;
    }
    startupdata.ReplySignal = result;

        /* create audio folio's thread */
        /* !!! it'd be nice if I could get pri, name, and stack size from aif file */
    if ( (result = CreateItemVA (MKNODEID(KERNELNODE,TASKNODE),
                                 TAG_ITEM_PRI,                  190,
                                 TAG_ITEM_NAME,                 "audio",    /* !!! might be worth inheriting this from elsewhere */
                                 CREATETASK_TAG_PC,             AudioFolioThread,
                                 CREATETASK_TAG_STACKSIZE,      sizeof ThreadStack,
                                 CREATETASK_TAG_SP,             ThreadStack + sizeof ThreadStack,
                                 CREATETASK_TAG_ARGC,           &startupdata,
                              /* CREATETASK_TAG_ALLOCDTHREADSP, 0,  */  /* @@@ add this back in if we allocate a stack here */
                                 CREATETASK_TAG_SUPER,          0,  /* make priveleged */
                                 TAG_END) ) < 0 ) goto clean;

        /* wait for notification of success/failure from AudioFolioThread */
    WaitSignal (startupdata.ReplySignal);
    result = startupdata.Result;
    DBUG(("AudioFolioThread returned $%08lx\n", result));

clean:
    if (startupdata.ReplySignal > 0) FreeSignal (startupdata.ReplySignal);
    return result;
}

static void AudioFolioThread (struct afStartupData *startupdata)
{
	Err errcode;

	DBUG(("AudioFolioThread: replytask: $%08lx,$%08lx\n", startupdata->ReplyTask, startupdata->ReplySignal));

        /* start audio folio. if successful, this won't return */
	errcode = StartAudioFolio (startupdata);

	    /* if we got here, we got an error */
	NotifyStartupFailure (startupdata, errcode);
}

/* called from swiRunAudioSignalTask() */
void NotifyStartupSuccess (void *startupdatat)
{
    struct afStartupData * const startupdata = (struct afStartupData *)startupdatat;

    DBUG(("NotifyStartupSuccess() folio=%ld\n", AudioBase->af_Folio.fn.n_Item));

    startupdata->Result = AudioBase->af_Folio.fn.n_Item;
    SuperInternalSignal (startupdata->ReplyTask, startupdata->ReplySignal);
}

/* called from AudioFolioThread() (task) */
static void NotifyStartupFailure (struct afStartupData *startupdata, Err errcode)
{
    DBUG(("NotifyStartupFailure() errcode=%ld\n", errcode));

    startupdata->Result = errcode;
    SendSignal (startupdata->ReplyTask->t.n_Item, startupdata->ReplySignal);
}


/* -------------------- the rest of the folio startup */

/******************************************************************/
int32 StartAudioFolio( void *startupdata )
{
	Item AudioFolioItem = 0;
	Item DurKnob = 0;
	int32 Result;
	char *HeadName, *TailName;
	Item	Tmp;

/* PRT(("AudioFolio: CPUFLAGS = 0x%x\n", KernelBase->kb_CPUFlags)); */

        /* @@@ memdebug doesn't actually seem to work */
    if ((Result = CreateMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
                                   MEMDEBUGF_FREE_PATTERNS |
                                   MEMDEBUGF_PAD_COOKIES |
                                   MEMDEBUGF_CHECK_ALLOC_FAILURES |
                                   MEMDEBUGF_KEEP_TASK_DATA,
                                   NULL )) < 0) goto error;

/* OpenMathFolio to get MathBase */
	Result = OpenMathFolio();
	if (Result < 0)
	{
	    ERR(("StartAudioFolio: Error opening math folio\n"));
		goto error;
	}

/* Create the Audio folio item. */
	if ((AudioFolioItem = CreateItem(MKNODEID(KERNELNODE,FOLIONODE),AudioFolioTags)) < 0) {
		Result = AudioFolioItem;
	    ERR(("StartAudioFolio: Error creating audio folio\n"));
		goto error;
	}

        /*
            Open audio folio for this thread. This open will not block
            because the open code specifically looks for our task.
        */
	if ((Result = FindAndOpenFolio (AUDIOFOLIONAME)) < 0) {
	    ERR(("StartAudioFolio: Error opening audio folio\n"));
		goto error;
	}

	HeadName = "head.dsp";
	TailName = "tail.dsp";

/* Load first and last instrument. Other instruments exec between. */
/* Set special mode for code linker. 940812 */
	Tmp = LoadInsTemplate ( HeadName, 0 );
	gHeadInstrumentItem = AllocInstrumentSpecial( Tmp, 200, AF_SPECIAL_HEAD );
	if (gHeadInstrumentItem < 0)
	{
		Result = gHeadInstrumentItem;
	    ERR(("StartAudioFolio: Error loading instrument '%s'\n", HeadName));
		goto error;
	}
	DurKnob = GrabKnob(gHeadInstrumentItem, "CountDown");
	if (DurKnob < 0)
	{
		Result = DurKnob;
	    ERR(("StartAudioFolio: head.dsp wrong version. unable to grab 'CountDown' knob\n"));
		goto error;
	}

/* Get head amplitude knob for master volume control. */
	gHeadAmpKnob = GrabKnob(gHeadInstrumentItem, "Amplitude");
	if (gHeadAmpKnob < 0)
	{
	    ERR(("StartAudioFolio: head.dsp wrong version. unable to grab 'Amplitude' knob\n"));
	    /* !!! goto error; ? */
	}
	Result = StartInstrument ( gHeadInstrumentItem, NULL );
	if (Result < 0) goto error;

	Tmp = LoadInsTemplate ( TailName, 0 );
	gTailInstrumentItem = AllocInstrumentSpecial( Tmp, 0, AF_SPECIAL_TAIL );
	if (gTailInstrumentItem < 0)
	{
		Result = gTailInstrumentItem;
	    ERR(("StartAudioFolio: Error loading instrument '%s'\n", TailName));
		goto error;
	}
	Result = StartInstrument ( gTailInstrumentItem, NULL );
	if (Result < 0) goto error;

/* Load external instruments in User mode. */
	OscInstrumentItem = LoadInsExternal("oscupdownfp.dsp", 0);
	if (OscInstrumentItem < 0)
	{
		Result = OscInstrumentItem;
	    ERR(("StartAudioFolio: Error loading external instrument 'oscupdownfp'\n"));
		goto error;
	}

/* Load instrument used to split execution rates. 940811 */
	gSplitExecTemplate = LoadInsTemplate("splitexec.dsp", 0);
	if (gSplitExecTemplate < 0)
	{
		gSplitExecTemplate = 0;
	    ERR(("StartAudioFolio: Could not load 'splitexec.dsp'\n"));
	}

	TraceAudio(0);

/* Recover free pages. */
	ScavengeMem();

        /* run signal stuff until shutdown */
	Result = RunAudioSignalTask (DurKnob, startupdata);
	ERR(("StartAudioFolio: ERROR: RunAudioSignalTask() returned! Old dsp files?\n"));
  #if 0	    /* 940802: removed to let this fall thru to clean up code */
	return(Result);   /* Should never get here! */
  #endif

error:
  #if 0     /* !!! keep this? if so, use correct resource release functions, not just DeleteItem() */
	DeleteItem(OscInstrumentItem);
	DeleteItem(gTailInstrumentItem);
	DeleteItem(DurKnob);
	DeleteItem(gHeadInstrumentItem);
	DeleteItem(AudioFolioItem);
  #endif

    DumpMemDebug (NULL);
    DeleteMemDebug();

	return Result;
}

/******************************************************************/
/* This is called by the kernel when the folio item is opened by an application. */
int32 OpenAudioBase (AudioFolio *ab)
{
	int32 Result = 0;
/*
** Check to see if OpenAudioFolio() was called by the folio itself.
** If so, don't wait for semaphore or it will deadlock.  940314
*/
	if(CURRENTTASK->t.n_Item != ab->af_Folio.fn.n_Owner)
	{
/* Wait for folio to get ready. The folio is locked and unlocked in StartAudioFolio */
		Result = SuperLockSemaphore(ab->af_ReadySemaphore, SEM_WAIT);
					if( Result < 0) return Result;
		Result = SuperUnlockSemaphore(ab->af_ReadySemaphore);
	}

	return Result;
}

#if 0       /* @@@ only need this if we are going to try to unload */
            /* this version doesn't actually work */
/******************************************************************/
/* This is called by the kernel when the folio item is closed by an application. */
int32 CloseAudioBase (AudioFolio *ab)
{
    int32 result;

    Superkprintf ("CloseAudioBase() $%08lx usage=%ld\n", ab, ab->af_Folio.f_OpenCount);

	if (ab->af_Folio.f_OpenCount == 2) {
	    Task *threadp;

	    if ((threadp = (Task *)LookupItem (gAudioFolioThread)) != NULL)
            SuperInternalSignal (threadp, SIGF_ABORT);

        /* really need to wait for the thread to finish */
	}

	return 0;
}
#endif

/******************************************************************/
/* This is called by the kernel when the folio item is created. */
int32 InitAudioFolio (AudioFolio *ab)
{
	int32 Result;
	ItemRoutines *itr;
	TagArg Sema4Tags[2];
	Item Sema4;

	AudioBase = ab;
	AudioBase->af_TraceMask = TRACEMASK;

TRACEE(TRACE_INT,TRACE_ITEM,("InitAudioFolio(0x%lx)\n", ab));

/* Create a semaphore for locking folio access. 940309
** See OpenAudioBase() where the semaphore is locked as a test of readyness.
*/
	Sema4Tags[0].ta_Tag = TAG_ITEM_NAME;
	Sema4Tags[0].ta_Arg = (int32 *) "AFReadySem4";
	Sema4Tags[1].ta_Tag = TAG_END;
	Sema4 = SuperCreateItem(MKNODEID(KERNELNODE,SEMA4NODE), Sema4Tags);
	if (Sema4 < 0)
	{
		ERR(("InitAudioFolio: Could not create Ready Semaphore! 0x%x\n", Sema4));
		return (Sema4);
	}

/* Lock semaphore to prevent others from racing ahead. 940309, 940314 */
	Result = SuperLockSemaphore( Sema4, 0 );
	if( Result < 0) return Result;
	AudioBase->af_ReadySemaphore = Sema4;

/* Set pointers to required Folio functions. */
	itr = AudioBase->af_Folio.f_ItemRoutines;
	itr->ir_Delete = internalDeleteAudioItem;
	itr->ir_Create = internalCreateAudioItem;
	itr->ir_Find = internalFindAudioItem;
	itr->ir_Open = internalOpenAudioItem;
	itr->ir_Close = internalCloseAudioItem;
	itr->ir_SetPriority = internalSetItemPriority;
	itr->ir_SetOwner = internalSetItemOwner; /* 940617 */

	AudioBase->af_Folio.f_FolioDeleteTask = (void (*)()) DeleteAudioTaskHook;
	AudioBase->af_Folio.f_FolioCreateTask = (int32 (*)()) CreateAudioTaskHook;

/* Clear Timer */
	AudioBase->af_Time = 0;
	AudioBase->af_DSPP = &DSPPData;

	AudioBase->af_GlobalIndex = 0;

/* Initialize Item tracking lists. */
	InitList (&AudioBase->af_TemplateList, "AudioTemplates");
	InitList (&AudioBase->af_SampleList, "AudioSamples");

/* Save startup directory in AudioBase */
	if(AudioStartupDirectory)
	{
		AudioBase->af_StartupDirectory = afi_AllocateString(AudioStartupDirectory);
		if(AudioBase->af_StartupDirectory)
		{
			strcpy(AudioBase->af_StartupDirectory, AudioStartupDirectory);
		}
		else
		{
			return AF_ERR_NOMEM;
		}
	}


	Result = DSPP_Init();
	if (Result) return Result;

	Result = InitAudioTimer();
	if (Result < 0) return Result;

	Result = InitAudioDMAInterrupts();
	if (Result < 0) return Result;

	Result = InitDefaultTuning();
	if (Result < 0) return Result;

TRACER(TRACE_INT,TRACE_ITEM,("InitAudioFolio returns 0x%lx\n", Result));
	return Result;
}

/******************************************************************/
/* This is called by the kernel when the folio item is deleted. */
int32 TermAudioFolio (AudioFolio *ab)
{
  int32 Result;

TRACEE(TRACE_INT,TRACE_ITEM,("TermAudioFolio(0x%lx)\n", ab));

	dsppTermDuckAndCover(); /* 941209 */
	
	Result = TermDefaultTuning();
	if (Result < 0) return Result;

	Result = TermAudioDMAInterrupts();
	if (Result < 0) return Result;

	Result = TermAudioTimer();
	if (Result < 0) return Result;


	Result = DSPP_Term();
	if (Result) return Result;

	if(AudioBase->af_StartupDirectory) afi_FreeString(AudioBase->af_StartupDirectory);

TRACER(TRACE_INT,TRACE_ITEM,("TermAudioFolio returns 0x%lx\n", Result));
	return Result;
}

/******************************************************************/
/*
** Internal routines required of all folios.
*/

/*
** This routine is passed an item pointer in n.
** The item is (I think) allocated by CreateItem in the kernel
** based on information in the AudioNodeData array.
*/
Item internalCreateAudioItem (void *n, uint8 ntype, void *args)
{
	Item Result = -1;

TRACEE(TRACE_INT,TRACE_ITEM,("internalCreateAudioItem(0x%lx, %d, 0x%lx)\n", n, ntype, args));
	if( args )
	{
		Result = (Item) afi_IsRamAddr ( (char *) args, sizeof(int32) );
		CHECKRSLT(("Bad TagArg pointer in internalCreateAudioItem\n"));
	}

	CHECKAUDIOOPEN;

	TRACKMEM(("Begin internalCreateAudioItem: n=0x%x, ntype=0x%x\n", n,ntype));

	switch (ntype)
	{


	case AUDIO_TEMPLATE_NODE:
		Result = internalCreateAudioTemplate ((AudioInsTemplate *)n, (TagArg *)args);
		break;

	case AUDIO_INSTRUMENT_NODE:
		DBUG(("Call internalCreateAudioIns\n"));
		Result = internalCreateAudioIns ((AudioInstrument *)n, (TagArg *)args);
		break;

	case AUDIO_KNOB_NODE:
		Result = internalCreateAudioKnob ((AudioKnob *)n, (TagArg *)args);
		break;

	case AUDIO_SAMPLE_NODE:
		Result = internalCreateAudioSample ((AudioSample *)n, (TagArg *)args);
		break;

	case AUDIO_CUE_NODE:
		Result = internalCreateAudioCue ((AudioCue *)n, (TagArg *)args);
		break;

	case AUDIO_ENVELOPE_NODE:
		Result = internalCreateAudioEnvelope ((AudioEnvelope *)n, (TagArg *)args);
		break;

	case AUDIO_ATTACHMENT_NODE:
		Result = internalCreateAudioAttachment ((AudioAttachment *)n, (TagArg *)args);
		break;

	case AUDIO_TUNING_NODE:
		Result = internalCreateAudioTuning ((AudioTuning *)n, (TagArg *)args);
		break;

	case AUDIO_PROBE_NODE:
		Result = internalCreateAudioProbe ((AudioProbe *)n, (TagArg *)args);
		break;

	}

	TRACKMEM(("End internalCreateAudioItem: n=0x%x\n", n));

error:
TRACER(TRACE_INT,TRACE_ITEM,("internalCreateAudioItem returns 0x%lx\n", Result));
  return (Result);
}

/******************************************************************/
int32
internalDeleteAudioItem (Item it, Task *t)
{
  Node *n;
  int32 Result;

TRACEE(TRACE_INT,TRACE_ITEM,("DeleteAudioItem (it=0x%lx, Task=0x%lx)\n", it, t));

  n = (Node *) LookupItem (it);
TRACEB(TRACE_INT,TRACE_ITEM,("DeleteAudioItem: n = $%x, type = %d\n", n, n->n_Type));


	TRACKMEM(("Begin internalDeleteAudioItem: it=0x%x\n", it));

  switch (n->n_Type)
  {
  case AUDIO_INSTRUMENT_NODE:
    Result = internalDeleteAudioIns ((AudioInstrument *)n);
    break;

  case AUDIO_TEMPLATE_NODE:
    Result = internalDeleteAudioTemplate ((AudioInsTemplate *)n);
    break;

  case AUDIO_SAMPLE_NODE:
    Result = internalDeleteAudioSample ((AudioSample *)n);
    break;

  case AUDIO_KNOB_NODE:
    Result = internalDeleteAudioKnob ((AudioKnob *)n);
    break;

  case AUDIO_CUE_NODE:
    Result = internalDeleteAudioCue ((AudioCue *)n);
    break;

  case AUDIO_ENVELOPE_NODE:
    Result = internalDeleteAudioEnvelope ((AudioEnvelope *)n);
    break;

  case AUDIO_ATTACHMENT_NODE:
    Result = internalDeleteAudioAttachment ((AudioAttachment *)n);
    break;

  case AUDIO_TUNING_NODE:
    Result = internalDeleteAudioTuning ((AudioTuning *)n);
    break;

  case AUDIO_PROBE_NODE:
    Result = internalDeleteAudioProbe ((AudioProbe *)n);
    break;

  default:
ERR(("internalDeleteAudioItem: unrecognised type = %d\n", n->n_Type));
	Result = AF_ERR_BADITEM;
  }

	TRACKMEM(("End internalDeleteAudioItem: it=0x%x\n", it));

TRACER(TRACE_INT,TRACE_ITEM,("DeleteAudioItem: Result = $%x\n", Result));
  return Result;
}

/******************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/setaudioiteminfo
 |||	SetAudioItemInfo - Sets parameters of an audio item.
 |||
 |||	  Synopsis
 |||
 |||	    Err SetAudioItemInfo (Item audioItem, TagArg *tagList)
 |||
 |||	    Err SetAudioItemInfoVA (Item audioItem, uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    This procedure sets one or more parameters of an audio item. This is the
 |||	    only way to set parameters for an audio item. The parameters are
 |||	    determined by the tag arguments.
 |||
 |||	  Arguments
 |||
 |||	    audioItem                    Item number of an audio item to modify.
 |||
 |||	    tagList                      List of parameters to set. See each audio
 |||	                                 Item pages for list of tags that are supported
 |||	                                 by each item type.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    If this function fails, the audio item you were modifying may have been
 |||	    partially modified and thus in an undefined state.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    GetAudioItemInfo(), Attachment, Cue, Envelope, Instrument, Knob, Probe,
 |||	    Sample, Template, Tuning
 |||
 **/
int32 swiSetAudioItemInfo( Item AnyItem, TagArg *tp )
{
	Node *n;
	int32 Result;

TRACEE(TRACE_INT,TRACE_ITEM,("SetAudioItemInfo (it=0x%lx,Tags=0x%lx)\n", AnyItem, tp));
	CHECKAUDIOOPEN;

	if ((n = (Node *)LookupItem(AnyItem)) == NULL) return AF_ERR_BADITEM;   /* 940411: trap invalid item number. */
	/* !!! clean up proliferation of returns after thaw. */
	if (n->n_SubsysType != AUDIONODE) return AF_ERR_BADITEM;

	Result = afi_IsRamAddr( (char *) tp, sizeof(TagArg) );
	if (Result < 0)
	{
		ERR(("Bad TagArg address =  0x%x.\n", tp));
		return Result;
	}

	Result = AF_ERR_UNIMPLEMENTED;
	switch (n->n_Type)
	{
	case AUDIO_INSTRUMENT_NODE:
/*		Result = internalSetInstrumentInfo ((AudioInstrument *)n, tp); */
		break;

	case AUDIO_TEMPLATE_NODE:
/*		Result = internalSetTemplateInfo ((AudioInsTemplate *)n, tp); */
		break;

/* Save copy of sample in case modification fails, we can restore valid copy. 941128 */
	case AUDIO_SAMPLE_NODE:
		{
			AudioSample TempSample;
			memcpy( (char *) &TempSample, (char *) n, sizeof(AudioSample));  /* Save passed in sample in temp. */
			Result = internalSetSampleInfo ((AudioSample *)n, tp);
			if( Result < 0 )
			{
				memcpy( (char *) n, (char *) &TempSample, sizeof(AudioSample));  /* Back out any changes. */
			}
		}
		break;

	case AUDIO_KNOB_NODE:
/*		Result = internalSetKnobInfo ((AudioKnob *)n, tp); */
		break;

	case AUDIO_CUE_NODE:
/*		Result = internalSetCueInfo ((AudioCue *)n, tp); */
		break;

	case AUDIO_ENVELOPE_NODE:
 		Result = internalSetEnvelopeInfo ((AudioEnvelope *)n, tp);
		break;

	case AUDIO_ATTACHMENT_NODE:
		Result = internalSetAttachmentInfo ((AudioAttachment *)n, tp);
		break;

 	case AUDIO_TUNING_NODE:
 		Result = internalSetTuningInfo ((AudioTuning *)n, tp);
		break;

 	default:
 		Result = AF_ERR_BADITEM;
	}

	return Result;
}

/******************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/getaudioiteminfo
 |||	GetAudioItemInfo - Gets information about an audio item.
 |||
 |||	  Synopsis
 |||
 |||	    Err GetAudioItemInfo (Item audioItem, TagArg *tagList)
 |||
 |||	  Description
 |||
 |||	    This procedure queries settings of several kinds of audio items. It takes a
 |||	    tag list with the ta_Tag fields set to parameters to query and fills in the
 |||	    ta_Arg field of each TagArg with the parameter requested by that TagArg.
 |||
 |||	    SetAudioItemInfo() can be used to set audio item parameters.
 |||
 |||	  Arguments
 |||
 |||	    audioItem                    Item number of an audio item to query.
 |||
 |||	    tagList                      Pointer to tag list containing AF_TAG_ tags to
 |||	                                 query. The ta_Arg fields of each matched tag
 |||	                                 is filled in with the corresponding setting
 |||	                                 from the audio item.
 |||
 |||	                                 See each audio Item pages for list of tags
 |||	                                 that are supported by each item type.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	    Returns AF_ERR_BADTAG if this function is passed any tag that is not listed
 |||	    above.
 |||
 |||	  Additional Results
 |||
 |||	    Fills in the ta_Arg field of each matched TagArg in tagList with the
 |||	    corresponding setting from the audio item.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    Not all audio Item types support being queried in this way. Those that
 |||	    don't support it, return AF_ERR_UNIMPLEMENTED.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    SetAudioItemInfo(), Attachment, Cue, Envelope, Instrument, Knob, Probe,
 |||	    Sample, Template, Tuning
 |||
 **/
int32 GetAudioItemInfo( Item AnyItem, TagArg *tp )
{
	Node *n;
	int32 Result = AF_ERR_UNIMPLEMENTED;

	if ((n = (Node *)LookupItem(AnyItem)) == NULL) return AF_ERR_BADITEM;   /* 940411: trap invalid item number. */
	/* !!! clean up proliferation of returns after thaw. */
	if (n->n_SubsysType != AUDIONODE) return AF_ERR_BADITEM;

	switch (n->n_Type)
	{
	case AUDIO_INSTRUMENT_NODE:
		Result = internalGetInstrumentInfo ((AudioInstrument *)n, tp);
		break;

	case AUDIO_TEMPLATE_NODE:
/*		Result = internalGetTemplateInfo ((AudioInsTemplate *)n, tp); */
		break;

	case AUDIO_SAMPLE_NODE:
		Result = internalGetSampleInfo ((AudioSample *)n, tp);
		break;

	case AUDIO_KNOB_NODE:
		Result = internalGetKnobInfo ((AudioKnob *)n, tp);
		break;

	case AUDIO_CUE_NODE:
/*		Result = internalGetCueInfo ((AudioCue *)n, tp); */
		break;

	case AUDIO_ENVELOPE_NODE:
/* 		Result = internalGetEnvelopeInfo ((AudioEnvelope *)n, tp); */
		break;

	case AUDIO_ATTACHMENT_NODE:
		Result = internalGetAttachmentInfo ((AudioAttachment *)n, tp);
		break;

 	case AUDIO_TUNING_NODE:
/*		Result = internalGetTuningInfo ((AudioTuning *)n, tp); */
		break;

 	default:
 		Result = AF_ERR_BADITEM;
	}

	return Result;
}

/******************************************************************/
Item internalFindAudioItem (int32 ntype, TagArg *tp)
{
TRACEE(TRACE_INT,TRACE_ITEM,("internalFindAudioItem (0x%lx, 0x%lx)\n", ntype, tp));

	return AF_ERR_UNIMPLEMENTED;
}

/******************************************************************/
Item internalOpenAudioItem (Node *n, void *args)
{
	Item ret = -1;
TRACEE(TRACE_INT,TRACE_ITEM,("OpenAudioItem (0x%lx, 0x%lx)\n", n, args));

  switch (n->n_Type)
  {
  case AUDIO_TEMPLATE_NODE:
    ret = internalOpenAudioTemplate ((AudioInsTemplate *)n, (TagArg *)args);
    break;
  case AUDIO_SAMPLE_NODE:
    ret = internalOpenAudioSample ((AudioSample *)n, (TagArg *)args);
    break;
  }
  return (ret);
}

/******************************************************************/
int32 internalCloseAudioItem (Item it, Task *t)
{
	ItemNode *itn;
	int32 ret=AF_ERR_BADITEM;

TRACEE(TRACE_INT,TRACE_ITEM,("CloseAudioItem (0x%lx, 0x%lx)\n", it, t));
 	itn = (ItemNode *) LookupItem(it);

  switch (itn->n_Type)
  {
  case AUDIO_TEMPLATE_NODE:
    ret = internalCloseAudioTemplate (it, (AudioInsTemplate *)itn);
    break;
  case AUDIO_SAMPLE_NODE:
    ret = internalCloseAudioSample (it, (AudioSample *)itn);
    break;
  }
  return (ret);
}

/******************************************************************/
static int32 internalSetItemPriority(ItemNode *n, uint8 pri, struct Task *t)
{
	int32 OldPri;

	if( pri > 200)
	{
		ERR((" audio SetItemPri: %d > 200\n", pri));
		return AF_ERR_OUTOFRANGE;
	}

	OldPri = (int32) n->n_Priority;
	n->n_Priority = pri;

	return (OldPri);
}

/***************************************************************** 940617 */
static Err internalSetItemOwner(ItemNode *n, Item NewOwner, struct Task *t)
{
DBUG(("internalSetItemOwner: n->n_Owner = 0x%x, NewOwner = 0x%x\n", n->n_Owner, NewOwner ));
	if(n->n_Type == AUDIO_TEMPLATE_NODE)
	{
		n->n_Owner = NewOwner;
		return 0;
	}
	else
	{
		return AF_ERR_UNIMPLEMENTED;
	}
}

/******************************************************************/
/********* Task Specific Folio Data *******************************/
/******************************************************************/
void DeleteAudioTaskHook(Task *theTask)
{
	AudioFolioTaskData *aftd;
	int32 folioIndex;

	folioIndex = AudioBase->af_Folio.f_TaskDataIndex;
	aftd = (AudioFolioTaskData *) theTask->t_FolioData[folioIndex];
	if (aftd)
	{
		DBUG(("Tearing down private-data block 0x%x for task 0x%x\n",
			aftd, theTask->t.n_Item));
		theTask->t_FolioData[AudioBase->af_Folio.f_TaskDataIndex] = NULL;
		SuperMemFree(aftd, sizeof (AudioFolioTaskData));
	}
}

/*******************************************************************
** Allocate memory for task specific data for folio.
*******************************************************************/
AudioFolioTaskData *SetupAudioFolioTaskDataFor(Task *theTask)
{
	AudioFolioTaskData *aftd;
	int32 folioIndex;

DBUG(("SetupAudioFolioTaskDataFor: at 0x%x\n", theTask ));
	folioIndex = AudioBase->af_Folio.f_TaskDataIndex;
DBUG(("SetupAudioFolioTaskDataFor: folioindex = %d\n", AudioBase->af_Folio.f_TaskDataIndex));
	aftd = (AudioFolioTaskData *) theTask->t_FolioData[folioIndex];
	if (aftd == NULL)
	{
		aftd = (AudioFolioTaskData *) SuperMemAlloc(sizeof(AudioFolioTaskData),
					      MEMTYPE_ANY | MEMTYPE_FILL);
		if (!aftd)
		{
			ERR(("Could not create file-folio private data for task %d\n",
				theTask->t.n_Item));
			return (AudioFolioTaskData *) NULL;
		}
		theTask->t_FolioData[folioIndex] = aftd;
	}
DBUG(("SetupAudioFolioTaskDataFor: aftd = 0x%x\n", aftd));
	return aftd;
}

/*******************************************************************
** This called by the kernel when a task is created.
*******************************************************************/
int32 CreateAudioTaskHook(Task *theTask, TagArg *tags)
{
	AudioFolioTaskData *aftd;

	aftd = SetupAudioFolioTaskDataFor(theTask);
	if (!aftd) {
		return AF_ERR_NOMEM;
	}
DBUG(("CreateAudioTaskHook: tags = 0x%x\n", tags ));

	return theTask->t.n_Item;
}

/******************************************************************/
/********* Miscellaneous ******************************************/
/******************************************************************/

/*
    Internal function to stick in function table for unimplemented things.
*/
static Err AFReserved (void)
{
    ERR(("Call to RESERVED function!! Why?\n"));
    return AF_ERR_UNIMPLEMENTED;
}

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/traceaudio
 |||	TraceAudio - Turns specific audio diagnostics on or off (OBSOLETE)
 |||
 |||	  Synopsis
 |||
 |||	    uint32 TraceAudio (uint32 traceMask)
 |||
 |||	  Description
 |||
 |||	    Development versions of the audio folio have diagnostic information
 |||	    available which can be turned on or off with this function. To help 3DO
 |||	    Technical Support diagnose a bug, you may be asked to turn on certain TRACE_
 |||	    flags and send the result for interpretation.
 |||
 |||	    This function has no effect in production versions of the audio folio.
 |||
 |||	    Note that this is a system-wide setting. It affects all clients of the
 |||	    audio folio, not just the task that calls TraceAudio().
 |||
 |||	  Arguments
 |||
 |||	    traceMask                    Set of flags indicating which information
 |||	                                 should be on. A set flag turns on the
 |||	                                 corresponding trace feature, a cleared flag
 |||	                                 turns off the corresponding trace feature. 0
 |||	                                 turns off all tracing.
 |||
 |||	                                 See Flags section for complete description of
 |||	                                 each trace flag.
 |||
 |||	  Flags
 |||
 |||	    TRACE_ATTACHMENT             Trace an attachment.
 |||
 |||	    TRACE_DSP                    Trace DSP specific calls such as
 |||	                                 AllocAmplitude().
 |||
 |||	    TRACE_ENVELOPE               Trace an envelope.
 |||
 |||	    TRACE_INT                    Trace internal information.
 |||
 |||	    TRACE_ITEM                   Trace item creation and deletion.
 |||
 |||	    TRACE_KNOB                   Trace a knob.
 |||
 |||	    TRACE_NOTE                   Trace note playing.
 |||
 |||	    TRACE_OFX                    Trace parsing of DSP instrument files, DSP
 |||	                                 resource allocation, and so on.
 |||
 |||	    TRACE_SAMPLE                 Trace AIFF sample loading, and so on.
 |||
 |||	    TRACE_TIMER                  Trace a timer.
 |||
 |||	    TRACE_TOP                    Trace the parameters passed to, and returned
 |||	                                 from, most audio folio calls.
 |||
 |||	    TRACE_TUNING                 Trace a tuning.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the previous trace mask setting.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V20.
 |||
 |||	  Examples
 |||
 |||	        // Turn on a few interesting things
 |||	    TraceAudio (TRACE_TOP | TRACE_INT | TRACE_KNOB);
 |||
 |||	        // Turn off all tracing
 |||	    TraceAudio (0);
 |||
 |||	  Caveats
 |||
 |||	    Given that this is a system-wide setting that any task can set at any time,
 |||	    it really isn't possible to use the return value of this function to create
 |||	    a stack of previous settings. As soon as more than one task starts setting
 |||	    TraceAudio(), the stack would be invalid.
 |||
 |||	    As of V24, this function has no longer has any effect.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 **/
uint32 swiTraceAudio ( uint32 Mask )
{
	uint32 OldMask;
	OldMask = AudioBase->af_TraceMask;
	AudioBase->af_TraceMask = Mask;
	return OldMask;
}

/******************************************************************/
/* Useful for assigning unique indices for tracking and debugging */
/******************************************************************/
uint32 swiIncrementGlobalIndex( void )
{
	return AudioBase->af_GlobalIndex++;
}


/*******************************************************************
** Folio control 940506
*******************************************************************/
 /**
 |||	AUTODOC PRIVATE swisetaudiofolioinfo
 |||	SetAudioFolioInfo - Set certain system-wide audio folio settings.
 |||
 |||	  Synopsis
 |||
 |||	    Err SetAudioFolioInfo( const TagArg *tags )
 |||
 |||	    Err SetAudioFolioInfoVA( uint32 tag1, ... )
 |||
 |||	    Err SuperSetAudioFolioInfo( const TagArg *tags )
 |||
 |||	    Err SuperSetAudioFolioInfoVA( uint32 tag1, ... )
 |||
 |||	  Description
 |||
 |||	    Sets certain system-wide audio folio settings.
 |||
 |||	  Arguments
 |||
 |||	    tags                    Tags to set.
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_SAMPLE_RATE      (frac16) Sets audio folio's sample rate
 |||	                            in samples / second. Initial default is 44100.
 |||
 |||	    AF_TAG_AMPLITUDE        (uint32) Sets audio folio's master volume
 |||	                            level. Initial default is 0x7fff.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V24.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    GetAudioFolioInfo()
 **/
int32 swiSetAudioFolioInfo( const TagArg *tp )
{
	int32 Result;
	const TagArg *TagPtr;
	int32 SampleRate;

TRACEE(TRACE_INT,TRACE_ITEM,("swiSetAudioFolioInfo (Tags=0x%lx)\n",  tp));
	CHECKAUDIOOPEN;

/* Verify that caller is priveledged. */
	if ((CURRENTTASK->t.n_Flags & TASK_SUPER) == 0)
	{
		ERR(("swiSetAudioFolioInfo: caller must be priveledged!\n"));
/*		ERR(("swiSetAudioFolioInfo: OK for now for testing only!\n")); */
		return(AF_ERR_BADPRIV); /* 940608 */
	}

/* Validate memory address. */
	Result = afi_IsRamAddr( (char *) tp, sizeof(TagArg) );
	if (Result < 0)
	{
		ERR(("Bad TagArg address =  0x%x.\n", tp));
		return Result;
	}

	if (tp)
	{
		for (TagPtr = tp; TagPtr->ta_Tag != 0; TagPtr++)
		{
DBUG(("swiSetAudioFolioInfo:  Tag = %d, Arg = 0x%x\n", TagPtr->ta_Tag, TagPtr->ta_Arg));

			switch (TagPtr->ta_Tag)
			{
			case AF_TAG_SAMPLE_RATE:
/* Check to see that we will have enough DSPP ticks in a sample frame.940608  */
				SampleRate = ConvertF16_32( (ufrac16) TagPtr->ta_Arg );
				Result = DSPPAdjustTicksPerFrame( SampleRate );
				if( Result < 0) return Result;

				DSPPData.dspp_SampleRate = SampleRate;
PRT(("SetAudioFolioInfo: Sample rate changed to %d\n", DSPPData.dspp_SampleRate ));

/* Recalculate all sample base frequencies based on new rate for proper tuning. */
				Result = UpdateAllSampleBaseFreqs();
				if( Result ) return Result;

/* Update audio timer rate. */
				if( AudioBase->af_DesiredTimerRate )
				{
					Result = lowSetAudioRate ( AudioBase->af_DesiredTimerRate );
					if( Result < 0 ) return Result;
				}
				break;

			case AF_TAG_AMPLITUDE:
				if( gHeadAmpKnob )
				{
					Result = swiTweakRawKnob( gHeadAmpKnob, (int32) TagPtr->ta_Arg );
					if( Result < 0 ) return Result;
PRT(("SetAudioFolioInfo: Amplitude changed to %d\n", (int32) TagPtr->ta_Arg ));
				}
				break;

			default:
				return AF_ERR_UNIMPLEMENTED;
				break;
			}
		}
	}
	return Result;
}

/******************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/getaudiofolioinfo
 |||	GetAudioFolioInfo - Get system-wide audio settings.
 |||
 |||	  Synopsis
 |||
 |||	    Err GetAudioFolioInfo (TagArg *tagList)
 |||
 |||	  Description
 |||
 |||	    Queries audio folio for certain system-wide settings. It takes a
 |||	    tag list with the ta_Tag fields set to parameters to query and fills in the
 |||	    ta_Arg field of each TagArg with the parameter requested by that TagArg.
 |||
 |||	    For normal multiplayers, these settings are set to the described normal
 |||	    values during startup. Other kinds of hardware or execution environments
 |||	    (e.g. cable) may have different settings.
 |||
 |||	    None of these paramaters can be set directly by an application.
 |||
 |||	  Arguments
 |||
 |||	    tagList                      Pointer to tag list containing AF_TAG_ tags to
 |||	                                 query. The ta_Arg fields of each matched tag
 |||	                                 is filled in with the corresponding setting
 |||	                                 from the audio folio.
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_SAMPLE_RATE           (frac16) Returns the DSP's sample rate in
 |||	                                 samples/second expressed as a 16.16 value.
 |||	                                 Normally this is 44,100 samples/second. For
 |||	                                 certain cable applications this can be 48,000
 |||	                                 samples/second.
 |||
 |||	                                 When the sample rate is something other than
 |||	                                 44,100, substitute this to everywhere in the
 |||	                                 audio documentation that specifies the sample
 |||	                                 rate as 44,100 samples/second. If your
 |||	                                 application needs to cope with variable DSP
 |||	                                 sample rate and you have need the sample rate
 |||	                                 for something, then you should read this value
 |||	                                 from here rather than hardcoding 44,100, or
 |||	                                 any other sample rate, into your code.
 |||
 |||	    AF_TAG_AMPLITUDE             (uint32) Returns the audio folio's master
 |||	                                 volume level. Normally this is 0x7fff.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	    Returns AF_ERR_BADTAG if this function is passed any tag that is not listed
 |||	    above.
 |||
 |||	  Additional Results
 |||
 |||	    Fills in the ta_Arg field of each matched TagArg in tagList with the
 |||	    corresponding setting from the audio folio.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V24.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    GetAudioItemInfo()
 |||
 **/
Err GetAudioFolioInfo( TagArg *taglist )
{
    const TagArg *tstate;
    TagArg *t;

    for (tstate = taglist; (t = NextTagArg (&tstate)) != NULL; ) {
        DBUG(("GetAudioFolioInfo: Tag = %d\n", t->ta_Tag));

        switch (t->ta_Tag) {
            case AF_TAG_SAMPLE_RATE:
                t->ta_Arg = (TagData)Convert32_F16 (DSPPData.dspp_SampleRate);
                break;

            case AF_TAG_AMPLITUDE:
                {
                    TagArg querytags[] = {
                        { AF_TAG_CURRENT },
                        TAG_END
                    };
                    Err errcode;

                    if ( (errcode = GetAudioItemInfo (gHeadAmpKnob, querytags)) < 0 ) return errcode;
                    t->ta_Arg = querytags[0].ta_Arg;
                }
                break;

            default:
                return AF_ERR_UNIMPLEMENTED;
        }
    }

    return 0;
}

/**************************************************************/
void swiNotImplemented( void )
{
	ERR(("Audiofolio SWI not implemented.\n"));
	while(1); /* Hang to prevent calls from going undetected. */
}
