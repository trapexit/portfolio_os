#include "access.h"
#include "doaccess.h"
#include "AccessLoad.h"
#include "FilesView.h"
#include "AccessUtility.h"
#include "MsgStrings.h"
#include "AccessText.h"
#include "event.h"

  


int32 FilesViewFileSelect(ScreenDescr *screen, FILESVIEW *fv, int32 first)
{
	int32	CurrentButtonPos, LastButtonPos;
	long	pad;
	Rect	boundary;
	GrafCon gc;
	Rect *FilesViewRect;
	int32 retvalue;

	if (fv->filenum == 0) return ERR_GENERIC;

	FilesViewRect = fv->rect;

	CurrentButtonPos = fv->currentfileindex - fv->start_visible_fileindex;
	LastButtonPos = fv->visible_filenum - 1;
	if (LastButtonPos < 0) return ERR_GENERIC;

	SetFGPen( &gc, fv->hlcolor );
	DrawBox( &gc, screen->sd_BitmapItem, FilesViewRect->rect_XLeft - 1, 
			FilesViewRect->rect_YTop - 1, FilesViewRect->rect_XRight + 1, 
			FilesViewRect->rect_YBottom + 1 );

	FilesViewGetButtonRect(&boundary, CurrentButtonPos, fv); 

	if ( first )
		{
		FileHiliteButton(screen, &boundary, fv->saveptr, fv->hlcolor);
		}

	while (1) 
		{
		pad = AccGetControlPad( ControlMove ); 
		if (pad & (ControlRight))
			{ 
			// file selection made, leaves highlight
			WaitABit(10);
			retvalue = SUCCESS;
			goto DONE;
			}
				
		if ((pad & ControlUp) && (fv->currentfileindex > 0))
			{
			fv->currentfileindex -= 1;
			UnHiliteButton(screen, &boundary, fv->saveptr);
						
			if (CurrentButtonPos > 0) 
				{
				CurrentButtonPos -= 1;
				FilesViewGetButtonRect(&boundary, CurrentButtonPos, fv);
				FileHiliteButton(screen, &boundary, fv->saveptr, fv->hlcolor);
				WaitABit(10);
				}
			else 
				{
				fv->start_visible_fileindex -= 1;
				FilesViewDrawDialog(screen, fv, 
						(FILESVIEW_DRAW_BG | FILESVIEW_DRAW_FG) );
				FileHiliteButton(screen, &boundary, fv->saveptr, fv->hlcolor);		
				WaitABit(10);
				}
			}
		
						
		if ((pad & ControlDown) && (fv->currentfileindex < fv->filenum - 1)) 
			{
			fv->currentfileindex += 1;
			UnHiliteButton(screen, &boundary, fv->saveptr);
						
			if (CurrentButtonPos < LastButtonPos) 
				{
				CurrentButtonPos += 1;
				FilesViewGetButtonRect(&boundary, CurrentButtonPos, fv);
				FileHiliteButton(screen, &boundary, fv->saveptr, fv->hlcolor);
				WaitABit(10);
				}
			else 
				{
				fv->start_visible_fileindex += 1;
				FilesViewDrawDialog(screen, fv, 
						(FILESVIEW_DRAW_BG | FILESVIEW_DRAW_FG) );								
				FileHiliteButton(screen, &boundary, fv->saveptr, fv->hlcolor);		
				WaitABit(10);
				}
			}
		}

DONE:
	SetFGPen( &gc, fv->bgcolor );
	DrawBox( &gc, screen->sd_BitmapItem, FilesViewRect->rect_XLeft - 1, 
			FilesViewRect->rect_YTop - 1, FilesViewRect->rect_XRight + 1, 
			FilesViewRect->rect_YBottom + 1 );
	return( retvalue );
}


int32 FilesViewGetButtonRect(Rect* boundary, int32 ButtonPos, FILESVIEW *fv)
{
	Rect *FilesViewRect;
	int32 charheight;
	
	FilesViewRect = fv->rect;
	// еее height is the same for all characters
	charheight = FntGetCurrentFontHeight();
						
	boundary->rect_XLeft = fv->rect->rect_XLeft + fv->hl_xoffset;
	boundary->rect_XRight = fv->rect->rect_XRight;
	boundary->rect_YTop = fv->rect->rect_YTop + fv->hl_yoffset 
			+ (ButtonPos * (charheight + fv->line_gap));
	boundary->rect_YBottom = fv->rect->rect_YTop + fv->hl_yoffset 
			+ ((ButtonPos+1) * (charheight + fv->line_gap)) -	fv->line_gap;			 // current character height
	
	return SUCCESS;
}



