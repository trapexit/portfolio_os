/*	$Id: appdigest.c,v 1.35 1994/08/15 22:11:43 markn Exp $
**
**	Disk resident security code
**
**	3DO Confidential -- Contains 3DO Trade Secrets -- internal use only
*/
#if defined(APPDIGEST) && defined(ENCRYPT)
/* Modern disc dipirs (even encrypted) do not use digesting */

#include "types.h"
#include "inthard.h"
#include "clio.h"
#include "discdata.h"
#include "setjmp.h"
#include "rom.h"
#include "dipir.h"

#ifdef macdipir
#include <stdlib.h>
#include <stdio.h>
#define DEBUG2 1
#endif

#if 0
#define DEBUG2 1
#define DIGEST_WHOLE_DISC 1
#define NO_STOP_BAD_DIGEST 1
#endif

#define DEFAULT_MAX_PAIN 15

#ifndef macdipir
extern uint32 ScaledRand (uint32 scale);
extern void srand(unsigned int seed);
#endif

extern uint32 ReadHardwareRandomNumber(void);

extern DeviceRoutines *dvr;
extern DipirRoutines *dipr;

int32 digest_start_block,digest_size_bytes, app_start_block;
int32 MaxPain=DEFAULT_MAX_PAIN;


#define PHYS_BS		(2*1024)
#define LOG_BS		(32*1024)

#define MAX_DIGEST_CHECKS 128

uint32 digest_table[4*MAX_DIGEST_CHECKS];
int32 LastBlock;

/* place where third label would end up */
#define THIRD_LABEL_DIGEST (0x080f)
/* block to ignore so Fatty Bear digest works */
#define FATTY_BEAR_DIGEST (0x306)

#ifdef macdipir
uint32 
ScaledRand(uint32 scale) 
{
	int r = rand();
	uint32 answer = r;
		
	if ( scale > 0 )
		answer = r - scale*(r / scale);
	else
		answer = r;
	return (answer);
}
#endif

static int32 digest_start32, digest_end32;

/* certain 32k blocks need to be excluded;  the digest itself, romtag tables,
   and a block in Fatty Bear 
*/
int32 
UseDigest(int32 block) 
{
    /* exclude the third label */
    if (block == THIRD_LABEL_DIGEST) return 0;
    /* exclude the digest table itself */
    if (block >= digest_start32 && block <= digest_end32) return 0;
#ifdef COMBINE
    if (block == FATTY_BEAR_DIGEST) return 0;
#endif
    return 1;
}

int 
d_memcmp(const char *a, const char *b, int n)
{
	while (n > 0) {
		if (*a++ != *b++) {
			return 1;
		}
		n--;
	}
	return 0;
}

#ifndef DIGEST_WHOLE_DISC

/* 
   Digest 32K Blocks, comparing them to the table of digests passed in.
   The block of digests passed in should be RSA signature checked.
*/

static 
int DigestBlocks(DipirEnv *de, int32 starts[], int32 num_blocks, 
		 uint32 dig_table[]) 
{
	int32 status;
	uint32 startblock;
	int32 dcount = 0;
	int32 block_count;
	int32 bcount = 0;
	unsigned char *digest;
	char *NextBuff;

	PUTS("===Digest Blocks===");
	digest = de->digest;
#ifdef DEBUG2
	PUTS("buffers @");PUTHEX((int)de->databuff1);PUTHEX((int)de->databuff2);
	PUTS("digest @");PUTHEX((int)de->digest);
	PUTS("table @");PUTHEX((int)&digest_table);
#endif

	PUTS("LB=");PUTHEX(LastBlock);
	PUTS("SB=");PUTHEX(starts[bcount]);

	while (num_blocks > 0) {
		/* adjust by de_FirstBlock (should be 150) and convert */
		/* start to 2K block number for actual data read */
		startblock = de->FirstBlock + 
			starts[bcount++] * (LOG_BS/PHYS_BS);
		/* perform sanity check */
		if (startblock > LastBlock-15) {
			PUTS("D-O.O.R.!");
			return 2;
		} else {
			DoubleBuffer(de);
			block_count = (LOG_BS/PHYS_BS);

			ASYNCREADBLOCK(startblock);
			startblock += 1;
			block_count--;
			DIGESTINIT();
			while (block_count) {
				if(WAIT()) {
					PUTS("READ ERR");
					return 0;
				}
				SwitchBuffers(de);
				ASYNCREADBLOCK(startblock);
				startblock += 1;
				block_count--;
				UPDATEDIGEST(NextBuff, PHYS_BS);
			}
			if(WAIT()) {
				PUTS("READ ERR");
				return 0;
			}
			UPDATEDIGEST(de->CurrentBuff, PHYS_BS);
			FINALDIGEST();

			status = d_memcmp((char *)&dig_table[dcount],
						(char *) digest,16);
			if (status) {
				PUTS("D-BAD BK=");PUTHEX(starts[bcount-1]);
#if 0
				PUTS("digest read===");
				for(i=0; i<4; i++) { 
					PUTHEX(*(int32 *)&digest[i*4]);
					PUTS(","); 
				}
				PUTS("===");
				PUTS("digest entry===");
				for(i=0; i<4; i++) { 
					int32 n;
					n = *(int32 *)(&dig_table[dcount+i*4]);
					PUTHEX(n);
					PUTS(","); 
				}
				PUTS("===");
#endif
#ifndef macdipir
				return 0;	/* failed digest check */
#endif
			}
#if 0
			else {PUTS("D-OK BK=");PUTHEX(starts[bcount-1]);}
#endif
		}
		num_blocks--;
		dcount += 4;
	}
	PUTS("DCOMP GOOD");
	return 1;
}


