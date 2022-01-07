/*
	File:		AV110.h

	Contains:	xxx put contents here xxx

	Written by:	George Mitsuoka

	Copyright:	© 1993 by 3DO, Inc., all rights reserved.

	Change History (most recent first):

		<13>	 9/12/94	DM		Added 48kHz function prototype and corrected the PCM defines for
									AV110.
		<12>	 8/26/94	DM		Added ti version address
		<11>	 8/24/94	DM		Added separate analog reset routine
		<10>	 8/10/94	GM		Fixed definitions of high byte interrupt bits.
		 <9>	 4/25/94	BCK		Added PTS interrupt control routines.
		 <8>	 4/19/94	BCK		Added comment not to use the Repeat register.
		 <7>	  4/5/94	BCK		Removed invisble characters introduced during the check-in
									process.
		 <6>	  4/5/94	BCK		Added FMVReadAV110Register(register) to mask the register reads
									to 8 bits. Added FMVAV110ReadPTS() to read the PTS value.
		 <5>	  3/5/94	GM		Corrected PATCH_SIZE definition.
		 <4>	  3/5/94	GM		Added PATCH and PATCH size #defines.
		 <3>	11/22/93	GDW		Added audio bypass mode.
		 <2>	11/19/93	GDW		Added additional functions for additional stream support.
*/

/* file: AV110.h */
/* TI TMS320AV110 MPEG Audio decoder definitions */
/* 4/23/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#ifndef AV110_HEADER
#define AV110_HEADER

/* register number definitions */
#define	TIVERSION		0x6D		/* version number of the chip */
									/* chips prior to version 2.2 are 0x00 */
									/* version 2.2 will be 0x22  and so on */
#define ANC0			0x06L		/* ancillary data low byte */
#define ANC1			0x07L
#define ANC2			0x08L
#define ANC3			0x09L		/* ancillary data high byte */
#define ANC_AV			0x6cL		/* available ancillary data count */
#define ATTEN_L			0x1eL		/* left channel attenuator */
#define ATTEN_R			0x20L		/* right channel attenuator */
#define AUD_ID			0x22L		/* audio stream identifier */
#define AUD_ID_EN		0x24L		/* audio stream id enable */
#define BALE_LIMH		0x69L		/* buffer almost empty limit high bits */
#define BALE_LIML		0x68L		/* buffer almost empty limit low byte */
#define BALF_LIMH		0x6bL		/* buffer almost full limit high bits */
#define BALF_LIML		0x6aL		/* buffer almost full limit low byte */
#define BUFFH			0x13L		/* input buffer word counter high bits */
#define BUFFL			0x12L		/* input buffer word counter low byte */
#define CRC_ECM			0x2aL		/* CRC error handling */
#define DATAIN			0x18L		/* audio data input register */
#define DIF				0x6fL
#define DMPH			0x46L		/* de-emphasis mode */
#define DRAM_EXT		0x3eL		/* memory status */
#define EOS				0x3aL		/* end of stream */
#define FREE_FORMH		0x15L		/* free format frame length high bits */
#define FREE_FORML		0x14L		/* free format frame length low byte */
#define HEADER0			0x5eL		/* frame header low byte */
#define HEADER1			0x5fL
#define HEADER2			0x60L
#define HEADER3			0x61L		/* frame header high byte */
#define INTRH			0x1bL		/* interrupt request register high byte */
#define INTRL			0x1aL		/* interrupt request register low byte */
#define INTR_ENH		0x1dL		/* interrupt enable register high byte */
#define INTR_ENL		0x1cL		/* interrupt enable register low byte */
#define IRC0			0x78L		/* internal reference clock low byte */
#define IRC1			0x79L
#define IRC2			0x7aL
#define IRC3			0x7bL
#define IRC4			0x7cL		/* internal reference clock high bit */
#define IRC_LOAD		0x7eL		/* load internal reference clock */
#define LATENCY			0x3cL		/* decoder latency selection */
#define MUTE			0x30L		/* muting */
#define PATCH			0x52L		/* microcode patch register */
#define PCM_DIV			0x6eL		/* pcm clock divider value low byte */
#define PCM_18			0x16L		/* output precision select */
#define PCM_FS			0x44L		/* output sampling rate */
#define PCM_ORD			0x38L		/* output order select */
#define PLAY			0x2eL		/* output decoded audio data */
#define APTS0			0x62L		/* presentation time stamp low byte */
#define APTS1			0x63L
#define APTS2			0x64L
#define APTS3			0x65L
#define APTS4			0x66L		/* presentation time stamp high bit */
#define REPEAT			0x34L		/* repeat next audio frame - NOT VALID WITHOUT EXTERNAL DRAM */
#define RESET			0x40L		/* reset decoder */
#define RESTART			0x42L		/* flush data buffers */
#define SRC0			0x72L		/* system reference clock low byte */
#define SRC1			0x73L
#define SRC2			0x74L
#define SRC3			0x75L
#define SRC4			0x76L		/* system reference clock high bit */
#define SIN_EN			0x70L		/* serial input enable */
#define SKIP			0x32L		/* skip next audio frame */
#define STR_SEL			0x36L		/* input stream configuration */
#define SYNC_ECM		0x2cL		/* synchronization error handling */
#define SYNC_LOCK		0x28L		/* good sync word count */
#define SYNC_ST			0x26L		/* synchronization status */

