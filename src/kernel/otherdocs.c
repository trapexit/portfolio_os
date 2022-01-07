/* $Id: otherdocs.c,v 1.16 1994/10/10 23:37:45 peabody Exp $ */

/* If we compile this file without this line, we get */
/* "Error: no external declaration in translation unit" */

extern int main(void);

/*
 * This file contains AutoDoc blocks that
 * have not yet been moved into place, or
 * that really have no other place to be.
 */

/**
|||	AUTODOC PUBLIC spg/kernel/mknodeid
|||	MkNodeID - Create an item type value
|||
|||	  Synopsis
|||
|||	    int32 MkNodeID( uint8 a , uint8 b )
|||
|||	  Description
|||
|||	    This macro creates an item type value, a 32-bit value that
|||	    specifies an item type and the folio in which the item type is
|||	    defined.  This value is required by other procedures that deal
|||	    with items, such as CreateItem() and FindItem().
|||
|||	  Arguments
|||
|||	    a                           The item number of the folio in which
|||	                                the item type of the item is defined.
|||	                                For a list of folio item numbers, see
|||	                                the Portfolio Items chapter.
|||
|||	    b                           The item type number for the item.
|||	                                For a list of item type numbers, see
|||	                                the Portfolio Items chapter.
|||
|||	  Return Value
|||
|||	    The macro returns an item type value or an error code (a negative
|||	    value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in nodes.h V20.
|||
|||	  Associated Files
|||
|||	    nodes.h                     ANSI C Macro definition
|||
|||	  See Also
|||
|||	    CreateItem(), FindItem(), FindNamedItem(), FindVersionedItem()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/callfolio
|||	CallFolio - Invokes from the folio vector table
|||	            a folio procedure that doesn't return a value.
|||
|||	  Synopsis
|||
|||	    void CallFolio( const Folio *folio, int32 func, args )
|||
|||	  Description
|||
|||	    This macro allows a task to invoke a folio procedure
|||	    directly from the folio vector table, thereby bypassing
|||	    the normal procedure interface.  It can be used only for
|||	    folio procedures that do not return a value.  (To invoke
|||	    a folio procedure that does return a value, use
|||	    CallFolioRet().)  This approach, which is slightly faster
|||	    than invoking the procedure through the normal interface,
|||	    is intended for use by the folios themselves, but it can
|||	    be also be used by applications.
|||
|||	    Note: Most tasks should invoke folio procedures in the
|||	    normal way, using the interfaces described in this
|||	    manual.  Only tasks that require the utmost in speed and
|||	    efficiency should invoke folio procedures directly from
|||	    the vector table which I guess is everyones programs.
|||
|||	    Example of AddTail(listP, nodeP) using CallFolio:
|||	         CallFolio(KernelBase, KBV_ADDTAIL, (listP, nodeP));
|||
|||	    Example of n = RemTail(l) using CallFolioRet
|||	         CallFolioRet(KernelBase, KBV_REMTAIL, (l), n, (Node *));
|||
|||	  Arguments
|||
|||	    folio                       A pointer to the folio item that
|||	                                contains the procedure.  Use
|||	                                LookupItem() to get this pointer.  For
|||	                                the item number of a folio (which you
|||	                                pass to LookupItem() see the Portfolio
|||	                                Items chapter or call
|||	                                OpenFolio(FindFolio()).
|||
|||	    func                        The index of the vector table entry
|||	                                for the procedure.  This index (which
|||	                                is always a negative integer, because
|||	                                the table grows backward in memory) is
|||	                                listed in the header file for the folio
|||	                                that contains the procedure.
|||
|||	    args                        The arguments for the procedure,
|||	                                separated by commas and enclosed within
|||	                                parentheses.
|||
|||	  Implementation
|||
|||	    Macro implemented in folio.h V20.
|||
|||	  Associated Files
|||
|||	    folio.h
|||
|||	  See Also
|||
|||	    CallFolioRet()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/callfolioret
|||	CallFolioRet - Invokes from the folio vector table
|||	               a folio procedure that returns a value.
|||
|||	  Synopsis
|||
|||	    void CallFolioRet( const Folio *folio, int32 func, args, ret, cast )
|||
|||	  Description
|||
|||	    This macro allows a task to invoke a folio procedure
|||	    directly from the folio vector table, thereby bypassing
|||	    the normal procedure interface.  It can be used only for
|||	    folio procedures that return a value.  (To invoke a folio
|||	    procedure that does not return a value, use CallFolio().)
|||	    This approach, which is slightly faster than invoking the
|||	    procedure through the normal interface, is intended for
|||	    use by the folios themselves, but it can be also be used
|||	    by applications.
|||
|||	    Note: Most tasks should invoke folio procedures in the
|||	    normal way, using the interfaces described in this
|||	    manual.  Only tasks that require the utmost in speed and
|||	    efficiency should invoke folio procedures directly from
|||	    the vector table.
|||
|||	  Arguments
|||
|||	    folio                       A pointer to the folio item that
|||	                                contains the procedure.  Use
|||	                                LookupItem() to get this pointer.  For
|||	                                the item number of a folio (which you
|||	                                pass to LookupItem()), see the
|||	                                Portfolio Items chapter or call
|||	                                OpenItem(FindFolio()).
|||
|||	    func                        The index of the vector table entry
|||	                                for the folio procedure to invoke.
|||	                                This index (a negative integer,
|||	                                because the table grows backward in
|||	                                memory) is listed in the header file
|||	                                for the folio.
|||
|||	    args                        The arguments for the procedure,
|||	                                separated by commas and enclosed
|||	                                within parentheses
|||
|||	    ret                         A variable to receive the result from
|||	                                the folio procedure.  The result is
|||	                                coerced to the type specified by the
|||	                                cast argument before it is assigned to
|||	                                this variable.
|||
|||	    cast                        The desired type for the result,
|||	                                surrounded by parentheses.  The result
|||	                                returned by the folio procedure is
|||	                                cast to this type.
|||
|||	  Implementation
|||
|||	    Macro implemented in folio.h V20.
|||
|||	  Associated Files
|||
|||	    folio.h
|||
|||	  See Also
|||
|||	    CallFolio()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/getfoliofunc
|||	GetFolioFunc - Return pointer to folio function.
|||
|||	  Synopsis
|||
|||	    FolioFunc GetFolioFunc (const Folio *folio, int32 func)
|||
|||	  Description
|||
|||	    This macro returns the address of a folio function from
|||	    a folio vector table.
|||
|||	  Arguments
|||
|||	    folio                       A pointer to the folio item that
|||	                                contains the procedure.  Use
|||	                                LookupItem() to get this pointer.  For
|||	                                the item number of a folio (which you
|||	                                pass to LookupItem()), see the
|||	                                Portfolio Items chapter or call
|||	                                OpenFolio(FindFolio()).
|||
|||	    func                        The index of the vector table entry
|||	                                for the procedure.  This index (which
|||	                                is always a negative integer, because
|||	                                the table grows backward in memory) is
|||	                                listed in the header file for the
|||	                                folio that contains the procedure.
|||
|||	  Implementation
|||
|||	    Macro implemented in folio.h V24.
|||
|||	  Associated Files
|||
|||	    folio.h
|||
|||	  See Also
|||
|||	    CallFolio(), CallFolioRet()
|||
**/

