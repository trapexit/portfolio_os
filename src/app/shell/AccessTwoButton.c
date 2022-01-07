/* File : AccessTwoButton.c */

#include "access.h"
#include "doaccess.h"
#include "AccessUtility.h"
#include "AccessText.h"
#include "AccessTwoButton.h"
#include "stdio.h"

// NOTE: this varies per TV, on my Sony its actually 284 for X
// We really need a preferences setting for this...

#define X_VISUAL_AREA 320
#define Y_VISUAL_AREA 240

int32
AccessButtons(aa_args, buttons)
AccessArgs	*aa_args;
int32		buttons;
{
	ScreenDescr		screen;
	Rect			DialogBGRect = {100, 120 - 26, 220, 120 + 36};
	DialogStruct	DialogB1Struct;
	DialogStruct	DialogB2Struct;
	long	*saveptr;
	long	pad;
	int32	quit;
	int32	CurrentButton = BUTTON1;

	ManyStrings	DialogTextStrings;

	char	eText[100];
	int32	i;

	// Do as much stuff as necessary into the back buffer.
	// Do DisplayScreen() to bring buffer to foreground

	// еее Need To Do This : Setup offscreen buffer for Hilite stuff
	saveptr = GetSaveArea();
	UtlGetScreenStruct(&screen, aa_args->aa_Screen);

	// Break up long strings into a few short ones
	Text_GetManyStrings(&DialogTextStrings, aa_args->aa_Text,
			MAX_STRING_LENGTH, MAX_DIALOG_TEXT_STRINGS);

	sprintf(eText, "Strings %d", DialogTextStrings.ms_Count);
	DIAGNOSTIC(eText);

	for (i = 0; i < DialogTextStrings.ms_Count; i++)
		{
		strcpy(eText, DialogTextStrings.ms_Strings[i]);
		DIAGNOSTIC(eText);
		}

	// Draw a background color.  Eventually this will do cool stuff like shadow
	Buttons_GetDialogBackgroundRect(&DialogBGRect, aa_args,
			&DialogTextStrings, buttons);
	DrawDialogBackground(&screen, aa_args, &DialogBGRect);

	Buttons_DrawTitle(&screen, &DialogBGRect, aa_args->aa_Title,
			aa_args->aa_FGPen, aa_args->aa_BGPen);
	Buttons_DrawText(&screen, &DialogBGRect, &DialogTextStrings,
			aa_args->aa_FGPen, aa_args->aa_BGPen);

	Buttons_GetButtonStruct(aa_args, &DialogB1Struct,
			&DialogBGRect, BUTTON1, buttons);
	Buttons_DrawDialogItems(&screen, &DialogB1Struct);
	if (buttons > 1)
		{
		Buttons_GetButtonStruct(aa_args, &DialogB2Struct,
				&DialogBGRect, BUTTON2, buttons);
		Buttons_DrawDialogItems(&screen, &DialogB2Struct);
		}

	// еее Need To Do This : Need to call this to bring backbuffer to front
	DisplayScreen(screen.sd_ScreenItem, 0);

	// Hilite default button
	HiliteButton(&screen, &(DialogB1Struct.drect), saveptr,
			aa_args->aa_HilitePen);

	// Wait for selection
	quit = 0;
	while (!quit)
		{
		pad = AccGetControlPad( ControlMove );
		if (pad & ControlA) quit++;

		if ((pad & ControlRight) && (CurrentButton == BUTTON1)
				&& (buttons > 1))
			{
			UnHiliteButton(&screen, &(DialogB1Struct.drect), saveptr);
			CurrentButton = BUTTON2;
			HiliteButton(&screen, &(DialogB2Struct.drect), saveptr,
					aa_args->aa_HilitePen);
			WaitABit(10);
			}

		if ((pad & ControlLeft) && (CurrentButton == BUTTON2))
			{
			UnHiliteButton(&screen, &(DialogB2Struct.drect), saveptr);
			CurrentButton = BUTTON1;
			HiliteButton(&screen, &(DialogB1Struct.drect), saveptr,
					aa_args->aa_HilitePen);
			WaitABit(10);
			}
		}

	switch (CurrentButton)
		{
		case BUTTON1 :
			FlashIt(&screen, &(DialogB1Struct.drect), saveptr, 3,
					aa_args->aa_HilitePen);
			break;
		case BUTTON2 :
			FlashIt(&screen, &(DialogB2Struct.drect), saveptr, 3,
					aa_args->aa_HilitePen);
			break;
		}

	WaitABit(4);
	free((char *) saveptr);
	Text_DisposeManyStrings(&DialogTextStrings);
	Buttons_DisposeButtonStruct(&DialogB1Struct);
	if (buttons > 1)
		{
		Buttons_DisposeButtonStruct(&DialogB2Struct);
		}
	return(CurrentButton);
}



