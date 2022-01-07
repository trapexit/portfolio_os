/* File : FilesView.h */
/*
** 930819 PLB Changed number of filenames to FILESVIEW_MAX_NUM_FILES
*/

#ifndef __FILESVIEW_H
#define __FILESVIEW_H

#include "AccessUtility.h"
#include "AccessText.h"

#define FILESVIEW_MAX_NUM_FILES (100)

typedef struct filesview_{
char	pathname[MAX_PATHNAME_LEN];
char*	filenames[FILESVIEW_MAX_NUM_FILES];
int32	filenum;
int32	start_visible_fileindex;
int32	visible_filenum;
int32	currentfileindex;
Color	bgcolor;
Color	txtcolor;
Color	hlcolor;
Rect*	rect;
int32	hl_xoffset;
int32	hl_yoffset;
int32	line_gap;
long*	saveptr;
} FILESVIEW, *FILESVIEWPTR;

#define	FILESVIEW_DRAW_BG	1
#define FILESVIEW_DRAW_FG	2

#define FILESVIEW_HL_X_OFFSET	2
#define FILESVIEW_HL_Y_OFFSET	2

int32	FilesViewFileSelect(ScreenDescr *screen, FILESVIEW *filesview, int32 first);
int32	FilesViewGetButtonRect(Rect* boundary, int32 ButtonPos, FILESVIEW *filesview);
void 	FilesViewFreeFiles(FILESVIEW *filesview);
int32 	FilesViewDrawDialog(ScreenDescr *screen, FILESVIEW *files, int32 bg_fg);
Boolean FilesViewIsEmpty(FILESVIEW *filesview);
int32 	FilesViewGetVisXCharLen(FILESVIEW *filesview, int32 index);

#endif
