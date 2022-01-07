
/******************************************************************************
**
**  $Id: SetMixer.c,v 1.7 1994/10/10 19:21:25 vertex Exp $
**
**  Lib3DO routine to set volumes on the global mixer.
**
**  This routine is not thread-safe.
**
******************************************************************************/


#include "audio.h"
#include "utils3do.h"
#include "init3do.h"

Boolean SetMixer(int nChannel, int32 nVolume, int32 nBalance)
{
	int32 nLeftLevel, nRightLevel;
	int i;

	nVolume >>= 2;	/* Split master volume over four channels */

	if (nBalance < 0x4000)
		{
		nLeftLevel = nVolume;
		nRightLevel = (nBalance * nVolume) >> 14;
		}
	else
		{
		nRightLevel = nVolume;
		nLeftLevel = ((0x8000 - nBalance) * nVolume) >> 14;
		}

	if (nChannel < 0)
		{
		i = 0;
		nChannel = 3;
		}
	else
		{
		i = nChannel;
		}

	for (; i <= nChannel; i++)
		{
		if (!SetMixerChannel(i, nLeftLevel, nRightLevel))
			return FALSE;
		}

	return TRUE;
}
