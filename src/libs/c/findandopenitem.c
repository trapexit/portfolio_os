/* $Id: findandopenitem.c,v 1.5 1994/09/10 02:52:22 vertex Exp $ */

#include "types.h"
#include "nodes.h"
#include "item.h"

/**
|||	AUTODOC PUBLIC spg/kernel/findandopendevice
|||	FindAndOpenDevice - Find a device by name and open it.
|||
|||	  Synopsis
|||
|||	    Item FindAndOpenDevice( const char *name )
|||
|||	  Description
|||
|||	    This macro finds a device whose name matches the name argument.
|||	    The search is not case-sensitive (that is, the Kernel does not
|||	    distinguish uppercase and lowercase letters in device names).
|||	    If the device is found, it is opened.
|||
|||	  Arguments
|||
|||	    name                        The name of the device to find.
|||
|||	  Return Value
|||
|||	    The macro returns the item number of the device or an error code
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in device.h V24.
|||
|||	  Associated Files
|||
|||	    device.h                    ANSI C Macro definition
|||
|||	  See Also
|||
|||	    OpenNamedDevice(), FindAndOpenItem()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/findandopenfolio
|||	FindAndOpenFolio - Find a folio by name and open it.
|||
|||	  Synopsis
|||
|||	    Item FindAndOpenFolio( const char *name )
|||
|||	  Description
|||
|||	    This macro finds a folio whose name matches the name argument.
|||	    The search is not case-sensitive (that is, the Kernel does not
|||	    distinguish uppercase and lowercase letters in folio names).  You
|||	    can use this macro in place of FindAndOpenNamedItem() to find a
|||	    folio and open it.
|||
|||	    The names of folios are contained in the nodes.h header file.
|||
|||	  Arguments
|||
|||	    name                        The name of the folio to find.
|||
|||	  Return Value
|||
|||	    The macro returns the item number of the folio or an error
|||	    code if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in folio.h V24.
|||
|||	  Associated Files
|||
|||	    folio.h                     ANSI C Macro definition
|||
|||	  See Also
|||
|||	    CallFolio(), CallFolioRet(), FindAndOpenNamedItem()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/findandopennameditem
|||	FindAndOpenNamedItem - Find an item by name and open it.
|||
|||	  Synopsis
|||
|||	    Item FindAndOpenNamedItem( int32 ctype, const char *name )
|||
|||	  Description
|||
|||	    This procedure finds an item of the specified type whose name
|||	    matches the name argument.  The search is not case-sensitive (that
|||	    is, the Kernel does not distinguish uppercase and lowercase
|||	    letters in item names). When an item is found, it is automatically
|||	    opened and prepared for use.
|||
|||	  Arguments
|||
|||	    ctype                       The type of the item to find.  Use
|||	                                MkNodeID() to create this value.
|||
|||	    name                        The name of the item to find.
|||
|||	  Return Value
|||
|||	    The procedure returns the number of the item that was opened.  It
|||	    returns an error code if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V24.
|||
|||	  Associated Files
|||
|||	    item.h                      ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  See Also
|||
|||	    FindAndOpenItem()
|||
**/

Item FindAndOpenNamedItem(int32 cntype, const char *name)
{
TagArg tags[2];

    tags[0].ta_Tag = TAG_ITEM_NAME;
    tags[0].ta_Arg = (void *)name;
    tags[1].ta_Tag = 0;

    return FindAndOpenItem(cntype,tags);
}
