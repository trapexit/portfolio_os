/* File : AccessSave.c */
/*
** 930818 PLB, Prepend directory so that we return full pathname.
*/

#include "access.h"
#include "doaccess.h"
#include "AccessUtility.h"
#include "AccessFileGet.h"
#include "DirectoryHelper.h"
#include "AccessText.h"
#include "FilesView.h"
#include "MsgStrings.h"
#include "AccessTwoButton.h"
#include "filesystem.h"


int SaverTypeFlag;

int32
AccessFileGet(aa_args, labelnames)
AccessArgs		*aa_args;
FILEGETTEXTS	*labelnames;
{
	DialogStruct	dialogStruct[MAX_FILE_GET_ITEMS];
	FILESVIEW	    fv;
	FileGetButtonArgs	localArgs;
	Rect	DialogBGRect = {40, 60, 280, 200};
	Rect	fvRect;
/*???	Rect	boundary;*/
	Color	DialogBGCol;
	Color	DialogTxtCol;
	Color	HLCol;
	long	*saveptr;
	long	pad;
	int32	quit;
	int32	CurrentButton;
	int32	result;
	int32	ret;
	int32	err;
	char	selectfilename[MAX_FILENAME_LEN];
	ScreenDescr	screen;
	int32 newdirectory;

	DIAGNOSTIC("AccessFileGet");

	if((aa_args->aa_Flags & AAFLAGS_WRITE_ABLE) != 0) {
		SaverTypeFlag = FILE_IS_READONLY;
	}
	else SaverTypeFlag = 0;

	ret = CANCEL;
	newdirectory = 1;

	UtlGetScreenStruct(&screen, aa_args->aa_Screen);
	DialogBGCol = aa_args->aa_BGPen;
	DialogTxtCol = aa_args->aa_FGPen;
	HLCol = aa_args->aa_HilitePen;

	// Do as much stuff as necessary into the back buffer.
	// Do DisplayScreen() to bring buffer to foreground

	err = FileGet_SetupDialog(&dialogStruct[0], labelnames, &DialogBGRect,
			DialogTxtCol, DialogBGCol, aa_args->aa_ShadowPen);
	if (err) return err;

	strcpy(fv.pathname, dialogStruct[FILE_GET_DIRECTORY_ITEM].dtext);

	fv.hl_xoffset = fv.hl_yoffset = fv.line_gap = 2;

	fvRect.rect_XLeft = dialogStruct[FILE_GET_INSTR_ITEM].drect.rect_XLeft;
	fvRect.rect_YTop  = dialogStruct[FILE_GET_INSTR_ITEM].drect.rect_YBottom
			+ 10;
	fvRect.rect_XRight = dialogStruct[FILE_GET_RETURN_ITEM].drect.rect_XLeft
			-10;
/*???			+ 16;*/
/*???	fvRect.rect_XRight = dialogStruct[FILE_GET_NEXT_ITEM].drect.rect_XRight;*/
	fvRect.rect_YBottom = DialogBGRect.rect_YBottom - 10;

	fv.rect = &fvRect;
	fv.visible_filenum = (fv.rect->rect_YBottom - fv.rect->rect_YTop
			- fv.hl_yoffset)
			/ (FntGetCurrentFontHeight() + fv.line_gap);
  	fv.bgcolor = aa_args->aa_ShadowPen;
	fv.txtcolor = DialogTxtCol;
	fv.hlcolor = HLCol;
	fv.saveptr = GetSaveArea();

	// fv->pathname gets set to "" if error
    err = GetFileNames(&fv);		// allocates memory for filenames
	if (err) return err;

	// еее Need To Do This : Setup offscreen buffer for Hilite stuff
	saveptr = GetSaveArea();

	// Draw a background color.  Eventually this will do cool stuff like shadow
	DrawDialogBackground(&screen, aa_args, &DialogBGRect);

	ResetCurrentFont();

	FileGet_DrawAllDialogItems(&screen, &dialogStruct[0], MAX_FILE_GET_ITEMS);

	FilesViewDrawDialog(&screen, &fv, FILESVIEW_DRAW_FG | FILESVIEW_DRAW_BG);

	// еее Need To Do This : Need to call this to bring back buffer to front
	DisplayScreen(screen.sd_ScreenItem, 0);

	if (!FilesViewIsEmpty(&fv))
		{
		err = FilesViewFileSelect(&screen, &fv, newdirectory);
		newdirectory = 0;
    	if (err) return err;
	    // get the filename into the return string buffer
    	strcpy(selectfilename, fv.filenames[fv.currentfileindex]);
		}
	else selectfilename[0] = '\0';

	// Hilite default button
	CurrentButton = FILE_GET_RETURN_ITEM;
	HiliteButton(&screen, &(dialogStruct[CurrentButton].drect), saveptr, HLCol);

	// Wait for selection
	quit = 0;
	while (!quit)
		{
		pad = AccGetControlPad( ControlMove );
		if (pad & (ControlAccept))
			{
			localArgs.dscreen = &screen;
			localArgs.dstruct = &(dialogStruct[CurrentButton]);
			localArgs.pad = pad;
			localArgs.saveptr = saveptr;
			localArgs.filesview = &fv;

			switch (CurrentButton)
				{
				case FILE_GET_NEXT_ITEM :
					localArgs.localargs = (long *)
							&(dialogStruct[FILE_GET_DIRECTORY_ITEM]);
					break;
				default :
					localArgs.localargs = NULL;
					break;
				}

			result = dialogStruct[CurrentButton].action(&localArgs);

			switch(result)
				{
				case NEXTSAVERFOUND:
					FilesViewFreeFiles(&fv);
					strcpy(fv.pathname,
					  dialogStruct[FILE_GET_DIRECTORY_ITEM].dtext);
					err = GetFileNames(&fv);
					// allocates memory for filenames
					if (err)
						{
						quit++;
						ret = CANCEL;
						break;
						}
					//DrawDialogBackground(aa_args, fv.rect);
					fv.start_visible_fileindex = fv.currentfileindex = 0;
					FilesViewDrawDialog(&screen, &fv, FILESVIEW_DRAW_FG);
					newdirectory = 1;
					break;
				case RETURN :
					quit++;
					ret = CANCEL;
					break;
				case PROCESS :
/* 930818 PLB, Prepend directory so that we return full pathname. */
	   			 	strncpy(aa_args->aa_StringBuffer,
							fv.pathname, aa_args->aa_StringBufferSize);
	   			 	strncat(aa_args->aa_StringBuffer,
							"/", (aa_args->aa_StringBufferSize -
								strlen(aa_args->aa_StringBuffer) - 1) );
	   			 	strncat(aa_args->aa_StringBuffer,
							selectfilename, (aa_args->aa_StringBufferSize -
								strlen(aa_args->aa_StringBuffer) - 1));

// OLD  			 strncpy(aa_args->aa_StringBuffer,
// OLD						selectfilename, aa_args->aa_StringBufferSize);


					aa_args->aa_StringBuffer[aa_args->aa_StringBufferSize-1]
							= '\0';
					quit++;
					ret = OK;
					break;
				default :
					break;
				}
			}

		if ((pad & ControlLeft) || newdirectory)
			{
			if (!FilesViewIsEmpty(&fv))
				{
		  		UnHiliteButton(&screen,
				  &(dialogStruct[CurrentButton].drect), saveptr);

				err = FilesViewFileSelect(&screen, &fv, newdirectory);
				newdirectory = 0;
				// Always hilite the first filename
   				if (err) quit++;
				DIAGNOSTIC("After SelectFileGetFile");

  				// get the filename into the return string buffer
  				strcpy(selectfilename,
				fv.filenames[fv.currentfileindex]);

				// Hilite default button
				CurrentButton = FILE_GET_RETURN_ITEM;
				HiliteButton(&screen,
				  &(dialogStruct[CurrentButton].drect), saveptr, HLCol);
				WaitABit(10);
				}
			}

		if ((pad & ControlDown) && ((CurrentButton + 1) == FILE_GET_PROCESS_ITEM) && (FilesViewIsEmpty(&fv)))
		  continue;

		if ((pad & ControlUp) && (CurrentButton > FILE_GET_RETURN_ITEM))
			{
			UnHiliteButton(&screen, &(dialogStruct[CurrentButton].drect), saveptr);
			CurrentButton--;
			HiliteButton(&screen, &(dialogStruct[CurrentButton].drect), saveptr, HLCol);
			WaitABit(10);
			}

		if ((pad & ControlDown) && (CurrentButton < FILE_GET_PROCESS_ITEM))
			{
			UnHiliteButton(&screen, &(dialogStruct[CurrentButton].drect), saveptr);
			CurrentButton++;
			HiliteButton(&screen, &(dialogStruct[CurrentButton].drect), saveptr, HLCol);
			WaitABit(10);
			}

		}

	FlashIt(&screen, &(dialogStruct[CurrentButton].drect), saveptr, 3, HLCol);
	WaitABit(10);
	free((char *) saveptr);
	FileGet_DestroyDialog(&dialogStruct[0], MAX_FILE_GET_ITEMS);
	FilesViewFreeFiles(&fv);
	FreeSaveArea(fv.saveptr);
	return(ret);

}



