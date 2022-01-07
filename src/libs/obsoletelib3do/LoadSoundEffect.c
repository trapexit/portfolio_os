
/******************************************************************************
**
**  $Id: LoadSoundEffect.c,v 1.8 1994/11/01 00:49:41 peabody Exp $
**
**  Lib3DO routines to load/free sound effects.
**
**  These routines ARE NOT thread-safe, due to the use of a static variable.
**
******************************************************************************/


#include "audio.h"
#include "mem.h"
#include "utils3do.h"
#include "debug3do.h"
#include "init3do.h"

/* this points to the head of a list of all soundInfo's created by LoadSoundEffect */
/* It is used by FreeSoundEffects() to clean up nicely */

static SoundInfo *SoundInfoHead;

/*
    Note: nNumVoices is only passed to LoadSoundFX() which ignores it.
*/
Item LoadSoundEffect(char* sFilename, int nNumVoices)
{
	SoundInfo *theSoundInfo;
	Item retval = -1;

	if ((theSoundInfo = (SoundInfo *)ALLOCMEM(sizeof(SoundInfo),MEMTYPE_ANY)) == NULL) return -1;

	retval = LoadSoundFX(sFilename, nNumVoices, theSoundInfo);
	if(retval < 0) {
		DIAGNOSE(("LoadSoundEffect failed"));
		FREEMEM( theSoundInfo, sizeof(SoundInfo) );
	} else {
		theSoundInfo->next = SoundInfoHead;
		SoundInfoHead = theSoundInfo;
	}

	return retval;
}

void FreeSoundEffects(void)
{
	SoundInfo * si = SoundInfoHead;

	while(si) {
		DetachSample(si->iSoundAttachment);
		UnloadSample(si->iSoundData);
		UnloadInstrument(si->iSoundEffect);
		si = si->next;
		FREEMEM(SoundInfoHead, sizeof(SoundInfo));
		SoundInfoHead = si;
	}
}
