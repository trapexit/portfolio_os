/* $Id: dspp_addresses.h,v 1.1 1994/09/08 21:05:55 phil Exp $ */
#pragma include_only_once
#ifndef _dspp_addresses_h
#define _dspp_addresses_h
/*
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/


#ifdef ASIC_BULLDOG
/****************************************************
** These are the DSPP addresses for memory mapped I/O
** in the DSPP data memory.
****************************************************/
// #define DSPI_START              0x1000
// #define DSPI_END                   0x1FFF
// #define DSPI_IO_LOW                0x1300
// #define DSPI_IO_HIGH               0x13FA

#define DSPI_NUM_CHANNELS          (0x0020)
#define DSPI_FIFO_OSC              (0x0300)

#define DSPI_FIFO_CURRENT_OFFSET   (0x00)              /* OFFSETS IN FIFO_OSC FOR EACH CHANNEL */
#define DSPI_FIFO_NEXT_OFFSET      (0x01)
#define DSPI_FIFO_FREQUENCY_OFFSET (0x02)
#define DSPI_FIFO_PHASE_OFFSET     (0x03)
#define DSPI_FIFO_OSC_SIZE         (0x04)

#define DSPI_FIFO_BUMP             (0x0380)

#define DSPI_FIFO_BUMP_CURR_OFFSET  (0x00)              /* OFFSETS IN CURR_BUMP FOR EACH CHANNEL */ 
#define DSPI_FIFO_BUMP_STATUS_OFFSET  (0x04)

#define DSPI_INPUT0                (0x03C0)
#define DSPI_INPUT1                (0x03C1)
#define DSPI_OUTPUT0               (0x03C8)
#define DSPI_OUTPUT1               (0x03C9)
#define DSPI_OUTPUT2               (0x03CA)
#define DSPI_OUTPUT3               (0x03CB)
#define DSPI_OUTPUT4               (0x03CC)
#define DSPI_OUTPUT5               (0x03CD)
#define DSPI_OUTPUT6               (0x03CE)
#define DSPI_OUTPUT7               (0x03CF)
#define DSPI_INPUT_CONTROL         (0x03D0)
#define DSPI_OUTPUT_CONTROL        (0x03D1)
#define DSPI_INPUT_STATUS          (0x03D2)
#define DSPI_OUTPUT_STATUS         (0x03D3)
#define DSPI_CPU_INT0              (0x03F0)
#define DSPI_CPU_INT1              (0x03F1)
#define DSPI_AUDLOCK               (0x03F4)
#define DSPI_CLOCK                 (0x03F8)
#define DSPI_NOISE                 (0x03F9)
#define DSPI_PC                    (0x03FA)
#endif

/****************************************************
** Addresses for DSPP registers and memory spaces.
****************************************************/
#ifdef ASIC_BULLDOG
/* %Q Fix DSPP_BASE */
#define DSPP_BASE                  (0x00050000)
#define DSPX_CODE_MEMORY           (DSPP_BASE + 0x0000)
#define DSPX_CODE_MEMORY_LOW       ((vuint32 *) (DSPP_BASE + 0x0000))
#define DSPX_CODE_MEMORY           ((vuint32 *) (DSPP_BASE + 0x0000))
#define DSPX_CODE_MEMORY_LOW       ((vuint32 *) (DSPP_BASE + 0x0000))
#define DSPX_CODE_MEMORY_HIGH      ((vuint32 *) (DSPP_BASE + 0x0FFF))
#define DSPX_DATA_MEMORY           ((vuint32 *) (DSPP_BASE + 0x1000))
#define DSPX_DATA_MEMORY_LOW       ((vuint32 *) (DSPP_BASE + 0x1000))
#define DSPX_DATA_MEMORY_HIGH      ((vuint32 *) (DSPP_BASE + 0x1FFF))
#define DSPX_INTERRUPT_SET         ((vuint32 *) (DSPP_BASE + 0x4000))
#define DSPX_INTERRUPT_CLR         ((vuint32 *) (DSPP_BASE + 0x4004))
#define DSPX_INTERRUPT_ENABLE      ((vuint32 *) (DSPP_BASE + 0x4008))
#define DSPX_INTERRUPT_DISABLE     ((vuint32 *) (DSPP_BASE + 0x400C))
	#define DSPXB_INT_SOFT0        (0x0400)
	#define DSPXB_INT_SOFT1        (0x0200)
	#define DSPXB_INT_TIMER        (0x0100)
	#define DSPXB_INT_INPUT_UNDER  (0x0080)
	#define DSPXB_INT_INPUT_OVER   (0x0040)
	#define DSPXB_INT_OUTPUT_UNDER (0x0020)
	#define DSPXB_INT_OUTPUT_OVER  (0x0010)
	#define DSPXB_INT_UNDEROVER    (0x0008)
	#define DSPXB_INT_DMALATE      (0x0004)
	#define DSPXB_INT_CONSUMED     (0x0002)
	#define DSPXB_INT_DMANEXT      (0x0001)
	#define DSPXB_INT_ALL_DMA      (DSPXB_INT_DMANEXT   |
	                                DSPXB_INT_CONSUMED  |
	                                DSPXB_INT_DMALATE   |
	                                DSPXB_INT_UNDEROVER )
	                                