void FilesViewFreeFiles(FILESVIEW *fv)
{
	int32 i;
	
	for (i = 0; i < fv->filenum; i++)
		free(fv->filenames[i]);
}



int32 FilesViewDrawDialog(
	ScreenDescr *screen, 
	FILESVIEW *fv,
	int32				bg_fg)
{
	GrafCon	localGrafCon;
	int32 i;
	int32 textlen;
	char filename[MAX_FILENAME_LEN + 3]; // extra room for ellipses
	Rect tempRect;
	ManyStrings NoFileTextStrings;
	int32 charheight;
	int32 lines, CurrentButtonPos;
	
	DIAGNOSTIC("FilesViewDrawDialog");
	
	CurrentButtonPos = fv->currentfileindex - fv->start_visible_fileindex;

	// for now, all chars will return the same height given a font
	charheight = FntGetCurrentFontHeight();
		
	if (bg_fg & FILESVIEW_DRAW_BG)
		{
		SetFGPen(&localGrafCon, fv->bgcolor);
		SetBGPen(&localGrafCon, fv->bgcolor);
		FillRect(screen->sd_BitmapItem, &localGrafCon, fv->rect);
		}
	
	if (bg_fg & FILESVIEW_DRAW_FG)
		{
		// Set FG and FG colors for text
 		SetFGPen(&localGrafCon, fv->txtcolor);
		SetBGPen(&localGrafCon, fv->bgcolor);

		if (fv->filenum == 0)
			{
 			if (fv->pathname[0] == '\0')
				{
				textlen = FntGetVisCharLenInRect(fv->rect, MSG_NO_SAVER);
				Text_GetManyStrings(&NoFileTextStrings, MSG_NO_SAVER, 
						textlen, fv->visible_filenum);
				}
			else
				{
				textlen = FntGetVisCharLenInRect(fv->rect,		MSG_NO_FILES);
				Text_GetManyStrings(&NoFileTextStrings, MSG_NO_FILES, 
						textlen, fv->visible_filenum);
				}
		
			for (i=0; i<NoFileTextStrings.ms_Count; i++)
				{
				localGrafCon.gc_PenX = fv->rect->rect_XLeft + fv->hl_xoffset;
				localGrafCon.gc_PenY = fv->rect->rect_YTop 
						+ fv->hl_yoffset + (i * (charheight + fv->line_gap));

		 		DrawWooText(&localGrafCon, screen->sd_BitmapItem, 
						NoFileTextStrings.ms_Strings[i]);
				}
			return (SUCCESS);
			}
	
		for (i = 0; i< MAX_FILENAME_LEN; i++) filename[i] = ' ';				

		// Calculate the spaces needed to indicate truncated filenames
		tempRect.rect_XLeft = fv->rect->rect_XLeft + fv->hl_xoffset;
		tempRect.rect_XRight = fv->rect->rect_XRight;
		
		lines = ((fv->filenum - fv->start_visible_fileindex) 
				< fv->visible_filenum) 
				? (fv->filenum - fv->start_visible_fileindex) 
				: fv->visible_filenum;
		
		for (i = 0; i < lines; i++)
			{
/*???			if ( i != CurrentButtonPos )*/
/*???				{*/
/*???		 		SetFGPen(&localGrafCon, fv->txtcolor);*/
/*???				SetBGPen(&localGrafCon, fv->bgcolor);*/
/*???				}*/
/*???			else*/
/*???				{*/
/*???		 		SetFGPen(&localGrafCon, fv->bgcolor);*/
/*???				SetBGPen(&localGrafCon, fv->txtcolor);*/
/*???				}*/

			tempRect.rect_YTop = fv->rect->rect_YTop 
					+ fv->hl_yoffset + (i * (charheight + fv->line_gap));
			tempRect.rect_YBottom = tempRect.rect_YTop + charheight;
	
			localGrafCon.gc_PenX = tempRect.rect_XLeft;
			localGrafCon.gc_PenY = tempRect.rect_YTop;

			FntDrawTextInRect(screen, &localGrafCon, &tempRect, 
					fv->filenames[fv->start_visible_fileindex + i]);
			}
		}
	
	return(SUCCESS);
}



// Function: FilesViewIsEmpty
// Return: check if filesview contains any file 
//         Right now it always starts from the first character in line.
// Modifies: None
// Requires: 
Boolean FilesViewIsEmpty(FILESVIEW *fv)
{
	if (fv->filenum == 0) return TRUE;
	else return FALSE;
}


