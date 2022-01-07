/* $Id: stdlibcom.h,v 1.3 1994/12/21 21:29:04 peabody Exp $ */

extern Item _stdlib_MacItem;
extern Item _stdlib_MacIORItm;
extern IOReq *_stdlib_MacIOReq;
extern int InitMacIO(void);
extern int filbuf(FILE *);
FILE *AllocFileBlock ( void );
extern int32 internalfwrite(const void *ptr, int32 size, int32 nitems, FILE *stream);

#define DBUG(x)  /*{kprintf x;}*/
#define DBUG1(x)  /*{kprintf x;}*/
#define TESTALLOCMEM ALLOCMEM
#define TESTFREEMEM FREEMEM

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

/* call this one, NOT <stdio.h> */
#define min(a,b) (((a) < (b)) ? (a) : (b))
