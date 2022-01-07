/*
	 debugTest.c
	 
	 Debugger Folio Test
	 
	 by Anup Murarka
	 
	 ©1994 by The 3DO Company, all rights reserved
			confidential and proprietary	 
	 
*/


#include "debuggerfolio.h"

// Macros for debugging
#define PRT(x)    { printf x ; }
#define ERR(x)    { printf x ; }
#define DBUG(x)   PRT(x)

// Macro to simplify error checking
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		PrintfSysErr(val); \
		goto cleanup; \
	} \
	else \
	{ \
		PRT(("%s succeeded!\n", name)); \
	}

//@@@@@ normally automagically called swi's
int32	__swi(DebuggerSWI+0x0010) DebugTaskLaunch( Task* taskControl, void*	startAddr );
int32	__swi(DebuggerSWI+0x0011) DebugTaskKill( Item taskID );

int main( )
{
	int32 Result;
	Item Bone1, Bone2;
	Item DebuggerFolioNum;
	
	//Result = OpenDebuggerFolio();
	//CHECKRESULT( Result, "OpenDebuggerFolio" );

	//@@@@@ normally called for us by cstartup.o or by OpenDebuggerFolio
	//DebugTaskLaunch(CURRENTTASK, (void*) &main);

	kprintf("running test program\n");

	kprintf("exiting test program\n");

	//@@@@@ normally called for us by cstartup.o or by CloseDebuggerFolio()
	DebugTaskKill(CURRENTTASK->t.n_Item);			// swi call

	return (int) 0;
}
