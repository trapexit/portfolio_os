/* $Id: dspp_knobs.c,v 1.11 1994/09/08 21:09:53 phil Exp $ */
/****************************************************************
**
** DSPP Instrument Knobs
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
** 931215 PLB Make Knob name matching case insensitive.
** 940406 PLB Add break after calculation for synthetic frequency.
**				Removed PRT
** 940817 PLB Shift Frequency by ShiftRate to compensate for execution rate.
****************************************************************/


#include "audio_internal.h"
#include "touch_hardware.h"


#define DBUG(x)   /* PRT(x) */

/*****************************************************************/
int32 DSPPAttachKnob(AudioKnob *aknob, DSPPInstrument *dins, char *knobname)
{
/* Scan Knobs of template to find knob. */
	DSPPTemplate *dtmp;
	DSPPKnob *dknb;
	int32 Result = AF_ERR_BADNAME;
	
	aknob->aknob_DeviceInstrument = NULL; /* until successful */
	dtmp = dins->dins_Template;
	dknb = dtmp->dtmp_Knobs;
	if (dknb == NULL) return AF_ERR_NOKNOBS;  /* no knobs */

/* Scan names */
	do
	{
DBUG(("Knob search: ( %s ),  dknb->dknb_Name = %s\n",
	knobname, dknb->dknb_Name));
		if (!strcasecmp( dknb->dknb_Name, knobname )) /* 931215 */
		{
			aknob->aknob_DeviceInstrument = (void *) dins;
			aknob->aknob_DeviceKnob = dknb;
			Result = 0;
		}
		else
		{
/* Link to next variable length knob */
			dknb = DSPPNextKnob(dtmp->dtmp_Knobs, dknb);
		}
	} while ((dknb != NULL) && (Result < 0));
	
	return Result;
}

/*****************************************************************/
DSPPKnob *DSPPNextKnob( DSPPKnob *Base, DSPPKnob *dknb )
{
/* Use char pointer for byte addition */
	char *cp = (char *) Base;
	
	if ((dknb->dknb_Next) == 0) return NULL;
	return (DSPPKnob *) (cp + dknb->dknb_Next);
}

/*****************************************************************/
int32 DSPPDetachKnob( AudioKnob *aknob )
{
	if (aknob->aknob_DeviceInstrument == NULL) return AF_ERR_NOINSTRUMENT;
	aknob->aknob_DeviceInstrument = NULL;
	aknob->aknob_DeviceKnob = NULL;
	return 0;
}

/*****************************************************************/
int32 DSPPPutKnob( DSPPInstrument *dins, DSPPKnob *dknb, int32 val, int32 *NewValPtr, int32 IfConvert)
{
	int32 i;
	DSPPKnobResource *dknr;
	DSPPResource *drsc, *drscarray;
	int32 calcval, rsi;
	int32 max;
	   
	dknr = dknb->dknb_Resources;

DBUG(("------------\nDSPPPutKnob: knob = $%x, #resources = %d\n"
	, dknb, dknb->dknb_NumResources));

	drscarray = dins->dins_Resources;
		
	for (i=0; i<dknb->dknb_NumResources; i++)
	{
/* Convert input to device dependant value. */
		if (IfConvert)
		{
			switch(dknr->dknr_CalcType)
			{
			case CALC_NOP:
				calcval = val;
				break;
			case CALC_LINEAR:
				calcval = ( dknr->dknr_Data1 * val ) + dknr->dknr_Data2;
				break;
			case CALC_SCALE:
				calcval = ( dknr->dknr_Data1 * val ) / dknr->dknr_Data2;
				break;
				
			case CALC_DIVIDE_SR:
/* Shift Frequency by Shift Rate to compensate for execution rate. 940817 */
				calcval = (val / DSPPData.dspp_SampleRate) << dins->dins_RateShift;
				break; /* 940406 */
				
			case CALC_SHIFT_DIVIDE_SR:  /* 940227 For extended precision oscillators. */
				{
/* Shift Frequency by Shift Rate to compensate for execution rate. 940817 */
					int32 Shifter = dknr->dknr_Data1 + dins->dins_RateShift;
				
					max = ((uint32) 0xFFFFFFFF) >> (Shifter + 1);
					if( val > max )
					{
						calcval = (val / DSPPData.dspp_SampleRate ) << Shifter;
					}
					else
					{
						calcval = (val << Shifter) / DSPPData.dspp_SampleRate;
					}
				}
	/* %T */	PRT(("max = 0x%x, val = 0x%x, calcval = 0x%x\n", max, val, calcval ));
				break;
				
			default:
				ERR(("DSPPPutKnob: bad calc type = 0x%x\n", dknr->dknr_CalcType));
				return AF_ERR_BADCALCTYPE;
			}
		}
		else
		{
			calcval = val;
		}
		
		if( i == 0)
		{
/* Clip to legal range. */
			if( calcval > dknb->dknb_Max )
			{
				calcval = dknb->dknb_Max;
			}
			else
			{
				if( calcval < dknb->dknb_Min )
				{
					calcval = dknb->dknb_Min;
				}
			}
/* Use value calculated for KnobResource[0]for any subsequent resources. */
/* Currently, all knobs have just 1 KnobResource. */
			val = calcval;

/* Pass calculated result back to caller for saving state. */
			*NewValPtr = calcval;
		}
		
/* Write data to appropriate DSPP location */
		rsi = dknr->dknr_RsrcIndex;
		drsc = &drscarray[rsi];
		
DBUG(("rsi = $%x, drsc = $%x, type = $%x\n", rsi, drsc, drsc->drsc_Type));

		if (drsc->drsc_Type != DRSC_EI_MEM) return AF_ERR_BADKNOBRSRC;
		
DBUG(("Set EI[$%x] to $%x\n", drsc->drsc_Allocated, calcval));

		dsphWriteEIMem(drsc->drsc_Allocated, calcval);
		
/* advance to next knob resource */
		dknr++;
	}
	return 0;	
}

