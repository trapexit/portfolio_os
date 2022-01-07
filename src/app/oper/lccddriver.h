/*
$Id: lccddriver.h,v 1.9 1995/02/08 22:10:43 bungee Exp $

$Log: lccddriver.h,v $
 * Revision 1.9  1995/02/08  22:10:43  bungee
 * Modified to use +/- 1% for variable pitch (to match MKE), instead of 5%.
 * Also modified status byte that is returned in the clamshell case when it's
 * stuck shut (and there was previously a disc present).  If we're playing and
 * then try to eject the disc in s/w (ie, CDROMCMD_OPEN_DRAWER), and we land
 * in the stuck state (ie, clamshell), we now clear the DiscIn bit in the
 * status byte.
 *
 * Revision 1.8  1994/12/15  01:45:48  bungee
 * Added TOC retry support.
 *
 * Revision 1.7  1994/12/08  19:41:41  bungee
 * Added CD-i support.
 *
 * Revision 1.6  1994/11/17  20:30:38  bungee
 * Added support for new system errors, ER_NoHardware and ER_Kr_TaskKilled.
 *
 * Revision 1.5  1994/11/17  04:15:34  bungee
 * Added DEVELOPMENT ifdef for .dbgdata field support.
 *
 * Revision 1.4  1994/11/09  23:58:45  bungee
 * Updated killing-daemon support.  Updated clamshell support.  Updated
 * DiscData() to use less stack; and removed potential bug.
 *
 * Revision 1.3  1994/11/02  20:39:08  bungee
 * Removed DEBUG.
 *
 * Revision 1.2  1994/11/02  19:18:39  limes
 * Formatting changes: "//" comments gone (for unifdef) and
 * tabs changed from 4-column to 8-column (for other tools).
 *
 * Revision 1.1	 1994/11/02  13:16:24  bungee
 * Initial revision
 *

    File:	LCCDDriver.h

    Contains:	Defines, enums, etc. for LCCD CD-ROM driver.

    NOTE:   This is header is >INTERNAL ONLY< and should only be referenced
	    by the LCCD CD-ROM driver.	All applications needing to use the
	    cd-rom driver API should include "cdrom.h"
*/

#ifndef LCCD_H
#define LCCD_H

/* device id stuff */
#define LCCD_MAX_NUM_DEV	    1
#define LCCD_MANU_ID		    0x1000
#define LCCD_MANU_DEV_NUM	    0x0050

/* LCCD Firmware Commands */
/* (overhead) */
#define CD_LED			    0x01
/* (format) */
#define CD_SETSPEED		    0x04
#define CD_SECTORFORMAT		    0x06
#define CD_SENDBYTES		    0x07
/* (transport) */
#define CD_PPSO			    0x08
#define CD_SEEK			    0x09
/* (reporting) */
#define CD_CIPREPORTEN		    0x0B
#define CD_QREPORTEN		    0x0C
#define CD_SWREPORTEN		    0x0D
#define CD_CIPREPORT		    0x1B
#define CD_QREPORT		    0x1C
#define CD_SWREPORT		    0x1D
/* (unused) */
#define CD_SPINDOWNTIME		    0x05
#define CD_SENDBYTES		    0x07
#define CD_CHECKWO		    0x0A
#define CD_TRISTATE		    0x0F
#define CD_READROM		    0x20
#define CD_MECHTYPE		    0x21

enum CIPStates {
    CIPNone = 0xFF,	/* Invalid CIPState (Only Defined In Driver) */
    CIPOpen = 0,	/* 0 */
    CIPStop,		/* 1 (this is stable in drawer, transient in clamshell mechanism) */
    CIPPause,		/* 2 */
    CIPPlay,		/* 3 */
    CIPOpening,		/* 4 */
    CIPStuck,		/* 5 */
    CIPClosing,		/* 6 */
    CIPStopAndFocused,	/* 7 (this is stable in clamshell, transient in drawer mechanism) */
    CIPStopping,	/* 8 */
    CIPFocusing,	/* 9 */
    CIPFocusError,	/* a */
    CIPSpinningUp,	/* b */
    CIPUnreadable,	/* c */
    CIPSeeking,		/* d */
    CIPSeekFailure,	/* e */
    CIPLatency		/* f */
};

