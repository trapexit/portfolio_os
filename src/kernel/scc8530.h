/* $Id: scc8530.h,v 1.3 1994/07/06 20:00:26 limes Exp $ */

#ifndef	SCC8530_H
#define	SCC8530_H

/*
 * Zilog 8530 Serial Communications Controller
 */

typedef struct {
    vuint32		ctlB;
    vuint32		dtaB;
    vuint32		ctlA;
    vuint32		dtaA;
} scc8530;

/*
 * Any two accesses to the Z8530 must be separated
 * by at least six P-clocks, plus 200ns.
 */
#define	SCC_WAIT_CNT	10
#define	SCC_HOLDOFF()	{ int dct = SCC_WAIT_CNT; while (dct-->0); }

/*
 * How to calculate Baud Rate constants ...
 * XXX- A 14.7456Mhz Xtal and x16 clock mode works really well.
 */

#define	SCC_XTAL_HZ	4000000
#define	SCC_CLKDIV	1

#define	SCC_BAUD_CONST(br)		((uint32)(((SCC_XTAL_HZ/(2*SCC_CLKDIV))+((br)/2))/(br)-2))
#define	SCC_BAUD_CONST_HI(br)		(0xFF & (SCC_BAUD_CONST(br) >> 8))
#define	SCC_BAUD_CONST_LO(br)		(0xFF & SCC_BAUD_CONST(br))

#define	SCC_CONST_BAUD(bc)		((SCC_XTAL_HZ/(2*SCC_CLKDIV))/((bc)+2))

/*
 * 		4MHZ XTAL, x1 CLOCK MODE
 *         des.           actual
 *	   rate  const     rate      error
 * 
 *	   50.0 0x9C3E    50.0000    exact
 *	   75.0 0x6829    74.9991  -0.0012%
 *	  110.0 0x4704   109.9989  -0.0010%
 *	  134.5 0x3A14   134.4990  -0.0007%
 *	  150.0 0x3413   150.0038   0.0025%
 *	  200.0 0x270E   200.0000    exact
 *	  300.0 0x1A09   299.9850  -0.0050%
 *	  600.0 0x0D03   600.0600   0.0100%
 *	 1200.0 0x0681  1199.7600  -0.0200%
 *	 1800.0 0x0455  1800.1800   0.0100%
 *	 2400.0 0x033F  2400.9604   0.0400%
 *	 4800.0 0x019F  4796.1631  -0.0799%
 *	 9600.0 0x00CE  9615.3846   0.1603%
 *	19200.0 0x0066 19230.7692   0.1603%
 *	38400.0 0x0032 38461.5385   0.1603%
 *	57600.0 0x0021 57142.8571  -0.7937%
 *
 * 		4MHZ XTAL, x16 CLOCK MODE
 *	   50   0x09C2    50         exact
 *	   75   0x0681    74.9850  -0.0200%
 *	  110   0x046E   110.0352   0.0320%
 *	  134.5 0x039F   134.5533   0.0396%
 *	  150   0x033F   150.0600   0.0400%
 *	  200   0x026F   200         exact
 *	  300   0x019F   299.7602  -0.0799%
 *	  600   0x00CE   600.9615   0.1603%
 *	 1200   0x0066  1201.9231   0.1603%
 *	 1800   0x0043  1811.5942   0.6441%
 *	 2400   0x0032  2403.8462   0.1603%
 *	 4800   0x0018  4807.6923   0.1603%
 *	 9600   0x000B  9615.3846   0.1603%
 *	19200   0x0005 17857.1429  -6.9940%
 *	38400   0x0001 41666.6667   8.5069%
 *	57600   0x0000 62500        8.5069%
 *
 *
 * 9600 is the fastest we can reliably communicate with the Macintosh:
 * the error is too large on higher baud rates. Perhaps a 4mhz xtal
 * was not the best choice ... a more standard one might be:
 *
 *		14.7456 Mhz XTAL
 *         des.           actual
 *	   rate  const     rate      error
 *	   50   0x23FE    50         exact
 *	   75   0x17FE    75         exact
 *	  110   0x105B   110.0024   0.0022%
 *	  134.5 0x0D60   134.5009   0.0007%
 *	  150   0x0BFE   150         exact
 *	  200   0x08FE   200         exact
 *	  300   0x05FE   300         exact
 *	  600   0x02FE   600         exact
 *	 1200   0x017E  1200         exact
 *	 1800   0x00FE  1800         exact
 *	 2400   0x00BE  2400         exact
 *	 4800   0x005E  4800         exact
 *	 9600   0x002E  9600         exact
 *	19200   0x0016 19200         exact
 *	38400   0x000A 38400         exact
 *	57600   0x0006 57600         exact
 */

