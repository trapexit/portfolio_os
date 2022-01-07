/* File : AccessUtility.h */

#ifndef __ACCESS_UTILITY_H
#define	__ACCESS_UTILITY_H

#include "types.h"
#include "event.h"
#include "access.h"
#include "doaccess.h"
#include "filefunctions.h"

#define	TickCount() GrafBase->gf_VBLNumber

typedef struct ScreenDescr_ {
	int32 	sd_nFrameBufferPages;
	int32 	sd_nFrameByteCount;
	Item 	sd_ScreenItem;
	Item	sd_BitmapItem;
	Bitmap	*sd_Bitmap;
} ScreenDescr;

void	DrawBox( GrafCon *gc, Item bitmap, int32 left, int32 top, int32 right, int32 bottom );
void	FileHiliteButton(ScreenDescr *screen, Rect *boundary, long *saveptr, Color Col);
void	HiliteButton(ScreenDescr *screen, Rect *boundary, long *saveptr, Color Col);
void	UnHiliteButton(ScreenDescr *screen, Rect *boundary, long *saveptr);
void	WaitABit(long time);
void	DrawDialogBackground(ScreenDescr *screen, AccessArgs *args, Rect *bounds);
long	*GetSaveArea(void);
void	FreeSaveArea(long *ptr);
int32	DrawWooText(GrafCon *gc, Item bits, uint8 *text);
void	FlashIt(ScreenDescr *screen, Rect *boundary, long *saveptr, int32 times, Color Col);
long	AccGetControlPad( int32 continuous );
void	UtlGetScreenStruct(ScreenDescr *screen, Item screenitem);

#endif
