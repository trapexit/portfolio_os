/******************************************************************************
**
**  $Id: sound3d.c,v 1.24 1994/09/27 20:19:23 peabody Exp $
**
**  3D Sound
**
**  By: Phil Burk
**
**  Copyright (c) 1992-1994, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  930422 PLB  Connect Left and Right Amplitude knobs properly.
**  940525 PLB  Improved Get3DSoundRadiusTime()
**  940927 WJB  Added package id.
**
**  Initials:
**
**  PLB: Phil Burk (phil)
**  WJB: Bill Barton (peabody)
**
*****************************************************************/
/*
** O- Start3DSound() needs to adjust initial counts to simulate being at Pos4D.
** O- resolve 0x300 EO address problem
** O- API changes for sound played from OutFIFO
*/
#include "types.h"
#include "stdio.h"
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "kernel.h"
#include "mem.h"
#include "stdarg.h"
#include "strings.h"
#include "operror.h"
#include "operamath.h"
#include "audio.h"
#include "music.h"
#include "sound3d.h"

#include "music_internal.h"     /* package id */


DEFINE_MUSICLIB_PACKAGE_ID(sound3d)

frac16 Atan2F16 (frac16 x, frac16 y);
/* return the arctangent of the ratio y/x
** The result assume 256.0 units in the circle (or 16,777,216 units if
** used as an integer)
*/

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

int32 IfDebug3D;

#define MIN_RADIUS  500
#define SIGNEXTEND(n) (((n) & 0x8000) ? (n)|0xFFFF0000 : (n))

#ifndef ABS
#define ABS(x) (((x)<0) ? -(x) : (x) )
#endif

#define CLIPTO(x,min,max) \
	{ if ((x) < (min)) \
		{ (x) = (min); } \
		else { if ((x) > (max)) (x) = (max); } }


/* Macro to simplify error checking. */
#define CHECKVAL(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		PrintfSysErr(Result); \
		goto cleanup; \
	}

static Err s3dReadLeftRightCount( Sound3D *Snd3D, uint32 *LeftPtr, uint32 *RightPtr );

/****************************************************************/

Sound3D *Open3DSound( void )
/* Allocate in this routine in case size changes. */
{
	Sound3D *Snd3D;

    PULL_MUSICLIB_PACKAGE_ID(sound3d);

	Snd3D = (Sound3D *) malloc(sizeof(Sound3D));
	return Snd3D;
}

/****************************************************************/
int32 Close3DSound( Sound3D *Snd3D )
{
	if (Snd3D) free (Snd3D);
	return 0;
}

/****************************************************************/
int32 Attach3DSoundSample( Sound3D *Snd3D, Item Sample )
{
	int32 Result;

	Result = AttachSample( Snd3D->s3d_InsItem, Sample, "LeftInFIFO" );
	CHECKVAL(Result, "AttachSample");
	Result = AttachSample( Snd3D->s3d_InsItem, Sample, "RightInFIFO" );
	CHECKVAL(Result, "AttachSample");

cleanup:
	return Result;
}

/****************************************************************/
int32 Unload3DSound( Sound3D *Snd3D )
{
	int32 Result = 0;
DBUG(("Unload3DSound(0x%x)\n", Snd3D));

/* This will automatically release all knobs. */
	Result = UnloadInstrument( Snd3D->s3d_InsItem );
	Snd3D->s3d_InsItem = 0;
	return Result;
}