/*
 * Z8530 register numbers and bit layouts
 */

#define	SCC_CSR		0	/* r: tx/rx/ext status */
#define	    SCC_CSR_RXRDY		0x01
#define	    SCC_CSR_ZEROCT		0x02
#define	    SCC_CSR_TXRDY		0x04
#define	    SCC_CSR_DCD			0x08
#define	    SCC_CSR_SYNC		0x10
#define	    SCC_CSR_CTS			0x20
#define	    SCC_CSR_TXURUN		0x40
#define	    SCC_CSR_BREAK		0x80

#define	    SCC_CSR_ATRESET_MASK	(SCC_CSR_CTS|SCC_CSR_DCD|SCC_CSR_TXRDY)
#define	    SCC_CSR_ATRESET_VAL		(SCC_CSR_CTS|SCC_CSR_DCD|SCC_CSR_TXRDY)

#define	SCC_MODE	0	/* w: init, modes */
#define	    SCC_MODE_EXTRES		0x10
#define	    SCC_MODE_ABORT		0x18
#define	    SCC_MODE_RXINTEN		0x20
#define	    SCC_MODE_TXINTACK		0x28
#define	    SCC_MODE_ERRORRES		0x30
#define	    SCC_MODE_IUSRES		0x38
#define	    SCC_MODE_RXCRCRES		0x40
#define	    SCC_MODE_TXCRCRES		0x80
#define	    SCC_MODE_TXURUN		0xC0

#define	SCC_SRC		1	/* r: special receive conditions */
#define	    SCC_SRC_ALLSENT		0x01
#define	    SCC_SRC_RESCODE2		0x02
#define	    SCC_SRC_RESCODE1		0x04
#define	    SCC_SRC_RESCODE0		0x08
#define	    SCC_SRC_PARERR		0x10
#define	    SCC_SRC_RRXOVF		0x20
#define	    SCC_SRC_FRAME		0x40

#define	SCC_INTSETUP	1	/* w: tx/rx intr mode */
#define	    SCC_INTSETUP_EXTINTEN	0x01
#define	    SCC_INTSETUP_TXINTEN	0x02
#define	    SCC_INTSETUP_PARISSRC	0x04
#define	    SCC_INTSETUP_RXINTDIS	0x00
#define	    SCC_INTSETUP_RXINT1ST	0x08
#define	    SCC_INTSETUP_RXINTALL	0x10
#define	    SCC_INTSETUP_RXINTSRC	0x18
#define	    SCC_INTSETUP_DMARCV		0x20
#define	    SCC_INTSETUP_DMAFUNC	0x40
#define	    SCC_INTSETUP_DMAENAB	0x80

#define	SCC_IVEC	2	/* rw: intr vector */

#define	SCC_IPEND	3	/* r: intr pending bits */
#define	    SCC_IPEND_EXTBIP		0x01
#define	    SCC_IPEND_TXBIP		0x02
#define	    SCC_IPEND_RRXBIP		0x04
#define	    SCC_IPEND_EXTAIP		0x08
#define	    SCC_IPEND_TXAIP		0x10
#define	    SCC_IPEND_RRXAIP		0x20

#define	SCC_RXPARM	3	/* w: rx control */
#define	    SCC_RXPARM_RXEN		0x01
#define	    SCC_RXPARM_SCLI		0x02
#define	    SCC_RXPARM_ASEARCH		0x04
#define	    SCC_RXPARM_CRCEN		0x08
#define	    SCC_RXPARM_HUNT		0x10
#define	    SCC_RXPARM_AUTOEN		0x20
#define	    SCC_RXPARM_5BIT		0x00
#define	    SCC_RXPARM_7BIT		0x40
#define	    SCC_RXPARM_6BIT		0x80
#define	    SCC_RXPARM_8BIT		0xC0

#define	SCC_MISCPARM	4	/* w: tx/rx misc parms */
#define	    SCC_MISCPARM_PARENB		0x01
#define	    SCC_MISCPARM_PAREVEN	0x02
#define	    SCC_MISCPARM_SYNC		0x00
#define	    SCC_MISCPARM_1STOP		0x04
#define	    SCC_MISCPARM_15STOP		0x08
#define	    SCC_MISCPARM_2STOP		0x0C
#define	    SCC_MISCPARM_SYNC8		0x00
#define	    SCC_MISCPARM_SYNC15		0x10
#define	    SCC_MISCPARM_SDLC		0x20
#define	    SCC_MISCPARM_EXTSYNC	0x30
#define	    SCC_MISCPARM_CLK1		0x00
#define	    SCC_MISCPARM_CLK16		0x40
#define	    SCC_MISCPARM_CLK32		0x80
#define	    SCC_MISCPARM_CLK64		0xC0

