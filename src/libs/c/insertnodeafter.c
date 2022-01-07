/* $Id: insertnodeafter.c,v 1.3 1994/09/10 02:52:22 vertex Exp $ */

#include "types.h"
#include "nodes.h"
#include "list.h"


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/kernel/insertnodeafter
|||	InsertNodeAfter - Insert a node into a list after another node already
|||	                  in the list.
|||
|||	  Synopsis
|||
|||	    void InsertNodeAfter(Node *oldNode, Node *newNode);
|||
|||	  Description
|||
|||	    This function lets you insert a new node into a list, AFTER
|||	    another node that is already in the list.
|||
|||	  Arguments
|||
|||	    oldNode                     The node after which to insert the
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
|||	    RemHead(), RemNode(), RemTail(), InsertNodeBefore()
|||
**/

void InsertNodeAfter(Node *oldNode, Node *newNode)
{
    newNode->n_Prev         = oldNode;
    newNode->n_Next         = oldNode->n_Next;
    oldNode->n_Next->n_Prev = newNode;
    oldNode->n_Next         = newNode;
}