/****************************************************************/
int32 Load3DSound( Sound3D *Snd3D )
/*	Loads a 3D sound instrument.
**  Returns 0 or an error code.
*/
{
	int32 Result = 0;
DBUG(("Load3DSound(0x%x)\n", Snd3D));

	Snd3D->s3d_InsItem = LoadInstrument("sampler3d.dsp", 0, 100);
	CHECKVAL(Snd3D->s3d_InsItem, "LoadInstrument");

	Snd3D->s3d_LeftAlphaKnob = GrabKnob(Snd3D->s3d_InsItem, "LeftAlpha");
	CHECKVAL(Snd3D->s3d_LeftAlphaKnob, "GrabKnob");
	Snd3D->s3d_RightAlphaKnob = GrabKnob(Snd3D->s3d_InsItem, "RightAlpha");
	CHECKVAL(Snd3D->s3d_RightAlphaKnob, "GrabKnob");

	Snd3D->s3d_LeftBetaKnob = GrabKnob(Snd3D->s3d_InsItem, "LeftBeta");
	CHECKVAL(Snd3D->s3d_LeftBetaKnob, "GrabKnob");
	Snd3D->s3d_RightBetaKnob = GrabKnob(Snd3D->s3d_InsItem, "RightBeta");
	CHECKVAL(Snd3D->s3d_RightBetaKnob, "GrabKnob");

	Snd3D->s3d_LeftFeedKnob = GrabKnob(Snd3D->s3d_InsItem, "LeftFeed");
	CHECKVAL(Snd3D->s3d_LeftFeedKnob, "GrabKnob");
	Snd3D->s3d_RightFeedKnob = GrabKnob(Snd3D->s3d_InsItem, "RightFeed");
	CHECKVAL(Snd3D->s3d_RightFeedKnob, "GrabKnob");

	Snd3D->s3d_LeftFreqKnob = GrabKnob(Snd3D->s3d_InsItem, "LeftFreq");
	CHECKVAL(Snd3D->s3d_LeftFreqKnob, "GrabKnob");
	Snd3D->s3d_RightFreqKnob = GrabKnob(Snd3D->s3d_InsItem, "RightFreq");
	CHECKVAL(Snd3D->s3d_RightFreqKnob, "GrabKnob");

/* 930422 */
	Snd3D->s3d_LeftAmplitudeKnob = GrabKnob(Snd3D->s3d_InsItem, "LeftVolume");
	CHECKVAL(Snd3D->s3d_LeftAmplitudeKnob, "GrabKnob");
	Snd3D->s3d_RightAmplitudeKnob = GrabKnob(Snd3D->s3d_InsItem, "RightVolume");
	CHECKVAL(Snd3D->s3d_RightAmplitudeKnob, "GrabKnob");

	Snd3D->s3d_MinRadius = MIN_RADIUS;

cleanup:
	return Result;
}

static Err s3dReadLeftRightCount( Sound3D *Snd3D, uint32 *LeftPtr, uint32 *RightPtr )
{
	int32 Result;

	Result = ReadProbe(Snd3D->s3d_LeftCountProbe, (int32 *) LeftPtr );
	CHECKVAL(Result, "s3dReadLeftRightCount: ReadProbe");
	Result = ReadProbe(Snd3D->s3d_RightCountProbe, (int32 *) RightPtr );
	CHECKVAL(Result, "s3dReadLeftRightCount: ReadProbe");

cleanup:
	return Result;
}

