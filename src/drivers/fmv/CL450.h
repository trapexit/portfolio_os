/*
	File:		CL450.h

	Contains:	xxx put contents here xxx

	Written by:	xxx put writers here xxx

	Copyright:	© 1993 by 3DO, Inc., all rights reserved.

	Change History (most recent first):

		<13>	 7/12/94	GM		Changed prototype for FMVDo450Command to match function with
									unused arguments removed.
		<12>	  5/3/94	GDW		Removed some defines and prototypes.
		<11>	  4/8/94	GM		Added MAXFRAMSKIP #define.
		<10>	 3/17/94	GM		Added definition for new PTS interrupt bit.
		 <9>	 2/28/94	GM		Added casts to #defines to avoid compiler warnings.
		 <8>	 2/25/94	GM		Added #defines to support I frame search mode.
		 <7>	 2/14/94	GM		Updated #defines to be consistent with latest CL450
									specification. Added prototypes for bitstream flushing commands.
		 <6>	 2/10/94	GM		Changed public routine name prefix to FMVVidDec from FMV450 to
									assist in development of Thomson version driver.
		 <5>	  2/7/94	GM		Added prototype for ReadSCR function. This is only used with
									diagnostic microcode and should be removed in future versions.
		 <4>	  2/1/94	GM		Updated interrupt mask definitions to reflect latest CL450
									specifications. Modified interrupt routine prototypes to match
									changes in CL450.c
		 <3>	 1/14/94	GM		Added prototypes for PTS support
		 <2>	11/30/93	GM		Changed prototypes for FMV450RunBootCode and FMV450RunCode to
									support download of microcode from Opera filesystem.

*/

/* file: CL450.h */
/* CL450 video decoder definitions */
/* 3/25/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#ifndef CL450_HEADER
#define CL450_HEADER

/* CL450 Internal Registers (From Page 8-3 in the CL450 manual) */

#define CMEM_CONTROL	0x80L	/* Internal reset, byte swap, diagnostics */
#define CMEM_DATA		0x02L	/* FIFO read port (diagnostics only) */
#define CMEM_DMACTRL	0x84L	/* FIFO status, CFLEVEL assertion, DMA enable */
#define CMEM_STATUS		0x82L	/* FIFO counters (diagnostics only) */
#define CPU_CONTROL		0x20L	/* Run enable bit */
#define CPU_IADDR		0x3eL	/* IMEM write address (initialization only) */
#define CPU_IMEM		0x42L	/* IMEM write data (initialization only) */
#define CPU_INT			0x54L	/* Internal interrupt status (diagnostics only) */
#define CPU_INTENB		0x26L	/* Internal interrupt enables (diagnostics only) */
#define CPU_PC			0x22L	/* Internal interrupt enable, program counter */
#define CPU_TADDR		0x38L	/* TMEM address (diagnostics only) */
#define CPU_TMEM		0x46L	/* TMEM data (diagnostics only) */
#define DRAM_REFCNT		0xacL	/* Refresh clock count */
#define HOST_CONTROL	0x90L	/* Interrupt mode setting */
#define HOST_INTVECR	0x9cL	/* Read interrupt vector and IPID */
#define HOST_INTVECW	0x98L	/* Write interrupt vector and IPID */
#define HOST_RADDR		0x88L	/* Pointer to command/status register */
#define HOST_RDATA		0x8cL	/* Data port to command/status register */
#define HOST_NEWCMD		0x56L	/* New command bit */
#define HOST_SCR0		0x92L	/* SCR - lower portion */
#define HOST_SCR1		0x94L	/* SCR Ð middle portion */
#define HOST_SCR2		0x96L	/* SCR - upper portion */
#define VID_CONTROL		0xecL	/* Pointer to indirect video register */
#define VID_REGDATA		0xeeL	/* Data port for indirect video registers */
#define VID_CHROMA		0x0aL	/* Chrominance data port (diagnostics only) */
#define VID_Y			0x00L	/* Luminance data port (diagnostics only) */

/* CL450 Indirect Video Registers */

#define VID_SELACTIVE	0x8L	/* Width of active region (diagnostics only) */
#define VID_SELAUX		0xcL	/* Pixel data high byte */
#define VID_SELGB		0xbL	/* Border green, blue values (diagnostics only) */
#define VID_SELR		0xaL	/* Border red value (diagnostics only) */
#define VID_SELA		0x0L	/* Conversion coefficients (diagnostics only) */
#define VID_SELB		0x1L	/* Conversion coefficients (diagnostics only) */
#define VID_SELBOR		0x9L	/* Left border size (diagnostics only) */
#define VID_SELMODE		0x7L	/* RGB or YCbCr mode (diagnostics only) */

/* CL450 command / status registers (indirect) */

