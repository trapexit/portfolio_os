/* $Id: juggler.c,v 1.33 1994/10/18 21:35:38 peabody Exp $ */
/****************************************************************
**
** Juggler - hierarchical event scheduler
**
** By:  Phil Burk
**
** Copyright (c) 1993, 3DO Company.
** All rights reserved.
** This program is proprietary and confidential.
**
*****************************************************************
** 930705 PLB Traverse list safely in BumpJuggler
** 931207 PLB Added StopCollection to stop children, fixes crash on second song.
** 931208 PLB Fix crash if new score played after other score deleted.
**            Problem was due to sequences being left in active object list.
**            Fixed with StopCollection method
** 940325 PLB Pass StopTime to StopFunction() so that user can chain objects.
** 940804 PLB Added StopTime parameter to stop and abort method calls for Collection
**            This fixed the timing error for MIDI transitions using StopFunction
** 940805 WJB Added autodocs.
**            Made internal functions static.
** 940805 WJB Touched up autodocs.
** 940809 PLB Fixed query of RepeatDelay and StopDelay in GetObjectInfo() for Jugglee
**            Were returning StartDelay by mistake.
** 940906 WJB Fixed \x characters in autodocs.
****************************************************************/
/*
 * O- stop jugglee if playing when destroyed.
 * O- move addition of delays to root jugglee class
 */

#include "types.h"
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "io.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "stdarg.h"
#include "strings.h"
#include "operror.h"
#include "strings.h"
#include "stdio.h"
#include "audio.h"
/* #include "music.h" */
#include "juggler.h"
#include "handy_tools.h"

#include "music_internal.h"     /* package id */


DEFINE_MUSICLIB_PACKAGE_ID(juggler)

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */
#define	DBUGX(x) /* PRT(x) */

#ifdef PARANOID
#define CHECKOBJECT(cob) \
	{ \
		int32 COBResult; \
		COBResult = ValidateObject((COBObject *)cob); \
		if(COBResult) return COBResult; \
	}

#else
#define CHECKOBJECT(cob) /* */
#endif

static int32 BadMethod ( COBObject *Object )
{
	ERR(("Method not supported for object 0x%x\n", Object));
	return COBJ_ERR_NO_METHOD;
}

/* Macro to simplify error checking. */
#define CHECKRESULT(name) \
	if (Result < 0) \
	{ \
		ERR(("Failure in %s: $%x\n", name, Result)); \
		goto cleanup; \
	}

/****************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/defineclass
 |||	DefineClass - Defines a class of objects.
 |||
 |||	  Synopsis
 |||
 |||	    int32 DefineClass( COBClass *Class, COBClass *SuperClass,
 |||	                       int32 DataSize )
 |||
 |||	  Description
 |||
 |||	    ***At this writing, this call is for internal use only, and not available
 |||	    to user tasks.***
 |||
 |||	    This procedure is part of the juggler object-oriented toolbox that
 |||	    supports the various classes of juggler objects.  This procedure defines a
 |||	    new class of objects with the given size.  All methods for this class are
 |||	    inherited from the specified SuperClass.  New methods are added to the
 |||	    class by setting function pointers in the Class structure.
 |||
 |||	    See the chapter "Playing Juggler Objects" in the Portfolio
 |||	    Programmer's Guide for more information about object-oriented
 |||	    programming and the data structures necessary for this procedure.
 |||
 |||	  Arguments
 |||
 |||	    Class                        A pointer to the COBClass data structure for
 |||	                                 the class.
 |||
 |||	    SuperClass                   A pointer to the COBClass data structure for
 |||	                                 the superclass.
 |||
 |||	    DataSize                     A value indicating the size of the new
 |||	                                 class.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    cobj.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    CreateObject(), DestroyObject(), ValidateObject()
 |||
 **/
int32 DefineClass( COBClass *Class, COBClass *SuperClass, int32 DataSize)
{
	int32 Result = 0;

DBUG(( "DefineClass(0x%x, 0x%x, %d )\n", Class, SuperClass, DataSize ));

	if (SuperClass != NULL)
	{
/*		*Class = *SuperClass; */
		bcopy( (void *) SuperClass, (void *) Class, sizeof( COBClass ) );
	}

	Class->Super = SuperClass;
	Class->DataSize = DataSize;

DBUG(( "DefineClass: Class->Term = 0x%x )\n", Class->Term ));
DBUG(( "DefineClass: Class->DataSize = %d )\n", Class->DataSize ));
	return Result;
}

/****************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/validateobject
 |||	ValidateObject - Validates an object.
 |||
 |||	  Synopsis
 |||
 |||	    int32 ValidateObject( COBObject *cob )
 |||
 |||	  Description
 |||
 |||	    This procedure is part of the juggler object-oriented toolbox that
 |||	    supports the various classes of juggler objects.  This procedure returns
 |||	    an error code if the object is not a valid objectthat is, if the COBObject
 |||	    data structure element dedicated to object validity doesn't confirm
 |||	    validity.
 |||
 |||	  Arguments
 |||
 |||	    cob                          A pointer to the COBObject data structure
 |||	                                 for the object.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if the object is valid or an error code (a
 |||	    negative value) if the object isn't valid.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    cobj.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    CreateObject(), DefineClass(), DestroyObject()
 |||
 **/
int32 ValidateObject( COBObject *cob )
{
	if(cob->cob_ValidationKey != VALID_OBJECT_KEY)
	{
		ERR(("ValidateObject: 0x%x is not an object!\n", cob));
		return -1;  /* %Q */
	}
	return 0;
}

/****************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/createobject
 |||	CreateObject - Creates an object of the given class.
 |||
 |||	  Synopsis
 |||
 |||	    COBObject *CreateObject( COBClass *Class )
 |||
 |||	  Description
 |||
 |||	    This procedure is part of the juggler object-oriented toolbox that
 |||	    supports the various classes of juggler objects.  This procedure creates
 |||	    an object of the given class.  Execute the object's Init() method to
 |||	    correctly set up the object.
 |||
 |||	  Arguments
 |||
 |||	    Class                        A pointer to the COBClass data structure for
 |||	                                 the class (currently available:
 |||	                                 &SequenceClass and &CollectionClass as
 |||	                                 defined in juggler.h).
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns a pointer to the COBObject data structure defining
 |||	    the object if successful or an error code (a negative value) if an error
 |||	    occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    cobj.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    DefineClass(), DestroyObject(), ValidateObject()
 |||
 **/
COBObject *CreateObject( COBClass *Class)
{
	COBObject *cob;

DBUG(( "\nCreateObject(0x%x)\n", Class ));
	if(Class->DataSize == 0)
	{
		ERR(("DataSize = 0!\n"));
		return NULL;
	}

	cob = (COBObject *) UserMemAlloc(Class->DataSize, MEMTYPE_FILL); /* JG_MA%00001 */
	if (cob  ==  NULL)
	{
		ERR(("CreateObject: Could not alloc object.\n"));
		return NULL;
	}

	cob->cob_ValidationKey = VALID_OBJECT_KEY;
	cob->Class = Class;
	Class->Init( cob );

DBUG(( "CreateObject: returns 0x%x\n", cob ));
	return cob;
}

