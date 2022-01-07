/* $Id: item.c,v 1.117.1.1 1994/12/19 20:57:27 vertex Exp $ */

#include "types.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "task.h"
#include "folio.h"
#include "semaphore.h"
#include "interrupts.h"
#include "msgport.h"
#include "kernel.h"
#include "mem.h"
#include "io.h"
#include "device.h"
#include "driver.h"
#include "timer.h"
#include "operror.h"
#include "debug.h"

#include "string.h"
#include "stdio.h"
#include "internalf.h"

extern void Panic(int halt,char *);

#ifdef MASTERDEBUG
#define DBUGCI(x)	if (CheckDebug(KernelBase,0)) printf x
#define DBUGDI(x)	if (CheckDebug(KernelBase,3)) printf x
#define DBUGOI(x)	if (CheckDebug(KernelBase,5)) printf x
#define DBUGSIP(x)	if (CheckDebug(KernelBase,10)) printf x
#define DBUGSIO(x)	if (CheckDebug(KernelBase,28)) printf x
#else
#define DBUGCI(x)
#define DBUGDI(x)
#define DBUGOI(x)
#define DBUGSIP(x)
#define DBUGSIO(x)
#endif

/*#define DEBUG*/
#define DBUG(x)	 /*{ if (isUser() == 0)	printf x ; else kprintf x; }*/
#define LDBUG(x)	/*printf x*/

#define DBUGWF(x)	/*printf x*/
#define PAUSE	/*if (ntype == FOLIONODE)	Pause();*/
#define DBUGCRT(x)	/*printf x*/
#define DBUGGI(x)	/*printf x*/

#define	MIN_ITEM		16
#define MAX_ITEMS		4096
#define ITEMS_PER_BLOCK		128
#define	MAX_BLOCKS		(MAX_ITEMS/ITEMS_PER_BLOCK)

static ItemEntry *ItemPtrTable[MAX_BLOCKS];
static int32      freeItemIndex = 0;

ItemEntry *
NewItemTableBlock(void)
{
    ItemEntry *ie;
    ItemEntry *p;
    int32      i;

    if (KernelBase->kb_MaxItem >= MAX_ITEMS)
	return 0;
    ie = (ItemEntry *)ALLOCMEM(sizeof(ItemEntry) * ITEMS_PER_BLOCK,
			       MEMTYPE_ANY);
    if (ie == 0)
	return 0;
    ItemPtrTable[KernelBase->kb_MaxItem/ITEMS_PER_BLOCK] = ie;

    /* Initialize the new table. All ie_ItemAddr are set to NULL,
     * and all ie_ItemInfo fields are set to the index number of the
     * entry that comes after them, except for the last entry in the
     * block, which gets the value of freeItemIndex instead
     */

    i = KernelBase->kb_MaxItem;
    p = ie;
    while (TRUE)
    {
        p->ie_ItemAddr = NULL;

        i++;
        if (i == KernelBase->kb_MaxItem + ITEMS_PER_BLOCK)
        {
            p->ie_ItemInfo = freeItemIndex;
            break;
        }

        p->ie_ItemInfo = i;
        p++;
    }

    freeItemIndex = KernelBase->kb_MaxItem;
    if (freeItemIndex == 0)
        freeItemIndex = MIN_ITEM;

    KernelBase->kb_MaxItem += ITEMS_PER_BLOCK;
    DBUGCI(("NewItemTableBlock: New MaxItem=%ld\n",KernelBase->kb_MaxItem));
    return ie;
}

ItemEntry **
InitItemTable(void)
{
    KernelBase->kb_ItemTable = ItemPtrTable;
    KernelBase->kb_MaxItem = 0;
    if (NewItemTableBlock() == 0)
    {
#ifdef DEVELOPMENT
	printf("Panic: Could not allocate first ItemTableBlock\n");
	while (1);	/* serious problem here */
#else
	Panic(1,"No ItemTableBlock\n");
#endif
    }
    DBUG(("ItemTable at:%lx\n",(uint32)ItemPtrTable));
    return ItemPtrTable;
}

ItemEntry *
GetItemEntryPtr(i)
Item i;
{
    ItemEntry *p;
    uint32 j;
    i &= ITEM_INDX_MASK;
    if (KernelBase->kb_MaxItem <= i)	return 0;
    j = i/ITEMS_PER_BLOCK;	/* which block */
    p = ItemPtrTable[j];
    i -= j*ITEMS_PER_BLOCK;	/* which one in this block? */
    return p + i;
}

Item AssignItem(p,i)
void *p;
Item i;
{
	ItemEntry *ie;
	DBUG(("AssignItem(%lx,%lx)\n",(long)p,i));
	if ((uint32)p & 0xf)
	{
#ifdef DEVELOPMENT
		printf("error in AssignItem p=%lx not divisable by 16\n",(uint32)p);
#endif
		return -1;
	}
	ie = GetItemEntryPtr(i);
	if (ie == 0) return -1;
	ie->ie_ItemAddr = p;
	ie->ie_ItemInfo = i;
	return i;
}

Item
GetItem(p)
void *p;
{
    int32	iix, i;
    ItemEntry *t;

    DBUG(("GetItem(%lx)\n",(uint32)p));

    if ((uint32)p & 0xf)
    {
#ifdef DEVELOPMENT
	DBUGCI(("Bad GetItem call addr&15 != 0, p=%lx\n",(uint32)p));
#endif
	return -1;
    }

    DBUGGI(("GetItem st=%d t=%d\n",((Node *)p)->n_SubsysType,\
	    ((Node *)p)->n_Type ));

    /* The available entries within the item table are kept in a linked list.
     * The list is made by having each empty slot in the table point to
     * another empty slot.
     */

    iix = freeItemIndex;
    if (iix == 0)
    {
        if (NewItemTableBlock() == NULL)
        {
            printf("GetItem: unable to extend system item table\n");
            return -1;
        }
        iix = freeItemIndex;
    }

    t = GetItemEntryPtr(iix);

    i = (t->ie_ItemInfo & ITEM_INDX_MASK);
    if (i >= MIN_ITEM)
    {
        /* okky dokky, we've got an empty slot we can use */
        freeItemIndex = i;
    }
    else if (i == 0)
    {
        /* no more free items, we'll need to extend the table later */
        freeItemIndex = 0;
    }

    /* set things up, and return */
    t->ie_ItemAddr = p;
    t->ie_ItemInfo = (t->ie_ItemInfo & ITEM_GEN_MASK) | iix;

    return t->ie_ItemInfo;
}

/**
|||	AUTODOC PUBLIC spg/kernel/lookupitem
|||	LookupItem - Get a pointer to an item.
|||
|||	  Synopsis
|||
|||	    void *LookupItem( Item i )
|||
|||	  Description
|||
|||	    This procedure finds an item by its item number and returns the
|||	    pointer to the item.
|||
|||	    Note:  Because items are owned by the system, user tasks cannot
|||	    change the values of their fields.  They can, however, read the
|||	    values contained in the public fields.
|||
|||	  Arguments
|||
|||	    i                           The number of the item to look up.
|||
|||	  Return Value
|||
|||	    The procedure returns the pointer to the item or NULL if the item
|||	    does not exist.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    item.h                      ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  See Also
|||
|||	    CheckItem(), CreateItem(), FindItem(), FindNamedItem(),
|||	    FindVersionedItem()
|||
**/