/* dma, firq stuff */
#define AnvilFeatureReg		    ((Madam *)MADAM)->AnvilFeature
#define DMAZeroCurPtr		    ((Madam *)MADAM)->CD0ToRam.Address
#define DMAZeroCurLen		    ((Madam *)MADAM)->CD0ToRam.Length
#define DMAZeroNextPtr		    ((Madam *)MADAM)->CD0ToRam.NextAddress
#define DMAZeroNextLen		    ((Madam *)MADAM)->CD0ToRam.NextLength
#define DMAOneCurPtr		    ((Madam *)MADAM)->CD1ToRam.Address
#define DMAOneCurLen		    ((Madam *)MADAM)->CD1ToRam.Length
#define DMAOneNextPtr		    ((Madam *)MADAM)->CD1ToRam.NextAddress
#define DMAOneNextLen		    ((Madam *)MADAM)->CD1ToRam.NextLength

#define CH0_DMA_FIRQ_PRIORITY	    201
#define CH1_DMA_FIRQ_PRIORITY	    200

enum DMAChannelsEtc {
    DMA_CH0 = 0x01,
    DMA_CH1 = 0x02,
    CUR_DMA = 0x04,
    NEXT_DMA = 0x08
};

/* internal buffer stuff */
#define MAX_NUM_SNARF_BLKS	    50
#define MIN_NUM_DATA_BLKS	    6
#define MAX_NUM_DATA_BLKS	    19
/* NOTE: Subcode blk count must be at least 3 blks more than data blk count...AND must be an even number */
#define MIN_NUM_SUBCODE_BLKS	    10
#define MAX_NUM_SUBCODE_BLKS	    22
#define PREFETCH_HIGH_WATER_MARK    0

#define kExtendedSpaceSize	    32768
#define kStandardSpaceSize	    16384
#define kSubcodeBufSizePlusSome	    2160L
#define kDataBlkSize		    2352L
#define kSubcodeBlkSize		    98L
#define kHighWaterBufferZone	    3
#define kLCCDMaxStatusLength	    12
#define kDataTrackTOCEntry	    0x04

/* speed & pitch settings (we use +/- 1% of normal for fast/slow pitch) */
#define kPPct000		    0x00
#define kPPct050		    0x32
#define kNPct050		    0xCE
#define kPPct010		    0x0A
#define kNPct010		    0xF6
#define kFineEn			    0x10
#define kSingleSpeed		    0x01
#define kDoubleSpeed		    0x02

/* Data Availability Flags */
#define CRC_ERROR_BIT		    0x02
#define kNoData			    -1
#define kBadData		    -2
#define kNoSubcode		    -3

/* TOC Building Flags */
#define TOC_GOT_A0		    0x01
#define TOC_GOT_A1		    0x02
#define TOC_GOT_A2		    0x04

/* Cd_State Bits */
#define CD_DOOR_OPEN_SWITCH	    0x00000001	    /* current state of open switch		   */ 
#define CD_DOOR_CLOSED_SWITCH	    0x00000002	    /* current state of close switch		   */
#define CD_USER_SWITCH		    0x00000004	    /* current state of user switch		   */
#define CD_RSRV1_SWITCH		    0x00000008	    /* NOTE: The low 8 bits of cd_State are	   */
#define CD_RSRV2_SWITCH		    0x00000010	    /*	     reserved for the current state of the */
#define CD_RSRV3_SWITCH		    0x00000020	    /*	     switch bits, as returned in the most  */
#define CD_RSRV4_SWITCH		    0x00000040	    /*	     recent switch report.		   */
#define CD_RSRV5_SWITCH		    0x00000080

