/* File : AccessText.h */

#ifndef __ACCESSTEXT_H
#define	__ACCESSTEXT_H

#include "string.h"
#include "discdata.h"
#include "AccessUtility.h"

#define	CHAR_WIDTH	8
#define	CHAR_HEIGHT	8

#define	JUSTIFY_CENTER	0
#define	JUSTIFY_LEFT	1

#define	TEXT_JUSTIFY_MODE	JUSTIFY_LEFT


#define MAX_FILENAME_LEN  FILESYSTEM_MAX_NAME_LEN
#define MAX_PATHNAME_LEN  FILESYSTEM_MAX_NAME_LEN
#define DEFAULT_CHAR_LEN   8

#define	MAX_DIALOG_TEXT_STRINGS		5
#define	MAX_STRING_LENGTH			30

typedef struct ManyStrings_ {
	char	*ms_Strings[MAX_DIALOG_TEXT_STRINGS];
	int32	ms_Count;
	int32	ms_MaxLength;
} ManyStrings;

void	Text_GetManyStrings(ManyStrings *strstruct, char *instring, int32 maxstrlen, int32 maxstrings);
void	Text_DisposeManyStrings(ManyStrings *strstruct);
int32	Text_FindGoodBreakInText(char *text, int32 *position);
void	Text_StringCreate(char **otex, char *itext);
int32	FntGetVisCharLenInRect(Rect *rect, char *text);
int32 	FntGetTextWidth(char *text);
int32 	FntGetCharWidth(char ch, Boolean DefaultMaxWid);
int32	FntGetCurrentFontHeight(void);
int32 	FntDrawTextInRect(ScreenDescr *screen, GrafCon *localGrafCon, Rect *rect, char *text);
int32	FntGetMaxCharWidth(void);
int32 	_fntGetMaxCharWidth(FontEntry *fe);
#endif