void
Buttons_DrawDialogItems(screen, dstruct)
ScreenDescr		*screen;
DialogStruct	*dstruct;
{
	GrafCon	localGrafCon;
	int32	i;

//	DIAGNOSTIC("Buttons_DrawDialogItems()");

	// Set FG and BG colors for text
	SetFGPen(&localGrafCon, dstruct->fgcol);
	SetBGPen(&localGrafCon, dstruct->bgcol);

	// Draw the buttons
	localGrafCon.gc_PenX = dstruct->drect.rect_XLeft;
//	localGrafCon.gc_PenY = dstruct->drect.rect_YTop;

	FntDrawTextInRect(screen, &localGrafCon, &(dstruct->drect),
		dstruct->dtext);

	if (dstruct->select)
		{
		SetFGPen(&localGrafCon, dstruct->shcol);
		DrawBox( &localGrafCon, screen->sd_BitmapItem,
				dstruct->drect.rect_XLeft - 2,
				dstruct->drect.rect_YTop - 2,
				dstruct->drect.rect_XRight,
				dstruct->drect.rect_YBottom );

		SetFGPen(&localGrafCon, dstruct->shcol);
		for (i = 1; i < 2; i++)
			{
			MoveTo(&localGrafCon, dstruct->drect.rect_XLeft + i,
					dstruct->drect.rect_YBottom + i);
			DrawTo(screen->sd_BitmapItem, &localGrafCon,
					dstruct->drect.rect_XRight + i,
					dstruct->drect.rect_YBottom + i);
			DrawTo(screen->sd_BitmapItem, &localGrafCon,
					dstruct->drect.rect_XRight + i,
					dstruct->drect.rect_YTop + i);
			}
		}

//	DIAGNOSTIC("Return : Save_DrawDialogItems()");
}



void
Buttons_DrawTitle(screen, dialogrect, text, fgcol, bgcol)
ScreenDescr		*screen;
Rect			*dialogrect;
char			*text;
Color			fgcol, bgcol;
{
	GrafCon	localGrafCon;
	Rect newrect;

	// Set FG and BG colors for text
	SetFGPen(&localGrafCon, fgcol);
	SetBGPen(&localGrafCon, bgcol);


	memcpy((char *)&newrect,dialogrect,sizeof(struct Rect));
	newrect.rect_XLeft += 6;
	newrect.rect_YTop += 6;
	FntDrawTextInRect(screen,&localGrafCon,&newrect,text);

//	localGrafCon.gc_PenX = dialogrect->rect_XLeft + 6;
//	localGrafCon.gc_PenY = dialogrect->rect_YTop + 6;
//	DrawWooText(&localGrafCon, screen->sd_BitmapItem, text);

}



void
Buttons_DrawText(screen, dialogrect, Text, fgcol, bgcol)
ScreenDescr		*screen;
Rect			*dialogrect;
ManyStrings		*Text;
Color			fgcol, bgcol;
{
	GrafCon	localGrafCon;
	int32	count;

	// Set FG and BG colors for text
	SetFGPen(&localGrafCon, fgcol);
	SetBGPen(&localGrafCon, bgcol);

	for (count = 0; count < Text->ms_Count; count++)
		{
		switch (TEXT_JUSTIFY_MODE)
			{
			case JUSTIFY_CENTER :
				localGrafCon.gc_PenX = (Coord) ((X_VISUAL_AREA / 2)
						- (FntGetTextWidth(Text->ms_Strings[count]) / 2));
				break;
			default : // Justify Left
				localGrafCon.gc_PenX = dialogrect->rect_XLeft + 10;
				break;
			}

		localGrafCon.gc_PenY = dialogrect->rect_YTop + 20
				+ (count * (FntGetCurrentFontHeight() + 2));
		DrawWooText(&localGrafCon, screen->sd_BitmapItem,
				Text->ms_Strings[count]);
		}

}



