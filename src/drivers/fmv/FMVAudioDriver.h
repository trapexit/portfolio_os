/*
	File:		FMVAudioDriver.h

	Contains:	xxx put contents here xxx

	Written by:	George Mitsuoka

	Copyright:	© 1993 by 3DO, Inc., all rights reserved.

	Change History (most recent first):

		 <7>	 4/25/94	BCK		Added FMVAudioSetSWPTSMechanism().
		 <6>	 4/11/94	BCK		Removed interrupt routines.
		 <5>	 2/28/94	GDW		Removed unused routines.
		 <4>	  2/8/94	GDW		Added prototype for control.
		 <3>	  2/7/94	GDW		Added Status and Control routine changes for new API.
		 <2>	11/19/93	GDW		Added additional functions for additional stream support.
*/

/* file: FMVAudioDriver.h */
/* FMV audio driver definitions */
/* 4/25/93 George Mitsuoka */
/* The 3DO Company Copyright 1993 */

#ifndef FMVAUDIODRIVER_HEADER
#define FMVAUDIODRIVER_HEADER

#include "device.h"

int32 FMVAudioInit( void );
void FMVAudioDevOpen( Device *dev );
void FMVAudioDevClose( Device *dev );
void FMVAudioAbortIO( IOReq *io );
int32 FMVAudioCmdRead( IOReq *ior );
int32 FMVAudioCmdWrite( IOReq *ior );
int32 FMVAudioCmdPlay( void );
int32 FMVAudioCmdStatus( IOReq *ior );
int32 FMVAudioCmdControl( IOReq *ior );
void FMVAudioSetSWPTSMechanism(int32 useSWPTS);

#endif
