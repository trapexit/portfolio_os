/* $Id: debuggerfolio.c,v 1.9 1994/09/15 21:13:41 vertex Exp $ */

/*
	$Log: debuggerfolio.c,v $
 * Revision 1.9  1994/09/15  21:13:41  vertex
 * Remove annoying output messages when starting up.
 *
 * Revision 1.8  1994/09/15  17:19:23  vertex
 * Changes for demand-loading
 *
 * Revision 1.7  1994/08/02  02:09:26  anup
 * Fixed (hopefully finally) the log & id keywords
 * Removed old unused & unreferenced functions
 * Verified IRQ bug fixes
 *
*/

/*
	File:		DebuggerFolio.c

	Contains:	Code for implementing Debugger Folio for use with Opera OS

	Written by:	Anup K Murarka

	Copyright:
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

				29.01.94	akm		First Writing

	To Do:
*/

#define DebugVersion "1.0d1"

/*
	Temporary symbol definitions
*/
#define INTERNALDEBUG

#include "types.h"
#include "debug.h"
#include "aif.h"
#include "kernel.h"
#include "super.h"
#include "debuggerfolio.h"
#include "debuggerfolio_private.h"
#include "debuggerlowmem.h"
#include "hostcomms.h"
#include "switrace.h"


// Folio prototypes
int32		InitDebugBase(DbgrFolio *db);
int32		OpenDebugBase(DbgrFolio *db);
void		CloseDebugBase(DbgrFolio *db);
int32		CreateDebuggerFolio( void );
static int32 ProcessBoneTag( SkelBone *bone, void *p, uint32 tag, uint32 arg);

// Prototypes for required Folio internal routines
int32 internalDeleteSkelItem (Item it, Task *t);
Item internalCreateSkelItem (void *n, uint8 ntype, void *args);
int32 internalCloseSkelItem (Item it, Task *t);
Item internalFindSkelItem (int32 ntype, TagArg *tp);
Item internalOpenSkelItem (Node *n, void *args);
int32 internalSetBoneInfo (SkelBone *bone, TagArg *args);

// Support code prototypes
Item internalCreateSkelBone (SkelBone *bone, TagArg *args);
Item internalDeleteSkelBone (SkelBone *sc, Task *t);

// Internal SWI prototypes
int32 swiConnectTasks( Item Bone1, Item Bone2 );
int32 swiSetSkelItemInfo( Item AnyItem, TagArg *tp );

/*
	external assembly routines for monitor handled calls
*/
extern void swiDebugTaskLaunch( void );
extern void GoMonitorSWIHandler( void );
extern void	SayHelloVector( int32 helloValue );
extern uint32 FolioMonitorVector(MonitorSelector selector, ...);

//int32 swiTaskLaunch( Task* taskControl, void*	startAddr );
void swiTaskExit( Item taskID );

/***************************************************************\
* Data & necessary structures                                   *
\***************************************************************/

DbgrFolio *DbgrBase;    // Pointer to Folio base

/*
** SWI - Software Interrupt Functions.
** Fill this with the names of your own function pointers. %C
** These must match the order of SWIs declared in DebuggerFolio.h .
** Note reverse order!
*/
void *(*DebuggerSWIFuncs[])() =		{
																	  (void *(*)())swiTaskExit,								/*   000000111 */
																	  (void *(*)())GoMonitorSWIHandler,				/*   000000110 */
																	  (void *(*)())swiSetSkelItemInfo,				/*   00000010f */
																	  (void *(*)())swiConnectTasks,						/*   00000010e */
																	  (void *(*)())GoMonitorSWIHandler,				/*   00000010d From here down are for compatibility with the monitor */
																	  (void *(*)())GoMonitorSWIHandler,				/*   00000010c */
																	  (void *(*)())GoMonitorSWIHandler,				/*   00000010b */
																	  (void *(*)())swiDebugTaskLaunch,				/*   00000010a */
																	  (void *(*)())GoMonitorSWIHandler,				/*   000000109 */
																	  (void *(*)())GoMonitorSWIHandler,				/*   000000108 */
																	  (void *(*)())GoMonitorSWIHandler,				/*   000000107 */
																	  (void *(*)())GoMonitorSWIHandler,				/*   000000106 */
																	  (void *(*)())GoMonitorSWIHandler,				/*   000000105 */
																	  (void *(*)())GoMonitorSWIHandler,				/*   000000104 */
																	  (void *(*)())GoMonitorSWIHandler,				/*   000000103 */
																	  (void *(*)())GoMonitorSWIHandler,				/* 2 000000102 */
																	  (void *(*)())GoMonitorSWIHandler,				/* 1 000000101 */
																	  (void *(*)())GoMonitorSWIHandler,				/* 0 000000100 */
																	};
#define NUM_DBGRSWIFUNCS (sizeof(DebuggerSWIFuncs)/sizeof(void *))