void *
LookupItem(i)
Item i;
{
	ItemEntry *ie;
	ItemNode  *in;

	LDBUG(("LookupItem(%lx)\n",i));

	ie = GetItemEntryPtr(i);
	if (ie == 0)	return 0;

	in = (ItemNode *)ie->ie_ItemAddr;
	if ((in == NULL) || (in->n_Item != i)) return 0;

        /* I believe this test is useless, since the comparison above between
         * n->n_Item and i will catch any difference in generation counts.
         *
	 * if ( (i&ITEM_GEN_MASK) != (ie->ie_ItemInfo & ITEM_GEN_MASK) ) return 0;
         */

	return (void *)in;
}

void
FreeItem(i)
Item i;
{
	ItemEntry *ie;
	Node *n = (Node *)LookupItem(i);
	DBUG(("FreeItem(%lx) n=%lx\n",i,(uint32)n));
	if (n)
	{
		ie = GetItemEntryPtr(i);
		ie->ie_ItemAddr  = 0;
		ie->ie_ItemInfo += 0x10000;	/* bump generation # */

		i &= ITEM_INDX_MASK;
		if (i >= MIN_ITEM)
		{
                    /* link this free entry into our "list" of free entries */
		    ie->ie_ItemInfo = (ie->ie_ItemInfo & ~ITEM_INDX_MASK) | freeItemIndex;
		    freeItemIndex   = i;
		}
	}
}

/**
|||	AUTODOC PUBLIC spg/kernel/checkitem
|||	CheckItem - Check to see if an item exists.
|||
|||	  Synopsis
|||
|||	    void *CheckItem( Item i, uint8 ftype, uint8 ntype )
|||
|||	  Description
|||
|||	    This procedure checks to see if a specified item exists. To
|||	    specify the item, you use an item number, an item-type number, and
|||	    the item number of the folio in which the item type is defined.
|||	    If all three of these values match those of the item, the
|||	    procedure returns a pointer to the item.
|||
|||	  Arguments
|||
|||	    i                           The item number of the item to be
|||	                                checked.
|||
|||	    ftype                       The item number of the folio that
|||	                                defines the item type.  (This is the
|||	                                same value that is passed to the
|||	                                MkNodeID() macro.) For a list of folio
|||	                                item numbers, see the `Portfolio
|||	                                Items' chapter.
|||
|||	    ntype                       The item-type number for the item.
|||	                                (This is the same value that is passed
|||	                                to the MkNodeID() macro.)  For a list
|||	                                of item-type numbers, see the
|||	                                `Portfolio Items' chapter.
|||
|||	  Return Value
|||
|||	    If the item exists (and the values of all three arguments match
|||	    those of the item), the procedure returns a pointer to the item.
|||	    If the item does not exist, the procedure returns NULL.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    item.h                      ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  See Also
|||
|||	    CreateItem(), FindItem(), FindNamedItem(), FindVersionedItem(),
|||	    LookupItem(), MkNodeID()
|||
**/

void *
CheckItem(Item i,uint8 SubsysType,uint8 Type)
{
	Node *n;
	n = (Node *)LookupItem(i);
	DBUG(("CheckItem(%lx), n=%lx\n",i,n));
	if (n)
	{
		if (n->n_SubsysType != SubsysType) return 0;
		if (n->n_Type != Type) return 0;
	}
	return (void *)n;
}

void setitemvr(Item ri) {
    ItemNode *in;
    Task *t = KernelBase->kb_CurrentTask;

    if(ri >= 0) {
	     in = (ItemNode *)LookupItem(ri);
	     if(!in->n_Version && !in->n_Revision) {
		in->n_Version = t->t.n_Version;
		in->n_Revision = t->t.n_Revision;
	     }
    }
}

Item
internalCreateKernelItem(void *n,uchar ntype,void *args)
{
	Item ret;

	DBUG(("internalCreateKernelItem ntype=%lx\n",(uint32)ntype));

	switch (ntype) {
	    case TASKNODE:
		ret = internalCreateTask((Task *)n,(TagArg *)args);
		break;
	    case SEMA4NODE:
		ret = internalCreateSemaphore((Semaphore *)n,(TagArg *)args);
		break;
	    case FOLIONODE:
		ret = internalCreateFolio((Folio *)n,(TagArg *)args);
		setitemvr(ret);
		break;
	    case MSGPORTNODE:
		ret = internalCreateMsgPort((MsgPort *)n,(TagArg *)args);
		break;
	    case MESSAGENODE:
		ret = externalCreateMsg((Msg *)n,(TagArg *)args);
		break;
	    case FIRQNODE:
		ret = internalCreateFirq((FirqNode *)n,(TagArg *)args);
		break;
	    case IOREQNODE:
		ret = internalCreateIOReq((IOReq *)n,(TagArg *)args);
		break;
	    case DRIVERNODE:
		ret = internalCreateDriver((Driver *)n,(TagArg *)args);
		setitemvr(ret);
		break;
	    case DEVICENODE:
		ret = internalCreateDevice((Device *)n,(TagArg *)args);
		setitemvr(ret);
		break;
	    case TIMERNODE:
		ret = internalCreateTimer((Timer *)n,(TagArg *)args);
		break;
	    case ERRORTEXTNODE:
		ret = internalCreateErrorText((ErrorText *)n,(TagArg *)args);
		break;
	    case HLINTNODE:
		ret = internalCreateHLInt((FirqNode *)n,(TagArg *)args);
		break;
	    default:
		ret = BADSUBTYPE;
	}
    return ret;
}

int32
IncreaseResourceTable(t,increase)
Task *t;
int32 increase;
{
	Item *newt;
	int32 oldsize = t->t_ResourceCnt;
	int32 newsize = oldsize + increase;

	DBUGCRT(("CRT(%lx,%d,%d)\n",(uint32)t,oldsize,newsize));
	if (increase <= 0) return 0;
	newt = (Item *)ALLOCMEM (newsize * sizeof(Item),MEMTYPE_ANY);
	VERIFYMEM("CRT:after AllocMem\n");
	if (newt)
	{
		int32 c = oldsize;
		Item *p = newt;
		Item *oldt = t->t_ResourceTable;
		int32 nextSlot;
		int32 oldCount;

		/* transfer contents of old resource table */
		while (c--)	*p++ = *oldt++;

		/* Free old resource table */
		FREEMEM(t->t_ResourceTable,oldsize*sizeof(Item));

                oldCount           = t->t_ResourceCnt;
		t->t_ResourceCnt   = newsize;
		t->t_ResourceTable = newt;

		/* initialize rest of array */
		VERIFYMEM("CRT:about to init array:\n");
		newsize -= oldsize;	/* leftover to init */

                nextSlot = oldCount + 1;
		while (newsize--)
		{
		    *p++ = (nextSlot | 0x80000000);
		    nextSlot++;
                }
                p--;
                *p = (t->t_FreeResourceTableSlot | 0x80000000);
                t->t_FreeResourceTableSlot = oldCount;

		return 0;
	}
	VERIFYMEM("before ret from CRT\n");
	DBUGCRT(("CRT returns newt=%lx\n",(uint32)newt));
	return -1;
}

