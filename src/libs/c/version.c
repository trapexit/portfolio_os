/* $Id: version.c,v 1.12 1994/10/11 22:24:31 limes Exp $ */

#include "types.h"
#include "task.h"
#include "aif.h"

/*
 * print_vinfo(): Print version information from AIF and 3DO headers,
 * with whatstring and copyright (for module signon banner).
 */

extern char	whatstring[];
extern char	copyright[];

void
print_vinfo()
{
	char	       *wsp = whatstring;

	if ((wsp[0] == '@') &&
	    (wsp[1] == '(') &&
	    (wsp[2] == '#') &&
	    (wsp[3] == ')'))
		wsp += 4;
	while (*wsp == ' ')
		wsp ++;

	Print3DOHeader (&__my_3DOBinHeader, wsp, copyright);
}