/* User functions for Skeleton folio*/
void *(*DbgrUserFuncs[])() = {
  (void *(*)())DeleteDbgTask,         /* -3 */
  (void *(*)())CreateDbgTask,         /* -2 */
  (void *(*)())GetSkelItemInfo     /* -1 */
};
#define NUM_DBGRUSERFUNCS (sizeof(DbgrUserFuncs)/sizeof(void *))

/*
	Tell Kernel how to allocate Items used by this folio

	As the folio grows to create & deal with other types
	this structure needs to grow to describe them
*/
struct NodeData DbgrNodeData[] =
{
  { 0, 0 },   /* Does this have to be empty??? */
  { sizeof(SkelBone), NODE_NAMEVALID|NODE_ITEMVALID }, // SKELETON_BONE_NODE = 1
};
#define DBGRNODECOUNT (sizeof(DbgrNodeData)/sizeof(NodeData))

/* Tags used when creating the Folio */
TagArg DbgrFolioTags[] =
{
  { TAG_ITEM_NAME,								(void *) kDebuggerFolioName },			// name of folio
  { CREATEFOLIO_TAG_NUSERVECS,		(void *) NUM_DBGRUSERFUNCS },   		// number of user functions
  { CREATEFOLIO_TAG_USERFUNCS,		(void *) DbgrUserFuncs },       		// list of user functions
// The following are only used for supervisor level folios
  { CREATEFOLIO_TAG_DATASIZE,			(void *) sizeof (DbgrFolio) },  	// size of debugger folio
  { CREATEFOLIO_TAG_NSWIS,				(void *) NUM_DBGRSWIFUNCS },				// number of SWI functions
  { CREATEFOLIO_TAG_SWIS,					(void *) DebuggerSWIFuncs },				// list of swi functions
  { CREATEFOLIO_TAG_INIT,					(void *) ((int32)InitDebugBase) },	// initialization code
  { CREATEFOLIO_TAG_ITEM,					(void *) DEBUGGERNODE },
  { CREATEFOLIO_TAG_OPENF,				(void *) ((int32)OpenDebugBase) },
//	{	CREATEFOLIO_TAG_CLOSEF,				(void *) ((int32)CloseDebugBase) },
  { CREATEFOLIO_TAG_NODEDATABASE,	(void *) DbgrNodeData },						// Skeleton node database
  { CREATEFOLIO_TAG_MAXNODETYPE,	(void *) DBGRNODECOUNT },					// number of nodes
  { TAG_END,											(void *) 0 },											// end of tag list
};


extern char BuildDate[];

/***************************************************************\
* Code                                                          *
\***************************************************************/

/*
	In the current version of the kernel, folios are started by running
	them as an application.  This will probably change.
*/
int main(int argc, char **argv)
{
	if (argc != DEMANDLOAD_MAIN_CREATE)
		return 0;


	print_vinfo();

//	printf("DebuggerSWIFuncs located at %p\n",DebuggerSWIFuncs);
//	printf("InitDebugBase located at %p\n",InitDebugBase);
//	Debug();

	return CreateDebuggerFolio();

#ifdef SWITrace
  if (CallBackSuper(InstallSWITrace,0,0) < 0)
    printf("Couldn't install swi trace mechanism\n");
#endif

}



int32 CreateDebuggerFolio(void)
{
	Item i;

	// Create the Skeleton folio item
	if ((i = CreateItem(MKNODEID(KERNELNODE,FOLIONODE),DbgrFolioTags)) < 0)
	{
		DBUG(("Cannot create Debugger Folio: error = 0x%x\n", i));
	}
	return i;
}



/*
		NOTE:
			Everything below this runs in Supervisor state
*/

/*
	InformDebugger replaces the informdebugger startup task
*/
void	InformDebugger(void)
{
	Task				myTask;				// used only to calc offsets
	ItemNode 		myItemNode;		// used only to calc offsets
	KernelArea	*monitorKernelArea;

	monitorKernelArea = (KernelArea*) FolioMonitorVector(mf_GetKernelArea);
	if (monitorKernelArea == nil)
	{
		SDBUG(("Unable to get KernelArea ptr from monitor\n"));
		SDBUG(("InformDebugger located at %p\n",InformDebugger));
		SDBUG(("FolioMonitorVector located at %p\n",FolioMonitorVector));
		return;
	}

	monitorKernelArea->KernelBasePtr = (void*) KernelBase;
	monitorKernelArea->CurrentTask_Off = (void*) ((char *)&KernelBase->kb_CurrentTask - (char *)&KernelBase->kb);
	monitorKernelArea->Task_t_Off = (void *)((char *)&myTask.t - (char *)&myTask.t);
	monitorKernelArea->StackBase_Off = (void *)((char *)&myTask.t_StackBase - (char *)&myTask.t);
	monitorKernelArea->StackSize_Off = (void *)((char *)&myTask.t_StackSize - (char *)&myTask.t);
	monitorKernelArea->ItemNode_Name_off = (void *)((char *)&myItemNode.n_Name - (char *)&myItemNode.n_Next);
}