/* take the list of blocks to be digested, pull out the proper digest
   entries.  If flag is zero, do a signature check */

static 
int32 MapBlock(DipirEnv *de,int32 start_table[],uint32 dig_table[],uint32 flag)
{
	int32 digest_block_offset;
	int32 block_count;
	int32 startblock;
	int32 next_digest_block;
	int32 j, status, dcount = 0;
	int32 entry_count = 0;
	uint32 *point;
	char *NextBuff;

        PUTS("MAPBLOCK");

	/* digest_start_block set from romtag */
	startblock = digest_start_block;
	{
		int numdigests = (int)(LastBlock*de->BlockSize)/(32*1024);
		if ((numdigests & (512-1)) == 0)
		{
#if 0
			PUTS("fudging digest size was=");
			PUTHEX(digest_size_bytes);
#endif
			digest_size_bytes += 8192; /* funny */
		}
	}
    	block_count = (digest_size_bytes)/PHYS_BS;

	PUTS("block_count=");PUTHEX(block_count);

	next_digest_block = start_table[dcount]/128;
	digest_block_offset = (start_table[dcount] % 128) * 4;
	dcount++;

	DoubleBuffer(de);
    	ASYNCREADBLOCK(startblock);
    	startblock += 1;

    	DIGESTINIT();
    	while (block_count) {
		if(WAIT()) {
			PUTS("READ ERR");
			return 0;
		}
		SwitchBuffers(de);
		/*
		 * if we're on the last block, don't digest the signature
		 * and don't start a new read
		 */
		if (block_count==1) {
			UPDATEDIGEST(NextBuff, PHYS_BS-SIG_LEN);
		} else {
			/* start the new read */
			ASYNCREADBLOCK(startblock);
			UPDATEDIGEST(NextBuff, PHYS_BS);
		}
		while (startblock-digest_start_block-1 == next_digest_block) {
			/*
			 * we can use point as a convenient data pointer 
			 * to the block read in
			 */
			point = (uint32 *)NextBuff;
			/* copy 16 bytes */
			for(j=0; j<4;j++) {
				dig_table[entry_count++] = 
					point[digest_block_offset++];
			}
			next_digest_block = start_table[dcount]/128;
			digest_block_offset = (start_table[dcount] % 128) * 4;
			dcount++;
		}
		startblock += 1;
		block_count--;
        }
#if 0
    	if(WAIT()) { PUTS("READ ERR"); return 0; }
#endif
    	FINALDIGEST();

	if (!flag)
		status = RSAFINALAPP((POINTER)NextBuff+PHYS_BS-SIG_LEN, SIG_LEN);
	else 
		status = 1;
	PUTS("status=");PUTHEX(status);
	return status;
}

static uint32 HardwareRandomNumber(void)
{
	uint32 rn;
	volatile int32 total;
	int32 count = 10;

#ifdef macdipir
	rn = rand();
#else
	Clio *clio = (Clio *)CLIO;
	rn = clio->clio_DSPPNoise;
#endif
	/* delay for at least 1 usec per bit */
	while (count--)
		total += total + count;
#ifdef macdipir
	srand( clock() );
	return (rn << 16) | rand();
#else
	return (rn << 16) | clio->RandSample;
#endif
}

