/*  :ts=8 bk=0
 *
 * vbltest.c:	Test the VBL manager.
 *
 * Leo L. Schwab					9407.11
 ***************************************************************************
 * Copyright 1994 The 3DO Company.  All Rights Reserved.
 *
 * 3DO Trade Secrets  -  Confidential and Proprietary
 ***************************************************************************
 *			     --== RCS Log ==--
 * $Id: vbltest.c,v 1.2 1994/10/28 05:31:50 ewhac Exp $
 *
 * $Log: vbltest.c,v $
 * Revision 1.2  1994/10/28  05:31:50  ewhac
 * Altered references to GetV{BL,RAM}IOReq() to CreateV{BL,RAM}IOReq().
 * Also changed corresponding DeleteItem() calls to DeleteV{BL,RAM}IOReq(),
 * where required.
 *
 * Revision 1.1  1994/08/25  23:07:53  ewhac
 * Initial revision
 *
 */
#include <types.h>
#include <graphics.h>
#include <mem.h>
#include <stdio.h>
#include <string.h>

#include "vbl.h"	/*  Hey!  No fair peeking!  :-)  */


struct argtable {
	char	*argname;
	int32	tag;
};


/* vbltest.c */
int main(int ac, char **av);
Err parseargs(int ac, char **av);
void openstuff(void);
void closestuff(void);
void die(char *str);



/*
 * Data for keeping track of displays.
 */
Item		vramIO, vblIO;
Item		sgrpitem;
Item		scritems[1];
int32		dlay;
uint8		backscreen;

int32		length, forceline, slipline;
int32		slipstream, virs;
int32		*sportlines, **forcefirst, **patchaddr;


TagArg		scrtags[] = {
	CSG_TAG_SCREENCOUNT,	(void *) 1,
	TAG_END,		0
};

TagArg		gva[] = {
	VBL_TAG_LENGTH,		(void *) &length,
	VBL_TAG_FORCELINE,	(void *) &forceline,
	VBL_TAG_SLIPLINE,	(void *) &slipline,
	VBL_TAG_SLIPSTREAM,	&slipstream,
	VBL_TAG_VIRS,		&virs,
	VBL_TAG_REPORTSPORTLINES, &sportlines,
	VBL_TAG_REPORTFORCEFIRST, &forcefirst,
	VBL_TAG_REPORTPATCHADDR, &patchaddr,
	TAG_END,		0
};

TagArg		*sva;


struct argtable	argtab[] = {
	"length",	VBL_TAG_LENGTH,
	"forceline",	VBL_TAG_FORCELINE,
	"slipline",	VBL_TAG_SLIPLINE,
	"slipstream",	VBL_TAG_SLIPSTREAM,
	"virs",		VBL_TAG_VIRS,
	"invalid",	VBL_TAG_REPORTSPORTLINES,
	NULL,		TAG_END
};





