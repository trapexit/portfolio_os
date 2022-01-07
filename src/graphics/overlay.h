/*  :ts=8 bk=0
 *
 * overlay.h:	Structure definitions for overlay management.
 *
 * Leo L. Schwab					9404.28
 ***************************************************************************
 * Copyright 1994 The 3DO Company.  All Rights Reserved.
 *
 * 3DO Trade Secrets  -  Confidential and Proprietary
 ***************************************************************************
 *			     --== RCS Log ==--
 * $Id: overlay.h,v 1.4 1994/10/24 05:20:45 ewhac Exp $
 *
 * $Log: overlay.h,v $
 * Revision 1.4  1994/10/24  05:20:45  ewhac
 * Added new flag bit to deal with VDL change tracking.
 *
 * Revision 1.3  1994/08/25  22:55:16  ewhac
 * Removed VDLHeader and FullVDL structure definitions to vdl.h.
 *
 * Revision 1.2  1994/06/10  01:50:56  ewhac
 * Added support for SetScreenColors().
 *
 * Revision 1.1  1994/05/31  22:43:10  ewhac
 * Initial revision
 *
 */
#ifndef	__OVERLAY_H
#define	__OVERLAY_H


#ifndef __GRAPHICS_H
#include <graphics.h>
#endif

#ifndef	__VDL_H
#include "vdl.h"
#endif


/*
 * Unpublished ItemNode Type value for Overlays.  The contents of this
 * Item are to remain unpublished (since it will one day be turning into
 * a Screen under the new paradigm).
 */
#define	TYPE_OVERLAY	6


/*
 * This structure encapsulates the VDL mods required to display an Overlay.
 * A total of four are required:  one for each video field, times two for
 * double buffering.
 */
typedef struct OverlayVDL {
	FullVDL	*ovv_Receive,
		*ovv_Handoff,
		*ovv_Stomped,		/*  FullVDL we stomped.	*/
		*ovv_StompNext;		/*  Original next ptr.	*/
	uint32	ovv_StompDMAC;		/*  Original DMACtl.	*/
	VDL	*ovv_Spliced;		/*  VDL we spliced.	*/
} OverlayVDL;


#define	N_OVVDLS	4

typedef struct Overlay {
	ItemNode	ov_Node;
	OverlayVDL	ov_VDL[N_OVVDLS];	/*  0,2 == Even field.	*/
	Bitmap		*ov_bm;
	Task		*ov_SigTask;		/*  For graphics FIRQ.	*/
	int32		ov_SigMask;
	int32		ov_XPos,
			ov_YPos,
			ov_Width,
			ov_Height;
	uint32		ov_Flags;
} Overlay;

#define	OVF_FREEBANK_0		0x1
#define	OVF_FREEBANK_1		0x2
#define	OVF_FREEBANKMASK	0x3
#define	OVF_PENDINGBANK_0	0x4
#define	OVF_PENDINGBANK_1	0x8
#define	OVF_PENDINGBANKMASK	0xC
#define	OVF_INSTALLBANK_0	0x10
#define	OVF_INSTALLBANK_1	0x20
#define	OVF_INSTALLBANKMASK	0x30
#define	OVF_INUSE		0x100
#define	OVF_DEATHLOCK		0x200
#define	OVF_COLORCHANGE		0x400
#define	OVF_VDLCHANGE		0x800

#define	PENDINGBANK_SHIFT	2
#define	INSTALLBANK_SHIFT	4


/*
 * Initialization tags.
 */
enum OverlayTags {
	OVTAG_BITMAPITEM = 256,
	OVTAG_TOPEDGE,
	MAX_OVTAG
};


#define	SPLICEOP_EXISTING	0
#define	SPLICEOP_NEW		1


/*
 * Values for the vdl_Flags field.
 */
#define	VDLF_SPLICED		1



#endif	/*  __OVERLAY_H  */