/* register value definitions */

#define ATTEN_MSK		0x3fL		/* attenuation register mask */
#define ENABLE			0x01L		/* enable various features */
#define DISABLE			0x00L		/* disable various features */
#define ERR_MUTE		0x01L		/* mute on error */
#define ERR_REPEAT		0x02L		/* repeat last frame on error */
#define ERR_SKIP		0x03L		/* skip invalid frame on error */
#define DEEMPH_DLY		0x01L		/* 5/15 µsec deemphasis mode */
#define DEEMPH_J17		0x03L		/* CCITT J.17 deemphasis */
#define PCM32			0x02L		/* 32 Khz sampling rate */
#define PCM44			0x00L		/* 44.1 Khz sampling rate */
#define PCM48			0x01L		/* 48 Khz sampling rate */
#define MSBFIRST		0x00L		/* output most significant bit first */
#define LSBFIRST		0x01L		/* output least significant bit first */
#define PARALLELINPUT	0x00L		/* parallel input select */
#define SERIALINPUT		0x01L		/* serial input select */
#define STREAM_AUDIO	0x00L		/* MPEG audio stream input */
#define STREAM_PACKET	0x01L		/* MPEG packet stream input */
#define STREAM_SYSTEM	0x02L		/* MPEG system stream input */
#define STREAM_BYPASS	0x03L		/* audio bypass */
#define SYNC_UNLOCKED	0x00L		/* synchronization status unlocked */
#define SYNC_RECOVER	0x02L		/* attempting to recover lost sync */
#define SYNC_LOCKED		0x03L		/* sync locked */

/* interrupt registers bits */

/* in high byte */
#define INT_EOS_FOUND	0x1000L				/* set when eos found */
#define INT_SRC_DETECT	0x0800L				/* set when src detected */
#define INT_DEEMPH_MOD	0x0400L				/* set when de-emphasis is changed */
#define INT_FS_MOD		0x0200L				/* set when sampling frequency is changed */
#define INT_OUT_UFLOW	0x0100L				/* set when PCM output buffer underflows */

/* in low byte */
#define INT_ANC_FULL	0x0080L				/* set when ancillary data is full */
#define INT_ANC_AVAIL	0x0040L				/* set when ancillary data is registered */
#define INT_CRC_ERROR	0x0020L				/* set when CRC error is detected */
#define INT_BALF		0x0010L				/* set when input buffer is above BALF limit */
#define INT_BALE		0x0008L				/* set when input buffer is below BALE limit */
#define INT_PTS			0x0004L				/* set when a valid PTS is registered */
#define INT_HEADER		0x0002L				/* set when a valid header is registered */
#define INT_SYNC		0x0001L				/* set when a change in sync status occurs */

/* misc. #defines */

#define PATCH_SIZE		198L				/* number of patch microinstructions */


/* macro definitions */

#define FMVReadAV110Register(register)	(FMVReadAudioRegister(register) & 0xff)

/* function prototypes */

void FMVAnalogResetAV110( void );
void FMVResetAV110( void );
void FMVAV110Init(uint32 dataFormat);
void FMVAV110SetOutputSampleFrequency( uint32 outputFrequency );
void FMVAV110StartDecoder( void );
void FMVAV110StopDecoder( void );
void FMVAV110MuteDecoder( void );
void FMVAV110EnableInputAlmostEmptyInterrupt( void );
void FMVAV110EnableInputAlmostFullInterrupt( void );
void FMVAV110EnableOutputUnderflowInterrupt( void );
void FMVAV110EnableEndOfStreamInterrupt( void );
void FMVAV110EnableValidHeaderInterrupt( void );
void FMVAV110SetupPTSInterrupt( int32 enable );
void FMVAV110InitPTSInterrupt(void);
#if MIABUILD == 1
void FMVAV110EnableFSModInterrupt(void) ;
#endif
void FMVAV110ClearInterrupt(void);
int32 FMVAV110ReadIntStatus( void );
uint32 FMVAV110ReadPTS(int32* highBit);
void FMVAV110SendEOS( void );
void FMVAV110SetAudioStream(void);
void FMVAV110SetPacketStream(void);
void FMVAV110SetSystemStream(void);
void FMVAV110SetByPass(void);


#endif


