/*  :ts=8 bk=0
 *
 * autodocs.c:	Repository of The Tao of Graphics.
 *
 * Yvonne Tornatta & Leo L. Schwab			9409.15
 ***************************************************************************
 *
 * $Id: autodocs.c,v 1.13 1994/10/28 04:48:32 ewhac Exp $
 *
 */

/**
|||	AUTODOC PUBLIC gpg/graphics/addscreengroup
|||	AddScreenGroup -  Adds a screen group to the display.
|||
|||	  Synopsis
|||
|||	    Err AddScreenGroup (Item screenGroup,
|||				TagArg *targs)
|||
|||	    Err AddScreenGroupVA (Item screenGroup,
|||				  uint32 targs, ...)
|||
|||	  Description
|||
|||	    This function adds the specified screenGroup to the graphics folio's
|||	    display mechanism.  After this call, the screens of the group can be made
|||	    visible with a call to DisplayScreen().  This function returns zero if
|||	    the group was added to the display, or an error code (a negative value) if
|||	    an error occurs.
|||
|||	  Arguments
|||
|||	    screenGroup                  Item number of the screenGroup to be
|||	                                 added.
|||
|||	    targs                        Pointer to an array of tag arguments.
|||	                                 These values control the initial display
|||	                                 configuration of the group.  The last
|||	                                 element of the array must be the value
|||	                                 TAG_END.  For a list of tag arguments for
|||	                                 screen group items, see the "Portfolio
|||	                                 Items" chapter in the Operating System
|||	                                 Programmer's Guide.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	    GRAFFERR_SGINUSE will be returned if AddScreenGroup() is called for a
|||	    screenGroup that has already been added with the AddScreenGroup()
|||	    call.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    DisplayScreen(), RemoveScreenGroup()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/clonevrampages
|||	CloneVRAMPages - Copies a single page of VRAM to other contiguous
|||	                 pages of VRAM.
|||
|||	  Synopsis
|||
|||	    Err CloneVRAMPages (Item ioreq,
|||				void *dest,
|||				void *src,
|||				uint32 numpages,
|||				uint32 mask)
|||
|||	  Description
|||
|||	    This function is a convenience call for SPORT device transfers.  It
|||	    copies the specified src VRAM page to a number of VRAM pages, starting at
|||	    the specified dest address and specified numPages number of pages.  The
|||	    mask argument controls which bits of the destination will be modified by
|||	    bits of the source.
|||
|||	    This function uses the SPORT device to do the transfer.  It doesn't
|||	    return until the transfer is completed.  That is, there is an implicit
|||	    wait for the next VBlank to occur, as SPORT transfers take place only
|||	    during vertical blank.
|||
|||	  Arguments
|||
|||	    ioreq                        Item number of the I/O request.
|||
|||	    dest                         Pointer to the first byte of the
|||	                                 first page of the destination VRAM.
|||
|||	    src                          Pointer to the first byte of the page
|||	                                 of source VRAM.
|||
|||	    numpages                     Number of destination pages to
|||	                                 receive the copy of the source page.
|||
|||	    mask                         Write mask, which controls which bits
|||	                                 of the source are copied to the destination.
|||
|||	  Return Value
|||
|||	    The function returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Notes
|||
|||	    The use of a mask value of other than all ones (0xFFFFFFFF) is
|||	    discouraged.  Future systems may be unable to support this
|||	    feature in a performance-conscious manner.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in graphics.lib V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    CopyVRAMPages(), SetVRAMPages(), SetVRAMPagesDefer(),
|||	    CopyVRAMPagesDefer(), CloneVRAMPagesDefer(), GetVRAMIOReq(),
|||	    GetPageSize()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/clonevrampagesdefer
|||	CloneVRAMPagesDefer - Copies one page of VRAM to a contiguous range of
|||	                      VRAM pages asynchronously.
|||
|||	  Synopsis
|||
|||	    Err CloneVRAMPagesDefer (Item ioreq,
|||				     void *dest,
|||				     void *src,
|||	                             uint32 numpages,
|||				     uint32 mask)
|||
|||	  Description
|||
|||	    This function is a convenience call for SPORT device transfers.  It
|||	    copies the specified src VRAM page to a number of VRAM pages, starting at
|||	    the specified dest address and specified numPages number of pages.  The
|||	    mask argument controls which bits of the destination will be modified by
|||	    bits of the source.
|||
|||	    This function uses the SPORT device to do the transfer.  It does not wait
|||	    for the transfer to complete, but returns immediately.  This allows the
|||	    application to continue its work while the SPORT I/O request is in the
|||	    queue.  When using a deferred request such as this, you should use
|||	    WaitIO() when you want to wait until the I/O request completes.
|||
|||	  Arguments
|||
|||	    ioreq                        Item number of the I/O request.
|||
|||	    dest                         Pointer to the first byte of
|||	                                 the first page of the destination VRAM.
|||
|||	    src                          Pointer to the first byte of the page
|||	                                 of source VRAM.
|||
|||	    numpages                     Number of destination pages to
|||	                                 receive the copy of the source page.
|||
|||	    mask                         Write mask, which controls which bits
|||	                                 of the source are copied to the destination.
|||
|||	  Return Value
|||
|||	    The function returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Notes
|||
|||	    The use of a mask value of other than all ones (0xFFFFFFFF) is
|||	    discouraged.  Future systems may be unable to support this
|||	    feature in a performance-conscious manner.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in graphics.lib V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    CopyVRAMPages(), SetVRAMPages(), SetVRAMPagesDefer(),
|||	    CopyVRAMPagesDefer(), CloneVRAMPages(), GetVRAMIOReq(),
|||	    GetPageSize()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/closegraphicsfolio
|||	CloseGraphicsFolio - Closes the graphics folio.
|||
|||	  Synopsis
|||
|||	    Err CloseGraphicsFolio (void)
|||
|||	  Description
|||
|||	    This function terminates access to the graphics folio, freeing any
|||	    resources allocated by calling OpenGraphicsFolio().
|||
|||	    Once the graphics folio is closed, the task should no longer call
|||	    any functions from the folio.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in graphics.lib V21.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    OpenGraphicsFolio()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/copyvrampages
|||	CopyVRAMPages -  Copies a contiguous number of VRAM pages  to a VRAM
|||	                 destination.
|||
|||	  Synopsis
|||
|||	    Err CopyVRAMPages (Item ioreq,
|||			       void *dest,
|||			       void *src,
|||	                       uint32 numpages,
|||			       uint32 mask)
|||
|||	  Description
|||
|||	    This function is a convenience call for SPORT device transfers.  It
|||	    copies the specified numpages number of pages from the specified src VRAM
|||	    pages to the specified dest VRAM pages.  The mask argument controls which
|||	    bits of the destination will be modified by bits of the source.
|||
|||	    This function uses the SPORT device to do the transfer.  It doesn't
|||	    return until the transfer is completed.  That is, there is an implicit
|||	    wait for the next VBlank to occur, as SPORT transfers take place only
|||	    during vertical blank.
|||
|||	  Arguments
|||
|||	    ioreq                        Input: Item number of the I/O request.
|||
|||	    dest                         Pointer to the VRAM buffer to
|||	                                 receive the copy.
|||
|||	    src                          Pointer to the VRAM buffer that's
|||	                                 the source of the copy.
|||
|||	    numpages                     Number of VRAM pages to be copied.
|||
|||	    mask                         Write mask, which controls which bits
|||	                                 of the source get copied to the destination.
|||
|||	  Return Value
|||
|||	    The function returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Notes
|||
|||	    The use of a mask value of other than all ones (0xFFFFFFFF) is
|||	    discouraged.  Future systems may be unable to support this
|||	    feature in a performance-conscious manner.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in graphics.lib V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    CloneVRAMPages(), SetVRAMPages(), SetVRAMPagesDefer(),
|||	    CopyVRAMPagesDefer(), CloneVRAMPagesDefer(), GetVRAMIOReq(),
|||	    GetPageSize()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/copyvrampagesdefer
|||	CopyVRAMPagesDefer - Copies a contiguous number of VRAM pages to a VRAM
|||	                     destination.
|||
|||	  Synopsis
|||
|||	    Err CopyVRAMPagesDefer (Item ioreq,
|||				    void *dest,
|||				    void *src,
|||	                            uint32 numpages,
|||				    uint32 mask)
|||
|||	  Description
|||
|||	    This function is a convenience call for SPORT device transfers.  It
|||	    copies the specified numpages number of pages from the specified src VRAM
|||	    pages to the specified dest VRAM pages.  The mask argument controls which
|||	    bits of the destination will be modified by bits of the source.
|||
|||	    This function uses the SPORT device to do the transfer.  It does not wait
|||	    for the transfer to complete, but returns immediately.  This allows the
|||	    application to continue its work while the SPORT I/O request is in the
|||	    queue.  When using a deferred request such as this, you should use
|||	    WaitIO() when you want to wait until the I/O request completes.
|||
|||	  Arguments
|||
|||	    ioreq                        Item number of the I/O request.
|||
|||	    dest                         Pointer to the VRAM buffer to
|||	                                 receive the copy.
|||
|||	    src                          Pointer to the VRAM buffer that's
|||	                                 the source of the copy.
|||
|||	    numpages                     Number of VRAM pages to be copied.
|||
|||	    mask                         Write mask, which controls which bits
|||	                                 of the source are copied to the destination.
|||
|||	  Return Value
|||
|||	    The function returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Notes
|||
|||	    The use of a mask value of other than all ones (0xFFFFFFFF) is
|||	    discouraged.  Future systems may be unable to support this
|||	    feature in a performance-conscious manner.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in graphics.lib V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    CloneVRAMPages(), SetVRAMPages(), SetVRAMPagesDefer(), CopyVRAMPages(),
|||	    CloneVRAMPagesDefer(), GetVRAMIOReq(), GetPageSize()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/createbitmap
|||	CreateBitmap - Creates a bitmap.
|||
|||	  Synopsis
|||
|||	    Item CreateBitmap (TagArg *args)
|||
|||	    Item CreateBitmapVA (uint32 args, ...)
|||
|||	  Description
|||
|||	    This macro creates a bitmap item having the characteristics
|||	    specified by the tag arguments.
|||
|||	  Tag Arguments
|||
|||	    The following tags must be present:
|||
|||	    * CBM_TAG_DONE - Marks the end of the tag argument list.
|||
|||	    * CBM_TAG_WIDTH - Sets the bitmap width, in pixels.
|||
|||	    * CBM_TAG_HEIGHT - Specifies the bitmap height, in pixels.
|||
|||	    * CBM_TAG_BUFFER - Specifies the starting address of the bitmap
|||	      buffer.
|||
|||	    The following tags are optional:
|||
|||	    * CBM_TAG_CLIPWIDTH - Specifies the width of the clipping region
|||	      (default:  the value of CBM_TAG_WIDTH)
|||
|||	    * CBM_TAG_CLIPHEIGHT - Specifies the height of the clipping region
|||	      (default: the value of CBM_TAG_HEIGHT)
|||
|||	    * CBM_TAG_CLIPX - Specifies the left edge of the clipping region
|||	      (default: 0)
|||
|||	    * CBM_TAG_CLIPY - Specifies the top edge of the clipping region
|||	      (default: 0)
|||
|||	    * CBM_TAG_WATCHDOGCTR - Specifies the timeout value (in µsec) (default:
|||	      1000000)
|||
|||	    * CBM_TAG_CECONTROL - Specifies the CEControl register to be loaded
|||	      into hardware when this bitmap is used (default:
|||	      (B15POS_PDC | B0POS_PPMP | CFBDSUB | CFBDLSB_CFBD0 | PDCLSB_PDC0) )
|||
|||	  Arguments
|||
|||	    args                         Pointer to an array of tag arguments
|||	                                 for the bitmap.  The last element of the
|||	                                 array must be the value TAG_END.  For a list
|||	                                 of tag arguments for bitmaps, see the
|||	                                 "Portfolio Items" chapter in the
|||	                                 Operating System Programmer's Guide.
|||
|||	  Return Value
|||
|||	    The function returns the item number of the new bitmap or an error number
|||	    (a negative value) if an error occurs.
|||
|||	    GRAFERR_BUFWIDTH will be returned if the specified width of the bitmap
|||	    buffer is not valid.
|||
|||	    GRAFERR_BADBITMAPSPEC will be returned if there is an error in the bitmap
|||	    specification not covered by some other error code.
|||
|||	    GRAFERR_NOWRITEACCESS will be returned if the memory specified by the
|||	    bitmap buffer is not writable by the calling task.
|||
|||	    GRAFERR_BADCLIP will be returned if illegal clip values (for instance,
|||	    larger than the size of the bitmap area) are specified.
|||
|||	  Notes
|||
|||	    CBM_TAG_CLIPX and CBM_TAG_CLIPY have the effect of moving the
|||	    origin of the bitmap coordinate system to the specified
|||	    location.
|||
|||	  Implementation
|||
|||	    Macro implemented in graphics.h V20.
|||
|||	  Associated File
|||
|||	    graphics.h
|||
|||	  See Also
|||
|||	    CreateItem(), DeleteBitmap(), DeleteItem()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/createscreengroup
|||	CreateScreenGroup - Creates a screen group and all its screens, bitmaps,
|||	                    buffers, and VDLs.
|||
|||	  Synopsis
|||
|||	    Item CreateScreenGroup (Item *screenItemArray,
|||				    TagArg *targs)
|||
|||	    Item CreateScreenGroupVA (Item *screenItemArray,
|||				      uint32 targs, ...)
|||
|||	  Description
|||
|||	    This function is the workhorse of the graphics display architecture.  It
|||	    creates a complete display environment, with all the associated components
|||	    and data structures, based on the specifications made in the targs
|||	    argument.  The function returns two pieces:  It returns the item number
|||	    of a newly created ScreenGroup structure, and it fills out the specified
|||	    screenItemArray with item numbers for the screen structures that have been
|||	    created.
|||
|||	    The tag args supplied as an argument to the function allow the programmer
|||	    to set all of the parameters that control the creation of the screen
|||	    group.  However, the programmer doesn't need to specify all of the tag
|||	    args.  Each possible tag arg has an associated default value, which is
|||	    used if the tag arg is not supplied.  In fact, if the programmer specifies
|||	    a tag arg array with no entries, or specifies NULL as the targs argument,
|||	    the defaults are sufficient to create a simple single-buffer screen group.
|||
|||	    Note:  Only one bitmap per screen is supported.
|||
|||	  Tag Arguments
|||
|||	    Here are the tag args that can be passed to CreateScreenGroup():
|||
|||	    * CSG_TAG_SCREENCOUNT - This tag arg allows you to define how many screens
|||	      will be created for the new screen group.  Typically, you will create two
|||	      screens in order to create double-buffered displays.  But you may want a
|||	      screen group with only a single screen, to show a static display for
|||	      instance.  Or you may want to create a stereoscope display with three or
|||	      four screens in the group.  If you don't specify a screen count, the
|||	      default number is one.
|||
|||	    * CSG_TAG_SCREENHEIGHT - You use this tag arg to
|||	      specify the actual height of your screen's frame buffer.  This may be
|||	      different from the display height of the screen, which is the height of
|||	      the visible portion of your screen.  The default width is that
|||	      reported by calling QueryGraphics() with the tag
|||	      QUERYGRAF_TAG_DEFAULTHEIGHT.
|||
|||	    * CSG_TAG_DISPLAYHEIGHT - You use this tag arg to specify the
|||	      display height of your screen.  The display height is the visible portion
|||	      of the screen.  You may create a screen that's taller than the display
|||	      height.  The display height must always be equal to or less than the
|||	      screen height.  The default is that reported by calling
|||	      QueryGraphics() with the tag QUERYGRAF_TAG_DEFAULTHEIGHT.
|||
|||	    * CSG_TAG_BITMAPCOUNT - This tag arg allows you to specify how many bitmaps
|||	      each screen will have.  Each screen within a screen group has to have the
|||	      same number and configuration of bitmaps.  The count specified by this tag
|||	      arg is the bitmap count per screen, not the total bitmap count.  The default
|||	      bitmap count per screen is one.
|||
|||	    * CSG_TAG_BITMAPWIDTH_ARRAY - In a screen
|||	      with multiple bitmaps, each bitmap can have its own width.  If you want to
|||	      specify a unique width per bitmap, or if you want your bitmaps to have a
|||	      width that's other than the system's default width, you use this
|||	      tag arg to supply the pointer to a table of widths.The elements of the
|||	      array are interpreted as int32 values.  There must be one entry for each
|||	      bitmap of a single screen.  If you don't specify a bitmap width array,
|||	      then all bitmaps will be of the width reported by calling
|||	      QueryGraphics() with the tag QUERYGRAF_TAG_DEFAULTWIDTH.
|||
|||	    * CSG_TAG_BITMAPHEIGHT_ARRAY - In a screen with multiple bitmaps, each bitmap
|||	      can have its own height.  If you want to specify a unique height per
|||	      bitmap, you use this tag arg to supply the pointer to a table of heights.
|||	      The elements of the array are interpreted as int32 values.  There must be
|||	      one entry for each bitmap of a single screen.  If you have only one bitmap
|||	      per screen, you don't need to specify a height for that bitmap; the
|||	      default is to use the height reported by calling
|||	      QueryGraphics() with the tag QUERYGRAF_TAG_DEFAULTHEIGHT.  If
|||	      you have more than one bitmap per screen, you must specify a
|||	      bitmap width array.
|||
|||	    * CSG_TAG_BITMAPBUF_ARRAY - Normally, the system will allocate your frame buffers for you.  However, you may wish to
|||	      have specific preallocated frame buffers associated with the screens of
|||	      your display.  You can designate your own buffers using this tag arg.
|||	      Paired with this tag arg is the pointer to an array of buffer pointers.The
|||	      elements in the array are interpreted as pointers to the starting
|||	      addresses of your frame buffers.  There must be an entry for every bitmap
|||	      in every screen.  For example, if you have two screens and each screen has
|||	      three bitmaps, then the bitmap buffer array must have six entries.If you
|||	      don't specify a bitmap buffer array, the default action is for the
|||	      system to allocate the appropriate frame buffers for you.  Even if you
|||	      choose to have the system allocate buffers for you, you can use the
|||	      CSG_TAG_SPORTBITS tag arg to specify SPORT access details.
|||
|||	    * CSG_TAG_SPORTBITS - This tag arg allows the programmer to take SPORT
|||	      transfers into consideration when setting up the bitmap frame buffers.
|||	      SPORT transfers have certain memory requirements and restrictions, all of
|||	      which are handled by using the correct flag bits when allocating frame
|||	      buffer memory; for instance, a call to GetBankBits() is usually required
|||	      to make sure that SPORT memory transfers occur in the same bank of memory.
|||	      You get to specify flag bits that are to be used when allocating frame
|||	      buffer buffers with this tag arg.Not only does this tag arg allow you to
|||	      control memory allocations, but the presence of this tag arg implies that
|||	      all normal SPORT care (page alignment, page size adjustment) should be
|||	      taken when making the allocation.If this tag arg is used but the
|||	      associated value is null, this designates that any buffers allocated by
|||	      the system need to all be in the same bank of memory, though any bank will
|||	      do.If this tag arg is absent, the default is to presume that SPORT
|||	      transfers don't have to be considered when allocating frame buffers.
|||
|||	    * CSG_TAG_VDLTYPE -  Every screen needs a VDL.  The graphics folio supports
|||	      several types of VDLs.  You can supply your own VDL, or you can choose to
|||	      have the system construct one for you.  If you supply your own, you have
|||	      to specify the type of VDL you are supplying.  If you choose to have the
|||	      system construct a VDL for you, you may wish to specify the type of VDL to
|||	      be built (rather than just accepting the default).
|||
|||	      The types of VDLs supported by the system include:
|||
|||	      * VDLTYPE_SIMPLE - The simple VDL has one entry, which points to the
|||	        frame buffer and has CLUT and display control words.  This type of VDL
|||	        is the default.
|||
|||	      * VDLTYPE_FULL -This VDL has an entry for every line of the display, which
|||	        entry includes a buffer address and a full CLUT.
|||
|||	      * VDLTYPE_COLOR - With this VDL, there is an entry for every line of the
|||	        display, which entry has the space for a full CLUT.  The color VDL does
|||	        not allow line-by-line modifications of the buffer address.  This type of
|||	        VDL is not yet supported.
|||
|||	      * VDLTYPE_ADDRESS - This VDL provides an entry for every line of the display,
|||	        which entry has a buffer address.  This allows line-by-line control over
|||	        the data that will be displayed on any given line.  The address VDL does
|||	        not allow line-by-line CLUT modifications.  This type of VDL is not yet
|||	        supported.
|||
|||	      * VDLTYPE_DYNAMIC - The dynamic VDL can be modified with wild abandon.  It
|||	        can grow and shrink dynamically.  This type of VDL gives the programmer
|||	        maximum flexibility, but it requires a great amount of processing overhead
|||	        and so it's the least attractive type of VDL in that regard.
|||
|||	      If you don't specify a VDL type, the default type is VDLTYPE_SIMPLE.
|||
|||	    * CSG_TAG_VDLPTR_ARRAY - If you want to specify your own VDLs to go with your own bitmaps, you may do so
|||	      using this tag arg.  Paired with this tag arg is a pointer to an array of
|||	      VDLs.  This tag argument is used in conjunction with
|||	      CSG_TAG_VDLLENGTH_ARRAY, and is an optional tag argument.The elements of
|||	      the array are interpreted as pointers to VDLs.  You must specify a VDL for
|||	      each screen you want created.You specify the type of VDLs you're
|||	      providing by using the tag arg CSG_TAG_VDLTYPE.  If you don't specify
|||	      one of those tag args, you get the normal default type for the tag arg.If
|||	      you don't specify an array of VDLs, the system will create the VDLs
|||	      for you.
|||
|||	    * CSG_TAG_VDLLENGTH_ARRAY - This tag is used in conjunction with
|||	      CSG_TAG_VDLPTR_ARRAY to specify your own VDLs to go with your own bitmaps.
|||	      Each submitted VDL in the VDL array must have a corresponding LENGTH
|||	      associated with it, as VDL list proofing is in effect.  (This is the
|||	      length in words.)   This is an optional tag argument.  If you choose to
|||	      submit a VDL list of your own, rather than using one of the defaults, then
|||	      the VDL list must be fully filled in at the time you call
|||	      CreateScreenGroup.
|||
|||	    * CSG_TAG_DISPLAYTYPE - Specifies what kind of display you want
|||	      the system to build for you (e.g. "Narrow" PAL or normal PAL).
|||	      The default display type is the one reported by calling
|||	      QueryGraphics() with the tag QUERYGRAF_TAG_DEFAULTDISPLAYTYPE.
|||
|||	    * CSG_TAG_DONE -  Put this tag arg at the end of the list
|||	      to terminate the list.
|||
|||	  Arguments
|||
|||	    screenItemArray              Pointer to an array of Item
|||	                                 fields.  The array is filled in by
|||	                                 CreateScreenGroup() if the call is
|||	                                 successful.
|||
|||	    targs                        A pointer to an array of tag
|||	                                 arguments.  The last element of the array
|||	                                 must be the value TAG_END.  If there are no
|||	                                 tag arguments, this argument must be NULL.
|||
|||	  Return Value
|||
|||	    The function returns the item number of the newly created screen group or
|||	    an error number (a negative value) if an error occurs.
|||
|||	    GRAFERR_NOMEM will be returned if any memory allocation attempted on the
|||	    behalf of the task fails.
|||
|||	    GRAFERR_VDLWIDTH will be returned if an illegal width is specified for the
|||	    VDL.
|||
|||	  Caveats
|||
|||	    Multiple bitmaps per Screen are not yet supported.
|||
|||	    Only VDLs of type VDLTYPE_FULL are currently supported.
|||
|||	    SPORT-conscious bitmap buffer allocation is not yet done
|||	    "automatically."  You must supply a value from GetBankBits() to
|||	    take advantage of this feature.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    AddScreenGroup(), DeleteScreenGroup(), RemoveScreenGroup()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/createvblioreq
|||	CreateVBLIOReq - Returns an I/O request for use with WaitVBL()
|||	                 and WaitVBLDefer().
|||
|||	  Synopsis
|||
|||	    Item CreateVBLIOReq (void)
|||
|||	  Description
|||
|||	    This function is a convenience call for the vbl timer. This
|||	    function creates and returns an I/O request that is suitable for
|||	    use with WaitVBL() and WaitVBLDefer().
|||
|||	  Return Value
|||
|||	    This function returns the item number of the I/O request or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in graphics.h V24.
|||
|||	  Associated Files
|||
|||	    graphics.h
|||
|||	  See Also
|||
|||	    DeleteVBLIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/createvramioreq
|||	CreateVRAMIOReq - Returns an I/O request for use with SPORT transfer
|||	                  functions.
|||
|||	  Synopsis
|||
|||	    Item CreateVRAMIOReq (void)
|||
|||	  Description
|||
|||	    This function is a convenience call for SPORT device transfers.  This
|||	    function creates and returns an I/O request that is suitable for use with
|||	    the SPORT transfer functions.
|||
|||	  Return Value
|||
|||	    This function returns the item number of the I/O request or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in graphics.h V24.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    CloneVRAMPages(), SetVRAMPages(), SetVRAMPagesDefer(),
|||	    CopyVRAMPagesDefer(), CloneVRAMPagesDefer(), CopyVRAMPages(),
|||	    DeleteVRAMIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/deletebitmap
|||	DeleteBitmap - Deletes a bitmap item.
|||
|||	  Synopsis
|||
|||	    Err DeleteBitmap (Item bitmap)
|||
|||	  Description
|||
|||	    This macro deletes a bitmap item created by CreateBitmap().
|||
|||	  Arguments
|||
|||	    bitmap                       The bitmap Item to delete, as obtained
|||	                                 from CreateBitmap().
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in graphics.h V21.
|||
|||	  Associated File
|||
|||	    graphics.h
|||
|||	  See Also
|||
|||	    CreateBitmap(), DeleteItem()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/deletescreengroup
|||	DeleteScreenGroup - Deletes a screen group and its associated data
|||	                    structures.
|||
|||	  Synopsis
|||
|||	    Err DeleteScreenGroup (Item screenGroupItem)
|||
|||	  Description
|||
|||	    This folio routine undoes what CreateScreenGroup() does.  In releases
|||	    previous to 1.3, DeleteScreenGroup() was #define'd to
|||	    DeleteItem(), which was insufficient.
|||
|||	    DeleteScreenGroup() will free memory for Bitmap Items if and only if the
|||	    memory was allocated by a CreateScreenGroup() call.  Screen and VDL
|||	    Items associated with the ScreenGroup Item will be deleted.
|||
|||	  Arguments
|||
|||	    screenGroupItem              The item number of a ScreenGroup
|||	                                 structure.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Caveats
|||
|||	    Tons.  Mostly to do with how the system decides what it's
|||	    responsible for freeing.  For example, if you
|||	    CreateScreenGroup() a default Screen, then SetVDL() a custom
|||	    VDL, DeleteScreenGroup() will attempt to delete your custom VDL
|||	    (leaving the original hanging around).
|||
|||	    In general, if you get clever with VDLs and Bitmaps, don't
|||	    assume DeleteScreenGroup() will be as clever as you.  Unwind
|||	    all your modifications before calling DeleteScreenGroup().
|||
|||	    DeleteScreenGroup() may get smarter in the future.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics.lib V21.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    CreateScreenGroup()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/deletevblioreq
|||	DeleteVBLIOReq - Delete an I/O request created by
|||	                 CreateVBLIOReq().
|||
|||	  Synopsis
|||
|||	    Err DeleteVBLIOReq (Item ioreq)
|||
|||	  Description
|||
|||	    This macro deletes a VBL timer I/O request item created by
|||	    CreateVBLIOReq().
|||
|||	  Arguments
|||
|||	    ioreq                       The item number of the I/O request to
|||	                                delete.
|||
|||	  Return Value
|||
|||	    The macro returns 0 if the I/O request was successfully deleted or
|||	    an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in graphics.h V24.
|||
|||	  Associated Files
|||
|||	    graphics.h
|||
|||	  See Also
|||
|||	    CreateVBLIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/deletevdl
|||	DeleteVDL - Deletes a VDL and all its associated data structures.
|||
|||	  Synopsis
|||
|||	    Err DeleteVDL (Item vdlItem)
|||
|||	  Description
|||
|||	    This macro is a convenience call to DeleteItem().  It deletes a VDL that
|||	    was created with a call to SubmitVDL().  Any data structures and buffers
|||	    allocated by SubmitVDL() are freed too.
|||
|||	    After the call to this macro, the VDL item number becomes invalid.  If the
|||	    VDL is currently being used in a screen that's being displayed, the
|||	    screen will go black.
|||
|||	  Arguments
|||
|||	    vdlItem                      The item number of a VDL structure.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in graphics.h V20.
|||
|||	  Associated File
|||
|||	    graphics.h
|||
|||	  Caveats
|||
|||	    DeleteScreenGroup() will not delete VDLs that were created with the
|||	    initial CreateScreenGroup() call if new VDLs have been attached to the
|||	    screens with SetVDL() calls.  If a new VDL has been attached to a screen
|||	    in the ScreenGroup being deleted with the DeleteScreenGroup call, then the
|||	    new VDL will be deleted as well.
|||
|||	  See Also
|||
|||	    DeleteItem(), SetVDL(), SubmitVDL()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/deletevramioreq
|||	DeleteVRAMIOReq - Delete an I/O request created by
|||	                  CreateVRAMIOReq().
|||
|||	  Synopsis
|||
|||	    Err DeleteVRAMIOReq (Item ioreq)
|||
|||	  Description
|||
|||	    This macro deletes a SPORT I/O request item created by
|||	    CreateVRAMIOReq().
|||
|||	  Arguments
|||
|||	    ioreq                       The item number of the I/O request to
|||	                                delete.
|||
|||	  Return Value
|||
|||	    The macro returns 0 if the I/O request was successfully deleted or
|||	    an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in graphics.h V24.
|||
|||	  Associated Files
|||
|||	    graphics.h
|||
|||	  See Also
|||
|||	    CreateVRAMIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/disablehavg
|||	DisableHAVG - Disables horizontal averaging.
|||
|||	  Synopsis
|||
|||	    Err DisableHAVG (Item screenItem)
|||
|||	  Description
|||
|||	    This function disables horizontal averaging of the specified screen.
|||	    Vertical averaging is left undisturbed.
|||
|||	  Arguments
|||
|||	    screenItem                   Item number of the Screen whose
|||	                                 display is to be modified.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    DisableVAVG(), EnableHAVG(), EnableVAVG()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/disablevavg
|||	DisableVAVG - Disables vertical averaging.
|||
|||	  Synopsis
|||
|||	    Err DisableVAVG (Item screenItem)
|||
|||	  Description
|||
|||	    This function disables vertical averaging of the specified screen.
|||	    Horizontal averaging is left undisturbed.
|||
|||	  Arguments
|||
|||	    screenItem                   Item number of the screen whose
|||	                                 display is to be modified.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    DisableHAVG(), EnableHAVG(), EnableVAVG()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/displayscreen
|||	DisplayScreen - Displays the bitmap(s) of the specified screen (or
|||	                screens).
|||
|||	  Synopsis
|||
|||	    Err DisplayScreen (Item screenItem0,
|||			       Item screenItem1)
|||
|||	  Description
|||
|||	    This function causes the bitmap(s) of the specified screen (or screens)
|||	    to be displayed.  The screenItem0 argument specifies the screen to be
|||	    added to the display.  For a stereoscopic or interlaced display, the
|||	    screenItem1 argument can specify a second screen structure to be displayed
|||	    in the odd field of the video frame.
|||
|||	    The second argument can be NULL, or the same as the first, to specify a
|||	    simple single-screen display.
|||
|||	  Arguments
|||
|||	    screenItem0                  Item number of the screen that is to
|||	                                 be added to the display.
|||
|||	    screenItem1                  Optional item number of the screen
|||	                                 that is to be added to the second (the odd)
|||	                                 field of the display frame.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	    GRAFERR_NOTOWNER will be returned if the screen item(s) specified are not
|||	    owned by the current task and have not been opened with OpenItem() calls.
|||
|||	    GRAFERR_SGNOTINUSE will be returned if AddScreenGroup() has not been
|||	    called for the ScreenGroup containing the specified screen(s).
|||
|||	    GRAFERR_MIXEDSCREENS will be returned if the specified screen items belong
|||	    to different ScreenGroups.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    AddScreenGroup(), DisplayScreen(), ModifyVDL(), SetVDL(),
|||	    SubmitVDL()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/drawcels
|||	DrawCels - Draws a list of cels (one or more) to the buffer of a bitmap.
|||
|||	  Synopsis
|||
|||	    Err DrawCels (Item bitmapItem,
|||			  CCB *ccb)
|||
|||	  Description
|||
|||	    This function sets up the cel engine according to parameters in the
|||	    bitmapItem, and then submits to the cel hardware the list of cels
|||	    specified by the ccb argument.  On return, the cels have been rendered.
|||
|||	  Arguments
|||
|||	    bitmapItem                   Item number of a bitmap structure.
|||
|||	    ccb                          Pointer to the first of one or
|||	                                 more CCB structures.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	    GRAFERR_CELTIMEOUT will be returned if the cel engine times out during the
|||	    rendering process.  The most likely cause of a cel engine time-out is a
|||	    bad cel or CCB data being passed to the cel engine.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||	    Became a SWI in graphics folio V24.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  Notes
|||
|||	    DrawCels() operates faster when the task or thread calling it
|||	    is the owner of the bitmap, as opposed to simply having called
|||	    OpenItem() on the bitmap.
|||
|||	  See Also
|||
|||	    DrawScreenCels()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/drawchar
|||	DrawChar - Draws a text character to a console's window.
|||
|||	  Synopsis
|||
|||	    Err DrawChar (GrafCon *gcon,
|||			  Item bitmapItem,
|||			  uint32 character)
|||
|||	  Description
|||
|||	    This function plots the specified text character to the window of the
|||	    console.  The pen position in the GrafCon is updated based on the width or
|||	    height of the rendered character.
|||
|||	    The fonts flag, FONT_VERTICAL, controls whether the height or width of the
|||	    rendered character is modified.  If the current FONT_VERTICAL fonts flag
|||	    is set, then the height is modified; if the flag is clear, then the width
|||	    is modified.
|||
|||	  Arguments
|||
|||	    gcon                         A pointer to a GrafCon
|||	                                 structure for pen position information.
|||
|||	    bitmapItem                   An item number for the bitmap to
|||	                                 receive the rendering.
|||
|||	    character                    The text character to be printed to
|||	                                 the console's window.  The character
|||	                                 argument is unsigned 32-bit to allow for
|||	                                 international character sets.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    DrawText8()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/drawscreencels
|||	DrawScreenCels - Draws a list of cels (one or more) to the bitmaps of a
|||	                 screen.
|||
|||	  Synopsis
|||
|||	    Err DrawScreenCels (Item screenItem,
|||				CCB *ccb)
|||
|||	  Description
|||
|||	    This function traverses the list of bitmaps associated with the screen
|||	    specified by the screenItem argument, calling DrawCels() for each bitmap.
|||	    On return, all cels have been rendered into all bitmaps of the screen.
|||
|||	  Arguments
|||
|||	    screenItem                   Item number of a screen structure.
|||
|||	    ccb                          Pointer to the first of one or
|||	                                 more CCB structures.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||	    Became a SWI in graphics folio V24.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  Notes
|||
|||	    DrawScreenCels() operates faster when the task or thread calling it
|||	    is the owner of the bitmap, as opposed to simply having called
|||	    OpenItem() on the bitmap.
|||
|||	  See Also
|||
|||	    DrawCels()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/drawtext16
|||	DrawText16 - Draws a text character to a console's window.
|||
|||	  Synopsis
|||
|||	    Err DrawText16 (GrafCon *gcon,
|||			    Item bitmapItem,
|||			    uint16 *text)
|||
|||	  Description
|||
|||	    This function plots the specified text string to the window of the
|||	    console.  It accepts a 16-bit null-terminated text string and calls
|||	    DrawChar() to plot each text character.
|||
|||	    This function is the same as DrawText8(), except that the characters are
|||	    16 bits wide.
|||
|||	  Arguments
|||
|||	    gcon                         A pointer to a GrafCon
|||	                                 structure for pen position information.
|||
|||	    bitmapItem                   An item number for the bitmap to
|||	                                 receive the rendering.
|||
|||	    text                         A 16-bit null-terminated text string
|||	                                 to be printed to the console's window.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    DrawChar(), DrawText8()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/drawtext8
|||	DrawText8 - Draws a text character to a console's window.
|||
|||	  Synopsis
|||
|||	    Err DrawText8 (GrafCon *gcon,
|||			   Item bitmapItem,
|||			   uint8 *text)
|||
|||	  Description
|||
|||	    This function plots the specified text string to the window of the
|||	    console.  It accepts an 8-bit null-terminated text string and calls
|||	    DrawChar() to plot each text character.
|||
|||	  Arguments
|||
|||	    gcon                         A pointer to a GrafCon
|||	                                 structure for pen position information.
|||
|||	    bitmapItem                   An item number for the bitmap to
|||	                                 receive the rendering.
|||
|||	    text                         An 8-bit null-terminated text string
|||	                                 to be printed to the console's window.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    DrawChar(), DrawText16()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/drawto
|||	DrawTo - Draws a line from the GrafCon's current position to a
|||	         specified position.
|||
|||	  Synopsis
|||
|||	    Err DrawTo (Item bitmapItem,
|||			GrafCon *grafcon,
|||			Coord x,
|||			Coord y)
|||
|||	  Description
|||
|||	    The function draws a line from the current pen position to the specified
|||	    x and y coordinates.  On completion, the endpoint becomes the new current
|||	    pen position.
|||
|||	    Note that this function draws a line that includes both the start and end
|||	    points.  This function uses the cel hardware.
|||
|||	  Arguments
|||
|||	    bitmapItem                   The item number of the bitmap.
|||
|||	    grafcon                      Pointer to a GrafCon
|||	                                 structure.
|||
|||	    x                            The column of the line's
|||	                                 endpoint.
|||
|||	    y                            The row of the line's endpoint.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    MoveTo()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/enablehavg.c
|||	EnableHAVG - Enables horizontal averaging.
|||
|||	  Synopsis
|||
|||	    Err EnableHAVG (Item screenItem)
|||
|||	  Description
|||
|||	    This function enables horizontal averaging of the specified screen.
|||	    Vertical averaging is left undisturbed.
|||
|||	  Arguments
|||
|||	    screenItem                   Item number of the screen whose
|||	                                 display is to be modified.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    DisableHAVG(), DisableVAVG, EnableVAVG()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/enablevavg.c
|||	EnableVAVG - Enables vertical averaging.
|||
|||	  Synopsis
|||
|||	    Err EnableVAVG (Item screenItem)
|||
|||	  Description
|||
|||	    This function enables vertical averaging of the specified screen.
|||	    Horizontal averaging is left undisturbed.
|||
|||	  Arguments
|||
|||	    screenItem                   Item number of the screen whose
|||	                                 display is to be modified.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    DisableHAVG(), DisableVAVG, EnableHAVG()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/fastmapcel
|||	FastMapCel - Maps this cel onto any four corners.
|||
|||	  Synopsis
|||
|||	    void FastMapCel (CCB *ccb,
|||			     Point *quad)
|||
|||	  Description
|||
|||	    This convenience routine maps a cel onto the four points specified by the
|||	    quad argument.  The mapping is created by setting the delta fields in the
|||	    specified ccb argument.  The deltas will cause the cel to be rendered such
|||	    that the four corners of the imagery map to the four corners.
|||
|||	    The quad argument points to an array of four-point structures.  In the
|||	    normal configuration of the quad, the first entry refers to the top-left
|||	    corner of the imagery, the second entry refers to top-right, the third to
|||	    bottom-right, and the fourth to bottom-left.  Those corners of the cel
|||	    imagery map to those four entries in the quad, no matter what values are
|||	    given to the quad entries.
|||
|||	    This function expects that the CCB structure's width and height
|||	    fields have been initialized by the FastMapCelInit() routine.
|||
|||	  Arguments
|||
|||	    ccb                          Pointer to the CCB structure
|||	                                 whose delta fields are modified to map to
|||	                                 the quad.
|||
|||	    quad                          Pointer to a four-entry array of
|||	                                 point structures.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in graphics.lib V21.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  Caveats
|||
|||	    The FastMapCel routines are not compatible with the MapCel routine, and
|||	    cannot be called interchangeably.  FastMapCel operates at a lower
|||	    precision to the MapCel routine, and produces slightly different output.
|||
|||	  See Also
|||
|||	    FastMapCel(), FastMapCelInit(), MapCel()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/fastmapcelf16
|||	FastMapCelf16 - Maps this cel onto any four corners.
|||
|||	  Synopsis
|||
|||	    void FastMapCelf16 (CCB *ccb,
|||				Point *quad)
|||
|||	  Description
|||
|||	    This convenience routine maps a cel onto the four points specified by the
|||	    quad argument.  The mapping is created by setting the delta fields in the
|||	    specified ccb argument.  The deltas will cause the cel to be rendered such
|||	    that the four corners of the imagery map to the four corners.
|||
|||	    The quad argument points to an array of four-point structures.  In the
|||	    normal configuration of the quad, the first entry refers to the top-left
|||	    corner of the imagery, the second entry refers to top-right, the third to
|||	    bottom-right, and the fourth to bottom-left.  Those corners of the cel
|||	    imagery map to those four entries in the quad, no matter what values are
|||	    given to the quad entries.
|||
|||	    This routine is different from MapCel() and FastMapCel() in that it
|||	    expects the values in the quad structure to be frac16 values and not
|||	    int32 values.
|||
|||	    This function expects that the CCB structure's width and height
|||	    fields have been initialized by the FastMapCelInit() routine.
|||
|||	  Arguments
|||
|||	    ccb                          Pointer to the CCB structure
|||	                                 whose delta fields are modified to map to
|||	                                 the quad.
|||
|||	    quad                         Pointer to a four-entry array of
|||	                                 Point structures.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in graphics.lib V21.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  Caveats
|||
|||	    The FastMapCel routines are not compatible with the MapCel routine, and
|||	    cannot be called interchangeably.
|||
|||	  See Also
|||
|||	    FastMapCelInit(), FastMapCelf16(), MapCel()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/fastmapcelinit
|||	FastMapCelInit - Initializes a CCB for FastMapCel use.
|||
|||	  Synopsis
|||
|||	    void FastMapCelInit (CCB *ccb)
|||
|||	  Description
|||
|||	    This convenience routine will initialize a CCB for use with MapCel.  This
|||	    routine assumes that the ccb_Width and ccb_Height fields of the CCB are
|||	    correct when the routine is called, and that the routine will replace the
|||	    current values of the ccb_Width and ccb_Height fields.  If the ccb_Width
|||	    and ccb_Height of the cel are both powers of two, then they will be
|||	    replaced by the log(base 2) of their respective values.  If one or both of
|||	    the fields are not a power of two, the ccb_Width will be replaced by
|||	    (-0x10000/ccb_Width) and ccb_Height will be replaced by
|||	    (0x10000/ccb_Height).
|||
|||	    Note:  This routine cannot be called multiple times for the same CCB.
|||
|||	  Arguments
|||
|||	    ccb                          Pointer to the CCB structure
|||	                                 to bemodified.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in graphics.lib V21.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  Caveats
|||
|||	    The FastMapCel routines are not compatible with the MapCel routine, and
|||	    cannot be called interchangeably.
|||
|||	    Since this routine modifies the ccb_Width and ccb_Height fields of the
|||	    CCB, it cannot be called multiple times for the same CCB.
|||
|||	  See Also
|||
|||	    FastMapCel(), FastMapCelf16(), MapCel()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/fillrect
|||	FillRect - Draws a filled rectangle.
|||
|||	  Synopsis
|||
|||	    Err FillRect (Item bitmapItem,
|||			  GrafCon *gc,
|||			  Rect *r)
|||
|||	  Description
|||
|||	    This function draws a rectangle to the display.
|||
|||	    The right and bottom edges of the rectangle are not drawn, to allow
|||	    multiple rectangles to be rendered without unnecessary overstriking of the
|||	    boundary pixels (cel engine emulation).
|||
|||	  Arguments
|||
|||	    bitmapItem                   The item number of the bitmap.
|||
|||	    gc                           Pointer to a GrafCon
|||	                                 structure.
|||
|||	    r                            Pointer to a Rect structure that
|||	                                 describes the rectangle to be rendered.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Notes
|||
|||	    This function will be changed to draw the entire rectangle,
|||	    including the specified right and bottom edges.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    WritePixel()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/getcurrentfont
|||	GetCurrentFont - Gets the pointer to the system's default font.
|||
|||	  Synopsis
|||
|||	    Font *GetCurrentFont (void)
|||
|||	  Description
|||
|||	    This function returns the pointer to the current font.
|||
|||	  Return Value
|||
|||	    The function returns a pointer to the current font.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    ResetCurrentFont()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/getfirstdisplayinfo
|||	GetFirstDisplayInfo - Retrieve information about display modes
|||			      supported by the system.
|||
|||	  Synopsis
|||
|||	    DisplayInfo *GetFirstDisplayInfo (void)
|||
|||	  Description
|||
|||	    This routine returns a pointer to a DisplayInfo structure, which
|||	    describes the characteristics of the display modes available to
|||	    applications.
|||
|||	    The display characteristics are represented as an array of Tag
|||	    arguments (to allow for future expansion).  You can search the
|||	    array for the tag value representing the characteristic in which
|||	    you're interested; the associated argument is the value of that
|||	    characteristic.
|||
|||	    The DisplayInfo structure is a list node; you may traverse the
|||	    list using the kernel list traversal routines (NEXTNODE(), et
|||	    al).  Graphics may be supporting any number of display modes.
|||
|||	  Tag Arguments
|||
|||	    The tag values found in the DisplayInfo tag array are:
|||
|||	    DI_TYPE - int32
|||	    	The type identifier of the display mode.  This would be the
|||	    	value you would pass to CreateScreenGroup() as the argument
|||	    	to the CSG_TAG_DISPLAYTYPE tag to create a Screen of that
|||	    	type.
|||
|||	    	The currently defined (and ever-expanding) list of DI_TYPE
|||	    	values are:
|||
|||	    	DI_TYPE_NTSC
|||	    		Standard 320 * 240 display.
|||
|||	    	DI_TYPE_PAL1
|||	    		"Narrow" PAL display (320 * 288).
|||
|||	    	DI_TYPE_PAL2
|||	    		Normal ("Wide") PAL display (384 * 288).
|||
|||	    DI_WIDTH - int32
|||	    	The width of this display mode, in pixels.
|||
|||	    DI_HEIGHT - int32
|||	    	The height of this display mode, in pixels.
|||
|||	    DI_FIELDTIME - int32
|||	    	Duration of a single field of video, in microseconds.
|||
|||	    DI_FIELDFREQ - int32
|||	    	Number of fields of video per second.
|||
|||	    DI_ASPECT - frac16
|||	    	Aspect ratio of this display mode's pixels.
|||
|||	    DI_ASPECTW - int32
|||	    	Width component of this display mode's pixel aspect ratio.
|||
|||	    DI_HEIGHT - int32
|||	    	Height component of this display mode's pixel aspect ratio.
|||
|||	    DI_NOINTERLACE - int32
|||	    	Boolean value: non-zero if this display mode does not
|||	    	support	interlaced video.
|||
|||	    DI_NOSTEREO - int32
|||	    	Boolean value: non-zero if this display mode does not
|||	    	support stereoscopic displays.
|||
|||	  Return Value
|||
|||	    Returns a pointer to the first DisplayInfo structure in the
|||	    list.
|||
|||	  Notes
|||
|||	    This routine is most likely to be useful to individuals wishing
|||	    to have "auto-sensing" applications that work on both continents
|||	    (hint, hint...).
|||
|||	    Do not under any circumstances make assumptions about the layout
|||	    of the tag argument array in the DisplayInfo structure.  You
|||	    must perform explicit searches through the array for the
|||	    information in which you're interested.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V22 (?).
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    QueryGraphics(), QueryGraphicsList()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/getpixeladdress
|||	GetPixelAddress - Gets a pointer to the pixel at the specified
|||	                  coordinates.
|||
|||	  Synopsis
|||
|||	    void *GetPixelAddress (Item scrbitItem,
|||				   Coord x,
|||				   Coord y)
|||
|||	  Description
|||
|||	    This function returns a pointer to the pixel at the specified
|||	    coordinates.
|||
|||	    The argument scrbitItem can refer to either a Screen Item or a
|||	    Bitmap Item.  If scrbitItem is a Bitmap Item, the returned
|||	    pointer is the address of the pixel in the Bitmap's buffer.  If
|||	    scrbitItem is a Screen Item, the returned pointer is the address
|||	    of the pixel in the buffer of the Bitmap associated with the
|||	    Screen.
|||
|||	  Arguments
|||
|||	    screenItem                   Item number of the Screen or Bitmap
|||					 containing the pixel.
|||
|||	    x                            The column of the desired pixel
|||	                                 address.
|||
|||	    y                            Row of the desired pixel address.
|||
|||	  Return Value
|||
|||	    The function returns a pointer to the pixel in the buffer of the
|||	    relevant Bitmap, or NULL if an error occurs (such as specifying
|||	    a pixel that lies outside the clip boundaries of the Bitmap).
|||
|||	  Notes
|||
|||	    Future display modes may cause this routine to return addresses
|||	    of varying alignments.  Try not to assume too much about the
|||	    characteristics of the memory to which the returned address
|||	    points.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    ReadPixel()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/getvblioreq
|||	GetVBLIOReq - Returns an I/O request for use with timer functions.
|||
|||	  Synopsis
|||
|||	    Item GetVBLIOReq (void)
|||
|||	  Description
|||
|||	    This function creates and returns an I/O request that is suitable for use
|||	    with the WaitVBL() and WaitVBLDefer() functions.
|||
|||	  Return Value
|||
|||	    This function returns the item number of the I/O request or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in graphics.lib V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    WaitVBL(), WaitVBLDefer(), DeleteItem()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/getvramioreq
|||	GetVRAMIOReq - Returns an I/O request for use with SPORTtransfer
|||	               functions.
|||
|||	  Synopsis
|||
|||	    Item GetVRAMIOReq (void)
|||
|||	  Description
|||
|||	    This function is a convenience call for SPORT device transfers.  This
|||	    function creates and returns an I/O request that is suitable for use with
|||	    the SPORT transfer functions.
|||
|||	  Return Value
|||
|||	    This function returns the item number of the I/O request or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in graphics.lib V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    CloneVRAMPages(), SetVRAMPages(), SetVRAMPagesDefer(),
|||	    CopyVRAMPagesDefer(), CloneVRAMPagesDefer(), CopyVRAMPages(), DeleteItem()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/makeccbrelative
|||	MakeCCBRelative - Creates a CCB relative address.
|||
|||	  Synopsis
|||
|||	    int32 MakeCCBRelative (field, linkObject)
|||
|||	  Description
|||
|||	    This macro allows you to turn the absolute address of an object into the
|||	    sort of relative address needed by the cel engine.  The first argument is
|||	    a pointer to the field to receive the relative address, and the second
|||	    argument is a pointer to the object to be referenced.  This macro does the
|||	    math required to create a correct relative offset, which is not simply the
|||	    difference between the two addresses.
|||
|||	    For instance, to create a relative pointer to a "next cel"  you
|||	    would use these arguments:
|||
|||	    cel->ccb_NextPtr = (CCB *)CCB_Relative( &cel- >ccb_NextPtr,&NextCel );
|||
|||	    To make sure your cel indicates it has a relative pointer to the next cel,
|||	    you might want to explicitly clear the control flag:
|||
|||	    ClearFlag(cel->ccb_Flags, CCB_NPABS );
|||
|||	    This macro is required because the cel engine uses pipelined prefetch
|||	    techniques that keep the engine's internal addressing ahead of the
|||	    internal logic that processes the addresses and data in the cel control
|||	    block.
|||
|||	  Arguments
|||
|||	    field                        The address of the object that will
|||	                                 receive the relative link to the other
|||	                                 object.  The object can be the next CCB, cel
|||	                                 data, or the PLUT.  This argument can be any
|||	                                 value that can be coerced to an int32.
|||
|||	    linkObject                   The address to the object that will
|||	                                 be linked.  The object can be the next CCB,
|||	                                 cel data, or the PLUT.  This argument can be
|||	                                 any value that can be coerced to an int32.
|||
|||	  Return Value
|||
|||	    The function returns an int32 value that is the relative offset between
|||	    the two addresses corrected (offset) as required by the cel engine.
|||
|||	  Implementation
|||
|||	    Macro implemented in graphics.h V20.
|||
|||	  Associated File
|||
|||	    graphics.h
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/makeclutbackgroundentry
|||	MakeCLUTBackgroundEntry - Constructs a color entry to set the background
|||	                          color of the display.
|||
|||	  Synopsis
|||
|||	    VDLEntry MakeCLUTBackgroundEntry (uint8 r,
|||					      uint8 g,
|||					      uint8 b)
|||
|||	  Description
|||
|||	    This macro constructs a VDL color entry that sets the background
|||	    color of the display according to the specified RGB components.
|||
|||	  Arguments
|||
|||	    r                            An unsigned byte specifying the red
|||	                                 component of the color to be built, with a
|||	                                 value in the range of 0-255.
|||
|||	    g                            An unsigned byte specifying the green
|||	                                 component of the color to be built, with a
|||	                                 value in the range of 0-255.
|||
|||	    b                            An unsigned byte specifying the blue
|||	                                 component of the color to be built, with a
|||	                                 value in the range of 0-255.
|||
|||	  Return Value
|||
|||	    The macro returns a 32-bit color entry value.
|||
|||	  Implementation
|||
|||	    Macro implemented in graphics.h V20.
|||
|||	  Associated File
|||
|||	    graphics.h
|||
|||	  See Also
|||
|||	    SetScreenColor(), MakeCLUTGreenEntry(), MakeCLUTBlueEntry(),
|||	    MakeCLUTRedEntry(), MakeCLUTColorEntry()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/makeclutcolorentry
|||	MakeCLUTColorEntry - Packs a color index and red, green, and blue values
|||	                     into a single word.
|||
|||	  Synopsis
|||
|||	    VDLEntry MakeCLUTColorEntry (uint8 index,
|||					 uint8 r,
|||					 uint8 g,
|||					 uint8 b)
|||
|||	  Description
|||
|||	    This macro takes four color-specification arguments and packs them
|||	    together into a single 32-bit word.  The color-entry word generated by
|||	    MakeCLUTColorEntry() is suitable for passing to functions such as
|||	    SetScreenColor() that expect this kind of argument.
|||
|||	  Arguments
|||
|||	    index                        An unsigned byte specifying an index
|||	                                 into a color palette, with a value in the
|||	                                 range of 0-31 for normal colors and 32 for
|||	                                 the background color.
|||
|||	    r                            An unsigned byte specifying the red
|||	                                 component of the color to be built, with a
|||	                                 value in the range of 0-255.
|||
|||	    g                            An unsigned byte specifying the green
|||	                                 component of the color to be built, with a
|||	                                 value in the range of 0-255.
|||
|||	    b                            An unsigned byte specifying the blue
|||	                                 component of the color to be built, with a
|||	                                 value in the range of 0-255.
|||
|||	  Return Value
|||
|||	    The macro returns a 32-bit color entry value.
|||
|||	  Implementation
|||
|||	    Macro implemented in graphics.h V20.
|||
|||	  Associated File
|||
|||	    graphics.h
|||
|||	  See Also
|||
|||	    SetScreenColor(), MakeCLUTGreenEntry(), MakeCLUTBlueEntry(),
|||	    MakeCLUTRedEntry()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/makeclutblueentry
|||	MakeCLUTBlueEntry - Packs a color index and blue values into a single
|||	                    word.
|||
|||	  Synopsis
|||
|||	    VDLEntry MakeCLUTBlueEntry (uint8 index,
|||					uint8 b)
|||
|||	  Description
|||
|||	    This macro takes a blue color-specification argument and packs it into a
|||	    single 32-bit word.  The color-entry word generated by MakeCLUTBlueEntry()
|||	    is suitable for  passing to functions such as SetScreenColor() that
|||	    expect this kind of argument.
|||
|||	  Arguments
|||
|||	    index                        An unsigned byte specifying an index
|||	                                 into a color palette, with a value in the
|||	                                 range of 0-31 for normal colors and 32 for
|||	                                 the background color.
|||
|||	    b                            An unsigned byte specifying the blue
|||	                                 component of the color to be built, with a
|||	                                 value in the range of 0-255.
|||
|||	  Return Value
|||
|||	    The macro returns a 32-bit color entry value.
|||
|||	  Implementation
|||
|||	    Macro implemented in graphics.h V20.
|||
|||	  Associated File
|||
|||	    graphics.h
|||
|||	  See Also
|||
|||	    SetScreenColor(), MakeCLUTColorEntry(), MakeCLUTGreenEntry(),
|||	    MakeCLUTRedEntry()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/makeclutgreenentry
|||	MakeCLUTGreenEntry - Packs a color index and green values into a single
|||	                     word.
|||
|||	  Synopsis
|||
|||	    VDLEntry MakeCLUTGreenEntry (uint8 index,
|||					 uint8 g)
|||
|||	  Description
|||
|||	    This macro takes a green color-specification argument and packs it into a
|||	    single 32-bit word.  The color-entry word generated by
|||	    MakeCLUTGreenEntry() is suitable for passing to functions such as
|||	    SetScreenColor() that expect this kind of argument.
|||
|||	  Arguments
|||
|||	    index                        An unsigned byte specifying an index
|||	                                 into a color palette, with a value in the
|||	                                 range of 0-31 for normal colors and 32 for
|||	                                 the background color.
|||
|||	    g                            An unsigned byte specifying the green
|||	                                 component of the color to be built, with a
|||	                                 value in the range of 0-255.
|||
|||	  Return Value
|||
|||	    The macro returns a 32-bit color entry value.
|||
|||	  Implementation
|||
|||	    Macro implemented in graphics.h V20.
|||
|||	  Associated File
|||
|||	    graphics.h
|||
|||	  See Also
|||
|||	    SetScreenColor(), MakeCLUTColorEntry(), MakeCLUTRedEntry(),
|||	    MakeCLUTBlueEntry()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/makeclutredentry
|||	MakeCLUTRedEntry - Packs a color index and red values into a single word.
|||
|||	  Synopsis
|||
|||	    VDLEntry MakeCLUTRedEntry (uint8 index,
|||				       uint8 r)
|||
|||	  Description
|||
|||	    This macro takes a red color-specification argument and packs it into a
|||	    single 32-bit word.  The color-entry word generated by MakeCLUTRedEntry()
|||	    is suitable for passing to functions such as SetScreenColor() that expect
|||	    this kind of argument.
|||
|||	  Arguments
|||
|||	    index                        An unsigned byte specifying an index
|||	                                 into a color palette, with a value in the
|||	                                 range of 0-31 for normal colors and 32 for
|||	                                 the background color.
|||
|||	    r                            An unsigned byte specifying the red
|||	                                 component of the color to be built, with a
|||	                                 value in the range of 0-255.
|||
|||	  Return Value
|||
|||	    The macro returns a 32-bit color entry value.
|||
|||	  Implementation
|||
|||	    Macro implemented in graphics.h V20.
|||
|||	  Associated File
|||
|||	    graphics.h
|||
|||	  See Also
|||
|||	    SetScreenColor(), MakeCLUTColorEntry(), MakeCLUTGreenEntry(),
|||	    MakeCLUTBlueEntry()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/makergb15
|||	MakeRGB15 - Creates a 3DO RGB value out of separate red, green, and blue
|||	            values.
|||
|||	  Synopsis
|||
|||	    uint32 MakeRGB15 (r, g, b)
|||
|||	  Description
|||
|||	    This macro creates a 3DO RGB value out of separate red, green,  and blue
|||	    values.  The composite RGB value created by this macro is suitable for
|||	    writing to a bitmap, a CCB's PLUT, or anywhere else that the default
|||	    RGB data type is called for.
|||
|||	  Arguments
|||
|||	    r                            The red value to be included in the
|||	                                 RGB composite.  This can be any integer
|||	                                 value; the value is coerced by the macro to
|||	                                 a uint32 value.
|||
|||	    g                            The green value to be included in the
|||	                                 RGB composite.  This can be any integer
|||	                                 value; the value is coerced by the macro to
|||	                                 a uint32 value.
|||
|||	    b                            The blue value to be included in the
|||	                                 RGB composite.  This can be any integer
|||	                                 value; the value is coerced by the macro to
|||	                                 a uint32 value.
|||
|||	    r, g, and b must all be in the range 0-31.
|||
|||	  Return Value
|||
|||	    The macro returns a composite RGB value.
|||
|||	  Implementation
|||
|||	    Macro implemented in graphics.h V20.
|||
|||	  Associated File
|||
|||	    graphics.h
|||
|||	  See Also
|||
|||	    MakeRGB15Pair()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/makergb15pair
|||	MakeRGB15Pair - Creates a double 3DO RGB value out of separate red, green,
|||	                and blue values.
|||
|||	  Synopsis
|||
|||	    uint32 MakeRGB15Pair (r, g, b)
|||
|||	  Description
|||
|||	    This macro creates a double 15-bit 3DO RGB value out of separate red,
|||	    green, and blue values.  The "double" part of the value comes from
|||	    creating a 16-bit value and then merging it with a copy of itself shifted
|||	    over 16 bits.  The composite double RGB value created by this macro is
|||	    suitable to be used as an argument toSetVRAMPages(), or anywhere else that
|||	    the double RGB data type is called for.
|||
|||	  Arguments
|||
|||	    r                            The red value to be included in the
|||	                                 RGB composite.  This can be any integer
|||	                                 value; the value is coerced by the macro to
|||	                                 a uint32 value.
|||
|||	    g                            The green value to be included in the
|||	                                 RGB composite.  This can be any integer
|||	                                 value; the value is coerced by the macro to
|||	                                 a uint32 value.
|||
|||	    b                            The blue value to be included in the
|||	                                 RGB composite.  This can be any integer
|||	                                 value; the value is coerced by the macro to
|||	                                 a uint32 value.
|||
|||	    r, g, and b must all be in the range 0-31.
|||
|||	  Return Value
|||
|||	    The macro returns a composite-double RGB15 value.
|||
|||	  Implementation
|||
|||	    Macro implemented in graphics.h V20.
|||
|||	  Associated File
|||
|||	    graphics.h
|||
|||	  See Also
|||
|||	    MakeRGB15()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/mapcel
|||	MapCel - Maps this cel onto any four corners.
|||
|||	  Synopsis
|||
|||	    void MapCel (CCB *ccb,
|||			 Point *quad)
|||
|||	  Description
|||
|||	    This function maps a cel onto the four points specified by the quad
|||	    argument.  The mapping is created by setting the delta fields in the
|||	    specified ccb argument.  The deltas will cause the cel to be rendered such
|||	    that the four corners of the imagery map to the four corners.
|||
|||	    The quad argument points to an array of four-point structures.  In the
|||	    normal configuration of the quad, the first entry refers to the top-left
|||	    corner of the imagery, the second entry refers to top-right, the third to
|||	    bottom-right, and the fourth to bottom-left.  Those corners of the cel
|||	    imagery map to those four entries in the quad, no matter what values are
|||	    given to the quad entries.
|||
|||	    This function expects that the CCB structure's width and height
|||	    fields have valid values (the width and height fields are add-ons that
|||	    aren't required by the cel engine hardware.)
|||
|||	  Arguments
|||
|||	    ccb                          Pointer to the CCB structure
|||	                                 whose delta fields are modified to map to
|||	                                 the quad.
|||
|||	    quad                         Pointer to a four-entry array of
|||	                                 point structures.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    FastMapCel(), FastMapCelf16(), FastMapCelInit()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/modifyvdl
|||	ModifyVDL - Changes one or more attributes of a VDL.
|||
|||	  Synopsis
|||
|||	    Err ModifyVDL (Item vdlItem,
|||			   TagArg *vdlTags)
|||
|||	    Err ModifyVDLVA (Item vdlItem,
|||			     uint32 vdlTags, ...)
|||
|||	  Description
|||
|||	    This function is used to change one or more attributes of a VDL.  When
|||	    executed, it looks through each entry of the VDL for opcodes, and then
|||	    changes the specified attributes in each opcode (if they need changing).
|||	    This means that ModifyVDL() affects the entire VDL, regardless of its
|||	    type.  It works equally well on simple VDLs and custom VDLs.
|||
|||	    This is a preliminary implementation of ModifyVDL().  Future
|||	    implementations will include the ability to modify individual VDL entries.
|||
|||	  Arguments
|||
|||	    vdlItem                      Item number of the VDL to be
|||	                                 modified.
|||
|||	    vdlTags                      Pointer to the array of TagArgs used
|||	                                 to specify the modifications to the VDL.
|||
|||	    These are the values of the possible tags and their effects.  The effects
|||	    are described in the "Graphic Programmer's Guide."
|||
|||	  Return Value
|||
|||	    The function returns a zero if successful or an error code (a negative
|||	    value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V21.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    SubmitVDL(), SetVDL(), DeleteVDL()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/moveto
|||	MoveTo - Moves the pen position of a GrafCon to a new location.
|||
|||	  Synopsis
|||
|||	    void MoveTo (GrafCon *gc,
|||			 Coord x,
|||			 Coord y)
|||
|||	  Description
|||
|||	    This function moves the GrafCon's pen position to a new location.
|||	    After this move, functions such as DrawTo() and CopyRect() will use this
|||	    pen position to do their work.
|||
|||	  Arguments
|||
|||	    gc                           Pointer to a GrafCon
|||	                                 structure.
|||
|||	    x                            Column to which to move the pen.
|||
|||	    y                            Row to which to move the pen.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    DrawTo()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/opengraphicsfolio
|||	OpenGraphicsFolio - Opens the graphics folio for use by the calling task.
|||
|||	  Synopsis
|||
|||	    Err OpenGraphicsFolio (void)
|||
|||	  Description
|||
|||	    This function issues an OpenItem() call to open the graphics folio for
|||	    use by the calling task.  If the graphics folio is successfully opened,
|||	    this function sets the global variable GrafFolioNum to the item number of
|||	    the graphics folio item.  It also sets the global variable GrafBase to the
|||	    pointer to the graphics folio's GrafFolio structure.  GrafBase must be
|||	    properly initialized before calling any of the graphics folio functions.
|||
|||	    This function is a convenience call that is provided in the graphics.lib,
|||	    and is not part of the graphics folio.
|||
|||	    After the graphics folio is opened, the task may use any of the other
|||	    graphics folio functions.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in graphics.lib V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    CloseGraphicsFolio()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/querygraphics
|||	QueryGraphics - Gets graphics environment information.
|||
|||	  Synopsis
|||
|||	    Err QueryGraphics (int32 tag,
|||			       void *ret)
|||
|||	  Description
|||
|||	    QueryGraphics looks up a value in the environment based on the passed-in
|||	    tag and deposits that value in the address pointed to by ret.
|||
|||	  Tag Arguments
|||
|||	    The currently available tags and their return values follow:
|||
|||	    * QUERYGRAF_TAG_END - No return value.
|||
|||	    * QUERYGRAF_TAG_FIELDFREQ - int32 - Returns the display field
|||	      frequency (in Hz) of the current environment. The return value is 60 for
|||	      NTSC and 50 for PAL.
|||
|||	    * QUERYGRAF_TAG_FIELDTIME - int32 - Returns the length of time for a
|||	      single video display in usec.
|||
|||	    * QUERYGRAF_TAG_FIELDCOUNT - uint32 - Returns a number that is incremented
|||	      every vblank period.
|||
|||	    * QUERYGRAF_TAG_DEFAULTWIDTH - int32 - Returns the default display width
|||	      for the current hardware.  The return value will be 320 on every machine
|||	      that is capable of generating a 320-pixel-wide display.  This may not
|||	      be the preferred width for that environment (PAL for instance).
|||
|||	    * QUERYGRAF_TAG_DEFAULTHEIGHT - int32 - Returns the default display height
|||	      for the current hardware.  The return value will be 240 on
|||	      every machine that is capable of generating a 240-line-tall display.  This
|||	      may not be the preferred height for that environment (PAL for instance).
|||
|||	    * QUERYGRAF_TAG_DEFAULTDISPLAYTYPE - int32 - Returns the default display type
|||	      in this environment.  DI_TYPE_NTSC will be returned on current NTSC
|||	      machines.  DI_TYPE_PAL1 will be returned on current PAL machines.
|||
|||	  Arguments
|||
|||	    tag                          tag number indicating information
|||	                                 requested.
|||
|||	    ret                          pointer to the location in memory to
|||	                                 deposit the requested value.
|||
|||	  Return Value
|||
|||	    The function returns 0 if it was successful or an error code (a negative
|||	    value) if an error occurred.
|||
|||	    GRAFERR_BADTAG will be returned if an unknown tag is passed in.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V21.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    QueryGraphicsList()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/querygraphicslist
|||	QueryGraphicsList - Gets graphics environment information.
|||
|||	  Synopsis
|||
|||	    Err QueryGraphicsList (TagArg *ta)
|||
|||	    Err QueryGraphicsListVA (uint32 ta, ...)
|||
|||	  Description
|||
|||	    The function looks up values in the environment based on passed in the
|||	    ta_Tag elements of the TagArg array, and deposits those values in the
|||	    addresses pointed to by the ta_Arg associated values.
|||
|||	    The currently available tags and their return values follow:
|||
|||	    * QUERYGRAF_TAG_END - No return value. Ends processing of the tag list.
|||
|||	    * QUERYGRAF_TAG_FIELDFREQ - int32 - Returns the display field frequency (in
|||	      Hz) of the current environment.  The return value is 60 for NTSC and 50
|||	      for PAL.QUERYGRAF_TAG_FIELDTIME  int32  Returns the length of time for a
|||	      single video display in nsec.
|||
|||	    * QUERYGRAF_TAG_FIELDCOUNT - uint32 - Returns a number that is
|||	      incremented every vblank period.
|||
|||	    * QUERYGRAF_TAG_DEFAULTWIDTH - int32 - Returns the default display width for
|||	      the current hardware.  The return value will be 320 on every machine that
|||	      is capable of generating a 320-pixel-wide display.  This may not be the
|||	      preferred width for that environment (PAL for instance).
|||
|||	    * QUERYGRAF_TAG_DEFAULTHEIGHT - int32 - Returns the default display height for
|||	      the current hardware.  The return value will be 240 on every machine that
|||	      is capable of generating a 240-line-tall display.  This may not be the
|||	      preferred height for that environment (PAL for instance).
|||
|||	    * QUERYGRAF_TAG_DEFAULTDISPLAYTYPE - int32 - Returns the default display type
|||	      in this environment.  DI_TYPE_NTSC will be returned on current NTSC
|||	      machines.  DI_TYPE_PAL1 will be returned on current PAL machines.
|||
|||	  Arguments
|||
|||	    tag                          tag number indicating information
|||	                                 requested
|||
|||	    ret                          pointer to the location in memory to
|||	                                 deposit the requested value.
|||
|||	  Return Value
|||
|||	    The function returns 0 if it was successful or an error code (a negative
|||	    value) if an error occurred.
|||
|||	    GRAFERR_BADTAG will be returned if an unknown tag is passed in.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V21.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    QueryGraphics()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/readpixel
|||	ReadPixel - Reads a pixel from the display currently associated with a
|||	            GrafCon.
|||
|||	  Synopsis
|||
|||	    Color ReadPixel (Item bitmapItem,
|||			     GrafCon *gc,
|||			     Coord x,
|||			     Coord y)
|||
|||	  Description
|||
|||	    This function reads a pixel from the display of the specified GrafCon.
|||	    The x and y coordinates specify the offset into the pixmap of the
|||	    GrafCon's bitmap.  The value of the pixel there is read and returned
|||	    to the caller.
|||
|||	  Arguments
|||
|||	    bitmapItem                   The item number of the bitmap.
|||
|||	    gc                           Pointer to a GrafCon structure.
|||
|||	    x                            column of the pixel to be read.
|||
|||	    y                            Row of the pixel to be read.
|||
|||	  Return Value
|||
|||	    This function returns the pixel value at the specified location or an
|||	    error code (a negative value) if an error occurs.
|||
|||	    GRAFERR_COORDRANGE is returned if the specified location is outside the
|||	    bounds of the specified bitmap.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    GetPixelAddress()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/removescreengroup
|||	RemoveScreenGroup - Removes a screen group from the display.
|||
|||	  Synopsis
|||
|||	    Err RemoveScreenGroup (Item screenGroup)
|||
|||	  Description
|||
|||	    The function removes the specified screenGroup from the graphics
|||	    folio's display mechanism.  After this call, none of the screens of
|||	    the group can be made visible.
|||
|||	    If one of the screens of the group is visible when this call is made, the
|||	    screen is removed from the display.
|||
|||	  Arguments
|||
|||	    screenGroup                  Item number of the ScreenGroup to be
|||	                                 removed.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    AddScreenGroup()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/resetcurrentfont
|||	ResetCurrentFont - Resets the system's default font.
|||
|||	  Synopsis
|||
|||	    Err ResetCurrentFont (void)
|||
|||	  Description
|||
|||	    This function resets the system font to the default font.  It is
|||	    suggested that you call ResetCurrentFont() at the start of the program
|||	    run, if you are using any of the font functions.  You must call
|||	    ResetCurrentFont() when your program exits if you changed the system font
|||	    in your program.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    GetCurrentFont()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/resetreadaddress
|||	ResetReadAddress - Resets a bitmap's read address.
|||
|||	  Synopsis
|||
|||	    Err ResetReadAddress (Item bitmapItem)
|||
|||	  Description
|||
|||	    This function resets the read pointer to the bitmap to be the same as the
|||	    write pointer, which is the default setting.
|||
|||	  Arguments
|||
|||	    bitmapItem                   Item number of the bitmap structure
|||	                                 whose read address is to be reset.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    SetReadAddress()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/resetscreencolors
|||	ResetScreenColors - Resets a screen's colors to a linear palette.
|||
|||	  Synopsis
|||
|||	    Err ResetScreenColors (Item screenItem)
|||
|||	  Description
|||
|||	    The function resets the colors of the specified screen.  The colors are
|||	    set to a linear ascending palette.
|||
|||	  Arguments
|||
|||	    screenItem                   Item number of the screen structure
|||	                                 whose palette is to be reset.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/setbgpen
|||	SetBGPen - Sets the background pen of a GrafCon.
|||
|||	  Synopsis
|||
|||	    void SetBGPen (GrafCon *gc,
|||			   Color c)
|||
|||	  Description
|||
|||	    This function sets the background pen of the specified GrafCon to the
|||	    specified color.  The color value is a coded 15-bit RGB format value, with
|||	    each 5-bit value acting as an index into the CLUT color registers.  After
|||	    this call, the surrounding white space of text (and other graphics
|||	    elements that use the background pen) will be rendered in the new
|||	    background color.
|||
|||	    Note that the color values produced here are different from the color
|||	    values produced by MakeCLUTColorEntry().  Here is an example showing how
|||	    to create a color value:
|||
|||	    color = (red << 10) || (green << 5) || (blue << 0);
|||
|||	    Or you could use Use MakeRGB15().
|||
|||	  Arguments
|||
|||	    gc                           Pointer to a GrafCon
|||	                                 structure.
|||
|||	    c                            Value to be stored as the new
|||	                                 background pen color.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    MakeRGB15(), SetFGPen()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/setcecontrol
|||	SetCEControl - Sets the cel engine control word.
|||
|||	  Synopsis
|||
|||	    Err SetCEControl (Item bitmapItem,
|||			      int32 controlWord,
|||			      int32 controlMask)
|||
|||	  Description
|||
|||	    This function sets the cel engine control word of the bitmap specified by
|||	    the bitmapItem argument.  After this call, the newly modified control word
|||	    will be used whenever the bitmap is used for cel rendering.  The control
|||	    mask argument (controlMask) controls those bits of the bitmap's
|||	    control word that will be modified by a call to SetCEControl().
|||
|||	  Arguments
|||
|||	    bitmapItem                   Item number of a bitmap structure.
|||
|||	    controlWord                  Value for the new control word for
|||	                                 the bitmap.
|||
|||	    controlMask                  Bit mask that controls whether the
|||	                                 corresponding bit in controlWord will be
|||	                                 transferred to the CEControl word.  If the
|||	                                 controlMask bit is set, the corresponding
|||	                                 bit in controlWord is transferred to the
|||	                                 CEControl word; if the controlMask bit is
|||	                                 clear, the corresponding bit in controlWord
|||	                                 is ignored.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    DrawCels(), DrawScreenCels()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/setcewatchdog
|||	SetCEWatchDog - Sets the length of time the cel engine can do one draw.
|||
|||	  Synopsis
|||
|||	    Err SetCEWatchDog (Item bitmapItem,
|||			       int32 db_ctr)
|||
|||	  Description
|||
|||	    This function controls how long DrawCels() lets the cel engine do one
|||	    draw before it is killed.  The delay interval is expressed in
|||	    microseconds.  Its intent is to give you a way to abort cel engine
|||	    lock-ups (which might be caused by bad data, a linked list that is a loop,
|||	    or other problems.)
|||
|||	    Currently, the default counter is 1,000,000 microseconds (1 second).
|||
|||	  Arguments
|||
|||	    bitmapItem                   Item number of a bitmap structure.
|||
|||	    db_ctr                       Value for the draw counter, in
|||	                                 microseconds.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	    GRAFERR_BADDEADBOLT will be returned if an illegal value is specified.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    DrawCels(), DrawScreenCels()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/setclipheight
|||	SetClipHeight - Sets the clip height of a bitmap.
|||
|||	  Synopsis
|||
|||	    Err SetClipHeight (Item bitmapItem,
|||			       int32 clipHeight)
|||
|||	  Description
|||
|||	    This function uses the clipHeight argument to set the clip height of the
|||	    bitmap specified by the bitmapItem argument.  The clipHeight must be no
|||	    greater than the height of the bitmap's buffer.
|||
|||	    After setting the clip width and height, rendering to the bitmap (both
|||	    with the rendering primitives and with calls to the cel engine) will be
|||	    clipped to stay within the clip rectangle.
|||
|||	  Arguments
|||
|||	    bitmapItem                   Item number of a bitmap structure.
|||
|||	    clipHeight                   Value for the new clip height for the
|||	                                 bitmap.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    SetClipWidth(), SetClipOrigin()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/setcliporigin
|||	SetClipOrigin - Sets the clip origin of a bitmap.
|||
|||	  Synopsis
|||
|||	    Err SetClipOrigin (Item bitmapItem,
|||			       int32 x,
|||			       int32 y)
|||
|||	  Description
|||
|||	    This function sets the clip origin of a bitmap.  You specify the item
|||	    number of a bitmap and the x and y coordinates that specify the top-left
|||	    corner of the clip rectangle within the bitmap.
|||
|||	    Note that the (0,0) coordinate always refers to the top-left corner of the
|||	    clip window, regardless of whether the clip window is set to the full size
|||	    of the bitmap or to some subregion of the bitmap.  The clip rectangle that
|||	    you specify must fit within the parameters of the bitmap; that is, the
|||	    clip x offset plus the clip window width must not exceed the actual width
|||	    of the bitmap.  The same is true for the height, or y, offset.  The order
|||	    of operations setting width/height first or setting origin first is
|||	    significant.
|||
|||	    If the read address is the same as the write address (this is normal),
|||	    then the read buffer address is modified the same as the write buffer
|||	    address (this is what you would expect).  But, if you have used
|||	    SetReadAddress() to set the read address to something other than the write
|||	    buffer, then the read address is not disturbed by the call to
|||	    SetClipOrigin().
|||
|||	  Arguments
|||
|||	    bitmapItem                   Item number of a bitmap structure.
|||
|||	    x                            Value for the top left corner x
|||	                                 coordinate.
|||
|||	    y                            Value for the top left corner y
|||	                                 coordinate  (At this time, if the y argument
|||	                                 is an odd number, it is forced to the next
|||	                                 numerically lower even number, and a warning
|||	                                 is returned.)
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    SetClipWidth()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/setclipwidth
|||	SetClipWidth - Sets the clip width of a bitmap.
|||
|||	  Synopsis
|||
|||	    Err SetClipWidth (Item bitmapItem,
|||			      int32 clipWidth)
|||
|||	  Description
|||
|||	    This function uses the clipWidth argument to set the clip width of the
|||	    bitmap specified by the bitmapItem argument.  The clipWidth must be no
|||	    greater than the width of the bitmap's buffer.
|||
|||	    After setting the clip width and height, rendering to the bitmap (both
|||	    with the rendering primitives and with calls to the cel engine) will be
|||	    clipped to stay within the clip rectangle.
|||
|||	  Arguments
|||
|||	    bitmapItem                   Item number of a bitmap structure.
|||
|||	    clipWidth                    Value for the new clip width for the
|||	                                 bitmap.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    SetClipWidth(), SetClipOrigin()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/setcurrentfontccb
|||	SetCurrentFontCCB - Sets the font.
|||
|||	  Synopsis
|||
|||	    Err SetCurrentFontCCB (CCB *ccb)
|||
|||	  Description
|||
|||	    This function sets the CCB for the current font.  If you want to modify
|||	    the CCB of the current font, you need to do the following:
|||
|||	    * call GetCurrentFont()
|||
|||	    * copy the CCB pointed to by the font structure
|||
|||	    * edit your copy of the CCB
|||
|||	    * then call SetCurrentFontCCB()
|||
|||	    When you submit a CCB, the ccb_PLUTPtr field should either be significant
|||	    or set to NULL.  The font flag, FONT_VERTICAL, determines whether
|||	    characters are printed vertically (top to bottom) or horizontally (left to
|||	    right).  If the flag is set, the characters are printed vertically.
|||
|||	  Arguments
|||
|||	    ccb                          Pointer to the CCB.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    GetCurrentFont()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/setfgpen
|||	SetFGPen - Sets the foreground pen of a GrafCon.
|||
|||	  Synopsis
|||
|||	    void SetFGPen (GrafCon *gc,
|||			   Color c)
|||
|||	  Description
|||
|||	    This function sets the foreground pen of the specified GrafCon to the
|||	    specified color.  The color value is a coded 15-bit RGB format value, with
|||	    each 5-bit value acting as an index into the CLUT color registers.  After
|||	    this call, the imagery part of text (and other graphics elements that use
|||	    the foreground pen, which include line drawing, rectangle and ellipse
|||	    rendering) will be rendered in the new foreground color.
|||
|||	    Note that the color values produced here are different from the color
|||	    values produced by MakeCLUTColorEntry().  The example shows how to create
|||	    a color value:
|||
|||	    color = (red << 10) || (green << 5) || (blue << 0);
|||
|||	    Or you could use MakeRGB15();
|||
|||	  Arguments
|||
|||	    gc                           Pointer to a GrafCon
|||	                                 structure.
|||
|||	    c                            Value to be stored as the new
|||	                                 foreground pen color.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    MakeRGB15(), SetBGPen()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/setreadaddress
|||	SetReadAddress - Sets a bitmap's read address.
|||
|||	  Synopsis
|||
|||	    Err SetReadAddress (Item bitmapItem,
|||				ubyte *buffer,
|||				int32 width)
|||
|||	  Description
|||
|||	    This function sets the read pointer to the bitmap to the specified
|||	    buffer, which  has the specified pixel width.  After this call, the
|||	    specified buffer would be used when cel renderings take place that include
|||	    reading from a source buffer before writing to the destination buffer.
|||
|||	  Arguments
|||
|||	    bitmapItem                   Item number of the bitmap
|||	                                 structure whose read address is to be set.
|||
|||	    buffer                       Pointer to the data buffer to be used
|||	                                 as the read buffer.
|||
|||	    width                        Pixel count of the width of the
|||	                                 buffer.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    ResetReadAddress()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/setscreencolor
|||	SetScreenColor - Sets a color in the system's palette.
|||
|||	  Synopsis
|||
|||	    Err SetScreenColor (Item screenItem,
|||				int32 colorEntry)
|||
|||	  Description
|||
|||	    This function sets a color in the palette of the screen specified by the
|||	    screenItem argument.  The colorEntry argument is a packed color value
|||	    containing an index and individual red, green, and blue components.
|||
|||	    Each of the four components in the colorEntry argument consists of one
|||	    byte of the 4-byte word.  The index is the high-order byte, followed by
|||	    bytes of red, green, and blue, with the blue value in the low-order byte.
|||	    The colorEntry argument can be constructed using the MakeCLUTColorEntry()
|||	     macro.
|||
|||	  Arguments
|||
|||	    screenItem                   Item number of a screen structure.
|||
|||	    colorEntry                   32-bit color entry comprising an
|||	                                 index, a red, a green, and a blue value.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	    GRAFERR_INDEXRANGE will be returned if the index value in the color entry
|||	    is larger than 32.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    MakeCLUTColorEntry(), ReadCLUTColor()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/setscreencolors
|||	SetScreenColors - Sets a number of colors in the system's palette.
|||
|||	  Synopsis
|||
|||	    Err SetScreenColors (Item screenItem,
|||				 uint32 *entries,
|||				 int32 count)
|||
|||	  Description
|||
|||	    This function sets a number of colors in the palette of the screen
|||	    specified by the screenItem argument.  The entries argument points to an
|||	    array of packed color values, each containing an index and individual red,
|||	    green, and blue components.  The number of entries is specified by the
|||	    count argument.
|||
|||	    Each of the four components in each entry's argument consists of one
|||	    byte of the 4-byte word.  The index is the high-order byte, followed by
|||	    bytes of red, green, and blue, with the blue value in the low-order byte.
|||	    The entries argument can be constructed using the MakeCLUTColorEntry()
|||	     macro.
|||
|||	  Arguments
|||
|||	    screenItem                   Item number of a screen
|||	                                 structure.
|||
|||	    entries                      Pointer to an array of 32-bit color
|||	                                 entries, each of which comprises an index, a
|||	                                 red, a green, and a blue value.
|||
|||	    count                        Number of entries in the color array.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	    GRAFERR_INDEXRANGE will be returned if one or more color entries has an
|||	    index value greater than 32.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    MakeCLUTColorEntry()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/setvdl
|||	SetVDL - Sets a screen to display a specific VDL.
|||
|||	  Synopsis
|||
|||	    Err SetVDL (Item screenItem,
|||			Item vdlItem)
|||
|||	  Description
|||
|||	    This function associates the VDL specified by the vdlItem argument with
|||	    the screen specified by the screenItem argument.
|||
|||	    Every screen has an associated VDL.  After this call, when DisplayScreen()
|||	     is called referring to the specified screen the new VDL will be used to
|||	    create the display.
|||
|||	  Arguments
|||
|||	    screenItem                   Item number of a screen structure.
|||
|||	    vdlItem                      Item number of the associated VDL.
|||
|||	  Return Value
|||
|||	    This function returns the item number of the current VDL (the last one
|||	    submitted) or an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    DisplayScreen(), SubmitVDL(), ModifyVDL(), DeleteVDL()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/setvrampages
|||	SetVRAMPages - Sets contiguous pages of VRAM to a specified value.
|||
|||	  Synopsis
|||
|||	    Err SetVRAMPages (Item ioreq,
|||			      void *dest,
|||			      int32 value,
|||			      int32 numpages,
|||			      int32 mask)
|||
|||	  Description
|||
|||	    This convenience routine allows one or more pages of VRAM, referred to by
|||	    the dest argument, to be set to the specified value.  The number of VRAM
|||	    pages set is controlled by the numPages argument.  Pages can be set to any
|||	    value, via the immediate Flashwrite mode in the VRAM.  As expected, it is
|||	    very fast.
|||
|||	    This function uses the SPORT device to do the transfer.  It takes effect
|||	    immediately; there is no implicit wait for the next VBlank to occur.
|||
|||	  Arguments
|||
|||	    ioreq                        Item number of the I/O request.
|||
|||	    dest                         Pointer to the first byte of
|||	                                 the first page of the destination VRAM.
|||
|||	    value                        32-bit value that is to be written to
|||	                                 the VRAM pages.
|||
|||	    numpages                     Number of destination pages to
|||	                                 receive the copy of the source page.
|||
|||	    mask                         Write mask, which controls which bits
|||	                                 of the source get copied to the destination.
|||
|||	  Return Value
|||
|||	    The function returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Notes
|||
|||	    The use of a mask value of other than all ones (0xFFFFFFFF) is
|||	    discouraged.  Future systems may be unable to support this
|||	    feature in a performance-conscious manner.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in graphics.lib V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    CloneVRAMPages(), CopyVRAMPages(), SetVRAMPagesDefer(),
|||	    CopyVRAMPagesDefer(), CloneVRAMPagesDefer(), GetVRAMIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/setvrampagesdefer
|||	SetVRAMPagesDefer - Sets contiguous pages of VRAM to a specified value.
|||
|||	  Synopsis
|||
|||	    Err SetVRAMPagesDefer (Item ioreq,
|||				   void *dest,
|||				   int32 val,
|||				   int32 numpages,
|||				   int32 mask)
|||
|||	  Description
|||
|||	    This function allows one or more pages of VRAM, referred to by the dest
|||	    argument, to be set to the specified value.  The number of VRAM pages set
|||	    is controlled by the numPages argument.  Pages can be set to any value,
|||	    via the immediate Flashwrite mode of the VRAM.  As expected, it is very
|||	    fast.  SetVRAMPagesDefer() does not issue a WaitIO() on the I/O request.
|||	    It is up to the calling task to issue the WaitIO() at some time before the
|||	    I/O request can be reused.
|||
|||	    This function uses the SPORT device to do the transfer.  It takes effect
|||	    immediately; there is no implicit wait for the next VBlank to occur.
|||
|||	  Arguments
|||
|||	    ioreq                        Item number of the I/O request.
|||
|||	    dest                         Pointer to the first byte of
|||	                                 the first page of the destination VRAM.
|||
|||	    val                          32-bit value that is to be written to
|||	                                 the VRAM pages.
|||
|||	    numpages                     Number of destination pages to
|||	                                 receive the copy of the source page.
|||
|||	    mask                         Write mask, which controls which bits
|||	                                 of the source get copied to the destination.
|||
|||	  Return Value
|||
|||	    The function returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Notes
|||
|||	    The use of a mask value of other than all ones (0xFFFFFFFF) is
|||	    discouraged.  Future systems may be unable to support this
|||	    feature in a performance-conscious manner.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in graphics.lib V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    CloneVRAMPages(), CopyVRAMPages(), SetVRAMPages(), CopyVRAMPagesDefer(),
|||	    CloneVRAMPagesDefer(), GetVRAMIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/submitvdl
|||	SubmitVDL - Submits a VDL for approval by the system.
|||
|||	  Synopsis
|||
|||	    Item SubmitVDL (VDLEntry *VDLDataPtr,
|||			    int32 length,
|||			    int32 type)
|||
|||	  Description
|||
|||	    This function accepts an absolute VDL specified by the VDLDataPtr
|||	    argument and proofs it for use in the system.  As part of the approval
|||	    process the VDL is copied into system data space and therefore can't
|||	    be directly modified by the application after the call to SubmitVDL().
|||
|||	  Arguments
|||
|||	    VDLDataPtr                   Pointer to the VDL to be approved by
|||	                                 the system.
|||
|||	    length                       The total length of the VDL in words.
|||
|||	    type                         The type of the VDL being submitted
|||	                                 (currently, only VDLTYPE_FULL is
|||					 accepted).
|||
|||	  Return Value
|||
|||	    This function returns an item number for the VDL or an error code (a
|||	    negative value) if an error occurs.
|||
|||	    GRAFERR_PROOF_ERR is returned if the VDL is rejected due to illegal
|||	    opcodes, bad headers, bad pointers, or illegal control values.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  Caveats
|||
|||	    Under certain circumstances, the system VDL that is created may need to
|||	    occupy more memory than the VDL list passed in.  This can be caused by
|||	    alignment constraints in the hardware.  One way to avoid the problem is to
|||	    pad every line in the input VDL list to a multiple of four words (but
|||	    without increasing the length count in the DMA control word).
|||
|||	  Bugs
|||
|||	    "Performance-conscious" is not a phrase I would use to describe
|||	    this routine...
|||
|||	  See Also
|||
|||	    DisplayScreen(), ModifyVDL(), SetVDL(), DeleteVDL()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/waitvbl
|||	WaitVBL - Waits the specified number of VBLs.
|||
|||	  Synopsis
|||
|||	    Err WaitVBL (Item ioreq,
|||			 uint32 numfields)
|||
|||	  Description
|||
|||	    This function is a convenience call that is actually not part of the
|||	    graphics folio.  It is included here because it resides in the
|||	    graphics.lib for the moment.  It actually uses the timer device, which is
|||	    completely independent of the graphics folio.
|||
|||	    This function allows the task to wait the specified number of VBLs.  The
|||	    I/O Request argument is set up to use the timer device.  The numfields
|||	    argument is a count of the number of video fields to wait for.
|||
|||	  Arguments
|||
|||	    ioreq                        The item number of the I/O request.
|||
|||	    numfields                    Number of VBLs to wait.
|||
|||	  Return Value
|||
|||	    The function returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Caveats
|||
|||	    Because of the multitasking nature of the system, your task may
|||	    not regain control until later than you may want.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in graphics.lib V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    WaitVBLDefer(), GetVBLIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/waitvbldefer
|||	WaitVBLDefer - Waits the specified number of VBLs.
|||
|||	  Synopsis
|||
|||	    Err WaitVBLDefer (Item ioreq,
|||			      uint32 numfields)
|||
|||	  Description
|||
|||	    This function is a convenience call that is actually not part of the
|||	    graphics folio.  It is included here because it resides in the
|||	    graphics.lib for the moment.  It actually uses the timer device, which is
|||	    completely independent of the graphics folio.
|||
|||	    This function allows the task to wait the specified number of VBLs.  The
|||	    I/O request argument is set up to use the timer device.  The numfields
|||	    argument is a count of the number of video fields to wait for.
|||
|||	    This function differs from WaitVBL() as it does not wait for the
|||	    operation to complete.  Instead, control returns to the calling
|||	    application immediately, which can do more work while the I/O request is
|||	    in the queue.  When using the deferred version of the request, WaitIO()
|||	    should be used when you want to wait for the completion of the I/O
|||	    request.
|||
|||	  Arguments
|||
|||	    ioreq                        The item number of the I/O request.
|||
|||	    numfields                    Number of VBLs to wait.
|||
|||	  Return Value
|||
|||	    The function returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in graphics.lib V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
|||	  See Also
|||
|||	    WaitVBL(), GetVBLIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC gpg/graphics/writepixel
|||	WritePixel - Writes a pixel to a bitmap in a GrafCon's foreground
|||	             color.
|||
|||	  Synopsis
|||
|||	    Err WritePixel (Item bitmapItem,
|||			    GrafCon *gc,
|||			    Coord x,
|||			    Coord y)
|||
|||	  Description
|||
|||	    This function writes a pixel to the specified bitmap's display.  The
|||	    pixel is rendered in the foreground color of the GrafCon structure.  It is
|||	    rendered on the column and row specified by the x and y arguments
|||	    respectively.
|||
|||	  Arguments
|||
|||	    bitmapItem                   Item number of a bitmap structure.
|||
|||	    gc                           Pointer to a GrafCon
|||	                                 structure.
|||
|||	    x                            Column where the pixel is to be
|||	                                 written.
|||
|||	    y                            Row where the pixel is to be written.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	    GRAFERR_COORDRANGE is returned if the specified location is ouside the
|||	    bitmap boundaries.
|||
|||	  Notes
|||
|||	    The current implementation of this routine is *very* slow.  This
|||	    will change in the future.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V20.
|||
|||	  Associated Files
|||
|||	    graphics.h, graphics.lib
|||
**/

/* keep the compiler happy... */
extern int foo;