/****************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/destroyobject
 |||	DestroyObject - Destroys an object.
 |||
 |||	  Synopsis
 |||
 |||	    int32 DestroyObject( COBObject *Object )
 |||
 |||	  Description
 |||
 |||	    This procedure is part of the juggler object-oriented toolbox that
 |||	    supports the various classes of juggler objects.  This procedure gets rid
 |||	    of an object.  Execute the object's Term() method to close the object,
 |||	    then use this call to get rid of the object by deleting its COBObject data
 |||	    structure and freeing the memory used to store it.
 |||
 |||	  Arguments
 |||
 |||	    Object                       A pointer to the COBObject data structure
 |||	                                 for the object.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    cobj.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    CreateObject(), DefineClass(), ValidateObject()
 |||
 **/
int32 DestroyObject( COBObject *Object)
{
	COBClass *Class;
	int32 Result = 0;

	CHECKOBJECT(Object);

	Class = Object->Class;

DBUG(("DestroyObject(0x%x)\n", Object));
	if (Object == NULL) return COBJ_ERR_NULL_OBJECT;
	Object->Class->Term(Object);
DBUG(("DestroyObject: free object.\n"));
	UserMemFree(Object, Class->DataSize);   /* JG_MF%00001 */
DBUG(("DestroyObject returns 0\n"));
	return Result;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/classes/juggleeclass
 |||	JuggleeClass - Root juggler class.
 |||
 |||	  Description
 |||
 |||	    This is the root class used to derive other juggler classes. It cannot
 |||	    be used as a stand-alone class.
 |||
 |||	  Super Class
 |||
 |||	    None
 |||
 |||	  Methods
 |||
 |||	    !!!
 |||
 |||	  Implementation
 |||
 |||	    V20
 |||
 |||	  Associated Files
 |||
 |||	    juggler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    CollectionClass, SequenceClass
 |||
 **/
COBClass JuggleeClass;
 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/classes/sequenceclass
 |||	SequenceClass - A single sequence of events.
 |||
 |||	  Description
 |||
 |||	    Objects of this class can maintain and playback a single sequence of
 |||	    events.
 |||
 |||	  Super Class
 |||
 |||	    JuggleeClass
 |||
 |||	  Methods
 |||
 |||	    !!!
 |||
 |||	  Implementation
 |||
 |||	    V20
 |||
 |||	  Associated Files
 |||
 |||	    juggler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    CollectionClass
 |||
 **/
COBClass SequenceClass;
 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/classes/collectionclass
 |||	CollectionClass - Multiple parallel sequences and collections.
 |||
 |||	  Description
 |||
 |||	    Objects of this class can maintain and playback in parallel an
 |||	    assembly of sequences and other collections.
 |||
 |||	  Super Class
 |||
 |||	    JuggleeClass
 |||
 |||	  Methods
 |||
 |||	    !!!
 |||
 |||	  Implementation
 |||
 |||	    V20
 |||
 |||	  Associated Files
 |||
 |||	    juggler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    SequenceClass
 |||
 **/
COBClass CollectionClass;

JugglerContext JugglerCon;

static int32 InitJugglee ( Jugglee *Self)
{

	DBUG(("InitJugglee(0x%x)\n", Self));
	return 0;
}

static int32 TermJugglee ( COBObject *Self )
{
	DBUG(("TermJugglee\n"));
	return 0;
}

static int32 PrintJugglee ( Jugglee *Self )
{

	CHECKOBJECT(Self);

	PRT(("Print: Jugglee at 0x%x\n", Self));
	return 0;
}

/****************************************************************************/
static int32 StartJugglee ( Jugglee *Self, Time StartTime, int32 NumRepeats, Jugglee *Parent )
{
	int32 Result=0;

	CHECKOBJECT(Self);

	if( Self->jglr_Active == TRUE)
	{
		StopObject( Self, StartTime );
	}

	Self->jglr_Parent = Parent;
	if (Self->jglr_StartFunction != NULL)
	{
		(*Self->jglr_StartFunction)(Self, StartTime);
	}
	Self->jglr_Active = TRUE;
	Self->jglr_RepeatCount = NumRepeats;

	Self->Class->Rewind(Self, StartTime );
	Self->jglr_StartTime += Self->jglr_StartDelay;
	return Result;
}

/****************************************************************************/
static int32 StopJugglee ( Jugglee *Self, Time StopTime )
{
	int32 Result=0;

	CHECKOBJECT(Self);

	Self->jglr_Active = FALSE;
	if (Self->jglr_StopFunction != NULL)
	{
DBUG(("Stopped at %d\n", StopTime));
		(*Self->jglr_StopFunction)(Self, StopTime);  /* 940325 Pass StopTime */
	}

/* Report DONE to parent for hierarchical nesting. */
	if(Self->jglr_Parent != NULL)
	{
DBUG(("StopJugglee: Report to Parent: 0x%x\n", Self->jglr_Parent ));
		Result = ((Jugglee *)Self->jglr_Parent)->Class->Done( Self->jglr_Parent, StopTime, Self );
	}

	return Result;
}

/****************************************************************************/
static int32 AbortJugglee ( Jugglee *Self, Time StopTime )
{
	int32 Result=0;

	CHECKOBJECT(Self);

	Self->jglr_Active = FALSE;

	return Result;
}

/****************************************************************************/
static int32 RewindJugglee ( Jugglee *Self, Time RewindTime )
{

DBUG(("RewindJugglee: Self = 0x%x, Time = 0x%x\n", Self, RewindTime));
	Self->jglr_StartTime = RewindTime;
	Self->jglr_NextTime = RewindTime;
	Self->jglr_CurrentIndex = 0;

	return 0;
}


/****************************************************************************/
static int32 SetJuggleeInfo( Jugglee *Self, TagArg *TagList )
{
	uint32 tagc, *tagp, temp;

	CHECKOBJECT(Self);

	tagp = (uint32 *)TagList;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
			temp = *tagp++;
			DBUG(("SetJuggleeInfo: Tag = %d, Arg = $%x\n",
					tagc, *tagp));
			switch (tagc)
			{

			case JGLR_TAG_CONTEXT:
				Self->jglr_UserContext = (void *) temp;
				break;
			case JGLR_TAG_START_FUNCTION:
				Self->jglr_StartFunction = (int32 (*)()) temp;
				break;
			case JGLR_TAG_REPEAT_FUNCTION:
				Self->jglr_RepeatFunction = (int32 (*)()) temp;
				break;
			case JGLR_TAG_STOP_FUNCTION:
				Self->jglr_StopFunction = (int32 (*)()) temp;
				break;

			case JGLR_TAG_START_DELAY:
				Self->jglr_StartDelay = (Time) temp;
				break;
			case JGLR_TAG_REPEAT_DELAY:
				Self->jglr_RepeatDelay = (Time) temp;
				break;
			case JGLR_TAG_STOP_DELAY:
				Self->jglr_StopDelay = (Time) temp;
				break;
			case JGLR_TAG_MUTE:
				if (temp)
				{
					Self->jglr_Flags |= JGLR_FLAG_MUTE;
				}
				else
				{
					Self->jglr_Flags &= ~JGLR_FLAG_MUTE;
				}
				break;
			}
		}
	}
	return 0;
}

