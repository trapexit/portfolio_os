/*  :ts=8 bk=0
 *
 * vdltest.c:	Test some VDL stuff.
 *
 * Leo L. Schwab					9410.10
 ***************************************************************************
 * Copyright 1994 The 3DO Company.  All Rights Reserved.
 *
 * 3DO Trade Secrets  -  Confidential and Proprietary
 ***************************************************************************
 * $Id: vdltest.c,v 1.2 1994/10/28 05:31:50 ewhac Exp $
 ***************************************************************************
 * Usage guide for hapless new users.
 *
 * vdltest [-c] [-i] [-n<#entries>] [-d<addr>]
 *
 * where:
 *	-c:	Create custom VDL.
 *	-i:	Install (SetVDL()) created VDL to Screen.
 *	-n#:	Create # VDL units over height of Screen (default =
 *		height of screen).
 *	-d<adrr>:
 *		Diagnose VDL at address <addr>.  <addr> may be in hex,
 *		octal, or decimal.
 *
 *	-c or -i must be specified for the Screen to be displayed.
 *
 * While inside, the control pad does the following:
 *	A:		Toggle horizontal averaging via {En,Dis}ableHAVG().
 *	B:		Toggle vertical averaging via {En,Dis}ableVAVG().
 *	Left Shift:	Toggle horizontal averaging via ModifyVDL().
 *	Right Shift:	Togger vertical averaging via ModifyVDL().
 *	X/Stop:		Exit program.
 */
#include <types.h>
#include <graphics.h>
#include <event.h>
#include <mem.h>
#include <stdio.h>
#include <string.h>

#include "vdl.h"	/*  Hey!  No fair peeking!  :-)  */


#define	NSCREENS	1


typedef struct RastPort {
	struct Screen	*rp_Screen;
	struct Bitmap	*rp_Bitmap;
	Item		rp_ScreenItem;
	Item		rp_BitmapItem;
	int32		rp_RastSize;
	int32		rp_RastPages;
} RastPort;


/* vdltest.c */
int main(int ac, char **av);
void drawscreen(struct RastPort *rp);
Item buildVDL(struct RastPort *rp, int32 nunits);
void diagnoseVDL(uint32 *inssrc);
uint32 gimmedispmod(int32 width);
uint32 advanceraster (uint32 startaddr, int32 linewidth, int32 nlines);
void parseargs(int ac, char **av);
long getjoybits(long shiftbits);
void openstuff(void);
void closestuff(void);
void die(char *str);



int32		nunits;
int		create, install, diagnose;
void		*diagaddr;


Item		vblIO, vramIO;
Item		sgrpitem;
RastPort	rports[1], *rp = rports;

TagArg		scrtags[] = {
	CSG_TAG_SCREENCOUNT,	(void *) NSCREENS,
	CSG_TAG_DONE,		0
};

TagArg		vdltags[] = {
	CREATEVDL_TAG_HAVG,	0,
	CREATEVDL_TAG_VAVG,	0,
	TAG_END,		0
};

int32		*havg = (int32 *) &vdltags[0].ta_Arg;
int32		*vavg = (int32 *) &vdltags[1].ta_Arg;




