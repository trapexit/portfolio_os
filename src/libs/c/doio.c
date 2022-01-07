/* $Id: doio.c,v 1.24 1994/11/17 03:22:42 vertex Exp $ */

#include "types.h"


/* In the past, io.h defined DoIO() as a regular functions which was
 * implemented in this source file, and part of clib.lib. io.h now defines
 * DoIO() as a SWI. The same is true for other functions like WaitIO(),
 * WaitPort(), etc.
 *
 * These stubs are here so that code compiled with the old version of io.h
 * and msgport.h will work with the new clib.lib. This is very useful for old
 * link libs that don't get recompiled.
 */


Err __swi(KERNELSWI+37) DoIOSWI(Item ior, void *ioiP);
Err __swi(KERNELSWI+41) WaitIOSWI(Item ior);
Item __swi(KERNELSWI+40) WaitPortSWI(Item mp,Item msg);


Err DoIO(Item ioReq, void *ioInfo)
{
    return DoIOSWI(ioReq,ioInfo);
}

Err WaitIO(Item ior)
{
    return WaitIOSWI(ior);
}

Item WaitPort(Item mp,Item msg)
{
    return WaitPortSWI(mp,msg);
}