/*****************************************************************************/


int32 AllocItemSlot(Task *t)
{
Item  *ip;
int32  i;

    ip = t->t_ResourceTable;

    /* t_FreeResourceTable contains either 0xffffffff, or the index of
     * a free entry within the table. If it has the index of a free entry,
     * the entry within the table itself contains the index of another
     * free entry, or 0xffffffff. We basically have a linked-list of free
     * entries.
     */

    i = t->t_FreeResourceTableSlot;
    if (i < 0)
    {
        /* no free slots in sight, try and extend the table */
        if (IncreaseResourceTable(t,(int32)4) != 0)
            return -1;

        ip = t->t_ResourceTable;
        i  = t->t_FreeResourceTableSlot;
    }

    if (ip[i] == 0xffffffff)
    {
        /* This entry doesn't point to another entry. This is
         * therefore the last free slot in the table. We set the
         * field to 0xffffffff to indicate this fact.
         */
        t->t_FreeResourceTableSlot = 0xffffffff;
    }
    else
    {
        /* Keep track of the next empty slot. We clear the high bit to
         * indicate the slot number is valid.
         */
        t->t_FreeResourceTableSlot = (ip[i] & 0x7fffffff);
    }

    return i;
}


/*****************************************************************************/


static void FreeItemSlot(Task *t, int32 slot)
{
    t->t_ResourceTable[slot]   = t->t_FreeResourceTableSlot | 0x80000000;
    t->t_FreeResourceTableSlot = slot;
}


/*****************************************************************************/


int32 FindItemSlot(Task *t, Item it)
{
Item  *ip;
int32  i;

    if (it < 0)
        return -1;

    ip = t->t_ResourceTable;

    DBUGDI(("FindItemSlot(ip=%lx it=%lx\n",(uint32)ip,(uint32)it));
    DBUGDI(("ResourceCnt=%d\n",t->t_ResourceCnt));

    for (i = 0; i < t->t_ResourceCnt; i++)
    {
#ifdef DEBUG
        DBUG(("*ip=%lx ",(uint32)*ip));
        if ((i & 3) ==3) DBUG(("\n"));
#endif
        if (*ip == it)
            return i;

        ip++;
    }

    return -1;
}


/*****************************************************************************/


void
oldRemoveItem(Task *t, Item item)
{
int32 slot;

    /* Remove item from task's resource table */
    if (t)
    {
	slot = FindItemSlot(t,item);
	if (slot >= 0)
	    FreeItemSlot(t,slot);
    }
}

void
RemoveItem(item)
Item item;
{
    oldRemoveItem((Task *)LookupItem(((ItemNode *)LookupItem(item))->n_Owner),
		  item);
}

/**
|||	AUTODOC PUBLIC spg/kernel/opennameddevice
|||	OpenNamedDevice - Open a named device.
|||
|||	  Synopsis
|||
|||	    Item OpenNamedDevice( char *name, a )
|||
|||	  Description
|||
|||	    This macro opens a device that you specify by name.
|||
|||	  Arguments
|||
|||	    name                        The name of the device to open.
|||
|||	    a                           This argument, which is not currently
|||	                                used, must be 0.
|||
|||	  Return Value
|||
|||	    The procedure returns the item number for the opened device or an
|||	    error code if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in device.h V20.
|||
|||	  Associated Files
|||
|||	    device.h                    ANSI C Macro definition
|||
|||	  See Also
|||
|||	    CloseNamedItem(), OpenItem(), CloseItem(), FindItem()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/closenameddevice
|||	CloseNamedDevice - Close a device previously opened with
|||	                   OpenNamedDevice().
|||
|||	  Synopsis
|||
|||	    Err CloseNamedDevice(Item devItem);
|||
|||	  Description
|||
|||	    This macro closes a device that was previously opened via a
|||	    call to OpenNamedDevice().
|||
|||	  Arguments
|||
|||	    devItem                     The device's Item, as returned by
|||	                                OpenNamedDevice()
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if the item was closed successfully or an
|||	    error code (a negative value) if an error occurs.  Possible error
|||	    codes include:
|||
|||	    BADITEM                     The devItem argument is not an item.
|||
|||	    ER_Kr_ItemNotOpen           The devItem argument is not an opened
|||	                                item.
|||
|||
|||	  Implementation
|||
|||	    Macro implemented in device.h V22.
|||
|||	  Associated Files
|||
|||	    device.h                    ANSI C Macro definition
|||
|||	  See Also
|||
|||	    OpenNamedItem(), OpenItem(), CloseItem(), FindItem()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/openitem
|||	OpenItem - Open a system item.
|||
|||	  Synopsis
|||
|||	    Item OpenItem( Item FoundItem, void *args )
|||
|||	  Description
|||
|||	    This procedure opens a system item for access.
|||
|||	  Arguments
|||
|||	    item                        The number of the item to open.  To
|||	                                find the item number of a system item,
|||	                                use FindItem(), FindDevice(),
|||	                                FindFolio(), or FindNamedItem().
|||
|||	    args                        A pointer to an array of tag
|||	                                arguments.  Currently, there are no
|||	                                system items that require tag
|||	                                arguments; for these and for any
|||	                                future items you're not sending tag
|||	                                arguments to, this argument must be
|||	                                NULL.  If you send tag arguments (for
|||	                                any items in the future that use
|||	                                them), the tag arguments can be in any
|||	                                order, and the last element of the
|||	                                array must be the value TAG_END.  For
|||	                                a list of tag arguments for each item
|||	                                type, see the `Portfolio Items'
|||	                                chapter.
|||
|||	  Return Value
|||
|||	    The procedure returns the number of the item that was opened or an
|||	    error code if an error occurs.
|||
|||	    Note:  The item number for the opened item is not always the same
|||	    as the item number returned by FindItem().  When accessing the
|||	    opened item you should use the return from OpenItem(), not the
|||	    return from FindItem(). You can also call the FindAndOpenItem()
|||	    function for to do both a search and an open operation in an
|||	    atomic manner.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    item.h                      ARM C "swi" declaration
|||
|||	  Notes
|||
|||	    To close an opened item, use CloseItem().
|||
|||	  See Also
|||
|||	    CloseItem(), FindItem()
|||
**/