int
main (ac, av)
int	ac;
char	**av;
{
	Item	vdli;
	Err	err;

	parseargs (ac, av);
	openstuff ();

	drawscreen (rp);

	if (create) {
		if (!nunits)
			nunits = rp->rp_Bitmap->bm_Height;

		if ((vdli = buildVDL (rp, nunits)) < 0)
			die ("VDL creation failed.\n");
	}

	if (install)
		if (SetVDL (rp->rp_ScreenItem, vdli) < 0)
			die ("SetVDL() failed.\n");

	if (create  ||  install)
		DisplayScreen (rp->rp_ScreenItem, 0);

	if (diagnose) {
		if (!diagaddr) {
			VDL	*vdl;

			if (!(vdl = LookupItem (vdli)))
				die ("Diagnose what!?\n");

			diagaddr = vdl->vdl_DataPtr;
		}
		diagnoseVDL (diagaddr);
	} else {
		int	hstate, vstate;

		hstate = vstate = 0;
		while (1) {
			uint32	bits;

			err = 0;
			if ((bits = getjoybits (ControlX)) == ControlX)
				break;
			if (bits & ControlA) {
				if (hstate)
					err = DisableHAVG (rp->rp_ScreenItem);
				else
					err = EnableHAVG (rp->rp_ScreenItem);
				hstate ^= 1;
			}
			if (bits & ControlB) {
				if (vstate)
					err = DisableVAVG (rp->rp_ScreenItem);
				else
					err = EnableVAVG (rp->rp_ScreenItem);
				vstate ^= 1;
			}
			if (bits & ControlLeftShift) {
				*havg ^= 1;
				err = ModifyVDL (vdli, vdltags);
			}
			if (bits & ControlRightShift) {
				*vavg ^= 1;
				err = ModifyVDL (vdli, vdltags);
			}
			if (err)
				PrintfSysErr (err);
			WaitVBL (vblIO, 2);
		}
	}
	closestuff ();
	printf ("Normal exit.\n");
}







void
drawscreen (rp)
struct RastPort	*rp;
{
	register int	i;
	GrafCon		gc;
	Rect		r;

	r.rect_YTop = 0;
	r.rect_YBottom = rp->rp_Bitmap->bm_Height;

	for (i = 32;  --i >= 0; ) {
		r.rect_XRight = (r.rect_XLeft = i * 10) + 2;
		SetFGPen (&gc, MakeRGB15 (0, 0, i));
		FillRect (rp->rp_BitmapItem, &gc, &r);

		r.rect_XLeft = r.rect_XRight;
		r.rect_XRight += 2;
		SetFGPen (&gc, MakeRGB15 (i, 0, 0));
		FillRect (rp->rp_BitmapItem, &gc, &r);

		r.rect_XLeft = r.rect_XRight;
		r.rect_XRight += 2;
		SetFGPen (&gc, MakeRGB15 (0, i, 0));
		FillRect (rp->rp_BitmapItem, &gc, &r);

		r.rect_XLeft = r.rect_XRight;
		r.rect_XRight += 4;
		SetFGPen (&gc, MakeRGB15 (i, i, i));
		FillRect (rp->rp_BitmapItem, &gc, &r);
	}
}



Item
buildVDL (rp, nunits)
struct RastPort	*rp;
int32		nunits;
{
	FullVDL	*fv, *fvbuf;
	VDL	*vdl;
	Bitmap	*bm;
	Item	vdli;
	uint32	dispmod;
	uint32	curbuf, prevbuf;
	int32	size, wide, high;
	int32	vhigh;
	int32	gun;
	int	i, ii, idx;

	bm = rp->rp_Bitmap;
	wide = bm->bm_Width;
	high = bm->bm_Height;

	size = sizeof (FullVDL) * nunits;
	if (!(fvbuf = AllocMem (size, MEMTYPE_VRAM | MEMTYPE_FILL)))
		die ("Can't allocate VRAM for VDL.\n");
	fv = fvbuf;

	dispmod = gimmedispmod (wide);

	curbuf = prevbuf = (uint32) bm->bm_Buffer;
	for (i = nunits, ii = 0;  --i >= 0;  fv++, ii++) {
		vhigh = (high * (i + 1)) / nunits  -  (high * i) / nunits;

		fv->fv.DMACtl	= VDL_ENVIDDMA | VDL_LDCUR | VDL_LDPREV |
				  VDL_LEN_FULL_FMT | dispmod |
				  (vhigh << VDL_LINE_SHIFT);
		fv->fv.CurBuf	= (void *) curbuf;
		fv->fv.PrevBuf	= (void *) prevbuf;
		fv->fv.NextVDL	= (VDLHeader *) (fv + 1);

		fv->fv_DispCtl	= DEFAULT_DISPCTRL;

		/*
		 * Slowly invert colorset from top to bottom.
		 */
		for (idx = 32;  --idx >= 0; ) {
			gun = (idx << 3) | (idx >> 2);
			gun = (i * gun  +  (nunits - 1 - i) * (255 - gun)) /
				(nunits - 1);

			fv->fv_Colors[idx] =
			 MakeCLUTColorEntry (idx, gun, gun, gun);
			if (!idx)
				fv->fv_Colors[32] =
				 MakeCLUTBackgroundEntry (gun, gun, gun);
		}

		prevbuf = advanceraster (prevbuf, wide, curbuf == prevbuf  ?
							vhigh - 1  :  vhigh);
		curbuf = advanceraster (curbuf, wide, vhigh);
	}
	fv--;
	fv->fv.NextVDL = NULL;

	if ((vdli = SubmitVDL ((VDLEntry *) fvbuf, size, VDLTYPE_FULL)) < 0)
	{
		PrintfSysErr (vdli);
		printf ("VDL @ 0x%08lx rejected by system.\n", fvbuf);
		return (vdli);
	}
	vdl = LookupItem (vdli);

	printf ("Custom VDL installed.\n");
	printf ("Source @ 0x%08lx, system copy @ 0x%08lx\n",
		fvbuf, vdl->vdl_DataPtr);

	return (vdli);
}




