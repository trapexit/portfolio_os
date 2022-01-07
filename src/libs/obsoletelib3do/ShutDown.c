
/******************************************************************************
**
**  $Id: ShutDown.c,v 1.8 1994/10/05 19:17:52 vertex Exp $
**
**  Lib3DO mis-named routine; really just closes the audio folio.
**
******************************************************************************/


#include "init3do.h"
#include "audio.h"

void ShutDown(void)
{
	CloseAudioFolio();
}