int32
FileGet_ReturnFunc(args)
FileGetButtonArgs	*args;
{
	int32		retcode;

//	DIAGNOSTIC("FileGet_ReturnFunc()");

	retcode = CONT;

	if (args->pad & ControlAccept)
		{
		retcode = RETURN;
		}

	WaitABit(10);
	return(retcode);
}



int32
FileGet_NextFunc(args)
FileGetButtonArgs	*args;
{
	DialogStruct	*dirstruct;
	Color			tmpcol;
	int32			err = SUCCESS;
	int32			retval = CONT;

	dirstruct = (DialogStruct *) args->localargs;

	if (args->pad & ControlAccept)
		{
		tmpcol = dirstruct->fgcol;

		// clear the old directory list before going away for
		// GetNextSaverDeviceName which takes forever
		FilesViewDrawDialog(args->dscreen, args->filesview, FILESVIEW_DRAW_BG);
		dirstruct->fgcol = dirstruct->bgcol;

		Buttons_DrawDialogItems(args->dscreen, dirstruct);

		err = GetNextSaverDeviceName(dirstruct->dtext, dirstruct->dtext, 0, SaverTypeFlag);
		dirstruct->drect.rect_XRight = dirstruct->drect.rect_XLeft
				+ (((int32) strlen(dirstruct->dtext)) * CHAR_WIDTH);

		dirstruct->fgcol = tmpcol;
		Buttons_DrawDialogItems(args->dscreen, dirstruct);
		WaitABit(10);
		if (err == SUCCESS)
		  retval = NEXTSAVERFOUND;
		}

	WaitABit(10);
	return retval;
}



