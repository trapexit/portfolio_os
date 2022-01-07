#ifndef	__NSIO_H__
#define	__NSIO_H__

/*
 * Generic serial stuff
 */

#define	SIOFLAGS_COMPLETE	0x80000000

/**********************************************************************/

/*
 * System Implementation Specific Constants
 */

#define	NS_BASE		0x031C0040
#define	NS_BAUD		4800
#define	NS_XTAL		4000000	/* Toshiba will use (24960000/325) */

/*
 * We feed a[432] from the CPU into a[210] on the UART,
 * so these one-byte registers appear as one per word.
 */
typedef volatile uint32		ns_reg;

/*
 * Given a field name, REG_LVAL mushes it into
 * the proper "lvalue" format that actually gets
 * to the data. "fname" is declared as "ns_reg"
 * declared above.
 */
#define	REG_LVAL(fname)	fname

/**********************************************************************/

/*
 *		Chip-Specific data on the National Semiconductor
 *		PC16550C/NS16550AF Universal Asynchronous
 *		Receiver/Transmitter with FIFOs
 */
#define	NS_ADDRESS_SPACE_REGS	8
typedef union ns16550 {
    ns_reg		ns_a[NS_ADDRESS_SPACE_REGS];
    struct {			/* Read/Write registers */
	ns_reg		rw___0;
	ns_reg		rw_ier;	/* 1: Interrupt Enable Register */
	ns_reg		rw___2;
	ns_reg		rw_lcr;	/* 3: Line Control Register */
	ns_reg		rw_mcr;	/* 4: MODEM Control Register */
	ns_reg		rw___5;
	ns_reg		rw_msr;	/* 6: MODEM Status Register */
	ns_reg		rw_scr;	/* 7: Scratch Register */
    } ns_rw;
    struct {			/* Read-Only registers */
	ns_reg		ro_rbr;	/* 0: Receiver Buffer Register */
	ns_reg		ro___1;
	ns_reg		ro_iir;	/* 2: Interrupt Ident. Register */
	ns_reg		ro___3;
	ns_reg		ro___4;
	ns_reg		ro_lsr;	/* 5: Line Status Register */
	ns_reg		ro___6;
	ns_reg		ro___7;
    } ns_ro;
    struct {			/* Write-Only registers */
	ns_reg		wo_thr;	/* 0: Transmitter Holding Register */
	ns_reg		wo___1;
	ns_reg		wo_fcr;	/* 2: FIFO Control Register */
	ns_reg		wo___3;
	ns_reg		wo___4;
	ns_reg		wo_ftr;	/* 5: (factory test only) */
	ns_reg		wo___6;
	ns_reg		wo___7;
    } ns_wo;
    struct {			/* Divisor Latch registers (LIMITED ACCESS) */
	ns_reg		dl_ls;	/* 0: Divisor Latch (LS) */
	ns_reg		dl_ms;	/* 1: Divisor Latch (MS) */
	ns_reg		dl___2;
	ns_reg		dl___3;
	ns_reg		dl___4;
	ns_reg		dl___5;
	ns_reg		dl___6;
	ns_reg		dl___7;
    } ns_dl;
} ns16550;

/*
 * Convenience field names
 */
#define	ns_rbr	REG_LVAL(ns_ro.ro_rbr)	/* 0: Receiver Buffer Register */
#define	ns_thr	REG_LVAL(ns_wo.wo_thr)	/* 0: Transmitter Holding Register */
#define	ns_ier	REG_LVAL(ns_rw.rw_ier)	/* 1: Interrupt Enable Register */
#define	ns_iir	REG_LVAL(ns_ro.ro_iir)	/* 2: Interrupt Ident. Register */
#define	ns_fcr	REG_LVAL(ns_wo.wo_fcr)	/* 2: FIFO Control Register */
#define	ns_lcr	REG_LVAL(ns_rw.rw_lcr)	/* 3: Line Control Register */
#define	ns_mcr	REG_LVAL(ns_rw.rw_mcr)	/* 4: MODEM Control Register */
#define	ns_lsr	REG_LVAL(ns_ro.ro_lsr)	/* 5: Line Status Register */
#define	ns_msr	REG_LVAL(ns_rw.rw_msr)	/* 6: MODEM Status Register */
#define	ns_scr	REG_LVAL(ns_rw.rw_scr)	/* 7: Scratch Register */