// This is called by the kernel when the folio item is first created.
int32 InitDebugBase(DbgrFolio *db)
{
  ItemRoutines *itr;

  DbgrBase = db;       	// Where am I located?

  // Set pointers to required Folio functions
  itr = DbgrBase->dbgr_Folio.f_ItemRoutines;
  itr->ir_Delete = internalDeleteSkelItem;
  itr->ir_Create = internalCreateSkelItem;
  itr->ir_Find = internalFindSkelItem;
  itr->ir_Open = internalOpenSkelItem;
  itr->ir_Close = internalCloseSkelItem;

  DebugFolioPublicFlags->reserved = 0;
  DebugFolioPublicFlags->SWITraceEnable = 0;

  InitList( &DbgrBase->dbgr_TaskList, "AllBones" );

  InformDebugger();

  return 0;
}

/*
	This is called by the kernel when the folio item is opened

	This routine will eventually take the task info and add a
	new node to it's internal list.  Then when the task dies,
	the node will be removed and the host Debugger will be
	notified
*/
int32 OpenDebugBase(DbgrFolio *db)
{
	return 0;
}

void CloseDebugBase(DbgrFolio *db)
{
}



/*
	SWI functions
*/
int32 swiSetSkelItemInfo( Item AnyItem, TagArg *tp )
{
	Node *n;
	int32 Result;

	SDBUG (("swiSetSkelItemInfo:\n"));
	n = (Node *)LookupItem(AnyItem);
	if (n)
	{
		if (n->n_SubsysType != DEBUGGERNODE)
			return DB_ERR_BADITEM;
	}
	else
	{
		return DB_ERR_BADITEM;
	}

	// Needs more validation.  See TagProcessor in folio
	Result = SuperIsRamAddr( (char *) tp, sizeof(TagArg) );
	if (Result < 0)
	{
		ERR(("Bad TagArg address =  0x%x.\n", tp));
		return Result;
	}

	Result = DB_ERR_NOTSUPPORTED;
	switch (n->n_Type)
	{
		case DEBUGGER_TASK_NODE:
			Result = internalSetBoneInfo ((SkelBone *)n, tp);
			break;

	 	default:
	 		Result = DB_ERR_BADITEM;
			break;
	}

	return Result;
}


int32 swiConnectTasks( Item Bone1, Item Bone2 )
{
	/*
		this does nothing, it is a place holder only
		I have no idea what swi's we want to implement
	*/
	return 0;
}


/*

*/
int32 swiInformDebugger(TagArg *TagList)
{
	/*
		this does nothing, it is a place holder only
		I have no idea what swi's we want to implement
	*/
	return 0;
}



int32 internalSetBoneInfo (SkelBone *bone, TagArg *args)
{
	uint32 tagc, *tagp;
	Item Result;

	tagp = (uint32 *)args;

	if (tagp)
	{
		while ((tagc = *tagp++) != TAG_END)
		{
			Result = ProcessBoneTag( bone, 0, tagc, *tagp++ );
			if (Result < 0) return Result;
		}
	}
	return Result;
}


/*
	Internal routines required of all priveledged folios
*/

/*
	This routine is passed an item pointer in n.
	The item is allocated by CreateItem in the kernel
	based on information in the SkelNodeData array.
*/
Item internalCreateSkelItem(void *n, uint8 ntype, void *args)
{
	Item Result = DB_ERR_BADITEM;
	SDBUG (("internalCreateSkelItem: (0x%lx, %d, 0x%lx)\n", n, ntype, args));

	switch (ntype)
	{
		case DEBUGGER_TASK_NODE:
			Result = internalCreateSkelBone ((SkelBone *)n, (TagArg *)args);
			break;

		default:
			break;
	}

	return Result;
}


int32 internalDeleteSkelItem (Item it, Task *t)
{
	Node *n;
	int32 Result;

	SDBUG (("DeleteSkelItem (0x%lx, 0x%lx)\n", it, t));
	n = (Node*) LookupItem(it);

	switch (n->n_Type)
	{
		case DEBUGGER_TASK_NODE:
			Result = internalDeleteSkelBone ((SkelBone *)n, t);
			break;

		default:
			ERR(("internalDeleteAudioItem: unrecognised type = %d\n", n->n_Type));
			Result = DB_ERR_BADITEM;
			break;
	}

	return Result;
}



Item internalFindSkelItem (int32 ntype,  TagArg *tp)
{
	SDBUG (("FindSkelItem (%d, 0x%x)\n", ntype, tp));

  return -1;
}


