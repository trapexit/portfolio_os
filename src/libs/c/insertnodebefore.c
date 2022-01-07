
#include "types.h"
#include "nodes.h"
#include "list.h"


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/kernel/insertnodebefore
|||	InsertNodeBefore - Insert a node into a list before another node
|||	                   already in the list.
|||
|||	  Synopsis
|||
|||	    void InsertNodeBefore(Node *oldNode, Node *newNode);
|||
|||	  Description
|||
|||	    This function lets you insert a new node into a list, BEFORE
|||	    another node that is already in the list.
|||
|||	  Arguments
|||
|||	    oldNode                     The node before which to insert the
|||	                                new node.
|||
|||	    newNode                     The node to insert in the list.
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
|||	  Notes
|||
|||	    A node can be included only in one list.
|||
|||	  Caveats
|||
|||	    GIGO (`garbage in, garbage out')
|||
|||	  See Also
|||
|||	    AddHead(), AddTail(), InsertNodeFromHead(), InsertNodeFromTail(),
|||	    RemHead(), RemNode(), RemTail(), InsertNodeAfter()
|||
**/

void InsertNodeBefore(Node *oldNode, Node *newNode)
{
    newNode->n_Next         = oldNode;
    newNode->n_Prev         = oldNode->n_Prev;
    oldNode->n_Prev->n_Next = newNode;
    oldNode->n_Prev         = newNode;
}