int32
FileGet_ProcessFunc(args)
FileGetButtonArgs	*args;
{
	int32		retcode;

//	DIAGNOSTIC("FileGet_Process_Func()");

	retcode = CONT;

	if (args->pad & ControlAccept)
		{
		retcode = PROCESS;
		}

	WaitABit(10);
	return(retcode);
}



void
FileGet_DestroyDialog(dstruct, noitems)
DialogStruct	*dstruct;
int32				noitems;
{
	int32		i;

	for (i = 0; i < noitems; i++)
		{
		free(dstruct->dtext);
		dstruct++;
		}
}



/*
void
FileGet_DrawDialogItems(screen, dstruct)
ScreenDescr		*screen;
DialogStruct	*dstruct;
{
	GrafCon	localGrafCon;

	DIAGNOSTIC("FileGet_DrawDialogItems()");

	// Set FG and BG colors for text
	SetFGPen(&localGrafCon, dstruct->fgcol);
	SetBGPen(&localGrafCon, dstruct->bgcol);

	// Draw the buttons
	localGrafCon.gc_PenX = dstruct->drect.rect_XLeft;
	localGrafCon.gc_PenY = dstruct->drect.rect_YTop;

	DrawWooText(&localGrafCon, screen->sd_BitmapItem, dstruct->dtext);

	DIAGNOSTIC("Return : FileGet_DrawDialogItems()");
}
*/



void
FileGet_DrawAllDialogItems(screen, dstruct, noitems)
ScreenDescr		*screen;
DialogStruct	*dstruct;
int32		noitems;
{
	int32		i;

//	DIAGNOSTIC("FileGet_DrawAllDialogItems()");

	for (i = 0; i < noitems; i++)
		{
		Buttons_DrawDialogItems(screen, dstruct);
		dstruct++;
		}

//	DIAGNOSTIC("Return : FileGet_DrawAllDialogItems()");
}



