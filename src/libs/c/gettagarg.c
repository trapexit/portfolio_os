/* $Id: gettagarg.c,v 1.2 1994/10/13 20:17:02 vertex Exp $ */

#include <tags.h>

/**
|||	AUTODOC PUBLIC spg/kernel/gettagarg
|||	GetTagArg - Find TagArg in list and return its ta_Arg field.
|||
|||	  Synopsis
|||
|||	    TagData GetTagArg (const TagArg *tagList, uint32 tag,
|||	                       TagData defaultValue)
|||
|||	  Description
|||
|||	    This function calls FindTagArg() to locate the specified tag.
|||	    If it is found, it returns the ta_Arg value from the found TagArg.
|||	    Otherwise, it returns the default value supplied. This is handy
|||	    when resolving a tag list that has optional tags that have suitable
|||	    default values.
|||
|||	  Arguments
|||
|||	    tagList                     The list of tags to scan. Can be NULL.
|||
|||	    tag                         The tag ID to look for.
|||
|||	    defaultValue                Default value to use when specified tag
|||	                                isn't found in tagList.
|||
|||	  Return Value
|||
|||	    ta_Arg value from found TagArg or defaultValue.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V24.
|||
|||	  Examples
|||
|||	    void dosomething (const TagArg *tags)
|||	    {
|||	        uint32 amplitude = (uint32)GetTagData (tags, MY_TAG_AMPLITUDE, (TagData)0x7fff);
|||	        .
|||	        .
|||	        .
|||	    }
|||
|||	  Caveats
|||
|||	    It's a good idea to always use casts for the default value and
|||	    result. Don't assume anything about the type definition of TagData
|||	    other than it is a 32-bit value.
|||
|||	  Associated Files
|||
|||	    tags.h                      ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  See Also
|||
|||	    FindTagArg(), NextTagArg()
|||
**/
TagData GetTagArg (const TagArg *ta, uint32 tag, TagData defaultval)
{
    return ((ta = FindTagArg (ta, tag)) != NULL) ? ta->ta_Arg : defaultval;
}