/* its a very small array */
static void do_sort(int32 starts[], int32 size) 
{
	int32 i,j;
	int32 tmp;

	for (i=0; i<size; i++) {
		for (j=i+1; j<size; j++) {
			if(starts[i] > starts[j]) {
				tmp = starts[i];
				starts[i]=starts[j];
				starts[j]=tmp;	
			}
		}
	}
}

#else /* DIGEST_WHOLE_DISC */

int32 curblocknr=-1;

char *
getMD5Value(DipirEnv *de, int32 blocknr) 
{
	int32 dblocknr = blocknr/(LOG_BS/PHYS_BS);
	int32 tablebase = dblocknr/128;
	int32 tableoffset = dblocknr%128 ;
	
	if (tablebase != curblocknr)
	{

		PUTS("digest_start_block=");PUTHEX(digest_start_block);
		PUTS("Reading digest table block:"); PUTHEX(tablebase);
		SingleBuffer(de, (char *)digest_table);
		curblocknr = tablebase;
		ASYNCREADBLOCK((uint32)(digest_start_block + curblocknr));
		if (WAIT()) { PUTS("READ ERR"); /*return error?*/ }
	}
	return ((char *)digest_table) + tableoffset*16;
}

#endif /* DIGEST_WHOLE_DISC */

int32 
VerifyDiscDigest(DipirEnv *de) 
{
	int32 scale, offset;
	int32 status;
	int32 val;

	PUTS("VerifyDiscDigest");
	if (MaxPain == 0)
		return 1;	/* the min amount of pain (none). */
	if (MaxPain == 1)
		MaxPain = DEFAULT_MAX_PAIN;  /* use the default */
	MaxPain = (MaxPain >= MAX_DIGEST_CHECKS) ? 
			MAX_DIGEST_CHECKS-1 : MaxPain;
	PUTS("MaxPain=");PUTHEX((int)MaxPain);

	/* FirstBlock = digest_start_block + (digest_size_bytes / PHYS_BS)+1; */
	LastBlock = de->DiscLabel->dl_VolumeBlockCount;
	PUTS("Lastbk=");PUTHEX((int)LastBlock);

	/* generate the digest locations on the disc */
	digest_start32 = (digest_start_block - (2 * FRAMEperSEC)) / 
				(LOG_BS/PHYS_BS);
	val = (digest_size_bytes/PHYS_BS) / (LOG_BS/PHYS_BS);
	digest_end32 = digest_start32 + ((val < 12) ? 12 : val);

	/* convert into 32K block range, rather than 2K block range */
	offset = (app_start_block/(LOG_BS/PHYS_BS)) +1;
	scale = (LastBlock - app_start_block) / (LOG_BS/PHYS_BS);
	/* simple sanity check */
	if (scale <= 0)
		scale = LastBlock/(LOG_BS/PHYS_BS);

	PUTS("START32=");PUTHEX(digest_start32);
	PUTS("END32=");PUTHEX(digest_end32);
	PUTS("Maxblk=");PUTHEX((int)scale);
	PUTS("digestsizebytes=");PUTHEX(digest_size_bytes);

#ifndef DIGEST_WHOLE_DISC
	/* seed the random number routine from the hardware */
	srand((int)HardwareRandomNumber());

	/*
	 * generate the random block table
	 * do not check signature file itself (its got a signature anyway),
	 * and the place where the 3rd volume label/romtag would go
	 * because we can't tell if there is a volume label there without
	 * actually looking for the magic number.
	 */
	{
		int32 starts[MAX_DIGEST_CHECKS];
		int32 count;
		int32 j;
		/*
		 * NOTE: this needs to prevent duplicate blocks to check from 
		 * being selected
		 */
		for (count=0; count < MaxPain; count++)
		{
			/*
			 * starts is in in units of 32K blocks
			 * NOTE: if we cannot find 15 blocks to digest, 
			 * the title will appear to just hang...
			 * so a certain minimum size is required.
			 * Currently, we get this min size via the 
			 * signature file
			 */
			do {
				val = ScaledRand(scale) + offset;
			} while (!UseDigest(val));
			starts[count]=val;
		}
		/* Now sort the table to speed access */
		do_sort(starts, MaxPain);
		/* Now clear the rest of the table */
		for (j=MaxPain; j<MAX_DIGEST_CHECKS; j++)
			starts[j]= -1; 

		/*  extract table of digest from this starting point */    
		if (!MapBlock(de,starts,digest_table,0))
		{
			PUTS("BOGUS DIGEST");
			return 0; /* digest itself failed RSA check */
		} else 
			PUTS("DIGEST OK");

		/* now compare the digest extracts with the disc */
		status = DigestBlocks(de, starts, MaxPain, digest_table);
	}
	PUTS("Dig Block status=");PUTHEX((int)status);
#else		/* DIGEST_WHOLE_DISC : CHECK ENTIRE DISC DIGEST */
	{
		int32 startblock;
		int32 block_count;
		int status;
		int digest_status = 0;
		char *NextBuff;
		char *addrOfMD5Value;

		/* Dale Luck's patch for correction to digest_size_bytes */
		{
			int32 numdigests = (LastBlock*de->BlockSize)/(32*1024);
			if ((numdigests & (512-1)) == 0)
			{
#if 0
				PUTS("fudging digest size was=");
				PUTHEX(digest_size_bytes);
#endif 
				digest_size_bytes += 8192; /* funny */
			}
		}
		/* verify entire digest is valid */
		PUTS("+++++ Checking entire disc digest +++++");
		startblock = digest_start_block;
		block_count = (digest_size_bytes)/PHYS_BS-1;
		PUTS("block_count=");PUTHEX(block_count);
		DoubleBuffer(de);
		ASYNCREADBLOCK(startblock);
		startblock += 1;

		DIGESTINIT();
		while (block_count)
		{
			if(WAIT()) {
				PUTS("READ ERR");
				return 0;
			}
			SwitchBuffers(de);
			ASYNCREADBLOCK(startblock);
			UPDATEDIGEST(NextBuff, PHYS_BS);
			startblock += 1;
			block_count--;
		}
		if(WAIT()) {
			PUTS("READ ERR");
			return 0;
		}
		UPDATEDIGEST(de->CurrentBuff, PHYS_BS-SIG_LEN);
		/*
		 * digest block check code goes here; until it does, just
		 * keep digest compare from stuff in last block of digest
		 */
		FINALDIGEST();

		status = RSAFINALAPP((POINTER)de->CurrentBuff+PHYS_BS-SIG_LEN,
					SIG_LEN);
		if (status == 0)
			return status;
		PUTS("DigestTable OK!");

		/* for all 32K blocks compute and compare MD5 signatures */
		PUTS("app_start_block=");PUTHEX(app_start_block);
		startblock = (app_start_block+16) & ~0xf;
		PUTS("startblock=");PUTHEX(startblock);
		PUTS("&digest_table=");PUTHEX(digest_table);
		while (startblock < LastBlock)
		{
			int32 MD5BlkNr;

			MD5BlkNr = startblock / (LOG_BS/PHYS_BS);
			if (!UseDigest(MD5BlkNr)) {
				startblock += (LOG_BS/PHYS_BS);
				continue;   /* this drops us to start of loop again */
			}
			addrOfMD5Value = getMD5Value(de, startblock);
			PUTS("db=");PUTHEX(startblock);
			DoubleBuffer(de);
			block_count = LOG_BS/PHYS_BS;
			ASYNCREADBLOCK((int32)(de->FirstBlock+startblock++));
			block_count--;

			DIGESTINIT();
			while (block_count--) {
				if(WAIT()) {
					PUTS("READ ERR");
					return 0;
				}
				SwitchBuffers(de);
				ASYNCREADBLOCK((int32)(de->FirstBlock+startblock++));
				UPDATEDIGEST(NextBuff, PHYS_BS);
			}
			if(WAIT()) {
				PUTS("READ ERR");
				return 0;
			}
			UPDATEDIGEST(de->CurrentBuff, PHYS_BS);
			FINALDIGEST();

			status = d_memcmp((char *)addrOfMD5Value,
					(char *) de->digest, 16);
			digest_status |= status;
			if (status) {
				PUTS("D-BAD BK=");PUTHEX(MD5BlkNr);
#ifdef DEBUG2
				PUTS("digest read===");
				for(i=0; i<4; i++) { 
					PUTHEX(*(int32 *)&de->digest[i*4]);
					PUTS(","); 
				}
				PUTS("===");
				PUTS("digest entry===");
				for (i=0; i<4; i++) { 
					int32 n;
					n = *(int32 *)(addrOfMD5Value+i*4);
					PUTHEX(n);PUTS(","); 
				}
				PUTS("===");
#endif
#ifndef NO_STOP_BAD_DIGEST
				return 0;	/* failed digest check */
#endif
			}
#ifdef DEBUG2
			else {PUTS("D-OK BK=");PUTHEX(MD5BlkNr);}
#endif
		}
		if (digest_status) {
			PUTS("DIGEST COMPARE BAD");
			return 0;
		}
		PUTS("DIGEST COMPARE GOOD");
		return 1;
	}
#endif /* DIGEST_WHOLE_DISC */
	return status;	
}

