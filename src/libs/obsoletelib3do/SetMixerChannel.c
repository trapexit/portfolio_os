
/******************************************************************************
**
**  $Id: SetMixerChannel.c,v 1.6 1994/11/01 00:51:28 peabody Exp $
**
**  Lib3DO routine to set volume on a channel of the global mixer.
**
**  This routine is not thread-safe.
**
******************************************************************************/


#include "audio.h"
#include "utils3do.h"
#include "debug3do.h"

extern Item TheMixer;

Boolean SetMixerChannel(int nChannel, int32 nLeftLevel, int32 nRightLevel)
{
	static int nLastChannel = -1;
	static Item iLastLeft = -1;
	static Item iLastRight = -1;

	static char LeftName[] = "LeftGain0";
	static char RightName[] = "RightGain0";

	if (nChannel != nLastChannel)
		{
		ReleaseKnob(iLastLeft);
		ReleaseKnob(iLastRight);
		nLastChannel = nChannel;

		LeftName[8] = nChannel + '0';
		RightName[9] = nChannel + '0';
		iLastLeft = GrabKnob(TheMixer, LeftName);
		iLastRight = GrabKnob(TheMixer, RightName);
		if (iLastLeft < 0 || iLastRight < 0)
			{
			DIAGNOSE(("Cannot attach balance knobs"));
			return FALSE;
			}
		}
	TweakKnob(iLastLeft, nLeftLevel);
	TweakKnob(iLastRight, nRightLevel);

	return TRUE;
}
