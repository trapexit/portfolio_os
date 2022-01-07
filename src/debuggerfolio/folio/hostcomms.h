/* $Id: hostcomms.h,v 1.3 1994/08/02 02:40:30 anup Exp $ */

/*
 * $Log: hostcomms.h,v $
 * Revision 1.3  1994/08/02  02:40:30  anup
 * Added Id and Log keywords
 *
 */

/*
	File:		hostcomms.h

	Contains:	Header for describing host communications buffer (DemandBuffComm)

	Written by:	Anup K Murarka

	Copyright:	©1994 by The 3DO Company, all rights reserved
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

				08.07.94	akm		First Writing

	To Do:
*/


typedef enum ActionRequests
{
	PRINTFCMD = 1,
	READCMD,
	CRTWRTCMD,
	APPWRTCMD,
	ASKCMD,
	FILEINFOCMD,
	WRTFILECMD,
	PRINTCHARCMD,
	ASKCHARCMD,
	FILENAMECMD,
	CDREADCMD,
	SENDTOAPP,
	TASKLAUNCHED,
	TASKEXITED,
	SWITRACE,
	BUSY = -1
} ActionRequests;


typedef struct MonitorBufferHdr
{
	int32	mb_Semaphore;
	int32	mb_RequestReady;
	int32	mb_OwnerID;
	int32	mb_CallerID;
	int32	mb_CmdID;
	int32	*mb_RequestStatus;
} MonitorBufferHdr;

typedef struct MonitorBuffer
{
	int32	mb_Semaphore;
	int32	mb_RequestReady;
	int32	mb_OwnerID;
	int32	mb_CallerID;
	int32	mb_CmdID;
	int32	*mb_RequestStatus;
	int32	mb_reserved[26];
} MonitorBuffer;

typedef struct LaunchMessage
{
	MonitorBufferHdr	lm_header;
	Task*				lm_TaskPtr;
	void*				lm_AIFStart;
	uint32*			lm_StackBase;
	uint32				lm_StackSize;
	Item				lm_Item;
	char				lm_TaskName[64];		// first 64 chars of name
	uint32				reserved[5];
} LaunchMessage;

typedef struct SWITraceMessage
{
	MonitorBufferHdr	mb_header;
	uint32				st_Opcode;
	void*				st_returnAddr;
	Task*				st_currentTask;
	uint32				st_registers[13];
	uint32				reserved[10];
} SWITraceMessage;

