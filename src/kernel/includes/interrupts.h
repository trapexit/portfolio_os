#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

/*****

$Id: interrupts.h,v 1.17 1994/11/01 19:02:51 bungee Exp $

$Log: interrupts.h,v $
 * Revision 1.17  1994/11/01  19:02:51  bungee
 * Added LCCD interrupt bits, INT_CD0, INT_CD1, and INT_CDOVRFLW.  Also
 * added DSPP placeholder bits, as they were not previously defined.
 *
 * Revision 1.16  1994/09/01  22:10:29  vertex
 * Added "const" to pertinent (char *) parameters
 *
 * Revision 1.15  1994/01/28  23:20:15  sdas
 * Include Files Self-consistency - includes dependant include files
 *
 * Revision 1.14  1994/01/21  02:13:33  limes
 * recover from rcs bobble:
 *
 * +  * Revision 1.15  1994/01/20  01:58:12  sdas
 * +  * C++ compatibility - updated
 * +  *
 * +  * Revision 1.14  1994/01/18  02:37:03  dale
 * +  * Corrected Copyright
 * +  * added #pragma include_only_once
 * +  * changed _<filename>_H to __<filename>_H
 * +  * misc cleanup
 *
 * Revision 1.15  1994/01/20  01:58:12  sdas
 * C++ compatibility - updated
 *
 * Revision 1.14  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.13  1993/04/22  19:27:15  dale
 * correct typo for INT_DMAIEXTN
 *
 * Revision 1.12  1993/03/25  02:58:36  dale
 * removed ";" from end of some macros.
 *
 * Revision 1.11  1993/03/16  06:51:36  dale
 * api
 *
 * Revision 1.10  1993/03/14  02:01:10  dale
 * CREATEFIRQ-> CREATEFIRG_TAG
 *
 * Revision 1.9  1993/02/17  04:43:21  dale
 * added simple library routines for creating Items
 *
 * Revision 1.8  1993/02/10  08:46:43  dale
 * massive changes to TAG architecture
 *
 * Revision 1.7  1992/12/10  09:29:55  dale
 * New master interrupt control routine. FirqInterruptControl
 *
 * Revision 1.6  1992/10/24  01:37:21  dale
 * fix id
 *

 *****/

/*
    Copyright (C) 1993, The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#pragma force_top_level
#pragma include_only_once

/* Interrrupt Node */

#include "types.h"
#include "item.h"
#include "nodes.h"

typedef struct FirqNode
{
	ItemNode firq;
	int32 (*firq_F)(struct FirqNode *firqP);	/* ptr to routine to call */
	int32	firq_Data;	/* local data for handler */
	int32	firq_Num;	/* which firq int we handle */
} FirqNode, *FirqNodeP;

enum firq_tags
{
	CREATEFIRQ_TAG_DATA = TAG_ITEM_LAST+1,
	CREATEFIRQ_TAG_CODE,
	CREATEFIRQ_TAG_FLAGS,
	CREATEFIRQ_TAG_NUM		/* which interrupt slot */
};

#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */

extern Item CreateFIRQ(const char *name, uint8 pri, int32 (*code)(), int32 num);

#ifdef  __cplusplus
}
#endif  /* __cplusplus */

#define DeleteFIRQ(x)	DeleteItem(x)

/* sources in interrupt word 0 */

#define INT_V0	0		/* UGO HBLANK */
#define INT_V1	1		/* UGO VBLANK */
#define INT_EXPANSION	2
#define INT_TIM15	3
#define INT_TIM13	4
#define INT_TIM11	5
#define INT_TIM9	6
#define INT_TIM7	7
#define INT_TIM5	8
#define INT_TIM3	9
#define INT_TIM1	10
#define INT_DSPP	11
#define INT_DSPPRAM0	12
#define INT_DSPPRAM1	13
#define INT_DSPPRAM2	14
#define INT_DSPPRAM3	15
#define INT_DRD0	16
#define INT_DRD1	17
#define INT_DRD2	18
#define INT_DRD3	19
#define INT_DRD4	20
#define INT_DRD5	21
#define INT_DRD6	22
#define INT_DRD7	23
#define INT_DRD8	24
#define INT_DRD9	25
#define INT_DRD10	26
#define INT_DRD11	27
#define INT_DRD12	28
#define INT_DMA_EXP	29
#define INT_Spare	30
#define INT_SecondPri	31

/* interrupt sources in second word */
/* subtract 32 to get the bit # */

#define INT_DPLY	32	/* Player Bus */
#define INT_DIPIR	33	/* Disk inserted */
#define INT_PD		34	/* Slow Bus */
#define INT_RAMDSPPN	35
#define INT_UNCLEOUT	36
#define INT_UNCLEIN	37
#define INT_DMAEXTOUT	38
#define INT_DMAEXTIN	39
#define INT_BADBITS	40

#define INT_DSPPUNDRFLW	41
#define INT_DSPPOVRFLW	42
#define INT_CD0		43
#define INT_CD1		44
#define INT_CDOVRFLW	45

#define INT_MAX		46


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */

extern uint32 FirqInterruptControl(int32 intnum,int32 cmd);

#ifdef  __cplusplus
}
#endif  /* __cplusplus */

/* control words for FirqInterruptControl */
#define FIRQ_ENABLE (int32)0
#define FIRQ_DISABLE (int32)1
#define FIRQ_CLEAR (int32)2
#define FIRQ_SET (int32)3

#define EnableInterrupt(n) FirqInterruptControl(n,FIRQ_ENABLE)
#define DisableInterrupt(n) FirqInterruptControl(n,FIRQ_DISABLE)
#define ClearInterrupt(n) FirqInterruptControl(n,FIRQ_CLEAR)
#define SetInterrupt(n) FirqInterruptControl(n,FIRQ_SET)

/* bad bits */
#define BAD_UNDERFLOW_MASK	0x3FFF
#define BAD_OVERFLOW_SHIFT	14
#define BAD_OVERFLOW_MASK	(0xF<<BAD_OVERFLOW_SHIFT)
#define BAD_DMA_Priv		20
#define BAD_DMA_NFW		21
#define BAD_DMA_NFW_Addr	(0x3f<<(BAD_DMA_NFW_Addr+1))

#endif /* __INTERRUPTS_H */
