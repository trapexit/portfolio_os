/* $Id: audio_timer.c,v 1.57 1994/12/14 23:33:03 phil Exp $ */
/****************************************************************
**
** Timer for AudioFolio
**
** Includes a generalized Event Buffer that performs a function at a given time.
** It is a simple sorted list of events.
**
** Event types include:
**    AudioCue Items which can send signal to waiting task.
**    EnvelopeService events which maintain envelope real time progress.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/
/*
** 930211 PLB Fixed crash upon delete. Set InList after UniversalInsertNode
** 930708 PLB Use circular comparison for times for when clock wraparounds.
** 930830 PLB Check for too long a Duration.
** 940126 PLB Use actual ticks elapsed so that lost interrupts don't matter.
** 940413 PLB Fixed bad list walker in HandleTimerSignal()
** 940601 WJB Fixed warnings.
** 940608 WJB Replaced LaterThan...() macros with AudioTimeLaterThan...() macros.
** 940608 WJB Added autodocs for AudioTime macros.
** 940802 WJB Added support for demand loading.
** 940809 WJB Tweaked autodoc prefix.
** 940811 WJB Fixed AllocSignal() usage to correctly detect a 0 result.
** 941116 PLB Check for SIGF_ABORT signal.
** 941209 PLB Added support for DuckAndCover.
*/
#include "audio_internal.h"
#include "time.h"

/* Macros for debugging. */
#define DBUG(x)  /* PRT(x) */

#define MIN_DSP_DURATION (DEFAULT_SAMPLERATE/1000)
#define MAX_DSP_DURATION (0x7FFF)

/* Uncomment this if SWIs become preemptable. */
/* #define NEED_SEMAPHORE */

#define TICKSPERSECOND (240)


/* -------------------- AudioTime macro autodocs */

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/compareaudiotimes
 |||	CompareAudioTimes - Compare two AudioTime values with wraparound.
 |||
 |||	  Synopsis
 |||
 |||	    int32 CompareAudioTimes (AudioTime t1, AudioTime t2)
 |||
 |||	  Description
 |||
 |||	    CompareAudioTimes() compares two AudioTime values taking into account
 |||	    wraparound. The two time values are assumed to be within 0x7fffffff ticks
 |||	    of each other. Time differences larger than this value will produce
 |||	    incorrect comparisons.
 |||
 |||	  Arguments
 |||
 |||	    t1                          First AudioTime value to compare.
 |||
 |||	    t2                          Second AudioTime value to compare.
 |||
 |||	  Return Value
 |||
 |||	    >0                          If t1 is later than t2.
 |||
 |||	    0                           If t1 equals t2.
 |||
 |||	    <0                          If t1 is earlier than t2.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in audio.h V22.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    AudioTimeLaterThan(), AudioTimeLaterThanOrEqual()
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/audiotimelaterthan
 |||	AudioTimeLaterThan - Compare two AudioTime values with wraparound.
 |||
 |||	  Synopsis
 |||
 |||	    int AudioTimeLaterThan (AudioTime t1, AudioTime t2)
 |||
 |||	  Description
 |||
 |||	    AudioTimeLaterThan() compares two AudioTime values, taking into account
 |||	    wraparound.
 |||
 |||	    The two time values are assumed to be within 0x7fffffff ticks of each
 |||	    other. The time differences larger than this number will produce incorrect
 |||	    comparisons.
 |||
 |||	  Arguments
 |||
 |||	    t1                          First AudioTime value to compare.
 |||
 |||	    t2                          Second AudioTime value to compare.
 |||
 |||	  Return Value
 |||
 |||	    TRUE                        If t1 is later than t2.
 |||
 |||	    FALSE                       If t1 is earlier than or equal to t2.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in audio.h V22.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    AudioTimeLaterThanOrEqual(), CompareAudioTimes()
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/audiotimelaterthanorequal
 |||	AudioTimeLaterThanOrEqual - Compare two AudioTime values with wraparound.
 |||
 |||	  Synopsis
 |||
 |||	    int AudioTimeLaterThanOrEqual (AudioTime t1, AudioTime t2)
 |||
 |||	  Description
 |||
 |||	    AudioTimeLaterThanOrEqual() compares two AudioTime values, taking into
 |||	    account wraparound.
 |||
 |||	    The two time values are assumed to be within 0x7fffffff ticks of each
 |||	    other. Time differences larger than that will produce incorrect
 |||	    comparisons.
 |||
 |||	  Arguments
 |||
 |||	    t1                          First AudioTime value to compare.
 |||
 |||	    t2                          Second AudioTime value to compare.
 |||
 |||	  Return Value
 |||
 |||	    TRUE                        If t1 is later than or equal to t2.
 |||
 |||	    FALSE                       If t1 is earlier than t2.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in audio.h V22.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    AudioTimeLaterThan(), CompareAudioTimes()
 |||
 **/


/* -------------------- Code */

static vuint32 *DSPPTickCounter = (vuint32 *) 0x3403FB8;   /* Hack to count DSPP ticks! */