#define CD_NO_DISC_PRESENT	    0x00000100	    /* no disc is present			   */
#define CD_UNREADABLE		    0x00000200	    /* an unreadable disc is present		   */
#define CD_CACHED_INFO_AVAIL	    0x00000400	    /* initial STATUS and DISCDATA available	   */
#define CD_DEVICE_ONLINE	    0x00000800	    /* good disc is inserted, can process CMD_READ */
#define CD_DIPIR_PENDING	    0x00001000	    /* we just got a CLOSE SwRpt for our device	   */
#define CD_JUST_DIPIRED		    0x00002000	    /* a dipir event just occured (on some device) */

#define CD_DEVICE_ERROR		    0x00004000	    /* device returned Stuck, FocusFail, Unreadable, SeekFailure */

#define CD_READING_TOC_INFO	    0x00008000	    /* currently reading a (session's) TOC	   */
#define CD_GOT_ALL_TRACKS	    0x00010000	    /* we read all the tracks for the current TOC  */
#define CD_READ_NEXT_SESSION	    0x00020000	    /* we saw an 0x05/0xB0 (multisession) entry	   */
#define CD_READING_INITIAL_TOC	    0x00040000	    /* are we reading the 1st session's TOC?	   */

#define CD_SNARF_OVERRUN	    0x00080000	    /* the snarf buffer ran out of snarf blocks	   */
#define CD_PREFETCH_OVERRUN	    0x00100000	    /* the prefetch space has been filled	   */

#define CD_PREFETCH_SUBCODE_ENABLED 0x00200000	    /* is subcode requested for this read?	   */
#define CD_CURRENTLY_PREFETCHING    0x00400000	    /* are we currently reading data (PLAYing)?	   */
#define CD_SUBCODE_SYNCED_UP	    0x00800000	    /* is the subcode currently "locked-in"?	   */
#define CD_GONNA_HAVE_TO_STOP	    0x01000000	    /* have we not fully stopped prefetching?	   */

#define CD_DRAWER_MECHANISM	    0x02000000	    /* presently shows if a SW_USER was pressed	   */
#define CD_USING_MIN_BUF_SPACE	    0x04000000	    /* are we using "minimum" memory usage mode?   */
#define CD_READ_IOREQ_BUSY	    0x08000000	    /* currently processing CMD_READ (re: gHWM)	   */
#define CD_DISC_IS_CDI		    0x10000000	    /* is it a CD-I disc?                          */
#define CD_DISC_IS_CDI_WITH_AUDIO   0x20000000	    /* is it a CD-I disc with audio tracks?        */
#define CD_ALREADY_RETRIED_TOC_READ 0x40000000      /* have we already made a 2nd attempt?         */

/* status byte bits */
#define SB_READY		    0x01	    /* drive ready (at least attempted to read TOC)*/
#define SB_DBLSPD		    0x02	    /* drive is in double speed mode		   */
#define SB_RSRV1		    0x04	    /* reserved					   */
#define SB_RSRV2		    0x08	    /* reserved					   */
#define SB_ERROR		    0x10	    /* drive error (this never actually gets set)  */
#define SB_SPINUP		    0x20	    /* disc is spinning				   */
#define SB_DISCIN		    0x40	    /* disc present				   */
#define SB_DOOR			    0x80	    /* drawer is closed				   */

/* subcode stuff */
#define SYNC_MARK		    0x80
#define NUM_SYNCS_NEEDED_2_PASS	    3
#define NUM_NOSYNCS_NEEDED_2_FAIL   7

/* mechanism-type stuff */
#define kClamWithClampMechanism	    0x00
#define kClamWithBallMechanism	    0x01
#define kDrawerMechanism	    0x02

