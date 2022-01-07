/* $Id: touch_hardware.h,v 1.3 1994/09/08 21:10:30 phil Exp $ */

/****************************************************************
**
** Touch Opera Hardware or fake it with a PRT
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************
** 940907 PLB Change PutHard to WriteHardware
***************************************************************/

#ifndef _TOUCH_HARDWARE_H
#define _TOUCH_HARDWARE_H

#ifdef FAKETOUCH
extern int32 verbose;

#define WriteHardware(addr,val) \
	{ if(verbose) PRT(("WriteHardware: %9x to %9x\n", (uint32) addr, val)); }

#define ReadHardware(addr ) \
	{ if(verbose) PRT(("ReadHardware: %9x\n", (uint32) addr)); }

#else

#define WriteHardware(addr,val) { *(addr) = val; }
#define ReadHardware(addr) *(addr)
	
#endif

#endif
