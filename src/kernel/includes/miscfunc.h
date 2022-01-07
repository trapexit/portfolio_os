/* $Id: miscfunc.h,v 1.3 1994/01/28 23:20:20 sdas Exp $ */
/* miscfunc.h */

/*
    Copyright (C) 1993, The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#include "types.h"
#include "list.h"
#include "nodes.h"

struct MiscFuncs
{
	void *(*mf_memcpy)(void * /* s1 */, const void * /*s2*/, size_t /*n*/ );
	void *(*mf_memset)(void * /*s*/, int /*c*/, size_t /*n*/);
	void (*mf_RemNode)( Node *n);
	void (*mf_InsertNodeFromTail)(List *l, Node *n);
	void (*mf_AddTail)(List *l, Node *n);
	Node *(*mf_RemTail)(List *l);
	void (*mf_AddHead)(List *l, Node *n);
	Node *(*mf_RemHead)(List *l);
	void (*mf_InsertNodeFromHead)(List *l, Node *n);
	void (*mf_InitList)(List *l, char *name);
	void (*mf___rt_sdiv)(void);	/* asm only */	/* 10 */
	void (*mf___rt_udiv)(void);	/* asm only */	/* 11 */
	void (*mf___rt_sdiv10)(void);	/* asm only */	/* 12 */
	void (*mf___rt_udiv10)(void);	/* asm only */	/* 13 */
};

extern struct MiscFuncs *MiscFuncs;

#define Macro_memcpy(a,b,c) (*MiscFuncs->mf_memcpy)(a,b,c)
#define Macro_memset(a,b,c) (*MiscFuncs->mf_memset)(a,b,c)

#define Macro_RemNode(a)	(*MiscFuncs->mf_RemNode)(a)
#define Macro_InsertNodeFromTail(a,b)	(*MiscFuncs->mf_InsertNodeFromTail)(a,b)
#define Macro_InsertNodeFromHead(a,b)	(*MiscFuncs->mf_InsertNodeFromHead)(a,b)
#define Macro_AddTail(a,b)	(*MiscFuncs->mf_AddTail)(a,b)
#define Macro_RemTail(a)	(*MiscFuncs->mf_RemTail)(a)
#define Macro_AddHead(a,b)	(*MiscFuncs->mf_AddHead)(a,b)
#define Macro_RemHead(a)	(*MiscFuncs->mf_RemHead)(a)
#define Macro_InitList(a,name)	(*MiscFuncs->mf_InitList)(a,name)
