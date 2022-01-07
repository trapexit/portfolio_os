/* $Id: soundspooler.c,v 1.64 1994/11/17 04:38:23 phil Exp $ */
/****************************************************************************
**
**  Simple sound spooler.  Queue blocks of audio data for DSP.
**
**  By: Phil Burk and Darren Gibbs
**
**  Copyright (c) 1992-1994, 3DO Company.
**  This program is proprietary and confidential.
**
**---------------------------------------------------------------------------
**
**  History:
**
**  931212 PLB  Created.
**  940202 NMD  Modified ssplCreateSoundSpooler to contain a buffer sequence number.
**  940216 NMD  Added a bufferProcessor to ssplAbort so buffers attached to the
**              spooler can be munged appropriately.
**  940224 PLB  Merged Neal's changes with Greg's.
**  940405 PLB  Revert PrintError changes to allow 1p2 compile.
**  940418 PLB  Clear buffer signals in ssplAbort().
**              Prevent race condition in ssplSendBuffer().
**              Use DetachSample() instead of DeleteItem().
**  940421 PLB  ssplStartSpooler wasn't checking result from StartAttachment()
**              Add ssplReset(), ssplAbort() calls it.
**  940509 WJB  Fixed warnings in ssplReset() and ssplAbort().
**              Clears signals of all buffers in active queue in ssplStartSpooler().
**  940511 WJB  Added a triple bang on "stutter" race condition in ssplSendBuffer().
**  940516 WJB  Tweaked includes, diagnostic functions, comments.
**  940516 WJB  Added autodocs for ssplReset().
**  940516 WJB  Streamlined list management functions.
**              Added sspl prefix to diagnostic functions.
**  940516 WJB  Commented out ssplSetSequenceNum().
**  940516 WJB  Documented the implicit necessity to call ssplProcessSignals()
**              immediately after WaitSignal().
**  940517 WJB  Added autodoc for ssplProcessSignals() w/ note about necessity
**              to call ssplProcessSignals() immediately after WaitSignal().
**  940519 WJB  Added SoundBufferFunc callback system.
**  940525 WJB  Added some initial values for Result.
**  940527 WJB  Updated autodocs.
**  940527 WJB  Replaced music.h with musicerror.h.
**  940531 WJB  Added NULL pointer tolerance to ssplDeleteSoundSpooler() and
**              ssplDeleteSoundBufferNode().
**  940531 WJB  Updated documentation.
**              Removed some redundant initialization in ssplCreateSoundSpooler().
**              Added more stuff to ssplDumpSoundSpooler().
**  940531 WJB  Minor code improvements.
**  940531 WJB  Added ssplBufferToFreeList() at beginning of ssplSendBuffer().
**              Updated docs a bit.
**  940531 WJB  Balanced ssplCreate/DeleteSoundBufferNode().
**  940531 WJB  Privatized ssplCreate/DeleteSoundBufferNode().
**  940620 WJB  Tweaked debug code a bit.
**  940622 WJB  Tweaked debug code a bit.
**  940623 WJB  Added some more debugging code.
**  940623 WJB  Tweaked debug code a bit more.
**  940701 WJB  Added code for new ssplStartSpoolerTags() for eventual publication.
**  940714 WJB  Published ssplStartSpoolerTags(). Added ssplIsSpoolerActive().
**  940727 WJB  Changed AUTODOC subsystem prefix to new standard.
**  940811 WJB  Reverted (TagData) back to (void *) to make this build under 1.3.
**  940906 WJB  Added sample DMA alignment check in ssplSetBufferAddressLength().
**  940906 WJB  Added NULL pointer / zero length trap in ssplSendBuffer().
**  940907 WJB  Added RequestedBuffers list.
**  940907 WJB  Optimized ssplDumpSoundSpooler() and ssplDeleteSoundSpooler().
**  940907 WJB  Added ssplUnrequestBuffer().
**  940908 WJB  Revised ssplSpoolData() and ssplPlayData() to use ssplUnrequestBuffer() on failure.
**  940909 WJB  Added a documentation procedure note for the convenience functions.
**  940913 WJB  Tweaked autodocs.
**  940913 WJB  Added ssplStartSpoolerTagsVA().
**  940913 WJB  Switched over to using varargs functions.
**  940926 PLB  Use SendAttachment() to avoid race condition.
**  940927 WJB  Added usage of ClearCurrentSignals().
**  941004 WJB  Privatized SSPL_FLAG_STARTED and renamed it to SSPL_F_STARTED.
**  941005 WJB  Added SSPL_F_PAUSED.
**  941005 WJB  Added ssplGetSpoolerStatus(). Turned ssplIsSpoolerActive() into a macro.
**  941111 WJB  Replaced MakeSample() with CreateSample().
**  941116 PLB  Removed use of SendAttachment()
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**  NMD: Neil Day
**
****************************************************************************/

#include <audio.h>
#include <list.h>
#include <mem.h>
#include <musicerror.h>         /* ML_ERR_ */
#include <operror.h>            /* PrintError() */
#include <stdio.h>              /* printf() */
#include <task.h>               /* WaitSignal() */
#include <types.h>
#include <varargs_glue.h>       /* VAGLUE_ stuff */

#include <soundspooler.h>

#include "music_internal.h"     /* package id */


/* -------------------- Debugging control */

#define DEBUG_CheckCompletionOrder  0       /* prints a message when buffers appear to complete out of order */


/* -------------------- Package ID */

DEFINE_MUSICLIB_PACKAGE_ID(soundspooler)


/* -------------------- Macros */

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
/*	PRT(("Ran %s, got 0x%x\n", name, val)); */ \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}

    /* process nodes in list (including removing them)
       notes: only evaluates NextNode() when IsNode() is true */
#define PROCESSLIST(l,n,s,t)                                \
    for ( n = (t *)FirstNode(l);                            \
          IsNode(l,n) && ((s = (t *)NextNode(n)), TRUE);    \
          n = s )

/* -------------------- Internal structure definitions */

    /* Internal SoundSpooler.sspl_Flags */
#define SSPL_F_STARTED  0x01        /* Spooler's instrument has been started.
                                       Set by ssplStartSpooler();
                                       cleared by ssplStopSpooler(). Mapped to
                                       SSPL_STATUS_F_STARTED in
                                       ssplGetSpoolerStatus(). */

#define SSPL_F_PAUSED   0x02        /* Spooler's instrument has been paused.
                                       Set by ssplPause();
                                       cleared by ssplResume(),
                                       ssplStartSpooler() and ssplStopSpooler().
                                       Mapped to SSPL_STATUS_F_PAUSED in
                                       ssplGetSpoolerStatus(). */


/* -------------------- Local functions */

static Err CheckSampleAlignment (const void *addr, uint32 len);
static void ssplClearBufferSignal( SoundSpooler *sspl, SoundBufferNode *sbn );
static SoundBufferNode *ssplCreateSoundBufferNode( SoundSpooler *sspl );
static Err ssplDeleteSoundBufferNode(  SoundSpooler *sspl, SoundBufferNode *sbn );


/* -------------------- Diagnostics */

#ifndef RUNTIME

static void DumpSoundBufferList (const List *sbnlist);

/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/sspldumpsoundspooler
 |||	ssplDumpSoundSpooler - Print debug info for SoundSpooler.
 |||
 |||	  Synopsis
 |||
 |||	    void ssplDumpSoundSpooler ( const SoundSpooler *sspl )
 |||
 |||	  Description
 |||
 |||	    Prints debugging information for SoundSpooler including the contents of
 |||	    both the active and free queues (calls ssplDumpSoundBufferNode() for each
 |||	    SoundBufferNode).
 |||
 |||	  Arguments
 |||
 |||	    sspl                        SoundSpooler to print.
 |||
 |||	  Return Value
 |||
 |||	    None
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplDumpSoundBufferNode()
**/

void ssplDumpSoundSpooler( const SoundSpooler *sspl )
{
	printf("SoundSpooler 0x%08lx: nbufs=%ld sigs=0x%08lx\n", sspl, sspl->sspl_NumBuffers, sspl->sspl_SignalMask);

    DumpSoundBufferList (&sspl->sspl_ActiveBuffers);
    DumpSoundBufferList (&sspl->sspl_RequestedBuffers);
    DumpSoundBufferList (&sspl->sspl_FreeBuffers);
}

/* dump a list of SBN's */
static void DumpSoundBufferList (const List *sbnlist)
{
	const SoundBufferNode *sbn;

	printf ("%s:\n", sbnlist->l.n_Name);
    SCANLIST (sbnlist, sbn, SoundBufferNode)
	{
		ssplDumpSoundBufferNode( sbn );
	}
}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/sspldumpsoundbuffernode
 |||	ssplDumpSoundBufferNode - Print debug info for SoundBufferNode.
 |||
 |||	  Synopsis
 |||
 |||	    void ssplDumpSoundBufferNode ( const SoundBufferNode *sbn )
 |||
 |||	  Description
 |||
 |||	    Prints debugging information for a SoundBufferNode.
 |||
 |||	  Arguments
 |||
 |||	    sbn                         SoundBufferNode to print.
 |||
 |||	  Return Value
 |||
 |||	    None
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplDumpSoundSpooler()
**/

void ssplDumpSoundBufferNode( const SoundBufferNode *sbn )
{
	printf ("  SBN #%ld (0x%08lx): sig=0x%08x addr=0x%08x len=%ld",
	    sbn->sbn_SequenceNum, sbn, sbn->sbn_Signal, sbn->sbn_Address, sbn->sbn_NumBytes);

	{
        TagArg tags[] = {
            { AF_TAG_STATUS, 0 },
            TAG_END
        };

        if (GetAudioItemInfo (sbn->sbn_Attachment, tags) >= 0)      /* this isn't necessarily supported */
            printf (" attstat=%ld", (int32)tags[0].ta_Arg);
    }

	printf ("\n");
}


#endif


/* -------------------- List management */

#define ssplBufferToFreeList(sspl,sbn)      ssplMoveBufferToList (&(sspl)->sspl_FreeBuffers, (sbn))
#define ssplBufferToActiveList(sspl,sbn)    ssplMoveBufferToList (&(sspl)->sspl_ActiveBuffers, (sbn))
#define ssplBufferToRequestedList(sspl,sbn) ssplMoveBufferToList (&(sspl)->sspl_RequestedBuffers, (sbn))

/*************************************************************************
** Remove from one of the lists.
*************************************************************************/
static void ssplRemoveBufferFromList( SoundBufferNode *sbn )
{
	if(sbn->sbn_Flags & SBN_FLAG_INLIST)
	{
		RemNode( (Node *) sbn );
		sbn->sbn_Flags &= ~SBN_FLAG_INLIST;
	}

}

/*************************************************************************
** Move a SoundBufferNode to a list, possibly from a different list.
*************************************************************************/
static void ssplMoveBufferToList (List *list, SoundBufferNode *sbn)
{
    ssplRemoveBufferFromList (sbn);

    AddTail (list, (Node *)sbn);
	sbn->sbn_Flags |= SBN_FLAG_INLIST;
}


