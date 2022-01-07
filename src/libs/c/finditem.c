/* $Id: finditem.c,v 1.18 1994/09/12 18:38:40 limes Exp $ */
#include "types.h"
#include "nodes.h"
#include "item.h"

/**
|||	AUTODOC PUBLIC spg/kernel/finddevice
|||	FindDevice - Find a device by name.
|||
|||	  Synopsis
|||
|||	    Item FindDevice( const char *name )
|||
|||	  Description
|||
|||	    This macro finds a device whose name matches the name argument.
|||	    The search is not case-sensitive (that is, the Kernel does not
|||	    distinguish uppercase and lowercase letters in device names).
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
|||	    Macro implemented in device.h V20.
|||
|||	  Associated Files
|||
|||	    device.h                    ANSI C Macro definition
|||
|||	  See Also
|||
|||	    OpenNamedDevice(), FindItem()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/findsemaphore
|||	FindSemaphore - Find a semaphore by name.
|||
|||	  Synopsis
|||
|||	    Item FindSemaphore( const char *name )
|||
|||	  Description
|||
|||	    This macro finds a semaphore with the specified name.  The search
|||	    is not case-sensitive (that is, the Kernel does not distinguish
|||	    uppercase and lowercase letters in semaphore names).  You can use
|||	    this macro in place of FindNamedItem() to find a semaphore.
|||
|||	    For information about semaphores, which are used to control access
|||	    to shared resources, see the `Sharing System Resources' chapter in
|||	    the 3DO Portfolio Programmer's Guide.
|||
|||	  Arguments
|||
|||	    name                        The name of the semaphore to find.
|||
|||	  Return Value
|||
|||	    The macro returns the item number of the semaphore that
|||	    was found or an error code if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in semaphore.h V20.
|||
|||	  Associated Files
|||
|||	    semaphore.h                 ANSI C Macro definition
|||
|||	  See Also
|||
|||	    CreateSemaphore()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/findfolio
|||	FindFolio - Find a folio by name.
|||
|||	  Synopsis
|||
|||	    Item FindFolio( const char *name )
|||
|||	  Description
|||
|||	    This macro finds a folio whose name matches the name argument.
|||	    The search is not case-sensitive (that is, the Kernel does not
|||	    distinguish uppercase and lowercase letters in folio names).  You
|||	    can use this macro in place of FindNamedItem() to find a folio.
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
|||	    Macro implemented in folio.h V20.
|||
|||	  Associated Files
|||
|||	    folio.h                     ANSI C Macro definition
|||
|||	  See Also
|||
|||	    CallFolio(), CallFolioRet()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/findtask
|||	FindTask - Find a task by name.
|||
|||	  Synopsis
|||
|||	    Item FindTask( const char *name )
|||
|||	  Description
|||
|||	    This macro finds a task with the specified name.  The search is
|||	    not case-sensitive (that is, the kernel does not distinguish
|||	    uppercase and lowercase letters in task names).  You can use this
|||	    macro in place of FindNamedItem() to find a task.
|||
|||	  Arguments
|||
|||	    name                        The name of the task to find.
|||
|||	  Return Value
|||
|||	    The macro returns the item number of the task that was
|||	    found or an error code if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in task.h V20.
|||
|||	  Associated Files
|||
|||	    task.h                      ANSI C M        acro definition
|||
|||	  See Also
|||
|||	    CreateThread()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/findmsgport
|||	FindMsgPort - Find a message port by name.
|||
|||	  Synopsis
|||
|||	    Item FindMsgPort( const char *name )
|||
|||	  Description
|||
|||	    This macro finds a message port with the specified name.  The
|||	    search is not case-sensitive (that is, the Kernel does not
|||	    distinguish uppercase and lowercase letters in message port
|||	    names).  You can use this macro in place of FindNamedItem() to
|||	    find a message port.
|||
|||	  Arguments
|||
|||	    name                        The name of the message port to find.
|||
|||	  Return Value
|||
|||	    The macro returns the item number of the message port that
|||	    was found or an error code if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in msgport.h V20.
|||
|||	  Associated Files
|||
|||	    msgport.h                   ANSI C Macro definition
|||
|||	  See Also
|||
|||	    CreateMsgPort()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/findnameditem
|||	FindNamedItem - Find an item by name.
|||
|||	  Synopsis
|||
|||	    Item FindNamedItem( int32 ctype, const char *name )
|||
|||	  Description
|||
|||	    This procedure finds an item of the specified type whose name
|||	    matches the name argument.  The search is not case-sensitive (that
|||	    is, the Kernel does not distinguish uppercase and lowercase
|||	    letters in item names).
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
|||	    The procedure returns the number of the item that was found.  It
|||	    returns an error code if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V20.
|||
|||	  Associated Files
|||
|||	    item.h                      ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  See Also
|||
|||	    FindItem(), FindVersionedItem()
|||
**/

Item FindNamedItem(int32 cntype, const char *name)
{
	TagArg	tags[2];
	tags[0].ta_Tag = TAG_ITEM_NAME;
	tags[0].ta_Arg = (void *)name;
	tags[1].ta_Tag = 0;
	return FindItem(cntype,tags);
}