#define	ns_ls	REG_LVAL(ns_dl.dl_ls)	/* 0: Divisor Latch (LS) */
#define	ns_ms	REG_LVAL(ns_dl.dl_ms)	/* 1: Divisor Latch (MS) */

/*
 * Receive Buffer Register
 *
 * Verify that NS_LSR_DR is set before reading this register; any
 * error bits (OE, PE, FE or BI) correspond to the next value
 * available from NS_RBR.
 *
 * Reading from this register consumes and returns the next character
 * on the front of the RxFIFO (and may cause NS_LSR_{DR,OE,PE,FE,BI}
 * to change state).
 */
#define	NS_RBR		0	/* Receiver Buffer Register (RO) */

/*
 * Transmit Holding Register
 *
 * NS_LSR_THRE reflects "TxFIFO empty", but what we really want to see
 * is a "TxFIFO not full" indication. So, we can only start writing to
 * NS_THR when NS_LSR_THRE is active, but we can then write up to 16
 * bytes to NS_THR before waiting TxRDY (or THRE) again.
 */
#define	NS_THR		0	/* Transmitter Holding Register (WO) */

/*
 * Interrupt Enable Resgister
 *
 * If ERBFI is set, RxDA and RxTO interrupts can occur.
 * If ETBEI is set, TxRDY interrupts can occur.
 * If ELSI is set, RxLS interrupts can occur.
 * If EDSSI is set, MODEM interrupts can occur.
 */
#define	NS_IER		1	/* Interrupt Enable Register */
#define	NS_IER_ERBFI	0x01	/* Enable Received Data Available Interrupt */
#define	NS_IER_ETBEI	0x02	/* Enable Transmitter Holding Register Empty Interrupt */
#define	NS_IER_ELSI	0x04	/* Enable Receiver Line Status Interrupt */
#define	NS_IER_EDSSI	0x08	/* Enable MODEM Status Interrupt */

/*
 * Interrupt Identification Register
 *
 * RxLS indicates that an OE, PE, FE or BI condition occurred, and is
 * cleared by reading NS_LSR.
 *
 * RxDA indicates that the RxFIFO trigger level has been reached, and
 * is cleared when the RxFIFO is drained to below the trigger level.
 *
 * RxTO indicates that the RxFIFO is not empty but not yet at the
 * RxFIFO trigger level, and at least 4 char times have passed since a
 * character was received. It is cleared by reading NS_RBR.
 *
 * TxRDY indicates that the TxFIFO is empty, and is cleared by reading
 * NS_IIR (if NS_IIR returns TxRDY) or writing to NS_THR.
 *
 * MODEM indicates that CTS, DSR, RI, or DCD changed; it is cleared by
 * reading NS_MSR.
 */
#define	NS_IIR		2	/* Interrupt Ident. Register (RO) */
#define	NS_IIR_IPEND	0x01	/* "0" if interrupt pending */
#define	NS_IIR_INTID	0x0E	/* Interrupt ID */
#define	NS_IIR_INTID_RxLS	0x06	/* OE, PE, FE or BI */
#define	NS_IIR_INTID_RxDA	0x04	/* RxFIFO Trigger Level reached */
#define	NS_IIR_INTID_RxTO	0x0C	/* RxFIFO Timeout */
#define	NS_IIR_INTID_TxRDY	0x02	/* TxFIFO empty */
#define	NS_IIR_INTID_MODEM	0x00	/* MODEM Status */
#define	NS_IIR_FIFOEN	0xC0	/* FIFO Enable Indicators */

/*
 * FIFO Control Register (WO)
 *
 * Changing FIFOEN clears RxFIFO and TxFIFO.
 *
 * RFRES and XFRES are self-clearing, no followup write
 * of "0" to these bits is required.
 */