/* -------------------- SoundBufferFunc support */

/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/soundbufferfunc
 |||	(*SoundBufferFunc)() - SoundSpooler callback function typedef.
 |||
 |||	  Synopsis
 |||
 |||	    typedef int32 (*SoundBufferFunc) ( SoundSpooler *sspl,
 |||	                                       SoundBufferNode *sbn,
 |||	                                       int32 msg )
 |||
 |||	  Description
 |||
 |||	    This callback system is a superset of the (*UserBufferProcessor)() system.
 |||	    It offers 2 basic improvements over the former:
 |||
 |||	    1.  It provides a method of getting buffer start as well as completion
 |||	        notification.
 |||
 |||	    2.  UserBufferProcessor can't be called by functions such as
 |||	        ssplDetachInstrument() or ssplPlayData() because these functions
 |||	        do not have a UserBufferProcessor argument.  The SoundBufferFunc is
 |||	        stored in the SoundSpooler structure, so all support functions
 |||	        can call it.
 |||
 |||	    The client can install a callback function pointer of this type in the
 |||	    SoundSpooler by calling ssplSetSoundBufferFunc().  Several sound spooler
 |||	    functions send notification messages to this function when the state of a
 |||	    SoundBufferNode changes.  When a buffer is started, one of the start class
 |||	    messages is sent to the SoundBufferFunc.  When a buffer ends, one of the
 |||	    end class messages is sent.
 |||
 |||	    This callback function is called with a message id, a pointer to the
 |||	    SoundSpooler and SoundBufferNode whose state has changed.  The callback
 |||	    function can call non-destructive sound spooler functions like
 |||	    ssplGetUserData(), ssplGetSequencenNum(), etc, but is not permitted to do
 |||	    anything to change the state of the sound spooler.  In particular, calling
 |||	    ssplRequestBuffer() or ssplSendBuffer() typically will confuse the sound
 |||	    spooler's list processing functions. Also, the state of SSPL_STATUS_F_ACTIVE
 |||	    is undefined when inside a SoundBufferFunc function.
 |||
 |||	    Generally, the callback function must be able to handle all message types.
 |||	    To simplify message handling, you can get the class of a message by
 |||	    calling ssplGetSBMsgClass().  This is extremely useful if you only care to
 |||	    know that a buffer started or finished, but not the specifics of how it
 |||	    did this.
 |||
 |||	    The callback function is required to return a result.  The sound spooler
 |||	    considers a return value >= 0 from the callback function to indicate
 |||	    success.  This value does NOT propogate back to the client.
 |||
 |||	    If the callback returns a value <0, that value is returned to the client
 |||	    through the sound spooler function that called the callback function.  In
 |||	    this case, the sound spooler function terminates immediately.  Any
 |||	    function that can process multiple SoundBufferNodes (e.g. ssplAbort(), or
 |||	    ssplProcessSignals()) doesn't complete its active queue processing when
 |||	    the callback function returns an error code.  If the cause of the callback
 |||	    failure can be resolved by the client, the client can usually call the
 |||	    offending function again to continue where it left off.
 |||
 |||	    Use of the SoundBufferFunc and UserBufferProcessor systems are mutually
 |||	    exclusive.  When a UserBufferProcessor is supplied to a function that
 |||	    supports it and a SoundBufferFunc is installed, that function fails
 |||	    and returns an error code prior to doing anything.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        SoundSpooler.
 |||
 |||	    sbn                         SoundBufferNode changing state.
 |||
 |||	    msg                         A message id (SSPL_SBMSG_...).  See below.
 |||
 |||	  Messages
 |||
 |||	    Start Class
 |||
 |||	    SSPL_SBMSG_INITIAL_START    Initial Start.  ssplStartSpooler() sends this
 |||	                                message for the first buffer in active queue.
 |||
 |||	    SSPL_SBMSG_LINK_START       Link Start.  ssplProcessSignals() sends this
 |||	                                message for the next buffer in the active queue
 |||	                                after it removes a completed buffer.
 |||
 |||	    SSPL_SBMSG_STARVATION_START Starvation Start.  ssplSendBuffer() sends this
 |||	                                message for the buffer sent to it if that buffer
 |||	                                causes the spooler to restart after being starved.
 |||
 |||	    End Class
 |||
 |||	    SSPL_SBMSG_COMPLETE         Complete.  ssplProcessSignals() sends this message
 |||	                                for every completed buffer that it removes from
 |||	                                the active queue.
 |||
 |||	    SSPL_SBMSG_ABORT            Abort.  ssplAbort() sends this message for every
 |||	                                buffer (completed or otherwise) that it removes
 |||	                                from the active queue.
 |||
 |||	  Return Value
 |||
 |||	    Client should return non-negative value on success, or a negative 3DO
 |||	    error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Client-supplied callback function used by music.lib V22.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplSetSoundBufferFunc(), ssplGetSBMsgClass(), ssplStartSpooler(),
 |||	    ssplAbort(), splProcessSignals(), ssplSendBuffer()
**/

/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplgetsbmsgclass
 |||	ssplGetSBMsgClass - Get class of message passed to SoundBufferFunc.
 |||
 |||	  Synopsis
 |||
 |||	    int32 ssplGetSBMsgClass ( int32 msg )
 |||
 |||	  Description
 |||
 |||	    Returns the class of a message passed to SoundBufferFunc.
 |||
 |||	  Arguments
 |||
 |||	    msg                         Message id passed to SoundBufferFunc.
 |||
 |||	  Return Value
 |||
 |||	    One of the SSPL_SBMSGCLASS_... values defined in soundspooler.h.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in soundspooler.h V22.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    (*SoundBufferFunc)()
**/

/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplsetsoundbufferfunc
 |||	ssplSetSoundBufferFunc - Install new SoundBufferFunc in SoundSpooler.
 |||
 |||	  Synopsis
 |||
 |||	    Err ssplSetSoundBufferFunc ( SoundSpooler *sspl,
 |||	                                 SoundBufferFunc func )
 |||
 |||	  Description
 |||
 |||	    Installs a new SoundBufferFunc callback function in a SoundSpooler.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        SoundSpooler to affect.
 |||
 |||	    func                        New callback function pointer to install,
 |||	                                or NULL to disable callback.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success, or negative 3DO error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V22.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    (*SoundBufferFunc)()
**/

Err ssplSetSoundBufferFunc (SoundSpooler *sspl, SoundBufferFunc func)
{
    sspl->sspl_SoundBufferFunc = func;

    return 0;
}

/*************************************************************************
**  CallSoundBufferFunc() - call sspl->sspl_SoundBufferFunc if non-NULL.
*************************************************************************/
static int32 CallSoundBufferFunc (SoundSpooler *sspl, SoundBufferNode *sbn, int32 msg)
{
    return sspl->sspl_SoundBufferFunc
        ? sspl->sspl_SoundBufferFunc (sspl, sbn, msg)
        : 0;
}

/*************************************************************************
**  CheckCallBackCollision() - consider presence of both client-supplied
**  callbacks an error.
*************************************************************************/
#define CheckCallBackCollision(sspl,userbufproc) ((sspl)->sspl_SoundBufferFunc && (userbufproc) != NULL ? ML_ERR_BAD_ARG : 0)


/** (this one's here only because it doesn't really belong anywhere else)
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/userbufferprocessor
 |||	(*UserBufferProcessor)() - Callback function called by
 |||	                           ssplProcessSignals(), ssplAbort(), and
 |||	                           ssplReset().
 |||
 |||	  Synopsis
 |||
 |||	    void (*UserBufferProcessor) ( SoundSpooler *sspl,
 |||	                                  SoundBufferNode *sbn )
 |||
 |||	  Description
 |||
 |||	    This user callback function is called by ssplProcessSignals() for each
 |||	    buffer that has finished playing and ssplAbort() for each buffer that is
 |||	    removed from the active buffer queue.  This function can be used as
 |||	    notification that the sound spooler is done with a given buffer.  For
 |||	    example, if sound is being spooled from a network connection, this
 |||	    function could be used to reply the network packet that contained the
 |||	    sample data for a given buffer.
 |||
 |||	    Using the SoundBufferNode's UserData field can help identify the buffer
 |||	    being passed to this function (for example, UserData could be a pointer to
 |||	    the aforementioned network packet).  See ssplGetUserData().
 |||
 |||	    It is not legal to do anything to disturb the sound spooler's active
 |||	    buffer queue during this function.  In particular, do not call
 |||	    ssplSendBuffer() or anything that might call ssplSendBuffer() as this will
 |||	    confuse the list processor in ssplProcessSignals().
 |||
 |||	    The state of SSPL_STATUS_F_ACTIVE is undefined when inside a UserBufferProcessor
 |||	    function.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        Pointer to the SoundSpooler passed to
 |||	                                ssplProcessSignals() or ssplAbort().
 |||
 |||	    sbn                         Pointer to the completed (or aborted)
 |||	                                SoundBufferNode that was just
 |||	                                removed from the active queue.
 |||
 |||	  Return Value
 |||
 |||	    None
 |||
 |||	  Implementation
 |||
 |||	    Client-supplied callback function used by music.lib V21.
 |||
 |||	  Note
 |||
 |||	    This facility has been superceded by the SoundBufferFunc system.
 |||
 |||	    There are plenty of sound spooler functions in which buffers can be
 |||	    removed from the active queue without any means for the client to provide
 |||	    a UserBufferProcessor function.  This list includes, but is not limited to
 |||	    ssplDeleteSoundSpooler(), ssplAttachInstrument(), ssplDetachInstrument(),
 |||	    and ssplPlayData().  Consider using a SoundBufferFunc instead of a
 |||	    UserBufferProcessor.
 |||
 |||	  See Also
 |||
 |||	    (*SoundBufferFunc)(), ssplProcessSignals(), ssplAbort(), ssplReset()
**/


/* -------------------- Public functions */

/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplpause
 |||	ssplPause - Pauses the SoundSpooler
 |||
 |||	  Synopsis
 |||
 |||	    Err ssplPause ( SoundSpooler *sspl )
 |||
 |||	  Description
 |||
 |||	    ssplPause pauses SoundSpooler's sample player instrument. Stops playback
 |||	    and retains the sample data position so that playback can be resumed at
 |||	    the same position with ssplResume(). This differs from the
 |||	    ssplStartSpooler() ssplStopSpooler() pair which do not retain the current
 |||	    sample playback position.
 |||
 |||	    The playback cannot be resumed from the paused location if
 |||	    ssplStartSpooler() or ssplStopSpooler() are called between ssplPause() and
 |||	    ssplResume(). Multiple calls to ssplPause() have no effect; pause
 |||	    does not nest.
 |||
 |||	    Sets SSPL_STATUS_F_PAUSED.
 |||
 |||	  Argument
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success, or negative 3DO error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplResume(), ssplStopSpooler(), audiofolio/PauseInstrument(),
 |||	    ssplGetSpoolerStatus()
