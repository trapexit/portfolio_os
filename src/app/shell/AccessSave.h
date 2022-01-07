/* File : AccessSave.h */

#ifndef __ACCESS_SAVE_H
#define	__ACCESS_SAVE_H

#include "access.h"
#include "AccessUtility.h"
#include "AccessButtons.h"

#define	RETURN		0
#define	SAVE		1
#define	CONT		2
#define	CLEARNAME	3
#define	GOTONAME	4

#define	SAVE_TITLE_ITEM		0
#define	SAVE_DIRECTORY_ITEM	1
#define	SAVE_ENTER_ITEM		2
#define	SAVE_RETURN_ITEM	3
#define	SAVE_NEXT_ITEM		4
#define	SAVE_SAVE_ITEM		5
#define	SAVE_FILE_START		6

#define	SAVE_FILE_LEN		14

#define	MAX_SAVE_ITEMS	(SAVE_FILE_START+SAVE_FILE_LEN)

#define	SEARCH_UP			0
#define	SEARCH_DOWN			1

typedef struct SaveName_ {
	DialogStruct	*dstruct;
	char		name[SAVE_FILE_LEN + 1];
	int32		namelen;
	int32		currentnamepos;
	int32		maxvalid;
} SaveName, *SaveNamePtr;

typedef struct SaveButtonArgs_ {
	ScreenDescr		*dscreen;
	DialogStruct	*dstruct;
	long	*saveptr;
	long	pad;
	long	*localargs;
	Color	hicol;
} SaveButtonArgs, *SaveButtonArgsPtr;

int32	AccessSave(AccessArgs *args);
Err		Save_SetupDialog(DialogStruct *dstruct, SaveName *name, Rect *bgrect, AccessArgs *aa_args);
void	Save_DrawAllDialogItems(ScreenDescr *screen, DialogStruct *dstruct, int32 noitems);
void	Save_DestroyDialog(DialogStruct *dstruct, int32 noitems);
void	Save_DrawFullName(ScreenDescr *screen, SaveName *name);
int32	Save_ReturnFunc(SaveButtonArgs *args);
int32	Save_NextFunc(SaveButtonArgs *args);
int32	Save_SaveFunc(SaveButtonArgs *args);
int32	Save_FileFunc(SaveButtonArgs *args);

char	FntGetNxtChar(char currentchar, int32 direction);

#endif