/***************************************************************/
/* Interrupt routine */
int32 AudioTimerIntHandler(void)
{
	int32 TickCount;
	int32 TicksElapsed;

/* Increment timer based on change in tick count from DSPP.
** This allows the audio timer to recover from lost interrupts
** which would have caused the timer to lose ticks.
** This technique could be used to optimise the timer so
** that we only interrupt when we reach a pending timer value,
** or reach a minimum time that gurantees we don't wrap.  This would
** require DSPP code changes that could be costly.
*/
	TickCount = *DSPPTickCounter; /* 940126 */
	TicksElapsed = ( TickCount - AudioBase->af_LastTickCount ) & 0xFFFF;
	REALTIME += TicksElapsed;
	AudioBase->af_LastTickCount = TickCount;

#if 0
	if( TicksElapsed > 1 )
	{
		AudioBase->af_TimerTicksLost = TicksElapsed - 1;
	}
#endif

	if (AudioBase->af_TimerPending)
	{
		if (!AudioTimeLaterThan(AudioBase->af_NextTime,REALTIME))
		{
			SuperinternalSignal(AudioBase->af_AudioTask,
				AudioBase->af_TimerSignal);
			AudioBase->af_TimerPending = FALSE;
		}
	}
	return 0;
}

static
TagArg FirqTags[] =
{
	TAG_ITEM_PRI,		(void *)100,
	TAG_ITEM_NAME,	    "AudioTimer",
	CREATEFIRQ_TAG_CODE, 	(void *) ((int32)AudioTimerIntHandler),
	CREATEFIRQ_TAG_NUM, 	(void *) INT_DSPP,
	TAG_END,	NULL
};

/***************************************************************/
int32 InitAudioTimer ( void )
{
	Item it;
	TagArg ListSema4Tags[2];
	TagArg RateSema4Tags[2];

	ListSema4Tags[0].ta_Tag = TAG_ITEM_NAME;
	ListSema4Tags[0].ta_Arg = (int32 *) "AFTimerListSem4";
	ListSema4Tags[1].ta_Tag = TAG_END;

	RateSema4Tags[0].ta_Tag = TAG_ITEM_NAME;
	RateSema4Tags[0].ta_Arg = (int32 *) "AFTimerRateSem4";
	RateSema4Tags[1].ta_Tag = TAG_END;

/* Clear Timer */
	AudioBase->af_Time = 0;
	AudioBase->af_NextTime = 0;
	AudioBase->af_TimerPending = FALSE;

/* Create a semaphore for managing Timer list. */
	it = SuperCreateItem(MKNODEID(KERNELNODE,SEMA4NODE), ListSema4Tags);
	if (it < 0)
	{
		ERR(("InitAudioTimer: Could not create List Semaphore! 0x%x\n", it));
		return (it);
	}
	AudioBase->af_TimerListSemaphore = it;

/* Create a semaphore for managing Timer Rate. */
	it = SuperCreateItem(MKNODEID(KERNELNODE,SEMA4NODE), RateSema4Tags);
	if (it < 0)
	{
		ERR(("InitAudioTimer: Could not create Rate Semaphore! 0x%x\n", it));
		return (it);
	}
	AudioBase->af_TimerDurSemaphore = it;

/* Init list for holding timer requests. */
	InitList( &AudioBase->af_TimerList, "AudioTimer");

/* Allocate signal for interrupt to signal foreground if it is NextTime */
    {
        const int32 sig = SuperAllocSignal( 0 );

        if (sig <= 0)
        {
            ERR(("InitAudioTimer could not SuperAllocSignal\n"));
            return sig ? sig : AF_ERR_NOSIGNAL;
        }
        AudioBase->af_TimerSignal = sig;
    }
	AudioBase->af_AudioTask = CURRENTTASK;

/* Make an interrupt request item. */
	AudioBase->af_TimerFIRQ = SuperCreateItem(MKNODEID(KERNELNODE,FIRQNODE),FirqTags);
	if (AudioBase->af_TimerFIRQ < 0)
	{
		ERR(("InitAudioTimer failed to create interrupt: 0x%x\n", AudioBase->af_TimerFIRQ));
	}

	EnableInterrupt(INT_DSPP);

DBUG(("&Time = 0x%x\n", &AudioBase->af_Time));

	return AudioBase->af_TimerFIRQ;
}

/***************************************************************/
int32 TermAudioTimer ( void )
{
	DisableInterrupt(INT_DSPP);

/* Delete a semaphore for managing Timer list. */
	if(AudioBase->af_TimerDurSemaphore) afi_SuperDeleteItem(AudioBase->af_TimerDurSemaphore);
	if(AudioBase->af_TimerListSemaphore) afi_SuperDeleteItem(AudioBase->af_TimerListSemaphore);

/* Delete signal for interrupt to signal foreground if it is NextTime */
	if(AudioBase->af_TimerSignal) {
        SuperFreeSignal(AudioBase->af_TimerSignal);
        AudioBase->af_TimerSignal = 0;
    }

/* Delete an interrupt request item. */
	if(AudioBase->af_TimerFIRQ) afi_SuperDeleteItem(AudioBase->af_TimerFIRQ);

	return AudioBase->af_TimerFIRQ;
}

/*****************************************************************/
bool MatchTimerNode( Node *NewNode, Node *ListNode)
{
	AudioEvent *ln, *nn;

	ln = (AudioEvent *) ListNode;
	nn = (AudioEvent *) NewNode;
	return (AudioTimeLaterThanOrEqual(ln->aevt_TriggerAt,nn->aevt_TriggerAt));
}

int32 PostAudioEvent( AudioEvent *aevt, AudioTime Time )
{
	int32 Result = 0;

/* Check to see if in use. */
	if (aevt->aevt_InList != NULL)
	{
		ERR(("PostAudioEvent: Event in use!\n"));
		Result = AF_ERR_INUSE;
		goto error;
	}

/* Make this is the next one if it is before current one. */
	if (AudioBase->af_TimerPending)
	{
		if (AudioTimeLaterThan(AudioBase->af_NextTime,Time))
		{
			AudioBase->af_NextTime = Time;
		}
	}
	else
	{
		AudioBase->af_NextTime = Time;
		AudioBase->af_TimerPending = TRUE;
	}
	aevt->aevt_TriggerAt = Time;

/* Insert request into timer chain in sorted order. */
	UniversalInsertNode(&AudioBase->af_TimerList, (Node *) aevt, MatchTimerNode);
	aevt->aevt_InList = &AudioBase->af_TimerList; /* 930211 */

error:
	return Result;
}