#define CSTAT_CURCMDID	0x08L	/* current command id */
#define CSTAT_CMDSTAT	0x09L	/* command status */
#define CSTAT_INTSTAT	0x0AL	/* interrupt status */
#define CSTAT_BUFLEVEL	0x0BL	/* bitstream buffer level */

/* CMEM_control register definitions */

#define CMCTRL_RESET	0x41L	/* Internal reset */
#define CMCTRL_START	0x02L	/* Reset reset bits*/
#define CMCTRL_SWAP		0x20L	/* Byte swap coded fifo data */

/* CMEM_dmactrl register definitions */

#define CMDMA_STATUSMSK	0x1e0L	/* status bits mask */
#define CMDMA_0Q		0x000L	/* 0-3 empty entries in coded data fifo */
#define CMDMA_1Q		0x100L	/* 4-7 empty entries in coded data fifo */
#define CMDMA_2Q		0x080L	/* 8-11 empty entries in coded data fifo */
#define CMDMA_3Q		0x040L	/* 12-15 empty entries in coded data fifo */
#define CMDMA_4Q		0x020L	/* 16 empty entries in coded data fifo */
#define CMDMA_CNTRLMASK	0x01eL	/* control bits mask */
#define CMDMA_1QE		0x010L	/* CFLEVEL asserted when 4-7 entries empty */
#define CMDMA_2QE		0x008L	/* CFLEVEL asserted when 8-11 entries empty */
#define CMDMA_3QE		0x004L	/* CFLEVEL asserted when 12-15 entries empty */
#define CMDMA_4QE		0x002L	/* CFLEVEL asserted when 16 entries empty */
#define CMDMA_DISABLE	0x000L	/* Disable CFLEVEL assertion */
#define CMDMA_ENABLE	0x001L	/* Enable DMA */

/* HOST_control register definitions */

#define HCTRL_AIC		0x8000L	/* auto interrupt clear */
#define HCTRL_VIE		0x4000L	/* vectored interrupt enable */
#define HCTRL_NOTINT	0x0080L	/* 0 if interrupt pending */

/* HOST_intvec[rw] registers definitions */

#define HINTV_IPID_MASK	0x0700L	/* interrupt priority ID mask */
#define HINTV_IVEC_MASK	0x00ffL	/* interrupt vector mask */

/* HOST_newcmd regster definitions */

#define H_NEWCOMMAND	0x01L	/* execute command */

/* System clock reference registers */

#define SCR2_GLCKSELECT	0x1000L	/* select global clock as reference */
#define SCR2_SCLKSELECT	0x0000L	/* select system clock as reference */
#define SCR2_DIVISORMSK	0x0ff8L	/* system clock divisor mask */
#define SCR2_SYSCLKHMSK	0x0003L	/* system clock hi 2 bits mask */
#define SCR1_SYSCLKMMSK	0x7fffL	/* system clock middle 15 bits mask */
#define SCR0_SYSCLKLMSK	0x7fffL	/* system clock low 15 bits mask */

/* CPU_control register definitions */

#define CPU_RUNDISABLE	0x00L	/* stop */
#define CPU_RUNENABLE	0x01L	/* go! */

/* CPU_pc register definitions */

#define CPU_INTDISABLE	0x00L	/* interrupt disable */
#define CPU_INTENABLE	0x0200L	/* interrupt enable */

/* DRAM_REFCNT register definitions */

#define DRAM_REFCNTMSK	0x0fffL	/* dram refresh clock count */

/* indirect video register definitions */

#define VID_VRIDMSK		0x1eL	/* video register id mask */
#define VID_MODEYCRCB	0x00L	/* YCrCb mode */
#define VID_MODERGB		0x01L	/* RGB mode */

/* interrupt register definitions */

/* CL450 commands */

/* boot microcode commands */
#define BOOT_WRITEDRAM	0x00L	/* write dram block when running boot code */
#define BOOT_READDRAM	0x01L	/* read dram block when running boot code */

/* standard CL450 video commands */
// #define MPGV_ABORT		0x8000L	/* abort execution */
#define MPGV_RESET		0x8000L	// reset
#define MPGV_INQBUFFER	0x8001L	/* inquire buffer fullness */
#define MPGV_SETTHRESH	0x0103L	/* set threshold for ready-for-data interrupt */
#define MPGV_SETINTMASK	0x0104L	/* set intterupt mask */
#define MPGV_SETVIDRATE	0x0105L	/* set video rate 50/60 Hz */
#define MPGV_SETWINDOW	0x0406L	/* set window into decoded pictures */
#define MPGV_SETBORDER	0x0407L	/* set border size and color */
#define MPGV_NEWPACKET	0x0408L	/* synchronize video packets */
#define MPGV_PLAY		0x000DL	/* play video */
#define MPGV_SETCLRMODE	0x0111L	/* set color mode rgb / ycrcb */