/***********************************************************************/
int32 Start3DSound( Sound3D *Snd3D, PolarPosition4D *Pos4D , TagArg *TagList )
/* Starts a 3D sound playing at the given polar coordinates.
** NOT FULLY IMPLEMENTED %Q  Starts in head not at Pos4D.
*/
{
	int32 Result = 0;

DBUG(("Start3DSound(0x%x, 0x%x, 0x%x)\n", Snd3D, Pos4D, TagList));
	Result = StartInstrument( Snd3D->s3d_InsItem, TagList );
	CHECKVAL(Result, "StartInstrument");

/* Get Frame Counts */
	Snd3D->s3d_LeftCountProbe = CreateProbe(Snd3D->s3d_InsItem,
		"EO_LeftCount", NULL );
	CHECKVAL(Snd3D->s3d_LeftCountProbe, "CreateProbe");
	Snd3D->s3d_RightCountProbe = CreateProbe(Snd3D->s3d_InsItem,
		"EO_RightCount", NULL );
	CHECKVAL(Snd3D->s3d_LeftCountProbe, "CreateProbe");

DBUG(("Start3DSound: LeftProbe = 0x%x, RightProbe = 0x%x\n",
		Snd3D->s3d_LeftCountProbe, Snd3D->s3d_RightCountProbe));

	Snd3D->s3d_InitialFrame = (uint32) GetAudioFrameCount();
DBUG(("Start3DSound: FrameCount = 0x%x\n", Snd3D->s3d_InitialFrame ));

	Result = s3dReadLeftRightCount( Snd3D, &Snd3D->s3d_InitialLeft, &Snd3D->s3d_InitialRight );
	CHECKVAL(Result, "Start3DSound: s3dReadLeftRightCount");

DBUG(("Snd3D->s3d_InitialLeft = 0x%x\n", Snd3D->s3d_InitialLeft));
DBUG(("Snd3D->s3d_InitialRight = 0x%x\n", Snd3D->s3d_InitialRight));

cleanup:
	return Result;
}

/***********************************************************************/
int32 Move3DSound( Sound3D *Snd3D, PolarPosition4D *Start4D, PolarPosition4D *End4D )
/* Move the sound to the given position to arrive at the given time
** in 3D Ticks.
*/
{
	int32 Result = 0;
	frac16 LeftRate, RightRate;
	BothEarParams BEP;
	int32 DesiredPD, DeltaPD;
	int32 ActualRadius;
	int32 DeltaRadius, PhaseDelay;

DBUG(("Move3DSound(0x%x, 0x%x, 0x%x)\n", Snd3D, Start4D, End4D));
	Calc3DSoundRates ( Start4D, End4D , &LeftRate, &RightRate );
// %T	Set3DSoundRates ( Snd3D, LeftRate, RightRate );

/* Adjust for radial tracking. */
	Get3DSoundRadius(Snd3D, &ActualRadius, &PhaseDelay);
	DeltaRadius = End4D->pp4d_Radius - ActualRadius;
	LeftRate -= DeltaRadius*2;
	RightRate -= DeltaRadius*2;

/* Adjust for Phase Delay. */
	DesiredPD = MulSF16(DISTANCE_TO_EAR<<1,
		SinF16(End4D->pp4d_Theta));
	DeltaPD = DesiredPD - PhaseDelay;
	LeftRate -= DeltaPD;
	RightRate += DeltaPD;
	Set3DSoundRates ( Snd3D, LeftRate, RightRate );

	Calc3DSoundFilters ( End4D, Snd3D->s3d_MinRadius, &BEP );
	Set3DSoundFilters ( Snd3D, &BEP );

	return Result;
}

/****************************************************************/
static int32 Interpolate (frac16 Scale, frac16 Range, int32 MinVal, int32 MaxVal)
{
	int32 Result, Rem;
DBUG(("Interpolate: Scale = 0x%x, Range = 0x%x\n", Scale, Range ));
DBUG(("Interpolate: MinVal = 0x%x, MaxVal = 0x%x\n", MinVal, MaxVal ));

	Result = (DivRemSF16(&Rem, MulSF16((MaxVal - MinVal), Scale), Range) + MinVal);
DBUG(("Interpolate: Result = 0x%x\n", Result ));
	return Result;
}

/****************************************************************/
frac16 NormalizeAngle( frac16 Angle )
{
	if (Angle > HALFCIRCLE) Angle -= FULLCIRCLE;
	if (Angle < -HALFCIRCLE) Angle += FULLCIRCLE;
	return Angle;
}

