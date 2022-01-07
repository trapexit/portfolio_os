#ifndef __FOLIO_H
#define __FOLIO_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: folio.h,v 1.52 1994/11/23 21:23:41 vertex Exp $
**
**  Kernel folio management definitions
**
******************************************************************************/


#include "types.h"
#include "nodes.h"
#include "item.h"
#include "task.h"

#ifndef EXTERNAL_RELEASE
/* This is the structure of the Node Creation DataBase that each */
/* folio must have to use the CreateItem interface */

typedef struct NodeData
{
	uint8 size;
	uint8 flags;
} NodeData;

/* The upper 4 bits of the flags field are put in the n_Flags */
/* field of the created Node. The other four bits are used to */
/* tell the allocator other things for initialization */

typedef struct ItemRoutines
{
	/* Folio helper routines for its Items */
	Item  (*ir_Find)(int32 ntype, TagArg *tp);
	Item  (*ir_Create)(void *n, uint8 ntype, void *args);
	int32 (*ir_Delete)(Item it, struct Task *t);
	Item  (*ir_Open)(Node *n, void *args);
	int32 (*ir_Close)(Item it,struct Task *t);
	int32 (*ir_SetPriority)(ItemNode *n, uint8 pri, struct Task *t);
	Err   (*ir_SetOwner)(ItemNode *n, Item newOwner, struct Task *t);
	Item  (*ir_Load)(int32 ntype, TagArg *tp);
} ItemRoutines;
#endif /* EXTERNAL_RELEASE */

typedef struct Folio
{
	ItemNode fn;
	int32 f_OpenCount;
	uint8  f_TaskDataIndex;
	uint8 f_MaxSwiFunctions;
	uint8 f_MaxUserFunctions;
	uint8 f_MaxNodeType;

#ifndef EXTERNAL_RELEASE

	NodeData *f_NodeDB;
	int32  (*f_OpenFolio)(struct Folio *f);
	void  (*f_CloseFolio)(struct Folio *f);
	int32  (*f_DeleteFolio)(struct Folio *f);

	/* Folio helper routines for its Items */
	ItemRoutines *f_ItemRoutines;
	/* routines for task creation/deletion swapping */
	int32  (*f_FolioCreateTask)(struct Task *t,TagArg *tagpt);
	void (*f_FolioDeleteTask)(struct Task *t);
	void (*f_FolioRestoreTask)(struct Task *t);
	void (*f_FolioSaveTask)(struct Task *t);
	uint32	*f_DebugTable;

        /* additions for demand-loading */
	void    *f_DemandLoad;

	/* reserved for future expansion */
	int32	reserved2[6];

#else /* EXTERNAL_RELEASE */

        int32    f_Private[17];

#endif /* EXTERNAL_RELEASE */
} Folio, *FolioP;

enum folio_tags
{
	CREATEFOLIO_TAG_NUSERVECS = TAG_ITEM_LAST+1,	/* # of uservecs */
	CREATEFOLIO_TAG_USERFUNCS	/* ptr to uservec table */
#ifndef EXTERNAL_RELEASE
	,
	CREATEFOLIO_TAG_DATASIZE,
	CREATEFOLIO_TAG_INIT,
	CREATEFOLIO_TAG_NODEDATABASE,
	CREATEFOLIO_TAG_MAXNODETYPE,
	CREATEFOLIO_TAG_ITEM,
	CREATEFOLIO_TAG_OPENF,
	CREATEFOLIO_TAG_CLOSEF,
	CREATEFOLIO_TAG_DELETEF,
	CREATEFOLIO_TAG_NSWIS,		/* number of swis */
	CREATEFOLIO_TAG_SWIS,		/* ptr to SWI table */
	CREATEFOLIO_TAG_TASKDATA	/* per task data struct */
#endif /* EXTERNAL_RELEASE */
};

