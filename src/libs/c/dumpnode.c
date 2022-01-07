/* $Id: dumpnode.c,v 1.4 1994/09/21 19:57:26 vertex Exp $ */

#include "types.h"
#include "nodes.h"
#include "list.h"
#include "stdio.h"


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/clib/dumpnode
|||	DumpNode - Print contents of a node.
|||
|||	  Synopsis
|||
|||	    void DumpNode (const Node *node, const char *banner)
|||
|||	  Description
|||
|||	    This function prints out the contents of a Node structure for
|||	    debugging purposes.
|||
|||	  Arguments
|||
|||	    node                        The node to print.
|||
|||	    banner                      Descriptive text to print before the
|||	                                node contents. May be NULL.
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
**/

void DumpNode(const Node *n, const char *banner)
{
    if (banner)
	printf("%s: ",banner);

    printf("node @ $%08lx, subsys %u, type %u, ", n, n->n_SubsysType, n->n_Type);
    printf("size %d, pri %u, ",n->n_Size, n->n_Priority);

    if (n->n_Flags & NODE_NAMEVALID)
    {
        if (n->n_Name)
            printf("name '%s'\n",n->n_Name);
        else
            printf("<null name>\n");
    }
    else
    {
        printf("<unnamed>\n");
    }
}
