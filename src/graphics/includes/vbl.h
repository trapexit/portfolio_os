/*  :ts=8 bk=0
 *
 * vbl.h:	Definitions for VBL manager.
 *
 * Leo L. Schwab					9407.07
 ***************************************************************************
 * Copyright 1994 The 3DO Company.  All Rights Reserved.
 *
 * 3DO Trade Secrets  -  Confidential and Proprietary
 ***************************************************************************
 *			     --== RCS Log ==--
 * $Id: vbl.h,v 1.2 1994/09/14 03:11:22 ewhac Exp $
 *
 * $Log: vbl.h,v $
 * Revision 1.2  1994/09/14  03:11:22  ewhac
 * Added prototypes for [GS]etVBLAttrsVA().
 *
 * Revision 1.1  1994/08/25  23:13:03  ewhac
 * Initial revision
 *
 */
#ifndef	__VBL_H
#define	__VBL_H

/*
 * Tag arguments for fiddling with the VBL VDLs.
 */
enum VBLTags {
	VBL_TAG_LENGTH = 256,
	VBL_TAG_FORCELINE,	/*  Line # where forced first happens	*/
	VBL_TAG_SLIPLINE,	/*  Line # to enable SlipStream		*/
	VBL_TAG_SLIPSTREAM,	/*  Turn on/off SlipStream		*/
	VBL_TAG_VIRS,		/*  Turn on/off internal VIRS generation*/
	VBL_TAG_REPORTSPORTLINES,
	VBL_TAG_REPORTFORCEFIRST,
	VBL_TAG_REPORTPATCHADDR,
	MAX_VBL_TAG
};


Err	SetVBLAttrs (TagArg *args);
Err	SetVBLAttrsVA (uint32 arg, ...);
Err	GetVBLAttrs (TagArg *args);
Err	GetVBLAttrsVA (uint32 arg, ...);

Err	SuperSetVBLAttrs (TagArg *args);
Err	SuperGetVBLAttrs (TagArg *args);
Err	SuperInternalGetVBLAttrs (TagArg *args);


#endif	/*  __VBL_H  */
