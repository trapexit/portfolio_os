/* $Id: */

#include "types.h"
#include "nodes.h"
#include "list.h"


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/kernel/getnodeposfromtail
|||	GetNodePosFromTail - Get the ordinal position of a node within a
|||	                     list, counting from the tail of the list.
|||
|||	  Synopsis
|||
|||	    int32 GetNodePosFromTail(const List *l, const Node *n);
|||
|||	  Description
|||
|||	    This function scans the supplied list backward looking for the
|||	    supplied node, and returns the ordinal position of the node within
|||	    the  list. If the node doesn't appear in the list, the function
|||	    returns -1.
|||
|||	  Arguments
|||
|||	    l                           A pointer to the list to scan for
|||	                                the node.
|||
|||	    n                           A pointer to a node to locate in
|||	                                the list.
|||
|||	  Return Value
|||
|||	    The ordinal position of the node within the list counting from
|||	    the tail of the list, or -1 if the node isn't in the list. The
|||	    last node in the list has position 0, the second to last node has
|||	    position 1, etc.
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

int32 GetNodePosFromTail(const List *l, const Node *n)
{
Node   *nn;
uint32  count;

    count = 0;
    SCANLISTB(l,nn,Node)
    {
        if (n == nn)
            return (int32)count;

        count++;
    }

    return -1;
}
