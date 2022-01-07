/* File : AccessButtons.h */

#ifndef	__BUTTONS_H
#define	__BUTTONS_H

#include "access.h"
#include "doaccess.h"

typedef struct DialogStruct_ {
	Rect	drect;
	char	*dtext;
	int32	select;
	Color	fgcol;
	Color	bgcol;
	Color	shcol;
	int32	(*action)();
} DialogStruct, *DialogStructPtr;

#endif





