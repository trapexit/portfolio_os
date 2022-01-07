
/******************************************************************************
**
**  $Id: OpenAudio.c,v 1.7 1994/11/01 00:50:59 peabody Exp $
**
**  Lib3DO routine to open the audio folio and load/start a mixer.
**
**  This function is not thread-safe.
**
******************************************************************************/


#include "audio.h"
#include "string.h"
#include "init3do.h"
#include "debug3do.h"

#define DEFAULT_MIXER_SETTING 0x2000

Item TheMixer = -1;

Boolean OpenAudio(void)
{
	char LeftName[32];
	char RightName[32];
	Item left, right;
	char cChannel;

	strcpy( LeftName, "LeftGain0" );
	strcpy( RightName, "RightGain0" );

	if (OpenAudioFolio() < 0)
		{
		DIAGNOSE(("Cannot open audio folio"));
		return FALSE;
		}

	TheMixer = LoadInstrument("mixer4x2.dsp", 0, 100);

	if (TheMixer < 0)
		{
		DIAGNOSE_SYSERR(TheMixer, ("Cannot load mixer instrument"));
		return FALSE;
		}

	for (cChannel = '0'; cChannel < '4'; cChannel++)
		{
		LeftName[8] = cChannel;
		RightName[9] = cChannel;
		left = GrabKnob(TheMixer, LeftName);
		right = GrabKnob(TheMixer, RightName);
		if (right < 0 || left < 0)
			{
			DIAGNOSE(("Cannot set mixer channel"));
			return FALSE;
			}
		TweakKnob(right, DEFAULT_MIXER_SETTING);
		TweakKnob(left, DEFAULT_MIXER_SETTING);
		ReleaseKnob(right);
		ReleaseKnob(left);
		}

	StartInstrument(TheMixer, NULL);

	return TRUE;
}
