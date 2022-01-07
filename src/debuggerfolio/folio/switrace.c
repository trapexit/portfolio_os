/* $Id: switrace.c,v 1.4 1994/09/15 00:15:31 vertex Exp $ */

/*
 * $Log: switrace.c,v $
 * Revision 1.4  1994/09/15  00:15:31  vertex
 * No longer includes the private internalf.h kernel include file
 *
 * Revision 1.3  1994/08/02  02:41:25  anup
 * Added Id and Log keywords
 *
 */

/*
	File:		SWITrace.c

	Contains:	Code for implementing SWI trace mechanism

	Written by:	Anup K Murarka

	Copyright:
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

				08.07.94	akm		First Writing

	To Do:
*/

#include "types.h"
#include "debug.h"
#include "aif.h"
#include "kernel.h"

#include "debuggerlowmem.h"
#include "debuggerfolio.h"
#include "debuggerfolio_private.h"
#include "hostcomms.h"

#include "switrace.h"


typedef struct UserRegSet
{
	uint32	R0;
	uint32	R1;
	uint32	R2;
	uint32	R3;
	uint32	R4;
	uint32	R5;
	uint32	R6;
	uint32	R7;
	uint32	R8;
	uint32	R9;
	uint32	R10;
	uint32	R11;
	uint32	R12;
} UserRegSet;


static ARMOpcode	oldSWIVectorOpcode;
static Boolean	SWITraceInstalled;
extern uint32		OldSwiVector;					// data storage for old vector

extern void SwiTraceEntry(void);		// assembly SWI entry vector

void	SWIIntercept(uint32	swiOpcode, void*	returnAddr, UserRegSet*	registers);

#define SWITraceOpcode	((uint32)(0xE59FF000 + (uint32) SWITraceProc - 0x08))


Err InstallSWITrace(void)
{
	ARMOpcode	oldSWIHandler = *SWIVector;

	if (SWITraceInstalled)
	{
		return -1;
	}

	oldSWIVectorOpcode = oldSWIHandler;
	// now calculate the effective address of the old opcode
	// and store that where the trace handler can access it
	oldSWIHandler &= 0x00FFFFFF;	// mask the opcode
	oldSWIHandler <<= 2;			// mul by 4
	oldSWIHandler += 8;				// add the PC prefetch
	oldSWIHandler += (uint32) SWIVector;	// add the current PC

	OldSwiVector = oldSWIHandler;
	SWITraceInstalled = true;
	DebugFolioPublicFlags->SWITraceEnable = 1;

	*SWITraceProc = SwiTraceEntry;
	// adjust since the opcode is PC-relative to 0
	*SWIVector = (SWITraceOpcode - (uint32) SWIVector);

	//SDBUG(("Installed SWI Trace handler\n"));

	return 0;
}


Err	RemoveSWITrace(void)
{
	if (!SWITraceInstalled)
	{
		return -1;
	}

	*SWIVector = oldSWIVectorOpcode;
	OldSwiVector = 0;

	SWITraceInstalled = false;
	DebugFolioPublicFlags->SWITraceEnable = 0;

	//SDBUG(("Removed SWI Trace handler\n"));

	return 0;
}


Err	EnableSWITrace(Boolean	on)
{
	if (!SWITraceInstalled)
		return -1;

	DebugFolioPublicFlags->SWITraceEnable = on;

	return 0;
}

void SWIIntercept(uint32 swiOpcode, void* returnAddr, UserRegSet* registers)
{
	register SWITraceMessage *lmDemandBuf = (SWITraceMessage*) (0x03740000 - sizeof(MonitorBuffer));
	Task*		curTask;
	uint32		*curReg, *destReg;
	int			regNdx;

	// wait until the buffer is available
	EnableIrq();
	while (lmDemandBuf->mb_header.mb_Semaphore != 0)
		;

	lmDemandBuf->mb_header.mb_Semaphore = -1;			// we own the block

	lmDemandBuf->mb_header.mb_OwnerID = -1;				// debugger is recipient
	lmDemandBuf->mb_header.mb_CmdID = SWITRACE;
	lmDemandBuf->mb_header.mb_RequestStatus = nil;		//@@@@@may need async processing

	// specific data
	curTask = CURRENTTASK;
	lmDemandBuf->st_currentTask = curTask;
	lmDemandBuf->st_Opcode = swiOpcode;										// need assembly routine to get return addr
	lmDemandBuf->st_returnAddr = returnAddr;

	// copy the registers
	curReg = (uint32*) registers;
	destReg = lmDemandBuf->st_registers;
	for (regNdx = 0; regNdx < 14; regNdx++)
		*destReg++ = *curReg++;

	// finish up & send it
	lmDemandBuf->mb_header.mb_RequestReady = -1;

	SayHelloVector(0);
}