#define LOWPASS_MAX 0x6A00
#define NOTCH_MAX 0x1000
#define DSP_ONE 0x7FFF
/***********************************************************************/
int32 Calc3DSoundEar ( PolarPosition4D *Pos4D, int32 MinRadius, EarParams *ERP )
{
	frac16 nt;
	int32 Alpha, Beta, Feed, Amplitude, Radius;

	nt = NormalizeAngle(Pos4D->pp4d_Theta);
DBUG(("Calc3DSoundEar: nt = 0x%x\n", nt));
	if (nt < 0)
	{
		if (nt < -QUARTERCIRCLE)
		{
/* head shadowed and behind, -180 => -90*/
			Alpha = LOWPASS_MAX;
			Beta = Interpolate(HALFCIRCLE+nt, QUARTERCIRCLE, NOTCH_MAX/2, 0);
		}
		else
		{
/* head shadowed, in front, -90 => 0 */
			Alpha = MulSF16(LOWPASS_MAX, SinF16(HALFCIRCLE+nt));
			Beta = Interpolate(QUARTERCIRCLE+nt, QUARTERCIRCLE, 0, NOTCH_MAX);
		}
	}
	else
	{
		if (nt > QUARTERCIRCLE)
		{
/* head direct and behind, 90 => 180 */
			Alpha = MulSF16(LOWPASS_MAX, SinF16((nt-QUARTERCIRCLE)));
			Beta = Interpolate(nt-QUARTERCIRCLE, QUARTERCIRCLE, 0, NOTCH_MAX/2);
		}
		else
		{
/* head direct, in front, 0 => 90 */
			Alpha = 0;
			Beta = Interpolate(nt, QUARTERCIRCLE, NOTCH_MAX, 0);
		}
	}

/* Drop off with increasing radius. */
	Alpha += Pos4D->pp4d_Radius;
	if (Alpha > LOWPASS_MAX) Alpha = LOWPASS_MAX;
	Feed = DSP_ONE - Alpha - Beta;
/*	Amplitude = DSP_ONE - ((Pos4D->pp4d_Radius * DSP_ONE) / MinRadius); */
	Radius = Pos4D->pp4d_Radius;
	if (Radius < MinRadius) Radius = MinRadius;
	Amplitude = (((DSP_ONE * MinRadius) / Radius) * MinRadius) / Radius ;

	ERP->erp_Alpha = Alpha;
	ERP->erp_Beta = Beta;
	ERP->erp_Feed = Feed;
	ERP->erp_Amplitude = Amplitude;

	return 0;
}

/***********************************************************************/
int32 Calc3DSoundFilters( PolarPosition4D *Pos4D, int32 MinRadius, BothEarParams *BEP)
{
	int32 Result;

	Calc3DSoundEar (Pos4D, MinRadius, &BEP->bep_RightEar);

/* Negate theta for opposite ear calculation. */
	Pos4D->pp4d_Theta = -Pos4D->pp4d_Theta;
	Result = Calc3DSoundEar (Pos4D, MinRadius, &BEP->bep_LeftEar);
	Pos4D->pp4d_Theta = -Pos4D->pp4d_Theta;

	return Result;
}
#define MIN_DELTA_TIME (40)
#define PP4D PolarPosition4D
/***********************************************************************/
int32 Calc3DSoundRates( PP4D *Start4D, PP4D *End4D, frac16 *LeftRate, frac16 *RightRate )
{
	int32 Result = 0;
	int32 DT;  /* Actual time elapsed in samples. */
	int32 dt;  /* Listeners compressed time. */
	int32 DeltaRadius;
	ufrac16 Rem;
	frac16 DeltaSine;
	frac16 Sigma; /* E*(sin - sin) */

	DT = (((int32)(uint16) End4D->pp4d_Time - (uint16) Start4D->pp4d_Time) & 0xFFFF);
	DeltaRadius = (End4D->pp4d_Radius - Start4D->pp4d_Radius);
DBUG(("DT = 0x%x, DeltaRadius = 0x%x\n", DT, DeltaRadius));

	CLIPTO(DeltaRadius, (-(DT>>3)), (DT>>3));
	dt = DT + DeltaRadius;
	DeltaSine = SubF16( SinF16(End4D->pp4d_Theta), SinF16(Start4D->pp4d_Theta) );
DBUG(("dt = 0x%x, DeltaSine = 0x%x\n", dt, DeltaSine));

	Sigma = MulSF16(Convert32_F16(DISTANCE_TO_EAR), DeltaSine);
	*LeftRate = DivRemUF16(&Rem, Convert32_F16(DT), AddF16(Convert32_F16(dt),Sigma));
	*RightRate = DivRemUF16(&Rem, Convert32_F16(DT), SubF16(Convert32_F16(dt),Sigma));

	return Result;
}