Item
internalOpenItem(Item founditem,void *args)
{
	Item ret;
	Node *n = (Node *)LookupItem(founditem);
	Folio *f;
	ItemRoutines *ir;
	int32 slot;

	DBUGOI(("OpenItem item=%lx\n",(uint32)founditem));
	if (n == 0)	return BADITEM;

	/* check to see if the item has been fully created */
	if((n->n_Flags & NODE_ITEMVALID) && (((ItemNode *)n)->n_ItemFlags & ITEMNODE_NOTREADY))
		return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantOpen);

	f = WhichFolio(MKNODEID(n->n_SubsysType,n->n_Type));
	ir = f->f_ItemRoutines;

	if (!ir->ir_Open)
	    return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantOpen);

        /* make room in the resource table */
	slot = AllocItemSlot(KernelBase->kb_CurrentTask);
	if (slot < 0)
            return MAKEKERR(ER_KYAGB,ER_C_NSTND,ER_Kr_RsrcTblOvfFlw);

	ret = (*ir->ir_Open)(n,args);

	DBUGOI(("OpenItem ret = %lx\n",ret));
	if (ret >= 0)
	{
		/* Install this item in the Tasks ResourceTable */
		DBUGOI(("ct=%lx ip = %lx\n",(uint32)KernelBase->kb_CurrentTask,(uint32)ip));
		CURRENTTASK->t_ResourceTable[slot] = ret | ITEM_WAS_OPENED;
	}
	else
	{
		FreeItemSlot(CURRENTTASK,slot);
	}
	DBUGOI(("OpenItem now returns: %d\n",ret));
	return ret;
}

int
BadPriv(n,t)
ItemNode *n;
Task *t;
{
    Item it;
    if (t->t.n_Flags & TASK_SUPER)	return FALSE;	/* super task? */
    it = t->t.n_Item;
    if (n->n_Owner == it)	return FALSE;	/* owner? */
    if (n->n_Item == it)	return FALSE;	/* self? */
    return TRUE;
}

/**
|||	AUTODOC PUBLIC spg/kernel/setitempri
|||	SetItemPri - Change the priority of an item.
|||
|||	  Synopsis
|||
|||	    int32 SetItemPri( Item i, uint8 newpri )
|||
|||	  Description
|||
|||	    This procedure changes the priority of an item.  Some items in the
|||	    operating system are maintained in lists, for example, tasks and
|||	    threads.  Some lists are ordered by priority, with higher-priority
|||	    items coming before lower-priority items.  When the priority of an
|||	    item in a list changes, the Kernel automatically rearranges the
|||	    list of items to reflect the new priority.  The item is moved
|||	    immediately before the first item whose priority is lower.
|||
|||	    A task must own an item to change its priority.  A task can change
|||	    its own priority even if it does not own itself.
|||
|||	  Arguments
|||
|||	    i                           The item number of the item whose
|||	                                priority to change.
|||
|||	    newpri                      The new priority for the item.
|||
|||	  Return Value
|||
|||	    The procedure returns the previous priority of the item or an
|||	    error code if an error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    item.h                      ARM C "swi" declaration
|||
|||	  Notes
|||
|||	    For certain item types, such as devices, the Kernel may change the
|||	    priority of an item to help optimize throughput.
|||
|||	  Caveats
|||
|||	    This function is not implemented for all known item types.
|||
|||	  See Also
|||
|||	    CreateItem()
|||
**/

int32
internalSetItemPriority(it,pri,t)
Item it;
uint8 pri;
Task *t;
{
	int32 ret;
	ItemNode *n = (ItemNode *)LookupItem(it);
	Folio *f;
	ItemRoutines *ir;

	DBUGSIP(("internalSetItemPriority(%d,%lx) n=%lx\n",it,(uint32)t,(uint32)n));
	if (!n)	return BADITEM;

	if (BadPriv(n,t)) return BADPRIV;

	f = WhichFolio(MKNODEID(n->n_SubsysType,n->n_Type));
	DBUGSIP(("internalSetItemPriority, f=%lx\n",(uint32)f));

	ir = f->f_ItemRoutines;
	if (ir->ir_SetPriority == 0)	return  NOSUPPORT;
	ret = (*ir->ir_SetPriority)(n,pri,t);

	return ret;
}

/**
|||	AUTODOC PUBLIC spg/kernel/setitemowner
|||	SetItemOwner - Change the owner of an item.
|||
|||	  Synopsis
|||
|||	    Err SetItemOwner( Item i, Item newOwner )
|||
|||	  Description
|||
|||	    This procedure makes another task the owner of an item.  A task
|||	    must be the owner of the item to change its ownership.  The item
|||	    is removed from the current task's resource table and placed in
|||	    the new owner's resource table.
|||
|||	    You normally use this procedure to keep a resource available after
|||	    the task that created it terminates.
|||
|||	    Note:  You cannot change the ownership for these types of items:
|||	    Thread Task, Device, MsgPort. You can only transfer the
|||	    ownership of semaphores if they are currently unlocked.
|||
|||	  Arguments
|||
|||	    i                           The item number of the item to give to
|||	                                a new owner.
|||
|||	    newOwner                    The item number of the new owner task.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if ownership of the item is changed or an
|||	    error code if an error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    item.h                      ARM C "swi" declaration
|||
|||	  Caveats
|||
|||	    This is not implemented for all items that should be able to have
|||	    their ownership changed.
|||
|||	  See Also
|||
|||	    CreateItem(), FindItem(), DeleteItem()
|||
**/

Err
internalSetItemOwner(it,newOwner,t)
Item it;
Item newOwner;
Task *t;
{
	int32 ret;
	ItemNode *n = (ItemNode *)LookupItem(it);
	Task *newOwnerp = (Task *)CheckItem(newOwner,KERNELNODE,TASKNODE);
	Folio *f;
	ItemRoutines *ir;
	int32 slot;

	DBUGSIO(("internalSetItemOwner(%d,%lx) n=%lx\n",it,(uint32)t,(uint32)n));
	DBUGSIO(("(task is %s, object is %s, newowner is %s)\n",t->t.n_Name,n->n_Name,newOwnerp->t.n_Name));
	if ((!newOwnerp) || (!n))	return BADITEM;

	/* should we make sure a task is not trying to own itself? */
	/* maybe this should be allowed!? */

	if (BadPriv(n,t)) return BADPRIV;

	f = WhichFolio(MKNODEID(n->n_SubsysType,n->n_Type));
	DBUGSIO(("internalSetItemOwner, f=%lx\n",(uint32)f));

	ir = f->f_ItemRoutines;
	if (ir->ir_SetOwner == 0)	return  NOSUPPORT;

	slot = AllocItemSlot(newOwnerp);
	if (slot < 0)
	    return MAKEKERR(ER_KYAGB,ER_C_NSTND,ER_Kr_RsrcTblOvfFlw);

	ret = (*ir->ir_SetOwner)(n,newOwner,t);

	if (ret >= 0)
	{
	    /* operation successful, now fix the resource table */
	    oldRemoveItem(t,it);
	    n->n_Owner = newOwner;
	    newOwnerp->t_ResourceTable[slot] = it; /* put in resource table */
	}
	else
	{
	    FreeItemSlot(newOwnerp,slot);
	}

	return ret;
}

