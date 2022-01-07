/*
	File:		CL450Diags.c

	Contains:	xxx put contents here xxx

	Written by:	xxx put writers here xxx

	Copyright:	© 1993 by 3DO, Inc., all rights reserved.

	Change History (most recent first):

		 <3>	 7/12/94	GM		Changed calls to FMVDo450Command to match new prototype with
									unused arguments removed.
		 <2>	11/30/93	GM		Commented out old stuff.

*/

/* file: CL450Diags.c */
/* CL450 Diagnostic routines removed from CL450.c */
/* 5/9/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#include "CL450.h"
#include "FMV.h"
#include "FMVErrors.h"
#include "FMVROM.h"
#include "Hollywood.h"
#include "Woody.h"

/* diagnostics */

/* get CL450 Status */
void CL450Status()
{
	int32 statusRegister, status;
	
	DEBUGP(("CL450Status:\n"));
	
	/* read the status registers */
	for( statusRegister = 0x8L; statusRegister <= 0xbL; statusRegister++ )
	{
		/* set up address to read from status registers */
		FMVWriteVideoRegister( HOST_RADDR, statusRegister );
		status = FMVReadVideoRegister( HOST_RDATA );
		DEBUGP(("status register 0x%02lx = 0x%04lx\n",statusRegister,status));
	}
#ifdef NEVER
	/* read indirect video registers */
	status = FMV450ReadIndirectVideoRegister( VID_SELACTIVE );
	DEBUGP(("video register VID_SELACTIVE = 0x%04lx\n",status));
	status = FMV450ReadIndirectVideoRegister( VID_SELAUX );
	DEBUGP(("video register VID_SELAUX = 0x%04lx\n",status));
	status = FMV450ReadIndirectVideoRegister( VID_SELGB );
	DEBUGP(("video register VID_SELGB = 0x%04lx\n",status));
	status = FMV450ReadIndirectVideoRegister( VID_SELR );
	DEBUGP(("video register VID_SELR = 0x%04lx\n",status));
	status = FMV450ReadIndirectVideoRegister( VID_SELA );
	DEBUGP(("video register VID_SELA = 0x%04lx\n",status));
	status = FMV450ReadIndirectVideoRegister( VID_SELB );
	DEBUGP(("video register VID_SELB = 0x%04lx\n",status));
	status = FMV450ReadIndirectVideoRegister( VID_SELBOR );
	DEBUGP(("video register VID_SELBOR = 0x%04lx\n",status));
	status = FMV450ReadIndirectVideoRegister( VID_SELMODE );
	DEBUGP(("video register VID_SELMODE = 0x%04lx\n",status));
#endif
}
	
