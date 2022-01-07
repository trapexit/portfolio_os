/* File : AccessSave.c */
/*
** 930818 PLB Prepend directory so that we return full pathname.
**            Add flags to call to GetNextSaverDeviceName()
*/
#include "access.h"
#include "doaccess.h"
#include "AccessUtility.h"
#include "AccessSave.h"
#include "DirectoryHelper.h"
#include "AccessText.h"
#include "AccessTwoButton.h"
#include "filesystem.h"

char FntGetNxtChar(char c, int32 dir);

int32
AccessSave(args)
AccessArgs	*args;
{
	DialogStruct	dialogStruct[MAX_SAVE_ITEMS];
	SaveName		theName;
	SaveButtonArgs	localArgs;
	ScreenDescr		screen;
	Rect	DialogBGRect = {70, 80, 250, 170};
	Color	DialogBGCol;
	Color	DialogTxtCol;
	long	*saveptr;
	long	pad;
	int32	quit;
	int32	vButton = SAVE_RETURN_ITEM;
	int32	hButton = 0;
	int32	CurrentButton;
	int32	result;

	DIAGNOSTIC("AccessSave");
	result = SUCCESS;

	UtlGetScreenStruct(&screen, args->aa_Screen);

	DialogBGCol = args->aa_BGPen;
	DialogTxtCol = args->aa_FGPen;

	// Initialize to default name here, if NULL string,
	// code will still do something intelligent
	if (args->aa_StringBufferSize > SAVE_FILE_LEN + 1)
		{
		strncpy(theName.name, args->aa_StringBuffer, SAVE_FILE_LEN);
		theName.name[SAVE_FILE_LEN] = '\0';
		theName.namelen = SAVE_FILE_LEN;
		}
	else
		{
		strcpy(theName.name, args->aa_StringBuffer);
		theName.namelen = args->aa_StringBufferSize;
		}

	theName.maxvalid = (int) strlen(theName.name);
	theName.currentnamepos = 0;
	theName.dstruct = &(dialogStruct[SAVE_FILE_START]);

	// Do as much stuff as necessary into the back buffer.
	// Do DisplayScreen() to bring buffer to foreground

	Save_SetupDialog(&dialogStruct[0], &theName, &DialogBGRect, args);

	// еее Need To Do This : Setup offscreen buffer for Hilite stuff
	saveptr = GetSaveArea();

	// Draw a background color.  Eventually this will do cool stuff like shadow
	DrawDialogBackground(&screen, args, &DialogBGRect);

	ResetCurrentFont();

	Save_DrawAllDialogItems(&screen, &dialogStruct[0], MAX_SAVE_ITEMS);

	// еее Need To Do This : Need to call this to bring back buffer to front
	DisplayScreen(screen.sd_ScreenItem, 0);

	// Hilite default button
	CurrentButton = vButton + hButton;
	HiliteButton(&screen, &(dialogStruct[CurrentButton].drect),
			saveptr, args->aa_HilitePen);

	// Wait for selection
	quit = 0;
	while (!quit)
		{
		if (vButton == SAVE_FILE_START)
			pad = AccGetControlPad( ControlMove | ControlA | ControlB );
		else
			pad = AccGetControlPad( ControlMove );
		if (pad & (ControlA | ControlB | ControlC))
			{
			localArgs.dscreen = &screen;
			localArgs.dstruct = &(dialogStruct[CurrentButton]);
			localArgs.pad = pad;
			localArgs.saveptr = saveptr;
			localArgs.hicol = args->aa_HilitePen;

			switch (vButton)
				{
				case SAVE_FILE_START :
				case SAVE_SAVE_ITEM :
					localArgs.localargs = (long *) &theName;
					break;
				case SAVE_NEXT_ITEM :
					localArgs.localargs = (long *)
							&(dialogStruct[SAVE_DIRECTORY_ITEM]);
					break;
				default :
					localArgs.localargs = NULL;
					break;
				}

			result = dialogStruct[CurrentButton].action(&localArgs);

			switch(result)
				{
				case RETURN :
					quit++;
					break;
				case CLEARNAME :
					hButton = 0; CurrentButton = vButton + hButton;
					break;
				case GOTONAME :
					UnHiliteButton(&screen,
							&(dialogStruct[CurrentButton].drect), saveptr);
					hButton = 0; vButton = SAVE_FILE_START;
					CurrentButton = vButton + hButton;
					HiliteButton(&screen, &(dialogStruct[CurrentButton].drect),
							saveptr, args->aa_HilitePen);
					break;
				case SAVE :
/* 930818 PLB, Prepend directory so that we return full pathname. */
	   			 	strncpy(args->aa_StringBuffer,
							dialogStruct[SAVE_DIRECTORY_ITEM].dtext, args->aa_StringBufferSize);
	   			 	strncat(args->aa_StringBuffer,
							"/", (args->aa_StringBufferSize -
								strlen(args->aa_StringBuffer) - 1));
	   			 	strncat(args->aa_StringBuffer,
							theName.name, (args->aa_StringBufferSize -
								strlen(args->aa_StringBuffer) - 1) );
					args->aa_StringBuffer[args->aa_StringBufferSize-1]
							= '\0';
//	OLD				strcpy(args->aa_StringBuffer, theName.name);

					quit++;
					break;
				default :
					break;
				}
			}

		if ((pad & ControlUp) && (vButton > SAVE_RETURN_ITEM))
			{
			UnHiliteButton(&screen, &(dialogStruct[CurrentButton].drect),
					saveptr);
			vButton--; CurrentButton = vButton;
			HiliteButton(&screen, &(dialogStruct[CurrentButton].drect),
					saveptr, args->aa_HilitePen);
			WaitABit(10);
		}

		if ((pad & ControlDown) && (vButton < SAVE_FILE_START))
			{
			UnHiliteButton(&screen, &(dialogStruct[CurrentButton].drect),
					saveptr);
			vButton++;
			CurrentButton = (vButton == SAVE_FILE_START)
					? vButton + hButton : vButton;
			HiliteButton(&screen, &(dialogStruct[CurrentButton].drect),
					saveptr, args->aa_HilitePen);
			WaitABit(10);
			}

		if (vButton == SAVE_FILE_START)
			{
			if ((pad & ControlLeft) && (hButton > 0))
				{
				UnHiliteButton(&screen, &(dialogStruct[CurrentButton].drect),
						saveptr);
				hButton--; CurrentButton = vButton + hButton;
				theName.currentnamepos -= 1;
				HiliteButton(&screen, &(dialogStruct[CurrentButton].drect),
						saveptr, args->aa_HilitePen);
				WaitABit(10);
				}

			if ((pad & ControlRight) && (hButton < SAVE_FILE_LEN - 1)
					&& (hButton < theName.maxvalid))
				{
				UnHiliteButton(&screen, &(dialogStruct[CurrentButton].drect),
						saveptr);
				hButton++; CurrentButton = vButton + hButton;
				theName.currentnamepos += 1;
				HiliteButton(&screen, &(dialogStruct[CurrentButton].drect),
						saveptr, args->aa_HilitePen);
				WaitABit(10);
				}
			}

		}

	FlashIt(&screen, &(dialogStruct[CurrentButton].drect), saveptr, 3,
			args->aa_HilitePen);
	WaitABit(4);
	free((char *) saveptr);
	Save_DestroyDialog(&dialogStruct[0], MAX_SAVE_ITEMS);
	return(result);

}



