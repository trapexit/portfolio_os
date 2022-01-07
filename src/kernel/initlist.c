/* $Id: initlist.c,v 1.13 1994/09/15 19:01:02 vertex Exp $ */
#include "types.h"
#include "kernelnodes.h"
#include "list.h"

static const char ListName[] = "List";

/**
|||	AUTODOC PUBLIC spg/kernel/initlist
|||	InitList - Initialize a list.
|||
|||	  Synopsis
|||
|||	    void InitList( List *l, const char *name )
|||
|||	  Description
|||
|||	    When you create a List structure, you must initialize it with a
|||	    call to InitList() before using it.  InitList() creates an empty
|||	    list by initializing the head (beginning-of-list) and tail
|||	    (end-of-list) anchors and by providing a name for a list.
|||
|||	  Arguments
|||
|||	    l                           A pointer to the list to be
|||	                                initialized.
|||
|||	    name                        The name of the list, or NULL to
|||	                                get the default name.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    list.h                      ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Caveats
|||
|||	    GIGO (`garbage in, garbage out')
|||
|||	  See Also
|||
|||	    AddHead(), AddTail(), InsertNodeFromHead(), InsertNodeFromTail(),
|||	    IsEmptyList(), RemHead(), RemNode(), RemTail(),
|||	    UniversalInsertNode()
|||
**/

void
InitList(List *l, const char *s)
{
    l->l.n_Type = LISTNODE;
    l->l.n_SubsysType = KERNELNODE;
    l->l.n_Size = sizeof(List);
    l->l.n_Flags = NODE_NAMEVALID;

    if (!s)
        s = ListName;

    l->l.n_Name = (char *)s;

    l->ListAnchor.head.links.blink = 0;
    l->ListAnchor.head.links.flink = (Link*)(&l->ListAnchor.tail.links.flink);
    l->ListAnchor.tail.links.blink = (Link*)(&l->ListAnchor.head.links.flink);
}
