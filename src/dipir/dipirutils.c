/* $Id: dipirutils.c,v 1.19 1994/08/31 18:23:45 markn Exp $ */
#include "types.h"
#include "inthard.h"
#include "rom.h"
#include "discdata.h"
#include "dipir.h"
#include "aif.h"

/* routines common to boot strap code */

_3DOBinHeader *
get3DOBinHeader(void)
{
	return (_3DOBinHeader *)(*(int *)(MAGIC_KERNELADR)+ sizeof(AIFHeader));
}

uint32 CurrentOsVersion(void)	/* Peek 0x10000 to get running sherry version */
{
	_3DOBinHeader *bh = get3DOBinHeader();
	return MakeInt16(bh->_3DO_Item.n_Version, bh->_3DO_Item.n_Revision);
}

uint8
Current3DOFlags(void)	/* Return sherry's _3DOBInHeader flags */
{
	_3DOBinHeader *bh = get3DOBinHeader();
	return (bh->_3DO_Flags);
}

RomTag *
FindRT(RomTag *rt, int subtype, int type)
{
	/* Find RomTag */
	while (rt->rt_SubSysType != 0)
	{
		if (rt->rt_SubSysType == subtype && rt->rt_Type == type)
			return rt;
		rt++;
	}
	return 0;
}

/*
 * Compute size of DRAM based on contents of the Madam System Control Register.
 *
 *             |  x8 == 0   |  x8 == 0  |  x8 == 1
 *  MSYS       | MSYS DRAM  | MSYS DRAM | MSYS
 *  2:0  VRAM  | 6:5  SET0  | 4:3 SET1  | 6:3  DRAM
 *  ---  ----  | ---- ----  | ---- ---- | ---- ----
 *             |  00  0 MB  |  00 0 MB  | 
 *  001  1 MB  |  01  1 MB  |  01 1 MB  | 0101 2 MB
 *  010  2 MB  |  10  4 MB  |  10 4 MB  | 1010 8 MB
 *
 * Combinations not shown in the above tables are not supported and
 * this routine is not guaranteed to return anything sensible (even
 * though it usually will do something vaguely rational).
 */
void
FindMemSize(uint32 *pDramSize, uint32 *pVramSize)
{
	uint32 bits = *MSYSBits;
	uint32 dram_size;
	uint32 vram_size;

	/*
	 * This may look complex, but it only takes about eight ARM
	 * instructions to evaluate. Basicly, take each DRAM set size
	 * field, and double it to get the left-shift count. Then
	 * shift "a quarter megabyte" left by the count, casting off
	 * anything less than a megabyte; this is easiest to do if you 
	 * shift "1" left by the count, then right by two. We turn it
	 * into megabytes later.
	 */
	dram_size =
		(((1<<((bits >> (DRAMSIZE_SET0SHIFT-1))&(DRAMSIZE_SETMASK<<1)))>>2) +
		 ((1<<((bits >> (DRAMSIZE_SET1SHIFT-1))&(DRAMSIZE_SETMASK<<1)))>>2));

	vram_size = ((bits & VRAMSIZE_MASK) >> VRAMSIZE_SHIFT);

	/* Convert to megabytes */
	*pDramSize = dram_size * 1024*1024;
	*pVramSize = vram_size * 1024*1024;
}

/*
 * Modify the MCTL register.
 */
uint32
ModMCTL(uint32 op, uint32 newmctl)
{
#define MINSPORTVCOUNT 10       /* safe place to turn on CLUT DMA */
#define MAXSPORTVCOUNT 13
	uint32 vtmp;
	uint32 oldmctl;

	oldmctl = *MCTL;
again:
	/* Wait for a safe place in the video display
	 * (between scan lines 10 and 13). */
	do {
		vtmp = *VCNT & VCNT_MASK;
	} while (vtmp < (MINSPORTVCOUNT<<VCNT_SHIFT) ||
		 vtmp >= (MAXSPORTVCOUNT<<VCNT_SHIFT));

	/* Set newmctl to the value we want to put in MCTL, based on op. */
	switch (op) 
	{
	case MCTL_OP_SET:
		break;
	case MCTL_OP_BITSET:
		newmctl = oldmctl | newmctl;
		break;
	case MCTL_OP_BITCLR:
		newmctl = oldmctl & ~newmctl;
		break;
	case MCTL_OP_NOP:
		return oldmctl;
	}
	op = MCTL_OP_SET;

	/* If we're setting VSCTXEN, do only VSCTXEN, then wait a frame. */
	if ((newmctl & VSCTXEN) && (*MCTL & VSCTXEN) == 0)
	{
		*MCTL |= VSCTXEN;
		newmctl |= VSCTXEN;
		goto delay;
	}
	/* If we're clearing CLUTXEN, do only CLUTXEN, then wait a frame. */
	if ((newmctl & CLUTXEN) == 0 && (*MCTL & CLUTXEN))
	{
		*MCTL &= ~CLUTXEN;
		newmctl &= ~CLUTXEN;
		goto delay;
	}
	*MCTL = newmctl;
	return oldmctl;
delay:
	/* Wait till we're past scan line 13. 
	 * This delays a full frame when we go to the loop at again: */
	do {
		vtmp = *VCNT & VCNT_MASK;
	} while (vtmp <= (MAXSPORTVCOUNT<<VCNT_SHIFT));
	goto again;
}