/*
    A folio's function table is an array of function pointers that extends in
    the negative direction.  FolioFunc is a pointer to a folio function.
    GetFolioFunc() gets the function pointer for a particular folio function.
*/
typedef int32 (*FolioFunc)();
#define GetFolioFunc(folio,func) ((const FolioFunc *)(folio))[func]
#ifndef EXTERNAL_RELEASE
    /*
        A macro similar to GetFolioFunc() that returns the address of a SWI
        (-f_MaxFunctions-1 is offset required to get to swi #0 in function table)
    */
  #define GetFolioSWIFunc(folio,swinum) \
    ((const FolioFunc *)(folio)) [ -( ((const Folio *)(folio))->f_MaxUserFunctions + 1 + swinum ) ]
#endif

/* To interpret this mismash of * and () */
/* _f is a pointer to table of function pointers */
/* This table extends in the negative direction */
/* These _could_ be updated to use GetFolioFunc() */
#define CALLFOLIO(folio,func,args) \
	{ \
		void (* *_f)() = (void (* *)())folio; \
		(*_f[func])args; \
	}
#define CALLFOLIORET(folio,func,args,ret,cast) \
	{ \
		int32 (* *_f)() = (int32 (* *)())folio; \
		ret = cast (*_f[func])args; \
	}
#define CallFolio(folio,func,args) \
	{ \
		void (* *_f)() = (void (* *)())folio; \
		(*_f[func])args; \
	}
#define CallFolioRet(folio,func,args,ret,cast) \
	{ \
		int32 (* *_f)() = (int32 (* *)())folio; \
		ret = cast (*_f[func])args; \
	}

/* Kernel vectors */
#define KBV_REMHEAD		-1
#define KBV_ADDHEAD		-2
#define KBV_REMTAIL		-3
#define KBV_ADDTAIL		-4
#define KBV_INSERTTAIL	-5
#define KBV_REMOVENODE	-6
#define KBV_ALLOCMEM	-7
#define KBV_FREEMEM		-8
#define KBV_INITLIST	-9
#define KBV_FINDNAMEDNODE	-10
#define KBV_SCAVENGEMEM	-11
#define KBV_LOCATEITEM	-12
#define KBV_LOOKUPITEM	-12
#define KBV_MEMSET		-13
#define KBV_MEMCPY		-14
#define KBV_GETPAGESIZE	-15
#define KBV_CHECKITEM	-16
#define KBV_FINSERT		-17
#define KBV_USEC2TICKS	-18
#define KBV_SETNODEPRI	-19
#define KBV_INSERTHEAD	-20
#define KBV_VFPRINTF	-21
#define KBV_GETSYSERR	-22
#define KBV_TICKS2TIMEVAL	-23
#define KBV_FINDMH	-25
#define KBV_ALLOCMEMML	-26
#define KBV_FREEMEMML	-27
#define KBV_ALLOCMEMLIST	-28
#define KBV_FREEMEMLIST	-29
#define KBV_ITEMOPENED	-32
#define KBV_FINDIMAGE	-38
#define KBV_GETMEMALLOCALIGNMENT -39
#define KBV_GETMEMTRACKSIZE -40
#define KBV_CHECKIO       -41
#define KBV_ISMEMREADABLE -42
#define KBV_ISMEMWRITABLE -43
#define KBV_FINDTAGARG    -44
#define KBV_NEXTTAGARG    -45

#ifndef EXTERNAL_RELEASE
#define VTYPE_SWI	0
#define VTYPE_USER	1
#define VTYPE_SUPER	2

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void __swi(KERNELSWI+23) *SetFunction(Item, int32 vnum, int32 vtype, void *newfunc);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* EXTERNAL_RELEASE */
#define FindFolio(n)        FindNamedItem(MKNODEID(KERNELNODE,FOLIONODE),(n))
#define FindAndOpenFolio(n) FindAndOpenNamedItem(MKNODEID(KERNELNODE,FOLIONODE),(n))


/*****************************************************************************/


#endif	/* __FOLIO_H */
