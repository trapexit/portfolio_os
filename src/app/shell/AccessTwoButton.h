/* File : AccessTwoButton.h */

#ifndef	__TWOBUTTON_H
#define	__TWOBUTTON_H

#include "access.h"
#include "doaccess.h"
#include "AccessUtility.h"
#include "AccessButtons.h"
#include "AccessText.h"

void	Buttons_GetButtonStruct(AccessArgs *aa_args, DialogStruct *dstruct, Rect *dialogrect, int32 button, int32 nobuttons);
void	Buttons_DrawDialogItems(ScreenDescr *screen, DialogStruct *dstruct);
void	Buttons_DrawTitle(ScreenDescr *screen, Rect *dialogrect, char *text, Color fgcol, Color bgcol);
void	Buttons_DrawText(ScreenDescr *screen, Rect *dialogrect, ManyStrings *Text, Color fgcol, Color bgcol);
void	Buttons_GetDialogBackgroundRect(Rect *bounds, AccessArgs *aa_args, ManyStrings *Text, int32 buttons);
void	Buttons_DisposeButtonStruct(DialogStruct *dstruct);

#endif