#ifdef macdipir
extern DipirEnv lde;

int	ReadDiscInfo (void) {
	}
	
int ReadSessionInfo (void) {
	}
	
int	ReadTOC (int i) {
	}
	
void EjectDisc (void) {	/* Yes, even for FMV cards :-) */
	}
	
void	AsyncReadBlock (int blk) {

    lde.DataExpected = lde.BlockSize;
		macRead( lde.CurrentBuff, blk*lde.BlockSize, lde.DataExpected ); 
	}
	
uint32	GetDriverInfo (void) {	/* Driver ID and version number */
	}
	
int32	 CheckGoldDisc (void) { /* check for gold disc */
	}
int32 GetMfgPlant (void) {   /* returns mfg plant number */
	}

int Wait (void) {
	return false;
	}

int	ReadBlock(int blockno) { /* read a block @blockno */

	AsyncReadBlock(blockno);
  return Wait();
	}
	
static DeviceRoutines dvrdvr =
{
	ReadBlock,
	ReadDiscInfo,
	ReadSessionInfo,
	ReadTOC,
	EjectDisc,
	AsyncReadBlock,
	GetDriverInfo,
	CheckGoldDisc,
	GetMfgPlant,
	Wait,
};

void realputchar( char c ) {
	void macputchar( char c );
	
	macputchar( c );
	}
	