**/

Err ssplPause( SoundSpooler *sspl )
{
    Err errcode;

	if ((errcode = PauseInstrument (sspl->sspl_SamplerIns)) >= 0)
        sspl->sspl_Flags |= SSPL_F_PAUSED;

    return errcode;
}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplresume
 |||	ssplResume - Resume SoundSpooler playback.
 |||
 |||	  Synopsis
 |||
 |||	    Err ssplResume ( SoundSpooler *sspl )
 |||
 |||	  Description
 |||
 |||	    ssplResume resumes sound playback after being paused with ssplPause().
 |||	    Playback is resumed from the sample data position in the spooled sound at
 |||	    which it was paused.
 |||
 |||	    This function has no effect if ssplStartSpooler() or ssplStopSpooler() are
 |||	    called between ssplPause() and ssplResume(). Multiple calls to ssplResume()
 |||	    have no effect; pause does not nest.
 |||
 |||	    Clears SSPL_STATUS_F_PAUSED.
 |||
 |||	  Argument
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success, or negative 3DO error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplPause(), audiofolio/ResumeInstrument(), ssplGetSpoolerStatus()
**/

Err ssplResume( SoundSpooler *sspl )
{
    Err errcode;

	if ((errcode = ResumeInstrument (sspl->sspl_SamplerIns)) >= 0)
        sspl->sspl_Flags &= ~SSPL_F_PAUSED;

    return errcode;
}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplstartspooler
 |||	ssplStartSpooler - Start SoundSpooler.
 |||
 |||	  Synopsis
 |||
 |||	    Err ssplStartSpooler ( SoundSpooler *sspl,
 |||	                           int32 Amplitude )
 |||
 |||	  Description
 |||
 |||	    ssplStartSpooler() starts the sound spooler's sample player instrument. It
 |||	    starts playback at the beginning of the first buffer in the active queue
 |||	    (submitted by ssplSendBuffer()). If there are no buffers in the active
 |||	    queue, playback will not begin until a buffer is submitted with a call to
 |||	    ssplSendBuffer().
 |||
 |||	    If there is a SoundBufferFunc installed and there is a buffer in the
 |||	    active queue, sends a SSPL_SBMSG_INITIAL_START message with a pointer to
 |||	    the first active buffer. If SoundBufferFunc returns an error code,
 |||	    ssplStartSpooler() terminates and returns that error code.
 |||
 |||	    This function sets SSPL_STATUS_F_STARTED and clears SSPL_STATUS_F_PAUSED.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||
 |||	    Amplitude                   A value of 0 to 0x7FFF that controls the
 |||	                                loudness.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success, or negative 3DO error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplStartInstrumentTags(), ssplStopSpooler(), ssplPause(), ssplSendBuffer(),
 |||	    (*SoundBufferFunc)(), audiofolio/StartInstrument(), ssplGetSpoolerStatus()
**/

Err ssplStartSpooler (SoundSpooler *sspl, int32 Amplitude)
{
    return ssplStartSpoolerTagsVA ( sspl,
                                    AF_TAG_AMPLITUDE, Amplitude,
                                    TAG_END );
}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplstartspoolertags
 |||	ssplStartSpoolerTags - Start SoundSpooler with tags.
 |||
 |||	  Synopsis
 |||
 |||	    Err ssplStartSpoolerTags ( SoundSpooler *sspl,
 |||	                               const TagArg *samplertagsint32 )
 |||
 |||	    Err ssplStartSpoolerTagsVA ( SoundSpooler *sspl,
 |||	                                 uint32 tag1, ... )
 |||
 |||	  Description
 |||
 |||	    ssplStartSpooler() starts the sound spooler's sample player instrument. It
 |||	    starts playback at the beginning of the first buffer in the active queue
 |||	    (submitted by ssplSendBuffer()). If there are no buffers in the active
 |||	    queue, playback will not begin until a buffer is submitted with a call to
 |||	    ssplSendBuffer().
 |||
 |||	    If there is a SoundBufferFunc installed and there is a buffer in the
 |||	    active queue, sends a SSPL_SBMSG_INITIAL_START message with a pointer to
 |||	    the first active buffer. If SoundBufferFunc returns an error code,
 |||	    ssplStartSpooler() terminates and returns that error code.
 |||
 |||	    This function sets SSPL_STATUS_F_STARTED and clears SSPL_STATUS_F_PAUSED.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||
 |||	    samplertags                 Tags to be passed to StartInstrument() for
 |||	                                sample player instrument. Useful tags are
 |||	                                AF_TAG_AMPLITUDE and AF_TAG_RATE. Can be
 |||	                                NULL.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success, or negative 3DO error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplStopSpooler(), ssplPause(), ssplSendBuffer(), (*SoundBufferFunc)(),
 |||	    audiofolio/StartInstrument(), ssplGetSpoolerStatus()
**/
VAGLUE_FUNC (Err,
             ssplStartSpoolerTagsVA (SoundSpooler *sspl, VAGLUE_VA_TAGS),
             ssplStartSpoolerTags (sspl, VAGLUE_TAG_POINTER) )
Err ssplStartSpoolerTags (SoundSpooler *sspl, const TagArg *samplertags)
{
	int32 Result = 0;
	SoundBufferNode *sbn;

/* start instrument (also has side effect of stopping any currently running attachment */
	Result = StartInstrument( sspl->sspl_SamplerIns, (TagArg *)samplertags );       /* !!! shouldn't need this cast */
	CHECKRESULT( Result, "ssplStartSpooler: StartInstrument");
	sspl->sspl_Flags |= SSPL_F_STARTED;
	sspl->sspl_Flags &= ~SSPL_F_PAUSED;

/* Clear signals for all buffers in active queue (940509).
   This must be done between StartInstrument() and StartAttachment()
   since this is the only time when there is guaranteed to be no
   buffer playing. */
    SCANLIST (&sspl->sspl_ActiveBuffers, sbn, SoundBufferNode) {
		ssplClearBufferSignal( sspl, sbn );
    }

/* Start first attachment in active queue, if there is one. */
	sbn = (SoundBufferNode *) FirstNode( &sspl->sspl_ActiveBuffers );
	if( IsNode( &sspl->sspl_ActiveBuffers, (Node *) sbn) )
	{
		Result = StartAttachment( sbn->sbn_Attachment, NULL ); /* 940421 */
		CHECKRESULT( Result, "ssplStartSpooler: StartAttachment");

		Result = CallSoundBufferFunc (sspl, sbn, SSPL_SBMSG_INITIAL_START);
		CHECKRESULT (Result, "ssplStartSpooler: (*SoundBufferFunc)(SSPL_SBMSG_INITIAL_START)");
	}

cleanup:
	return Result;
}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplstopspooler
 |||	ssplStopSpooler - Stop SoundSpooler.
 |||
 |||	  Synopsis
 |||
 |||	    Err ssplStopSpooler ( SoundSpooler *sspl )
 |||
 |||	  Description
 |||
 |||	    ssplStopSpooler stops the SoundSpooler's sample player instrument. Unlike
 |||	    ssplPause(), this call does not retain the current sample data position.
 |||	    Starting the spooler after stopping it will start sound playback from the
 |||	    beginning of the first buffer in the active queue.
 |||
 |||	    This function merely stops the SoundSpooler's sample player instrument. It
 |||	    does not disturb the active queue.
 |||
 |||	    This function clears SSPL_STATUS_F_STARTED and SSPL_STATUS_F_PAUSED.
 |||
 |||	  Argument
 |||
 |||	    sspl                        pointer to a SoundSpooler structure.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success, or negative 3DO error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplStartSpooler(), ssplPause(), ssplAbort(), audiofolio/StopInstrument(),
 |||	    ssplGetSpoolerStatus()
**/

Err ssplStopSpooler( SoundSpooler *sspl )
{
	int32 Result;

	Result = 0;

	if(sspl->sspl_SamplerIns)
	{
		Result = StopInstrument( sspl->sspl_SamplerIns, NULL );
		CHECKRESULT( Result, "ssplStopSpooler: StopInstrument");
	}
	sspl->sspl_Flags &= ~(SSPL_F_STARTED | SSPL_F_PAUSED);

cleanup:
	return Result;
}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplreset
 |||	ssplReset - Reset Sound Spooler.
 |||
 |||	  Synopsis
 |||
 |||	    Err ssplReset ( SoundSpooler *sspl,
 |||	                    void (*UserBufferProcessor) ( SoundSpooler *,
 |||	                                                  SoundBufferNode * ) )
 |||
 |||	  Description
 |||
 |||	    This function removes all SoundBufferNodes from the active queue
 |||	    and clears their associated signal bits.  ssplAbort() calls this
 |||	    function to clear the active queue after stopping the sound spooler.
 |||
 |||	    If a SoundBufferFunc is installed, a SSPL_SBMSG_ABORT message is sent to
 |||	    it with each SoundBufferNode removed from the active queue.  If
 |||	    UserBufferProcessor is non-NULL, each SoundBufferNode removed from the
 |||	    active queue is passed to it.  If both a SoundBufferFunc and
 |||	    UserBufferProcessor are supplied, ssplReset() fails and returns
 |||	    ML_ERR_BAD_ARG.
 |||
 |||	    If SoundBufferFunc fails, ssplReset() fails immediately and returns the
 |||	    error code returend by SoundBufferFunc without removing the rest of the
 |||	    buffers from the active queue.  Calling ssplReset() again safely picks
 |||	    up where it left off.
 |||
 |||	    Clears SSPL_STATUS_F_ACTIVE.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        SoundSpooler to reset.
 |||
 |||	    UserBufferProcessor         Pointer to a function to call with
 |||	                                each SoundBufferNode removed from the
 |||	                                active queue, or NULL.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success, or negative 3DO error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Note
 |||
 |||	    This function does not disturb the sample player instrument.  If the
 |||	    spooler has not been stopped, any data that has already been queued
 |||	    up will continue to play, even though the associated SoundBufferNodes
 |||	    have been removed from the active queue.  It is generally a good idea
 |||	    to call this function only after stopping the sound spooler, or use
 |||	    ssplAbort() instead.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplAbort(), (*SoundBufferFunc)(), (*UserBufferProcessor)(), ssplGetSpoolerStatus()
**/

