/* $Id: depotdipir.c,v 1.5 1994/09/23 21:20:34 markn Exp $ */

#define	APPSPLASH 1

#include "types.h"
#include "inthard.h"
#include "clio.h"
#include "discdata.h"
#include "rom.h"
#include "dipir.h"
#include "aif.h"
#include "depot.h"

DeviceRoutines *dvr;
DipirRoutines *dipr;

extern uint32 ModMCTL(uint32 op, uint32 newmctl);
extern int32 DefaultDisplayImage(void *image0, char *pattern);
uint8 Current3DOFlags(void);


#define	LOGICAL_BS	128

/*
 * Load a signed file from Depot space.
 */
static int
LoadDepot(DipirEnv *de, char *dst, uint32 block, uint32 bytes)
{
	PUTS("LoadDepot:"); PUTHEX(dst); PUTHEX(block); PUTHEX(bytes);
	SingleBuffer(de, de->databuff1);
	DIGESTINIT();
	/* Read logical blocks up to but not including the last block
	 * which contains real data (not counting signature). */
	while (bytes >= LOGICAL_BS + SIG_LEN)
	{
		if (READ(block, LOGICAL_BS))
		{
			PUTS("Depot: Visa read error");
			return 0;
		}
		block += LOGICAL_BS;
		bytes -= LOGICAL_BS;
		UPDATEDIGEST(de->databuff1, LOGICAL_BS);
		if (dst != NULL)
		{
			MEM_MOVE(dst, de->databuff1, LOGICAL_BS);
			dst += LOGICAL_BS;
		}
	}
	/* Read the last block of data (may or may not include the SIG) */
	if (READ(block, LOGICAL_BS))
	{
		PUTS("Depot: Visa read error"); 
		return 0;
	}
	block += LOGICAL_BS;
	UPDATEDIGEST(de->databuff1, bytes-SIG_LEN);
	if (dst != NULL)
	{
		MEM_MOVE(dst, de->databuff1, bytes-SIG_LEN);
		dst += LOGICAL_BS;
	}

	/* Now get the signature in databuff2 */
	if (bytes <= LOGICAL_BS)
	{
		/* Signature is in this block.  Move it to databuff2 */
		MEM_MOVE(de->databuff2, de->databuff1+bytes-SIG_LEN, 
			SIG_LEN);
	} else
	{
		/* Sig is not all in this block.  Construct it in databuff2 */
		MEM_MOVE(de->databuff2, de->databuff1+bytes-SIG_LEN,
			LOGICAL_BS-bytes+SIG_LEN);
		bytes -= LOGICAL_BS;
		if (READ(block, bytes))
		{
			PUTS("Depot read error");
			return 0;
		}
		MEM_MOVE(de->databuff2+SIG_LEN-bytes, de->databuff1, bytes);
	}

	FINALDIGEST();
	return RSAFINALTHDO((unsigned char *)de->databuff2, SIG_LEN);
}

/*
 * Read a block and see if it appears to contain a 3DO disc label.
 */
static bool
ValidLabel(DipirEnv *de, uint32 block)
{
	int i;
	DiscLabel *dl = (DiscLabel *)de->databuff1;

	de->CurrentBuff = de->databuff1;
	if (READ(block, sizeof(DiscLabel))) 
	{
		PUTS("VerifyLabel: BAD READ"); 
		return FALSE;
	}
	if (dl->dl_RecordType != 1) 
	{
		PUTS("VerifyLabel: record type != 1: ");
		PUTHEX(dl->dl_RecordType);
		return FALSE;
	}

	/* Check sync bytes (alternating pattern to check for bit shifts) */
	for (i=0; i < VOLUME_SYNC_BYTE_LEN; i++) 
	{
		if (dl->dl_VolumeSyncBytes[i] != VOLUME_SYNC_BYTE) 
		{
			PUTS("ValidLabel: bad sync byte:");
			PUTHEX(dl->dl_VolumeSyncBytes[i]);
			return FALSE;
		}
	}
	return TRUE;
}

/*
 * Entry point of Depot device dipir code.
 * Depot Dipir does this:
 *	Read splash screen from Depot ROM.  Signature-check and display it.
 *	Check card status:
 *	 if no card, we're done.
 *	 if non-Visa card, eject.
 *	 if Visa card, continue.
 *	Read splash screen from Visa ROM.  Signature-check and display it.
 *
 * If we find a problem with the Depot ROM, we do RESETDEVANDEXIT,
 * which puts us in an infinite dipir loop.
 * If we find a problem with the Visa ROM, we return DIPIR_RETURN_TROJAN,
 * which calls the Depot driver's EJECT function, which sets CARD_EJECT.
 * If there are no problems, return DIPIR_RETURN_SPECIAL rather than
 * DIPIR_RETURN_THREE_BUCKS, so that Depot will work even if it's not unit 0.
 */