#define	NS_FCR		2	/* FIFO Control Register (WO) */
#define	NS_FCR_FIFOEN	0x01	/* FIFO Enable */
#define	NS_FCR_RFRES	0x02	/* Recv FIFO Reset */
#define	NS_FCR_XFRES	0x04	/* Xmit FIFO Reset */
#define	NS_FCR_DMAMODE	0x08	/* DMA Mode Select */
#define	NS_FCR_RCVTRIG	0xC0	/* RCVR Trigger */
#define	NS_FCR_RXT_1	0x00	/* ... interrupt with 1 byte in RxFIFO */
#define	NS_FCR_RXT_4	0x40	/* ... interrupt with 4 bytes in RxFIFO */
#define	NS_FCR_RXT_8	0x80	/* ... interrupt with 8 bytes in RxFIFO */
#define	NS_FCR_RXT_14	0xC0	/* ... interrupt with 14 bytes in RxFIFO */

/*
 * Line Control Register
 *
 * FORCING PARITY BIT=0: set PEN=1, EPS=1, SPAR=1
 * FORCING PARITY BIT=1: set PEN=1, EPS=0, SPAR=1
 *
 * TO SEND A BREAK: (from the ns16550 docs)
 *	1. Load an all 0s, pad character, in response to THRE
 *	2. Set break after the next THRE
 *	3. Wait for the transmitter to be idle, (TEMT=1), and
 *	   clear break when normal transmission has to be restored.
 *	During the break, the Transmitter can be used as a character
 *	timer to accurately establish the break duration.
 * NOTE: many devices want at least 300ms for a break. We are probably
 * better off using our own services to wait than using the serial
 * transmit interrupts.
 */
#define	NS_LCR		3	/* Line Control Register */
#define	NS_LCR_WLS	0x03	/* Word Length Select (mask) */
#define	NS_LCR_WLS_5	 0x00	/* ... 5 bits */
#define	NS_LCR_WLS_6	 0x01	/* ... 6 bits */
#define	NS_LCR_WLS_7	 0x02	/* ... 7 bits */
#define	NS_LCR_WLS_8	 0x03	/* ... 8 bits */
#define	NS_LCR_STB	0x04	/* Number of Stop Bits */
#define	NS_LCR_STB_1	 0x00	/* ... 1 stop bit */
#define	NS_LCR_STB_15	 0x01	/* ... 1.5 stop bits (when WLS is 5bit) */
#define	NS_LCR_STB_2	 0x01	/* ... 2 stop bits (when WLS not 5bit) */
#define	NS_LCR_PEN	0x08	/* Parity Enable */
#define	NS_LCR_EPS	0x10	/* Even Parity Select */
#define	NS_LCR_SPAR	0x20	/* "Stick Parity" (see above) */
#define	NS_LCR_BREAK	0x40	/* Set Break */
#define	NS_LCR_DLAB	0x80	/* Divisor Latch Access Bit */

/*
 * Modem Control Register
 *
 * OUT1 and OUT2 are generic output bits, probably unused.
 * LOOP puts the UART into Loopback Testing Mode:
 *
 *	The transmitter Serial Out (SOUT) is set to Marking state; the
 *	receiver Serial Input (SIN) is disconnected; the output of the
 *	Transmitter Shift Register is "looped back" into the Receiver
 *	Shift Register input; the four MODEM control inputs (/DSR,
 *	/CTS, /RI and /DCD) are disconnected; and the four MODEM
 *	control outputs (/DTR, /RTS, /OUT1 and /OUT2) are internally
 *	connected to the four MODEM control Inputs; and the MODEM
 *	Control output pins are forced to their inactive state
 *	(high). In the loop-back mode, data that is transmitted is
 *	immediately received. This feature allows the processor to
 *	verify the transmit-and-received-data paths of the UART.
 *
 *	In the loopback mode, the receiver and transmitter interrupts
 *	are fully operational. Their sources are external to the
 *	part. The MODEM Control Interrupts are also operational, but
 *	the interrupt's sources are now the lower four bits of the
 *	MODEM Control Register instead of the four MODEM Control
 *	Inputs. The interrupts are still controlled by the Interrupt
 *	Enable Register.
 */
