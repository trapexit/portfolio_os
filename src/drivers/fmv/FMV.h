/*
	File:		FMV.h

	Contains:	xxx put contents here xxx

	Written by:	xxx put writers here xxx

	Copyright:	© 1994 by 3DO, Inc., all rights reserved.

	Change History (most recent first):

		 <1>	 2/25/94	GDW		first checked in
		 <2>	 2/10/94	GM		Changed DEBUGP macro to prefix all debugging printfs with
									"FMVDriver: "

*/

/* file: FMV.h */
/* define things here that change between versions */
/* 4/24/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#ifndef FMV_HEADER
#define FMV_HEADER

#ifdef THINK_C

#define DEBUG(args)

#else

#include "super.h"

#define DEBUGP(args)		{ Superkprintf("FMVDriver: "); Superkprintf args; }

// #define WOODYROMADDRFUCKED
#define FMVINROM

#include "task.h"

extern struct Task *gMainTask;
extern Item gClientTaskItem;
extern int32 gSignalVideoMode16, gSignalVideoMode24;
extern int32 gSignalAudioPatch1, gSignalAudioPatch2;
extern int32 gSignalLoaded, gSignalLoadError, gSignalDone, *gUcodeBuffer;

#endif

#endif