Err ssplReset (SoundSpooler *sspl, void (*UserBufferProcessor)(SoundSpooler *sspl, SoundBufferNode *sbn))
{
	SoundBufferNode *sbn, *next;
	int32 Result = 0;

        /* Check for collision between sspl_SoundBufferFunc and UserBufferProcessor */
	Result = CheckCallBackCollision (sspl, UserBufferProcessor);
	CHECKRESULT (Result, "ssplReset: Client supplied both callback functions");

        /* Move active buffers to free list. */
    PROCESSLIST (&sspl->sspl_ActiveBuffers, sbn, next, SoundBufferNode) {
		    /* move buffer to free list and clear its signal */
		ssplBufferToFreeList( sspl, sbn );
		ssplClearBufferSignal( sspl, sbn );  /* 940418 */

		    /* call callbacks */
        Result = CallSoundBufferFunc (sspl, sbn, SSPL_SBMSG_ABORT);
        CHECKRESULT (Result, "ssplReset: (*SoundBufferFunc)(SSPL_SBMSG_ABORT)");
		if(UserBufferProcessor)
		{
			(*UserBufferProcessor)( sspl, sbn );
		}
	}

        /* Move requested buffers to free list. */
    PROCESSLIST (&sspl->sspl_RequestedBuffers, sbn, next, SoundBufferNode) {
		ssplBufferToFreeList( sspl, sbn );
	}

cleanup:
	return Result;
}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplabort
 |||	ssplAbort - Abort SoundSpooler buffers in active queue.
 |||
 |||	  Synopsis
 |||
 |||	    Err ssplAbort ( SoundSpooler *sspl, void
 |||	                    (*UserBufferProcessor) ( SoundSpooler *,
 |||	                                             SoundBufferNode * ) )
 |||
 |||	  Description
 |||
 |||	    ssplAbort stops the spooler (by calling ssplStopSpooler()) and removes all
 |||	    buffers in the active queue (by calling ssplReset()), including the one
 |||	    currently being played.
 |||
 |||	    If a SoundBufferFunc is installed, a SSPL_SBMSG_ABORT message is sent to
 |||	    it with each SoundBufferNode removed from the active queue.  If
 |||	    UserBufferProcessor is non-NULL, each SoundBufferNode removed from the
 |||	    active queue is passed to it.  If both a SoundBufferFunc and
 |||	    UserBufferProcessor are supplied, ssplAbort() fails and returns
 |||	    ML_ERR_BAD_ARG.
 |||
 |||	    If SoundBufferFunc fails, ssplAbort() fails immediately and returns the
 |||	    error code returned by SoundBufferFunc without removing the rest of the
 |||	    buffers from the active queue.  Calling ssplAbort() again safely picks
 |||	    up where it left off.
 |||
 |||	    Clears SSPL_STATUS_F_STARED, SSPL_STATUS_F_PAUSED, and SSPL_STATUS_F_ACTIVE.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        SoundSpooler to abort.
 |||
 |||	    UserBufferProcessor         Pointer to a function to call with
 |||	                                each SoundBufferNode removed from the
 |||	                                active queue, or NULL.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success, or negative 3DO error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplStopSpooler(), ssplSendBuffer(), ssplProcessSignals(),
 |||	    ssplReset(), (*SoundBufferFunc)(), (*UserBufferProcessor)()
 |||	    ssplGetSpoolerStatus()
**/

Err ssplAbort( SoundSpooler *sspl,
				void (*UserBufferProcessor)( SoundSpooler *sspl, SoundBufferNode *sbn ) )
{
	int32 Result;

	Result = ssplStopSpooler( sspl );
	if (Result < 0) return Result;

	return ssplReset( sspl, UserBufferProcessor );
}


/**
 |||	AUTODOC INTERNAL mpg/musiclib/soundspooler/sspldeletesoundbuffernode
 |||	ssplDeleteSoundBufferNode - Delete a SoundBufferNode from a SoundSpooler.
 |||
 |||	  Synopsis
 |||
 |||	    Err ssplDeleteSoundBufferNode ( SoundSpooler *sspl,
 |||	                                    SoundBufferNode *sbn )
 |||
 |||	  Description
 |||
 |||	    This low level function deletes the speicified SoundBufferNodes belonging
 |||	    to a SoundSpooler.  This can be used to dynamically modify the number of
 |||	    buffers initially set by ssplCreateSoundSpooler().
 |||
 |||	  Arguments
 |||
 |||	    sspl                        Pointer to SoundSpooler owning
 |||	                                SoundBufferNode to remove.
 |||
 |||	    sbn                         Pointer to SoundBufferNode to delete.
 |||	                                Can be NULL.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success, or negative 3DO error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Class
 |||
 |||	    Low level
 |||
 |||	  Note
 |||
 |||	    This function can cause interruption in sound playback if called while
 |||	    spooler is running.
 |||
 |||	  Caveats
 |||
 |||	    This function doesn't remove the SoundBufferNode's signal bit from the
 |||	    composite signal mask in the SoundSpooler (fixed in music.lib V22).
 |||
 |||	    Because you can specify SoundBufferNodes out of sequence, the sequence
 |||	    number for remaining SoundBufferNodes may be greater than NumBuffers-1.
 |||	    Also, SoundBufferNodes created with ssplCreateSoundBufferNode() after
 |||	    calls to ssplDeleteSoundBufferNode() can create SoundBufferNodes with
 |||	    duplicate sequence numbers.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplCreateSoundBufferNode()
**/

static Err ssplDeleteSoundBufferNode(  SoundSpooler *sspl, SoundBufferNode *sbn )
{
    if (sbn) {
            /* decouple it from SoundSpooler */
        ssplRemoveBufferFromList( sbn );
        sspl->sspl_NumBuffers--;
        sspl->sspl_SignalMask &= ~sbn->sbn_Signal;

            /* free it */
        DetachSample( sbn->sbn_Attachment );  /* 940418 */
        DeleteCue( sbn->sbn_Cue );
        UnloadSample( sbn->sbn_Sample );
        FreeMem( sbn, sizeof(SoundBufferNode) );
    }

	return 0;
}


/*************************************************************************
** Attach buffer to this instrument.
*************************************************************************/
static Err ssplAttachBufferToInstrument( SoundSpooler *sspl, SoundBufferNode *sbn, Item SamplerIns )
{
	int32  Result;

/* Attach the sample to the instrument. */
/* @@@ it'd be nice if there were a CreateAttachmentVA() or AttachSampleTagsVA() */
	sbn->sbn_Attachment = AttachSample (SamplerIns, sbn->sbn_Sample, NULL);
	CHECKRESULT(sbn->sbn_Attachment,"ssplCreateSoundBufferNode: AttachSample");

/* Make sure attachments don't start automatically when StartInstrument() is called */
	Result = SetAudioItemInfoVA (sbn->sbn_Attachment,
                                 AF_TAG_SET_FLAGS, AF_ATTF_NOAUTOSTART,
                                 TAG_END);
	CHECKRESULT(Result,"SetAudioItemInfo");

/* Request a signal when the sample ends. */
	Result = MonitorAttachment( sbn->sbn_Attachment, sbn->sbn_Cue, CUE_AT_END );
	CHECKRESULT(Result,"MonitorAttachment");

cleanup:
	return Result;
}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplattachinstrument
 |||	ssplAttachInstrument - Attach new sample player instrument to SoundSpooler.
 |||
 |||	  Synopsis
 |||
 |||	    Err ssplAttachInstrument ( SoundSpooler *sspl,
 |||	                               Item SamplerIns )
 |||
 |||	  Description
 |||
 |||	    ssplAttachInstrument() installs a new sample player instrument in the
 |||	    SoundSpooler.  Any previously attached sample player instrument is detached
 |||	    (by calling ssplDetachInstrument()) before the new one is attached.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        pointer to a SoundSpooler structure.
 |||
 |||	    SamplerIns                  Instrument to attach buffer samples to.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success, or negative 3DO error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Class
 |||
 |||	    Low level
 |||
 |||	  Note
 |||
 |||	    Any buffers in the active queue are aborted by calling ssplAbort().  Note
 |||	    that there is no provision to supply a UserBufferProcessor function to
 |||	    ssplAttachInstrument().  An installed SoundBufferFunc works correctly,
 |||	    however.
 |||
 |||	  Caveats
 |||
 |||	    Leaves the SoundSpooler in an inderminate state if this function fails.
 |||	    A subsequent successful call to this function or to ssplDetachInstrument()
 |||	    restores the SoundSpooler to sanity.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplDetachInstrument(), ssplAbort()
**/

Err ssplAttachInstrument( SoundSpooler *sspl, Item SamplerIns )
{
	int32 Result;
	SoundBufferNode *sbn;

	Result = 0;

	if( sspl->sspl_SamplerIns )
	{
		ssplDetachInstrument( sspl );
	}

/* Attach to all buffers */
        /*
            @@@ This stuff could be split out into a function that ssplCreateSoundSpooler() could
                call.  this would reduce memory usage for apps that don't need the detach and
                abort code.  Or ssplCreateSoundSpooler() could simply call ssplAttachBufferToInstrument()
                directly.
            @@@ Note that this depends on all buffers being in the Free list -
                true as long as ssplDetachInstrument() actually gets called and as long as it
                actually calls ssplAbort()s
        */
	sbn = (SoundBufferNode *) FirstNode( &sspl->sspl_FreeBuffers );
	while (ISNODE( &sspl->sspl_FreeBuffers, (Node *) sbn))
	{
		Result = ssplAttachBufferToInstrument( sspl, sbn, SamplerIns );
		if( Result < 0 ) return Result;
		sbn = (SoundBufferNode *) NextNode((Node *) sbn);
	}
	sspl->sspl_SamplerIns = SamplerIns;

	return Result;

}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/sspldetachinstrument
 |||	ssplDetachInstrument - Detach the current sample player instrument from
 |||	                       the SoundSpooler.
 |||
 |||	  Synopsis
 |||
 |||	    Err ssplDetachInstrument ( SoundSpooler *sspl )
 |||
 |||	  Description
 |||
 |||	    ssplDetachInstrument removes the current sample player instrument from the
 |||	    SoundSpooler.  Any buffers in the active queue are aborted (calls
 |||	    ssplAbort()).  Spooler playback cannot be started until a new instrument
 |||	    is attached with ssplAttachInstrument().  Incidentally,
 |||	    ssplAttachInstrument() calls this function prior to attaching a new
 |||	    instrument, so it's pretty unlikely that you actually need to call this
 |||	    function directly.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success, or negative 3DO error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Class
 |||
 |||	    Low level
 |||
 |||	  Note
 |||
 |||	    Any buffers in the active queue are aborted by calling ssplAbort().  Note
 |||	    that there is no provision to supply a UserBufferProcessor function to
 |||	    ssplAttachInstrument().  An installed SoundBufferFunc works correctly,
 |||	    however.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplAttachInstrument(), ssplAbort()
**/

Err ssplDetachInstrument( SoundSpooler *sspl )
{
	int32 Result;
	SoundBufferNode *sbn;

	Result = 0;

	if( sspl->sspl_SamplerIns )
	{
		ssplAbort( sspl, NULL );

/* Detach from all buffers */
            /*
                @@@ This assumes that ssplAbort() was called in order for all
                    the SBN's to be in the free list.
            */
		sbn = (SoundBufferNode *) FirstNode( &sspl->sspl_FreeBuffers );
		while (ISNODE( &sspl->sspl_FreeBuffers, (Node *) sbn))
		{
			DetachSample( sbn->sbn_Attachment );  /* 940418 */
			sbn = (SoundBufferNode *) NextNode((Node *) sbn);
		}
		sspl->sspl_SamplerIns = 0;
	}

	return Result;

}