#define	NS_MCR		4	/* MODEM Control Register */
#define	NS_MCR_DTR	0x01	/* Data Terminal Ready */
#define	NS_MCR_RTS	0x02	/* Request to Send */
#define	NS_MCR_OUT1	0x04	/* System Implementation Specific */
#define	NS_MCR_OUT2	0x08	/* System Implementation Specific */
#define	NS_MCR_LOOP	0x10	/* Internal Loopback Mode */

/*
 * Line Status Register (RO)
 *
 * (The following descriptions assume we are in FIFO mode.)
 *
 * NS_LSR_DR is set when the Rx FIFO is not empty; it clears when the
 * last byte is read from NS_RBR.
 *
 * NS_LSR_OE is set when data is moved out of the shift register into
 * the RxFIFO, but the RxFIFO was full. The new charcter replaces the
 * old one in the RxFIFO.
 *
 * NS_LSR_PE is set when the character at the front of the RxFIFO was
 * received with bad parity.
 *
 * NS_LSR_FE is set when the character at the front of the RxFIFO was
 * received without a proper stop bit.
 *
 * NS_LSR_BI is set when the character at the front of the RxFIFO is
 * the NUL byte associated with a BREAK condition.
 *
 * NS_LSR_THRE indicates that the TxFIFO is empty (but the serial
 * output shift register may still be busy).
 *
 * NS_LSR_TEMT indicates that the TxFIFO and the shift register are
 * both empty. This can be used to verify that the final character has
 * indeed been transmitted before manipulating (for instance) the baud
 * rate or modem control signals.
 *
 * NS_LSR_RFERR indicates that there is a PE, FE, or BI somewhere in
 * the RxFIFO.
 */
#define	NS_LSR		5	/* Line Status Register (RO) */
#define	NS_LSR_DR	0x01	/* Data Ready */
#define	NS_LSR_OE	0x02	/* Overrun Error */
#define	NS_LSR_PE	0x04	/* Parity Error */
#define	NS_LSR_FE	0x08	/* Framing Error */
#define	NS_LSR_BI	0x10	/* Break Interrupt */
#define	NS_LSR_THRE	0x20	/* Transmitter Holding Register */
#define	NS_LSR_TEMT	0x40	/* Transmitter Empty */
#define	NS_LSR_RFERR	0x80	/* Error in RCVR FIFO */

/*
 * MODEM Status Register
 *
 * DCTS (DDSR, DDCD) indicates that the /CTS (/DSR, /DCD) input to the
 * chip has changed since the last time the CPU read NS_MSR.
 *
 * TERI is set when the /RI input to the chip has changed from a low
 * state to a high state since the last time the CPU read NS_MSR. This
 * corresponds to the end of the ring signal.
 *
 * CTS, DSR, RI and DCD reflect the incoming signals of similar names,
 * or the corresponding output bits (RTS, DTR, OUT1, OUT2) when the
 * UART is in Loopback mode.
 */
#define	NS_MSR		6	/* MODEM Status Register */
#define	NS_MSR_DCTS	0x01	/* Delta Clear to Send */
#define	NS_MSR_DDSR	0x02	/* Delta Data Set Ready */
#define	NS_MSR_TERI	0x04	/* Trailing Edge Ring Indicator */
#define	NS_MSR_DDCD	0x08	/* Delta Data Carrier Detect */
#define	NS_MSR_CTS	0x10	/* Clear to Send */
#define	NS_MSR_DSR	0x20	/* Data Set Ready */
#define	NS_MSR_RI	0x40	/* Ring Indicator */
#define	NS_MSR_DCD	0x80	/* Data Carrier Detect */

/*
 * Scratchpad Register
 *
 * The NS16550 does not use this register in any way; it is intended
 * as a temporary scratchpad for the programmer.
 */
