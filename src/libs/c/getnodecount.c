/* $Id: getnodecount.c,v 1.4 1994/09/10 02:52:22 vertex Exp $ */

#include "types.h"
#include "nodes.h"
#include "list.h"


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/kernel/getnodecount
|||	GetNodeCount - Count the number of nodes in a list.
|||
|||	  Synopsis
|||
|||	    uint32 GetNodeCount(const List *l);
|||
|||	  Description
|||
|||	    This function counts the number of nodes currently in the
|||	    list.
|||
|||	  Arguments
|||
|||	    l                           A pointer to the list to count the
|||	                                nodes of.
|||
|||	  Return Value
|||
|||	    The number of nodes in the list.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V24.
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
|||	    RemHead(), RemNode(), RemTail()
|||
**/

uint32 GetNodeCount(const List *l)
{
uint32  result;
Node   *n;

    result = 0;
    SCANLIST(l,n,Node)
    {
        result++;
    }

    return result;
}
