
/******************************************************************************
**
**  $Id: DrawImage.c,v 1.12 1994/11/03 18:19:12 vertex Exp $
**
**  Lib3DO routine to copy an image from its VRAM buffer to the screen.
**
******************************************************************************/


#include "displayutils.h"
#include "debug3do.h"

Boolean DrawImage(Item iScreen, ubyte* pbImage, ScreenContext *sc )
{
	Screen* screen;
	Item	VRAMIOReq;

	screen = (Screen*)LookupItem(iScreen);

	if (screen == NULL)	{
		DIAGNOSE(("Cannot locate screens"));
		return FALSE;
	} else {
		VRAMIOReq = CreateVRAMIOReq();
		CopyVRAMPages( VRAMIOReq, screen->scr_TempBitmap->bm_Buffer, pbImage, sc->sc_NumBitmapPages, ~0 );
		DeleteVRAMIOReq( VRAMIOReq );
		return TRUE;
	}
}
