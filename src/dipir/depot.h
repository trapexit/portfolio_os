/* $Id: depot.h,v 1.4 1994/09/23 21:20:34 markn Exp $ */

/* Depot XBus commands */
#define	CMD_READ_TRANSFER	0x20
#define	CMD_WRITE_TRANSFER	0x21
#define	CMD_DOWNLOAD_VISA	0x25

/*
 * Depot addressing.
 * Depot is divided into address ranges:
 *	DEPOT_ROM	Depot ROM.
 *	DEPOT_REG	Depot internal registers.
 *	VISA_ROM	Visa internal ROM.
 *	VISA_EXT	Visa external ROM.
 * Each address range has an external start address (XADDR) and length (LEN).
 * It also has an internal address (IADDR) which is the real hardware address.
 * External addresses are different from internal because the volume label
 * in DEPOT_ROM must start at an address which can be described by a
 * Min/Sec/Frame type of address, with each field 8 bits (see struct TOCInfo).
 */
#define	DEPOT_ROM_XADDR		0x0
#define	DEPOT_ROM_LEN		0x04000000
#define	DEPOT_ROM_IADDR		0x10000000

#define	DEPOT_REG_XADDR		0x0C000000
#define	DEPOT_REG_LEN		0x04000000
#define	DEPOT_REG_IADDR		0x0C000000

#define	VISA_ROM_XADDR		0x80000000
#define	VISA_ROM_LEN		0x04000000
#define	VISA_ROM_IADDR		0x1C000000

#define	VISA_EXT_XADDR		0x90000000
#define	VISA_EXT_LEN		0x04000000
#define	VISA_EXT_IADDR		0x26000000

/* Depot internal registers */
#define	DEPOT_REG_SLOT_INPUT	(DEPOT_REG_XADDR+0x9)
#define	DEPOT_REG_FUNC_CTL	(DEPOT_REG_XADDR+0xB)
#define	DEPOT_REG_VISA_CONFIG1	(DEPOT_REG_XADDR+0x11)
#define	DEPOT_REG_VISA_CONFIG2	(DEPOT_REG_XADDR+0x12)
#define	DEPOT_REG_VISA_CONFIG3	(DEPOT_REG_XADDR+0x13)
#define	DEPOT_REG_VISA_CONFIG4	(DEPOT_REG_XADDR+0x14)

/* Bits in DEPOT_REG_SLOT_INPUT */
#define	DEPOT_CARD_DETECT	0x02	/* Card is present in the slot */

/* Bits in DEPOT_REG_FUNC_CTL */
#define	DEPOT_CARD_RESET	0x01	/* Reset the card */
#define	DEPOT_CARD_EJECT	0x02	/* "Eject" the card */
#define	DEPOT_WRITE_ROM		0x04	/* Allow writing to card ROM space */
#define	DEPOT_SOFT_TRISTATE	0x08	/* Tristate PCMCIA outputs */
#define	DEPOT_WRITE_PROTECT	0x40	/* Disallow writing to card */
#define	DEPOT_CARD_POWER	0x80	/* Feed power to card */

/* Bits in DEPOT_REG_VISA_CONFIG4 */
#define	VISA_EXT_ROM		0x10	/* This Visa has an external ROM */

/*
 * Visa ROM contents
 * Visa ROM (internal or external) starts with 40 bytes of PCMCIA tuples.
 * Internal ROM has a splash screen immediately following tuples.
 * External ROM has a file system immediately following tuples.
 */
#define	VISA_ROM_SIZE		256	/* Size of internal ROM */
#define	VISA_ROM_TUPLE_SIZE	40	/* Size of PCMCIA tuples */