/*****************************************************************/
int32 UnpostAudioEvent( AudioEvent *aevt )
{

/* Check to see if in use. */
	if (aevt->aevt_InList != NULL)
	{
		ParanoidRemNode( (Node *) aevt );   /* Remove from timer chain. */
		aevt->aevt_InList = NULL;
	}
	return 0;
}

/*****************************************************************/
/* Set timer to signal on next matching time. */
void ScheduleNextAudioTimerSignal( void )
{
	AudioEvent *aevt;

/* Tell interrupt when to interrupt next. */
	aevt = (AudioEvent *) FIRSTNODE( &AudioBase->af_TimerList );
	if ( ISNODE(&AudioBase->af_TimerList, aevt) )
	{
TRACEB(TRACE_INT, TRACE_TIMER, ("RunAudioTimer: NextTime =  0x%x\n",
		aevt->aevt_TriggerAt));
		AudioBase->af_NextTime = aevt->aevt_TriggerAt;
		AudioBase->af_TimerPending = TRUE;
	}
}

/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/signalattime
 |||	SignalAtTime - Requests a wake-up call at a given time.
 |||
 |||	  Synopsis
 |||
 |||	    Err SignalAtTime (Item Cue, AudioTime Time)
 |||
 |||	  Description
 |||
 |||	    This procedure requests that a signal be sent to the calling item at the
 |||	    specified audio time. The cue item can be created using CreateCue().
 |||
 |||	    A task can get the cue's signal mask using GetCueSignal(). The task
 |||	    can then call WaitSignal() to enter wait state until the cue's signal
 |||	    is sent at the specified time.
 |||
 |||	    If you need to, you can call AbortTimerCue() to cancel the timer
 |||	    request before completion.
 |||
 |||	    See the test program "ta_timer.c" for a complete example.
 |||
 |||	  Arguments
 |||
 |||	    Cue                          Item number of a cue.
 |||
 |||	    Time                         The time at which to send a signal to the
 |||	                                 calling task.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    AbortTimerCue(), CreateCue(), GetCueSignal(), SleepUntilTime(),
 |||	    GetAudioRate()
 |||
 **/
int32 swiSignalAtTime( Item Cue, AudioTime Time)
/* Send a signal back to this task at the desired Time.
** This allows multiple signal waits.
*/
{
	AudioCue *acue;
	int32 Result = 0;

TRACEE(TRACE_INT, TRACE_TIMER, ("swiSignalAtTime (  0x%x, 0x%x )\n",
	Cue, Time));

	CHECKAUDIOOPEN;

	acue = (AudioCue *)CheckItem(Cue, AUDIONODE, AUDIO_CUE_NODE);
	if (acue == NULL)
	{
		Result = AF_ERR_BADITEM;
		goto done;
	}

TRACEB(TRACE_INT, TRACE_TIMER, ("swiSignalAtTime:  acue = 0x%x, next = 0x%x\n",
	acue, AudioBase->af_NextTime ));

/* Don't let another task wait on this cuz it will never wake up. */
	if(!OWNEDBYCALLER(acue))
	{
		ERR(("swiSignalAtTime: Cue 0x%x must be owned by caller.\n", Cue));
		Result = AF_ERR_NOTOWNER;
		goto done;
	}

#ifdef NEED_SEMAPHORE
/* Lock Semaphore cuz we're messing with the Timer list and AudioBase */
	SuperLockSemaphore( AudioBase->af_TimerListSemaphore, TRUE );
#endif

	Result = PostAudioEvent( (AudioEvent *) &acue->acue_Event, Time );

#ifdef NEED_SEMAPHORE
	SuperUnlockSemaphore(  AudioBase->af_TimerListSemaphore );
#endif

done:
TRACER(TRACE_INT, TRACE_TIMER, ("swiSignalAtTime returns  0x%x\n", Result));
	return Result;
}

/*****************************************************************/
/* Cancel a request to SignalAtTime */
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/aborttimercue
 |||	AbortTimerCue - Cancels a timer request enqueued with SignalAtTime().
 |||
 |||	  Synopsis
 |||
 |||	    Err AbortTimerCue (Item Cue)
 |||
 |||	  Description
 |||
 |||	    Cancels a timer request enqueued with SignalAtTime(). The signal
 |||	    associated with the cue will not be sent if the request is aborted before
 |||	    completion. Aborting a timer request after completion does nothing
 |||	    harmful. Aborting a cue that has not been posted as a timer request is
 |||	    harmless.
 |||
 |||	  Arguments
 |||
 |||	    Cue                          Cue item to abort. The task calling this
 |||	                                 function must own the Cue. The Cue may have
 |||	                                 already completed - nothing bad will happen.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V21.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    SignalAtTime()
 |||
 **/
