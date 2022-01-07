/* $Id: joytools.c,v 1.11 1994/03/01 01:36:27 shawn Exp $ */
/*
** SImple Joypad Access
** By:  Phil Burk
*/

/*
** Copyright (C) 1993, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

#include "types.h"
#include "kernel.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "mem.h"
#include "semaphore.h"
#include "io.h"
#include "strings.h"
#include "stdlib.h"
#include "debug.h"
#include "operamath.h"
#include "filefunctions.h"
#include "graphics.h"
#include "audio.h"
#include "stdio.h"
#include "event.h"

#include "audiodemo.h"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */


static ControlPadEventData cped;

/******************************************************************/
int32 InitJoypad( void )
{
	int32 Result;

	Result = InitEventUtility(1, 0, LC_ISFOCUSED);
	if (Result < 0)
	{
		PrintError(0,"init event utility in","InitJoypad",Result);
	}
	return Result;
}

/******************************************************************/
int32 TermJoypad( void )
{
	int32 Result;

	Result = KillEventUtility();
	if (Result < 0)
	{
		PrintError(0,"kill event utility in","TermJoypad",Result);
	}
	return Result;
}

/******************************************************************/
int32 ReadJoypad( uint32 *Buttons )
{
	int32 Result;
	Result = GetControlPad (1, FALSE, &cped);
	if (Result < 0) {
		PrintError(0,"get control pad data in","ReadJoypad",Result);
	}
	*Buttons = cped.cped_ButtonBits;
DBUG(("ReadJoypad: *Buttons = 0x%8x, err = 0x%x\n", *Buttons, Result ));
	return Result;
}

/******************************************************************/
int32 WaitJoypad( uint32 Mask, uint32 *Buttons )
{
	static int32 oldbutn = 0;
	int32 Result;
	int32 butn;
	do
	{
		Result = GetControlPad (1, TRUE, &cped);
		if (Result < 0)
		{
			PrintError(0,"read control pad in","WaitJoypad",Result);
			return Result;
		}
		butn = cped.cped_ButtonBits;
		if (butn == 0) oldbutn = 0;
	} while((butn&Mask) == (oldbutn&Mask));

	oldbutn = butn;
DBUG(("butn = 0x%x\n", butn));
	*Buttons = butn;
	return Result;
}