int32
Save_ReturnFunc(args)
SaveButtonArgs	*args;
{
	int32		retcode;

//	DIAGNOSTIC("Save_ReturnFunc()");

	retcode = (args->pad & ControlAccept) ? RETURN : CONT;

	WaitABit(10);
	return(retcode);
}



int32
Save_NextFunc(args)
SaveButtonArgs	*args;
{
	DialogStruct	*dirstruct;
	Color			tmpcol;

//	DIAGNOSTIC("Save_NextFunc()");

	dirstruct = (DialogStruct *) args->localargs;

	if (args->pad & ControlAccept)
		{
		tmpcol = dirstruct->fgcol;
		dirstruct->fgcol = dirstruct->bgcol;
		Buttons_DrawDialogItems(args->dscreen, dirstruct);
/* PLB, Skip READONLY filesystems. */
		GetNextSaverDeviceName(dirstruct->dtext, dirstruct->dtext, 0, FILE_IS_READONLY);

		dirstruct->fgcol = tmpcol;
		Buttons_DrawDialogItems(args->dscreen, dirstruct);
		}

	WaitABit(10);
	return(CONT);
}



int32
Save_SaveFunc(args)
SaveButtonArgs	*args;
{
	SaveName	*lname;
	int32		retcode;

//	DIAGNOSTIC("Save_SaveFunc()");

	retcode = CONT;
	lname = (SaveName *) args->localargs;

	if (args->pad & ControlAccept)
		{
		if (!lname->maxvalid) retcode = GOTONAME;
		else retcode = SAVE;
		}

	WaitABit(10);
	return(retcode);
}