/**
|||	AUTODOC PUBLIC spg/kernel/closeitem
|||	CloseItem - Close a system item.
|||
|||	  Synopsis
|||
|||	    Err CloseItem( Item i )
|||
|||	  Description
|||
|||	    System items are items that are created automatically by the
|||	    operating system, such as folios and device drivers, and thus do
|||	    not need to be created by tasks.  To use a system item, a task
|||	    first opens it by calling OpenItem(). When a task is done using a
|||	    system item, it calls CloseItem() to inform the operating system
|||	    that it is no longer using the item.
|||
|||	  Arguments
|||
|||	    i                           Number of the item to close.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if the item was closed successfully or an
|||	    error code (a negative value) if an error occurs.  Possible error
|||	    codes include:
|||
|||	    BADITEM                     The i argument is not an item.
|||
|||	    ER_Kr_ItemNotOpen           The i argument is not an opened item.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    item.h                      ARM C "swi" declaration
|||
|||	  See Also
|||
|||	    OpenItem()
|||
**/

int32
internalCloseItem(it,t)
Item it;
Task *t;
{
	int32 ret = 0;
	Node *n = (Node *)LookupItem(it);
	Folio *f;
	int32 slot;
	ItemRoutines *ir;

        slot = FindItemSlot(t,it|ITEM_WAS_OPENED);

	if (!n)	{
	    if(slot >= 0) {		/* if item is open but doesn't exist */
		FreeItemSlot(t,slot);
		return ret;
	    }
	    return BADITEM;
	}
	if (slot < 0) return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_ItemNotOpen);

	f = WhichFolio(MKNODEID(n->n_SubsysType,n->n_Type));

	ir = f->f_ItemRoutines;
	if (ir->ir_Close == 0)
	{
#ifdef DEVELOPMENT
	    printf("panic: bad ir_Close in CloseItem\n");
	    while (1);
#else
	    Panic(1,"bad ir_Close\n");
#endif

	}
	ret = (*ir->ir_Close)(it,t);

	if (ret >= 0)
	    FreeItemSlot(t,slot);

	return ret;
}

static int32
icki_c(n, p, tag, arg)
Node *n;
void *p;
uint32 tag;
uint32 arg;
{
	return 0;
}

/**
|||	AUTODOC PUBLIC spg/kernel/createitem
|||	CreateItem - Create an item.
|||
|||	  Synopsis
|||
|||	    Item CreateItem( int32 ct, TagArg *p )
|||
|||	    Item CreateItemVA( int32 ct, uint32 tags, ... );
|||
|||	  Description
|||
|||	    This macro creates an item.
|||
|||	    Note:  There are convenience procedures for creating most types of
|||	    items (such as CreateMsg() to create a message and CreateIOReq()
|||	    to create an I/O request).  You should use CreateItem() only when
|||	    no convenience procedure is available or when you need to supply
|||	    additional arguments for the creation of the item beyond what the
|||	    convenience routine provides.
|||
|||	  Arguments
|||
|||	    ct                          Specifies the type of item to create.
|||	                                Use MkNodeID() to generate this value.
|||
|||	    p                           A pointer to an array of tag
|||	                                arguments.  The tag arguments can be
|||	                                in any order.  The last element of the
|||	                                array must be the value TAG_END.  If
|||	                                there are no tag arguments, this
|||	                                argument must be NULL.  For a list of
|||	                                tag arguments for each item type, see
|||	                                the `Portfolio Items' chapter.
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the new item or an error
|||	    code if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in item.h V20.
|||
|||	  Associated Files
|||
|||	    item.h                      ANSI C Macro Definition
|||
|||	  Notes
|||
|||	    When you no longer need an item created with CreateItem(), use
|||	    DeleteItem() to
|||
|||	    delete it.
|||
|||	  See Also
|||
|||	    CheckItem(), CreateDevice(), CreateIOReq(), CreateMsg(),
|||	    CreateMsgPort(), CreateSemaphore(), CreateSmallMsg(),
|||	    CreateThread(), DeleteItem()
|||
**/

/* CreateSizedItem works this way:
	Someone calls CreateSizedItem with 3 parameters:
	cntype (complex node type) which is 8bits of folio and 8bits of type info.
	args which is some unnamed ptr that is just passed on.
	size which must be greater or equal to the smallest size for that node.
	    as seen in the NodeTable attached to the Folio.
	CreateSizedItem decodes the cntype into its two parts, the folio ID and
	the type field.
	If that folio does not exist it returns an error.
	It then checks to see if the type is within range for that folio.
	It then allocated the ItemNode and preinitializes the Item structures.
	Setting up size field if known, allocating an Item Number, etc.
	If there is some sort of problem it returns an error.
	If it gets this far then the individual CreateItem routine is called for
	that folio with a ptr to the Node and the args passed in.
	The routine must return either an error or the Item Number that corresponds
	to the ItemNode. Usually you just pass back n->n_Item.
	If a valid Item Number is passed back then the Item is installed in
	the Current Tasks Resource Table, to be deleted if the Task ever dies
	or if the task specifically requests it to be delete.
*/

Item
internalCreateSizedItem(cntype, args, size)
int32 cntype;
void *args;
int32 size;
{
	Item ret;
	Folio *f;
	uint8 ntype = NODEPART( cntype );
	ItemNode *n;
	ItemRoutines *ir;
	ItemNode in;
	uint32 cuflag;
	int32 slot;

	DBUGCI(("CreateSizedItem(%lx) size=%d\n",(uint32)cntype,size));
	DBUGCI(("args=%lx\n",args));

	f = WhichFolio(cntype);
	DBUGCI(("WhichFolio returns:%lx\n",f));
	if (f == 0)	return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadType);

	if (ntype > f->f_MaxNodeType)
	    return BADSUBTYPE;

	ir = f->f_ItemRoutines;
	if (ir->ir_Create == 0)
	{
#ifdef DEVELOPMENT
	    printf("bad folio!!, ir_Create is  0 in CreateItem\n");
	    while (1);
#else
	Panic(1,"ir_Create is 0\n");
#endif

	}

	/* memset(&in, 0, sizeof(inode)); */
	in.n_Name = NULL;
	in.n_ItemFlags = 0;

	ret = TagProcessorNoAlloc(&in, (TagArg *)args, icki_c, 0);
	if (ret < 0) return ret;

	cuflag = (in.n_ItemFlags & (uint8)ITEMNODE_UNIQUE_NAME);

	if (cuflag)
	{
	    /* Uniqe name can't be NULL */
	    if (in.n_Name == NULL)
	    {
		return BADNAME;
	    }
	    /* Don't support unique-named item for */
	    /* folios without find routine */
	    if (ir->ir_Find == NULL)
	    {
		return NOSUPPORT;
	    }
	}

	if (ir->ir_Find)
	{
	    TagArg tags[2];

	    tags[0].ta_Tag = TAG_ITEM_NAME;
	    tags[0].ta_Arg = in.n_Name;
	    tags[1].ta_Tag = TAG_END;

	    ret = (*ir->ir_Find)(NODEPART(cntype), tags);

	    /* For now, we don't worry about ITEMNODE_NOTREADY as    */
	    /* there is currently no folio with ir_Find support that */
	    /* has context-switching code in its CreateItem routine  */

	    if (ret >= 0)
	    {
		ItemNode *tn = (ItemNode *)LookupItem(ret);

		if (cuflag || (tn->n_ItemFlags & ITEMNODE_UNIQUE_NAME))
		{
		    return MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_Kr_UniqueItemExists);
		}
	    }
	}

	n = (ItemNode *)AllocateSizedNode(f, ntype, size);

	if (n == 0)	return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoMem);


	if ((int)n > 0)	/* valid item pointer? */
		n->n_ItemFlags |= ITEMNODE_NOTREADY;	/* prevent item from being found */

        slot = AllocItemSlot(CURRENTTASK);
        if (slot < 0)
        {
            if ((int32)n > 0)
                FreeNode(f,n);

            return MAKEKERR(ER_KYAGB,ER_C_NSTND,ER_Kr_RsrcTblOvfFlw);
        }

	DBUGCI(("f_CreateItem=%lx\n",(uint32)ir->ir_Create));
	ret = (*ir->ir_Create)(n,ntype,args);

	DBUGCI(("ret = %d\n",ret));
	if (ret >= 0) {
		DBUGCI(("ct=%lx\n",KernelBase->kb_CurrentTask));
                /* set the n_Owner field */
                DBUGCI(("%lx Setting Owner to %lx\n",n,KernelBase->kb_CurrentTask->t.n_Item));
                n = (ItemNode *)LookupItem(ret);
                n->n_Owner = KernelBase->kb_CurrentTask->t.n_Item;

                /* insert in the task's resource table */
                CURRENTTASK->t_ResourceTable[slot] = ret;

		if (cuflag) n->n_ItemFlags |= ITEMNODE_UNIQUE_NAME;
		n->n_ItemFlags &= ~ITEMNODE_NOTREADY;	/* allow item to be found and opened */
	}
	else
	{
		FreeItemSlot(CURRENTTASK,slot);

		if ((int32)n > 0)
		{
                    FreeNode(f,n);
                }
	}
	DBUGCI(("CreateItem returns: %d\n",ret));
	PAUSE;
	return ret;
}