void
diagnoseVDL (inssrc)
uint32	*inssrc;
{
	VDLHeader	*vh, *nextvdl;
	int32		pagesize;
	int32		line, nlines, num;
	int		pageshift;

	pagesize = GetPageSize (MEMTYPE_VRAM);
	pageshift = 0;
	while (1 << pageshift != pagesize)
		pageshift++;

	vh = (VDLHeader *) inssrc;
	line = num = 0;

	do {
		uint32	dma;
		uint32	p1, p2;
		uint32	*things;
		int32	len;
		int	i;

		printf ("Iteration #%d (Line #%d) @ 0x%08lx\n",
			num++, line, vh);

		p1 = (uint32) vh;
		p2 = (uint32) (vh + 1) - 1;
		if (p1 >> pageshift != p2 >> pageshift)
			printf
			 ("0x%08lx: VDLHeader crosses VRAM page boundary!\n",
			  vh);

		dma = vh->DMACtl;
		printf ("DMACtl = 0x%08lx, ", dma);

		nextvdl = vh->NextVDL;
		if (dma & VDL_RELSEL) {
			nextvdl = (VDLHeader *)
				   ((int32) nextvdl + (int32) (vh + 1));
			printf ("NextVDL @ 0x%08lx (Relative offset 0x%08lx)\n",
				nextvdl, vh->NextVDL);
		} else
			printf ("NextVDL @ 0x%08lx\n", nextvdl);

		printf ("CurBuf @ 0x%08lx, PrevBuf @ 0x%08lx\n",
			vh->CurBuf, vh->PrevBuf);

		len = (dma & VDL_LEN_MASK) >> VDL_LEN_SHIFT;
		nlines = (dma & VDL_LINE_MASK) >> VDL_LINE_SHIFT;
		p1 = (uint32) vh;
		p2 = (uint32) (vh + 1) + len * sizeof (uint32) - 1;
		if (p1 >> pageshift != p2 >> pageshift)
			printf ("VDL body crosses VRAM page boundary!\n");
		printf ("NLines = %d, Length = %d (+ 4):\n    ", nlines, len);

		things = (uint32 *) (vh + 1);
		for (i = 1;  i <= len;  i++) {
			printf ("0x%08lx", *things++);
			if (i != len)
				if (i & 3)
					printf (", ");
				else
					printf ("\n    ");

		}
		printf ("\n\n");

		line += nlines;
		vh = nextvdl;
	} while (nlines > 0  &&  vh);
}