void CL450Diags()
{
	uint32 temp, temp1;
	uint32 codeWordAddress, segmentAddress, TMEMAddress;
	uint32 addrHi, addrLo;

	
	DEBUGP(("CL450Diags: resetting the CL450\n"));
	FMVReset450();			/* soft CL450 reset */
	
	/* set registers to correct initial state */
	
	DEBUGP(("    Setting CMEM_DMACTRL = %08lx\n",0x1feL));
	FMVWriteVideoRegister( CMEM_DMACTRL, 0x1feL );		/* assert CFLEVEL on 4+ empty entries */
	DEBUGP(("    Read value = %08lx\n",FMVReadVideoRegister( CMEM_DMACTRL )));
return;

	DEBUGP(("    Setting CMEM_DMACTRL = %08lx\n",CMDMA_1Q));
	FMVWriteVideoRegister( CMEM_DMACTRL, CMDMA_1Q );		/* assert CFLEVEL on 4+ empty entries */
	DEBUGP(("    Read value = %08lx\n",FMVReadVideoRegister( CMEM_DMACTRL )));
	
	DEBUGP(("    Setting HOST_SCR2 = %08lx\n",SCR2_GLCKSELECT));	
	FMVWriteVideoRegister( HOST_SCR2, SCR2_GLCKSELECT );	/* use hollywood global clock */
	DEBUGP(("    Read value = %08lx\n",FMVReadVideoRegister( HOST_SCR2 )));
	
	DEBUGP(("    Setting System Clock divisor\n"));	
	FMV450SetSystemClockDivisor( HLWD_GCLK );				/* derives MPEG clock from global clock */
	
	temp = DRAM_RFRSHCNT & DRAM_REFCNTMSK;
	DEBUGP(("    Setting DRAM_REFCNT = %08lx\n",temp));	
	FMVWriteVideoRegister( DRAM_REFCNT, temp );				/* DRAM refresh count */
	DEBUGP(("    Read value = %08lx\n",FMVReadVideoRegister( DRAM_REFCNT )));
	
#ifdef NEVER
	DEBUGP(("    Loading boot code\n"));	
	if( FMV450RunBootCode() )
	{
		DEBUGP(("    Attempt to load boot code failed\n"));	
		return;
	}
	DEBUGP(("    verifying boot code operation\n"));

	temp = 0;
	while( temp )
	{
		CL450FillDRAM( 0L, 0x100, 0xffffffffL );
		CL450DumpDRAM( 0L, 0x100 );
	}
	
//	CL450FillDRAM( 0L, 0x800L, 0L );
//	CL450DumpDRAM( 0L, 0x800L );
//	CL450FillDRAM( 0L, 0x800L, 0xffffL );
//	CL450DumpDRAM( 0L, 0x800L );
#endif
while(1)
{
	codeWordAddress = segmentAddress = 0;
	for( temp1 = 0; temp1 < 5; temp1++)
	{
		/* load data into TMEM, the boot code will then move it to DRAM */
		/* initialize TMEMAddress, it autoincrements with each write */
		TMEMAddress = 0;
		FMVWriteVideoRegister( CPU_TADDR, TMEMAddress );
		
		while( TMEMAddress < TMEM_SIZE )
		{
			/* write to TMEM in two 16 bit chunks */
			FMVWriteVideoRegister( CPU_TMEM, ~codeWordAddress );
			FMVWriteVideoRegister( CPU_TMEM, codeWordAddress );
			
			codeWordAddress++;
			TMEMAddress += 2;
		}
		/* TMEM is now loaded*/
		/* execute command which copies TMEM to right place in DRAM */
		/* address is in short words, segmentAddress is in long words */
		/* address is split over two arguments 3:16 */
		temp = segmentAddress << 1;		/* convert to short word address */
		addrLo = temp & 0xffff;			/* extract low 16 bits */
		addrHi = temp >> 16;			/* extract high 3 bits */
		
		DEBUGP(("    transfering segment @ %08lx, size = %ld\n",temp, TMEM_SIZE));
		temp = 0L;
		FMVDo450Command( addrLo, addrHi, TMEM_SIZE, BOOT_WRITEDRAM, temp );
		
		segmentAddress += TMEM_SIZE;
	}
	DEBUGP(("    finished load, beginning read\n"));
	
	codeWordAddress = segmentAddress = 0;
	for( temp1 = 0; temp1 < 5; temp1++)
	{
		uint32 temp2, temp3;
		
		DEBUGP(("    reading segment, size = %ld\n", TMEM_SIZE));
		/* DRAM is now loaded with data */
		/* execute command which copies DRAM to right place in TMEM */
		/* address is in short words, segmentAddress is in long words */
		/* address is split over two arguments 3:16 */
		temp = segmentAddress << 1;		/* convert to short word address */
		addrLo = temp & 0xffff;			/* extract low 16 bits */
		addrHi = temp >> 16;			/* extract high 3 bits */
		
		temp = 0L;
		FMVDo450Command( addrLo, addrHi, TMEM_SIZE, BOOT_READDRAM, temp );
		
		/* read DRAM into TMEM and compare with expected values */
		/* initialize TMEMAddress, it autoincrements with each read */
		TMEMAddress = 0;
		FMVWriteVideoRegister( CPU_TADDR, TMEMAddress );
		
		while( TMEMAddress < TMEM_SIZE )
		{
			temp = (segmentAddress << 2) + (TMEMAddress << 1);
			
			/* read from TMEM in two 16 bit chunks */
			if( (~codeWordAddress & 0xffff) != (temp2 = FMVReadVideoRegister( CPU_TMEM ) ) )
				DEBUGP(("    addr: %08lx = %08lx\n",temp,temp2));
			if( (codeWordAddress & 0xffff) != (temp3 = FMVReadVideoRegister( CPU_TMEM ) ) )
				DEBUGP(("	 addr: %08lx = %08lx\n",temp+2L,temp2));
			
			codeWordAddress++;
			TMEMAddress += 2;
		}
		DEBUGP(("\nDone!\n"));
		segmentAddress += TMEM_SIZE;
	}
}
}

