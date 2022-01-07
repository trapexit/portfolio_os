#ifndef __DEBUG_H
#define __DEBUG_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: debug.h,v 1.18 1994/10/07 19:53:57 vertex Exp $
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define DEBUGGERNODE (0)
#define DEBUGGERSWI ((DEBUGGERNODE << 16) + 0x0100)

extern void __swi(0x1000e) kprintf(const char *fmt, ... );
extern int __swi(0x10000+30) MayGetChar(void);	/* get a char from diagport */
extern void __swi(0x101) Debug(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#define MACNAMEBUFBYTES 128

typedef struct dbghdr
{
	uint32	dbgLock;
	uint32	dbgReady;
} dbghdr;

typedef struct debugio
{
	uint32	reqOwner;	/* unused */
	uint32	reqCallerID;	/* unused */
	uint32	reqCommand;
	int32	*reqStatusPtr;
	uint32	ptrs[4];	/* misc other args */
	char    namebuf[MACNAMEBUFBYTES];
} debugio;

#ifndef EXTERNAL_RELEASE

/* print_vinfo() prints a versioned greeting banner */
extern void	print_vinfo(void);

typedef struct macFileIOWrite
{
	int32	Length;
	void*	Buffer;
} macFileIOWrite;


typedef struct macFileIORead
{
	int32	Length;
	int32	Offset;
	void*	Buffer;
} macFileIORead;

#define PCKT_ADDR	0x3ff80		/* or something */

/* streaming I/O data structures */

typedef struct MacStreamIO
{
	volatile uint8 *frame1Ptr;
	volatile uint8 *frame2Ptr;
	int32	top,left;
	int32	width,height;
	int32	modulo;	/* offset to next row */
	uint32	flags;

} MacStreamIO;

#define DMAFLAG_CONTINUOUS	1
#define DMAFLAG_LIVEVIDEO	2
#define DMAFLAG_STANDARD	4	/* row0,row1,row2,row3.... */

/* sent to debuger at user program abort */
#define ILLINS_ABORT	0x04
#define DATA_ABORT	0x10
#define PREFETCH_ABORT	0x0c

#endif /* EXTERNAL_RELEASE */

/*****************************************************************************/


#endif /* __DEBUG_H */
