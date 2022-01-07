/* $Id: miscfuncs.c,v 1.8 1994/09/15 18:49:58 vertex Exp $ */
#include "types.h"
#include "internalf.h"

void *
memcpy(void *a, const void *b, size_t c)
{
    return Macro_memcpy(a,b,c);
}

void *
memset(void *a, int b, size_t c)
{
    return Macro_memset(a,b,c);
}

void
RemNode(Node *n)
{
    Macro_RemNode(n);
}

void
InsertNodeFromHead(List *l, Node *n)
{
    Macro_InsertNodeFromHead(l, n);
}

void
TailInsertNode(List *l, Node *n)
{
    Macro_InsertNodeFromTail(l, n);
}

void
InsertNodeFromTail(List *l, Node *n)
{
    Macro_InsertNodeFromTail(l, n);
}

void
AddTail(List *l, Node *n)
{
    Macro_AddTail(l, n);
}

Node *
RemTail(List *l)
{
    return Macro_RemTail(l);
}

void
AddHead(List *l, Node *n)
{
    Macro_AddHead(l, n);
}

Node *
RemHead(List *l)
{
    return Macro_RemHead(l);
}

void
InitList(List *l, const char *name)
{
    Macro_InitList(l, (char *)name);
}

void
FixKernelFolio(t)
void *(*t[])();
{
    t[-14] = (void *(*)())MiscFuncs->mf_memcpy;
    t[-13] = (void *(*)())MiscFuncs->mf_memset;

    t[-1] = (void *(*)())MiscFuncs->mf_RemHead;
    t[-2] = (void *(*)())MiscFuncs->mf_AddHead;
    t[-3] = (void *(*)())MiscFuncs->mf_RemTail;
    t[-4] = (void *(*)())MiscFuncs->mf_AddTail;
    t[-5] = (void *(*)())MiscFuncs->mf_InsertNodeFromTail;
    t[-6] = (void *(*)())MiscFuncs->mf_RemNode;
    t[-9] = (void *(*)())MiscFuncs->mf_InitList;

    t[-20] = (void *(*)())MiscFuncs->mf_InsertNodeFromHead;

    t[-33] = (void *(*)())MiscFuncs->mf___rt_sdiv;
    t[-34] = (void *(*)())MiscFuncs->mf___rt_udiv;
    t[-35] = (void *(*)())MiscFuncs->mf___rt_sdiv10;
    t[-36] = (void *(*)())MiscFuncs->mf___rt_udiv10;


    /*t[-1] = (void *(*)())MiscFuncs->mf_*/

#ifdef	CPU_ARM
    {
/******************************************************************
 ***								***
 ***	PATCH THE "RemTail" BUG IN THE OLD MISC CODE		***
 ***								***
 ***	The original version of RemTail delivered on		***
 ***	many, many CD-ROMs has a small bug: it does		***
 ***	not increment the anchor pointer by four		***
 ***	bytes, and thus leaves the NEXT pointer of the		***
 ***	final element pointing to the head node.		***
 ***								***
 ***	Here, we recognize this bug by seeing if the		***
 ***	sixth instruction is a "moveq r15,r14", and if		***
 ***	it is, we change it to "addne r3,r3,#4" and		***
 ***	sets the condition on the following three		***
 ***	instructions to "ne".					***
 ***								***
 ***	If you change this, make sure it still			***
 ***	properly patches the original misc code, and		***
 ***	does not patch any subsequently fixed misc		***
 ***	code.							***
 ***								***
 ******************************************************************/
	uint32	*wp = (uint32 *)(MiscFuncs->mf_RemTail);
	if (wp[5] == 0x01A0F00E)		/* ARM "moveq r15,r14" instruction */
	{
	    uint8	*cp = (uint8 *)wp;
	    wp[5] = 0x12833004;			/* ARM "addne r3,r3,#4" instruction */
	    cp[6*4] = 0x10 | (cp[6*4] & 0x0F);	/* make it "ne" conditional */
	    cp[7*4] = 0x10 | (cp[7*4] & 0x0F);	/* make it "ne" conditional */
	    cp[8*4] = 0x10 | (cp[8*4] & 0x0F);	/* make it "ne" conditional */
	}
    }
#endif	/* CPU_ARM */
}
