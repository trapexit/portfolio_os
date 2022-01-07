/******************************************************************************
**
**  $Id: audio_probe.c,v 1.6 1994/12/01 05:42:29 phil Exp $
**
**  Audio Probes allow you to read outputs from DSP instruments
**
**  By: Phil Burk
**
**  Copyright (c) 1994, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  940923 PLB  Created.
**  940927 WJB  Cleaned up a bit (reduced code by 24 bytes).
**  940927 WJB  Improved tag trap in CreateProbe().
**
**  Initials:
**
**  PLB: Phil Burk (phil)
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include "audio_internal.h"

/* Macros for debugging. */
#define DBUG(x)   /* PRT(x) */

/******************************************************************/
/***** USER MODE **************************************************/
/******************************************************************/

 /**
 |||	AUTODOC PUBLIC spg/items/probe
 |||	Probe - An item to permite the CPU to read the output of a DSP instrument.
 |||
 |||	  Description
 |||
 |||	    A probe is an item that allows the CPU to read the output of a DSP
 |||	    instrument. It is sort of like the opposite of a knob. Multiple probes can
 |||	    be connected to a single output. A probe does not interfere with a
 |||	    connection between instruments.
 |||
 |||	  Folio
 |||
 |||	    audio (new for V24)
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_PROBE_NODE
 |||
 |||	  Create
 |||
 |||	    CreateProbe()
 |||
 |||	    CreateItem()
 |||
 |||	  Delete
 |||
 |||	    DetachProbe()
 |||
 |||	    DeleteItem()
 |||
 |||	  Query
 |||
 |||	    None
 |||
 |||	  Modify
 |||
 |||	    None
 |||
 |||	  Use
 |||
 |||	    ReadProbe()
 |||
 |||	  Tags
 |||
 |||	    None
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/createprobe
 |||	CreateProbe - Connects a task to an instrument controller.
 |||
 |||	  Synopsis
 |||
 |||	    Item CreateProbe( Item Instrument, const char *Name, const TagArg *Tags )
 |||	    Item CreateProbeVA( Item Instrument, const char *Name, uint32 tag1, ... )
 |||
 |||	  Description
 |||
 |||	    This procedure creates a Probe item that provides a fast connection between
 |||	    a task and one of an instrument's outputs.  You can then call
 |||	    ReadProbe() to poll the value of output.  When you finish with the
 |||	    probe item, you should call DeleteProbe()to deallocate the item.
 |||
 |||	    If you plan to use a probe several times, the proper procedure is to call
 |||	    CreateProbe() once to create the item, call ReadProbe() several times,
 |||	    and then call DeleteProbe() when finished.
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   The item number of the instrument.
 |||
 |||	    Name                         The name of the Output.
 |||
 |||	  Tags
 |||
 |||	    None
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number of the probe if successful or an
 |||	    error code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V24.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  Notes
 |||
 |||	    This procedure is case-insensitive to the names of probes.
 |||
 |||	  See Also
 |||
 |||	    ReadProbe(), DeleteProbe()
 |||
 **/
Item CreateProbe ( Item Instrument, const char *Name, const TagArg *Tags )
{
    /* quickie trap for unsupported tag args */
    /* @@@ remove when tags are actually added. */
    {
        const TagArg *tstate = Tags;

        if( NextTagArg (&tstate) )
        {
            ERR(("CreateProbe() now supports only name and instrument.\n"));
            return AF_ERR_BADTAG;
        }
    }

	return CreateItemVA( MKNODEID(AUDIONODE,AUDIO_PROBE_NODE),
	                     AF_TAG_NAME,       Name,
	                     AF_TAG_INSTRUMENT, Instrument,
                         TAG_JUMP,          Tags );
}

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/deleteprobe
 |||	DeleteProbe - Releases a task from a probe it has grabbed.
 |||
 |||	  Synopsis
 |||
 |||	    Err DeleteProbe( Item ProbeItem )
 |||
 |||	  Description
 |||
 |||	    This procedure deletes the probe item created by CreateProbe(), freeing its
 |||	    resource and disconnecting the task that created the probe item from the
 |||	    instrument containing the probe.
 |||
 |||	  Arguments
 |||
 |||	    ProbeItem                     Item number of probe to delete.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code (a negative value)
 |||	    if an error occurs.
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
 |||	    ReadProbe(), CreateProbe()
 |||
 **/

Err DeleteProbe( Item Probe )
{
    return DeleteItem( Probe );
}


/******************************************************************/
/******** Folio Creation of Probes *********************************/
/******************************************************************/

