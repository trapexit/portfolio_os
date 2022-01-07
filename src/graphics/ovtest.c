/*  :ts=8 bk=0
 *
 * ovtest.c:	A little tool to let me test the overlay facility.
 *		Based on the preliminary hack vdlhack.c.
 *
 * Leo L. Schwab					9404.28
 ***************************************************************************
 * Copyright 1994 The 3DO Company.  All Rights Reserved.
 *
 * 3DO Trade Secrets  -  Confidential and Proprietary
 ***************************************************************************
 *			     --== RCS Log ==--
 * $Id: ovtest.c,v 1.3 1994/10/28 05:31:50 ewhac Exp $
 *
 * $Log: ovtest.c,v $
 * Revision 1.3  1994/10/28  05:31:50  ewhac
 * Altered references to GetV{BL,RAM}IOReq() to CreateV{BL,RAM}IOReq().
 * Also changed corresponding DeleteItem() calls to DeleteV{BL,RAM}IOReq(),
 * where required.
 *
 * Revision 1.2  1994/06/10  01:51:40  ewhac
 * Added a few new goodies.
 *
 * Revision 1.1  1994/05/31  22:40:43  ewhac
 * Initial revision
 *
 */
#include <types.h>
#include <graphics.h>
#include <mem.h>
#include <stdio.h>
#include <string.h>

#ifdef INTERNAL_TEST
#include "overlay.h"	/*  Hey!  No fair peeking!  :-)  */
#endif


#define	PLAYSIZE	(16 * sizeof (uint32))


/* ovtest.c */
int main(int ac, char **av);
Item CreateBitmapTags(ulong arg, ...);
void createoverlay(void);
int32 bitmapsize(int32 x, int32 y);
void dumpScreenGroup(Item sgi);
void dumpScreen(Item si);
void dumpBitmap(Item bmi);
void dumpVDL(Item vdli);
void dumpOverlay(Item ovli);
void openstuff(void);
void closestuff(void);
void die(char *str);



/*
 * Data for keeping track of displays.
 */
Item		vramIO, vblIO;
Item		sgrpitem;
Item		scritems[1];
Item		ovbmi, ovi;

int32		ytop = 100;
int32		ohigh = 50;

TagArg		scrtags[] = {
	CSG_TAG_SCREENCOUNT,	(void *) 1,
	CSG_TAG_DONE,		0
};





int
main (ac, av)
int	ac;
char	**av;
{
	GrafCon	gc;
	Screen	*s = NULL;
	Bitmap	*bm;
	Item	bmi = 0;
	Rect	r;
	int32	dlay;
	int	backscreen = FALSE;
	char	*arg;
	char	buf[40];

	openstuff ();

	dlay = 240;
	ytop = 150;

	while (++av, --ac) {
		arg = *av;
		if (*arg == '-') {
			switch (*++arg) {
			case 'b':
				backscreen = TRUE;
				break;
			case 'n':
				dlay = atoi (++arg);
				break;
			case 'y':
				ytop = atoi (++arg);
				break;
			case 'h':
				ohigh = atoi (++arg);
				break;
			default:
				printf ("-%c: Unknown switch; ignored.\n",
					*arg);
				break;
			}
		} else
			printf ("%s: Unknown option, ignored.\n", arg);
	}

	if (backscreen) {
		sgrpitem = CreateScreenGroup (scritems, scrtags);
		if (sgrpitem < 0) {
printf ("failure code: %d\n", sgrpitem);
			die ("CreateScreenGroup failed.\n");
		}
		DisplayScreen (scritems[0], 0);
	}

	createoverlay ();


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
	}

	SetFGPen (&gc, MakeRGB15 (0, 0, 15));
	FillRect (ovbmi, &gc, &r);


	/*
	 * Dump data.
	 */
	printf ("sizeof (ItemNode) == %d\n\n", sizeof (ItemNode));

	printf ("******  Screen Data  ******\n");

	if (backscreen) {
		dumpScreenGroup (sgrpitem);
		dumpScreen (scritems[0]);
		dumpBitmap (bmi);
		dumpVDL (s->scr_VDLItem);
	} else
		printf ("-=# No background screen. #=-\n");

	printf ("******  Overlay Bitmap Data  ******\n");

	dumpBitmap (ovbmi);

