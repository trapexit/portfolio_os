#ifndef _H_BARF
# define _H_BARF

#ifdef EXTERNAL_RELEASE
/* cause an error if this file gets included somehow */
#include "THIS_FILE_IS_3DO-INTERNAL,_AND_IS_NOT_FOR_THE_USE_OF_DEVELOPERS"
#else /* EXTERNAL_RELEASE */

#ifndef _H_IO
# include "io.h"
#endif

#ifndef _H_DEVICE
# include "device.h"
#endif


/*
 * Block size supported by the server.
 * Redefine if server supports larger block size.
 * Must be integral multiple of 512 bytes.
 */
#define BARF_SERVER_BLOCK_SIZE		4096

#define BARF_REQUEST_BUFFER_SIZE   40
/*
 * The size of Barf reply blocks differ, depending upon the command.
 * We simplify life by making the reply block size be the largest
 * possible reply block: the maximum header, plus a data block.
 * Happily, this corresponds to a MultiRead reply, which will comprise
 * the vast majority of our traffic.
 */
#define BARF_REPL_BUF_MAX_HEADER_SIZE	12
#define BARF_REPLY_BUFFER_SIZE \
		(BARF_REPL_BUF_MAX_HEADER_SIZE + BARF_SERVER_BLOCK_SIZE)

/*
   Window size must be a power of two!
*/

#define BARF_WINDOW_SIZE           32

#define BARF_RETRY_MASK            0x80
#define BARF_RFU_MASK              0x70
#define BARF_OPERATION_MASK        0x0F

struct BarfDisk;

typedef struct WorkOrder {
  Link             wo;
  struct BarfDisk *wo_Disk;
  FileIOReq       *wo_FileIOReq;
  Err              wo_Error;
  List             wo_RequestsSubmitted;
  uint32           wo_NumRequestsSubmitted;
  uint32           wo_LowPage;
  uint32           wo_HighPage;
  uint32           wo_PageCount;
  uint32           wo_NextKey;
  uint32           wo_FirstUnrequestedPage;
  uchar            wo_Done;
  uchar            wo_Aborted;
  uchar            wo_NeedsAttention;
} WorkOrder;

/*
 * In the case of a directory read operation,
 * store next directory entry number in unused field.
 */
#define	wo_DirNextEntryNum	wo_HighPage

enum RequestPhase {
  REQUEST_IDLE,
  REQUEST_SENDING,
  REQUEST_SENT,
  REQUEST_RECEIVING
};


#define BARF_PAGE_BITS_SIZE	8	/* 8 * 32 bits = 256 pages max */

typedef struct ServerRequest {
  Link          req;
  WorkOrder    *req_WorkOrder;
  uint32	req_Flags;
  uint32	req_RetryBackOff;
  uint32        req_SequenceNumber;
  uint32        req_TimeoutStart;
  uint32        req_TimeoutLimit;
#ifdef DO_REQ_STATS
  uint32	req_TotalTime;		/* sum of (reply - start) times */
  uint32	req_MaxTime;		/* largest (reply - start) time */
  uint32	req_Pages;		/* original # of pages requested */
#endif
  enum RequestPhase req_Phase;
  uint32        req_FileUniqueID;
  uint32        req_LowPage;		/* abs. blk num of first blk we want */
  uint32        req_PagesPending;	/* number of blks not yet received */
  uint32        req_PageBits[BARF_PAGE_BITS_SIZE];
  					/* bit array for pages received; */
  					/* bit on means don't have page yet */
					/* high order bit of 0th array elem */
					/* corresponds to 0th page relative */
  					/* to req_LowPage */
  					/* Note: 256 pgs max per read-multi */
  IOReq        *req_IOReq;
  struct BarfRequest {
    uchar         br_OperationType; 	/* also holds retry bit */
    uchar         br_SeqMSB;
    uchar         br_SeqLSB;
    uchar         br_Buffer[BARF_REQUEST_BUFFER_SIZE];
  }             req_Barf;
} ServerRequest;

