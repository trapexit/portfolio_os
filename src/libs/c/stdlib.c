/* $Id: stdlib.c,v 1.26 1994/12/21 21:44:33 peabody Exp $ */
/******************************************************************
** "Standard I/O routines for Opera.
** NOTE: These may not conform exactly to what you expect but
** are pretty close.  Here are some known differences:
**   1) fopen() only supports "w", "r", and "a".
**
** 00001 PLB 8/27/92 Added routines to further conformance with stdio
** 00002 PLB 8/28/92 Added fwrite and fputs
** 00003 PLB 9/3/92  Automatically initialize on every call.
** 00004 Dale 9/8/92 Changed buffers to use pointers, etc.
** 00005 PLB 9/9/92 Removed fflush() from internalfwrite, reset _cp in fflush
**                  Optional memory allocation test. TESTALLOCMEM
** 00006 Dale 11/30/92 fixed fflush, it was reseting wrong bufptr
******************************************************************/

#include "types.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "io.h"
#include "debug.h"
#include "strings.h"
#include "mem.h"
#include "item.h"

/* call this one, NOT <stdio.h> */
#include "stdio.h"
#define min(a,b) (((a) < (b)) ? (a) : (b))

Item _stdlib_MacItem;
Item _stdlib_MacIORItm;
IOReq *_stdlib_MacIOReq = 0;

#define DBUG(x)	 /*{kprintf x;}*/
#define DBUG1(x)  /*{kprintf x;}*/

FILE _stdin,_stdout,_stderr;

FILE *stdin = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

/* Internal support routine prototypes. */
FILE *AllocFileBlock ( void );

/****************************************************************/
/* Check for writes past beginning or end of memory. */
/* #define TESTALLOC */
#ifndef TESTALLOC

#define TESTALLOCMEM ALLOCMEM
#define TESTFREEMEM FREEMEM
#define DBUGM(x)  /* {kprintf x;} */

#else

#define DBUGM(x)  {kprintf x;}

#define TESTALLOCWALL (1200)
#define TESTALLOCCHAR (0xE3)

void *TESTALLOCMEM(long size, ulong type)
{
    uchar *p, *b, *e;
    long i;

/* Allocate extra for padding and testing for corruption. */
    p = ALLOCMEM(size+(2*TESTALLOCWALL), type);
    if (p == 0) return 0;
/* Fill with strange value to detect corruption. */
    b = p;
    e = p+TESTALLOCWALL+size;
    for (i=0; i<TESTALLOCWALL; i++)
    {
         *b++ = TESTALLOCCHAR;
         *e++ = TESTALLOCCHAR;
    }
    return ((void *)(p+TESTALLOCWALL));
}

void TESTFREEMEM(void *ptr, long size)
{
    uchar *p;
    long i;

/* Point to beginning of actual allocated memory. */
    p = (char *) ptr - TESTALLOCWALL;

/* Check strange value to test for corruption. */
    for (i=0; i<TESTALLOCWALL; i++)
    {
         if (p[i] != TESTALLOCCHAR)
	 {
              kprintf("Corruption %d bytes before = $%x\n",
                       TESTALLOCWALL-i, p[i]);
         } 
         if (p[i+size+TESTALLOCWALL] != TESTALLOCCHAR)
	 {
              kprintf("Corruption %d bytes after = $%x\n",
                       i, p[i]);
         } 
    }

/* Free extra for padding and testing for corruption. */
    FREEMEM(p, size+(2*TESTALLOCWALL));
}
#endif
/****************************************************************/

static char *
AllocBuffer(stream)
FILE *stream;
{
	if (stream == NULL) return 0;
	stream->fcb_buffer = (uchar *)TESTALLOCMEM(FCB_BUFSIZE,MEMTYPE_ANY);
	if (stream->fcb_buffer)
	{
		stream->fcb_buffsize = FCB_BUFSIZE;
		stream->fcb_cp = stream->fcb_buffer;
	}
	else stream->fcb_buffer = 0;
	DBUG1(("AllocBuffer returns:%lx\n",stream->fcb_buffer));
	return (stream->fcb_buffer);
}

static TagArg IOTags[] =
{
	CREATEIOREQ_TAG_DEVICE, 0,
	TAG_END, 0,
};


/* This will get called automatically, we hope! */
int
InitMacIO()
{
#ifdef TESTALLOC
  char *pt;
#endif
	if (_stdlib_MacIOReq) return 1;  /* already inited */

	_stdlib_MacItem =  OpenItem(FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),"mac"),0);
	if ((int32)_stdlib_MacItem < 0)
	{
		DBUG1(("MacItem=%lx\n",_stdlib_MacItem));
		return 0;
	}
	IOTags[0].ta_Arg = (void *)_stdlib_MacItem;
	_stdlib_MacIORItm = CreateItem(MKNODEID(KERNELNODE,IOREQNODE), IOTags);
	if ((int32)_stdlib_MacIORItm < 0)
	{
		DBUG1(("MacIORItm=%lx\n",_stdlib_MacItem));
		return 0;
	}
	_stdlib_MacIOReq = (IOReq *)LookupItem(_stdlib_MacIORItm);

/* %T Test memory corruption tester. */
#ifdef TESTALLOC
	pt = TESTALLOCMEM(100, MEMTYPE_ANY);
        if (pt == 0) kprintf("Corruption test couldn't allocate!\n");
        pt[-7] = 0x23;
        pt[109] = 0x77;
        TESTFREEMEM(pt, 100);
        kprintf("Finished corruption test..\n");
