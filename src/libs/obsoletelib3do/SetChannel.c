
/******************************************************************************
**
**  $Id: SetChannel.c,v 1.5 1994/10/05 19:12:39 vertex Exp $
**
**  Lib3DO routine to connect an instrument to a channel of the global mixer.
**
**  This routine is not thread-safe.
**
******************************************************************************/


#include "audio.h"
#include "utils3do.h"


extern Item TheMixer;

Boolean SetChannel(Item iInstrument, int nChannel)
{
	char ChannelName[] = "Input0";

	ChannelName[5] = nChannel + '0';
	if (ConnectInstruments(iInstrument, "Output", TheMixer, ChannelName) < 0)
		return FALSE;
	else
		return TRUE;
}