void CL450DumpTMEM()
{
	uint32 codeWord, index, i;
	
	DEBUGP(("Dumping TMEM\n"));
	
	FMVWriteVideoRegister( CPU_TADDR, 0L );
	for( index = 0; index < TMEM_SIZE; )
	{
		DEBUGP(("%02lx: ", index));
		for( i = 0; i < 8; i++ )
		{
			codeWord = FMVReadVideoRegister( CPU_TMEM );
			DEBUGP((" %04lx", codeWord));
			index++;
		}
		DEBUGP(("\n"));
	}
}

void CL450DumpDRAM( int32 byteAddress, int32 length )
{
	int32 addrLo, addrHi, temp, TMEMAddress;
	
	while( length > 0L )
	{
		DEBUGP(("    reading segment, size = %ld\n", TMEM_SIZE));
		/* DRAM is now loaded with data */
		/* execute command which copies DRAM to right place in TMEM */
		/* address is in short words, segmentAddress is in long words */
		/* address is split over two arguments 3:16 */
		temp = byteAddress >> 1;		/* convert to short word address */
		addrLo = temp & 0xffff;			/* extract low 16 bits */
		addrHi = temp >> 16;			/* extract high 3 bits */
		
		/* read DRAM into TMEM */
		temp = 0L;
		FMVDo450Command( addrLo, addrHi, TMEM_SIZE, BOOT_READDRAM, temp );
		
		/* initialize TMEMAddress, it autoincrements with each read */
		TMEMAddress = 0L;
		FMVWriteVideoRegister( CPU_TADDR, TMEMAddress );
	
		while( TMEMAddress < TMEM_SIZE )
		{
			int32 word,i;
			
			DEBUGP(("%08lx: ",byteAddress));
			
			/* read from TMEM */
			for( i = 0; i < 8; i++ )
			{
				word = FMVReadVideoRegister( CPU_TMEM );
				DEBUGP(("%04lx ", word & 0xffffL));
			}
			DEBUGP(("\n"));
			byteAddress += 16; length -= 16;
			TMEMAddress += 8;
		}
	}
}