int32 swiAbortTimerCue( Item Cue )
{
	AudioCue *acue;
	int32 Result = 0;

TRACEE(TRACE_INT, TRACE_TIMER, ("swiAbortTimerCue (  0x%x )\n", Cue ));

	CHECKAUDIOOPEN;

	acue = (AudioCue *)CheckItem(Cue, AUDIONODE, AUDIO_CUE_NODE);
	if (acue == NULL)
	{
		Result = AF_ERR_BADITEM;
		goto done;
	}

TRACEB(TRACE_INT, TRACE_TIMER, ("swiAbortTimerCue:  acue = 0x%x\n", acue));

/* Don't let another task abort cuz we will never wake up. */
	if(!OWNEDBYCALLER(acue))
	{
		ERR(("swiAbortTimerCue: Cue 0x%x must be owned by caller.\n", Cue));
		Result = AF_ERR_NOTOWNER;
		goto done;
	}

#ifdef NEED_SEMAPHORE
/* Lock Semaphore cuz we're messing with the Timer list and AudioBase */
	SuperLockSemaphore( AudioBase->af_TimerListSemaphore, TRUE );
#endif

	Result = UnpostAudioEvent( (AudioEvent *) &acue->acue_Event );

	ScheduleNextAudioTimerSignal();

#ifdef NEED_SEMAPHORE
	SuperUnlockSemaphore(  AudioBase->af_TimerListSemaphore );
#endif

done:
TRACER(TRACE_INT, TRACE_TIMER, ("swiAbortTimerCue returns  0x%x\n", Result));
	return Result;
}


/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/getcuesignal
 |||	GetCueSignal - Returns a signal mask for a cue.
 |||
 |||	  Synopsis
 |||
 |||	    int32 GetCueSignal (Item Cue)
 |||
 |||	  Description
 |||
 |||	    This procedure returns a signal mask containing the signal bit
 |||	    allocated for an audio Cue item. The mask can be passed to
 |||	    WaitSignal() so a task can enter wait state until a Cue completes.
 |||
 |||	  Arguments
 |||
 |||	    Cue                          Item number of Cue.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a signal mask (a positive value) if successful
 |||	    or a non-positive value (0 or an error code) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    This function has always returned 0 when a bad item is passed in. It
 |||	    should probably have returned AF_ERR_BADITEM, but will continue to
 |||	    return 0 for this case.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    SignalAtTime(), SleepUntilTime(), MonitorAttachment()
 |||
 **/
int32 GetCueSignal( Item Cue )
{
	AudioCue *acue;
	int32 Sig;
TRACEE(TRACE_INT, TRACE_TIMER, ("GetCueSignal ( 0x%x)\n",
	Cue));

	acue = (AudioCue *)CheckItem(Cue, AUDIONODE, AUDIO_CUE_NODE);
	if (acue == NULL) return 0;

	Sig = acue->acue_Signal;
TRACER(TRACE_INT, TRACE_TIMER, ("GetCueSignal returns 0x%x\n", Sig));
	return Sig;
}

/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/sleepuntiltime
 |||	SleepUntilTime - Enters wait state until a specified audio time.
 |||
 |||	  Synopsis
 |||
 |||	    Err SleepUntilTime (Item Cue, AudioTime Time)
 |||
 |||	  Description
 |||
 |||	    This procedure will not return until the specified audio time is reached.
 |||	    If that time has already passed, it will return immediately. If it needs
 |||	    to wait, your task will enter wait state so that it does not consume CPU
 |||	    time.
 |||
 |||	  Arguments
 |||
 |||	    Cue                          The item number of the cue.
 |||
 |||	    Time                         Audio time to wait for
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Examples
 |||
 |||	        // If you want to wait for a specific number of ticks to go by,
 |||	        // you can get the current audio time from GetAudioTime() and
 |||	        // add your delay:
 |||	    SleepUntilTime (cue, GetAudioTime() + DELAYTICKS);
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    SignalAtTime(), CreateCue(), GetCueSignal(), GetAudioTime(),
 |||	    GetAudioRate()
 |||
 **/
int32 SleepUntilTime( Item Cue, AudioTime Time )
{
	uint32 CueSignal;
	int32 Result;

TRACEE(TRACE_INT, TRACE_TIMER, ("SleepUntilTime ( 0x%x, 0x%x )\n",
	Cue, Time));
	Result = SignalAtTime( Cue, Time );
	if (Result < 0) return Result;

	CueSignal = GetCueSignal ( Cue );
TRACEB(TRACE_INT, TRACE_TIMER, ("SleepUntilTime: Signal = 0x%x\n", CueSignal));
	WaitSignal( CueSignal );

TRACER(TRACE_INT, TRACE_TIMER, ("SleepUntilTime returns 0x%x\n", 0));
	return 0;
}

/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/sleepaudioticks
 |||	SleepAudioTicks - Enters wait state for a number of audio ticks (OBSOLETE)
 |||
 |||	  Synopsis
 |||
 |||	    Err SleepAudioTicks (int32 Ticks)
 |||
 |||	  Description
 |||
 |||	    This procedure puts the calling task in wait state for the specified
 |||	    number of ticks of the audio clock. If you use this procedure to
 |||	    schedule music events you'll regret it, because there will be creeping
 |||	    error in the absolute time. Use the cue-based procedures instead.
 |||
 |||	  Arguments
 |||
 |||	    Ticks                        The number of audio clock ticks to wait (at
 |||	                                 240 Hz or whatever the current rate is).
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Notes
 |||
 |||	    Use of this function is discouraged because it creates and deletes a
 |||	    cue, which wastes an Item number. Create your own Cue, keep it around,
 |||	    and use SleepUntilTime() instead.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    SleepUntilTime(), SignalAtTime(), CreateCue(), GetAudioRate()
 |||
 **/
int32 SleepAudioTicks( int32 Ticks )
{
	Item Cue;
	int32 Result;

	Cue = CreateItem ( MKNODEID(AUDIONODE,AUDIO_CUE_NODE), NULL );
	if (Cue < 0) return (int32) Cue;

	Result = SleepUntilTime( Cue, Ticks + REALTIME );

	DeleteItem ( Cue );
	return Result;
}