/* error stuff */
#define MakeCDErr(svr,class,err)    MakeErr(ER_DEVC,((Make6Bit('C')<<6)|Make6Bit('D')),svr,ER_E_SSTM,class,err)
/* generic system-type errors */
#define kCDErrAborted		    MakeCDErr(ER_SEVERE, ER_C_STND, ER_Aborted)
#define kCDErrBadArg		    MakeCDErr(ER_SEVERE, ER_C_STND, ER_BadIOArg)
#define kCDErrBadCommand	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_BadCommand)
#define kCDErrBadUnit		    MakeCDErr(ER_SEVERE, ER_C_STND, ER_BadUnit)
#define kCDErrNotPrivileged	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_NotPrivileged);
#define kCDErrSoftErr		    MakeCDErr(ER_SEVERE, ER_C_STND, ER_SoftErr)
#define kCDErrNoMemAvail	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_NoMem)
#define kCDErrNoLCCDDevices	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_NoHardware)
#define kCDErrIOInProgress	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_IONotDone)
#define kCDErrIOIncomplete	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_IOIncomplete)
#define kCDErrDeviceOffline	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_DeviceOffline)
#define kCDErrDeviceError	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_DeviceError)
#define kCDErrMediaError	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_MediaError)
#define kCDErrEndOfMedium	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_EndOfMedium)
/* LCCD-specific error messages */
#define kCDErrDaemonKilled	    MakeKErr(ER_SEVERE, ER_C_NSTND,ER_Kr_TaskKilled)
#define kCDErrSnarfBufferOverrun    kCDErrSoftErr
#define kCDErrModeError		    kCDErrBadArg
#define kCDNoErr		    0

/* used in state machines' return values */
#define kCmdNotYetComplete	    0
#define kCmdComplete		    1

#define kMajorDevStateMask	    0x00F0
#define kCmdStateMask		    0x0F00

enum CommandStates {
    kSendCmd =		0x0000,
    kWait4Tag =		0x0100,
    kWait4CIPState =	0x0200
};

enum DeviceStates {
    DS_INIT_REPORTS = 0x10,
	IR_START = 0x10,
	IR_SWEN,
	IR_QDIS,
	IR_MECH,
	IR_CIPEN,
    DS_OPEN_DRAWER = 0x20,
    DS_CLOSE_DRAWER = 0x30,
	CL_START = 0x30,
	CL_CLOSE,
	CL_DIPIR,
    DS_BUILDING_DISCDATA = 0x40,
	BDD_START = 0x40,
	BDD_PAUSE,
	BDD_SPEED,
	BDD_FORMAT,
	BDD_SEEK,
	BDD_PLAY,
	BDD_QEN,
	BDD_QDIS,
	BDD_CHK4MULTI,
	BDD_SEEK2PAUSE,
    DS_PROCESS_CLIENT_IOREQS = 0x50,
	RD_START = 0x50,
	RD_VARIABLE_PITCH,
	RD_LOOP,
	RD_RESTART,
	RD_SPEED,
	RD_FORMAT,
	RD_SEEK,
	RD_PREPARE,
	RD_PLAY,
	SQ_START = 0x50,
	SQ_QNOW,
	SQ_UNIQUE_QRPT,
    DS_STOP_PREFETCHING = 0x60,
	PE_START = 0x60,
	PE_PAUSE,
    DS_RECOVER_FROM_DIPIR = 0x70,
	RFD_START = 0x70,
	RFD_PAUSE,
	RFD_RECOVER
};

/* These will need to go in "cdrom.h" if we decide to make RESIZE_BUFFERS public */
#define CDROM_DEFAULT_BUFFERS	    0
#define CDROM_MINIMUM_BUFFERS	    1
#define CDROM_STANDARD_BUFFERS	    2

