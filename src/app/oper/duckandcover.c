/*	$Id: duckandcover.c,v 1.14 1994/12/23 01:25:45 markn Exp $
**
**	Emergency shutdown function(s) for just before Dipir hits.
**
**	Copyright 1993 by The 3DO Company Inc.
*/
#include "types.h"
#include "inthard.h"
#include "clio.h"

extern void ModMCTL(uint32 newmctl);

#define MINSPORTVCOUNT 10       /* safe place to turn off CLUT DMA */
#define MAXSPORTVCOUNT 13

extern uint32	Disable(void);
extern void	Enable(uint32);

extern int32	NotifyXBusDipirStart(void);
extern int32	NotifyXBusDipirEnd(void);


static uint32 oldmctl, olddmareqen;
static uint32 oldints;
static uint32 vtmp;

void
Duck(void)
{
	NotifyXBusDipirStart();

    /* We will be getting a dipir */

    /* Disable video DMA to avoid ROM aborts */
    oldmctl = *MCTL;
    olddmareqen = *DMAREQEN;

    /* disable DMA type things when its safe to do so, by waiting */
    /* for line 10, which graphics thinks is safe for sport transfers */
    /* if this is after line 13, spin too */

    ModMCTL(oldmctl & ~(VSCTXEN));

    while (( (*VCNT&VCNT_MASK)) <= (MAXSPORTVCOUNT<<VCNT_SHIFT)); /* wait for it, wait for it... */

    ModMCTL(oldmctl & ~(VSCTXEN|PLAYXEN|SLIPXEN|CLUTXEN));

    *DMAREQDIS = 0x7FFFFFFF;	/* Disable CLIO DMA */
}


void
MissedMe(void)
{
    *DMAREQEN = olddmareqen;	/* enable the CLIO DMA */

    ModMCTL(oldmctl & ~VSCTXEN);	/* everything on except video */

    while (( (*VCNT&VCNT_MASK)) <= (MAXSPORTVCOUNT<<VCNT_SHIFT)); /* wait for it, wait for it...     */

    ModMCTL(oldmctl);		/* and now video */

	NotifyXBusDipirEnd();
}

void ModMCTL(uint32 newmctl)
{

again:
    while ( ((vtmp=(*VCNT&VCNT_MASK)) < (MINSPORTVCOUNT<<VCNT_SHIFT)) ||
	(vtmp >= (MAXSPORTVCOUNT<<VCNT_SHIFT)) );

    oldints = Disable();
    if(((vtmp=(*VCNT&VCNT_MASK)) < (MINSPORTVCOUNT<<VCNT_SHIFT)) ||
    (vtmp >= (MAXSPORTVCOUNT<<VCNT_SHIFT))) {
	Enable(oldints);
	goto again;
    }
    *MCTL = newmctl;
    Enable(oldints);
}