/****************************************************************************/

static int32 GetJuggleeInfo( Jugglee *Self, TagArg *TagList )
{
	uint32 tagc, *tagp, temp;

	tagp = (uint32 *)TagList;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
DBUG(("GetJuggleeInfo: Tag = %d, Arg = $%x\n", tagc, *tagp));
			temp=0xFFFFFFFF;

			switch (tagc)
			{

			case JGLR_TAG_CONTEXT:
				temp = (uint32) Self->jglr_UserContext;
				break;
			case JGLR_TAG_START_FUNCTION:
				temp = (uint32) Self->jglr_StartFunction;
				break;
			case JGLR_TAG_REPEAT_FUNCTION:
				temp = (uint32) Self->jglr_StartFunction;
				break;
			case JGLR_TAG_STOP_FUNCTION:
				temp = (uint32) Self->jglr_StartFunction;
				break;
			case JGLR_TAG_MANY:
				temp = (uint32) Self->jglr_Many;
				break;
			case JGLR_TAG_START_DELAY:
				temp = (uint32) Self->jglr_StartDelay;
				break;
			case JGLR_TAG_REPEAT_DELAY:
/*				temp = (uint32) Self->jglr_StartDelay; 940809 */
				temp = (uint32) Self->jglr_RepeatDelay;
				break;
			case JGLR_TAG_STOP_DELAY:
/*				temp = (uint32) Self->jglr_StartDelay; 940809 */
				temp = (uint32) Self->jglr_StopDelay;
				break;
			case JGLR_TAG_MUTE:
				temp = ((Self->jglr_Flags & JGLR_FLAG_MUTE) != 0);
				break;
			}
			*tagp++ = temp;
		}
	}
	return 0;
}

/****************************************************************************/
/************* Sequence *****************************************************/
/****************************************************************************/

static int32 InitSequence ( Sequence *Self)
{

DBUG(("InitSequence(0x%x)\n", Self));
	Self->Class->Super->Init(Self);
	return 0;
}

int32 TermSequence ( Sequence *Self )
{

DBUG(("TermSequence(0x%x)\n", Self));
	FreeObject(Self);
	Self->Class->Super->Term(Self);
	return 0;
}

/****************************************************************************/
static int32 FreeSequence ( Sequence *Self )
{
	if(Self->seq_Events != NULL)
	{
		EZMemFree( Self->seq_Events );
		Self->seq_Events = NULL;
	}
	return 0;
}

/****************************************************************************/
static int32 AllocSequence ( Sequence *Self, int32 NumElements )
{
	FreeObject(Self);

	Self->seq_Events = (char *)EZMemAlloc( (NumElements * Self->seq_EventSize ), MEMTYPE_FILL);
	if(Self->seq_Events == NULL)
	{
		ERR(("AllocSequence: allocation failed.\n"));
		return AF_ERR_NOMEM;
	}
	else
	{
		Self->seq_Max = NumElements;
		Self->jglr_Many = 0;
	}

	return 0;
}

/****************************************************************************/
static int32 PrintSequence ( Sequence *Self )
{
	Self->Class->Super->Print(Self);
	PRT(("Sequence: Many = 0x%x\n", Self->jglr_Many));
	PRT(("Sequence: Events at 0x%x\n", Self->seq_Events));
	return 0;
}

#define LaterThan(t1,t2) (((int32)((t1)-(t2))) > 0)
#define TimeYet(t) (!LaterThan((t),JugglerCon.jcon_CurrentTime))

/****************************************************************************/
static int32 StartSequence ( Sequence *Self, Time StartTime, int32 NumRepeats, Jugglee *Parent )
{
	int32 Result;
DBUGX(("StartSequence(0x%x, %d, %d, ...)\n", Self, StartTime, NumRepeats));

	Result = Self->Class->Super->Start(Self, StartTime, NumRepeats, Parent);
	if (Result) goto error;

/* Add to Active Object List */
	AddTail(&JugglerCon.jcon_ActiveObjects, (Node *) Self);

/* Update NextTime in context */
	if(!LaterThan(StartTime, JugglerCon.jcon_NextTime))
	{
		JugglerCon.jcon_NextTime = StartTime;
	}

error:
	return Result;
}

/****************************************************************************/
static int32 StopSequence ( Sequence *Self, Time StopTime)
{
	int32 Result;

/* Remove from list %Q check first! */
DBUGX(("StopSequence, not active 0x%x\n", Self));
	if (Self->jglr_Active)
	{
DBUGX(("Removing 0x%x from active object list.\n", Self));
		RemNode( (Node *) Self );
	}

	Result = Self->Class->Super->Stop(Self, StopTime);
	return Result;
}

/****************************************************************************/
static int32 AbortSequence ( Sequence *Self, Time StopTime)
{
	int32 Result;

/* Remove from list %Q check first! */
	if (Self->jglr_Active)
	{
DBUGX(("Removing 0x%x from active object list.\n", Self));
		RemNode( (Node *) Self );
	}

	Result = Self->Class->Super->Abort(Self, StopTime);
	return Result;
}

/****************************************************************************/
static int32 RewindSequence ( Sequence *Self, Time RewindTime )
{

DBUG(("RewindSequence: Self = 0x%x, Time = %d\n", Self, RewindTime));
	Self->Class->Super->Rewind(Self, RewindTime);
/* Set next time based on first event. */
	Self->jglr_NextTime = (*(Time *) Self->seq_Events) + RewindTime;
DBUG(("RewindSequence: Self->jglr_NextTime = %d\n", Self->jglr_NextTime));

	return 0;
}