void CL450FillDRAM( int32 byteAddress, int32 length, int32 value )
{
	int32 addrLo, addrHi, temp, TMEMAddress, segmentAddress;
	
//	value = 0x00000001L;
	
	while( length > 0L )
	{
		DEBUGP(("    writing segment, size = %ld\n", TMEM_SIZE));

		/* start a read-modify write on DRAM, this is necessary because */
		/* !@#$ C-Cube screwed up and can only begin writes to 64 short */
		/* word boundaries */
		/* execute command which copies DRAM to right place in TMEM */
		/* address is in short words, segmentAddress is in long words */
		/* address is split over two arguments 3:16 */
		segmentAddress = byteAddress >> 2L;
		TMEMAddress = (segmentAddress % 32L) << 1L;			/* !@#$ C-Cube */
		segmentAddress &= 0xffffffe0;						/* !@#$ C-Cube */
		temp = segmentAddress << 1;		/* convert to short word address */
		addrLo = temp & 0xffff;			/* extract low 16 bits */
		addrHi = temp >> 16;			/* extract high 3 bits */
		
		temp = 0L;
		FMVDo450Command( addrLo, addrHi, TMEM_SIZE, BOOT_READDRAM, temp );
						 
		/* initialize TMEMAddress, it autoincrements with each write */
		FMVWriteVideoRegister( CPU_TADDR, TMEMAddress );
	
		while( TMEMAddress < TMEM_SIZE )
		{
			FMVWriteVideoRegister( CPU_TMEM, (value >> 16) & 0xffffL );
			FMVWriteVideoRegister( CPU_TMEM, value & 0xffffL );
			
			TMEMAddress += 2;
//			value += 0x00020002L;
			byteAddress += 4;
			length -= 4;
		}
		/* TMEM is now loaded with data */
		/* execute command which copies TMEM to right place in DRAM */
		
		/* write TMEM into DRAM */
		temp = 0L;
		FMVDo450Command( addrLo, addrHi, TMEM_SIZE, BOOT_WRITEDRAM, temp );
	}
}

uint32 FMVRead450DRAM( uint32 addr )
{
	uint32 value;

#ifdef WOODY_REV0
	/* use hw rework to read CL450 memory */
	/* set Woody GPIO pin low (this sets CL450 A[20] low) */
	/* output driver should already be enabled */
	WdySetControlClear( WDYGPIO0 );
	
	/* latch high bits into rework register */
	WdySetAddress( (addr >> 9L) & 0xffL );
	WdySetControlSet( WDYGPIO1 );
	WdySetControlClear( WDYGPIO1 );
	
	/* now fake out woody with a register read */
	value = FMVReadVideoRegister( addr & 0x1ffL );
	
	/* reenable register access */
	WdySetControlSet( WDYGPIO0 );
#endif
	return( value );
}

void FMVWrite450DRAM( uint32 addr, uint32 value )
{
#ifdef WOODY_REV0
	/* use hw rework to write low CL450 memory */
	/* set Woody GPIO pin low (this sets CL450 A[20] low) */
	/* output driver should already be enabled */
	WdySetControlClear( WDYGPIO0 );
	
	/* latch high bits into rework register */
	WdySetAddress( (addr >> 9L) & 0xffL );
	WdySetControlSet( WDYGPIO1 );
	WdySetControlClear( WDYGPIO1 );
	
	/* now fake out woody with a register write */
	FMVWriteVideoRegister( addr & 0x1ffL, value );
	
	/* reenable register access */
	WdySetControlSet( WDYGPIO0 );
#endif
}

