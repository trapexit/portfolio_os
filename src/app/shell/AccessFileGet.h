/* File : AccessFileGet.h */

#ifndef __ACCESS_FILEGET_H
#define	__ACCESS_FILEGET_H

#include "access.h"
#include "AccessSave.h"
#include "FilesView.h"



#define	RETURN		0
#define	PROCESS		1
#define	CONT		2
#define NEXTSAVERFOUND 3
#define GOTOSELECT	4

#define	MAX_FILE_GET_ITEMS	6
#define	MAX_LABEL_LEN	20

#define	FILE_GET_TITLE_ITEM		0
#define	FILE_GET_DIRECTORY_ITEM	1
#define	FILE_GET_INSTR_ITEM		2
#define	FILE_GET_RETURN_ITEM	3
#define	FILE_GET_NEXT_ITEM		4
#define	FILE_GET_PROCESS_ITEM	5

typedef struct SaveName_	FileGetName;
typedef struct SaveName_	*FileGetNamePtr;

typedef struct FileGetButtonArgs_ {
	ScreenDescr		*dscreen;
	DialogStruct	*dstruct;
	long	*saveptr;
	long	pad;
	long	*localargs;
	FILESVIEW	*filesview;
} FileGetButtonArgs, *FileGetButtonArgsPtr;

typedef struct filegettexts_ {
	char	labels[MAX_FILE_GET_ITEMS][MAX_LABEL_LEN];
}FILEGETTEXTS, *FILEGETTEXTSPTR;

int32	AccessFileGet(AccessArgs *args, FILEGETTEXTS *filegettexts);
int32	FileGet_SetupDialog(DialogStruct *dstruct, FILEGETTEXTS *labelnames, Rect *bgrect, Color c1, Color c2, Color c3);
//void	FileGet_DrawDialogItems(ScreenDescr *screen, DialogStruct *dstruct);
void	FileGet_DrawAllDialogItems(ScreenDescr *screen, DialogStruct *dstruct, int32 noitems);
void	FileGet_DestroyDialog(DialogStruct *dstruct, int32 noitems);
int32	FileGet_ReturnFunc(FileGetButtonArgs *args);
int32	FileGet_NextFunc(FileGetButtonArgs *args);
int32	FileGet_ProcessFunc(FileGetButtonArgs *args);

#endif