#endif

/* Allocate FILEs for stdin and stdout */
	if (AllocBuffer(stdin) == 0)	return 0;
	stdin->fcb_mode = FCB_READ_MODE;
	if (AllocBuffer(stdout) == 0)	return 0;
	stdout->fcb_mode = FCB_WRITE_MODE;
	stdout->fcb_numinbuf = stdout->fcb_buffsize;
	if (AllocBuffer(stderr) == 0)	return 0;
	stderr->fcb_mode = FCB_WRITE_MODE;
	stderr->fcb_numinbuf = stderr->fcb_buffsize;

	return 1;
}

/* Allocate a file control block */
FILE *AllocFileBlock ()
{
	FILE *stream;
	DBUGM(("Entering AllocFileBlock\n"));

	stream = (FILE *) TESTALLOCMEM(sizeof(FILE),MEMTYPE_FILL);
	if (stream == NULL)
	{
              DBUG1(("Could not allocate FILE block!!\n"));
	      return 0;
	}
        DBUGM(("About to allocate FILE buffer!!\n"));
	if (!AllocBuffer(stream))
	{
             DBUG1(("Could not allocate FILE buffer!!\n"));
	    TESTFREEMEM(stream,sizeof(FILE));
	    return 0;
	}

        DBUGM(("Leaving AllocFileBlock successfully.\n"));
	
	return stream;
}

#define CHECKMACIO(ret)  if (_stdlib_MacIOReq == 0) \
	{ \
		if (InitMacIO() == 0) \
		{ \
			DBUG1(("InitMacIO failed\n")); \
			return (ret); \
		} \
	}

#define CHECKSTREAM(name, ret) if (stream == 0) \
	{ kprintf("Stream = 0 in %s!!!\n", name); \
		return(ret); \
	} \
	CHECKMACIO(ret);

int32 internalfwrite(const void *ptr, int32 size, int32 nitems, FILE *stream)
{
	IOInfo ioInfo;
	int32 len = size*nitems;

/* flush any data in buffer */
	
	DBUG1(("\ninternalfwrite(%lx,%d)\n", ptr, len));
	DBUG1(("filename=%s\n", stream->fcb_filename));
	DBUG1(("position=%d\n", stream->fcb_currentpos));

	if (len == 0)	return 0;

	CHECKSTREAM("internalfwrite", -1);
	memset(&ioInfo,0,sizeof(ioInfo));
	ioInfo.ioi_Command = CMD_WRITE;
	ioInfo.ioi_Recv.iob_Buffer =  stream->fcb_filename;
	ioInfo.ioi_Recv.iob_Len = strlen(stream->fcb_filename)+1;
	ioInfo.ioi_Send.iob_Buffer = (void *)ptr;   /* @@@ need this cast because Send.iob_Buffer isn't declared const */
	ioInfo.ioi_Send.iob_Len = len;
/*
	ioInfo.ioi_Flags = 0;
	ioInfo.ioi_Unit = 0;
*/
	ioInfo.ioi_Offset = stream->fcb_currentpos;
	DoIO(_stdlib_MacIORItm,&ioInfo);
	
	if (_stdlib_MacIOReq->io_Error)
	{
		kprintf("error during fwrite=%d of %s\n",_stdlib_MacIOReq->io_Error, stream->fcb_filename);
		return 0;
	}
	stream->fcb_currentpos += len;
	if (stream->fcb_filesize < stream->fcb_currentpos)
	{ 
		stream->fcb_filesize = stream->fcb_currentpos;
	}
	DBUG(("fwrite returning:%d\n",nitems));
	
	return nitems;
}

/* this should really be a macro %Q */
int32 putc(char c, FILE *stream)
{
	/*while (1);*/
	DBUG(("putc(%lx) fcp_cp=%lx\n",c,stream->fcb_cp));
	if((stream == stdout) || (stream == stderr))
	{
		kprintf("%c", c);
	}
	else
	{
		if (--stream->fcb_numinbuf < 0)
                {
		     DBUG(("putc: call fflush, c = %x\n", c));
                     fflush(stream);
		     DBUG(("after flush: fcb_cp=%lx\n",stream->fcb_cp));
                }
		*stream->fcb_cp++ = c;
	}
	return c;
}

int32 fflush(FILE *stream)
{
	int32 len, nw;

	DBUG(("(fflush)\n"));
	if((stream == stdin) || (stream == stdout) || (stream == stderr) )
	{
		return 0;
	}
	
	len = stream->fcb_buffsize - stream->fcb_numinbuf;
DBUG1(("\nfflush: len = %d, numinbuf = %d\n", len, stream->fcb_numinbuf));
	if (len > stream->fcb_buffsize)
	{
              len = stream->fcb_buffsize;
DBUG1(("fflush: len clipped to %d\n", len));
	}
	stream->fcb_numinbuf += len;
	
	if (len > 0)
	{
		if (stream->fcb_mode & FCB_WRITE_MODE)
		{
			nw = internalfwrite (stream->fcb_buffer,
			     sizeof(char), len, stream);
			stream->fcb_cp = stream->fcb_buffer;
			if (nw != len) return EOF;
		}
	}
	return 0;
}
