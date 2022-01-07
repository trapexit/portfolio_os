/* $Id: fmv.c,v 1.3 1994/02/09 01:22:45 limes Exp $ */
/* file: fmv.c */
/* probe for fmv/external rom interface */

#include "types.h"
#include "io.h"
#include "driver.h"
#include "device.h"
#include "kernelnodes.h"
#include "debug.h"
#include "strings.h"
#include "kernel.h"
#include "operror.h"
#include "super.h"
#include "mem.h"
#include "inthard.h"
#include "setjmp.h"
#include "time.h"

extern Superkprintf(const char *fmt, ... );
#define DEBUG

#define DBUG(x)	 kprintf x
#define KBUG(x)	 /*Superkprintf x*/

extern int SoftReset(void);

int
probeFMV()
{
    Item ram;
    Item ramior;
    Err ret = 0;
    volatile uint32 romval = 0;
    IOInfo ioInfo;

    DBUG(("Probing for FMV\n"));
    /* this is wrong, all we need to do is wiggle the addresses and */
    /* see if something appears */
    ram = OpenNamedDevice("ram",0);
    ramior = CreateIOReq(0,0,ram,0);
    if (ramior < 0) goto byebye;

    memset(&ioInfo,0,sizeof(ioInfo));

    ioInfo.ioi_Recv.iob_Buffer = (void *)&romval;
    ioInfo.ioi_Recv.iob_Len = sizeof(romval);
    ioInfo.ioi_Unit = 4;
    ioInfo.ioi_Command = CMD_READ;

    ret = DoIO(ramior,&ioInfo);
    DBUG(("romval=%lx\n",romval));
    if (romval == 0xaa55aa55)
    {
	ret = 1;
   	DBUG(("Found rom preamble, but have we been here before?\n"));
    }
   /* prepare for dipir/reset to load the rom in */
#ifdef undef
    {
	uint32 oldmctl;
	oldmctl = *MCTL;
	*MCTL = oldmctl & ~(/*CLUTXEN|*/VSCTXEN);
	SoftReset();
	*MCTL = oldmctl;
    }
#endif
byebye:
    DeleteIOReq(ramior);
    CloseItem(ram);
    return (int)ret;
}