/**
|||	AUTODOC PRIVATE spg/kernel/getfolioswifunc
|||	GetFolioSWIFunc - Return pointer to folio SWI function.
|||
|||	  Synopsis
|||
|||	    FolioFunc GetFolioSWIFunc (const Folio *folio, int32 swiNum)
|||
|||	  Description
|||
|||	    This macro returns the address of a folio SWI function from
|||	    a folio vector table.
|||
|||	  Arguments
|||
|||	    folio                       A pointer to the folio item that
|||	                                contains the procedure.  Use
|||	                                LookupItem() to get this pointer.  For
|||	                                the item number of a folio (which you
|||	                                pass to LookupItem()), see the
|||	                                Portfolio Items chapter or call
|||	                                OpenFolio(FindFolio()).
|||
|||	    swiNum                      SWI number (0..f_MaxSWIFunctions-1)
|||	                                to look up.
|||
|||	  Implementation
|||
|||	    Private macro implemented in folio.h V24.
|||
|||	  Associated Files
|||
|||	    folio.h
|||
|||	  See Also
|||
|||	    GetFolioFunc(), CallFolio(), CallFolioRet()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/getcurrentsignals
|||	GetCurrentSignals - Gets the currently received signal bits
|||
|||	  Synopsis
|||
|||	    int32 GetCurrentSignals( void )
|||
|||	  Description
|||
|||	    This macro returns the signal bits that have been received for
|||	    the current task.  For information about signals, see the
|||	    description of the AllocSignal() procedure and the Communicating
|||	    Among Tasks chapter in the 3DO Portfolio Programmer's Guide.
|||
|||	  Return Value
|||
|||	    A 32-bit word in which all currently received signal bits are set.
|||
|||	  Implementation
|||
|||	    Macro implemented in kernel.h V20.
|||
|||	  Associated Files
|||
|||	    kernel.h                      ANSI C Macro definition
|||
|||	  See Also
|||
|||	    AllocSignal(), FreeSignal(), SendSignal(), WaitSignal()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/gettasksignals
|||	GetTaskSignals - Gets the currently received signal bits
|||
|||	  Synopsis
|||
|||	    int32 GetTaskSignals( Task *t )
|||
|||	  Description
|||
|||	    This macro returns the signal bits that have been received by
|||	    the specified task.  For information about signals, see the
|||	    description of the AllocSignal() procedure and the Communicating
|||	    Among Tasks chapter in the 3DO Portfolio Programmer's Guide.
|||
|||	  Return Value
|||
|||	    A 32-bit word in which all currently received signal bits for
|||	    the task are set.
|||
|||	  Implementation
|||
|||	    Macro implemented in task.h V21.
|||
|||	  Associated Files
|||
|||	    task.h                      ANSI C Macro definition
|||
|||	  See Also
|||
|||	    AllocSignal(), FreeSignal(), SendSignal(), WaitSignal()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/clearcurrentsignals
|||	ClearCurrentSignals - Clears some received signal bits.
|||
|||	  Synopsis
|||
|||	    Err ClearCurrentSignals( int32 sigMask )
|||
|||	  Description
|||
|||	    This macro resets the requested signal bits of the
|||	    current task to 0.
|||
|||	  Arguments
|||
|||	    sigMask                     A 32-bit word indicating which
|||	                                signal bits should be cleared.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Macro implemented in kernel.h V24.
|||
|||	  Associated Files
|||
|||	    kernel.h                      ANSI C Macro definition
|||
|||	  See Also
|||
|||	    AllocSignal(), FreeSignal(), SendSignal(), WaitSignal()
|||
**/
