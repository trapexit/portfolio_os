/*  :ts=8 bk=0
 *
 * bitmap.c:	Routines for bitmap management.
 *
 * Leo L. Schwab					9402.22
 ***************************************************************************
 * Copyright 1994 The 3DO Company.  All Rights Reserved.
 *
 * 3DO Trade Secrets  -  Confidential and Proprietary
 ***************************************************************************
 *			     --== RCS Log ==--
 * $Id: bitmap.c,v 1.3 1994/03/07 22:17:50 slandrum Exp $
 *
 * $Log: bitmap.c,v $
 * Revision 1.3  1994/03/07  22:17:50  slandrum
 * Got rid of compiler warnings
 *
 * Revision 1.2  1994/02/25  01:09:33  ewhac
 * Minor formatting changes.  (I'll get the hang of this yet...)
 *
 * Revision 1.1  1994/02/23  23:40:53  ewhac
 * Initial revision
 *
 */
#include <types.h>
#include <string.h>
#include <kernel.h>
#include <intgraf.h>
#include <inthard.h>


/***************************************************************************
 * #defines
 */
#define	SSSDBUG(x)


/***************************************************************************
 * Local structures.
 */
struct _bmi {
	int32	w, h, cw, ch, cx, cy, wdog, cectrl;
	void	*bmp;
};


/***************************************************************************
 * Vobal Glariables.
 */
static ValidWidth	validwidths[] = {
 {	32,	RMOD_32   | WMOD_32,	_UNDISPLAYABLE	},
 {	64,	RMOD_64   | WMOD_64,	_UNDISPLAYABLE	},
 {	96,	RMOD_96   | WMOD_96,	_UNDISPLAYABLE	},
 {	128,	RMOD_128  | WMOD_128,	_UNDISPLAYABLE	},
 {	160,	RMOD_160  | WMOD_160,	_UNDISPLAYABLE	},
 {	256,	RMOD_256  | WMOD_256,	_UNDISPLAYABLE	},
 {	320,	RMOD_320  | WMOD_320,	VDL_DISPMOD_320	},
 {	384,	RMOD_384  | WMOD_384,	VDL_DISPMOD_384	},
 {	512,	RMOD_512  | WMOD_512,	VDL_DISPMOD_512	},
 {	576,	RMOD_576  | WMOD_576,	_UNDISPLAYABLE	},
 {	640,	RMOD_640  | WMOD_640,	VDL_DISPMOD_640	},
 {	1024,	RMOD_1024 | WMOD_1024,	VDL_DISPMOD_1024},
 {	1056,	RMOD_1056 | WMOD_1056,	_UNDISPLAYABLE	},
 {	1088,	RMOD_1088 | WMOD_1088,	_UNDISPLAYABLE	},
 {	1152,	RMOD_1152 | WMOD_1152,	_UNDISPLAYABLE	},
 {	1280,	RMOD_1280 | WMOD_1280,	_UNDISPLAYABLE	},
 {	1536,	RMOD_1536 | WMOD_1536,	_UNDISPLAYABLE	},
 {	2048,	RMOD_2048 | WMOD_2048,	_UNDISPLAYABLE	},
 {	0,	0,			0		},
};


/***************************************************************************
 * Code.
 */
static int32
icb
(
struct Bitmap	*bm,
struct _bmi	*bmi,
uint32		t,
uint32		a
)
{
	SSSDBUG (("Enter icb %lx %lx\n", t, a));
	switch (t) {
	case CBM_TAG_WIDTH:
		bmi->w = a;
		break;
	case CBM_TAG_HEIGHT:
		bmi->h = a;
		break;
	case CBM_TAG_BUFFER:
		bmi->bmp = (void *) a;
		break;
	case CBM_TAG_CLIPWIDTH:
		bmi->cw = a;
		break;
	case CBM_TAG_CLIPHEIGHT:
		bmi->ch = a;
		break;
	case CBM_TAG_CLIPX:
		bmi->cx = a;
		break;
	case CBM_TAG_CLIPY:
		bmi->cy = a;
		break;
	case CBM_TAG_WATCHDOGCTR:
		bmi->wdog = a;
		break;
	case CBM_TAG_CECONTROL:
		bmi->cectrl = a;
		break;
	default:
		return (GRAFERR_BADTAG);
	}
	return 0;
}