uint32
gimmedispmod (width)
int32	width;
{
	switch (width) {
	case 320:
		return (VDL_DISPMOD_320);
	case 384:
		return (VDL_DISPMOD_384);
	case 512:
		return (VDL_DISPMOD_512);
	case 640:
		return (VDL_DISPMOD_640);
	case 1024:
		return (VDL_DISPMOD_1024);
	default:
		die ("Bad bitmap width (huh?).\n");
	}
}


uint32
advanceraster (startaddr, linewidth, nlines)
uint32	startaddr;
int32	linewidth, nlines;
{
	register uint32	addr;

	addr = startaddr;
	addr += (nlines >> 1) * linewidth * sizeof (uint32);
	if (nlines & 1) {
		if ((uint32) startaddr & 2)
			addr += linewidth * sizeof (uint32) - sizeof (uint16);
		else
			addr += sizeof (uint16);
	}
	return (addr);
}




void
parseargs (ac, av)
int	ac;
char	**av;
{
	char	*arg, c;

	while (++av, --ac) {
		arg = *av;
		if (*arg = '-') {
			switch (c = *(arg + 1)) {
			case 'c':
				create = TRUE;
				break;
			case 'i':
				install = TRUE;
				break;
			case 'd':
				diagnose = TRUE;
				diagaddr = (void *) strtol (arg + 2, NULL, 0);
				break;
			case 'n':
				nunits = strtol (arg + 2, NULL, 0);
				break;
			default:
				goto badarg;	/*  Look down.  */
			}
		} else {
badarg:			printf ("\"%s\":  ", arg);
			die ("Unknown argument.\n");
		}
	}
}


/***************************************************************************
 * Returns newly pressed bits in joystick.
 */
long
getjoybits (shiftbits)
long	shiftbits;
{
	static long		prevbits;
	register long		newbits, changed;
	ControlPadEventData	cped;

	/*  Which bits have changed since last test?  */
	if (GetControlPad (1, FALSE, &cped) < 0) {
		printf ("GetControlPad() failed.\n");
		return (0);
	}
	newbits = cped.cped_ButtonBits;
	changed = newbits ^ prevbits;

	/*  Return only positive transitions.  */
	changed = changed & newbits;

	/*  OR in current state of "shift" bits.  */
	changed |= newbits & shiftbits;
	prevbits = newbits;
	return (changed);
}


/***************************************************************************
 * Housekeeping
 */
void
openstuff ()
{
	RastPort	*rp;
	Item		scritems[NSCREENS];
	Err		err;
	int32		pagesize;
	int		n;

	if (OpenGraphicsFolio () < 0)
		die
		 ("Can't open graphics; please thump yourself in the head.\n");

	pagesize = GetPageSize (MEMTYPE_VRAM);

	if ((sgrpitem = CreateScreenGroup (scritems, scrtags)) < 0)
		die ("Can't create screengroup.\n");

	/*
	 * Fill in my RastPort thingies.
	 */
	for (rp = rports, n = NSCREENS;  n--;  rp++) {
		register Bitmap	*bm;

		rp->rp_ScreenItem = scritems[n];
		if (!(rp->rp_Screen = (Screen *) LookupItem (scritems[n])))
			die ("Woah!  Where's the screen?\n");
		rp->rp_Bitmap = bm = rp->rp_Screen->scr_TempBitmap;
		rp->rp_BitmapItem = rp->rp_Bitmap->bm.n_Item;

		rp->rp_RastPages = (bm->bm_Width * ((bm->bm_Height + 1) >> 1) *
				    sizeof (int32) + pagesize - 1) / pagesize;
		rp->rp_RastSize = rp->rp_RastPages * pagesize;
	}

	vblIO = CreateVBLIOReq ();
	vramIO = CreateVRAMIOReq ();

	/*
	 * Initialize event system.
	 */
	if (InitEventUtility (1, 0, LC_Observer) < 0)
		die ("Failed to InitEventUtility\n");
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
