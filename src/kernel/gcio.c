/* $Id: gcio.c,v 1.3 1994/06/03 18:15:44 sdas Exp $ */

#include <kernel.h>
#include <gfx.h>

#include "conio.h"

#ifdef	CIO_GCIO

/* #define	GCIO_DEBUG */

int                     gcioCurrCol;
int                     gcioCurrRow;
int                     gcioMaxCol;
int                     gcioMaxRow;

extern unsigned char    CharSet[][8];

int                     charcolor = -1;

extern unsigned         CPUFlags;

int
gcioInit (int x, int y)
{
    if (!(CPUFlags & KB_NODBGR)) {
#ifdef	GCIO_DEBUG
	printf("gcioInit: returning false\n");
#endif
	return 0;
    }
    gcioCurrCol = 0;
    gcioCurrRow = 0;
    gcioMaxCol = x;
    gcioMaxRow = y;
#ifdef	GCIO_DEBUG
    printf("gcio_init: returning true\n");
#endif
    return 1;
}

int
gcioPutChar (int c)
{
    unsigned char          *p = CharSet[c];
    unsigned                bits;
    unsigned                cumbits = 0;
    int                     x, y;

    if (c == '\n')
    {
	gcioCurrCol = 0;
	gcioCurrRow += 8;
    }

    if (gcioCurrCol >= gcioMaxCol)
    {
	gcioCurrCol = 0;
	gcioCurrRow += 8;
    }

    if (gcioCurrRow >= gcioMaxRow)
    {
	gcioCurrCol = 0;
	gcioCurrRow -= 8;
	ScrollRaster (0, 0, 240, 320, 0, -8);
	RectFill (0, 0, gcioCurrRow, 320, 8);
    }

    for (y = 0; y < 8; y++)
    {
	bits = *p++;
	cumbits |= bits;
	for (x = 0; x < 8; x++)
	{
	    if (bits & 1)
		foomyWritePixel (charcolor, gcioCurrCol + x, gcioCurrRow + y);
	    bits >>= 1;
	}
    }
    if (cumbits == 0)
	cumbits = 0xF;
    for (x = 8; x >= 0; x--)
    {
	if (cumbits & 0x80)
	    break;
	cumbits <<= 1;
    }
    gcioCurrCol += x + 1;
    return c;
}

#endif
