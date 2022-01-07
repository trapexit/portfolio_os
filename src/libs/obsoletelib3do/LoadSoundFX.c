
/******************************************************************************
**
**  $Id: LoadSoundFX.c,v 1.10 1994/11/01 00:48:06 peabody Exp $
**
**  Lib3DO routine.
**
******************************************************************************/


#include "audio.h"
#include "string.h"
#include "utils3do.h"
#include "debug3do.h"
#include "init3do.h"

/*
    Note: nNumVoices is ignored.
*/
Item LoadSoundFX(char* sFilename, int nNumVoices, SoundInfo *theSoundInfo)
{
	memset(theSoundInfo,0,sizeof *theSoundInfo);

	theSoundInfo->iSoundEffect = LoadInstrument("sampler.dsp", 0, 100);
	if (theSoundInfo->iSoundEffect < 0)
		{
		DIAGNOSE_SYSERR(theSoundInfo->iSoundEffect, ("Cannot load sampler instrument"));
		return theSoundInfo->iSoundEffect;
		}

	theSoundInfo->iSoundData = LoadSample(sFilename);
	if (theSoundInfo->iSoundData < 0)
		{
		DIAGNOSE_SYSERR(theSoundInfo->iSoundData, ("Cannot load sample %s", sFilename));
		UnloadInstrument(theSoundInfo->iSoundEffect);
		return theSoundInfo->iSoundData;
		}

	theSoundInfo->iSoundAttachment = AttachSample(theSoundInfo->iSoundEffect, theSoundInfo->iSoundData, NULL);
	if (theSoundInfo->iSoundAttachment < 0)
		{
		DIAGNOSE_SYSERR(theSoundInfo->iSoundAttachment, ("Cannot attach sample %s", sFilename));
		UnloadSample(theSoundInfo->iSoundData);
		UnloadInstrument(theSoundInfo->iSoundEffect);
		return theSoundInfo->iSoundAttachment;
		}

	return theSoundInfo->iSoundEffect;
}
