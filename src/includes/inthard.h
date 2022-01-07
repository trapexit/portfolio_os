/* $Id: inthard.h,v 1.13 1994/05/04 03:02:46 deborah Exp $ */
/* *************************************************************************
 * 
 * Opera Internal Hardware Definitions Include File
 * 
 * IMPORTANT NOTE: There is also an inthard.i for assembly includes.
 *
 * Copyright (C) 1992, New Technologies Group, Inc.
 * Confidential and Proprietary  -  All Rights Reserved
 * 
 * This file works with any tab space from 1 to 8.
 * 
 * HISTORY
 * Date   Author           Description
 * ------ ---------------- -------------------------------------------------
 * 930302 -RJ              Added MADAM_GREENWW definition
 * 930209 -RJ              Created a whole bunch of simplifying RMOD_ 
 *                         and WMOD_ definitions
 * 921206 -RJ              Major edit to bring up to current spec
 * 921204 -RJ              Burst this file out of sherrie.h, cut the public
 *                         part of sherrie.h into hardware.h
 * 921104 -RJ              Fixed definition of DRAMSETZ_4MEG
 * 921010 -RJ              Started name-busting this file, merged in 
 *                         definitions from example.c
 * 920724 -RJ Mical        Start overhaul
 * 920717 Stephen Landrum  Last edits before July handoff
 * 
 * ********************************************************************** */

#ifndef __INTHARD_H
#define __INTHARD_H

#include "hardware.h"



/* ===  ===================  ============================================ */
/* ===                       ============================================ */
/* ===  ARM CPU Definitions  ============================================ */
/* ===                       ============================================ */
/* ===  ===================  ============================================ */

/* === 26-bit ARM processor modes === */
#define ARM_MODE_USR26  0x00000000
#define ARM_MODE_FIQ26  0x00000001
#define ARM_MODE_IRQ26  0x00000002
#define ARM_MODE_SVC26  0x00000003


/* === 32-bit ARM processor modes === */
#define ARM_MODE_USR  0x00000010
#define ARM_MODE_FIQ  0x00000011
#define ARM_MODE_IRQ  0x00000012
#define ARM_MODE_SVC  0x00000013
#define ARM_MODE_ABT  0x00000017
#define ARM_MODE_UND  0x0000001B


/* === 26-bit PSR bit locations === */
#define ARM_MODE_MASK26  0x00000003
#define ARM_F_BIT26      0x04000000
#define ARM_I_BIT26      0x08000000
#define ARM_V_BIT26      0x10000000
#define ARM_C_BIT26      0x20000000
#define ARM_Z_BIT26      0x40000000
#define ARM_N_BIT26      0x80000000


/* === 32-bit PSR bit locations === */
#define ARM_MODE_MASK  0x0000001F
#define ARM_F_BIT      0x00000040
#define ARM_I_BIT      0x00000080
#define ARM_V_BIT      0x10000000
#define ARM_C_BIT      0x20000000
#define ARM_Z_BIT      0x40000000
#define ARM_N_BIT      0x80000000


/* === ARM Exception vectors === */
#define ARM_RSTV     0x00000000
#define ARM_UNDV     0x00000004
#define ARM_SWIV     0x00000008
#define ARM_PREABTV  0x0000000C
#define ARM_DATABTV  0x00000010
#define ARM_ADRV     0x00000014
#define ARM_IRQV     0x00000018
#define ARM_FIQV     0x0000001C


/* ===  ==========================  ===================================== */
/* ===                              ===================================== */
/* ===  Addresses of memory ranges  ===================================== */
/* ===                              ===================================== */
/* ===  ==========================  ===================================== */

#define ROM          ((vuint32 *)0x03000000)
#define SLOWBUS      ((vuint32 *)0x03100000)
#define SPORT        ((vuint32 *)0x03200000)
#define MADAM        ((vuint32 *)0x03300000)
#define CLIO         ((vuint32 *)0x03400000)
#define Other1       ((vuint32 *)0x03404000)
#define Other2       ((vuint32 *)0x03408000)
#define UNCLE        ((vuint32 *)0x0340C000)
#define TRACE        ((vuint32 *)0x03600000)
#define TRACEBIGRAM  ((vuint32 *)0x03800000)



/* ===  ===============  ================================================ */
/* ===                   ================================================ */
/* ===  MADAM addresses  ================================================ */
/* ===                   ================================================ */
/* ===  ===============  ================================================ */

/* The #defines that used to be here have been moved to madam.h. */
#include "madam.h"

/* ===  ==============  ================================================= */
/* ===                  ================================================= */
/* ===  CLIO Registers  ================================================= */
/* ===                  ================================================= */
/* ===  ==============  ================================================= */

/* The #defines that used to be here have been moved to clio.h. */
#include "clio.h"


/* ===  ===============  ================================================ */
/* ===                   ================================================ */
/* ===  Trace Registers  ================================================ */
/* ===                   ================================================ */
/* ===  ===============  ================================================ */

#define SRAM       ((vuint32 *)((ulong)TRACE+0x100000))
#define LINK_DATA  ((vuint32 *)((ulong)TRACE+0x1FFF00))
#define LINK_ADDR  ((vuint32 *)((ulong)TRACE+0x1FFF04))
#define LINK_FIFO  ((vuint32 *)((ulong)TRACE+0x1FFF08))
#define JOYSTICKS  ((vuint32 *)((ulong)TRACE+0x1FFF0C))



/* ===  =====================================  ========================== */
/* ===                                         ========================== */
/* ===  Bit definitions for various registers  ========================== */
/* ===                                         ========================== */
/* ===  =====================================  ========================== */

#define HardID_MASK  0xFF000000
#define MADAM_ID     0x01000000
#define CLIO_ID      0x02000000
#define UNCLE_ID     0x03000000

#define HardID_SHIFT  24

/* for all software rev. registers */
#define SoftRev_MASK    0xFF000000

#define SoftRev_SHIFT  24

/* Most other bit defines have been moved to clio.h or madam.h */

#endif /* of #ifdef __INTHARD_H */