#define BARF_MAX_RETRY_BACK_OFF	0x0010	/* maximum retry back off multiplier */

/* req_Flags values */
#define BARF_REQ_WAS_SENT_FLAG	0x0001	/* request was sent */
#define BARF_REQ_RETRY_FLAG	0x0002	/* doing smart retry for request */
#define BARF_REQ_SMART_FLAG	0x0004	/* doing smart retry for request */
#define BARF_REQ_WRITE_TIME_OUT_FLAG	0x0008	/* write of cmd timed out */
#define BARF_REQ_WRITTEN_OK	0x0010	/* write of cmd was OK: got callback */
/* and predicates */
#define BARF_REQ_WAS_SENT(req) \
	((req)->req_Flags & BARF_REQ_WAS_SENT_FLAG)
#define BARF_REQ_IS_RETRY(req) \
	((req)->req_Flags & BARF_REQ_RETRY_FLAG)
#define BARF_REQ_IS_SMART_RETRY(req) \
	((req)->req_Flags & BARF_REQ_SMART_FLAG)
#define BARF_REQ_WRITE_TIMED_OUT(req) \
	((req)->req_Flags & BARF_REQ_WRITE_TIME_OUT_FLAG)
#define BARF_REQ_WAS_WRITTEN_OK(req) \
	((req)->req_Flags & BARF_REQ_WRITTEN_OK)

typedef struct ServerReply {
  Link             reply;
  ServerRequest   *reply_Request;
  struct BarfDisk *reply_Disk;
  IOReq           *reply_IOReq;
#ifdef DYNAMIC_REPLY_SIZE
  uint32  	  reply_Size;		/* sizeof(this ServerReply) */
#endif
  struct BarfReply {
    uchar         br_OperationType;
    uchar         br_SeqMSB;
    uchar         br_SeqLSB;
    uchar         br_Status;
#ifdef DYNAMIC_REPLY_SIZE
    uchar	  br_Buffer[4];
#else
    uchar         br_Buffer[BARF_REPLY_BUFFER_SIZE];
#endif
  }                reply_Barf;
} ServerReply;

typedef struct BarfDisk {
  HighLevelDisk         bd;
  FileSystem           *bd_FileSystem;
  Device               *bd_NetDevice;
  uint8                 bd_NetUnit;
  uint8                 bd_NeedMoreReaders;
  uint8                 bd_Trouble;
  uint8                 bd_ServiceLock;
  uint32                bd_NeedToRelease;
  uint32                bd_OldestSequenceNumber;
  uint32                bd_NextSequenceNumber;
  uint32                bd_SequenceTableSize;
  uint32                bd_ServerWindowSize;
  uint32                bd_ReleaseFileIDs[8];
  uint32                bd_ReleaseDirectoryIDs[8];
  uint32		bd_PagesOutstanding;
  uint32		bd_NumServerRequestsOutstanding;
  List                  bd_WorkOrdersInUse;
  List                  bd_WorkOrdersFree;
  List                  bd_ServerRequestsFree;
  List                  bd_ServerRepliesFree;
  List                  bd_ServerRepliesReading;
  List                  bd_ServerRepliesToProcess;
  ServerRequest        *bd_ServerRequestsOutstanding[BARF_WINDOW_SIZE];
  uint32		bd_Flags;
#ifdef BD_SEMAPHORE
  Item			bd_Semaphore; 
#endif /* BD_SEMAPHORE */
} BarfDisk;

#define BARFDISK_SERVICE_UNLOCKED   0
#define BARFDISK_SERVICE_LOCKED     1
#define BARFDISK_SERVICE_HIT        2

/*
 * bd_Flags
 */
#define BARF_FLG_DAEMON_DISK_OPEN	0x1	/* daemon opened disk dev */

#endif /* EXTERNAL_RELEASE */
#endif /* _H_BARF */