#define	SCC_TXPARM	5	/* w: tx control */
#define	    SCC_TXPARM_TXCRCEN		0x01
#define	    SCC_TXPARM_RTS		0x02
#define	    SCC_TXPARM_CRC15		0x04
#define	    SCC_TXPARM_TXEN		0x08
#define	    SCC_TXPARM_BREAK		0x10
#define	    SCC_TXPARM_5BIT		0x00
#define	    SCC_TXPARM_7BIT		0x20
#define	    SCC_TXPARM_6BIT		0x40
#define	    SCC_TXPARM_8BIT		0x60
#define	    SCC_TXPARM_DTR		0x80

#define	SCC_SYNC1	6	/* w: sync, SDLC addr */

#define	SCC_SYNC2	7	/* w: sync, SDLC flag */

#define	SCC_RXDATA	8	/* r: rx data */

#define	SCC_TXDATA	8	/* w: tx data */

#define	SCC_MICR	9	/* w: master intr ctl and reset */
#define	    SCC_MICR_VIS		0x01
#define	    SCC_MICR_NV			0x02
#define	    SCC_MICR_DLC		0x04
#define	    SCC_MICR_MIE		0x08
#define	    SCC_MICR_STAT		0x10
#define	    SCC_MICR_RESB		0x40
#define	    SCC_MICR_RESA		0x80
#define	    SCC_MICR_RESET		0xC0

#define	SCC_MISC	10	/* rw: misc status and control */
#define	    SCC_MISC_ONLOOP		0x02
#define	    SCC_MISC_LOOPSEND		0x10
#define	    SCC_MISC_2CLKMISSING	0x40
#define	    SCC_MISC_1CLKMISSING	0x80

#define	SCC_CLOCK	11	/* w: clock mode ctrl */
#define	    SCC_CLOCK_TRXCO_XTAL	0x00
#define	    SCC_CLOCK_TRXCO_XCLK	0x01
#define	    SCC_CLOCK_TRXCO_BRGEN	0x02
#define	    SCC_CLOCK_TRXCO_DPLL	0x03
#define	    SCC_CLOCK_TRXCOI		0x04
#define	    SCC_CLOCK_TXCI_RTXC		0x00
#define	    SCC_CLOCK_TXCI_TRXC		0x08
#define	    SCC_CLOCK_TXCI_BRGEN	0x10
#define	    SCC_CLOCK_TXCI_DPLL		0x18
#define	    SCC_CLOCK_RXCI_RTXC		0x00
#define	    SCC_CLOCK_RXCI_TRXC		0x20
#define	    SCC_CLOCK_RXCI_BRGEN	0x40
#define	    SCC_CLOCK_RXCI_DPLL		0x60
#define	    SCC_CLOCK_RXCI_XTAL		0x80

#define	SCC_BAUDLO	12	/* w: baud time constant, low byte */

#define	SCC_BAUDHI	13	/* w: baud time constant, high byte */

#define	SCC_MISCCTL	14	/* w: misc control bits */
#define	    SCC_MISCCTL_BRGEN		0x01
#define	    SCC_MISCCTL_BRGSRC		0x02
#define	    SCC_MISCCTL_DTRREQ		0x04
#define	    SCC_MISCCTL_AUTOECHO	0x08
#define	    SCC_MISCCTL_LOOPBACK	0x10
#define	    SCC_MISCCTL_SEARCH		0x20
#define	    SCC_MISCCTL_MISSCLKRES	0x40
#define	    SCC_MISCCTL_DPLLDIS		0x60
#define	    SCC_MISCCTL_SRC_BRG		0x80
#define	    SCC_MISCCTL_SRC_RTXC	0xA0
#define	    SCC_MISCCTL_FM_MODE		0xC0
#define	    SCC_MISCCTL_NRZI_MODE	0xE0

#define	SCC_XINT	15	/* rw: ext intr status/ctrl */
#define	    SCC_XINT_ZCIE		0x02
#define	    SCC_XINT_DCDIE		0x08
#define	    SCC_XINT_SYNCIE		0x10
#define	    SCC_XINT_CTSIE		0x20
#define	    SCC_XINT_TXIE		0x40
#define	    SCC_XINT_BRKIE		0x80

#endif