Item
internalCreateBitmap
(
struct Bitmap	*bm,
struct TagArg	*args
)
{
	struct _bmi	bmi;
	ValidWidth	*vw;
	int32		rc0;
	Err		e;

	SSSDBUG (("internalCreateBitmap(%lx,%lx)\n", (int32)bm, (int32)args));

	memset (&bmi, 0, sizeof (bmi));

	bmi.wdog = WATCHDOG_DEFAULT;
	bmi.cectrl = CECONTROL_DEFAULT;

	if ((e = TagProcessor (bm, args, icb, &bmi)) < 0)
		return (e);

	if ((vw = FindValidWidth (bmi.w, 0)) == 0)
		return (GRAFERR_BUFWIDTH);

	rc0 = vw->vw_CelMods;
	bmi.w = vw->vw_Width;	/*  In case it got bumped.  */


	if (bmi.h < 1  ||  bmi.h > (1 << 11))
		return (GRAFERR_BADBITMAPSPEC);

	e = SuperValidateMem (CURRENTTASK,
			      (uint8 *) bmi.bmp,
			      bmi.w * bmi.h * 2);
	if (e < 0)
		/*  Shouldn't we return e?  */
		return (GRAFERR_NOWRITEACCESS);

	if (!bmi.cw)
		bmi.cw = bmi.w;
	if (!bmi.ch)
		bmi.ch = bmi.h;

	if (bmi.cx < 0  ||  bmi.cx >= bmi.w  ||
	    bmi.cy < 0  ||  bmi.cy >= bmi.h  ||
	    bmi.cw < 0  ||  bmi.cx + bmi.cw > bmi.w  ||
	    bmi.ch < 0  ||  bmi.cy + bmi.ch > bmi.h)
		return (GRAFERR_BADCLIP);

	bm->bm_Buffer		= (ubyte *) bmi.bmp;
	bm->bm_Width		= bmi.w;
	bm->bm_Height		= bmi.h;
	bm->bm_VerticalOffset	= 0;
	bm->bm_Flags		= 0;
	bm->bm_ClipWidth	= bmi.cw;
	bm->bm_ClipHeight	= bmi.ch;
	bm->bm_ClipX		= bmi.cx;
	bm->bm_ClipY		= bmi.cy;
	bm->bm_WatchDogCtr	= bmi.wdog >> 4;
	bm->bm_SysMalloc	= 0;
	bm->bm_CEControl	= bmi.cectrl;
	bm->bm_REGCTL0		= rc0;
	bm->bm_REGCTL1		= MAKE_REGCTL1 (bmi.w, bmi.h);
	bm->bm_REGCTL2		= (uint32) bmi.bmp;
	bm->bm_REGCTL3		= (uint32) bmi.bmp;

	InitList (&bm->bm_SharedList, "Bitmap shared access list\n");

	return (bm->bm.n_Item);
}


/**************************************************************************
 * The routine that discovers valid Bitmap widths, based on your needs.
 */
struct ValidWidth *
FindValidWidth (width, flags)
register int32	width;
register uint32	flags;
{
	register ValidWidth	*vw;
	register int32		i;

	for (vw = validwidths;  (i = vw->vw_Width) != 0;  vw++) {
		if (width > i)
			continue;

		if ((flags & BMF_DISPLAYABLE)  &&
		    vw->vw_DispMods == _UNDISPLAYABLE)
			continue;

		if (width == i)
			return (vw);

		if (flags & BMF_BUMPWIDTH)
			return (vw);
		else
			break;
	}
	return (NULL);
}