enum CDROMCommands {
    CDROMCMD_PASSTHROUGH = 3,
    CDROMCMD_DISCDATA,
    CDROMCMD_SETDEFAULTS,
    CDROMCMD_READ_SUBQ,
    CDROMCMD_READ_DISC_CODE,
    CDROMCMD_OPEN_DRAWER,
    CDROMCMD_CLOSE_DRAWER,
    CDROMCMD_RESIZE_BUFFERS,
    CDROMCMD_DIAG_INFO
};

enum SectorTypes {
    DA_SECTOR = 0x00,	    /* Values also correpsond to Bob's format eegister expectations */
    M1_SECTOR = 0x78,
    XA_SECTOR = 0xF8,	    /* Arbitrarily picked Mode2Form2 (since Form1/Form2 are now the "same" WRT EDC) */
    INVALID_SECTOR = 0xFF   /* invalid mode */
};

enum BufferBlockStates {
    BUFFER_BLK_FREE = 0,
    BUFFER_BLK_INUSE,
    BUFFER_BLK_VALID,
    BUFFER_BLK_BUCKET
};

enum CDROM_Speed {
  CDROM_SINGLE_SPEED	    = 1,
  CDROM_DOUBLE_SPEED	    = 2
};

enum CDROM_Pitch {
  CDROM_PITCH_SLOW	    = 1,
  CDROM_PITCH_NORMAL	    = 2,
  CDROM_PITCH_FAST	    = 3
};

enum CDROM_DensityCodes {
  CDROM_DEFAULT_DENSITY	    = 1,
  CDROM_DATA		    = 2,
  CDROM_MODE2_XA	    = 3,
  CDROM_DIGITAL_AUDIO	    = 4
};

enum CDROM_Block_Sizes {
  CDROM_M1_D		    = 2048,
  CDROM_DA		    = 2352,
  CDROM_DA_PLUS_ERR	    = 2353,
  CDROM_DA_PLUS_SUBCODE	    = 2448,
  CDROM_DA_PLUS_BOTH	    = 2449
};

enum CDROMAddressFormat {
  CDROM_Address_Blocks	    = 0,
  CDROM_Address_Abs_MSF	    = 1,
  CDROM_Address_Track_MSF   = 2
};

enum CDROM_Error_Recovery {
  CDROM_DEFAULT_RECOVERY	 = 1,
  CDROM_CIRC_RETRIES_ONLY	 = 2,
  CDROM_BEST_ATTEMPT_RECOVERY	 = 3
};

/*
  N.B.	The "retryShift" field is meaningful if and only if the errorRecovery
  field contains a meaningful value.  Specifying 0 for the retryShift field
  does _not_ mean "use the default value"!  The default value for retries
  is used iff the errorRecovery field contains the default.
*/

typedef union CDROMCommandOptions {
  uint32 asLongword;
  struct {
    unsigned int     densityCode   : 3;
    unsigned int     errorRecovery : 2;
    unsigned int     addressFormat : 2;
    unsigned int     readAhead	   : 2;
    unsigned int     timeoutShift  : 4; /* 2^N millisecond timeout */
    unsigned int     retryShift	   : 3; /* (2^N)-1 retries */
    unsigned int     speed	   : 2;
    unsigned int     pitch	   : 2;
    unsigned int     blockLength   : 12;
  } asFields;
} CDROMCommandOptions;

typedef struct CDDiscInfo {
  uint8		     discID;
  uint8		     firstTrackNumber;
  uint8		     lastTrackNumber;
  uint8		     minutes;
  uint8		     seconds;
  uint8		     frames;
} CDDiscInfo;

typedef struct CDTOCInfo {
  uint8		     reserved0;
  uint8		     addressAndControl;		/* The Ctl/Adr byte from Qcode with nybbles swapped! */
  uint8		     trackNumber;
  uint8		     reserved3;
  uint8		     minutes;
  uint8		     seconds;
  uint8		     frames;
  uint8		     reserved7;
} CDTOCInfo;

typedef struct CDSessionInfo {
    uint8	       valid;
    uint8	       minutes;
    uint8	       seconds;
    uint8	       frames;
    uint8	       rfu[2];
} CDSessionInfo;