DIPIR_RETURN
cddipir(DipirEnv *de)
{
	int status;
	void *image;
	uint8 reg;
	RomTag *rt;
	uint32 block;

	dvr = de->dvr;
	dipr = de->DipirRoutines;

	PUTS("==== Depot cddipir enter\n");
	if (Current3DOFlags() & _3DO_LUNCH) 
	{
		/* This is not a boot time dipir.
		 * We don't know the state of the video hardware; 
		 * an app may be using it.  Don't try to display 
		 * anything, but still do signature checks.  */
		image = NULL;
	} else
	{
		PUTS("Depot: turn on video");
		ModMCTL(MCTL_OP_BITSET, VSCTXEN | CLUTXEN);
		image = (void *) OS_LOAD_ADDR;
	}

	/* Display the Depot ROM splash screen. */
	rt = FindRT((RomTag *)de->databuff2, RSANODE, RSA_APPSPLASH);
	if (rt == NULL)
	{
		PUTS("Depot: No splash screen");
		RESETDEVANDEXIT();
		return DIPIR_RETURN_TROJAN;
	}
	status = LoadDepot(de, (char*)image,
		rt->rt_Offset + de->RomTagBlock, rt->rt_Size);
	if (status == 0)
	{
		PUTS("Depot: Cannot read splash screen");
		RESETDEVANDEXIT();
		return DIPIR_RETURN_TROJAN;
	}
	if (image != NULL) 
	{
		if (DISPLAYIMAGE(image, HW_SPLASH_PATTERN) == 0)
		{
			PUTS("Depot: DisplayImage failed");
			RESETDEVANDEXIT();
			return DIPIR_RETURN_TROJAN;
		}
	}

	/* See if there is a card in the slot. */
	de->CurrentBuff = &reg;
	READBLOCK(DEPOT_REG_SLOT_INPUT);
	PUTS("SLOT_INPUT="); PUTHEX(reg);
	if ((reg & DEPOT_CARD_DETECT) == 0)
	{
		/* No card in the slot */
		PUTS("Depot: no card");
		return DIPIR_RETURN_SPECIAL;
	}

	/* Display the image from the Visa ROM on the card. */
	de->CurrentBuff = &reg;
	READBLOCK(DEPOT_REG_VISA_CONFIG4);
	PUTS("VISA_CONFIG4="); PUTHEX(reg);
	if ((reg & VISA_EXT_ROM) == 0) 
	{
		/* Internal VISA ROM: the splash screen is at a known offset */
		PUTS("Internal VISA ROM");
		status = LoadDepot(de, (char*)image,
				VISA_ROM_XADDR + VISA_ROM_TUPLE_SIZE, 
				VISA_ROM_SIZE - VISA_ROM_TUPLE_SIZE);
	} else
	{
		/* External VISA ROM: looks like a file system, at known offset.
		 * Find the splash screen by looking thru the RomTags. */
		PUTS("External VISA ROM");
		block = VISA_EXT_XADDR + 2*VISA_ROM_TUPLE_SIZE;
		if (!ValidLabel(de, block))
			return DIPIR_RETURN_TROJAN;
		de->CurrentBuff = de->databuff2;
		block += sizeof(struct DiscLabel);
		if (READ(block, MAX_ROMTAG_BLOCK_SIZE))
			return DIPIR_RETURN_TROJAN;
		rt = FindRT((RomTag *)de->databuff2, RSANODE, RSA_APPSPLASH);
		if (rt == NULL)
		{
			PUTS("no splash screen on VISA ext ROM");
			return DIPIR_RETURN_TROJAN;
		}
		status = LoadDepot(de, (char*)image,
				rt->rt_Offset + block, rt->rt_Size);
	}
	if (status == 0)
	{
		PUTS("Cannot read splash screen");
		return DIPIR_RETURN_TROJAN;
	}
	if (image != NULL) 
	{
		if (DISPLAYIMAGE(image, HW_SPLASH_PATTERN) == 0)
		{
			PUTS("DisplayImage (Visa) failed");
			return DIPIR_RETURN_TROJAN;
		}
	}
	return DIPIR_RETURN_SPECIAL;
}