/************************************************************************/
/****** Rate Control  ***************************************************/
/************************************************************************/
int32 lowSetAudioDuration ( uint32 Duration )
{
	int32 Result;

	if((Duration < MIN_DSP_DURATION) || (Duration > MAX_DSP_DURATION))
	{
		ERR(("swiSetAudioDuration: Duration out of range = %d\n", Duration));
		return AF_ERR_OUTOFRANGE;
	}

	if (AudioBase->af_TimerDurKnob)
	{
		Result = swiTweakKnob( AudioBase->af_TimerDurKnob, Duration );
		AudioBase->af_Duration = Duration;
	}
	else
	{
		Result = AF_ERR_BADITEM;
	}

	return Result;
}

/************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/setaudioduration
 |||	SetAudioDuration - Changes duration of audio clock tick.
 |||
 |||	  Synopsis
 |||
 |||	    Err SetAudioDuration (Item Owner, uint32 Frames)
 |||
 |||	  Description
 |||
 |||	    This procedure changes the rate of the audio clock by specifying a
 |||	    duration for the clock's tick. The clock is driven by a countdown
 |||	    timer in the DSP. When the DSP timer reaches zero, it increments the
 |||	    audio clock by one. The DSP timer is decremented at the sample rate
 |||	    of 44,100 Hz. Thus tick duration is given in units of sample frames.
 |||
 |||	    The current audio clock duration can be read with GetAudioDuration().
 |||
 |||	    You must own the audio clock before changing its rate or duration. See
 |||	    OwnAudioClock().
 |||
 |||	  Arguments
 |||
 |||	    Owner                        Item number of owner ID from
 |||	                                 OwnAudioClock().
 |||
 |||	    Duration                     Duration of one audio tick in sample
 |||	                                 frames at 44,100 Hz.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    OwnAudioClock(), GetAudioDuration(), SetAudioRate(), GetAudioRate(),
 |||	    SignalAtTime(), SleepUntilTime(), GetAudioFolioInfo()
 |||
 **/
int32 swiSetAudioDuration ( Item Owner, uint32 Duration )
{
	int32 Result;

	if( Owner != AudioBase->af_TimerDurSemaphore )
	{
		ERR(("swiSetAudioDuration: Timer not owned.\n"));
		return AF_ERR_INUSE;
	}

	Result = lowSetAudioDuration( Duration );
	if( Result >= 0 )
	{
/* Even if the Sample Rate changes, don't adjust the clock. */
		AudioBase->af_DesiredTimerRate = 0; /* Disable */
	}
	return Result;

}

/************************************************************************/
int32 lowSetAudioRate ( frac16 Rate )
{
	ufrac16 Rem;
	uint32 Dur;
	frac16 Dur16;

	Dur16 = DivRemUF16( &Rem, Convert32_F16((uint32) DSPPData.dspp_SampleRate ), Rate );
/* Integerize with rounding by adding 0.5 before conversion. */
	Dur = ConvertF16_32( Dur16 + (Convert32_F16(1)>>1) );

	return lowSetAudioDuration ( Dur );
}

/************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/setaudiorate
 |||	SetAudioRate - Changes the rate of the audio clock.
 |||
 |||	  Synopsis
 |||
 |||	    Err SetAudioRate (Item Owner, frac16 Rate)
 |||
 |||	  Description
 |||
 |||	    This procedure changes the rate of the audio clock. The default rate is
 |||	    240 Hz. The range of the clock rate is 60 to 1000 Hz.
 |||
 |||	    The current audio clock rate can be read with GetAudioRate().
 |||
 |||	    You must own the audio clock before changing the rate or duration. See
 |||	    OwnAudioClock().
 |||
 |||	  Arguments
 |||
 |||	    Owner                        Item number of owner ID from
 |||	                                 OwnAudioClock().
 |||
 |||	    Rate                         16.16 fractional rate value in Hz.
 |||	                                 Must be in the range of 60 Hz to 1000 Hz.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    High clock rates may degrade system performance.
 |||
 |||	  Examples
 |||
 |||	    // Here is an example of changing the rate to 300 Hz (error checking
 |||	    // omitted for brevity).
 |||	    {
 |||	        Owner = OwnAudioClock();
 |||	        SetAudioRate (Owner, Convert32_F16 (300));
 |||	    }
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    OwnAudioClock(), GetAudioRate(), SetAudioDuration(),
 |||	    GetAudioDuration(), SignalAtTime(), SleepUntilTime()
 |||
 **/
int32 swiSetAudioRate ( Item Owner, frac16 Rate )
{
	int32 Result;

	if( Owner != AudioBase->af_TimerDurSemaphore )
	{
		ERR(("swiSetAudioRate: Timer not owned.\n"));
		return AF_ERR_INUSE;
	}

	Result = lowSetAudioRate ( Rate );
	if( Result >= 0 )
	{
/* Save rate so that if the Sample Rate changes we can adjust the clock. */
		AudioBase->af_DesiredTimerRate = Rate;
	}
	return Result;
}

/************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/getaudiotime
 |||	GetAudioTime - Queries current audio time.
 |||
 |||	  Synopsis
 |||
 |||	    AudioTime GetAudioTime (void)
 |||
 |||	  Description
 |||
 |||	    This procedure returns the current 32-bit audio time. This time comes
 |||	    from an interrupt from the DSP and typically increases at 240 Hz.
 |||
 |||	  Arguments
 |||
 |||	    None.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the current audio time expressed in audio ticks.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    Note that the audio clock rate can be changed by other tasks, so the
 |||	    audio time isn't guaranteed to be constant unless the task making this
 |||	    call takes ownership of the clock using OwnAudioClock(). Of course,
 |||	    owning the audio clock and hanging onto the lock forever causes any
 |||	    other task trying to the same to fail.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    SignalAtTime(), GetAudioRate(), GetAudioDuration(), OwnAudioClock(),
 |||	    GetAudioFolioInfo()
 |||
 **/
