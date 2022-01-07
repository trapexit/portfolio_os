/*  :ts=8 bk=0
 *
 * scrtest.c:	Test the screen system.
 *
 * Leo L. Schwab					9408.08
 ***************************************************************************
 * Copyright 1994 The 3DO Company.  All Rights Reserved.
 *
 * 3DO Trade Secrets  -  Confidential and Proprietary
 ***************************************************************************
 *			     --== RCS Log ==--
 * $Id: scrtest.c,v 1.2 1994/10/28 05:31:50 ewhac Exp $
 *
 * $Log: scrtest.c,v $
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
#include <stdio.h>



int main (int, char **);
void trydisplay (struct DisplayInfo *);
struct TagArg *findtag (struct TagArg *, uint32);
void openstuff (void);
void closestuff (void);
void die (char *);



char *di_tags[] = {
	"DI_END",
	"DI_TYPE",
	"DI_WIDTH",
	"DI_HEIGHT",
	"DI_FIELDTIME",
	"DI_FIELDFREQ",
	"DI_ASPECT",
	"DI_ASPECTW",
	"DI_ASPECTH",
	"DI_NOINTERLACE",
	"DI_NOSTEREO",
	"DI_NAME"
};
#define	N_DI_TAGS	(sizeof (di_tags) / sizeof (char *))


char *di_types[] = {
	"DI_TYPE_DEFAULT",
	"DI_TYPE_NTSC",
	"DI_TYPE_PAL1",
	"DI_TYPE_PAL2"
};
#define	N_DI_TYPES	(sizeof (di_types) / sizeof (char *))


Item	vblio;



int
main (ac, av)
int	ac;
char	**av;
{
	register DisplayInfo	*di;
	register TagArg		*ta;
	char			*diname;

	openstuff ();

	for (di = GetFirstDisplayInfo ();
	     NEXTNODE (di);
	     di = (DisplayInfo *) NEXTNODE (di))
	{
		for (ta = di->di_Tags;  ta->ta_Tag;  ta++) {
			if (ta->ta_Tag >= N_DI_TAGS)
				diname = "Unknown";
			else
				diname = di_tags[ta->ta_Tag];

			printf ("Tag %-5d (%15s), Arg = %10d (0x%08lx)\n",
				ta->ta_Tag, diname, ta->ta_Arg, ta->ta_Arg);
		}
		printf ("\n");

		trydisplay (di);
	}
	closestuff ();
}


void
trydisplay (di)
struct DisplayInfo *di;
{
	static TagArg	scrtags[] = {
		CSG_TAG_DISPLAYTYPE,	0,	/*  Filled in later.  */
		CSG_TAG_SCREENCOUNT,	(void *) 1,
		TAG_END,		0
	};
	Bitmap	*bm;
	Screen	*scr;
	Rect	r;
	GrafCon	gc;
	Item	sgrpitem;
	Item	scritem;
	Item	bmi;
	TagArg	*ta;
	char	buf[128];

	if (!(ta = findtag (di->di_Tags, DI_TYPE)))
		die ("Can't find DI_TYPE.\n");

	scrtags[0].ta_Arg = ta->ta_Arg;
	if ((sgrpitem = CreateScreenGroup (&scritem, scrtags)) < 0)
		die ("Can't create screengroup.\n");

	scr = (Screen *) LookupItem (scritem);
	bm = scr->scr_TempBitmap;
	bmi = bm->bm.n_Item;

	DisplayScreen (scritem, 0);


	/*
	 * Compute a few things.
	 */
	r.rect_XLeft = r.rect_YTop = 0;
	r.rect_XRight = bm->bm_Width;
	r.rect_YBottom = bm->bm_Height;
	SetFGPen (&gc, MakeRGB15 (0, 0, 31));
	FillRect (bmi, &gc, &r);

	SetFGPen (&gc, MakeRGB15 (31, 31, 31));
	MoveTo (&gc, 16, 16);
	sprintf (buf, "Width:  %d", bm->bm_Width);
	DrawText8 (&gc, bmi, buf);

	MoveTo (&gc, 16, 24);
	sprintf (buf, "Height: %d", bm->bm_Height);
	DrawText8 (&gc, bmi, buf);


	WaitVBL (vblio, 240);

	DeleteScreenGroup (sgrpitem);  sgrpitem = -1;
}


struct TagArg *
findtag (ta, val)
register struct TagArg	*ta;
register uint32		val;
{
	while (ta->ta_Tag) {
		if (ta->ta_Tag == val)
			return (ta);

		if (ta->ta_Tag == TAG_JUMP)
			ta = (TagArg *) ta->ta_Arg;
		else
			ta++;
	}
	return (NULL);
}


void
openstuff ()
{
	if (OpenGraphicsFolio () < 0)
		die ("Can't open graphics.\n");

	if ((vblio = CreateVBLIOReq ()) < 0)
		die ("Can't get VBLIOReq.\n");
}

void
closestuff ()
{
	DeleteVBLIOReq (vblio);
	CloseGraphicsFolio ();
}

void
die (str)
char *str;
{
	printf (str);
	closestuff ();
	exit (20);
}