void
Buttons_GetButtonStruct(aa_args, dstruct, dialogrect, button, nobuttons)
AccessArgs		*aa_args;
DialogStruct	*dstruct;
Rect	*dialogrect;
int32	button, nobuttons;
{
	int32		xmid, ymid;
	int32		slength = 0;

//	DIAGNOSTIC("Buttons_GetButtonStruct()");

	xmid = X_VISUAL_AREA / 2; ymid = Y_VISUAL_AREA / 2;

	switch (button)
		{
		case BUTTON2 :
			slength = FntGetTextWidth(aa_args->aa_ButtonTwo);
			slength = (slength > xmid) ? xmid : slength;
			dstruct->dtext = (char *) malloc((slength + 1) * sizeof(char));
			strcpy(dstruct->dtext, aa_args->aa_ButtonTwo);
			// Setup Rect for Button 2 button
			dstruct->drect.rect_XLeft = (Coord) (xmid
					+ ((dialogrect->rect_XRight - xmid) / 2) - (slength / 2));
			break;

		case BUTTON1 :
			slength = FntGetTextWidth(aa_args->aa_ButtonOne);
			slength = (slength > X_VISUAL_AREA) ? X_VISUAL_AREA : slength;
			dstruct->dtext = (char *) malloc((slength + 1) * sizeof(char));
			strcpy(dstruct->dtext, aa_args->aa_ButtonOne);
			// Setup Rect for Button 1 button
			if (nobuttons == 2)
				dstruct->drect.rect_XLeft = (Coord) (xmid
						- ((xmid - dialogrect->rect_XLeft) / 2)
						- (slength / 2));
			else
				dstruct->drect.rect_XLeft = (Coord) (xmid -  (slength / 2));
			break;
		}

	dstruct->drect.rect_XRight = dstruct->drect.rect_XLeft + ((Coord) slength);
	dstruct->drect.rect_YBottom = dialogrect->rect_YBottom - 10;
	dstruct->drect.rect_YTop = dstruct->drect.rect_YBottom
			- FntGetCurrentFontHeight();

	dstruct->select = 1;

	dstruct->fgcol = aa_args->aa_FGPen;
	dstruct->bgcol = aa_args->aa_BGPen;
	dstruct->shcol = aa_args->aa_ShadowPen;

//	DIAGNOSTIC("Returned : Buttons_GetButtonStruct()");

}



void
Buttons_DisposeButtonStruct(dstruct)
DialogStruct	*dstruct;
{
	free(dstruct->dtext);
}



void
Buttons_GetDialogBackgroundRect(bounds, aa_args, Text, buttons)
Rect		*bounds;
AccessArgs	*aa_args;
ManyStrings	*Text;
int32		buttons;
{
	int32	curWidth, strWidth = 0, textMaxWidth, textWidth, growAmount;
	int32	LRBorder = 10;
	int32	i;

	// Do the Y grow stuff
	growAmount = ((Text->ms_Count - 1) * (FntGetCurrentFontHeight() + 2)) / 2;
	if (growAmount % 2) growAmount++;
	bounds->rect_YTop -= growAmount;
	bounds->rect_YBottom += growAmount;

	textMaxWidth = 0;
	for (i = 0; i < Text->ms_Count; i++)
		{
		if ((textWidth = FntGetTextWidth(Text->ms_Strings[i])) > textMaxWidth)
			textMaxWidth = textWidth;
		}

	// Do the X grow stuff
	for (i = 0; i < 2 + buttons; i++)
		{
		switch(i)
			{
			case 0 :
				strWidth = FntGetTextWidth(aa_args->aa_Title);
				break;
			case 1 :
				strWidth = textMaxWidth;
				break;
			case 2 :
				if (buttons == 1)
					strWidth = FntGetTextWidth(aa_args->aa_ButtonOne);
				else
					strWidth = FntGetTextWidth(aa_args->aa_ButtonOne) * 2;
				break;
			case 3 :
				strWidth = FntGetTextWidth(aa_args->aa_ButtonTwo) * 2;
				break;
			}

		strWidth += (LRBorder * 2);
		strWidth = (strWidth > X_VISUAL_AREA) ? X_VISUAL_AREA: strWidth;
		curWidth = bounds->rect_XRight - bounds->rect_XLeft;
		if (strWidth > curWidth)
			{
			growAmount = (strWidth - curWidth) / 2;
			if (growAmount % 2) growAmount++;
			bounds->rect_XRight += growAmount;
			bounds->rect_XLeft -= growAmount;
			}
		}

}