typedef struct CDROM_Disc_Data {
    CDDiscInfo	    info;
    CDTOCInfo	    TOC[100];
    CDSessionInfo   session;
} CDROM_Disc_Data;

typedef struct SubQInfo {
    uint8   cmdTag;		    /* Required for MKE emulation */
    uint8   validByte;		    /* Required for MKE emulation */
    uint8   addressAndControl;
    uint8   trackNumber;
    uint8   Index;		    /* Or Point */
    uint8   minutes;
    uint8   seconds;
    uint8   frames;
    uint8   reserved;
    uint8   aminutes;		    /* Or PMIN */
    uint8   aseconds;		    /* Or PSEC */
    uint8   aframes;		    /* Or PFRAME */
} SubQInfo;

typedef struct snarfBlock {
    struct snarfBlockHeader {
	uint8	state;
	uint8	size;
    } blkhdr;
    uint8 blkdata[kLCCDMaxStatusLength];    /* (the max that bob can return is 12 bytes or so, see spec.) */
#ifdef DEVELOPMENT
    uint32 dbgdata[4];
#endif
} snarfBlock;

typedef struct sectorBlockHeader {
    uint8   *buffer;
    uint8   state;			/* State Of Given Buffer Block (FREE, INUSE, Etc.) */
    uint8   format;			/* Format That This Sector Was Read In As (DA, M1, M2) */
    uint32  MSF;
} sectorBlkHdr;

typedef uint8 subcodeBlock[196];
typedef struct subcodeBlockHeader {
    uint8   *buffer;
    uint8   state;
    uint8   reserved1;			/* these are here so that the subcodeBlkHdr struct will get crammed */
    uint8   reserved2;			/* into one 4-byte word.  without them, the struct is two words long. */
} subcodeBlkHdr;

