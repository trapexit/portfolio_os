/* $Id: dspp.h,v 1.49 1994/09/27 08:59:07 phil Exp $ */
#pragma include_only_once
#ifndef _dspp_h
#define _dspp_h
/*
** DSP Includes
** By:  Phil Burk
*/

/*
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

/*
** 921112 PLB Remove OUTPUT RSRC cuz basically just IMEM
** 930210 PLB Add ID_AIFC
** 930215 PLB Moved PRT(x) to audio_internal.h
** 931216 PLB Corrected LAST_EI_MEM to 0x6F, added IsDMATypeInput/Output
** 940228 PLB Added CALC_SHIFT_DIVIDE_SR
** 940608 PLB Added proto for DSPPAdjustTicksPerFrame()
** 940609 PLB Added dtmp_LibraryTemplateRefs
** 940811 PLB Added ezmem_tools.h and table_alloc.h
*/

#include "iff_fs_tools.h"
#include "ezmem_tools.h"
#include "table_alloc.h"
#include "dspp_touch.h"

/***********************************************************************
** This group of structures matches the contents of the OFX file
** so they cannot be changed without changing the file format.
*/
typedef struct DSPPResource /* Warning - must match 3INS file format. */
{
	int32  drsc_Type;
	int32  drsc_Many;
	uint32 drsc_Allocated;    /* Filled in when allocated */
	int32  drsc_References;   /* Count of internal and exported references */
} DSPPResource;

typedef struct DSPPRelocation  /* Warning - must match 3INS file format. */
{
	uint8  drlc_Attribute;  /* of resource */
	uint8  drlc_Flags;
	uint8  drlc_Width;      /* of field to update */
	uint8  drlc_Bit;        /* bit position of LSB of field*/
	int32  drlc_CodeIndex;  /* Index of subchunk in DCOD */
	int32  drlc_RsrcIndex;  /* Index of entry in Resource Chunk */
	int32  drlc_CodeOffset; /* Offset within subchunk */
} DSPPRelocation;

#define DINI_AT_ALLOC (0x0001)
#define DINI_AT_START (0x0002)

typedef struct DSPPDataInitializer  /* Warning - must match 3INS file format. */
{
	int32  dini_Many;       /* How many values, <= drsc_Many */
	int32  dini_Flags;
	int32  dini_RsrcIndex;  /* Index of entry in Resource Chunk */
	int32  dini_Reserved;   /* Set to zero. */
} DSPPDataInitializer;

typedef struct DSPPHeader  /* Warning - must match 3INS file format. */
{
	int32 dhdr_FunctionID; /* used to identify functionality if name changed */
	int32 dhdr_SiliconVersion;
	int32 dhdr_Reserved1;
	int32 dhdr_Reserved2;
} DSPPHeader;

typedef struct DSPPCodeHeader  /* Warning - must match 3INS file format. */
{
	int32	dcod_Type;
	int32	dcod_Offset;
	int32	dcod_Size;
} DSPPCodeHeader;

typedef struct DSPPKnobResource  /* Warning - must match 3INS file format. */
{
	int32  dknr_RsrcIndex;
	int32  dknr_CalcType;
	int32  dknr_Data1;
	int32  dknr_Data2;
} DSPPKnobResource;

typedef struct DSPPKnob  /* Warning - must match 3INS file format. */
{
	int32  dknb_Next;   /* offset within chunk of next knob */
	int32  dknb_Min;
	int32  dknb_Max;
	int32  dknb_Default;
	int32  dknb_NumResources;
	char   dknb_Name[AF_MAX_NAME_SIZE];
	DSPPKnobResource dknb_Resources[1];   /* variable size cuz of these */
} DSPPKnob;

/*******************************************************************/

typedef struct
{
	int32           dtmp_ShareCount;     /* This can be shared. Last one frees it. */
	int32           dtmp_NumResources;
	DSPPResource   *dtmp_Resources;
	char           *dtmp_ResourceNames;
	int32           dtmp_NumRelocations;
	DSPPRelocation *dtmp_Relocations;
	int32           dtmp_NumKnobs;
	DSPPKnob       *dtmp_Knobs;
	int32           dtmp_CodeSize;
	DSPPCodeHeader *dtmp_Codes;
	int32           dtmp_DataInitializerSize;
	DSPPDataInitializer *dtmp_DataInitializer;
	int32			dtmp_FunctionID;
	List			dtmp_LibraryTemplateRefs; /* List of references to DSPS library templates. */
} DSPPTemplate;