/*************************************************************************
** Clear signal associated with this buffer.
*************************************************************************/
static void ssplClearBufferSignal( SoundSpooler *sspl, SoundBufferNode *sbn )
{
    int32 signals;

        /* Clear any old signals. */
    signals = ClearCurrentSignals (sbn->sbn_Signal);

    if (signals) DBUG(("ssplClearBufferSignal: old signals cleared = 0x%08x\n", signals));
}


/**
 |||	AUTODOC INTERNAL mpg/musiclib/soundspooler/ssplcreatesoundbuffernode
 |||	ssplCreateSoundBufferNode - Add a new SoundBufferNode to a SoundSpooler.
 |||
 |||	  Synopsis
 |||
 |||	    SoundBufferNode *ssplCreateSoundBufferNode ( SoundSpooler *sspl )
 |||
 |||	  Description
 |||
 |||	    This low level function creates and adds a new SoundBufferNode structure
 |||	    to the SoundSpooler's buffer list. This can be used to dynamically modify
 |||	    the number of buffers initially set by ssplCreateSoundSpooler().
 |||
 |||	  Arguments
 |||
 |||	    sspl                        SoundSpooler to add new buffer to.
 |||
 |||	  Return Value
 |||
 |||	    Pointer to the newly added SoundBufferNode on success, or NULL on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Note
 |||
 |||	    You must attach the sample player instrument to the newly created
 |||	    SoundBufferNode by calling ssplAttachInstrument() before actually
 |||	    using it.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplDeleteSoundBufferNode()
**/

static SoundBufferNode *ssplCreateSoundBufferNode( SoundSpooler *sspl )
{
	int32 Result;
	SoundBufferNode *sbn;

	sbn = (SoundBufferNode *) AllocMem(sizeof(SoundBufferNode), MEMTYPE_FILL);
	if (sbn == NULL) return NULL;

/* Increment buffer count.  Have to do this first because
   ssplDeleteSoundBufferNode() decrements this unconditionally */
	sspl->sspl_NumBuffers++;

/* Make generic sample.  It doesn't really matter what the format is. */
	sbn->sbn_Sample = CreateSample(NULL);
	CHECKRESULT( sbn->sbn_Sample,"CreateSample");

/* Create a Cue for signalback */
	sbn->sbn_Cue = CreateCue( NULL );
	CHECKRESULT(sbn->sbn_Cue, "CreateCue");

/* Get signals from Cues so we can call WaitSignal() */
	sbn->sbn_Signal = GetCueSignal( sbn->sbn_Cue );
	sspl->sspl_SignalMask |= sbn->sbn_Signal;
	ssplClearBufferSignal( sspl, sbn );

/*	Give this buffer a sequence number, which just happens to be the
**	current buffer count. 940202 (or it would have been if we didn't have
**  to increment it first. 940531) */
	sbn->sbn_SequenceNum = sspl->sspl_NumBuffers-1;

/* Add to list of Free Buffers */
	ssplBufferToFreeList( sspl, sbn );

	return sbn;

cleanup:
	ssplDeleteSoundBufferNode( sspl, sbn );
	return NULL;
}


/* delete all SoundBufferNodes in a list */
static void DeleteSoundBufferList (SoundSpooler *sspl, List *sbnlist)
{
	SoundBufferNode *sbn, *next;

	PROCESSLIST (sbnlist, sbn, next, SoundBufferNode) {
        ssplDeleteSoundBufferNode (sspl, sbn);
    }
}

/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/sspldeletesoundspooler
 |||	ssplDeleteSoundSpooler - Delete a SoundSpooler.
 |||
 |||	  Synopsis
 |||
 |||	    Err ssplDeleteSoundSpooler ( SoundSpooler *sspl )
 |||
 |||	  Description
 |||
 |||	    ssplDeleteSoundSpooler disposes of a SoundSpooler created by
 |||	    ssplCreateSoundSpooler() after first stopping it with ssplStopSpooler().
 |||	    All SoundBufferNodes associated with the SoundSpooler are deleted.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||	                                Starting with music.lib v22, can be NULL.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success, or negative 3DO error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Caveats
 |||
 |||	    Doesn't tolerate a NULL sspl pointer prior to music.lib v22.
 |||
 |||	  Note
 |||
 |||	    This function does no notification of SoundBufferNode removal from
 |||	    the active queue. If this is an issue, call ssplAbort() prior to
 |||	    calling this function.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplCreateSoundSpooler(), ssplStopSoundSpooler()
**/

Err ssplDeleteSoundSpooler (SoundSpooler *sspl)
{
    Err errcode = 0;

	if (sspl) {
            /* Stop playback (quickie method, no need to abort here) */
        errcode = ssplStopSpooler (sspl);

            /* Delete SoundBufferNodes */
        DeleteSoundBufferList (sspl, &sspl->sspl_FreeBuffers);
        DeleteSoundBufferList (sspl, &sspl->sspl_ActiveBuffers);
        DeleteSoundBufferList (sspl, &sspl->sspl_RequestedBuffers);

            /* free spooler memory */
        FreeMem (sspl, sizeof *sspl);
    }

    return errcode;
}

/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplcreatesoundspooler
 |||	ssplCreateSoundSpooler - Create a SoundSpooler.
 |||
 |||	  Synopsis
 |||
 |||	    SoundSpooler *ssplCreateSoundSpooler ( int32 NumBuffers,
 |||	                                           Item SamplerIns )
 |||
 |||	  Description
 |||
 |||	    ssplCreateSoundSpooler creates a SoundSpooler data structure and
 |||	    initializes it.  Call ssplCreateSoundSpooler first to create a
 |||	    SoundSpooler data structure.
 |||
 |||	    This function allocates the requested number of SoundBufferNodes for the
 |||	    SoundSpooler.  SoundBufferNodes simply carry pointers to sample data; they
 |||	    do not have any embedded sample data.  The pointer and length of each
 |||	    sample buffer is stored into a SoundBufferNode with
 |||	    ssplSetBufferAddressLength() prior to enqueuing it with ssplSendBuffer().
 |||
 |||	  Arguments
 |||
 |||	    NumBuffers                  Specifies the number of buffers.  For
 |||	                                double buffering of audio data, use
 |||	                                NumBuffers=2. The more buffers you
 |||	                                use, the less chance you will have of
 |||	                                running out of data. A safe number to
 |||	                                use is 4-8.
 |||
 |||	    SamplerIns                  The DSP instrument that will play the
 |||	                                data.  For  example, you could use a
 |||	                                fixedstereosample.dsp  instrument to
 |||	                                play a 44.1 kHz, 16-bit, stereo
 |||	                                soundfile.  You can call ScanSample()
 |||	                                and SelectSamplePlayer() to determine
 |||	                                the best instrument to use for a
 |||	                                given sample format.  You will need
 |||	                                to connect this instrument to a mixer
 |||	                                or a direct out.dsp to hear the sound.
 |||
 |||	                                This setting can be changed after creation
 |||	                                by using ssplAttachInstrument().  Also,
 |||	                                this can be 0, if you call ssplAttachInstrument()
 |||	                                before actually starting the spooler.
 |||
 |||	  Return Value
 |||
 |||	    Pointer to new SoundSpooler on success, or NULL on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Caveats
 |||
 |||	    If this function fails, it messes up subsequent sequence numbering.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplDeleteSoundSpooler(), ssplStartSoundSpooler(), ssplSendBuffer(),
 |||	    ssplProcessSignals()
**/

/** removed documentation for NumBuffers:
 |||	                                This number can be changed after creation
 |||	                                by using ssplCreateSoundBufferNode() and
 |||	                                ssplDeleteSoundBufferNode().
**/

SoundSpooler *ssplCreateSoundSpooler( int32 NumBuffers, Item SamplerIns )
{
	SoundSpooler *sspl;
	int32 i, Result;
	SoundBufferNode *sbn;

    PULL_MUSICLIB_PACKAGE_ID(soundspooler);

	sspl = (SoundSpooler *) AllocMem(sizeof(SoundSpooler), MEMTYPE_FILL);
	if (sspl == NULL) return NULL;

/* Initialise lists that hold buffers. */
	InitList( &sspl->sspl_FreeBuffers,      "Free Buffers" );
	InitList( &sspl->sspl_ActiveBuffers,    "Active Buffers" );
	InitList( &sspl->sspl_RequestedBuffers, "Requested Buffers" );

/* Create desired buffers and add them to Free list. */
	for (i=0; i<NumBuffers; i++)
	{
		sbn = ssplCreateSoundBufferNode( sspl );
		if (sbn == NULL) goto error;
	}

	if( SamplerIns )
	{
		Result = ssplAttachInstrument( sspl, SamplerIns );
		if( Result < 0 ) goto error;
	}

	return sspl;

error:
	ssplDeleteSoundSpooler( sspl );
	return NULL;
}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplprocesssignals
 |||	ssplProcessSignals - Process completion signals that have been received.
 |||
 |||	  Synopsis
 |||
 |||	    int32 ssplProcessSignals ( SoundSpooler *sspl,
 |||	                               int32 SignalMask,
 |||	                               void (*UserBufferProcessor) ( SoundSpooler *,
 |||	                                                             SoundBufferNode * ) )
 |||
 |||	  Description
 |||
 |||	    ssplProcessSignals informs the spooler about which buffers have finished,
 |||	    so the spooler can move them to the free buffer queue.
 |||
 |||	    If a SoundBufferFunc in installed, a SSPL_SBMSG_COMPLETE message is sent
 |||	    to it with each completed SoundBufferNode removed from the active queue.
 |||	    Also, a SSPL_SBMSG_LINK_START message is sent with the next buffer in the
 |||	    active queue.  If UserBufferProcessor is non-NULL, each completed
 |||	    SoundBufferNode removed from the active queue is passed to it. If both a
 |||	    SoundBufferFunc and UserBufferProcessor are supplied, ssplProcessSignals()
 |||	    fails and returns ML_ERR_BAD_ARG.
 |||
 |||	    In SoundBufferFunc fails, ssplProcessSignals() fails immediately and
 |||	    returns the error code returned by SoundBufferFunc without processing the
 |||	    rest of the signals.  Calling ssplProcessSignals() again with the same
 |||	    signal set picks up where it left off.
 |||
 |||	    Clears SSPL_STATUS_F_ACTIVE when all of the active buffers have completed
 |||	    (i.e. the flag is cleared when this function has processed the completion signals for all of the
 |||	    buffers that were in the active queue at the time that this function was called).
 |||	    You can use this to detect when the last submitted buffer has finished.
 |||
 |||	  Arguments
 |||
 |||	    sspl                    Pointer to a SoundSpooler structure.
 |||
 |||	    SignalMask              Signals received from WaitSignal().
 |||
 |||	    UserBufferProcessor     Function for each completed SoundBufferNode,
 |||	                            or NULL.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success indicating number of completed buffers
 |||	    removed from the active queue, or negative 3DO error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  Notes
 |||
 |||	    ssplProcessSignals() synchronizes the sound spooler's queues to the set of
 |||	    signals collected at the last WaitSignal(). Many of the other sound
 |||	    spooler functions assume that the sound spooler has been synchronized in
 |||	    this way (in particular ssplSendBuffer()). Do not call any sound spooler
 |||	    functions between a WaitSignal() involving sound spooler signals and
 |||	    ssplProcessSignals().
 |||
 |||	  Caveats
 |||
 |||	    In the case where SoundBufferFunc(SSPL_SBMSG_COMPLETE) returns an error code,
 |||	    SoundBufferFunc(SSPL_SBMSG_LINK_START) isn't called for the next buffer in
 |||	    the active queue, because processing terminates immediately on receipt of
 |||	    of the failure of SoundBufferFunc(SSPL_SBMSG_COMPLETE) even when ssplProcessSignals()
 |||	    is called again.
 |||
 |||	  See Also
 |||
 |||	    ssplSendBuffer(), ssplAbort(), (*SoundBufferFunc)(), (*UserBufferProcessor)(),
 |||	    ssplGetSpoolerStatus()