/* extended CL450i video commands */
#define MPGV_PAUSE		0x000eL	/* pause */
// #define MPGV_SEARCH		0x010cL	/* advances to specified startcode in rate buffer */
#define MPGV_BLANK		0x010fL	/* display solid color, continue decoding */
#define MPGV_SINGLESTEP	0x000bL	/* frame advance */
#define MPGV_SCAN		0x000aL	/* skip to next I frame */
#define MPGV_SLOWMOTION	0x0109L	/* play at 1 / n times normal speed */
#define MPGV_CODEDSTILL	0x000cL	/* display coded still (2 distinct fields) */
// #define MPGV_PRFLUSHBS	0x8002L	/* high priority flush bitstream */
#define MPGV_FLUSHBS	0x8102L /* flush bitstream */
// #define MPGV_FASTFWD	0x0010L	/* fast forward */

/* misc #defines */

#define IMEM_SIZE			0x200L	/* 512 x 32 */
#define TMEM_SIZE			0x40L	/* 64 x 16 */
#define CACHE_START			0x5cL
#define CACHE_END			0x200L
#define MAXFRAMESKIP		32767L

#define MPEGSYSTEMCLOCKFREQUENCY	90000L	/* 90 Khz system clock */
#define DRAM_RFRSHCNT	300L	/* refresh clock count */

/* 3DO stuff */

#define FMVVIDDECMODEPLAY		((unsigned) 1L)
#define FMVVIDDECMODEKEYFRAMES	((unsigned) 2L)

#define CL450BORDERLEFT		50L	/* left border pixels */
#define CL450BORDERTOP16	20L	/* top border lines */
#define CL450BORDERTOP24	30L		/* top border lines */
//#define CL450BORDERTOP24	10L		/* top border lines */
#define CL450RBORDERCOLOR	0x00L	/* red component of border color */
#define CL450GBORDERCOLOR   0x00L	/* green component of border color */
#define CL450BBORDERCOLOR   0x00L	/* blue component of border color */

#define CL450XOFFSET		0x00L	/* window x offset */
#define CL450YOFFSET		0x00L	/* window y offset */
//#define CL450WINDOWWIDTH	704L	/* window width */
#define CL450WINDOWWIDTH	640L	/* window width */
#define CL450WINDOWHEIGHT	240L	/* window height */

/* CL450.c function prototypes */
#include "FMVErrors.h"
#include "FMVROM.h"

void		FMVReset450( void );
int32		FMVDo450Command( uint32 arg0, uint32 arg1, uint32 arg2, uint32 arg3, uint32 arg4 );
int32		FMVWrite450IMEM( uint32 startAddress, uint32 length, uint32 *data );
FMVError	FMV450RunBootCode( uint32 *bootCode );
FMVError	FMV450RunCode( uint32 *code );
FMVError	FMV450RunMPEG16BitCode( void );
FMVError	FMV450RunMPEG24BitCode( void );
void		FMV450SetSystemClockDivisor( uint32 clockFrequency );
uint32		FMV450ReadSystemClockReference( void );
void		FMV450WriteSystemClockReference( uint32 currentSCR );
void		FMV450WriteIndirectVideoRegister( uint32 vreg, uint32 value );
uint32		FMV450ReadIndirectVideoRegister( uint32 vreg );
void		FMV450SetRGBConversionCoefficients( uint32 k0, uint32 k1, uint32 k2, uint32 k3 );
int32		FMV450SetThreshold( int32 threshold );
int32		FMV450SetInterruptMask( int32 mask );
int32		FMV450SetBorderRGB( uint32 left, uint32 top, uint32 r, uint32 g, uint32 b );
int32 		FMVVidDecFlushBitstream( void );
int32		FMV450SetRGBColorMode( void );
int32		FMV450SetYCrCbColorMode( void );
uint32		FMV450GetStatusRegister( uint32 statusRegister );
void		FMV450ClearInterruptStatus( void );
void		FMV450EnableVSInterrupt( void );
int32		FMV450CheckVSInterrupt( void );
void		FMV450ClearVSInterrupt( void );

/* CL450Diags.c function prototypes */
void CL450Status( void );
void CL450Diags( void );
void CL450DumpTMEM( void );
void CL450DumpDRAM( int32 byteAddress, int32 length );
void CL450FillDRAM( int32 byteAddress, int32 length, int32 value );
uint32 FMVRead450DRAM( uint32 addr );
void FMVWrite450DRAM( uint32 addr, uint32 value );
void testDRAMStuff( void );
void CL450CompareDRAM( void );

#endif

