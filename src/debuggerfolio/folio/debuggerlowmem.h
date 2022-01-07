/* $Id: debuggerlowmem.h,v 1.3 1994/08/02 02:45:07 anup Exp $ */

/*
 * $Log: debuggerlowmem.h,v $
 * Revision 1.3  1994/08/02  02:45:07  anup
 * Added Id and Log keywords
 *
 */

/*
	File:		DebuggerLowMem.h

	Contains:	Declarations for low memory constants used by the debugger

	Written by:	Anup K Murarka

	Copyright:	©1994 by The 3DO Company, all rights reserved
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

				08.07.94	akm		First Writing

	To Do:
*/

typedef uint32	ARMOpcode;
typedef void	(*ExceptionVectorProc)(void);
// This may actually take parameters in the future
typedef uint32 (*FolioEntryVectorProc)(void);


#ifndef __cplusplus

// Low mem vector which has the opcode for branching to the SWI handler
#define SWIVector				((ARMOpcode*) 0x08)

#define	DebugFolioPublicFlags	((DebuggerFolioFlags*) 0x00000180)
#define SWITraceProc			((ExceptionVectorProc*) 0x00000198)
#define FolioEntryProc			((FolioEntryVectorProc*) 0x0000019C)

#define RESETVectorProc			((ExceptionVectorProc*) 0x000001E0)
#define UNDEFectorProc			((ExceptionVectorProc*) 0x000001E4)
#define SWIVectorProc			((ExceptionVectorProc*) 0x000001E8)
#define ABORTPREVectorProc		((ExceptionVectorProc*) 0x000001EC)
#define ABORTDATAVectorProc		((ExceptionVectorProc*) 0x000001F0)
#define ADDREXECPTVectorProc	((ExceptionVectorProc*) 0x000001F4)
#define IRQVectorProc			((ExceptionVectorProc*) 0x000001F8)
#define FIRQVectorProc			((ExceptionVectorProc*) 0x000001FC)

#else

const ARMOpcode*				RESETVector = 0x00;
const ARMOpcode*				UNDEFVector = 0x04;
const ARMOpcode*				SWIVector = 0x08;

const DebuggerFolioFlags*		DebugFolioPublicFlags = 0x0180;
const ExceptionVectorProc*		SWITraceProc = 0x0198;
const FolioEntryVectorProc*	FolioEntryProc = 0x019C;

const ExceptionVectorProc*		RESETVectorProc = 0x01E0;
const ExceptionVectorProc*		UNDEFectorProc = 0x01E4;
const ExceptionVectorProc*		SWIVectorProc = 0x01E8;
const ExceptionVectorProc*		ABORTPREVectorProc = 0x01EC;
const ExceptionVectorProc*		ABORTDATAVectorProc = 0x01F0;
const ExceptionVectorProc*		ADDREXECPTVectorProc = 0x01F4;
const ExceptionVectorProc*		IRQVectorProc = 0x01F8;
const ExceptionVectorProc*		FIRQVectorProc = 0x01FC;

#endif