AudioTime GetAudioTime( void )
{
	return AudioBase->af_Time;
}

/************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/getaudiorate
 |||	GetAudioRate - Asks for current audio clock rate.
 |||
 |||	  Synopsis
 |||
 |||	    frac16 GetAudioRate (void)
 |||
 |||	  Description
 |||
 |||	    This procedure returns the current clock rate in 16.16 fractional Hz,
 |||	    using fractions so that you can do more accurate timing calculations.
 |||
 |||	  Arguments
 |||
 |||	    None.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the current audio rate as a frac16 value in Hz.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Examples
 |||
 |||	        // To convert the result of this function to ticks per second, use
 |||	        // ConvertF16_32().
 |||	    TicksPerSecond = ConvertF16_32 (GetAudioRate());
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    SignalAtTime(), GetAudioDuration(), SetAudioRate()
 |||
 **/
frac16 GetAudioRate( void )
{
	ufrac16 Rem;
	frac16 Rate;
	frac16 Dur16;

	Dur16 = Convert32_F16( AudioBase->af_Duration );
	Rate = DivRemUF16( &Rem, Convert32_F16((uint32) DSPPData.dspp_SampleRate ), Dur16 );

TRACEB(TRACE_INT, TRACE_TIMER, ("Duration = %d, Rate = 0x%x\n",
		AudioBase->af_Duration, Rate ));
	return ( Rate );
}

/************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/ownaudioclock
 |||	OwnAudioClock - Takes ownership of audio clock.
 |||
 |||	  Synopsis
 |||
 |||	    Item OwnAudioClock (void)
 |||
 |||	  Description
 |||
 |||	    This procedure takes ownership of the audio clock so that it can change
 |||	    the rate or so that no other task can change the clock rate during
 |||	    ownership, or both. The audio clock rate is set to a default of around
 |||	    240 Hz. Taking clock ownership without changing the rate ensures that no
 |||	    other task will change the rate, which assures a steady clock.
 |||
 |||	    Note that a task can't change the clock rate without owning the clock.
 |||
 |||	    Ownership can be relinquished with DisownAudioClock().
 |||
 |||	  Arguments
 |||
 |||	    None.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number of a semaphore used to claim
 |||	    ownership of the clock if successful (a positive value). It returns
 |||	    an error code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    Owning the audio clock and hanging onto the lock forever causes any
 |||	    other task trying to the same to fail.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    DisownAudioClock(), SetAudioRate(), SetAudioDuration(),
 |||	    GetAudioRate(), GetAudioDuration(), SignalAtTime(), GetAudioTime(),
 |||	    SleepUntilTime()
 |||
 **/
Item   OwnAudioClock( void )
{
	int32 ls;

	CHECKAUDIOOPEN;

	ls = LockSemaphore ( AudioBase->af_TimerDurSemaphore, 0 );
	if (ls > 0)
	{
		return AudioBase->af_TimerDurSemaphore;
	}
	else
	{
		return AF_ERR_INUSE;
	}
}

/************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/disownaudioclock
 |||	DisownAudioClock - Gives up ownership of the audio clock.
 |||
 |||	  Synopsis
 |||
 |||	    Err DisownAudioClock (Item Owner)
 |||
 |||	  Description
 |||
 |||	    This procedure is the inverse of OwnAudioClock() it relinquishes
 |||	    ownership of the audio clock. Once a task does not own the audio clock,
 |||	    it can't change the audio clock rate or be assured that another task
 |||	    doesn't change it.
 |||
 |||	  Arguments
 |||
 |||	    Owner                        Item number of ownership returned from
 |||	                                 OwnAudioClock().
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    OwnAudioClock()
 |||
 **/
int32   DisownAudioClock( Item Owner )
{
	if (Owner == AudioBase->af_TimerDurSemaphore)
	{
		 return UnlockSemaphore ( AudioBase->af_TimerDurSemaphore );
	}
	else
	{
		return AF_ERR_BADITEM;
	}
}

/************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/getaudioduration
 |||	GetAudioDuration - Asks for the current duration of an audio clock tick.
 |||
 |||	  Synopsis
 |||
 |||	    uint32 GetAudioDuration (void)
 |||
 |||	  Description
 |||
 |||	    This procedure asks for the current duration of an audio clock tick
 |||	    measured in DSP sample frames. The DSP runs at 44,100 Hz.
 |||
 |||	  Arguments
 |||
 |||	    None.
 |||
 |||	  Return Value
 |||
 |||	    Current audio clock duration expressed as the actual number of sample
 |||	    frames between clock interrupts.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    SignalAtTime(), GetAudioRate(), SetAudioDuration(), GetAudioFolioInfo()
 |||
 **/
uint32 GetAudioDuration( void )
{
	return AudioBase->af_Duration ;
}