#define	NS_SCR		7	/* Scratch Register */

/*
 * Divisor Latch Access Registers.
 * 
 * NB: ACCESS TO THESE MUST BE CAREFULLY CONTROLLED: We must turn
 * NS_LCR_DLAB on before accessing them, then turn DLAB off again;
 * during this time, of course, NS_RBR, NS_THR and NS_IER are not
 * available, so we MUST NOT take an interrupt that talks to serial or
 * switches to another thread that talks to serial (Translation:
 * disable interrupts around changing the baud rate; Life Is Rough).
 *
 * divisor # = (frequency input) / (baud rate * 16)
 *
 * Standard Crystals (mentioned in NS16550 document; 18.432MHz RECOMMENDED):
 *
 *    baud   :    1.8432MHz :    3.0720MHz :   18.4320MHz :   24.0000MHz
 *    rate   :   div   %err :   div   %err :   div   %err :   div   %err
 * -------   + -----  ----- + -----  ----- + -----  ----- + -----  -----
 *      50   :  2304    ... :  3840    ... : 23040    ... : 30000    ...
 *      75   :  1536    ... :  2560    ... : 15360    ... : 20000    ...
 *     110   :  1047  0.026 :  1745  0.026 : 10473 -0.003 : 13636  0.003
 *     134.5 :   857 -0.058 :  1428 -0.034 :  8565  0.001 : 11152  0.004
 *     150   :   768    ... :  1280    ... :  7680    ... : 10000    ...
 *     200   :   576    ... :   960    ... :  5760    ... :  7500    ...
 *     300   :   384    ... :   640    ... :  3840    ... :  5000    ...
 *     600   :   192    ... :   320    ... :  1920    ... :  2500    ...
 *     900   :   128    ... :   213  0.156 :  1280    ... :  1667 -0.020
 *    1200   :    96    ... :   160    ... :   960    ... :  1250    ...
 *    1800   :    64    ... :   107 -0.312 :   640    ... :   833  0.040
 *    2000   :    58 -0.690 :    96    ... :   576    ... :   750    ...
 *    2400   :    48    ... :    80    ... :   480    ... :   625    ...
 *    3600   :    32    ... :    53  0.629 :   320    ... :   417 -0.080
 *    4800   :    24    ... :    40    ... :   240    ... :   313 -0.160
 *    5600   :    21 -2.041 :    34  0.840 :   206 -0.139 :   268 -0.053
 *    7200   :    16    ... :    27 -1.235 :   160    ... :   208  0.160
 *    9600   :    12    ... :    20    ... :   120    ... :   156  0.160
 *   12800   :     9    ... :    15    ... :    90    ... :   117  0.160
 *   19200   :     6    ... :    10    ... :    60    ... :    78  0.160
 *   31250   :     4 -7.840 :     6  2.400 :    37 -0.368 :    48    ...
 *   38400   :     3    ... :     5    ... :    30    ... :    39  0.160
 *   56000   :     2  2.857 :   ...    ... :    21 -2.041 :    27 -0.794
 *  112000   :     1  2.857 :   ...    ... :    10  2.857 :    13  3.022
 *  128000   :   ...    ... :   ...    ... :     9    ... :    12 -2.344
 *  144000   :   ...    ... :   ...    ... :     8    ... :    10  4.167
 *  250000   :   ...    ... :   ...    ... :     5 -7.840 :     6    ...
 *  288000   :   ...    ... :   ...    ... :     4    ... :     5  4.167
 *  300000   :   ...    ... :   ...    ... :     4 -4.000 :     5    ...
 *  375000   :   ...    ... :   ...    ... :     3  2.400 :     4    ...
 *  500000   :   ...    ... :   ...    ... :   ...    ... :     3    ...
 *  750000   :   ...    ... :   ...    ... :   ...    ... :     2    ...
 * 1500000   :   ...    ... :   ...    ... :   ...    ... :     1    ...
 *
 * NONstandard Crystals (other interesting values):
 *
 *    baud   :    0.1536MHz :    3.5795MHz :    4.0000MHz :   14.7456MHz
 *    rate   :   div   %err :   div   %err :   div   %err :   div   %err
 * -------   + -----  ----- + -----  ----- + -----  ----- + -----  -----
 *      50   :   192    ... :  4474  0.008 :  5000    ... : 18432    ...
 *      75   :   128    ... :  2983 -0.003 :  3333  0.010 : 12288    ...
 *     110   :    87  0.313 :  2034 -0.009 :  2273 -0.012 :  8378  0.002
 *     134.5 :    71  0.529 :  1663  0.020 :  1859 -0.014 :  6852  0.001
 *     150   :    64    ... :  1491  0.031 :  1667 -0.020 :  6144    ...
 *     200   :    48    ... :  1119 -0.036 :  1250    ... :  4608    ...
 *     300   :    32    ... :   746 -0.036 :   833  0.040 :  3072    ...
 *     600   :    16    ... :   373 -0.036 :   417 -0.080 :  1536    ...
 *     900   :    11 -3.030 :   249 -0.170 :   278 -0.080 :  1024    ...
 *    1200   :     8    ... :   186  0.232 :   208  0.160 :   768    ...
 *    1800   :     5  6.667 :   124  0.232 :   139 -0.080 :   512    ...
 *    2000   :     5 -4.000 :   112 -0.126 :   125    ... :   461 -0.043
 *    2400   :     4    ... :    93  0.232 :   104  0.160 :   384    ...
 *    3600   :   ...    ... :    62  0.232 :    69  0.644 :   256    ...
 *    4800   :     2    ... :    47 -0.834 :    52  0.160 :   192    ...
 *    5600   :   ...    ... :    40 -0.126 :    45 -0.794 :   165 -0.260
 *    7200   :   ...    ... :    31  0.232 :    35 -0.794 :   128    ...
 *    9600   :     1    ... :    23  1.322 :    26  0.160 :    96    ...
 *   12800   :   ...    ... :    17  2.812 :    20 -2.344 :    72    ...
 *   19200   :   ...    ... :    12 -2.900 :    13  0.160 :    48    ...
 *   31250   :   ...    ... :     7  2.271 :     8    ... :    29  1.694
 *   38400   :   ...    ... :     6 -2.900 :     7 -6.994 :    24    ...
 *   56000   :   ...    ... :     4 -0.126 :   ...    ... :    16  2.857
 *  112000   :   ...    ... :     2 -0.126 :   ...    ... :     8  2.857
 *  128000   :   ...    ... :   ...    ... :     2 -2.344 :     7  2.857
 *  144000   :   ...    ... :   ...    ... :   ...    ... :     6  6.667
 *  250000   :   ...    ... :   ...    ... :     1    ... :     4 -7.840
 *  288000   :   ...    ... :   ...    ... :   ...    ... :     3  6.667
 *  300000   :   ...    ... :   ...    ... :   ...    ... :     3  2.400
 *  375000   :   ...    ... :   ...    ... :   ...    ... :   ...    ...
 *  500000   :   ...    ... :   ...    ... :   ...    ... :     2 -7.840
 *  750000   :   ...    ... :   ...    ... :   ...    ... :   ...    ...
 * 1500000   :   ...    ... :   ...    ... :   ...    ... :   ...    ...
 *
 * The "0.1536Mhz" may not correspond to an actually available
 * crystal, but is the value we would use if we wanted to make sure
 * that even privileged software could never make the hardware go
 * faster than 9600 baud.
 */

#define	NS_LS		0	/* 0: Divisor Latch (LS) */
#define	NS_MS		1	/* 1: Divisor Latch (MS) */

#define	NS_BAUD_DIV(xtal,baud)		(xtal / (baud * 16))
#define	NS_BAUD_DIV_HI(xtal,baud)	(NS_BAUD_DIV(xtal,baud)/256)
#define	NS_BAUD_DIV_LO(xtal,baud)	(NS_BAUD_DIV(xtal,baud)%256)

#endif	/* __NSIO_H__ */
