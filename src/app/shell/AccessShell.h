/* File : AcessShell.h */

#ifndef __ACCESS_SHELL_H
#define	__ACCESS_SHELL_H

#include "access.h"

typedef struct Options_ {
	char	*text;
	int32	aa_Request;
	char	*aa_Title;
	char	*aa_Text;
} Options, *OptionsPtr;

#define	LINESPACE	20

int		main(int argc, char *argv[]);
bool	InitGame(void);
bool	InitScreen(void);
void	DrawOptions(OptionsPtr OptPtr, int32 NoOpts, AccessArgs *args);
void	GetOptionsRect(OptionsPtr OptPtr, Rect *boundary, int32 button);

#endif