/***********************************************************************/
int32 Set3DSoundRates ( Sound3D *Snd3D, frac16 LeftRate, frac16 RightRate )
{
	int32 LeftFreq, RightFreq;

	LeftFreq = MulUF16(NOMINAL3DFREQ, LeftRate);
	RightFreq = MulUF16(NOMINAL3DFREQ, RightRate);
	TweakKnob(Snd3D->s3d_LeftFreqKnob, LeftFreq);
	TweakKnob(Snd3D->s3d_RightFreqKnob, RightFreq);
	return 0;
}

/***********************************************************************/
int32 Set3DSoundFilters ( Sound3D *Snd3D, BothEarParams *BEP )
{
	TweakKnob(Snd3D->s3d_LeftAlphaKnob, BEP->bep_LeftEar.erp_Alpha);
	TweakKnob(Snd3D->s3d_RightAlphaKnob, BEP->bep_RightEar.erp_Alpha);

	TweakKnob(Snd3D->s3d_LeftBetaKnob, BEP->bep_LeftEar.erp_Beta);
	TweakKnob(Snd3D->s3d_RightBetaKnob, BEP->bep_RightEar.erp_Beta);

	TweakKnob(Snd3D->s3d_LeftFeedKnob, BEP->bep_LeftEar.erp_Feed);
	TweakKnob(Snd3D->s3d_RightFeedKnob, BEP->bep_RightEar.erp_Feed);

	TweakKnob(Snd3D->s3d_LeftAmplitudeKnob, BEP->bep_LeftEar.erp_Amplitude);
	TweakKnob(Snd3D->s3d_RightAmplitudeKnob, BEP->bep_RightEar.erp_Amplitude);

	return 0;
}

/***********************************************************************/
uint16 Get3DSoundTime( void )
{
	return (uint16) GetAudioFrameCount();
}

/***********************************************************************/
Item Get3DSoundInstrument( Sound3D *Snd3D )
{
	return Snd3D->s3d_InsItem;
}