**/

int32 ssplProcessSignals( SoundSpooler *sspl, int32 SignalMask,
	void (*UserBufferProcessor)( SoundSpooler *sspl, SoundBufferNode *sbn ) )
{
	SoundBufferNode *sbn, *next;
	int32 NumBuffers;
	int32 Result = 0;
  #if DEBUG_CheckCompletionOrder
    SoundBufferNode *checksbn;
  #endif

DBUG(("ssplProcessSignals( ..., 0x%08x )\n", SignalMask ));
	NumBuffers = 0;

/* Check for collision between sspl_SoundBufferFunc and UserBufferProcessor */
	Result = CheckCallBackCollision (sspl, UserBufferProcessor);
	CHECKRESULT (Result, "ssplProcessSignals: Client supplied both callback functions");

/* Scan for buffers with matching signal, move to free list. */
	sbn = (SoundBufferNode *) FirstNode( &sspl->sspl_ActiveBuffers );
  #if DEBUG_CheckCompletionOrder
    checksbn = sbn;
  #endif
	while (ISNODE( &sspl->sspl_ActiveBuffers, (Node *) sbn))
	{
DBUG(("ssplProcessSignals: sbn->sbn_Signal = 0x%08x\n", sbn->sbn_Signal ));
		next = (SoundBufferNode *) NextNode((Node *) sbn);

            /* process buffer is signal is set */
		if( sbn->sbn_Signal & SignalMask )
		{
		  #if DEBUG_CheckCompletionOrder
		    if (checksbn != sbn) {
                printf ("sspl: didn't completed\n");
                for (; checksbn != sbn; checksbn = (SoundBufferNode *)NextNode ((Node *)checksbn)) ssplDumpSoundBufferNode (checksbn);
            }
		    checksbn = next;
		  #endif

                /* move buffer to free list, increment count */
			ssplBufferToFreeList( sspl, sbn );
			NumBuffers++;

                /* call callbacks for completion of current attachment */
            Result = CallSoundBufferFunc (sspl, sbn, SSPL_SBMSG_COMPLETE);
            CHECKRESULT (Result, "ssplProcessSignals: (*SoundBufferFunc)(SSPL_SBMSG_COMPLETE)");
			if(UserBufferProcessor)
			{
				(*UserBufferProcessor)( sspl, sbn );
			}

                /* call callback for start of next attachment */
            if (ISNODE (&sspl->sspl_ActiveBuffers, (Node *)next)) {
                Result = CallSoundBufferFunc (sspl, next, SSPL_SBMSG_LINK_START);
                CHECKRESULT (Result, "ssplProcessSignals: (*SoundBufferFunc)(SSPL_SBMSG_LINK_START)");
            }
		}

		sbn = next;
	}
	Result = NumBuffers;

cleanup:
DBUG(("ssplProcessSignals finished (Result=%ld)\n",Result ));
	return Result;
}


 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplrequestbuffer
 |||	ssplRequestBuffer - Asks for an available buffer.
 |||
 |||	  Synopsis
 |||
 |||	    SoundBufferNode *ssplRequestBuffer ( SoundSpooler *sspl )
 |||
 |||	  Description
 |||
 |||	    ssplRequestBuffer returns an available SoundBufferNode from the free queue
 |||	    or NULL if none are available (all are in the active queue). When an
 |||	    available SoundBufferNode is returned, the client can attach sample data
 |||	    to it (by calling ssplSetBufferAddressLength()) and submit it to the
 |||	    active queue (by calling ssplSendBuffer()). This procedure is
 |||	    automatically done for you by the convenience functions ssplSpoolData()
 |||	    and ssplPlayData().
 |||
 |||	    This function gives you write permission to the returned SoundBufferNode.
 |||	    You have that write permission until the SoundBufferNode is returned to the
 |||	    SoundSpooler with ssplSendBuffer() or ssplUnrequestBuffer().
 |||
 |||	  Argument
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||
 |||	  Return Value
 |||
 |||	    Pointer to a SoundBufferNode if one is available, or NULL if no buffers
 |||	    are available.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Examples
 |||
 |||	    The following code fragment shows proper use of ssplRequestBuffer(),
 |||	    ssplUnrequestBuffer(), ssplSetBufferAddressLength(), and
 |||	    ssplSendBuffer().
 |||
 |||	    Err sendbuffer (SoundSpooler *sspl)
 |||	    {
 |||	        SoundBufferNode *sbn;
 |||	        Err errcode;
 |||
 |||	            // request a buffer
 |||	        if ((sbn = ssplRequestBuffer (sspl)) != NULL) {
 |||
 |||	                // fill buffer, on failure return unused buffer
 |||	                // to free queue
 |||	            if ((errcode = fillbuffer (sspl, sbn)) < 0) {
 |||	                ssplUnrequestBufffer (sspl, sbn);
 |||	                return errcode;
 |||	            }
 |||
 |||	                // on success, send buffer (always moves buffer to
 |||	                // either free or active queue - no need to unrequest it)
 |||	            if ((errcode = ssplSendBuffer (sspl, sbn)) < 0) return errcode;
 |||	        }
 |||
 |||	        return 0;
 |||	    }
 |||
 |||	    Err fillbuffer (SoundSpooler *sspl, SoundBufferNode *sbn)
 |||	    {
 |||	        void *addr;
 |||	        uint32 len;
 |||
 |||	        // your code here to fill buffer - results in addr, len
 |||
 |||	        return ssplSetBufferAddressLength (sspl, sbn, addr, len);
 |||	    }
 |||
 |||	  Notes
 |||
 |||	    Prior to music.lib V24, SoundBufferNodes were actually _removed_ from the
 |||	    free list; all knowledge of them was lost by the sound spooler until they were
 |||	    passed back to ssplSendBuffer() to put them back into one of the SoundSpooler's
 |||	    queues. If lost, ssplDeleteSoundSpooler() would lose memory and be unable to
 |||	    free some items.
 |||
 |||	    Starting with V24, the sound spooler automatically tracks all sound buffer
 |||	    nodes requested by ssplRequestBuffer() in a "requested" queue. Any buffers
 |||	    that are not submitted or "unrequested", get put back into the free list
 |||	    by ssplReset() and freed by ssplDeleteSoundSpooler().
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplUnrequestBuffer(), ssplSendBuffer(), ssplSetBufferAddressLength(),
 |||	    ssplSpoolData(), ssplPlayData()
 **/

SoundBufferNode *ssplRequestBuffer( SoundSpooler *sspl )
{
	SoundBufferNode *sbn;

	sbn = (SoundBufferNode *) FirstNode(&sspl->sspl_FreeBuffers);

	if ( IsNode( &sspl->sspl_FreeBuffers, (Node *) sbn) )
	{
	    ssplBufferToRequestedList (sspl, sbn);
		return sbn;
	}
	else
	{
		return NULL;
	}
}


