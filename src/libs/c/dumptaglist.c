/* $Id: dumptaglist.c,v 1.4 1994/10/13 20:17:02 vertex Exp $ */

#include <stdio.h>
#include <tags.h>

/**
|||	AUTODOC PUBLIC spg/kernel/dumptaglist
|||	DumpTagList - Print contents of a tag list.
|||
|||	  Synopsis
|||
|||	    void DumpTagList (const TagArg *tagList, const char *desc)
|||
|||	  Description
|||
|||	    This function prints out the contents of a TagArg list.
|||
|||	  Arguments
|||
|||	    tagList                     The list of tags to print.
|||
|||	    desc                        Description of tag list to print.
|||	                                Can be NULL.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V24.
|||
|||	  Associated Files
|||
|||	    tags.h                      ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  See Also
|||
|||	    NextTagArg(), GetTagArg()
|||
**/
void DumpTagList (const TagArg *tags, const char *desc)
{
    const TagArg *tstate, *t;

    if (desc) printf ("%s: ", desc);
    printf ("tag list @ $%08lx\n", tags);

    for (tstate = tags; (t = NextTagArg (&tstate)) != NULL; ) {
        printf ("{ 0x%02lx (%3lu), 0x%08lx (%ld) }\n", t->ta_Tag, t->ta_Tag, (uint32)t->ta_Arg, (int32)t->ta_Arg);
    }
}