/************************************************************************/
/****** Audio Timer Foreground process, never returns!  *****************/
/************************************************************************/
/*
** Receive signals from Interrupt and manage list of pending
** timer requests. Only returns if SIGF_ABORT received.
*/
int32 swiRunAudioSignalTask( Item DurKnob, void *startupdata )
{
	int32 Signals, Result;

/* Verify that caller is priveledged. Don't do anything before this check. */
	if ((CURRENTTASK->t.n_Flags & TASK_SUPER) == 0)
	{
		ERR(("swiRunAudioTimer only for Audio Folio task!\n"));
		return(AF_ERR_BADPRIV);
	}

/*-----------------------------------------------------------------*/
/* Perform any setup that must take place in supervisor mode.      */
/* Set up DSPP I Memory access. */
	Result = DSPP_InitIMemAccess();
	if(Result < 0) return Result;

/* Tell XBUS to do DuckAndCover when DIPIR occurs. 941209 */
	Result = dsppInitDuckAndCover();
	if( Result < 0 )
	{
		ERR(("dsppInitDuckAndCover() failed but life goes on...\n"));
	}
	
/*-----------------------------------------------------------------*/

	AudioBase->af_TimerDurKnob = DurKnob;

	swiSetAudioRate( AudioBase->af_TimerDurSemaphore, Convert32_F16(240) );

/* Init lost interrupt detection for audio clock. */
	AudioBase->af_LastTickCount = *DSPPTickCounter;
	AudioBase->af_TimerTicksLost = 0;


/* We are finally ready so unlock semaphore and allow others to use folio.940309 */
	Result = SuperUnlockSemaphore( AudioBase->af_ReadySemaphore );
	if( Result < 0) return Result;

/* now that all is well, notify main() that we succeeded */
/* (after here startupdata becomes invalid - not that this function should really care) */
	NotifyStartupSuccess (startupdata);

	while(1)
	{
TRACEB(TRACE_INT, TRACE_TIMER, ("swiRunAudioSignalTask: WaitSignal(0x%x)\n",
		AudioBase->af_TimerSignal));
DBUG(("swiRunAudioSignalTask: Wait for = 0x%x\n", AudioBase->af_TimerSignal | AudioBase->af_DMASignalMask ));
		Signals = SuperWaitSignal(AudioBase->af_TimerSignal | AudioBase->af_DMASignalMask);
/* ================ LONG PAUSE ===================== */
/*       SIGNAL OCCURS!                              */
/* ================ Start to Run Again ============= */
DBUG(("swiRunAudioSignalTask: Signals = 0x%x\n", Signals ));

/* Check for catastrophic receipt of SIGF_ABORT.  Game over!!!  941116 */
		if (Signals & SIGF_ABORT)
		{
			ERR(("AudioFolio received SIGF_ABORT!\n"));
			DSPP_Term();
			return -1;
		}

		if (Signals & AudioBase->af_TimerSignal)
		{
			HandleTimerSignal();
		}

		if (Signals & AudioBase->af_DMASignalMask)
		{
			HandleDMASignal( Signals );
		}
	}
}

/*****************************************************************/
int32 PerformCueSignal( AudioCue *acue )
{
TRACEB(TRACE_INT, TRACE_TIMER, ("RunAudioTimer: Signal =  0x%x\n", acue->acue_Signal));
	return SuperinternalSignal( acue->acue_Task, acue->acue_Signal );
}

/*****************************************************************/
/* Process Timer events. Called when timer interrupt occurs. */
int32 HandleTimerSignal( void )
{

	AudioEvent *aevt;
	AudioTime Time;

	Time = REALTIME;
TRACEB(TRACE_INT, TRACE_TIMER, ("HandleTimerSignal: Time =  0x%x\n", Time));

#ifdef NEED_SEMAPHORE
/* Lock Semaphore cuz we're messing with the Timer list and AudioBase */
		SuperLockSemaphore( AudioBase->af_TimerListSemaphore, TRUE);
#endif

#if 0
	if( AudioBase->af_TimerTicksLost > 0 )
	{
		ERR(("HandleTimerSignal: lost audio timer ticks!\n"));
		AudioBase->af_TimerTicksLost = 0;
	}
#endif

/* Check list to see who's next. */
	aevt = (AudioEvent *) FIRSTNODE( &AudioBase->af_TimerList );
TRACEB(TRACE_INT, TRACE_TIMER, ("RunAudioTimer: aevt =  0x%x\n", aevt));

/*
** 940413 Fixed list walker.  The old code relied on NEXTNODE to traverse the list.
** This caused a problem if an instrument had two envelopes and one ended with a FLS
** before the other one.  It would stop the instrument which removed the nextnode
** from the list causing it to leap into outer space.  FIRSTNODE only gets valid nodes.
*/
/* Process all nodes for this time. */
	while ( ISNODE(&AudioBase->af_TimerList, aevt) && AudioTimeLaterThanOrEqual(Time,aevt->aevt_TriggerAt) )
	{
/* Remove and process node.*/
 		UnpostAudioEvent( aevt );         /* Remove from list. */
		aevt->aevt_Perform( aevt );       /* Execute callback function. */
/* Since old one is removed, just keep working on first node. */
		aevt = (AudioEvent *) FIRSTNODE( &AudioBase->af_TimerList );
	}

	ScheduleNextAudioTimerSignal();

#ifdef NEED_SEMAPHORE
		SuperUnlockSemaphore( AudioBase->af_TimerListSemaphore);
#endif
	return 0;
}