#define DSPX_INT_DMANEXT_SET       ((vuint32 *) (DSPP_BASE + 0x4010))
#define DSPX_INT_DMANEXT_CLR       ((vuint32 *) (DSPP_BASE + 0x4014))
#define DSPX_INT_CONSUMED_SET      ((vuint32 *) (DSPP_BASE + 0x4020))
#define DSPX_INT_CONSUMED_CLR      ((vuint32 *) (DSPP_BASE + 0x4024))
#define DSPX_INT_CONSUMED_ENABLE   ((vuint32 *) (DSPP_BASE + 0x4028))
#define DSPX_INT_CONSUMED_DISABLE  ((vuint32 *) (DSPP_BASE + 0x402C))
#define DSPX_INT_DMALATE_SET       ((vuint32 *) (DSPP_BASE + 0x4030))
#define DSPX_INT_DMALATE_CLR       ((vuint32 *) (DSPP_BASE + 0x4034))
#define DSPX_INT_DMALATE_ENABLE    ((vuint32 *) (DSPP_BASE + 0x4038))
#define DSPX_INT_DMALATE_DISABLE   ((vuint32 *) (DSPP_BASE + 0x403C))
#define DSPX_INT_UNDEROVER_SET     ((vuint32 *) (DSPP_BASE + 0x4040))
#define DSPX_INT_UNDEROVER_CLR     ((vuint32 *) (DSPP_BASE + 0x4044))
#define DSPX_INT_UNDEROVER_ENABLE  ((vuint32 *) (DSPP_BASE + 0x4048))
#define DSPX_INT_UNDEROVER_DISABLE ((vuint32 *) (DSPP_BASE + 0x404C))
#define DSPX_DMA_STACK_ADDRESS     ((vuint32 *) (DSPP_BASE + 0x5000))

#define  DSPX_DMA_STACK        ((vuint32 *) (DSPP_BASE + 0x5000)
/* STACK OFFSETS FOR EACH CHANNEL */
#define DSPX_DMA_STACK_CHANNEL_SIZE  (0x10)
/* WORD OFFSETS IN STACK FOR EACH CHANNEL */
#define DSPX_DMA_ADDRESS_OFFSET      (0x00)              
#define DSPX_DMA_COUNT_OFFSET        (0x01)
#define DSPX_DMA_NEXT_ADDRESS_OFFSET (0x02)
#define DSPX_DMA_NEXT_COUNT_OFFSET   (0x03)

#define DSPX_DMA_STACK_CONTROL     ((vuint32 *) (DSPP_BASE + 0x5200))
	#define DSPXB_DMA_NEXTVALID    (0x0001)
	#define DSPXB_INT_DMANEXT_EN   (0x0002)
	#define DSPXB_DMA_GO_FOREVER   (0x0004)