void realputs( char *s ) {
	void macputs( char *s );
	
	macputs( s );
	}
	
void realputhex( long n ) {
	macputhex( n );
	}
	
void revectorDipirRoutines( DipirRoutines *de )
{
	de->putc = (void (*)(char))realputchar;
}

void revectorDriverRoutines( DeviceRoutines *dvre )
{
#if 0
	dvre->ReadBlock
	dvre->ReadDiskInfo
	dvre->ReadSessionInfo
	dvre->ReadTOC
	dvre->EjectDisk
	dvre->AsyncReadBlock
	dvre->GetDriverInfo
	dvre->CheckGoldDisk
	dvre->GetMfgPlant
	dvre->WaitReadBlock
#endif
}

main()
{
	extern DipirRoutines dr;
	int err;
	
	InitializeMac();
	
	/* initialize our device routines */
	
	lde.DipirRoutines = &dr;
	lde.dvr = &dvrdvr;
	lde.BlockSize = PHYS_BS;
	lde.DiscLabel->dl_VolumeBlockCount = 40960;	/* PutPut JP */
	lde.DiscLabel->dl_VolumeBlockCount = 171520;	/* itsabirdslife */
	lde.DiscLabel->dl_VolumeBlockCount = 40960;	/* Fatty's Bear */
	/* reading from a cdrom.image.  no two second offset necessary */
	lde.FirstBlock = 0;
	
	app_start_block = 608; /* that's where Bill starts digesting */
	app_start_block = 170000; /* itsabirdslife */
	app_start_block = 40960-2048; /* Fatty's Bear */
	
	digest_start_block = 26396+1; /* PutPut JP */
	digest_size_bytes	= 40960;  /* PutPut JP */
	digest_start_block = 146056+1; /* itsabirdslife */
	digest_size_bytes	= 172032;  /* itsabirdslife */
	digest_start_block = 23515+1; /* Fatty's Bear */
	digest_size_bytes	= 40960;  /* Fatty's Bear */
	
  /* initialize the globals so the macros work */
	
	dipr = lde.DipirRoutines;
	dvr = lde.dvr;
	
	dipr->putc = (void (*)(char))realputchar;
	dipr->puthex = (void (*)(int))realputhex;
	dipr->puts = (void (*)(char *))realputs;
	dipr->Wait = Wait;

	err = RSAInit();
	VerifyDiscDigest( &lde );
	exit(1);
}
#endif

#else /* APPDIGEST && ENCRYPT */

int AppDigestDummy(void) { return 0; }

#endif /* APPDIGEST && ENCRYPT */