Err
internalSetOwnerKernelItem(n,newowner,oldowner)
ItemNode *n;
Item newowner;
Task *oldowner;
{
Task *newownerP;

	/*Item *ip;*/
	/* We know that oldowner does in fact own */
	/* this Item */
	uint8 ntype = n->n_Type;
	switch (ntype)
	{
	    case DEVICENODE   : return 0;
	    case TASKNODE     : return internalSetTaskOwner((Task *)n,newowner);

	    case ERRORTEXTNODE:
            case FIRQNODE     :
            case TIMERNODE    :
            case DRIVERNODE   :
            case FOLIONODE    : newownerP = (Task *)LookupItem(newowner);
                                if (IsSameTaskContext(newownerP,oldowner))
                                {
                                    /* only allow the ownership to change amongst threads of a same task */
                                    return 0;
                                }
                                return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantSetOwner);

            case IOREQNODE    : return SetIOReqOwner((IOReq *)n,newowner);
            case SEMA4NODE    : return internalSetSemaphoreOwner((Semaphore *)n,newowner);
            default           : return NOSUPPORT;
        }
}

int32
internalSetPriorityKernelItem(ItemNode *n,uint8 pri,Task *t)
{
	uint8 ntype = n->n_Type;
	if (ntype == TASKNODE)	return internalChangeTaskPri((Task *)n,pri);
	else return MAKEKERR(ER_SEVER,ER_C_STND,ER_BadSubType);
}

Item
internalOpenKernelItem(n,a)
Node *n;
void *a;
{
	uint8 ntype = n->n_Type;
	DBUGOI(("OKI, ntype=%d n=%lx\n",ntype,n));
	if (a != 0)
	{
		printf("2nd parameter to OpenItem must be zero!\n");
	        return MAKEKERR(ER_SEVER,ER_C_STND,ER_BadPtr);
	}
	if (ntype == FOLIONODE)	return OpenFolio((Folio *)n,a);
	else if (ntype == DRIVERNODE) return OpenDriver((Driver *)n,a);
	else if (ntype == DEVICENODE) return OpenDevice((Device *)n,a);
	else return MAKEKERR(ER_SEVER,ER_C_STND,ER_BadSubType);
}

extern int32 CloseFolio(Folio *, Task *);

int32 internalCloseKernelItem(it,ct)
Item it;
Task *ct;
{
	/* CloseItem has already done pre verification */
	/* We know it is already opened */
	Node *n = (Node *)LookupItem(it);

	if (n->n_Type == FOLIONODE)
		return CloseFolio((Folio *)n,ct);
	else if (n->n_Type == DRIVERNODE)
		return CloseDriver((struct Driver *)n,ct);
	/* really a default */
	else /*if (n->n_Type == DEVICENODE)*/
		return CloseDevice((struct Device *)n,ct);
}

int32
internalDeleteKernelItem(it,t)
Item it;
Task *t;
{
	Node *n = (Node *)LookupItem(it);
	switch (n->n_Type)
	{
	    case TASKNODE:
		return internalKill((struct Task *)n,t);
	    case SEMA4NODE:
		return internalDeleteSemaphore((struct Semaphore *)n,t);
	    case FOLIONODE:
		return internalDeleteFolio((struct Folio *)n,t);
	    case MSGPORTNODE:
		return internalDeleteMsgPort((struct MsgPort *)n,t);
	    case MESSAGENODE:
		return internalDeleteMsg((struct Msg *)n,t);
	    case FIRQNODE:
		return internalDeleteFirq((struct FirqNode *)n,t);
	    case IOREQNODE:
		return internalDeleteIOReq((struct IOReq *)n,t);
	    case DRIVERNODE:
		return internalDeleteDriver((struct Driver *)n,t);
	    case DEVICENODE:
		return internalDeleteDevice((struct Device *)n,t);
	    case TIMERNODE:
		return internalDeleteTimer((struct Timer *)n,t);
	    case ERRORTEXTNODE:
		return internalDeleteErrorText((ErrorText *)n,t);
	    case HLINTNODE:
		return internalDeleteHLInt((struct FirqNode *)n,t);
	}
	/* should not get here! */
	return MAKEKERR(ER_SEVER,ER_C_STND,ER_SoftErr);
}

extern Item internalFindTask(char *);

Item
NodeToItem(n)
Node *n;
{
	struct ItemNode *in = (struct ItemNode *)n;
	if (!n)	return -1;
	if (n->n_Flags & NODE_ITEMVALID)	return in->n_Item;
	return -1;
}


/*****************************************************************************/


/* load a kernel item from disk */
Item internalLoadKernelItem(int32 ntype, TagArg *tags)
{
ItemNode inode;
Item     result;

    memset(&inode,0,sizeof(inode));

    result = TagProcessorNoAlloc(&inode,tags,NULL,0);
    if (result < 0)
        return result;

    if (inode.n_Name == NULL)
        return MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);

    switch (ntype)
    {
        case FOLIONODE : result = internalLoadFolio(inode.n_Name);
                         break;

        case DEVICENODE: result = internalLoadDevice(inode.n_Name);
                         break;

        case DRIVERNODE: result = internalLoadDriver(inode.n_Name);
                         break;

        default        : result = BADSUBTYPE;
    }

    return result;
}