typedef struct FIFOControl
{
	int32            fico_RsrcIndex;
	AudioAttachment *fico_CurrentAttachment;   /* Which sample is running. */
	List             fico_Attachments;
} FIFOControl;

typedef struct DSPPInstrument
{
	Node          dins_Node;
	uint8         dins_ActivityLevel;
	uint8         dins_RateShift; /* execution rate shifted right 940811 */
	uint8         dins_ExecFrame; /* if executing at half rate, which frame? 0=even */
	uint8         dins_Specialness;  /* Head, Tail or SplitExec */
	DSPPTemplate *dins_Template;
	int32         dins_NumResources;
	DSPPResource *dins_Resources;
	int32         dins_NumFIFOs;
	FIFOControl  *dins_FIFOControls;     /* Contains references to attached samples. */
	int32         dins_EntryPoint;
	int32         dins_DSPPCodeSize;
/* Two standard knobs for StartInstrument */
	AudioKnob     dins_FreqKnob;
	AudioKnob     dins_AmplitudeKnob;
	List          dins_EnvelopeAttachments;  /* List of direct attachments. */
} DSPPInstrument;

typedef struct
{
	Node	dext_Node;
	DSPPResource	*dext_Resource;
} DSPPExternal;

typedef struct
{
	uint32 alct_Next; /* next value to allocate %Q just sequential allocation */
	uint32 alct_TotalAvail;  /* total number available */
	uint32 alct_Offset;  /* add this to allocated value */
} Allocator;


/*
** When we have half rate calculations, we must allocate
** each odd/even frame separately.  If we change this from 2 we
** will have to change lots of code as well!
*/
#define DSPP_NUM_TICK_FRAMES (2)
typedef struct DSPPCodeList
{
	DSPPInstrument *dcls_ListHead;
	DSPPInstrument *dcls_ListTail;
	List dcls_InsList;
} DSPPCodeList;

typedef struct DSPPStatic
{
	DSPPCodeList  dspp_ActiveInstruments; /* 1/1 running DSPPInstruments.  Only used to link code. */
	DSPPCodeList  dspp_HalfRateInstruments[DSPP_NUM_TICK_FRAMES]; /* 1/2 rate DSPPInstruments.  Only used to link code. */
	List  dspp_ExternalList;
	int32 dspp_AvailableAmplitude;
	int32 dspp_AvailableTicks[DSPP_NUM_TICK_FRAMES];  /* Ticks currently unused. */
	int32 dspp_SampleRate;
	int32 dspp_MemWriteEI;   /* EI DSPP address of MemWrite Knob */
	int32 dspp_MemReadEI;    /* EI DSPP address of MemRead Knob */
	int32 dspp_IMEMAccessOn; /* True if access mechanism initialised. */
	int32 dspp_TicksPerFrame;      /* Maximum ticks that can be executed per frame. */
} DSPPStatic;

/**********************************************************************/

#define DSPP_AVAIL_TICKS (565)

#define DCOD_RUN_DSPP 0
#define DCOD_INIT_DSPP 1
#define DCOD_ARGS 2

/* Attributes */
#define DRSC_FIFO_NORMAL 0
#define DRSC_FIFO_STATUS 1
#define DRSC_FIFO_READ   2

#define DRSC_IMPORT 0x8000
#define DRSC_EXPORT 0x4000

#define CALC_NOP 0
#define CALC_LINEAR 1   /* ax+b */
#define CALC_SCALE  2   /* ax/b */
#define CALC_DIVIDE_SR  3   /* divide by sample rate */
#define CALC_SHIFT_DIVIDE_SR  4   /* shift then divide by sample rate 940228 */

#define DRLC_ADD 1
#define DRLC_LIST 2
#define DRLC_32 4

/* This section determines the resources available in the system
** and their starting locations, etc.  Preallocation is included.
*/

/* ======================== ANVIL ==================== */
#ifdef ASIC_ANVIL
#define FIRST_N_MEM    0x000
#define LAST_N_MEM     0x1FF
#define OFFSET_N_MEM   0x000