#define DSPX_CHANNEL_ENABLE        ((vuint32 *) (DSPP_BASE + 0x6000))
#define DSPX_CHANNEL_DISABLE       ((vuint32 *) (DSPP_BASE + 0x6004))
#define DSPX_CHANNEL_DIRECTION_SET ((vuint32 *) (DSPP_BASE + 0x6008))
#define DSPX_CHANNEL_DIRECTION_CLR ((vuint32 *) (DSPP_BASE + 0x600C))
#define DSPX_CHANNEL_8BIT_SET      ((vuint32 *) (DSPP_BASE + 0x6010))
#define DSPX_CHANNEL_8BIT_CLR      ((vuint32 *) (DSPP_BASE + 0x6014))
#define DSPX_CHANNEL_SQXD_SET      ((vuint32 *) (DSPP_BASE + 0x6018))
#define DSPX_CHANNEL_SQXD_CLR      ((vuint32 *) (DSPP_BASE + 0x601C))
#define DSPX_CHANNEL_RESET         ((vuint32 *) (DSPP_BASE + 0x6030))
#define DSPX_CHANNEL_STATUS        ((vuint32 *) (DSPP_BASE + 0x603C))
#define DSPX_AUDIO_DURATION        ((vuint32 *) (DSPP_BASE + 0x6040))
#define DSPX_AUDIO_TIME            ((vuint32 *) (DSPP_BASE + 0x6044))
#define DSPX_WAKEUP_TIME           ((vuint32 *) (DSPP_BASE + 0x6048))
#define ADC_CONFIG                 ((vuint32 *) (DSPP_BASE + 0x6050))
#define DAC_CONFIG                 ((vuint32 *) (DSPP_BASE + 0x6060))
#define DSPX_CONTROL               ((vuint32 *) (DSPP_BASE + 0x6080))
	#define DSPXB_GWILLING         (0x0001)
	#define DSPXB_STEP_CYCLE       (0x0002)
	#define DSPXB_STEP_PC          (0x0004)
	#define DSPXB_SNOOP            (0x0008)
#define DSPX_RESET                 ((vuint32 *) (DSPP_BASE + 0x6084))
#define DSPX_RBASE0                ((vuint32 *) (DSPP_BASE + 0x6090))
#define DSPX_RBASE4                ((vuint32 *) (DSPP_BASE + 0x6094))
#define DSPX_RBASE8                ((vuint32 *) (DSPP_BASE + 0x6098))
#define DSPX_RBASE12               ((vuint32 *) (DSPP_BASE + 0x609C))
#define DSPX_NR                    ((vuint32 *) (DSPP_BASE + 0x60A0))
#define DSPX_ACCUME                ((vuint32 *) (DSPP_BASE + 0x60A4))
#define DSPX_CODEBUS_RDADDR        ((vuint32 *) (DSPP_BASE + 0x60A8))
#define DSPX_CODEBUS_RDDATA        ((vuint32 *) (DSPP_BASE + 0x60AC))
#define DSPX_DATABUS_RDADDR        ((vuint32 *) (DSPP_BASE + 0x60B0))
#define DSPX_DATABUS_RDDATA        ((vuint32 *) (DSPP_BASE + 0x60B4))
#define DSPX_DATABUS_WRADDR        ((vuint32 *) (DSPP_BASE + 0x60B8))
#define DSPX_DATABUS_WRDATA        ((vuint32 *) (DSPP_BASE + 0x60BC))
#define DSPX_BUS_SIGNALS           ((vuint32 *) (DSPP_BASE + 0x60C0))
#endif

/* These defines use the Bulldog names but map them to Anvil addresses. */
#ifdef ASIC_ANVIL
#define DSPP_FIFOHEAD_ZERO_ADDRESS (0x0070)
#define DSPI_INPUT0                (0x00E8)
#define DSPI_INPUT1                (0x00E9) 
#define DSPI_NOISE                 (0x00EA)
#define DSPX_CODE_MEMORY           DSPPN16
#define DSPX_DMA_STACK             RAMtoDSPP0
#endif


#endif /* _dspp_addresses_h */