int32
FileGet_SetupDialog(dstruct, labelnames, bgrect, fgcol, bgcol, shcol)
DialogStruct	*dstruct;
FILEGETTEXTS	*labelnames;
Rect			*bgrect;
Color	bgcol, fgcol, shcol;
{
	int32		i;
	char	dtext[100];
	int32	select;
	Rect	drect;
	Coord	x, y, yoff, x1;
	Coord	rjust;
	int32	err;

	select = x = y = x1 = yoff = 0;

	rjust = (Coord) FntGetTextWidth(labelnames->labels[FILE_GET_NEXT_ITEM]);

	for (i = 0; i < MAX_FILE_GET_ITEMS; i++)
		{
		switch(i)
			{
			case FILE_GET_TITLE_ITEM :
				strcpy(dtext, labelnames->labels[FILE_GET_TITLE_ITEM]);
				yoff = 6;
				select = 0;
				x = bgrect->rect_XLeft + 6;
				y = bgrect->rect_YTop + yoff;
				x1 = bgrect->rect_XRight - 10 - rjust - 4;
				yoff = 10;
				break;
			case FILE_GET_DIRECTORY_ITEM :
				err = GetNextSaverDeviceName("", dtext,0,SaverTypeFlag);
				yoff += 12;
				select = 0;
				x = bgrect->rect_XLeft + 10;
				y = bgrect->rect_YTop + yoff;
				x1 = bgrect->rect_XRight - 10 - rjust - 4;
				break;
			case FILE_GET_INSTR_ITEM :
				strcpy(dtext, labelnames->labels[FILE_GET_INSTR_ITEM]);
				yoff += 14;
				x = bgrect->rect_XLeft + 10;
				y = bgrect->rect_YTop + yoff;
/*???				x1 = bgrect->rect_XRight - 10 - rjust - 4;*/
				x1 = bgrect->rect_XRight - 10;
				select = 0;
				break;
			case FILE_GET_RETURN_ITEM :
				strcpy(dtext, labelnames->labels[FILE_GET_RETURN_ITEM]);
				yoff = 70;
				x = bgrect->rect_XRight - 10 - rjust;
				y = bgrect->rect_YTop + yoff;
				select = 1;
				dstruct->action = FileGet_ReturnFunc;
				break;
			case FILE_GET_NEXT_ITEM :
				strcpy(dtext, labelnames->labels[FILE_GET_NEXT_ITEM]);
				yoff += 14;
				x = bgrect->rect_XRight - 10 - rjust;
				y = bgrect->rect_YTop + yoff;
				select = 1;
				dstruct->action = FileGet_NextFunc;
				break;
			case FILE_GET_PROCESS_ITEM :
				strcpy(dtext, labelnames->labels[FILE_GET_PROCESS_ITEM]);
				yoff += 14;
				x = bgrect->rect_XRight - 10 - rjust;
				y = bgrect->rect_YTop + yoff;
				select = 1;
				dstruct->action = FileGet_ProcessFunc;
				break;
			default :
				dtext[0] = '\0';
				x += bgrect->rect_XRight - 10;
				y = bgrect->rect_YBottom - 10;
				dstruct->action = NULL;
				break;
			}

		drect.rect_XLeft = x; drect.rect_YTop = y;
		drect.rect_YBottom = drect.rect_YTop + FntGetCurrentFontHeight();
		if (select)
			drect.rect_XRight = drect.rect_XLeft + FntGetTextWidth(dtext);
		else
			drect.rect_XRight = x1;

		dstruct->dtext = (char *) malloc((((int32) strlen(dtext)) + 1)
				* sizeof(char));
		if (!dstruct->dtext) return(ERR_SERIOUS_MEM_ERR);
		strcpy(dstruct->dtext, dtext);
		dstruct->drect.rect_XLeft = drect.rect_XLeft;
		dstruct->drect.rect_XRight = drect.rect_XRight;
		dstruct->drect.rect_YTop = drect.rect_YTop;
		dstruct->drect.rect_YBottom = drect.rect_YBottom;
		dstruct->select = select;
		dstruct->fgcol = fgcol;
		dstruct->bgcol = bgcol;
		dstruct->shcol = shcol;

		dstruct++;
		}

	return SUCCESS;
}