/****************************************************************************/
static int32 BumpSequence ( Sequence *Self )
{
	Time *EventPtr;
	int32 Indx;
	int32 Result=0;
	int32 InRange = TRUE;

DBUG(("BumpSequence(0x%x) at %8d\n", Self, JugglerCon.jcon_CurrentTime));

/* Is event index still in range? Sequence could have changed! */
	Indx = Self->jglr_CurrentIndex;
	if (Indx < Self->jglr_Many)
	{
/* Index into array of arbitrarily sized events. */
		EventPtr = (Time *) (Self->seq_Events + (Indx * (Self->seq_EventSize)));
		Self->jglr_NextTime = *EventPtr + Self->jglr_StartTime;

/* Loop through all events that are current. */
		while (TimeYet(Self->jglr_NextTime) && InRange)
		{
DBUG(("BumpSequence: Self->jglr_CurrentIndex = %4d,", Self->jglr_CurrentIndex));
DBUG((" Self->jglr_NextTime = %8d\n", Self->jglr_NextTime));
/* If current, call interpreter. */
			Result = (*Self->seq_InterpFunction)( Self, EventPtr,
				Self->jglr_UserContext);
			if (Result) goto error;
			Indx++;
			if(Indx < Self->jglr_Many)
			{
/* Just keep going. */
				Self->jglr_CurrentIndex = Indx;
/* Get Timestamp of next Event */
				EventPtr = (Time *) (((char *)EventPtr) + Self->seq_EventSize);
				Self->jglr_NextTime = *EventPtr + Self->jglr_StartTime;
			}
			else
			{
				InRange = FALSE;
			}
		}
	}
	else
	{
		InRange = FALSE;
	}

	if(!InRange)
	{
/* Do we repeat? */
		if(--(Self->jglr_RepeatCount) > 0)
		{
			Self->jglr_NextTime += Self->jglr_RepeatDelay;
			(*Self->Class->Rewind)(Self, Self->jglr_NextTime);
			if (Self->jglr_RepeatFunction != NULL)
			{
				(*Self->jglr_RepeatFunction)(Self, Self->jglr_NextTime);
			}

DBUG(("Rewound: Self->jglr_NextTime = %8d\n", Self->jglr_NextTime));
		}
		else
		{
/* Must be time to stop */
			(*Self->Class->Stop)(Self, Self->jglr_NextTime + Self->jglr_StopDelay);
		}
	}


/* Update NextTime in context */
	if(Self->jglr_Active &&
		( LaterThan( JugglerCon.jcon_NextTime, Self->jglr_NextTime )))
	{
DBUG(("BumpSequence: Sets context next time to %d\n", Self->jglr_NextTime));
		JugglerCon.jcon_NextTime = Self->jglr_NextTime;
	}
error:
	return Result;
}

/****************************************************************************/
static int32 SetSequenceInfo( Sequence *Self, TagArg *TagList )
{
	uint32 tagc, *tagp;
	void *temp;

	Self->Class->Super->SetInfo(Self, TagList);

	tagp = (uint32 *)TagList;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
			temp = (void *) *tagp++;
DBUG(("SetSequenceInfo: Tag = %d, Arg = $%x\n", tagc, *tagp));
			switch (tagc)
			{
			case JGLR_TAG_INTERPRETER_FUNCTION:
				Self->seq_InterpFunction = (int32 (*)()) temp;
				break;

			case JGLR_TAG_MAX:
				Self->seq_Max = (uint32) temp;
				break;

			case JGLR_TAG_MANY:
				Self->jglr_Many = (uint32) temp;
				break;

			case JGLR_TAG_EVENTS:
				Self->seq_Events = (char *) temp;
				break;

			case JGLR_TAG_EVENT_SIZE:
				Self->seq_EventSize = (uint32) temp;
				break;
			}
		}
	}
	return 0;
}

/****************************************************************************/
static int32 GetSequenceInfo( Sequence *Self, TagArg *TagList )
{
	uint32 tagc, *tagp;

	Self->Class->Super->GetInfo(Self, TagList);

	tagp = (uint32 *)TagList;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
DBUG(("GetSequenceInfo: Tag = %d, Arg = $%x\n", tagc, *tagp));
			switch (tagc)
			{

			case JGLR_TAG_INTERPRETER_FUNCTION:
				*tagp++ = (uint32) ((int32) Self->seq_InterpFunction);
				break;

			case JGLR_TAG_MAX:
				*tagp++ = (uint32) Self->seq_Max;
				break;

			case JGLR_TAG_EVENTS:
				*tagp++ = (uint32) Self->seq_Events;
				break;

			case JGLR_TAG_EVENT_SIZE:
				*tagp++ = (uint32) Self->seq_EventSize;
				break;

/* Don't overwrite with -1 cuz we would clobber result of superclass. */
			default:
				tagp++;
			}
		}
	}
	return 0;
}

/****************************************************************************/
/************* Collection *****************************************************/
/****************************************************************************/

static int32 InitCollection ( Collection *Self)
{

	DBUG(("InitCollection(0x%x)\n", Self));
	Self->Class->Super->Init(Self);
	InitList( &Self->col_Children, "Children" );
	return 0;
}

/****************************************************************************/
static int32 PrintCollection ( Collection *Self )
{
	Node *nd;
	PlaceHolder *plch;
	Jugglee *Child;

	Self->Class->Super->Print(Self);
	nd = FirstNode( &Self->col_Children );
	while (ISNODE(&Self->col_Children, nd))
	{
			plch = (PlaceHolder *)nd;
			Child = (Jugglee *) plch->plch_Thing;
			PRT(("    Child at 0x%x\n", Child));
			nd = NextNode(nd);
	}

	return 0;
}

/****************************************************************************/
static int32 AddCollection( Collection *Self, Jugglee *Child, int32 NumRepeats )
{
	PlaceHolder *plch;

/* Allocate PlaceHolder */
	plch = (PlaceHolder *) UserMemAlloc( sizeof(PlaceHolder), MEMTYPE_FILL ); /* JG_MA%00002 */
	if (plch == NULL) return AF_ERR_NOMEM;  /* %Q not AF */
DBUG(("AddCollection: Child = 0x%x\n", Child));
	plch->plch_Thing = Child;
	plch->plch_NumRepeats =  NumRepeats;

	AddTail( &Self->col_Children, (Node *) plch );
	Self->jglr_Many++;
	return 0;
}

/****************************************************************************/
static int32 GetNthFromCollection( Collection *Self, int32 N, Jugglee **pChild)
{
	PlaceHolder *plch = NULL;
	Node *nd;

	nd = FirstNode( &Self->col_Children );
	while (1)
	{
		if(ISNODE(&Self->col_Children, nd))
		{
			if (N == 0) break;
			nd = NextNode(nd);
			N--;
		}
		else
		{
			return -1;
		}

	}
	plch = (PlaceHolder *)nd;
	*pChild = (Jugglee *) plch->plch_Thing;
	return 0;
}

/****************************************************************************/
static int32 RemoveNthFromCollection( Collection *Self, int32 N)
{
	Node *nd;

	nd = FirstNode( &Self->col_Children );
	while (1)
	{
		if(ISNODE(&Self->col_Children, nd))
		{
			if (N == 0) break;
			nd = NextNode(nd);
			N--;
		}
		else
		{
			return -1;
		}

	}

	RemNode( nd );
	UserMemFree( nd, sizeof(PlaceHolder));  /* JG_MF%00002 */
	Self->jglr_Many--;
	return 0;
}

/****************************************************************************/
static int32 colStartChildren( Collection *Self, Time StartTime )
{
	int32 Result = 0, numrep;
	PlaceHolder *plch;
	Jugglee *jglr;

/* Start all children in parallel. Parallel mode. */
	Self->col_Pending = 0;
	plch = (PlaceHolder *) FirstNode( &Self->col_Children );
	while (IsNode( &Self->col_Children, plch))
	{
/* Get number of repeats from placeholder. */
		numrep = plch->plch_NumRepeats;
		jglr = plch->plch_Thing;
DBUG(("StartCollection: jglr = 0x%x\n", jglr));
		CHECKOBJECT(jglr);

/* Polymorphous tasker. */
		Result = StartObject(jglr, StartTime, numrep, Self );
		if (Result) goto error;
		Self->col_Pending++;
		plch = (PlaceHolder *)NextNode((Node *)plch);
	}
error:
	return Result;
}

