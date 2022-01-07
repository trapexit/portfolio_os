/* $Id: getnodeposfromhead.c,v 1.5 1994/09/10 02:52:22 vertex Exp $ */

#include "types.h"
#include "nodes.h"
#include "list.h"


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/kernel/getnodeposfromhead
|||	GetNodePosFromHead - Get the ordinal position of a node within a
|||	                     list, counting from the head of the list.
|||
|||	  Synopsis
|||
|||	    int32 GetNodePosFromHead(const List *l, const Node *n);
|||
|||	  Description
|||
|||	    This function scans the supplied list looking for the supplied
|||	    node, and returns the ordinal position of the node within the
|||	    list. If the node doesn't appear in the list, the function returns
|||	    -1.
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
|||	    the head of the list, or -1 if the node isn't in the list. The
|||	    first node in the list has position 0, the second node has position
|||	    1, etc.
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

int32 GetNodePosFromHead(const List *l, const Node *n)
{
Node   *nn;
uint32  count;

    count = 0;
    SCANLIST(l,nn,Node)
    {
        if (n == nn)
            return (int32)count;

        count++;
    }

    return -1;
}