/*****************************************************************/
/****** Item Creation ********************************************/
/*****************************************************************/

 /**
 |||	AUTODOC PUBLIC spg/items/cue
 |||	Cue - Audio Cue Item.
 |||
 |||	  Description
 |||
 |||	    This Item type is used to receive completion notification from the
 |||	    audio folio for these kinds of operations:
 |||
 |||	    . sample playback completion
 |||
 |||	    . envelope completion
 |||
 |||	    . audio timer request completion
 |||
 |||	    Each Cue has a signal bit associated with it. Because signals are
 |||	    task-relative, Cues cannot be shared between multiple tasks.
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_CUE_NODE
 |||
 |||	  Create
 |||
 |||	    CreateCue()
 |||
 |||	    CreateItem()
 |||
 |||	  Delete
 |||
 |||	    DeleteCue()
 |||
 |||	    DeleteItem()
 |||
 |||	  Query
 |||
 |||	    GetCueSignal()
 |||
 |||	  Modify
 |||
 |||	    None
 |||
 |||	  Use
 |||
 |||	    AbortTimerCue()
 |||
 |||	    MonitorAttachment()
 |||
 |||	    SignalAtTime()
 |||
 |||	    SleepUntilTime()
 |||
 |||	  Tags
 |||
 |||	    None
 |||
 |||	  See Also
 |||
 |||	    Attachment
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/createcue
 |||	CreateCue - Creates an audio cue.
 |||
 |||	  Synopsis
 |||
 |||	    Item CreateCue (TagArg *tagList)
 |||
 |||	  Description
 |||
 |||	    This macro calls CreateItem() to create an audio cue, which is an item
 |||	    associated with a system signal. A task can get the signal mask of the
 |||	    cue's signal using the GetCueSignal() call.
 |||
 |||	    When a task uses an audio timing call such as SignalAtTime(), it passes
 |||	    the item number of the audio cue to the procedure. The task then calls
 |||	    WaitSignal() to enter wait state, where it waits for the cue's signal
 |||	    at the specified time.
 |||
 |||	    Since each task has its own set signals, Cues cannot be shared among
 |||	    tasks.
 |||
 |||	    Call DeleteCue() to dispose of a Cue item.
 |||
 |||	  Arguments
 |||
 |||	    None
 |||
 |||	  Tags
 |||
 |||	    None
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number of the cue (a positive value) or an
 |||	    error code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in the audio.h V21.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    DeleteCue(), GetCueSignal(), MonitorAttachment(), SignalAtTime(),
 |||	    SleepUntilTime()
 |||
 **/
Item internalCreateAudioCue (AudioCue *acue, TagArg *args)
{
	uint32 tagc, *tagp;
	int32 Result;

	CHECKAUDIOOPEN;

    Result = TagProcessor( acue, args, afi_DummyProcessor, 0);
    if(Result < 0)
    {
    	ERR(("internalCreateAudioCue: TagProcessor failed.\n"));
    	return Result;
    }

TRACEE(TRACE_INT,TRACE_ITEM,("internalCreateAudioCue(0x%x, 0x%lx)\n", acue, args));
	tagp = (uint32 *)args;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
			switch (tagc)
			{
			default:
				if(tagc > TAG_ITEM_LAST)
				{
					ERR (("Unrecognized tag in internalCreateAudioCue - 0x%lx: 0x%lx\n",
					tagc, *tagp++));
					return(AF_ERR_BADTAG);
				}
				tagp++;
			}
		}
	}

    {
        const int32 sig = SuperAllocSignal( 0 );

        if (sig <= 0)
        {
            ERR(("internalCreateAudioCue could not SuperAllocSignal\n"));
            return sig ? sig : AF_ERR_NOSIGNAL;
        }
        acue->acue_Signal = sig;
    }

	acue->acue_Task = CURRENTTASK;
	acue->acue_Event.aevt_InList = NULL;
	acue->acue_Event.aevt_Perform = PerformCueSignal;

DBUG(("internalCreateAudioCue: Cue = 0x%x, Task = 0x%x, Signal = 0x%x\n",
	acue->acue_Event.aevt_Node.n_Item, acue->acue_Task, acue->acue_Signal));

	return (acue->acue_Event.aevt_Node.n_Item);
}

/******************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/deletecue
 |||	DeleteCue - Deletes an audio cue created by CreateCue().
 |||
 |||	  Synopsis
 |||
 |||	    Err DeleteCue (Item Cue)
 |||
 |||	  Description
 |||
 |||	    This macro deletes an audio cue and frees its resources (including its
 |||	    associated signal).
 |||
 |||	  Arguments
 |||
 |||	    Cue                          The item number of the cue to delete.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns zero if successful or an error code (a negative
 |||	    value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in audio.h V21.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    CreateCue()
 |||
 **/
int32 internalDeleteAudioCue (AudioCue *acue)
{

	Item CueOwnerItem;
	Task *OwnerTask;
	int32 Result;

TRACEE(TRACE_INT,TRACE_ITEM,("internalDeleteAudioCue(0x%x)\n", acue ));
#ifdef NEED_SEMAPHORE
/* Lock Semaphore cuz we're messing with the Timer list and AudioBase */
	SuperLockSemaphore( AudioBase->af_TimerListSemaphore, TRUE);
#endif


	CueOwnerItem = acue->acue_Event.aevt_Node.n_Owner;

	if(KernelBase->kb.fn.n_Version > 19)
	{
		OwnerTask = (Task *)LookupItem( CueOwnerItem );
		if (OwnerTask)
		{
			Result = SuperInternalFreeSignal( acue->acue_Signal, OwnerTask );
			if(Result) ERR(("Result of SuperInternalFreeSignal = 0x%x\n", Result));
		}
	}
	else
	{
		if( OWNEDBYCALLER(acue) )
		{
			SuperFreeSignal (acue->acue_Signal);
		}
	}

	UnpostAudioEvent( (AudioEvent *) &acue->acue_Event );
	ScheduleNextAudioTimerSignal();

#ifdef NEED_SEMAPHORE
	SuperUnlockSemaphore( AudioBase->af_TimerListSemaphore);
#endif

	acue->acue_Signal = 0;
TRACER(TRACE_INT, TRACE_TIMER, ("internalDeleteAudioCue returns 0x%x\n", 0));
	return (0);
}