/****************************************************************************/
static int32 colAbortChildren ( Collection *Self, Time StopTime)
{
	int32 Result;
	PlaceHolder *plch;
	Jugglee *jglr;

DBUG(("colAbortChildren(0x%x, %d, %d, ...)\n", Self, StopTime ));
	Result = 0;

/* Traverse list of children. Parallel mode. */
	plch = (PlaceHolder *) FirstNode( &Self->col_Children );
	while (IsNode( &Self->col_Children, plch))
	{
		jglr = plch->plch_Thing;
		Result = AbortObject(jglr, StopTime );
		CHECKRESULT("AbortObject");
		plch = (PlaceHolder *) NextNode( (Node *) plch );
	}

cleanup:
	return Result;
}
/****************************************************************************/
static int32 DoneCollection ( Collection *Self, Time DoneTime,  Jugglee *Child )
{
	int32 Result = 0;

	if(--Self->col_Pending <= 0)
	{
/* Do we repeat? */
		if(--(Self->jglr_RepeatCount) > 0)
		{
			DoneTime += Self->jglr_RepeatDelay;
			(*Self->Class->Rewind)(Self, DoneTime);
			if (Self->jglr_RepeatFunction != NULL)
			{
				(*Self->jglr_RepeatFunction)(Self, DoneTime);
			}
DBUG(("Restarting a collection from DoneCollection.\n"));
			Result = colStartChildren( Self, DoneTime );
		}
		else
		{
/* Must be time to stop */
			(*Self->Class->Stop)(Self, DoneTime + Self->jglr_StopDelay);
		}

	}
	return Result;
}

/****************************************************************************/
static int32 StartCollection ( Collection *Self, Time StartTime, int32 NumRepeats, Jugglee *Parent )
{
	int32 Result;

DBUG(("StartCollection(0x%x, %d, %d, ...)\n", Self, StartTime, NumRepeats));

	Result = Self->Class->Super->Start(Self, StartTime, NumRepeats, Parent);
	if (Result) goto error;

	Result = colStartChildren( Self, StartTime );

error:
	return Result;
}


/****************************************************************************/
static int32 StopCollection ( Collection *Self, Time StopTime)
{
	int32 Result;

DBUG(("StopCollection(0x%x, %d, %d, ...)\n", Self, StopTime ));

/*
** Don't just stop them because that will cause a DONE message which will
** recurse back to StopCollection.
*/
	Result = colAbortChildren( Self, StopTime );
	if (Result) goto error;

	Result = Self->Class->Super->Stop(Self, StopTime); /* 940804 Added StopTime */

error:
	return Result;
}

/****************************************************************************/
static int32 AbortCollection ( Collection *Self, Time StopTime)
{
	int32 Result;

DBUG(("AbortCollection(0x%x, %d, %d, ...)\n", Self, StopTime ));

	Result = colAbortChildren( Self, StopTime );
	if (Result) goto error;

	Result = Self->Class->Super->Abort(Self, StopTime); /* 940804 Added StopTime */
error:
	return Result;
}

/****************************************************************************/
/******** Juggler ***********************************************************/
/****************************************************************************/


 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/initjuggler
 |||	InitJuggler - Initializes the juggler mechanism for controlling events.
 |||
 |||	  Synopsis
 |||
 |||	    int32 InitJuggler( void )
 |||
 |||	  Description
 |||
 |||	    This procedure initializes the juggler scheduling system for controlling
 |||	    arbitrary events.  Note that the juggler must be initialized before
 |||	    calling any object-oriented, juggler, or MIDI playback procedures.
 |||
 |||	    See the chapter "Playing Juggler Objects" in the Portfolio
 |||	    Programmer's Guide for more information about the juggler and how it
 |||	    works.
 |||
 |||	  Arguments
 |||
 |||	    None.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    juggler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    TermJuggler(), BumpJuggler()
 |||
 **/
int32 InitJuggler( void )
{
	int32 Result;

    PULL_MUSICLIB_PACKAGE_ID(juggler);

/* Define Jugglee Classe. */
	Result = DefineClass ( &JuggleeClass , 0, sizeof(Jugglee));
	CHECKRESULT( "DefineClass" );
	JuggleeClass.Init = InitJugglee;
	JuggleeClass.Term = TermJugglee;
	JuggleeClass.Print = PrintJugglee;
	JuggleeClass.Start = StartJugglee;
	JuggleeClass.Stop = StopJugglee;
	JuggleeClass.Abort = AbortJugglee;
	JuggleeClass.Rewind = RewindJugglee;
	JuggleeClass.SetInfo = SetJuggleeInfo;
	JuggleeClass.GetInfo = GetJuggleeInfo;

/* Define Sequence Classe. */
	Result = DefineClass ( &SequenceClass , &JuggleeClass, sizeof(Sequence));
	CHECKRESULT( "DefineClass" );
	SequenceClass.Init = InitSequence;
	SequenceClass.Print = PrintSequence;
	SequenceClass.Start = StartSequence;
	SequenceClass.Stop = StopSequence;
	SequenceClass.Abort = AbortSequence;
	SequenceClass.Rewind = RewindSequence;
	SequenceClass.Bump = BumpSequence;
	SequenceClass.Alloc = AllocSequence;
	SequenceClass.Free = FreeSequence;
	SequenceClass.SetInfo = SetSequenceInfo;
	SequenceClass.GetInfo = GetSequenceInfo;

/* Define Collection Classe. */
	Result = DefineClass ( &CollectionClass , &JuggleeClass, sizeof(Collection));
	CHECKRESULT( "DefineClass" );
	CollectionClass.Init = InitCollection;
	CollectionClass.Print = PrintCollection;
	CollectionClass.Start = StartCollection;
	CollectionClass.Stop = StopCollection;
	CollectionClass.Add = AddCollection;
	CollectionClass.Abort = AbortCollection;
	CollectionClass.GetNthFrom = GetNthFromCollection;
	CollectionClass.RemoveNthFrom = RemoveNthFromCollection;
	CollectionClass.Done = DoneCollection;

	InitList( &JugglerCon.jcon_ActiveObjects, "ActiveObjects" );
	JugglerCon.jcon_NextTime = GetAudioTime();
	JugglerCon.jcon_CurrentTime = GetAudioTime();

cleanup:
	return Result;
}

/****************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/termjuggler
 |||	TermJuggler - Terminates the juggler mechanism for controlling events.
 |||
 |||	  Synopsis
 |||
 |||	    int32 TermJuggler( void )
 |||
 |||	  Description
 |||
 |||	    This procedure terminates the juggler, which eliminates support for
 |||	    object-oriented calls, juggler playback, and MIDI score playback.  A task
 |||	    should call this procedure when finished with the juggler.
 |||
 |||	  Arguments
 |||
 |||	    None.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    juggler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    InitJuggler(), BumpJuggler()
 |||
 **/
