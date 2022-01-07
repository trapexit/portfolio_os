/* $Id: ssio.c,v 1.4 1994/06/01 20:34:59 limes Exp $ */

#include <types.h>
#include "conio.h"

#ifdef	CIO_SSIO

/* #define	SSIO_DEBUG */

/***********************************************************************
 */

#define	SSIO_CSR_EXPECTED	0x21

#define	SSIO_CSR_RxREADY	0x10
#define	SSIO_CSR_TxREADY	0x20

/***********************************************************************
 */

static unsigned         ssioGetCSR (void);
static void             ssioPutCSR (unsigned val);
static unsigned         ssioGetDATA (void);
static void             ssioPutDATA (unsigned val);

/***********************************************************************
 */

/*
 * ssioInit: initialize the SSIO board
 * return 1 if it is present, 0 if absent.
 */
int
ssioInit (void)
{
    ssioPutCSR (0);
    if (ssioGetCSR () != SSIO_CSR_EXPECTED) {
#ifdef	SSIO_DEBUG
	printf("ssioInit: returning false\n");
#endif
	return 0;
    }
#ifdef	SSIO_DEBUG
    printf("ssioInit: returning true\n");
#endif
    serial_ok = 1;
    return 1;
}

/*
 * ssioGetChar: if ssio has a char ready, return it
 * otherwise return "-1"
 */

int
ssioGetChar (void)
{
    if (!(ssioGetCSR () & SSIO_CSR_RxREADY))
	return -1;
    return ssioGetDATA ();
}

int
ssioPutChar (int c)
{
    while (!(ssioGetCSR () & SSIO_CSR_TxREADY))
	;
    ssioPutDATA (c);
    return c;
}

/***********************************************************************
 */

#define	SSIO_DATA	(*((volatile int *)(0x03180040)))
#define	SSIO_CSR	(*((volatile int *)(0x03180044)))

/***********************************************************************
 */

extern int              ignoreabort;
extern int              abortignored;

static unsigned
ssioGetCSR (void)
{
    unsigned                val;

    ignoreabort = 1;
    do
    {
	abortignored = 0;
	val = SSIO_CSR;
    } while (abortignored);
    ignoreabort = 0;
    return val;
}

static void
ssioPutCSR (unsigned val)
{
/* XXX- use dataabortreruns */
    ignoreabort = 1;
    do
    {
	abortignored = 0;
	SSIO_CSR = val;
    } while (abortignored);
    ignoreabort = 0;
}

static unsigned
ssioGetDATA (void)
{
/* XXX- use dataabortreruns */
    unsigned                val;

    ignoreabort = 1;
    do
    {
	abortignored = 0;
	val = SSIO_DATA;
    } while (abortignored);
    ignoreabort = 0;
    return val;
}

static void
ssioPutDATA (unsigned val)
{
/* XXX- use dataabortreruns */
    ignoreabort = 1;
    do
    {
	abortignored = 0;
	SSIO_DATA = val;
    } while (abortignored);
    ignoreabort = 0;
}

#endif