Item internalCreateAudioProbe (AudioProbe *aprob, /* !!! const */ TagArg *args)
{
	Item InsItem;
	AudioInstrument *ains = NULL;
  	const char *Name = NULL;
  	int32 Result;
	DSPPInstrument *dins;
	DSPPResource *drsc;

TRACEE(TRACE_INT,TRACE_ITEM,("internalCreateAudioProbe(0x%x, 0x%lx)\n", aprob, args));

	Result = TagProcessor( aprob, args, afi_DummyProcessor, 0);
	if(Result < 0)
	{
		ERR(("internalCreateAudioProbe: TagProcessor failed.\n"));
		return Result;
	}

	{
        const TagArg *tstate;
        TagArg *t;

		for (tstate = args;
			 (t = NextTagArg (&tstate)) != NULL; )
		{
DBUG(("internalCreateAudioIns: Tag = %d, Arg = 0x%x\n", t->ta_Tag, t->ta_Arg));

			switch (t->ta_Tag)
			{
			case AF_TAG_INSTRUMENT:
				InsItem = (Item) t->ta_Arg;
				ains = (AudioInstrument *)CheckItem(InsItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
				break;

			case AF_TAG_NAME:
				Name = (const char *) t->ta_Arg; /* Validate length. */
				Result = afi_IsRamAddr( Name, 1);
				if(Result < 0) return Result;
				break;

			default:
				if(t->ta_Tag > TAG_ITEM_LAST)
				{
					ERR (("Unrecognized tag in internalCreateAudioProbe - 0x%lx: 0x%lx\n",
					t->ta_Tag, (Item) t->ta_Arg));
					return AF_ERR_BADTAG;
				}
			}
		}
	}

	if (Name == NULL) return AF_ERR_BADNAME;
	if (ains == NULL) return AF_ERR_BADITEM; /* 930828 */

/* Find matching resource. */
	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
	drsc = DSPPFindResource( dins, -1, (char *)Name);       /* !!! DSPPFindResource() should take a const char * */
	if (drsc == NULL) return AF_ERR_BADNAME;

/* Is it the right type? */
	if( drsc->drsc_Type == DRSC_I_MEM ) /* %Q Bulldog has no IMEM  */
	{
		aprob->aprob_IfEO = FALSE;
	} else if( drsc->drsc_Type == DRSC_EO_MEM )
	{
		aprob->aprob_IfEO = TRUE;
	} else
	{
		ERR(("internalCreateAudioProbe: %s not I or EO mem.\n", Name));
		return AF_ERR_BADNAME;
	}

/* OK use it. */
	aprob->aprob_DSPI_Address = drsc ->drsc_Allocated;


	AddTail( &ains->ains_ProbeList, (Node *) aprob );

	return (aprob->aprob_Item.n_Item);
}
/******************************************************************/
int32 internalDeleteAudioProbe (AudioProbe *aprob)
{

TRACEE(TRACE_INT,TRACE_ITEM,("internalDeleteAudioProbe(0x%lx)\n", aprob));

/* Remove from Instrument's List */
	ParanoidRemNode( (Node *) aprob );

	return (0);
}

/******************************************************************/
/***** SUPERVISOR MODE ********************************************/
/******************************************************************/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/readprobe
 |||	ReadProbe - Read the value of a Probe.
 |||
 |||	  Synopsis
 |||
 |||	    Err ReadProbe( Item ProbeItem, int32 *ValuePtr )
 |||
 |||	  Description
 |||
 |||	    Read the instantaneous value of an instrument output.
 |||
 |||	  Arguments
 |||
 |||	    ProbeItem                     Item number of the knob to be tweaked.
 |||
 |||	    ValuePtr                      Pointer to where to put result. Task
 |||                                      must have write permission for that address.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code (a negative value)
 |||	    if an error occurs.
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
 |||	    CreateProbe(), DeleteProbe()
 |||
 **/
Err swiReadProbe ( Item ProbeItem, int32 *ValuePtr )
{
	AudioProbe *aprob;
	int32 Result;

TRACEE(TRACE_TOP,TRACE_PROBE,("swiReadProbe(0x%x, 0x%lx)\n", ProbeItem, ValuePtr));

	aprob = (AudioProbe *)CheckItem(ProbeItem, AUDIONODE, AUDIO_PROBE_NODE);
	if (aprob == NULL) return AF_ERR_BADITEM; /* 930828 */

/*
** Make sure that the ValuePtr points to task owned memory.
** Otherwise a pirate could trick supervisor code into
** defeating security.
*/
	Result = SuperValidateMem (CURRENTTASK, ValuePtr, sizeof(int32) );
	if( Result )
	{
		ERR(("ReadProbe: Illegal ValuePtr = 0x%x\n", ValuePtr ));
		goto error;
	}

	if( aprob->aprob_IfEO )
	{
		Result = dsphReadEOMem( aprob->aprob_DSPI_Address, ValuePtr );
	} else
	{
		Result = dsphReadIMem( aprob->aprob_DSPI_Address, ValuePtr );
	}

error:
	return Result;
}