int32 TermJuggler( void )
{
	return 0;
}

/*****************************************************************************
** Give Juggler an opportunity to execute all of its active objects.
** Returns 1 if list is empty.
*****************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/bumpjuggler
 |||	BumpJuggler - Bumps the juggler data-structure index.
 |||
 |||	  Synopsis
 |||
 |||	    int32 BumpJuggler( Time CurrentTime, Time *NextTime,
 |||	                       int32 CurrentSignals, int32 *NextSignals )
 |||
 |||	  Description
 |||
 |||	    This procedure provides the juggler with a current time value.  The
 |||	    juggler checks unexecuted events in any currently active sequences or jobs
 |||	    to see if their time value is less than or equal to the current time
 |||	    value.  If so, it executes the event.  The juggler also checks all
 |||	    unexecuted events in all currently active sequences or jobs to see which
 |||	    event should be executed next.  It writes the time of that event into the
 |||	    NextTime variable so the calling task can wait until that time to bump the
 |||	    juggler once again.  The time value supplied is an arbitrary value in
 |||	    ticks and must be supplied from an external source.  Typically, the time
 |||	    value comes from the audio clock.
 |||
 |||	    ***At this writing, BumpJuggler() does not work with signals, so ignore
 |||	    this paragraph.***This procedure may also work with signals specified for
 |||	    each event.  The current signal mask lists current signal bits.  The
 |||	    juggler checks unexecuted events for events associated with current signal
 |||	    bits, and executes those events.  It writes the next set of signals to
 |||	    wait for in the NextSignals signal mask.  To use this feature properly,
 |||	    CurrentSignals should be set to 0 the first time you call BumpJuggler().
 |||	    For subsequent calls, you would have your program wait for the signals to
 |||	    be set (with WaitSignal()), or you would wait until the NextTime time is
 |||	    reached.  When either the signal arrives or the time occurs, you would
 |||	    call BumpJuggler() again with the new time and any signals that have
 |||	    arrived in the wait interval.
 |||
 |||	    See the example program "playmf.c" to see how BumpJuggler() is
 |||	    used.
 |||
 |||	  Arguments
 |||
 |||	    CurrentTime                  Value indicating the current time that the
 |||	                                 procedure is called.
 |||
 |||	    NextTime                     Pointer to a value where this procedure
 |||	                                 writes the time that it should be called
 |||	                                 again.
 |||
 |||	    CurrentSignals               The current signal mask.
 |||
 |||	    NextSignals                  Pointer to a signal mask where this
 |||	                                 procedure writes the signal bits for which
 |||	                                 the task should wait before making this call
 |||	                                 again.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.  If everything is finished or there are no active
 |||	    objects, this procedure returns 1.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    juggler.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    InitJuggler(), TermJuggler()
 |||
 **/
int32 BumpJuggler( Time CurrentTime, Time *NextTime, int32 CurrentSignals, int32 *NextSignals)
{
	Jugglee *jglr, *NextJglr;
	int32 Result = 0;

	JugglerCon.jcon_CurrentTime = CurrentTime;
	JugglerCon.jcon_CurrentSignals = CurrentSignals;
	JugglerCon.jcon_NextTime += 1000000;

/* Traverse list of active objects. */
	jglr = (Jugglee *)FirstNode( &JugglerCon.jcon_ActiveObjects );
	while (IsNode( &JugglerCon.jcon_ActiveObjects, jglr))
	{
DBUG(("BumpJuggler: jglr = 0x%x\n", jglr));
/* Polymorphous tasker. */
		CHECKOBJECT(jglr);
		NextJglr = (Jugglee *)NextNode((Node *)jglr); /* 930705, jglr might get removed */
		Result = jglr->Class->Bump(jglr);
		if (Result) goto done;
		jglr = NextJglr;
	}

done:
	*NextTime = JugglerCon.jcon_NextTime;
	*NextSignals = JugglerCon.jcon_NextSignals;

	if (Result == 0)
	{
		if (IsEmptyList( &JugglerCon.jcon_ActiveObjects))
		{
			Result = 1;
		}
	}

	return Result;
}


