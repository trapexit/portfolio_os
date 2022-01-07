/* $Id: dspp_touch.h,v 1.1 1994/09/08 21:05:55 phil Exp $ */
#pragma include_only_once
#ifndef _dspp_touch_h
#define _dspp_touch_h
/*
** DSP Includes for low level hardware driver
** By:  Phil Burk
*/

/*
** Copyright (C) 1994, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

#include "types.h"
#include "dspp_addresses.h"

/* This section determines the resources available in the system
** and their starting locations, etc.  Preallocation is included.
*/

/* ======================== ANVIL ==================== */
#ifdef ASIC_ANVIL
#define FIRST_N_MEM    0x000
#define LAST_N_MEM     0x1FF
#define OFFSET_N_MEM   0x000

#define FIRST_EI_MEM   0x008
#define LAST_EI_MEM    0x06F    /* 931216 was 0x70 */
#define OFFSET_EI_MEM  0x000

#define FIRST_I_MEM    0x008
#define LAST_I_MEM     0x0FF
#define OFFSET_I_MEM   0x100

#define FIRST_EO_MEM   0x004
#define LAST_EO_MEM    0x00F
#define OFFSET_EO_MEM  0x300

#define FIRST_IN_FIFO  0x000
#define LAST_IN_FIFO   0x00C
#define OFFSET_IN_FIFO 0x000

#define FIRST_OUT_FIFO 0x000
#define LAST_OUT_FIFO  0x003
#define OFFSET_OUT_FIFO 0x000
#endif

/* ------------------------ ANVIL  -------------------- */

/* ======================== BULLDOG ==================== */
#ifdef ASIC_BULLDOG
/* For now, use same table allocation scheme.
** Simply reserve some area as EI, some as I and some as EO.
** Does not take advantage of flat memory yet. %Q
*/
#define FIRST_N_MEM    0x000
#define LAST_N_MEM     0x3FF  /* 1K */
#define OFFSET_N_MEM   0x000

/*  Bulldog Memory Map, temporary
**  000-007 : Reserved EI
**  008-0FF : EI
**  100-107 : Reserved I
**  108-2EF : I 
**  2F0-2F3 : Reserved EO
**  2F4-2FF : EO
**  300-37F : FIFO_OSC
*/
#define FIRST_EI_MEM   0x008
#define LAST_EI_MEM    0x0FF  /* 248 */
#define OFFSET_EI_MEM  0x000

#define FIRST_I_MEM    0x008
#define LAST_I_MEM     0x1EF
#define OFFSET_I_MEM   0x100

#define FIRST_EO_MEM   0x004
#define LAST_EO_MEM    0x00F
#define OFFSET_EO_MEM  0x2F0

/* Allocate 8 as input FIFOs and 8 as output FIFOs %Q */
#define FIRST_IN_FIFO  0x000
#define LAST_IN_FIFO   0x017
#define OFFSET_IN_FIFO 0x000

#define FIRST_OUT_FIFO 0x000
#define LAST_OUT_FIFO  0x007
#define OFFSET_OUT_FIFO 0x018
#endif
/* ------------------------ BULLDOG -------------------- */

#define EI_IFSCALEOUTPUT 0x0000 /* These must match dspp_map.j in Forth */
#define EI_OUTPUTSCALAR  0x0001
#define EO_BENCHMARK   0x000  /* These must match dspp_map.j in Forth */
#define EO_MAXTICKS    0x001
#define EO_FRAMECOUNT  0x002


#define DSPP_SLEEP_OPCODE (0x8380)
#define DSPP_NOP_OPCODE   (0x8000)
#define DSPP_JUMP_OPCODE   (0x8400)

/* Constants *******************************************************/
#define INPUT_FIFO	(0)
#define OUTPUT_FIFO	(1)
/* This should be in clio.h %Q */
#define DSPP_GWILL	0x00000001
#define SILENCE_SIZE (32)

#define NUM_AUDIO_INPUT_DMAS (13)
#define NUM_AUDIO_OUTPUT_DMAS (4)
#define DDR0_CHANNEL (16)
#define DRD0_CHANNEL (0)

/* Macros to determine DMA Type */
#define IsDMATypeInput(chan) ((chan >= DRD0_CHANNEL) && (chan < (DRD0_CHANNEL + NUM_AUDIO_INPUT_DMAS)))
#define IsDMATypeOutput(chan) ((chan >= DDR0_CHANNEL) && (chan < (DDR0_CHANNEL + NUM_AUDIO_OUTPUT_DMAS)))

int32  dsphInitDSPP( void );
int32  dsphTermDSPP( void );
int32  dsphWriteIMem( int32 WriteAddr, int32 WriteValue );
int8  *dsphReadChannelAddress( int32 Channel );
uint32 dsphConvertChannelToInterrupt( int32 DMAChan );
void   dsphDisableDMA( int32 DMAChan );
void   dsphHalt( void );
void   dsphInitDMA( void );
void   dsphReset( void );
void   dsphResetAllFIFOs( void );
void   dsphResetFIFO( int32 DMAChan );
void   dsphSetDMANext (int32 DMAChan, int32 *NextAddr, int32 NextCnt);
void   dsphSetFullDMA (int32 DMAChan, int32 *Addr, int32 Cnt, int32 *NextAddr, int32 NextCnt );
void   dsphStart( void );
void   dsphWriteCodeMem( int32 CodeAddr, int32 Value );
void   dsphWriteEIMem( int32 DSPPAddr, int32 Value );
void   dsphDisableChannelInterrupt( int32 DMNAChan );
void   dsphEnableChannelInterrupt( int32 DMNAChan );


#endif /* _dspp_touch_h */
