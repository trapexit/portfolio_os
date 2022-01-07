/*
	File:		FMVVideoDriver.h

	Contains:	xxx put contents here xxx

	Written by:	George Mitsuoka

	Copyright:	© 1993 by 3DO, Inc., all rights reserved.

	Change History (most recent first):

		 <6>	11/17/94	GDW		Added pause routine.
		 <5>	  3/1/94	GM		Fixed prototypes to match latest FMVVideoDriver.c
		 <4>	 2/25/94	GM		Added prototypes for FMVVideoCmdSkipFrames and
									FMVVideoCmdKeyFrames.
		 <3>	  2/7/94	GDW		Added Status and Control routine changes for new API.
		 <2>	11/17/93	GDW		Added some prototypes to get rid of warnings.

*/

/* file: FMVVideoDriver.h */
/* FMV video driver definitions */
/* 4/28/93 George Mitsuoka */
/* The 3DO Company Copyright 1993 */

#ifndef FMVVIDEODRIVER_HEADER
#define FMVVIDEODRIVER_HEADER

#include "device.h"

int32 FMVVideoDMAtoRAMIntr( void );
int32 FMVVideoDMAfrRAMIntr( void );
int32 FMVVideoIntr( void );
void FMVDoPixClkDMA( void );
int32 FMVVideoInit( void );
void FMVVideoDevOpen( Device *dev );
void FMVVideoDevClose( Device *dev );
void FMVVideoAbortIO( IOReq *io );
int32 FMVVideoCmdRead( IOReq *ior );
int32 FMVVideoCmdWrite( IOReq *ior );
int32 FMVVideoCmdStep( IOReq *ior );
int32 FMVVideoCmdPixelMode16( void );
int32 FMVVideoCmdPixelMode24( void );
int32 FMVVideoSetPixelMode( void );
int32 FMVVideoCmdPlay( void );
int32 FMVVideoCmdPause( uint32 frameType );
int32 FMVVideoCmdStop( IOReq *ior );
int32 FMVVideoCmdSetSize( IOReq *ior );
int32 FMVVideoCmdModeSquare( void );
int32 FMVVideoCmdModeNTSC( void );
int32 FMVVideoCmdModePAL( void );
int32 FMVVideoCmdSetRandomDither( IOReq *ior );
int32 FMVVideoCmdSetMatrixDither( IOReq *ior );
int32 FMVVideoCmdSkipFrames( int32 count );
int32 FMVVideoCmdKeyFrames( void );
int32 FMVVideoCmdStatus( IOReq *ior );
int32 FMVVideoCmdControl( IOReq *ior );

#endif
