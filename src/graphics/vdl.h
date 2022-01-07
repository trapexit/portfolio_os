/*  :ts=8 bk=0
 *
 * vdl.h:	Standard structure definitions for VDL's.
 *
 * Leo L. Schwab					9407.02
 ***************************************************************************
 * Copyright 1994 The 3DO Company.  All Rights Reserved.
 *
 * 3DO Trade Secrets  -  Confidential and Proprietary
 ***************************************************************************
 *			     --== RCS Log ==--
 * $Id: vdl.h,v 1.1 1994/08/25 23:10:59 ewhac Exp $
 *
 * $Log: vdl.h,v $
 * Revision 1.1  1994/08/25  23:10:59  ewhac
 * Initial revision
 *
 */
#ifndef	__VDL_H
#define	__VDL_H

#ifndef	__HARDWARE_H
#include <hardware.h>
#endif


/***************************************************************************
 * #defines and constants.
 */
#define VDL_DISP_NOP		(VDL_DISPCTRL | \
				 VDL_BLSB_NOP | VDL_WINBLSB_NOP | \
				 VDL_HSUB_NOP | VDL_WINHSUB_NOP | \
				 VDL_VSUB_NOP | VDL_WINVSUB_NOP)

#define	VDL_DISP_SLIPBITS	(VDL_SLPDCEL | VDL_BACKTRANS)


/***************************************************************************
 * Structure definitions.
 */
/*
 * Common header to all VDL lists.  These four fields are required by
 * the Opera hardware, in this order.
 */
typedef struct VDLHeader {
	uint32			DMACtl;
	void			*CurBuf;
	void			*PrevBuf;
	struct VDLHeader	*NextVDL;
} VDLHeader;


/*
 * The most common flavor of VDL.  The 33rd color is the background register.
 */
typedef struct FullVDL {
	struct VDLHeader	fv;
	uint32			fv_DispCtl;
	uint32			fv_Colors[33];
} FullVDL;

#define	VDL_LEN_FULL		34
#define	VDL_LEN_FULL_FMT	(VDL_LEN_FULL << VDL_LEN_SHIFT)


/*
 * This flavor is used only by the system internally for stuff like the
 * forced CLUT transfer at VBlank and whatnot...
 */
typedef struct ShortVDL {
	struct VDLHeader	sv;
	uint32			sv_DispCtl;
	uint32			sv_Misc[3];
} ShortVDL;

#define	VDL_LEN_SHORT		4
#define	VDL_LEN_SHORT_FMT	(VDL_LEN_SHORT << VDL_LEN_SHIFT)


#endif	/*  __VDL_H  */
