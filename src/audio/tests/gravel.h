/* $Id: gravel.h,v 1.2 1994/02/09 02:04:35 limes Exp $ */
#pragma include_only_once

typedef struct GravelControl
{
	Item grvl_Noise1;
	Item grvl_Noise2;
	Item grvl_TimesPlus;
	Item grvl_DepthKnob;
	Item grvl_CenterKnob;
	Item grvl_AmpKnob;
	Item grvl_OutputIns;
} GravelControl;

#define FREQ1 (800)
#define FREQ2 (1400)
#define MODDEPTH (800)

int32 InitGravel( GravelControl *grvl );
int32 StartGravel( GravelControl *grvl, int32 Amplitude );
int32 StopGravel( GravelControl *grvl );
int32 TermGravel( GravelControl *grvl );
int32 SetGravelAmplitude( GravelControl *grvl, int32 Amplitude );