/*****************************************************************************/


Item
internalFindKernelItem(ntype,tagpt)
int32 ntype;
TagArg *tagpt;
{
	List *l = 0;
	ItemNode inode;
	Item ret;

	memset(&inode,0,sizeof(inode));

	ret = TagProcessorNoAlloc(&inode,tagpt,NULL,0);
	if (ret < 0)	return ret;
	if (inode.n_Name == 0)	return MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);

	if (ntype == TASKNODE)
	{
		ret = internalFindTask(inode.n_Name);
		goto done;
	}
	switch (ntype)
	{
	    case SEMA4NODE:	l = KernelBase->kb_Semaphores; break;
	    case FOLIONODE: 	l = KernelBase->kb_FolioList; break;
	    case DRIVERNODE:	l = KernelBase->kb_Drivers; break;
	    case DEVICENODE:	l = KernelBase->kb_Devices; break;
	    case MSGPORTNODE:	l = KernelBase->kb_MsgPorts; break;
	}
	if (l)
	{
		ret = NodeToItem(FindNamedNode(l,inode.n_Name));
		DBUG(("FindKernelNode list=%s\n",l->l.n_Name));
		if (ret>=0)	goto done;
		ret = MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);
		goto done;
	}
	DBUG(("FindKernelNode, no list to search\n"));

	ret = BADSUBTYPE;
done:
	return ret;
}

Item
internalFindItemFromItemTable(f,ntype,tagpt)
Folio	*f;
int8	ntype;
TagArg	*tagpt;
{
	ItemNode	inode;
	Item		ret;
	uint32		stype = NODETOSUBSYS(f);
	ItemEntry	**it = KernelBase->kb_ItemTable;
	ItemEntry	*ie;

	if (stype & ~NST_SYSITEMMASK)
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);

	memset(&inode,0,sizeof(inode));

	/* Borrow the kernel tag processor for now */
	ret = TagProcessorNoAlloc(&inode,tagpt,NULL,0);
	if (ret < 0)	return ret;
	if (inode.n_Name == 0)	return MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);

	for (ie = *it; ie != NULL; ie = *++it)
	{
	    ItemEntry *eie = ie + ITEMS_PER_BLOCK;

	    for (; ie < eie; ie++)
	    {
		ItemNode    *in = (ItemNode *)ie->ie_ItemAddr;
		char	    *name;

		if (in == 0)
			continue;

		if ((in->n_SubsysType != stype) || (in->n_Type != ntype))
			continue;
		name = in->n_Name;
		if (name == NULL)
			continue;
		if (strcasecmp(name, inode.n_Name) == 0)
		{
			ret = in->n_Item;
			goto done;
		}
	    }
	}
	ret = MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);

done:
	return ret;
}

/**
|||	AUTODOC PUBLIC spg/kernel/finditem
|||	FindItem - Find an item by type and tags.
|||
|||	  Synopsis
|||
|||	    Item FindItem( int32 cType, TagArg *tp )
|||
|||	    Item FindItemVA( int32 cType, uint32 tags, ...)
|||
|||	  Description
|||
|||	    This procedure finds an item of the specified type whose tag
|||	    arguments match those pointed to by the tp argument.  If more than
|||	    one item of the specified type has matching tag arguments, the
|||	    procedure returns the item number for the first matching item.
|||
|||	  Arguments
|||
|||	    cType                       Specifies the type of the item to
|||	                                find.  Use MkNodeID() to create this
|||	                                value.
|||
|||	    tp                          A pointer to an array of tag
|||	                                arguments.  The tag arguments can be
|||	                                in any order.  The array can contain
|||	                                some, all, or none of the possible tag
|||	                                arguments for an item of the specified
|||	                                type; to make the search more
|||	                                specific, include more tag arguments.
|||	                                The last element of the array must be
|||	                                the value TAG_END.  If there are no
|||	                                tag arguments, this argument must be
|||	                                NULL.  For a list of tag arguments for
|||	                                each item type, see the `Portfolio
|||	                                Items' chapter.
|||
|||	  Return Value
|||
|||	    The procedure returns the number of the first item that matches or
|||	    an error code if it can't find the item of if an error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    item.h                      ARM C "swi" declaration
|||
|||	  Caveats
|||
|||	    The procedure currently ignores most tag arguments.
|||
|||	  See Also
|||
|||	    CheckItem(), FindNamedItem(), FindVersionedItem(), LookupItem(),
|||
|||	    MkNodeID()
|||
**/

Item
internalFindItem(cntype,tagpt)
int32 cntype;
TagArg *tagpt;
{
	Folio *f;
	ItemRoutines *ir;
	Item ret;
	ItemNode *n;

	f = WhichFolio(cntype);
	if (f == 0)	return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadType);
	ir = f->f_ItemRoutines;
	if (ir->ir_Find == 0)
		ret = internalFindItemFromItemTable(f,NODEPART(cntype),tagpt);
	else ret = (*ir->ir_Find)(NODEPART(cntype),tagpt);
	if (ret >= 0)
	{
	    n=(ItemNode *)LookupItem(ret);
	    if(n && (n->n_ItemFlags & ITEMNODE_NOTREADY))
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);
	}
	return ret;
}

/**
|||	AUTODOC PUBLIC spg/kernel/deleteitem
|||	DeleteItem - Delete an item.
|||
|||	  Synopsis
|||
|||	    Err DeleteItem( Item i )
|||
|||	  Description
|||
|||	    This procedure deletes the specified item and frees any resources
|||	    (including memory) that were allocated for the item.
|||
|||	    Note:  There are convenience procedures for deleting most types of
|||	    items (such as DeleteMsg() to delete a message and DeleteIOReq()
|||	    to delete an I/O request).  You should use DeleteItem() only if
|||	    you used CreateItem() to create the item. If you used a
|||	    convenience routine to create an item, you must use the
|||	    corresponding convenience routine to delete the item.
|||
|||	  Arguments
|||
|||	    i                           Number of the item to be deleted.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if successful or an error code if an error
|||	    occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    item.h                      ARM C "swi" declaration
|||
|||	  Notes
|||
|||	    The item number of a deleted item is not reused.  If a task
|||	    specifies the item number of a deleted item, the Kernel informs
|||	    the task that the item no longer exists.
|||
|||	    Tasks can only delete items that they own.  If a task transfers
|||	    ownership of an item to another task, it can no longer delete the
|||	    item.  The lone exception to this is the task itself.  You can
|||	    always commit sepaku..
|||
|||	    When a task dies, the Kernel automatically deletes all of the
|||	    items in its resource table.
|||
|||	  See Also
|||
|||	    CheckItem(), CreateItem(), DeleteIOReq(), DeleteMsg(),
|||	    DeleteMsgPort(), DeleteThread(), exit()
|||
**/