/* -------------------- cobj macro docs */

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/printobject
 |||	PrintObject - Prints debugging information about an object.
 |||
 |||	  Synopsis
 |||
 |||	    Err PrintObject( CObject *obj )
 |||
 |||	  Description
 |||
 |||	    This is a method macro that is part of the juggler object-oriented
 |||	    toolbox.  An object's method can be called explicitly through the
 |||	    class structure function pointer or with one of the defined method macros.
 |||
 |||	    This method macro prints debugging information about an object.  The
 |||	    information includes a pointer to the object.
 |||
 |||	  Arguments
 |||
 |||	    obj                          A pointer to the CObject data structure for
 |||	                                 the object.
 |||
 |||	  Return Value
 |||
 |||	    This macro returns 0 if successful or an error code (a negative value) if
 |||	    an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in cobj.h V20.
 |||
 |||	  Associated Files
 |||
 |||	    cobj.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    SetObjectInfo(), GetObjectInfo(), StartObject(), StopObject(),
 |||	    AllocObject(), FreeObject(), GetNthFromObject(), RemoveNthFromObject()
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/setobjectinfo
 |||	SetObjectInfo - Sets values in the object based on tag args.
 |||
 |||	  Synopsis
 |||
 |||	    Err SetObjectInfo( CObject *obj, TagArg *tags )
 |||
 |||	  Description
 |||
 |||	    This is a method macro that is part of the juggler object-oriented
 |||	    toolbox.  An object's method can be called explicitly through the
 |||	    class structure function pointer, or with one of the defined method
 |||	    macros.
 |||
 |||	    This method macro sets the values in the object based on the supplied tag
 |||	    arguments.  Valid tag arguments are as follows:Tags for Jugglee SuperClass
 |||
 |||	    !!! the formatting of the following paragraph is totally hosed:
 |||
 |||	    As the jugglee is the superclass for the other juggler classes, its
 |||	    methods are inherited by other juggler classes.  The following tags are
 |||	    for SetObjectInfo()for the jugglee superclass:JGLR_TAG_CONTEXT
 |||	    <usercontext>  User-specified context; this may be a pointer to almost
 |||	    anything.  It is passed to the Interpreter function. JGLR_TAG_START_DELAY
 |||	    <ticks> The delay interval before starting this object.  This is used to
 |||	    stagger the execution of parallel objects within a collection for canon.
 |||	    JGLR_TAG_REPEAT_DELAY <ticks>  The time to wait between repetitions.
 |||	    JGLR_TAG_STOP_DELAY <ticks>  The time to wait after stopping.
 |||	    JGLR_TAG_START_FUNCTION <*function(object, time)>  This function is called
 |||	    before executing the first element of the object.JGLR_TAG_REPEAT_FUNCTION
 |||	    <*function(object, time)>,JGLR_TAG_STOP_FUNCTION <*function(object,
 |||	    time)>,JGLR_TAG_MUTE <flag>  If the flag is true, the object will be muted
 |||	    when played.  The Interpreter function must look at the mute flag.Tags for
 |||	    Sequence
 |||
 |||	    A sequence contains an array of events that are to be executed over time.
 |||	    An event consists of a timestamp, followed by a user-defined, fixed-size
 |||	    data field.  A sequence keeps track of time, advances through the events
 |||	    in its array, and calls a user-defined Interpreter function to
 |||	    "play" the event.
 |||
 |||	    Sequences will typically contain arrays of MIDI messages from a MIDI file.
 |||	    Sequences may also contain other things; these other things each require a
 |||	    different Interpreter function.  The possible sequence contents and their
 |||	    functions are as follows:FunctionPointers, Data    schedule arbitrary
 |||	    functions.Cel, Corners    draw this cel.Knob, Value    tweak a knob for
 |||	    timbral sequences.Collections    schedule scores at a very high level.
 |||
 |||	    The following are tags for a sequence:JGLR_TAG_INTERPRETER_FUNCTION
 |||	    Specifies the user-defined function to be called for interpreting events
 |||	    in a sequence.  This function is called at the time specified in the
 |||	    event.  When the function is called, it is passed a pointer to one of your
 |||	    events.  It is also passed a pointer to your context data structure that
 |||	    was set using JGLR_TAG_CONTEXT and SetObjectInfo().  The function
 |||	    prototype for interpreting events is given below:int32 InterpretEvent(
 |||	    Sequence *SeqPtr, void *CurrentEvent, void *UserContext)JGLR_TAG_MAX  Sets
 |||	    the maximum number of events that can be used.JGLR_TAG_MANY  Sets the
 |||	    actual number of events that can be used.JGLR_TAG_EVENTS <void *Events>
 |||	    Sets a pointer to your event array.JGLR_TAG_EVENT_SIZE <size_in_bytes>
 |||	    Sets the size of a single event in bytes so that an index into an array
 |||	    can be constructed.Other Tags
 |||
 |||	    Other tag arguments are listed below:
 |||
 |||	    JGLR_TAG_SELECTOR_FUNCTION
 |||
 |||	    JGLR_TAG_AUTO_DELETE
 |||
 |||	    JGLR_TAG_DURATION
 |||
 |||	  Arguments
 |||
 |||	    obj                          A pointer to the CObject data structure for
 |||	                                 the object.
 |||
 |||	    tags                         A pointer to the TagArg list.
 |||
 |||	  Return Value
 |||
 |||	    This macro returns 0 if successful or an error code (a negative value) if
 |||	    an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in cobj.h V20.
 |||
 |||	  Associated Files
 |||
 |||	    cobj.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    GetObjectInfo(), StartObject(), StopObject(), AllocObject(), FreeObject(),
 |||	    GetNthFromObject(), RemoveNthFromObject(), PrintObject()
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/getobjectinfo
 |||	GetObjectInfo - Gets the current settings of an object.
 |||
 |||	  Synopsis
 |||
 |||	    Err GetObjectInfo( CObject *obj, TagArg *tags )
 |||
 |||	  Description
 |||
 |||	    This is a method macro that is part of the juggler object-oriented
 |||	    toolbox.  An object's method can be called explicitly through the
 |||	    class structure function pointer, or with one of the defined method
 |||	    macros.  This method macro replaces the TagArg values in the specified tag
 |||	    arg list with the values currently defined for the object.  Use this
 |||	    procedure to get information about the current state of an object.
 |||
 |||	    See SetObjectInfo() for a list of the tag args supported.
 |||
 |||	  Arguments
 |||
 |||	    obj                          A pointer to the CObject data structure for
 |||	                                 the object.
 |||
 |||	    tags                         A pointer to the TagArg list.
 |||
 |||	  Return Value
 |||
 |||	    This macro returns 0 if successful or an error code (a negative value) if
 |||	    an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in cobj.h V20.
 |||
 |||	  Associated Files
 |||
 |||	    cobj.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    SetObjectInfo(), StopObject(), AllocObject(), FreeObject(),
 |||	    GetNthFromObject(), RemoveNthFromObject(), PrintObject(), StartObject()
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/startobject
 |||	StartObject - Starts an object so the juggler will play it.
 |||
 |||	  Synopsis
 |||
 |||	    Err StartObject( CObject *obj, uint32 time, int32 nrep,
 |||	                     CObject *par )
 |||
 |||	  Description
 |||
 |||	    This is a method macro that is part of the juggler object-oriented
 |||	    toolbox.  An object's method can be called explicitly through the
 |||	    class structure function pointer or with one of the defined method macros.
 |||
 |||	    This method macro starts an object at the given time.  Once an object is
 |||	    started, it's added to the active object list so that the juggler will
 |||	    play it when the juggler is bumped.  The "nrep" argument sets an
 |||	    object to be played a set number of repetitions.
 |||
 |||	  Arguments
 |||
 |||	    obj                          A pointer to the CObject data structure for
 |||	                                 the object.
 |||
 |||	    time                         A value indicating the time in audio clock
 |||	                                 ticks to start the object.
 |||
 |||	    nrep                         A value indicating the number of occurrences
 |||	                                 of the object.
 |||
 |||	    par                          A pointer to the CObject data structure that
 |||	                                 is the parent of this object.
 |||
 |||	  Return Value
 |||
 |||	    This macro returns 0 if successful or an error code (a negative value) if
 |||	    an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in cobj.h V20.
 |||
 |||	  Associated Files
 |||
 |||	    cobj.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    SetObjectInfo(), GetObjectInfo(), StopObject(), AllocObject(),
 |||	    FreeObject(), GetNthFromObject(), RemoveNthFromObject(), PrintObject()
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/stopobject
 |||	StopObject - Stops an object so the juggler won't play it.
 |||
 |||	  Synopsis
 |||
 |||	    int32 StopObject( CObject *obj, uint32 time )
 |||
 |||	  Description
 |||
 |||	    This is a method macro that is part of the juggler object-oriented
 |||	    toolbox.  An object's method can be called explicitly through the
 |||	    class structure function pointer, or with one of the defined method
 |||	    macros.
 |||
 |||	    This method macro stops an object at the specified time (measured in audio
 |||	    clock ticks).  Once an object is stopped, it's removed from the active
 |||	    object list, and will no longer be played by the juggler when the juggler
 |||	    is bumped.
 |||
 |||	  Arguments
 |||
 |||	    obj                          A pointer to the CObject data structure for
 |||	                                 the object.
 |||
 |||	    time                         A value indicating the time to stop the
 |||	                                 object.
 |||
 |||	  Return Value
 |||
 |||	    This macro returns 0 if successful or an error code (a negative value) if
 |||	    an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in cobj.h V20.
 |||
 |||	  Associated Files
 |||
 |||	    cobj.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    SetObjectInfo(), GetObjectInfo(), StartObject(), AllocObject(),
 |||	    FreeObject(), GetNthFromObject(), RemoveNthFromObject(), PrintObject()
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/abortobject
 |||	AbortObject - Abnormally stops a juggler object.
 |||
 |||	  Synopsis
 |||
 |||	    int32 AbortObject( CObject *obj, Time stopTime )
 |||
 |||	  Description
 |||
 |||	    This is a method macro that is part of the juggler object-oriented
 |||	    toolbox.  An object's method can be called explicitly through the
 |||	    class structure function pointer, or with one of the defined method
 |||	    macros.
 |||
 |||	    This method macro abnormally stops an object at the specified time
 |||	    (measured in audio clock ticks). Once an object is stopped, it's
 |||	    removed from the active object list, and will no longer be played by
 |||	    the juggler when the juggler is bumped.
 |||
 |||	    !!! how does this differ from StopObject().
 |||	        . no completion message sent for AbortObject() ?
 |||
 |||	  Arguments
 |||
 |||	    obj                          A pointer to the CObject data structure for
 |||	                                 the object.
 |||
 |||	    stopTime                     A value indicating the time to stop the
 |||	                                 object.
 |||
 |||	  Return Value
 |||
 |||	    This macro returns 0 if successful or an error code (a negative value) if
 |||	    an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in cobj.h V20.
 |||
 |||	  Associated Files
 |||
 |||	    cobj.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    StopObject(), SetObjectInfo(), GetObjectInfo(), StartObject(),
 |||	    AllocObject(), FreeObject(), GetNthFromObject(),
 |||	    RemoveNthFromObject(), PrintObject()
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/allocobject
 |||	AllocObject - Allocates memory for an object.
 |||
 |||	  Synopsis
 |||
 |||	    Err AllocObject( CObject *obj, int32 n )
 |||
 |||	  Description
 |||
 |||	    This is a method macro that is part of the juggler object-oriented
 |||	    toolbox.  An object's method can be called explicitly through the
 |||	    class structure function pointer, or with one of the defined method
 |||	    macros.
 |||
 |||	    This method macro allocates memory for an object.  It checks the amount of
 |||	    RAM required for the elements of the specified object, and then uses the
 |||	    argument n to see how many of those elements exist and need memory
 |||	    allocation.  This procedure is typically used for a sequence object, in
 |||	    which case n  specifies how many events must be stored in the sequence.
 |||
 |||	  Arguments
 |||
 |||	    obj                          A pointer to the CObject data structure for
 |||	                                 the object.
 |||
 |||	    n                            The number of elements that must be stored
 |||	                                 in memory.
 |||
 |||	  Return Value
 |||
 |||	    This macro returns 0 if successful or an error code (a negative value) if
 |||	    an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in cobj.h V20.
 |||
 |||	  Associated Files
 |||
 |||	    cobj.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    AllocObject(), FreeObject(), GetNthFromObject(), GetObjectInfo(),
 |||	    PrintObject(), RemoveNthFromObject(), SetObjectInfo(), StartObject(),
 |||	    StopObject()
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/freeobject
 |||	FreeObject - Frees memory allocated for an object.
 |||
 |||	  Synopsis
 |||
 |||	    Err FreeObject( CObject *obj )
 |||
 |||	  Description
 |||
 |||	    This is a method macro that is part of the juggler object-oriented
 |||	    toolbox.  An object's method can be called explicitly through the
 |||	    class structure function pointer, or with one of the defined method
 |||	    macros.
 |||
 |||	    This method macro frees memory allocated for the object using
 |||	    AllocObject().
 |||
 |||	  Arguments
 |||
 |||	    obj                          A pointer to the CObject data structure for
 |||	                                 the object.
 |||
 |||	  Return Value
 |||
 |||	    This macro returns 0 if successful or an error code (a negative value) if
 |||	    an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in cobj.h V20.
 |||
 |||	  Associated Files
 |||
 |||	    cobj.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    SetObjectInfo(), GetObjectInfo(), StopObject(), AllocObject(),
 |||	    GetNthFromObject(), RemoveNthFromObject(), PrintObject(), StartObject()
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/getnthfromobject
 |||	GetNthFromObject - Gets the nth element of a collection.
 |||
 |||	  Synopsis
 |||
 |||	    Err GetNthFromObject( CObject *obj, int32 n,
 |||	                          (void**) ptr )
 |||
 |||	  Description
 |||
 |||	    This is a method macro that is part of the juggler object-oriented
 |||	    toolbox.  An object's method can be called explicitly through the
 |||	    class structure function pointer, or with one of the defined method
 |||	    macros.
 |||
 |||	    This method macro returns a pointer to the nth element of a collection so
 |||	    that a task can find out what elements are included in the collection.
 |||	    Note that element numbering within a collection starts at 0.
 |||
 |||	  Arguments
 |||
 |||	    obj                          A pointer to the CObject data structure for
 |||	                                 the object.
 |||
 |||	    n                            A value indicating the element to get,
 |||	                                 starting at 0.
 |||
 |||	    ptr                          A variable in which to store a pointer to
 |||	                                 the element found.
 |||
 |||	  Return Value
 |||
 |||	    This macro returns 0 if successful or an error code (a negative value) if
 |||	    an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in cobj.h V20.
 |||
 |||	  Associated Files
 |||
 |||	    cobj.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    SetObjectInfo(), GetObjectInfo(), StopObject(), AllocObject(),
 |||	    FreeObject(), RemoveNthFromObject(), PrintObject(), StartObject()
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/juggler/functions/removenthfromobject
 |||	RemoveNthFromObject - Removes the nth element of a collection.
 |||
 |||	  Synopsis
 |||
 |||	    Err RemoveNthFromObject( CObject *obj, int32 n )
 |||
 |||	  Description
 |||
 |||	    This is a method macro that is part of the juggler object-oriented
 |||	    toolbox.  An object's method can be called explicitly through the
 |||	    class structure function pointer or with one of the defined method macros.
 |||	     This method macro removes the reference to the nth element (a sequence or
 |||	    a collection) of a collection.  Numbering starts from 0.
 |||
 |||	    Note that using this procedure to remove a sequence or collection from a
 |||	    higher collection doesn't delete the sequence or collection, but
 |||	    merely removes its reference in the higher collection so that the element
 |||	    is no longer part of the collection.
 |||
 |||	  Arguments
 |||
 |||	    obj                          A pointer to the CObject data structure for
 |||	                                 the object.
 |||
 |||	    n                            A value indicating the element to remove,
 |||	                                 starting at 0.
 |||
 |||	  Return Value
 |||
 |||	    This macro returns 0 if successful or an error code (a negative value) if
 |||	    an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in cobj.h V20.
 |||
 |||	  Associated Files
 |||
 |||	    cobj.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    SetObjectInfo(), GetObjectInfo(), StopObject(), AllocObject(),
 |||	    FreeObject(), GetNthFromObject(), PrintObject(), StartObject()
 |||
 **/
