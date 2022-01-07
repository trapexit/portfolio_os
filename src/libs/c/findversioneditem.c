/* $Id: findversioneditem.c,v 1.5 1994/09/10 02:52:22 vertex Exp $ */
#include "types.h"
#include "nodes.h"
#include "item.h"

/**
|||	AUTODOC PUBLIC spg/kernel/findversioneditem
|||	FindVersionedItem - Find an item by name and version.
|||
|||	  Synopsis
|||
|||	    Item FindVersionedItem( int32 ctype, const char *name, uint8 vers,
|||
|||	    uint8 rev )
|||
|||	  Description
|||
|||	    This procedure finds an item of a specified type by its name,
|||	    version number, and revision number.  These values are required
|||	    tag arguments for all items.  If all three values match, the
|||	    procedure returns the item number of the matching item.
|||
|||	  Arguments
|||
|||	    ctype                       The type of the item to find.  Use
|||	                                MkNodeID() to create this value.
|||
|||	    name                        The name of the item to find.
|||
|||	    vers                        The version number of the item to
|||	                                find.
|||
|||	    rev                         The revision number of the item to
|||	                                find.
|||
|||	  Return Value
|||
|||	    The procedure returns the number of the item that matches or an
|||	    error code if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V20.
|||
|||	  Associated Files
|||
|||	    item.h                      ANSI C P        rototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Caveats
|||
|||	    This currently returns a match if the version number and revision
|||	    number are greater than or equal to the values specified.
|||
|||	  See Also
|||
|||	    CheckItem(), FindItem(), FindNamedItem(), LookupItem()
|||
**/

Item FindVersionedItem(int32 cntype, const char *name, uint8 vers, uint8 rev)
{
	TagArg	tags[4];
	tags[0].ta_Tag = TAG_ITEM_NAME;
	tags[0].ta_Arg = (void *)name;
	tags[1].ta_Tag = TAG_ITEM_VERSION;
	tags[1].ta_Arg = (void *)vers;
	tags[2].ta_Tag = TAG_ITEM_REVISION;
	tags[2].ta_Arg = (void *)rev;
	tags[3].ta_Tag = 0;
	return FindItem(cntype,tags);
}