void testDRAMStuff()
{
	int32 addr,i;
	uint32 value = 0L;
	int32 addrLo, addrHi, temp, TMEMAddress, segmentAddress;
	uint32 byteAddress = 0L, length = 65536L;
#ifdef NEVER	
	while( length > 0L )
	{
		DEBUGP(("    writing segment, size = %ld\n", TMEM_SIZE));

		/* start a read-modify write on DRAM, this is necessary because */
		/* !@#$ C-Cube µcode can only begin writes to 64 short */
		/* word boundaries */
		/* execute command which copies DRAM to right place in TMEM */
		/* address is in short words, segmentAddress is in long words */
		/* address is split over two arguments 3:16 */
		segmentAddress = byteAddress >> 2L;
		TMEMAddress = (segmentAddress % 32L) << 1L;			/* !@#$ C-Cube */
		segmentAddress &= 0xffffffe0;						/* !@#$ C-Cube */
		temp = segmentAddress << 1;		/* convert to short word address */
		addrLo = temp & 0xffff;			/* extract low 16 bits */
		addrHi = temp >> 16;			/* extract high 3 bits */
		
		temp = 0L;
		FMVDo450Command( addrLo, addrHi, TMEM_SIZE, BOOT_READDRAM,
						 temp, temp, temp, temp ); /* bogus */
						 
		/* initialize TMEMAddress, it autoincrements with each write */
		FMVWriteVideoRegister( CPU_TADDR, TMEMAddress );
	
		while( TMEMAddress < TMEM_SIZE )
		{
			value = ~byteAddress;
			FMVWriteVideoRegister( CPU_TMEM, value & 0xffffL );
			
			TMEMAddress++;
			byteAddress += 2;
			length -= 2;
		}
		/* TMEM is now loaded with data */
		/* execute command which copies TMEM to right place in DRAM */
		temp = 0L;
		FMVDo450Command( addrLo, addrHi, TMEM_SIZE, BOOT_WRITEDRAM,
						 temp, temp, temp, temp ); /* bogus */
	}
	DEBUGP(("Write DRAM 0-65536\n"));
	for(addr = 0L; addr < 65536L;  )
	{
		value = ~addr;
		FMVWrite450DRAM( addr, value );
		addr += 2;
	}
	DEBUGP(("Read DRAM 0-65536\n"));
	for(addr = 0L; addr < 65536L;  )
	{
		value = FMVRead450DRAM( addr );
		if( (value & 0xffffL) != (~addr & 0xffffL) )
		{
			DEBUGP(("Err: addr 0x%08lx = %04lx\n",addr,value));
		}
		addr += 2;
	}

#endif	
	DEBUGP(("Read DRAM 0-2048\n"));
	for(addr = 0L; addr < 2048L;  )
	{
		DEBUGP(("0x%02lx: ",addr));
		for(i = 0; i < 8; i++)
		{
			value = FMVRead450DRAM( addr );
			DEBUGP((" 0x%04lx",value));
			addr += 2;
		}
		DEBUGP(("\n"));
	}
	DEBUGP(("Writing DRAM 0-2048 "));
	for( addr = 0L; addr < 2048L; addr+=2L )
	{
		FMVWrite450DRAM( addr, value );
		value += 0x01010101;
		DEBUGP(("."));
	}
	DEBUGP(("Read DRAM 0-2048\n"));
	for(addr = 0L; addr < 2048L;  )
	{
		DEBUGP(("0x%02lx: ",addr));
		for(i = 0; i < 8; i++)
		{
			value = FMVRead450DRAM( addr );
			DEBUGP((" 0x%04lx",value));
			addr += 2;
		}
		DEBUGP(("\n"));
	}
	DEBUGP(("Dumping DRAM via boot code\n"));
	CL450DumpDRAM( 0L, 2048L );
	
	DEBUGP(("Filling DRAM via boot code\n"));
	CL450FillDRAM( 0L, 2048L, 0xa55a5aa5L );
	
	DEBUGP(("Read DRAM 0-2048\n"));
	for(addr = 0L; addr < 2048L;  )
	{
		DEBUGP(("0x%02lx: ",addr));
		for(i = 0; i < 8; i++)
		{
			value = FMVRead450DRAM( addr );
			DEBUGP((" 0x%04lx",value));
			addr += 2;
		}
		DEBUGP(("\n"));
	}
}

#ifdef NEVER

void CL450CompareDRAM( void )
{
	uint32 lo,hi,codeWord,wAddr,bAddr;
	
	for( wAddr = 0L; wAddr < DRAM_SIZE; wAddr++ )
	{
		bAddr = wAddr << 2;
		
		hi = FMVRead450DRAM( bAddr );
		lo = FMVRead450DRAM( bAddr + 2L );
		codeWord = (hi & 0xffffL) << 16 | (lo & 0xffffL);
		
		if( codeWord != DRAMImage[ wAddr ] )
		{
			if( DRAMImage[ wAddr ] != 0L )
				DEBUGP(("0x%08lx: Opera %08lx != CL450 %08lx\n",bAddr,DRAMImage[ wAddr ],codeWord));
		}
	}
}

#endif