int32
Save_FileFunc(args)
SaveButtonArgs	*args;
{
	SaveName	*lname;
	int32		retval;

//	DIAGNOSTIC("Save_FileFunc()");

	retval = CONT;
	lname = (SaveName *) args->localargs;

	if (args->pad & ControlC)
		{
		strcpy(lname->name, "");
		lname->currentnamepos = 0;
		lname->maxvalid = 0;
		UnHiliteButton(args->dscreen, &(args->dstruct->drect), args->saveptr);
		Save_DrawFullName(args->dscreen, lname);
		HiliteButton(args->dscreen, &(lname->dstruct->drect), args->saveptr,
				args->hicol);
		WaitABit(10);
		return(CLEARNAME);
		}

	if (args->pad & (ControlA | ControlB))
		{

		if (args->pad & ControlA) {
			if (lname->name[lname->currentnamepos] == '\0')
				{
				lname->name[lname->currentnamepos] = 'A';
				}
			else
				{
				lname->name[lname->currentnamepos]
						= FntGetNxtChar(lname->name[lname->currentnamepos],
						SEARCH_UP);
				}
			}
		else
			{
			if (lname->name[lname->currentnamepos] == '\0')
				{
				lname->name[lname->currentnamepos] = 'Z';
				}
			else
				{
				lname->name[lname->currentnamepos]
						= FntGetNxtChar(lname->name[lname->currentnamepos],
						SEARCH_DOWN);
				}
			}

		if (lname->maxvalid == lname->currentnamepos)
			{
			lname->maxvalid += 1;
			lname->name[lname->maxvalid] = '\0';
			}

		args->dstruct->dtext[0] = lname->name[lname->currentnamepos];

		UnHiliteButton(args->dscreen, &(args->dstruct->drect), args->saveptr);
		Buttons_DrawDialogItems(args->dscreen, args->dstruct);
		HiliteButton(args->dscreen, &(args->dstruct->drect),
				args->saveptr, args->hicol);

		WaitABit(10);
		return(CONT);
		}

	WaitABit(10);
	return(CONT);
}



char FntGetNxtChar(char c, int32 dir)
{
	int32			found;
	unsigned char	retval;

	retval = (unsigned char) c;
	found = 0;
	while (!found)
		{
		if (dir == SEARCH_UP)
			retval = (retval == 0xFF) ? 0x00 : retval + 1;
		else
			retval = (retval == 0x00) ? 0xFF : retval - 1;
		if (FntGetCharWidth(((char) retval), FALSE) >= 0) found++;
		}

	return(retval);
}



void
Save_DestroyDialog(dstruct, noitems)
DialogStruct	*dstruct;
int32				noitems;
{
	int32		i;

	for (i = 0; i < noitems; i++) {
		free(dstruct->dtext);
		dstruct++;
	}
}



void
Save_DrawAllDialogItems(screen, dstruct, noitems)
ScreenDescr		*screen;
DialogStruct	*dstruct;
int32		noitems;
{
	int32		i;

//	DIAGNOSTIC("Save_DrawAllDialogItems()");

	for (i = 0; i < noitems; i++) {
		Buttons_DrawDialogItems(screen, dstruct);
		dstruct++;
	}

//	DIAGNOSTIC("Return : Save_DrawAllDialogItems()");
}



void
Save_DrawFullName(screen, name)
ScreenDescr		*screen;
SaveName		*name;
{
	int32		i;

//	DIAGNOSTIC("Save_DrawFullName()");

	for (i = 0; i < name->namelen; i++) {
		if ((name->name[i] == '\0') || (name->currentnamepos < i)) {
			strcpy(name->dstruct[i].dtext, " ");
		}
		Buttons_DrawDialogItems(screen, &(name->dstruct[i]));
	}

//	DIAGNOSTIC("Return : Save_DrawFullName()");
}