Item internalOpenSkelItem (Node *n, void *args)
{
	SDBUG (("OpenSkelItem (0x%x, 0x%lx)\n", n, args));

  return -1;
}


int32 internalCloseSkelItem (Item it, Task *t)
{
SDBUG (("CloseSkelItem (0x%lx, 0x%lx)\n", it, t));

  return -1;
}


/*
	Support for internal routines required of all folios.
*/
static int32 ProcessBoneTag( SkelBone *bone, void *p, uint32 tag, uint32 arg)
{
	SDBUG(("ProcessBoneTag: bone=%lx tag=%lx arg=%lx\n",bone,tag,arg));

	switch (tag)
  {
		case DB_TAG_LENGTH:
			bone->task_Start = arg;
			break;

		case DB_TAG_WEIGHT:
			bone->task_ID = arg;
			break;

		default:
			SDBUG (("Warning - unrecognized argument to InitSkelBone - 0x%lx: 0x%lx\n",
	             tag, arg));
     	return DB_ERR_BADTAG;
	 		break;
  }

	return 0;
}

/*
	Initialize a bone putting reasonable values in all of its fields.
	Can be used to reinitilize an already existing SkelBone
*/
Item internalCreateSkelBone(SkelBone *bone, TagArg *args)
{
	Item Result;

	bone->task_Start = 100;
	bone->task_ID = 100;

	SDBUG(("Call TagProcessor\n"));
  Result = TagProcessor( bone, args, ProcessBoneTag, 0);

	SDBUG(("Finished TagProcessor\n"));
  if (Result < 0)
		return Result;

	return bone->task_Node.n_Item;
}


Item internalDeleteSkelBone (SkelBone *sc, Task *t)
{
  SDBUG (("InternalDeleteSkelBone (0x%lx,0x%lx)\n", sc, t));

#if 0
/* Dale: Should we restrict deletion to creator?! */
  SDBUG (("sc->sc_Creator = 0x%lx, t->t.n_Item = 0x%lx\n", sc->sc_Creator, t->t.n_Item));
  if (sc->sc_Creator != t->t.n_Item)
  {
    return -1;
  }
#endif

  return 0;
}



int32 SendTaskLaunchCmd(void* swiAddress)
{
	volatile LaunchMessage *lmDemandBuf = (LaunchMessage*) (0x03740000 - sizeof(MonitorBuffer));
	Task*			curTask;
	AIFHeader	*taskAIFImage;
	char*		tempTaskNamePtr;

	//SDBUG(("swiTaskLaunch called!  taskControl: %p		startAddr: %p\n",taskControl,startAddr));

	// wait until the buffer is available
	EnableIrq();
	while (lmDemandBuf->lm_header.mb_Semaphore != 0)
		;

	lmDemandBuf->lm_header.mb_Semaphore = -1;			// we own the block

	lmDemandBuf->lm_header.mb_OwnerID = -1;				// debugger is recipient
	lmDemandBuf->lm_header.mb_CmdID = 0x0d;
	lmDemandBuf->lm_header.mb_RequestStatus = nil;		//@@@@@may need async processing

	// specific data
	curTask = CURRENTTASK;
	lmDemandBuf->lm_TaskPtr = curTask;
	taskAIFImage = (AIFHeader*) ((char*)swiAddress - Offset(AIFHeader*,aif_DebugInit));
	lmDemandBuf->lm_AIFStart = 0;										// need assembly routine to get return addr
	lmDemandBuf->lm_StackBase = curTask->t_StackBase;
	lmDemandBuf->lm_StackSize = curTask->t_StackSize;
	lmDemandBuf->lm_Item = curTask->t.n_Item;
	tempTaskNamePtr = (char*) lmDemandBuf->lm_TaskName;
	strncpy(tempTaskNamePtr,curTask->t.n_Name,64);

	// finish up & send it
	lmDemandBuf->lm_header.mb_RequestReady = -1;

	SayHelloVector(0);

	return 0;
}


void swiTaskExit( Item taskID )
{
	volatile MonitorBuffer *monDemandBuf = (MonitorBuffer*) (0x03740000 - sizeof(MonitorBuffer));

	//SDBUG (("swiTaskExit called!  taskID: %u\n",taskID));

	// wait until the buffer is available
	EnableIrq();
	while (monDemandBuf->mb_Semaphore != 0)
		;

	monDemandBuf->mb_Semaphore = -1;		// we own the block

	monDemandBuf->mb_OwnerID = -1;				// debugger is recipient
	monDemandBuf->mb_CmdID = 0x0e;
	monDemandBuf->mb_RequestStatus = nil;		// don't need async processing

	// specific data
	monDemandBuf->mb_reserved[0] = (int32) taskID;

	// finish up & send it
	monDemandBuf->mb_RequestReady = -1;

	SayHelloVector(0);
}