typedef struct cdrom
{
    Device	cdDev;				/* DO WE REALLY NEED THIS?!? */
    uint8	cd_unit;			/* xbus unit number of this device */

    IOReq	*cd_workingIOR;			/* pointer to ioReq currently being processed */
    List	cd_pendingIORs;			/* list of ioReq's to be processed */
    
    IOReq	*cd_cmdReq;			/* ioreq used for xbus cmd/stat transactions */
    IOReq	*cd_snarfReq;			/* ioreq used for "snarf" transactions (async. status responses) */
    IOReq	*cd_recoverReq;			/* ioreq used for dipir "duck-n-cover" transactions (after dipir) */
    
    uint8	cd_cmdBytes[4];			/* Buffer Used By Cd_CmdReq (Ioi_Send.Iob_Buffer) (Longest Cmd, SEEK, Is 4 Bytes) */

    uint8	cd_DataReadIndex;		/* points to next valid data block */
    uint8	cd_CurDataWriteIndex;		/* Points To Current Data Block...Used By Data FIRQ */
    uint8	cd_NextDataWriteIndex;		/* Points To Next Data Block...Used By Data FIRQ */

    uint8	    cd_NumDataBlks;				/* current number of blocks in prefetch (data) space */
    uint8	    cd_NumSubcodeBlks;				/* current number of blocks in subcode space */
    
    sectorBlkHdr    cd_DataBlkHdr[MAX_NUM_DATA_BLKS];		/* block headers for each data block in the data buffer */

    subcodeBlkHdr   cd_SubcodeBlkHdr[MAX_NUM_SUBCODE_BLKS];	/* block headers for each subcode block in the subcode buffer */
    subcodeBlock    *cd_SubcodeBuffer;				/* Buffer Pool For Sector Prefetch (Subcode) Engine [NOTE: Each Physical Block Includes 2 Logical Blocks] */
    uint8	*cd_ExtBuffers;

    uint8	cd_SubcodeReadIndex;		/* points to next valid data block */
    uint8	cd_CurSubcodeWriteIndex;	/* Points To Current Data Block...Used By Data FIRQ */
    uint8	cd_NextSubcodeWriteIndex;	/* Points To Next Data Block...Used By Data FIRQ */

    uint8	*cd_SubcodeTrueStart;		/* Actually Starting Addr In SubcodeBuffer Of The Sync Mark */

    snarfBlock	cd_SnarfBuffer[MAX_NUM_SNARF_BLKS];	/* buffer pool for async snarf responses */
    uint8	cd_SnarfReadIndex;		/* Points To Next Valid Snarf Block...Used By Async Response Processor */
    uint8	cd_SnarfWriteIndex;		/* points to next available snarf block...used by snarf callback routine */

    Item	cd_Ch0FIRQ;			/* Item Associated W/Ch0 (Data) DMA Handler */
    Item	cd_Ch1FIRQ;			/* Item Associated W/Ch1 (Subcode) DMA Handler */

    uint32	cd_SectorOffset;		/* Offset Of Sector(S) To Read During This CMD_READ IoRequest */
    uint32	cd_SectorsRemaining;		/* Number Of Sectors Left In This CMD_READ IoRequest */
    uint32	cd_BlockLength;			/* current setting of device block length */
    uint8	*cd_SavedRecvPtr;		/* save off ptr to the client's recv buf so that we can manipulate the ptr */
    uint8	cd_CurRetryCount;		/* current retry count for this sector */

    uint32	cd_DevState;			/* Current State Of Device Machine (The Major State) */
    uint32	cd_SavedDevState;		/* Place To Save State While We Stop The Prefetch Engine */
    
    uint32	cd_PrefetchStartOffset;		/* start of range of prefetched data */
    uint32	cd_PrefetchEndOffset;		/* end of range of prefetched data */
    uint32	cd_PrefetchCurMSF;		/* BCD MSF That Goes In Blkhdr.MSF To Be Able To Tell If It'S The Sector We'Re Interested In */
    uint8	cd_PrefetchSectorFormat;	/* the sector format we're currently using to prefetch data (this gets copied into blkhdr.format) */

    uint8	cd_CmdByteReceived;		/* last received cmd tag byte from device */
    uint8	cd_CIPState;			/* Last Achieved State Of Device (Received From A CIPReport)	 */
    uint8	cd_StatusByte;			/* Returned In .Ds_DeviceFlagWord */
    uint32	cd_MediumBlockCount;		/* Returned In .Ds_DeviceBlockCount */
    uint32	cd_State;			/* current state (flags) of device (i/o done, etc.) */
    
    SubQInfo	cd_LastQCode0;			/* Last Valid Qcode (Lead-In Area) Returned By A QRpt Cached (TOC Info) */
    SubQInfo	cd_LastQCode2;			/* Last Valid Qcode (Mode-2 Area) Returned By A QRpt Cached (UPC/EAN Info) */
    SubQInfo	cd_LastQCode3;			/* Last Valid Qcode (Mode-3 Area) Returned By A QRpt Cached (ISRC Info) */
	
    CDROMCommandOptions cd_DefaultOptions;
    struct {
	uint8	speed;
	uint8	pitch;
	uint8	format;
	uint8	subcode;
    } cd_CurrentSettings;
    
    CDDiscInfo	    cd_DiscInfo;		/* NOTE:  Do not change the order of these structs, or place anything */
    CDTOCInfo	    cd_TOC_Entry[100];		/*        between them.  These must be consistent with the struct     */
    CDSessionInfo   cd_SessionInfo;		/*        CDROM_Disc_Data.                                            */

    uint32	    cd_NextSessionTrack;
    uint32	    cd_TOC;
    uint8	    cd_BuildingTOC;
} cdrom;

typedef struct LCCDDeviceEntry {
  Node	cd;
  cdrom *cd_Device;
} LCCDDeviceEntry;

#endif