Err
Save_SetupDialog(dstruct, name, bgrect, aa_args)
DialogStruct	*dstruct;
SaveName		*name;
Rect			*bgrect;
AccessArgs		*aa_args;
{
	int32	i;
	int32	select = 0;
	int32	npos = 0;
	Rect	drect;
	Coord	x = 0, y = 0, yoff = 0;
	Coord	rjust;
	int32	maxcharwidth;
	int32	maxlen;
	int32	LRBorder;

//	DIAGNOSTIC("Save_SetupDialog()");

	rjust = (Coord) FntGetTextWidth("NEXT SAVER");
	maxcharwidth = FntGetMaxCharWidth();
	LRBorder = 10;

	for (i = 0; i < MAX_SAVE_ITEMS; i++) {

		switch(i) {
			case SAVE_TITLE_ITEM :
				Text_StringCreate(&(dstruct->dtext), aa_args->aa_Title);
				yoff = 6;
				select = 0;
				x = bgrect->rect_XLeft + 6;
				y = bgrect->rect_YTop + yoff;
				maxlen = bgrect->rect_XRight - x - LRBorder;
				break;
			case SAVE_DIRECTORY_ITEM :
				yoff = 32;
				dstruct->dtext = (char *) malloc((MAX_FILENAME_LEN + 1)
						* sizeof(char));
				if (!dstruct->dtext) return(ERR_SERIOUS_MEM_ERR);
				GetNextSaverDeviceName("", dstruct->dtext, 0, FILE_IS_READONLY); /* PLB */
				select = 0;
				x = bgrect->rect_XLeft + 10;
				y = bgrect->rect_YTop + yoff;
				maxlen = bgrect->rect_XRight - x - rjust - LRBorder;
				break;
			case SAVE_ENTER_ITEM :
				Text_StringCreate(&(dstruct->dtext), "ENTER NAME:");
				yoff = 22;
				maxlen = 0;
				select = 0;
				x = bgrect->rect_XLeft + 10;
				y = bgrect->rect_YBottom - yoff;
				break;
			case SAVE_RETURN_ITEM :
				Text_StringCreate(&(dstruct->dtext), "CANCEL");
				yoff = 18;
				x = bgrect->rect_XRight - 10 - rjust;
				y = bgrect->rect_YTop + yoff;
				select = 1;
				maxlen = 0;
				dstruct->action = Save_ReturnFunc;
				break;
			case SAVE_NEXT_ITEM :
				Text_StringCreate(&(dstruct->dtext), "NEXT SAVER");
				yoff += 14;
				x = bgrect->rect_XRight - 10 - rjust;
				y = bgrect->rect_YTop + yoff;
				select = 1;
				maxlen = 0;
				dstruct->action = Save_NextFunc;
				break;
			case SAVE_SAVE_ITEM :
				Text_StringCreate(&(dstruct->dtext), "SAVE");
				yoff += 14;
				x = bgrect->rect_XRight - 10 - rjust;
				y = bgrect->rect_YTop + yoff;
				maxlen = 0;
				select = 1;
				dstruct->action = Save_SaveFunc;
				break;
			case SAVE_FILE_START :
				// Here is where we process the first of the file name
				// buttons
				Text_StringCreate(&(dstruct->dtext), " ");
				if (name->maxvalid > npos) dstruct->dtext[0] = name->name[npos];
				x = bgrect->rect_XLeft + 10;
				y = bgrect->rect_YBottom - 12;
				select = 1;
				maxlen = 0;
				dstruct->action = Save_FileFunc;
				npos++;
				break;
			default :
				// Here is where we process the remainder of the file name
				// buttons
				Text_StringCreate(&(dstruct->dtext), " ");
				if (name->maxvalid > npos) dstruct->dtext[0] = name->name[npos];
				x += (maxcharwidth + 4);
				y = bgrect->rect_YBottom - 12;
				select = 1;
				maxlen = 0;
				dstruct->action = Save_FileFunc;
				npos++;
				break;
		}

		drect.rect_XLeft = x; drect.rect_YTop = y;
		drect.rect_YBottom = drect.rect_YTop + FntGetCurrentFontHeight();

		if (maxlen)
			{
			drect.rect_XRight = drect.rect_XLeft + maxlen;
			}
		else
			{
			drect.rect_XRight = drect.rect_XLeft + FntGetTextWidth(dstruct->dtext);
			}

		dstruct->drect.rect_XLeft = drect.rect_XLeft;
		dstruct->drect.rect_XRight = drect.rect_XRight;
		dstruct->drect.rect_YTop = drect.rect_YTop;
		dstruct->drect.rect_YBottom = drect.rect_YBottom;
		dstruct->select = select;

		dstruct->fgcol = aa_args->aa_FGPen;
		dstruct->bgcol = aa_args->aa_BGPen;
		dstruct->shcol = aa_args->aa_ShadowPen;

		dstruct++;
	}

	return(SUCCESS);

//	DIAGNOSTIC("Return : Save_SetupDialog()");
}





