#ifndef	CONIO_H
#define	CONIO_H

/* $Id: conio.h,v 1.4 1994/07/14 01:21:03 limes Exp $ */

/*
 * cio_mask is the set of console drivers
 * that are to be used. If you "undef" the
 * definition of the bit, the driver will
 * no longer be linked into the kernel.
 */

extern char		serial_ok;
extern uint32		cio_mask;

#define	CIO_GCIO	0x01		/* graphic console */
#define	CIO_OLIO	0x02		/* optical link console */
#define	CIO_SSIO	0x04		/* "old" slowbus serial card */
#define	CIO_ZSIO	0x08		/* ZS85C30 driver (paranoid version) */

/*
 * Disable the Graphic Console output;
 * it is only useful for debugging
 * within 3DO Engineering.
 */
#undef	CIO_GCIO

#ifdef	CIO_GCIO
extern int              gcioInit (int PixWide, int PixHigh);
extern int              gcioPutChar (int ch);
#endif

#ifdef	CIO_OLIO
extern int              olioInit (void);
extern int              olioGetChar (void);
extern int              olioPutChar (int ch);
#endif

#ifdef	CIO_SSIO
extern int              ssioInit (void);
extern int              ssioGetChar (void);
extern int              ssioPutChar (int ch);
#endif

#ifdef	CIO_ZSIO
extern int              zsioInit (void);
extern void		zsioFirqInit (void);
extern int              zsioGetChar (void);
extern int              zsioPutChar (int ch);
#endif

#endif