#define FIRST_EI_MEM   0x008
#define LAST_EI_MEM    0x06F    /* 931216 was 0x70 */
#define OFFSET_EI_MEM  0x000

#define FIRST_I_MEM    0x008
#define LAST_I_MEM     0x0FF
#define OFFSET_I_MEM   0x100

#define FIRST_EO_MEM   0x004
#define LAST_EO_MEM    0x00F
#define OFFSET_EO_MEM  0x300

#define FIRST_IN_FIFO  0x000
#define LAST_IN_FIFO   0x00C
#define OFFSET_IN_FIFO 0x000

#define FIRST_OUT_FIFO 0x000
#define LAST_OUT_FIFO  0x003
#define OFFSET_OUT_FIFO 0x000
#endif

/* ------------------------ ANVIL  -------------------- */

/* ======================== BULLDOG ==================== */
#ifdef ASIC_BULLDOG
/* For now, use same table allocation scheme.
** Simply reserve some area as EI, some as I and some as EO.
** Does not take advantage of flat memory yet. %Q
*/
#define FIRST_N_MEM    0x000
#define LAST_N_MEM     0x3FF  /* 1K */
#define OFFSET_N_MEM   0x000

/*  Bulldog Memory Map, temporary
**  000-007 : Reserved EI
**  008-0FF : EI
**  100-107 : Reserved I
**  108-2EF : I 
**  2F0-2F3 : Reserved EO
**  2F4-2FF : EO
**  300-37F : FIFO_OSC
*/
#define FIRST_EI_MEM   0x008
#define LAST_EI_MEM    0x0FF  /* 248 */
#define OFFSET_EI_MEM  0x000

#define FIRST_I_MEM    0x008
#define LAST_I_MEM     0x1EF
#define OFFSET_I_MEM   0x100

#define FIRST_EO_MEM   0x004
#define LAST_EO_MEM    0x00F
#define OFFSET_EO_MEM  0x2F0

/* Allocate 8 as input FIFOs and 8 as output FIFOs %Q */
#define FIRST_IN_FIFO  0x000
#define LAST_IN_FIFO   0x017
#define OFFSET_IN_FIFO 0x000

#define FIRST_OUT_FIFO 0x000
#define LAST_OUT_FIFO  0x007
#define OFFSET_OUT_FIFO 0x018
#endif
/* ------------------------ BULLDOG -------------------- */

#define EI_IFSCALEOUTPUT 0x0000 /* These must match dspp_map.j in Forth */
#define EI_OUTPUTSCALAR  0x0001
#define EO_BENCHMARK   0x000  /* These must match dspp_map.j in Forth */
#define EO_MAXTICKS    0x001
#define EO_FRAMECOUNT  0x002


/* Chunk types of OFX file */

#define	ID_3INS			MAKE_ID('3','I','N','S')
#define	ID_DSPP			MAKE_ID('D','S','P','P')
#define	ID_DSPS			MAKE_ID('D','S','P','S')
#define	ID_NAME			MAKE_ID('N','A','M','E')
#define	ID_ATSM			MAKE_ID('A','T','S','M')
#define	ID_HOOK			MAKE_ID('H','O','O','K')
#define	ID_DCOD			MAKE_ID('D','C','O','D')
#define	ID_DRSC			MAKE_ID('D','R','S','C')
#define	ID_DRLC			MAKE_ID('D','R','L','C')
#define	ID_DINI			MAKE_ID('D','I','N','I')
#define	ID_DKNB			MAKE_ID('D','K','N','B')
#define	ID_DNMS			MAKE_ID('D','N','M','S')
#define	ID_DHDR			MAKE_ID('D','H','D','R')
#define	ID_AIFF			MAKE_ID('A','I','F','F')
#define	ID_SSND			MAKE_ID('S','S','N','D')
#define	ID_COMM			MAKE_ID('C','O','M','M')
#define	ID_MARK			MAKE_ID('M','A','R','K')
#define	ID_INST			MAKE_ID('I','N','S','T')
#define	ID_APPL			MAKE_ID('A','P','P','L')
#define	ID_AIFC			MAKE_ID('A','I','F','C')
#define	ID_FVER			MAKE_ID('F','V','E','R')
#define	ID_ATNV			MAKE_ID('A','T','N','V')
#define	ID_ENVL			MAKE_ID('E','N','V','L')
#define	ID_ENVH			MAKE_ID('E','N','V','H')
#define	ID_PNTS			MAKE_ID('P','N','T','S')
#define	ID_CRDC			MAKE_ID('C','R','D','C')