#ifdef INTERNAL_TEST
	printf ("******  Overlay Data  ******\n");

	dumpOverlay (ovi);
#endif
	SetFGPen (&gc, MakeRGB15 (31, 31, 31));
	sprintf (buf, "Overlay ID#:  %08lx", ovi);
	MoveTo (&gc, 10, 0);
	DrawText8 (&gc, ovbmi, buf);

	sprintf (buf, "Overlay addr: 0x%08lx", LookupItem (ovi));
	MoveTo (&gc, 10, 8);
	DrawText8 (&gc, ovbmi, buf);


	/*
	 * Twiddle thumbs.
	 */
	WaitVBL (vblIO, dlay);

	DeleteItem (ovi);

	printf ("Deleted Overlay.\n");
}







Item
CreateBitmapTags (
uint32 arg,
...
)
{
	return (CreateBitmap ((TagArg *) &arg));
}



void
createoverlay ()
{
	int32	size;
	void	*buf;

	size = bitmapsize (320, ohigh);

	if ((buf = AllocMem
		    (size, MEMTYPE_VRAM | MEMTYPE_CEL | MEMTYPE_FILL)) == NULL)
		die ("Can't allocate overlay raster.\n");

	if ((ovbmi = CreateBitmapTags (CBM_TAG_WIDTH, 320,
				       CBM_TAG_HEIGHT, ohigh,
				       CBM_TAG_BUFFER, buf,
				       TAG_END)) < 0)
		die ("Can't create overlay Bitmap.\n");

	if ((ovi = DisplayOverlay (ovbmi, ytop)) < 0) {
		PrintfSysErr (ovi);
		die ("Can't create overlay Item.\n");
	}

}


int32
bitmapsize (x, y)
int32	x, y;
{
	/*
	 * Valid only for LRFORM buffers.
	 */
	y = (y + 1) >> 1;
	return (x * y * sizeof (int32));
}


/***************************************************************************
 * Structure printing.
 */
void
dumpScreenGroup (sgi)
Item	sgi;
{
	register ScreenGroup	*sg;

	sg = (ScreenGroup *) LookupItem (sgi);

	printf ("ScreenGroup Item #0x%08x (0x%08x)\n", sgi, sg);
}

void
dumpScreen (si)
Item	si;
{
	register Screen	*s;

	s = (Screen *) LookupItem (si);

	printf ("Screen Item #0x%08x (0x%08x)\n", si, s);
	printf (" scr_VDLPtr  = 0x%08lx\n", s->scr_VDLPtr);
	printf (" scr_VDLItem =#0x%08lx\n", s->scr_VDLItem);
	printf (" scr_VDLType = %d\n\n", s->scr_VDLType);
}

void
dumpBitmap (bmi)
Item	bmi;
{
	register Bitmap	*bm;

	bm = (Bitmap *) LookupItem (bmi);

	printf ("Bitmap Item #0x%08x (0x%08x)\n", bmi, bm);
	printf (" bm_Buffer = 0x%08lx\n\n", bm->bm_Buffer);
}

void
dumpVDL (vdli)
Item	vdli;
{
	register VDL	*vdl;

	vdl = (VDL *) LookupItem (vdli);

	printf ("VDL Item #0x%08x (0x%08x)\n", vdli, vdl);
	printf (" vdl_DataPtr  = 0x%08lx\n", vdl->vdl_DataPtr);
	printf (" vdl_Type     = %d\n", vdl->vdl_Type);
	printf (" vdl_DataSize = %d\n\n", vdl->vdl_DataSize);
}

#ifdef INTERNAL_TEST
void
dumpOverlay (ovi)
Item	ovi;
{
	register Overlay	*ov;

	ov = (Overlay *) LookupItem (ovi);

	printf ("Overlay Item #0x%08x (0x%08x)\n", ovi, ov);
	printf (" &ov_VDL[0] = 0x%08lx\n", &ov->ov_VDL[0]);
	printf (" receive[0] = 0x%08lx\n", ov->ov_VDL[0].ovv_Receive);
	printf (" handoff[0] = 0x%08lx\n\n", ov->ov_VDL[0].ovv_Handoff);
}
#endif





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