/************************************************************************
** Read EO and calculate current Radius, PhaseDelay and Time
************************************************************************/
static int32 Get3DSoundRadiusTime ( Sound3D *Snd3D, int32 *RadiusPtr, int32 *PhaseDelayPtr, int32 *TimePtr)
{
	uint32 LeftNormal,RightNormal, FrameNormal;
	uint32 FrameCount, LeftCount, RightCount;
	int32 LeftRadius, RightRadius, IntraAuralDelay;
	int32 Radius;
	int32 Result = 0;

/* Get EO stuff as quickly as possible. */
	FrameCount = GetAudioFrameCount();
	Result = s3dReadLeftRightCount( Snd3D, &LeftCount, &RightCount );
	CHECKVAL(Result, "Get3DSoundRadiusTime: s3dReadLeftRightCount");

/* Normalize based on change from original values. */
	FrameNormal = (FrameCount - Snd3D->s3d_InitialFrame) & 0xFFFF;
	LeftNormal = (LeftCount - Snd3D->s3d_InitialLeft) & 0xFFFF;
	RightNormal = (RightCount - Snd3D->s3d_InitialRight) & 0xFFFF;
DBUG(("Get3DSoundRadiusTime: FrameNormal = 0x%x, LeftNormal = 0x%x, RightNormal = 0x%x\n",
	FrameNormal, LeftNormal, RightNormal));

/* Calculate distance to right and left ear. */
	RightRadius = (FrameNormal - RightNormal) & 0xFFFF;
	LeftRadius = (FrameNormal - LeftNormal) & 0xFFFF;

/* Use maximum distance so that accidental negative excursions
** do not cause a glitch.
*/
#define MAX_REAL_LAG (0xD000)
	if( RightRadius > MAX_REAL_LAG )
	{
		DBUG(("Get3DSoundRadiusTime: RightRadius = %d\n", RightRadius ));
		RightRadius |= 0xFFFF0000;  /* Sign extend. */
	}
	if( LeftRadius > MAX_REAL_LAG )
	{
		DBUG(("Get3DSoundRadiusTime: LeftRadius = %d\n", LeftRadius ));
		LeftRadius |= 0xFFFF0000;
	}

/* Return an average as the actual radius.
** Negative radius means that sampler has overshot head. */
	Radius = (RightRadius + LeftRadius) / 2;
	if( Radius < 0)
	{
		DBUG(("Get3DSoundRadiusTime: Negative Radius!\n"));
		Radius = -Radius;
	}

/* Calculate IntraAuralDelay */
	IntraAuralDelay = (RightNormal - LeftNormal) & 0xFFFF;
	IntraAuralDelay = SIGNEXTEND( IntraAuralDelay );

	if(RadiusPtr) *RadiusPtr = Radius;
	if(PhaseDelayPtr) *PhaseDelayPtr = IntraAuralDelay;
DBUG(("Get3DSoundRadiusTime: Radius = 0x%x, PhaseDelay = 0x%x\n", Radius, IntraAuralDelay));

	if(TimePtr) *TimePtr = FrameCount;

 /* Check to see if ears are getting split apart. */
	IntraAuralDelay = ABS( IntraAuralDelay );
	if( IntraAuralDelay > 44 )
	{
		DBUG(("Get3DSoundRadiusTime: IntraAuralDelay = %d\n", IntraAuralDelay ));
	}

cleanup:
	return Result;
}

/***********************************************************************/
int32 Get3DSoundRadius( Sound3D *Snd3D, int32 *RadiusPtr, int32 *PhaseDelayPtr)
{
	return Get3DSoundRadiusTime( Snd3D, RadiusPtr, PhaseDelayPtr, NULL);
}

/***********************************************************************/
int32 Get3DSoundPos( Sound3D *Snd3D, PolarPosition4D *Pos4D )
{
	int32 PhaseDelay;
	frac16 q, w;
	int32 Rem;
	int32 Result = 0;

	Get3DSoundRadius( Snd3D, &Pos4D->pp4d_Radius, &PhaseDelay);

	CLIPTO(PhaseDelay, -((DISTANCE_TO_EAR<<1)+3), ((DISTANCE_TO_EAR<<1)+3));
	q = DivRemSF16( &Rem, Convert32_F16(PhaseDelay), Convert32_F16(DISTANCE_TO_EAR<<1));
	w = SqrtF16( SubF16( Convert32_F16(1), SquareSF16(q)));
DBUG(("Get3DSoundPos: q = 0x%x, w = 0x%x\n", q, w));

	Pos4D->pp4d_Theta = NormalizeAngle(Atan2F16(w, q));
DBUG(("Get3DSoundPos: Theta 0x%x\n", Pos4D->pp4d_Theta));
	if (Pos4D->pp4d_Radius < 0)
	{
		Pos4D->pp4d_Radius = -Pos4D->pp4d_Radius;
		Pos4D->pp4d_Theta += HALFCIRCLE;
	}
	Pos4D->pp4d_Phi = 0;


	return Result;
}
/***********************************************************************/
int32 Stop3DSound( Sound3D *Snd3D, TagArg *TagList )
{
	return StopInstrument(Snd3D->s3d_InsItem, TagList);
}