#define FREEVAR(ptr) if(ptr != NULL) \
	{ EZMemFree(ptr); \
		ptr = 0; \
	}


#define DSPP_WRITE_KNOB_NAME "IMemWriteAddr"
#define DSPP_READ_KNOB_NAME  "IMemReadAddr"

/* Macros *********************************************************/
#define CLIP(n,lo,hi) if (n<lo) { n=lo; } else { if (n>hi) n=hi; }

/*Data Structures *************************************************/

extern DSPPStatic DSPPData;
extern struct TableAllocator DSPPAllocators[];
extern short *Silence;
extern short *ScratchRAM;
extern Item gTailInstrumentItem;

/* Prototypes ****************************************************/

void DSPPJumpTo ( DSPPInstrument *FromIns, DSPPInstrument *ToIns );
void DSPPJumpFromSplitTo ( DSPPInstrument *SplitIns, DSPPInstrument *ToIns );

void PatchDSPPCode( int32 NAddr, int32 Opcode );
void DumpList ( List *theList );
void PatchCode16(int32 Value, int32 indx, uint16 *ar );
void DSPP_SilenceDMA( int32 DMAChan );


DSPPKnob *DSPPNextKnob( DSPPKnob *base, DSPPKnob *dknb );
DSPPResource *DSPPFindResource( DSPPInstrument *dins, int32 RsrcType,char *Name);
DSPPResource *DSPPFindRsrcType( DSPPInstrument *dins, int32 RsrcType,int32 *NextNum);
DSPPTemplate *DSPPCloneTemplate (AudioInsTemplate *aitp, DSPPTemplate *UserTmp);
FIFOControl *DSPPFindFIFO( DSPPInstrument *dins, char *Name);
char  *DSPPGetRsrcName(DSPPInstrument *dins, int32 indx);
int32  AllocateThing ( unsigned char *AllocTable, int32 Size);
int32  BitAllocate (Allocator *alct, int32 Many, uint32 *Allocated);
int32  DSPPAdjustTicksPerFrame( int32 SampleRate );
int32  DSPPAllocInstrument( DSPPTemplate *dtmp, DSPPInstrument **DinsPtr, int32 RateShift);
int32  DSPPAllocateResource(DSPPInstrument *dins, int32 indx);
int32  DSPPAttachEnvelope( DSPPInstrument *dins, AudioEnvelope *aenv, AudioAttachment *aatt);
int32  DSPPAttachKnob(AudioKnob *aknob, DSPPInstrument *dins, char *knobname);
int32  DSPPAttachSample( DSPPInstrument *dins, AudioSample *samp, AudioAttachment *aatt);
int32  DSPPConnectInstruments ( DSPPInstrument *dins_src, char *name_src, DSPPInstrument *dins_dst, char *name_dst);
int32  DSPPDetachKnob( AudioKnob *aknob );
int32  DSPPDetachSample( DSPPInstrument *dins, AudioSample *samp, char *FIFOName);
int32  DSPPDisconnectInstruments ( DSPPInstrument *dins_src, char *name_src, DSPPInstrument *dins_dst, char *name_dst);
int32  DSPPExportResource( DSPPResource *drsc, char *name);
int32  DSPPFreeInstrument( DSPPInstrument *dins, int32 IfOwned );
int32  DSPPFreeResource(DSPPInstrument *dins, int32 indx);
int32  DSPPHandleInsChunk ( iff_control *iffc, int32 ChunkType , int32 ChunkSize );
int32  DSPPImportResource( char *name, uint32 *Allocated);
int32  DSPPInitInsMemory( DSPPInstrument *dins, int32 AT_Mask );
int32  DSPPLoadInsTemplate( DSPPTemplate *dtp, char *filename);
int32  DSPPPauseInstrument( DSPPInstrument *dins );
int32  DSPPPutKnob( DSPPInstrument *dins, DSPPKnob *dknb, int32 val, int32 *NewValPtr, int32 IfConvert);
int32  DSPPReleaseAttachment( AudioAttachment *aatt);
int32  DSPPReleaseEnvAttachment( AudioAttachment *aatt );
int32  DSPPReleaseInstrument( DSPPInstrument *dins, TagArg *args );
int32  DSPPReleaseSampleAttachment ( AudioAttachment *aatt);
int32  DSPPRelocate( DSPPRelocation *drlc, int32 Value, DSPPCodeHeader *Codes, void (*PutFunc)());
int32  DSPPRelocateAll( DSPPInstrument *dins, DSPPCodeHeader *Codes);
int32  DSPPResumeInstrument( DSPPInstrument *dins );
int32  DSPPStartEnvAttachment( AudioAttachment *aatt );
int32  DSPPStartInstrument( AudioInstrument *ains, TagArg *args );
int32  DSPPStartSampleAttachment ( AudioAttachment *aatt, int32 IfFullStart );
int32  DSPPStopEnvAttachment( AudioAttachment *aatt );
int32  DSPPStopInstrument( DSPPInstrument *dins, TagArg *args);
int32  DSPPStopSampleAttachment( AudioAttachment *aatt);
int32  DSPPUnImportResource( char *name );
int32  DSPPUnloadTemplate( DSPPTemplate *dtp );
int32  DSPPValidateTemplate( DSPPTemplate *UserTmp );
int32  DSPPVerifyDataIitializer( DSPPTemplate *dtmp );
int32  DSPPVerifyDataInitializer( DSPPTemplate *dtmp );
int32  DSPP_AccessIMem( int32 WriteAddr, int32 WriteValue, int32 ReadAddr, int32 *ReadValuePtr );
int32  DSPP_AllocFIFO (int32 InOrOut);
int32  DSPP_FreeFIFO (int32 DMAChan);
int32  DSPP_Init( void );
int32  DSPP_InitIMemAccess( void );
int32  DSPP_Init_Touch( void );
int32  DSPP_LoadEmu( char *filename );
int32  DSPP_StartWaveform (AudioSample *SamplePtr, int32 DMAChan);
int32  DSPP_Term( void );
int32  DSPP_Term_Touch( void );
int32  FreeThing (Allocator *alct, int32 Many, uint32 Allocated);
int32  internalLoadTemplate( DSPPTemplate *dtp, iff_control *iffc);
int32  DSPPLoadPatch( DSPPTemplate *dtmp, DSPPInstrument **DInsPtr, int32 RateShift);
uint32 CvtByteToFrame(uint32 Byte, AudioSample *asmp);
uint32 CvtFrameToByte(uint32 Frame, AudioSample *asmp);
uint32 GetRsrcAttribute(DSPPResource *drsc, int32 Attribute);
void   DSPPDownloadCode(ushort *Image, int32 Entry, int32 CodeSize);
void   DSPP_SetDMA (int32 DMAChan, int32 *Addr, int32 Length);
void   DSPP_SetDMANext (int32 DMAChan, int32 *Addr, int32 Length);
void   DSPP_SetDMAQuick (int32 DMAChan, SampleDMA *sdma);
void   DSPP_SetFullDMA (int32 DMAChan, int32 *Addr, int32 Cnt, int32 *NextAddr, int32 NextCnt );
void   DSPP_SilenceNextDMA( int32 DMAChan );
void   DSPP_StopDMA(int32 DMAChan);
void   dsphDisableDMA( int32 DMAChan );
void   dsphHalt( void );
void   dsphInitDMA( void );
void   dsphReset( void );
void   dsphResetAllFIFOs( void );
void   dsphResetFIFO( int32 DMAChan );
void   dsphSetDMANext (int32 DMAChan, int32 *NextAddr, int32 NextCnt);
void   dsphSetFullDMA (int32 DMAChan, int32 *Addr, int32 Cnt, int32 *NextAddr, int32 NextCnt );
void   dsphStart( void );
void   dsphWriteCodeMem( int32 CodeAddr, int32 Value );
void   dsphWriteEIMem( int32 DSPPAddr, int32 Value );
void   dsphDisableChannelInterrupt( int32 DMNAChan );
void   dsphEnableChannelInterrupt( int32 DMNAChan );
int32  dsphReadEOMem( int32 ReadAddr, int32 *ValuePtr );
int32  dsphReadIMem( int32 ReadAddr, int32 *ValuePtr );
uint16 dsphGetAudioFrameCount( void );
int32  dsphGetAudioCyclesUsed( void );

#endif