int32
internalDeleteItem(it,t)
Item it;
Task *t;
{
	int32 ret;
	ItemNode *n = (ItemNode *)LookupItem(it);
	Folio *f;
	ItemRoutines *ir;


	DBUGDI(("internalDeleteItem(%d,%lx) n=%lx\n",it,(uint32)t,(uint32)n));
	if (!n)	return BADITEM;

	if (BadPriv(n,t)) return BADPRIV;

	f = WhichFolio(MKNODEID(n->n_SubsysType,n->n_Type));
	DBUGDI(("internalDeleteItem, f=%lx\n",(uint32)f));

	ir = f->f_ItemRoutines;

	if (ir->ir_Delete == 0)
	{
#ifdef DEVELOPMENT
	    printf("Bad ir_Delete in DeleteItem\n");
	    while (1);
#else
	    Panic(1,"Bad ir_Delete\n");
#endif

	}

	ret = (*ir->ir_Delete)(it,t);

	if (ret > 0)
	{
		/* Don't remove/free the item */
		ret = 0;
	}
	else
	if (ret == 0)
	{
	        /* This is resampled here because something wierd is
	         * done inside of DeleteFolio() with swizzling the address
	         * assigned to items.
	         */
	        n = (ItemNode *)LookupItem(it);
		RemoveItem(it);
		FreeNode(f, n);
	}

	DBUGDI(("internalDeleteItem, ret=%d\n",ret));

	return ret;
}


/* delete the item no matter who owns it */
int32
superinternalDeleteItem(Item i)
{
	ItemNode *n = (ItemNode *)LookupItem(i);
	Task *t;

	DBUGDI(("superinternalDeleteItem(%d,%lx) n=%lx\n",i,(uint32)t,(uint32)n));
#ifdef	DEVELOPMENT
	if (!n)	return BADITEM;
#endif
	t = (Task *)LookupItem(n->n_Owner);
#ifdef	DEVELOPMENT
	if (!t) return BADITEM;
#endif
	return internalDeleteItem(i,t);
}

int32
externalDeleteItem(i)
Item i;
{
	return internalDeleteItem(i,KernelBase->kb_CurrentTask);
}

int32 externalCloseItem(i)
Item i;
{
	return internalCloseItem(i,KernelBase->kb_CurrentTask);
}

int32 externalSetItemPriority(Item i,uint8 pri)
{
	return internalSetItemPriority(i,pri,KernelBase->kb_CurrentTask);
}

Err externalSetItemOwner(i,newOwner)
Item i;
Item newOwner;
{
	return internalSetItemOwner(i,newOwner,KernelBase->kb_CurrentTask);
}

/**
|||	AUTODOC PUBLIC spg/kernel/isitemopened
|||	IsItemOpened - Determine whether a task or thread has opened a given
|||	               item.
|||
|||	  Synopsis
|||
|||	    Err IsItemOpened( Item task, Item i )
|||
|||	  Description
|||
|||	    This function determines whether a task or thread has
|||	    currently got an item opened.
|||
|||	  Arguments
|||
|||	    task                        The task or thread to inquire
|||	                                about. For the current task, use
|||	                                KernelBase->kb_CurrentTask->t.n_Item
|||
|||	    i                           The number of the item to verify.
|||
|||	  Return Value
|||
|||	    This function returns >= 0 if the item was opened, or a negative
|||	    error code if it is not, or if the parameters are bogus.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V24.
|||
|||	  Associated Files
|||
|||	    item.h                      ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  See Also
|||
|||	    OpenItem(), CloseItem(), CheckItem(), LookupItem()
|||
**/

Err ItemOpened(Item task, Item i)
{
    Task *t = (Task *)CheckItem(task, KERNELNODE,TASKNODE);
    if (t == 0)	return BADITEM;
    if (FindItemSlot(t, i | ITEM_WAS_OPENED) < 0)
	return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_ItemNotOpen);
    return 0;
}


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/kernel/findandopenitem
|||	FindAndOpenItem - Find an item by type and tags and open it.
|||
|||	  Synopsis
|||
|||	    Item FindAndOpenItem( int32 cType, TagArg *tp )
|||
|||	    Item FindAndOpenItemVA( int32 cType, uint32 tags, ... );
|||
|||	  Description
|||
|||	    This procedure finds an item of the specified type whose tag
|||	    arguments match those pointed to by the tp argument.  If more than
|||	    one item of the specified type has matching tag arguments, the
|||	    procedure returns the item number for the first matching item.
|||	    When an item is found, it is automatically opened and prepared
|||	    for use.
|||
|||	  Arguments
|||
|||	    cType                       Specifies the type of the item to
|||	                                find.  Use MkNodeID() to create this
|||	                                value.
|||
|||	    tp                          A pointer to an array of tag
|||	                                arguments.  The tag arguments can be
|||	                                in any order.  The array can contain
|||	                                some, all, or none of the possible tag
|||	                                arguments for an item of the specified
|||	                                type; to make the search more
|||	                                specific, include more tag arguments.
|||	                                The last element of the array must be
|||	                                the value TAG_END.  If there are no
|||	                                tag arguments, this argument must be
|||	                                NULL.  For a list of tag arguments for
|||	                                each item type, see the `Portfolio
|||	                                Items' chapter.
|||
|||	  Return Value
|||
|||	    The procedure returns the number of the first item that matches or
|||	    an error code if it can't find the item of if an error occurs.
|||	    The returned item is already opened, so there is no need to
|||	    call OpenItem() on it. You should call CloseItem() on the
|||	    supplied item when you are done with it.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V24.
|||
|||	  Associated Files
|||
|||	    item.h                      ARM C "swi" declaration
|||
|||	  Notes
|||
|||	    To close an opened item, use CloseItem().
|||
|||	  See Also
|||
|||	    CheckItem(), FindNamedItem(), FindVersionedItem(), LookupItem(),
|||         MkNodeID(), OpenItem(), FindItem()
|||
**/

Item externalFindAndOpenItem(int32 cntype, TagArg *tags)
{
uint8         ntype;
Folio        *f;
ItemRoutines *ir;
Item          result;

    /* try and find it in memory first */
    result = internalFindItem(cntype,tags);
    if (result >= 0)
    {
        /* if it was found, then open it */
        return (internalOpenItem(result,NULL));
    }

    /* see what folio we need to talk to for this item type */
    f = WhichFolio(cntype);
    if (f == NULL)
    {
        /* unknown item type */
        return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadType);
    }

    ntype = NODEPART(cntype);
    if ((ntype < 0) || (ntype > f->f_MaxNodeType))
    {
        /* being asked to access an item type that the folio doesn't know about */
        return BADSUBTYPE;
    }

    ir = f->f_ItemRoutines;
    if (ir->ir_Load == NULL)
    {
        /* no load vector for this folio, so fail... */
        return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantOpen);
    }

    /* load it! */
    result = (*ir->ir_Load)(ntype,tags);
    if (result >= 0)
    {
        /* if it was loaded, then open it */
        result = internalOpenItem(result,NULL);
    }

    return result;
}
