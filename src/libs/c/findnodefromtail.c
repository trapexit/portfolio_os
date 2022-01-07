/* $Id: findnodefromtail.c,v 1.4 1994/09/10 02:53:59 vertex Exp $ */

#include "types.h"
#include "nodes.h"
#include "list.h"


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/kernel/findnodefromtail
|||	FindNodeFromTail - Return a pointer to a node appearing at a given
|||	                   ordinal position from the tail of the list.
|||
|||	  Synopsis
|||
|||	    Node *FindNodeFromTail(const List *l, uint32 position);
|||
|||	  Description
|||
|||	    This function scans the supplied list and returns a pointer
|||	    to the node appearing in the list at the given ordinal position
|||	    counting from the end of the list. NULL is returned if the list
|||	    doesn't contain that many items.
|||
|||	  Arguments
|||
|||	    l                           A pointer to the list to scan for the
|||	                                node.
|||
|||	    position                    The node position to look for,
|||	                                relative to the end of the list.
|||
|||	  Return Value
|||
|||	    A pointer to the node found, or NULL if the list doesn't contain
|||	    enough nodes.
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

Node *FindNodeFromTail(const List *l, uint32 position)
{
Node *n;

    SCANLISTB(l,n,Node)
    {
        if (!position)
            return n;

        position--;
    }

    return NULL;
}