/* scan list to see if node is in it */
/* !!! centralize/publish */
static bool IsNodeInList (const List *list, const Node *matchnode)
{
    const Node *node;

        /* scan list (optimizate for matchnode == NULL) */
    if (matchnode) SCANLIST (list, node, Node) {
        if (node == matchnode) return TRUE;
    }

    return FALSE;
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplunrequestbuffer
 |||	ssplUnrequestBuffer - Return an unused buffer to the sound spooler.
 |||
 |||	  Synopsis
 |||
 |||	    Err ssplUnrequestBuffer (SoundSpooler *sspl, SoundBufferNode *sbn)
 |||
 |||	  Description
 |||
 |||	    ssplUnrequestBuffer() puts an unused SoundBufferNode from the requested
 |||	    queue back into the free queue. Use it as a way to politely give up
 |||	    write access to a SoundBufferNode that was given to you by ssplRequestBuffer()
 |||	    that you aren't going to submit to ssplSendBuffer(). For example, This can be
 |||	    used as part of a clean up path that traps failures between ssplRequestBuffer()
 |||	    and ssplSendBuffer().
 |||
 |||	    This is automatically done for all SoundBufferNodes in the requested list
 |||	    by ssplReset(), ssplAbort(), and ssplDeleteSoundSpooler().
 |||
 |||	  Argument
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||
 |||	    sbn                         Pointer to a SoundBufferNode to return.
 |||	                                The SBN must actually be in the requested
 |||	                                or else this function will return an error.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success, negative error code on failure:
 |||	        ML_ERR_BAD_ARG - the supplied sbn was not actually on the requested list.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V24.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplRequestBuffer(), ssplSendBuffer()
 **/

Err ssplUnrequestBuffer (SoundSpooler *sspl, SoundBufferNode *sbn)
{
    DBUG(("ssplUnrequestBuffer() sbn = 0x%08x\n", sbn));

        /* validate that sbn is actually in Requested queue */
    if (!IsNodeInList (&sspl->sspl_RequestedBuffers, (Node *)sbn)) return ML_ERR_BAD_ARG;

        /* return sbn to free list */
    ssplBufferToFreeList (sspl, sbn);

    return 0;
}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplsetbufferaddresslength
 |||	ssplSetBufferAddressLength - Attach sample data to a SoundBufferNode.
 |||
 |||	  Synopsis
 |||
 |||	    Err ssplSetBufferAddressLength ( SoundSpooler *sspl,
 |||	                                     SoundBufferNode *sbn,
 |||	                                     char *Data,
 |||	                                     int32 NumBytes )
 |||
 |||	  Description
 |||
 |||	    ssplSetBufferAddressLength() attaches sample data to a SoundBufferNode
 |||	    returned by ssplRequestBuffer(). This procedure is used internally by
 |||	    ssplPlayData() and ssplSpoolData().
 |||
 |||	  Arguments
 |||
 |||	    sspl                        Pointer to SoundSpooler that owns sbn.
 |||
 |||	    sbn                         Pointer to a SoundBufferNode returned by
 |||	                                ssplRequestBuffer().
 |||
 |||	    Data                        Pointer to sample data to attach to this
 |||	                                SoundBufferNode.
 |||
 |||	    NumBytes                    Length of sample data (in bytes).
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success, or negative 3DO error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Notes
 |||
 |||	    The safe minimum size for spooler buffers appears to be around 256
 |||	    bytes. Values less than this risk having gaps in the playback due
 |||	    insufficient time to process transitions from one sample Attachment
 |||	    to another. This minimum is in reality more a function of the time
 |||	    required to play a sample Attachment than that of sample length in
 |||	    bytes. The value 256 was determined using fixedstereosample.dsp
 |||	    which at the time of this writing had the fastest DMA consumption
 |||	    of all the DSP instruments (176,400 bytes/sec). This means that this
 |||	    value should be a suitable minimum spooler buffer length for any
 |||	    sample playback instrument used with the sound spooler.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplRequestBuffer(), ssplSendBuffer(), ssplSpoolData()
**/

Err ssplSetBufferAddressLength( SoundSpooler *sspl, SoundBufferNode *sbn, char *Data, int32 NumBytes )
{
	Err errcode;

DBUG(("ssplSetBufferAddressLength: sbn = 0x%08x, Data=0x%08x, Len=0x%08x\n",
		sbn, Data, NumBytes ));

/* set the Sample Item to point to the sample data from this chunk */
	if( (Data != sbn->sbn_Address) || (NumBytes != sbn->sbn_NumBytes) )
	{
	        /* check sample DMA alignment */
	    if ((errcode = CheckSampleAlignment (Data, NumBytes)) < 0) goto clean;

            /*
                Update SBN

                Note: must do this regardless of success of SetAudioItemInfo()
                because there's no telling how far along SetAudioItemInfo()
                got before failing - it might have linked sample to this data.
            */
		sbn->sbn_Address  = Data;
		sbn->sbn_NumBytes = NumBytes;

	        /* point SBN's sample to this data */
        if ((errcode = SetAudioItemInfoVA (sbn->sbn_Sample,
                                           AF_TAG_ADDRESS,  Data,
                                           AF_TAG_NUMBYTES, NumBytes,
                                           TAG_END)) < 0) goto clean;
	}

	return 0;

clean:
    return errcode;
}

/* Check sample buffer dma alignment */
static Err CheckSampleAlignment (const void *addr, uint32 len)
{
        /* !!! this is magic and is likely to vanish in favor of a GetAudioFolioInfo() tag */
    #define SP_DMA_ALIGNMENT    4                       /* DMA alignment in bytes (must be a power of 2) */

    return (uint32)addr & (SP_DMA_ALIGNMENT-1) || len & (SP_DMA_ALIGNMENT-1)
        ? ML_ERR_BAD_SAMPLE_ALIGNMENT
        : 0;
}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplsetuserdata
 |||	ssplSetUserData - Store user data in a SoundBufferNode.
 |||
 |||	  Synopsis
 |||
 |||	    void ssplSetUserData ( SoundSpooler *sspl,
 |||	                           SoundBufferNode *sbn,
 |||	                           void *UserData )
 |||
 |||	  Description
 |||
 |||	    ssplSetUserData stores user data pointer in a SoundBufferNode. This can be
 |||	    used to attach any extra data of your choosing to a SoundBufferNode. The
 |||	    user data pointer can be retrieved from a SoundBufferNode by calling
 |||	    ssplGetUserData().
 |||
 |||	  Arguments
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||
 |||	    sbn                         Pointer to a sound buffer node.
 |||
 |||	    UserData                    User data pointer to store in
 |||	                                SoundBufferNode.  Can be NULL.
 |||
 |||	  Return Value
 |||
 |||	    None
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplGetUserData(), ssplSpoolData()
**/

void ssplSetUserData( SoundSpooler *sspl, SoundBufferNode *sbn, void *UserData )
{
	sbn->sbn_UserData = UserData;
}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplgetuserdata
 |||	ssplGetUserData - Get User Data from a SoundBufferNode.
 |||
 |||	  Synopsis
 |||
 |||	    void *ssplGetUserData ( SoundSpooler *sspl,
 |||	                            SoundBufferNode *sbn )
 |||
 |||	  Description
 |||
 |||	    ssplGetUserData returns the pointer stored in a SoundBufferNode by
 |||	    ssplSetUserData().
 |||
 |||	  Arguments
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||
 |||	    sbn                         Pointer to a sound buffer node.
 |||
 |||	  Return Value
 |||
 |||	    User data pointer stored in a SoundBufferNode by ssplSetUserData().
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplSetUserData()
**/

void *ssplGetUserData( SoundSpooler *sspl, SoundBufferNode *sbn )
{
	return sbn->sbn_UserData;
}

#if 0       /* removed 940516 */
/*************************************************************************
** Set Sequence Number of Buffer. This should never be used. 940202
*************************************************************************/
void ssplSetSequenceNum( SoundSpooler *sspl, SoundBufferNode *sbn, int32 sequenceNum )
{
	sbn->sbn_SequenceNum = sequenceNum;
}
#endif


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplgetsequencenum
 |||	ssplGetSequenceNum - Get Sequence Number of a SoundBufferNode.
 |||
 |||	  Synopsis
 |||
 |||	    int32 ssplGetSequenceNum ( SoundSpooler *sspl,
 |||	                               SoundBufferNode *sbn )
 |||
 |||	  Description
 |||
 |||	    ssplGetSequenceNum returns the "sequence number" of the specified
 |||	    SoundBufferNode.  This is an integer that indicates the allocation
 |||	    sequence of the SoundBufferNode.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||
 |||	    sbn                         Pointer to a sound buffer node.
 |||
 |||	  Return Value
 |||
 |||	    An integer that is the index of the buffer as allocated by
 |||	    ssplCreateSoundSpooler(). It goes from zero to NumBuffers-1.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplCreateSoundSpooler()
**/

int32 ssplGetSequenceNum( SoundSpooler *sspl, SoundBufferNode *sbn )
{
	return sbn->sbn_SequenceNum;
}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplsendbuffer
 |||	ssplSendBuffer - Send a buffer full of data.
 |||
 |||	  Synopsis
 |||
 |||	    int32 ssplSendBuffer ( SoundSpooler *sspl,
 |||	                           SoundBufferNode *sbn )
 |||
 |||	  Description
 |||
 |||	    ssplSendBuffer() submits a SoundBufferNode ready for playback to the active
 |||	    queue. If the spooler has already been started, playback of this buffer
 |||	    will begin immediately if there are no other buffers in the active queue.
 |||	    In this case if a SoundBufferFunc in installed, a
 |||	    SSPL_SBMSG_STARVATION_START message is sent with sbn.
 |||
 |||	    The SoundBufferNode is posted into the active buffer queue on success, or the
 |||	    free buffer list on failure.  In either case, you are not permitted to modify
 |||	    the SoundBufferNode passed to this function after calling it.
 |||
 |||	    When successful, sets SSPL_STATUS_F_ACTIVE.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||
 |||	    sbn                         Pointer to a SoundBufferNode to send (returned
 |||	                                from ssplRequestBuffer()).
 |||
 |||	  Return Value
 |||
 |||	    Positive value on success indicating the signal to be sent when the buffer
 |||	    finishes playing, or negative 3DO error code on failure.  Never returns
 |||	    zero.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Caveats
 |||
 |||	    In the case where ssplStartSpooler() is called with an empty active queue
 |||	    and ssplSendBuffer() actually starts the spooler, ssplSendBuffer() calls
 |||	    SoundBufferFunc(SSPL_SBMSG_STARVATION_START) instead of
 |||	    SoundBufferFunc(SSPL_SBMSG_INITIAL_START).  This isn't truely a starvation
 |||	    case, it's merely a difference in order of calls, but ssplSendBuffer()
 |||	    can't tell the difference.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplRequestBuffer(), ssplUnrequestBuffer(), ssplSetBufferAddressLength(),
 |||	    ssplSpoolData(), ssplGetSpoolerStatus()
**/

int32 ssplSendBuffer( SoundSpooler *sspl, SoundBufferNode *sbn )
{
	int32 Result = 0;
	bool started = FALSE;

/* Clear signal if still on. */
	ssplClearBufferSignal( sspl, sbn );

/* Unlink anything attached to this node */
	LinkAttachments( sbn->sbn_Attachment, 0 );

/* Put node temporarily back into the free list. On failure, the SBN winds up
   back on the free list, on succes, the SBN is placed into the active list. */
	ssplBufferToFreeList( sspl, sbn );

/* Check sample data address and length for non-zero */
    if (!sbn->sbn_Address || !sbn->sbn_NumBytes) {
        Result = ML_ERR_BAD_SAMPLE_ADDRESS;
        goto cleanup;
    }

/* Is there an active buffer to link to? */
	if( IsEmptyList( &sspl->sspl_ActiveBuffers ) )
	{
		if( sspl->sspl_Flags & SSPL_F_STARTED)
		{
			Result = StartAttachment( sbn->sbn_Attachment, NULL );
			CHECKRESULT( Result, "ssplSendBuffer: StartAttachment");

			started = TRUE;
		}
	}
	else
	{
		SoundBufferNode *sbnLast = (SoundBufferNode *) LastNode( &sspl->sspl_ActiveBuffers );

/* Link together so it will automatically play. */
/* This link call was moved up to prevent a possible race condition. 940418
** We should add a SendAttachment() call to the audiofolio to better solve this. */
		Result = LinkAttachments( sbnLast->sbn_Attachment, sbn->sbn_Attachment );
		CHECKRESULT( Result, "ssplSendBuffer: LinkAttachment");

/* Has the old one completed? */
/* Check signal, restart if completed! */

/* !!! Race condition:

       If final buffer finished between LinkAttachments() and here, the buffer
       we just added will have already started by here.  We'll notice that the signal arrived
       and restart the new buffer.  This will cause a "stutter".  It is however better than
       stalling, which is what happens if you look for signals before doing LinkAttachments().
       Also, if this is likely to happen, the spooler will be very near starvation and will
       more than likely "hiccup" anyway.  SendAttachment() should fix this.

       Also, doing a GetCurrentSignals() here means that client cannot call this function
       between a WaitSignal() and ssplProcessSignals().  Doing so means that the final buffer
       enqueued in the active queue may have finished and had its signal cleared by
       WaitSignal().  The result is that this function will attempt to LinkAttachments()
       to a buffer that has finished playing, sent its signal, and had that signal cleared.
       Since it doesn't see the signal, it doesn't know to call StartAttachment() --- the spooler
       hangs.

       This function expects that the signal for the final buffer was not cleared prior to calling
       this function - or that any completed buffers in the active queue have been removed by
       calling ssplProcessSignals().  A good general rule of thumb is to require calling
       ssplProcessSignals() after WaitSignal() before calling any other spooler functions.
*/

		if(GetCurrentSignals() & sbnLast->sbn_Signal)
		{
			if( sspl->sspl_Flags & SSPL_F_STARTED)
			{
				Result = StartAttachment( sbn->sbn_Attachment, NULL );
				CHECKRESULT( Result, "ssplSendBuffer: StartAttachment");

				started = TRUE;
			}
		}
	}

/* Add node to list of active buffers if start/link happened successfully. */
	ssplBufferToActiveList( sspl, sbn );

/* Call callback if started after starvation */
    if (started) {
            /* !!! sends SSPL_SBMSG_STARVATION_START for initial start if
                   ssplStartSpooler() was called with an empty queue. could
                   add state information to send SSPL_SBMSG_INITIAL_START in this case */
        Result = CallSoundBufferFunc (sspl, sbn, SSPL_SBMSG_STARVATION_START);
        CHECKRESULT (Result, "ssplSendBuffer: (*SoundBufferFunc)(SSPL_SBMSG_STARVATION_START)");
    }

	return sbn->sbn_Signal;

cleanup:
	return Result;
}


/* -------------------- Status functions */

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplisspooleractive
 |||	ssplIsSpoolerActive - Test if spooler has anything in its active queue.
 |||
 |||	  Synopsis
 |||
 |||	    bool ssplIsSpoolerActive (const SoundSpooler *sspl)
 |||
 |||	  Description
 |||
 |||	    Returns TRUE if the SoundSpooler has any SoundBufferNodes in its active queue.
 |||	    Simply calls ssplGetSpoolerStatus() and tests the SSPL_STATUS_F_ACTIVE flag.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||
 |||	  Return Value
 |||
 |||	    TRUE if SoundSpooler has any SoundBufferNodes in its active queue, FALSE otherwise.
 |||
 |||	  Caveats
 |||
 |||	    This state of SSPL_STATUS_F_ACTIVE is undefined when inside any
 |||	    SoundSpooler callback function. This is the case because the callback
 |||	    functions can be called inside the loop that processes the active queue.
 |||	    No guarantee is made about whether a SoundBufferNode is removed from
 |||	    the active queue before or after the callback functions are called.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in soundspooler.h V24.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplGetSpoolerStatus(), ssplSendBuffer(), ssplProcessSignals()
 **/

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplgetspoolerstatus
 |||	ssplGetSpoolerStatus - Get SoundSpooler status flags.
 |||
 |||	  Synopsis
 |||
 |||	    int32 ssplGetSpoolerStatus (const SoundSpooler *sspl)
 |||
 |||	  Description
 |||
 |||	    Returns a set of flags SSPL_STATUS_F_ flags describing the current
 |||	    state of the SoundSpooler.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||
 |||	  Return Value
 |||
 |||	    Any combination of these flags (always a postive value):
 |||
 |||	    SSPL_STATUS_F_ACTIVE        This flag are SoundBufferNodes in the
 |||	                                SoundSpooler's active queue.
 |||	                                It is set by ssplSendBuffer(). It can be
 |||	                                cleared by ssplProcessSignals()
 |||	                                when all of the buffers in the
 |||	                                active queue have completed. It is
 |||	                                always cleared by ssplReset() and
 |||	                                ssplAbort().
 |||
 |||	                                You can use this flag as an indicator
 |||	                                that all of the sound data submitted
 |||	                                by ssplSendBuffer() has been played
 |||	                                as of the most recent call to
 |||	                                ssplProcessSignals().
 |||
 |||	    SSPL_STATUS_F_STARTED       This flag indicates that the SoundSpooler
 |||	                                has been started.
 |||	                                Unless it has been paused, it is
 |||	                                playing sound or will as soon as
 |||	                                there is some to play. This flag is set by
 |||	                                ssplStartSpooler() and cleared by
 |||	                                ssplStopSpooler() and
 |||	                                ssplAbort().
 |||
 |||	    SSPL_STATUS_F_PAUSED        This flag indicates that the SoundSpooler has been paused.
 |||	                                It is only meaningful if
 |||	                                SSPL_STATUS_F_STARTED is also
 |||	                                set. This flag is set by ssplPause() and cleared
 |||	                                by ssplResume(),
 |||	                                ssplStartSpooler(), and
 |||	                                ssplStopSpooler().
 |||
 |||	  Caveats
 |||
 |||	    This state of SSPL_STATUS_F_ACTIVE is undefined when inside any
 |||	    SoundSpooler callback function. This is the case because the callback
 |||	    functions can be called inside the loop that processes the active queue.
 |||	    No guarantee is made about whether a SoundBufferNode is removed from
 |||	    the active queue before or after the callback functions are called.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V24.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplIsSpoolerActive(), ssplSendBuffer(), ssplProcessSignals()
 **/

int32 ssplGetSpoolerStatus (const SoundSpooler *sspl)
{
    int32 status = 0;

    if (!IsEmptyList (&sspl->sspl_ActiveBuffers))   status |= SSPL_STATUS_F_ACTIVE;
    if (sspl->sspl_Flags & SSPL_F_STARTED)          status |= SSPL_STATUS_F_STARTED;
    if (sspl->sspl_Flags & SSPL_F_PAUSED)           status |= SSPL_STATUS_F_PAUSED;

    return status;
}


/* -------------------- Convenience functions */
/*
    @@@ NOTE

    These functions are sort of "convenient examples". That is they are
    generally useful convenience functions and are serve as examples of how to
    use the sound spooler.

    Currently the source code for these functions appears in the sound spooler
    chapter of the Portfolio programmer manuals. When these functions are
    changed, the revised code should be sent to the documentation group so that
    they may update the sound spooler chapter.
*/

/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplspooldata
 |||	ssplSpoolData - Send a block full of data. (convenience routine)
 |||
 |||	  Synopsis
 |||
 |||	    int32 ssplSpoolData ( SoundSpooler *sspl,
 |||	                          char *Data,
 |||	                          int32 NumBytes,
 |||	                          void *UserData )
 |||
 |||	  Description
 |||
 |||	    ssplSpoolData() is a convenience function that does the following
 |||	    operations:
 |||
 |||	      * Requests an available SoundBufferNode, returns 0 if none are
 |||	        available.
 |||
 |||	      * Attaches sample data and optional user data pointer to the
 |||	        SoundBufferNode.
 |||
 |||	      * Submits the SoundBufferNode to the active queue.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||
 |||	    Data                        Pointer to the buffer of data.
 |||
 |||	    NumBytes                    Number of bytes of data to send.
 |||
 |||	    UserData                    Pointer to user data, or NULL.
 |||
 |||	  Return Value
 |||
 |||	    Positive value on success indicating the signal to be sent when the buffer
 |||	    finishes playing, zero if no SoundBufferNodes are available, or negative
 |||	    3DO error code for others kinds of failure.
 |||
 |||	  Implementation
 |||
 |||	    Convenience call implemented in music.lib V21.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplPlayData(), ssplRequestBuffer(), ssplSendBuffer(),
 |||	    ssplSetBufferAddressLength(), ssplSetUserData()
**/

int32 ssplSpoolData (SoundSpooler *sspl, char *Data, int32 NumBytes, void *UserData)
{
    SoundBufferNode *sbn;
    int32 result = 0;

        /* Request a buffer, return if one isn't available */
    if ((sbn = ssplRequestBuffer (sspl)) == NULL) goto clean;

        /*
            Set address and length of buffer.
            Return buffer to spooler on failure.
        */
    if ((result = ssplSetBufferAddressLength (sspl, sbn, Data, NumBytes)) < 0) {
        ssplUnrequestBuffer (sspl, sbn);
        goto clean;
    }

        /* Set User Data (can't fail). */
    ssplSetUserData( sspl, sbn, UserData );

        /* Send that buffer off to be played. */
    return ssplSendBuffer (sspl, sbn);

clean:
    return result;
}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundspooler/ssplplaydata
 |||	ssplPlayData - Waits for next available buffer then sends a block full of
 |||	               data to the spooler.  (convenience routine).
 |||
 |||	  Synopsis
 |||
 |||	    int32 ssplPlayData ( SoundSpooler *sspl,
 |||	                         char *Data,
 |||	                         int32 NumBytes )
 |||
 |||	  Description
 |||
 |||	    ssplPlayData() is a convenience function that does the following
 |||	    operations:
 |||
 |||	      * Requests an available SoundBufferNode.  If none are available, waits for
 |||	        one to become available.
 |||
 |||	      * Attaches sample data to the SoundBufferNode.
 |||
 |||	      * Submits the SoundBufferNode to the active queue.
 |||
 |||	  Arguments
 |||
 |||	    sspl                        Pointer to a SoundSpooler structure.
 |||
 |||	    Data                        Pointer to data buffer.
 |||
 |||	    NumBytes                    Number of bytes to send.
 |||
 |||	  Return Value
 |||
 |||	    Positive value on success indicating the signal to be sent when the buffer
 |||	    finishes playing, or negative 3DO error code on failure.  Never returns
 |||	    zero.
 |||
 |||	  Implementation
 |||
 |||	    Convenience call implemented in music.lib V21.
 |||
 |||	  Notes
 |||
 |||	    This function calls ssplProcessSignals() when forced to wait for a
 |||	    SoundBufferNode, but doesn't have a mechanism for the client to supply a
 |||	    UserBufferProcessor callback function.  If you use this function and
 |||	    require completion notification, use a SoundBufferFunc instead of a
 |||	    UserBufferProcessor.
 |||
 |||	  Associated Files
 |||
 |||	    soundspooler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ssplSpoolData(), ssplRequestBuffer(), ssplSendBuffer(),
 |||	    ssplSetBufferAddressLength(), ssplSetUserData(), ssplProcessSignals(),
 |||	    (*SoundBufferFunc)()
**/

int32 ssplPlayData (SoundSpooler *sspl, char *Data, int32 NumBytes)
{
    SoundBufferNode *sbn;
    int32 result = 0;

        /*
            Request a buffer. If no buffers are available,
            wait for some buffer signals to arrive and process
            them to free up a buffer.
        */
    while ((sbn = ssplRequestBuffer (sspl)) == NULL) {
        const int32 sigs = WaitSignal (sspl->sspl_SignalMask);

        DBUG(("ssplPlayBuffer: WaitSignal() got 0x%x\n", sigs));
        if ((result = ssplProcessSignals (sspl, sigs, NULL)) < 0) goto clean;
    }

        /*
            Set address and length of buffer.
            Return buffer to spooler on failure.
        */
    if ((result = ssplSetBufferAddressLength (sspl, sbn, Data, NumBytes)) < 0) {
        ssplUnrequestBuffer (sspl, sbn);
        goto clean;
    }

        /* Send that buffer off to be played. */
    return ssplSendBuffer (sspl, sbn);

clean:
    return result;
}