int
main (ac, av)
int	ac;
char	**av;
{
	GrafCon	gc;
	Screen	*s = NULL;
	Bitmap	*bm;
	Rect	r;
	Item	bmi = 0;
	Err	e;
	char	buf[40];

	openstuff ();

	dlay = 240;
	if (!(sva = AllocMem (sizeof (TagArg) * (ac + 1), 0)))
		die ("Can't allocate TagArgs.\n");

	parseargs (ac, av);

	if (backscreen) {
		sgrpitem = CreateScreenGroup (scritems, scrtags);
		if (sgrpitem < 0) {
printf ("failure code: %d\n", sgrpitem);
			die ("CreateScreenGroup failed.\n");
		}
		DisplayScreen (scritems[0], 0);
	}

	/*
	 * Render some intelligible stuff as it were.
	 */
	r.rect_XLeft = r.rect_YTop = 0;
	r.rect_XRight = 320;
	r.rect_YBottom = 240;

	if (backscreen) {
		s = (Screen *) LookupItem (scritems[0]);
		bm = s->scr_TempBitmap;
		bmi = bm->bm.n_Item;

		SetFGPen (&gc, MakeRGB15 (0, 15, 0));
		FillRect (bmi, &gc, &r);

		SetFGPen (&gc, MakeRGB15 (31, 31, 31));
		MoveTo (&gc, 0, 0);
		DrawTo (bmi, &gc, 319, 0);
		DrawTo (bmi, &gc, 319, 239);
		DrawTo (bmi, &gc, 0, 239);
		DrawTo (bmi, &gc, 0, 0);
	}

	/*
	 * Reconfigure vertical blank.
	 */
	if ((e = SetVBLAttrs (sva)) < 0) {
		PrintfSysErr (e);
		die ("SetVBLAttrs() failed.\n");
	}

	/*
	 * Dump data.
	 */
	if ((e = GetVBLAttrs (gva)) < 0) {
		PrintfSysErr (e);
		die ("GetVBLAttrs() failed.\n");
	}

	printf ("length = %d\n", length);
	printf ("forceline = %d, slipline = %d\n", forceline, slipline);
	printf ("slipstream is %s, VIRS is %s\n",
		slipstream ? "ON" : "OFF",
		virs ? "ON" : "OFF");

	printf ("sportlines at 0x%08lx, lines are %d - %d\n",
		sportlines, sportlines[0], sportlines[1]);

	printf ("forcefirst ptr at 0x%08lx, ptr is 0x%08lx\n",
		forcefirst, *forcefirst);

	printf ("patchaddr ptr at 0x%08lx, addr is 0x%08lx\n",
		patchaddr, *patchaddr);

	/*
	 * Twiddle thumbs.
	 */
	WaitVBL (vblIO, dlay);

	printf ("All done.\n");
}


Err
parseargs (ac, av)
int	ac;
char	**av;
{
	TagArg	*curtag;
	char	*arg;
	int32	val;

	curtag = sva;

	while (++av, --ac) {
		arg = *av;
		while (*arg) {
			*arg = tolower (*arg);
			arg++;
		}

		arg = *av;
		if (*arg == '-') {
			switch (*++arg) {
			case 'b':
				backscreen = TRUE;
				break;
			case 'n':
				dlay = atoi (++arg);
				break;
			default:
				printf ("-%c: Unknown switch; ignored.\n",
					*arg);
				break;
			}
		} else {
			struct argtable	*at;
			int		len;
			char		*equ;

			for (at = argtab;  *at->argname;  at++) {
				if (!strncmp (arg,
					      at->argname,
					      strlen (at->argname)))
				{
					if (equ = strchr (arg, '='))
						val = atoi (equ + 1);
					else
						val = 0;

					curtag->ta_Tag = at->tag;
					curtag->ta_Arg = (void *) val;
					curtag++;
					printf ("Tag %d, Arg %d\n",
						at->tag, val);
					break;
				}
			}
			if (!*at->argname)
				printf ("%s: Unknown option, ignored.\n",
					arg);
		}
	}
	curtag->ta_Tag = TAG_END;
	curtag->ta_Arg = NULL;
}


/***************************************************************************
 * Housekeeping
 */
void
openstuff ()
{
	if (OpenGraphicsFolio () < 0) {
		die ("Can't open Graphics folio.\n");
	}

	if (ResetCurrentFont () < 0)
		die ("Can't initialize graphics internal font.\n");

	/*
	 * Prepare rasters and such for use.
	 */
	vramIO = CreateVRAMIOReq ();
	vblIO = CreateVBLIOReq ();
}

void
closestuff ()
{
	DeleteVBLIOReq (vblIO);
	DeleteVRAMIOReq (vramIO);
}

void
die (str)
char *str;
{
	printf (str);
	closestuff ();
	exit (20);
}

void
dummy ()
{
	SuperSetVBLAttrs (NULL);
	SuperGetVBLAttrs (NULL);
}
