/* $Id: autodocs.c,v 1.12 1994/11/05 01:57:44 vertex Exp $ */

/**
|||	AUTODOC PUBLIC spg/lib3do/abortasyncloadfile
|||	AbortAsyncLoadFile - Abort an async file load in progress.
|||
|||	  Synopsis
|||
|||	    Err AbortAsyncLoadFile(LoadFileInfo *lf)
|||
|||	  Description
|||
|||	    This function aborts an asynchronous load already in progress.  It
|||	    works even if the load operation has already completed.
|||
|||	    If you asked AsyncLoadFile() to allocate the buffer then this
|||	    function frees the buffer.  If you specified an ioDonePort you
|||	    will NOT receive notification at that port after calling this
|||	    function.
|||
|||	    Do not call FinishAsyncLoadFile() after calling this function; this
|||	    function performs the Finish actions internally.
|||
|||	  Arguments
|||
|||	    lf                           A pointer to the LoadFileInfo structure
|||	                                 for the load operation.
|||
|||	  Return Value
|||
|||	    Returns ABORTED.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, blockfile.h
|||
|||	  See Also
|||
|||	    AsyncLoadFile()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/asyncloadfile
|||	AsyncLoadFile - Start an asynchronous file load operation.
|||
|||	  Synopsis
|||
|||	    Err AsyncLoadFile(char *fname, LoadFileInfo *lf)
|||
|||	  Description
|||
|||	    This function starts an asynchronous load of the specified file,
|||	    using the options in the LoadFileInfo structure.
|||
|||	    This function opens the file, gets status information (filesize,
|||	    etc) then starts the I/O.  Upon successful return from this
|||	    function you can refer to the buffer, bufSize, and bf
|||	    (BlockFileInfo) fields of the LoadFileInfo structure, even though
|||	    the I/O is still in progress.  The contents of the buffer, of
|||	    course, are indeterminate until the I/O completes.
|||
|||	    You must eventually follow a call to this function with a call to
|||	    either AbortAsyncLoadFile(), WaitAsyncLoadFile(), or
|||	    FinishAsyncLoadFile().
|||
|||	    The LoadFileInfo structure is defined in blockfile.h.  It contains values
|||	    that control the load operation.   If the buffer pointer in the structure
|||	    is NULL a buffer is allocated for you.  If non-NULL it is a pointer to a
|||	    buffer you allocated for the file.   If the ioDonePort value in the
|||	    structure is non-zero the I/O is set up to notify you via message when the
|||	    load completes.
|||
|||	    If you store a NULL buffer pointer into the LoadFileInfo structure and
|||	    allow this function to allocate a buffer for you, use the UnloadFile()
|||	    function to release the buffer when you no longer need its contents.
|||
|||	  Arguments
|||
|||	    fname                        The name of the file to load.
|||
|||	    lf                           A pointer to a LoadFileInfo structure which
|||	                                 describes the buffer and file
|||	                                 characteristics.
|||
|||	  Return Value
|||
|||	    Returns zero on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, blockfile.h
|||
|||	  See Also
|||
|||	    LoadFile(), LoadFileHere(), UnloadFile()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/blitfontchar
|||	BlitFontChar - Blit character pixels from font data area into memory
|||	               buffer.
|||
|||	  Synopsis
|||
|||	    int32 BlitFontChar(FontDescriptor *fDesc,
|||	                       uint32 theChar,
|||	                       void *blitInfo,
|||	                       void *dstBuf,
|||	                       int32 dstX,
|||	                       int32 dstY,
|||	                       int32 dstBPR,
|||	                       int32 dstColorIndex,
|||	                       int32 dstBPP)
|||
|||	  Description
|||
|||	    Blits character pixels from the font data area into a memory buffer,
|||	    performing any font formatspecific data decompression as needed.
|||
|||	    BlitFontChar() supports the internals of the text library; it should never
|||	    be necessary to call this function directly from an application program.
|||	    If you are implementing your own replacement text library, use
|||	    BlitFontChar() to render the font character pixels into your own memory
|||	    buffer. You can use the standard text library source code as a guide in
|||	    creating your own text library.
|||
|||	    BlitFontChar() currently supports only 8-bit-per-pixel destination format,
|||	    and always creates pixels suitable for an 8-bit coded cel with AMV
|||	    (alternate multiplier value) used to scale pixels already in the cell for
|||	    anti-aliasing.  Support for other destination formats may be added in the
|||	    future.
|||
|||	  Arguments
|||
|||	    fDesc                        Pointer to a FontDescriptor structure.
|||
|||	    theChar                      The specific character you want to blit to
|||	                                 the buffer.
|||
|||	    blitInfo                     The value returned from GetFontCharInfo()via
|||	                                 its **blitInfo  parameter, or NULL.  If you
|||	                                 pass NULL, an internal call to
|||	                                 GetFontCharInfo() is made.  If you are
|||	                                 already calling GetFontCharInfo() before
|||	                                 blitting each character, you can eliminate
|||	                                 this potentially expensive internal call by
|||	                                 saving the blitInfo from your call and
|||	                                 passing it to this function.
|||
|||	    destBuf                      Pointer to the cel buffer into which you
|||	                                 want to blit the character pixels.
|||
|||	    dstX                         x coordinate within the destBuf.
|||
|||	    dstY                         y coordinate within the destBuf.
|||
|||	    dstBPR                       The bytes-per-row of the destBuf.
|||
|||	    dstColorIndex                For 8-bit-per-pixel destination buffers, the
|||	                                 color index to OR into each pixel as the
|||	                                 character pixels are placed into the buffer.
|||	                                 The value must be in the range of 0 through
|||	                                 3 inclusive.
|||
|||	    dstBPP                       The bits-per-pixel of the destination
|||	                                 buffer.
|||
|||	  Return Value
|||
|||	    The width of the character, in pixels. Zero indicates no pixels were
|||	    placed in the destination buffer because the character is not within the
|||	    range of characters stored in the font.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, fontlib.h
|||
|||	  See Also
|||
|||	    GetFontCharInfo()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/cel_dataptr
|||	CEL_DATAPTR - Get the pointer to a cel's data buffer.
|||
|||	  Synopsis
|||
|||	    CelData * CEL_DATAPTR(ccb)
|||
|||	  Description
|||
|||	    This macro evaluates to a pointer to the cel's data buffer.  It takes the
|||	    CCB_SPABS flag into account and always returns an absolute pointer, even
|||	    when the CCB contains a relative offset in the ccb_SourcePtr field.
|||
|||	    This macro evaluates its argument more than once; be careful of side
|||	    effects.  It is usable in an expression context.
|||
|||	  Arguments
|||
|||	    ccb                          A pointer to a cel.
|||
|||	  Return Value
|||
|||	    A pointer to the cel's data buffer, a CelData* type.
|||
|||	  Implementation
|||
|||	    Macro
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/cel_nextptr
|||	CEL_NEXTPTR - Get the pointer to the next cel in a list.
|||
|||	  Synopsis
|||
|||	    CCB * CEL_NEXTPTR(ccb)
|||
|||	  Description
|||
|||	    This macro evaluates to a pointer to the next cel.  It takes the CCB_NPABS
|||	    flag into account and always returns an absolute pointer, even when the CCB
|||	    contains a relative offset in the ccb_NextPtr field.  It evaluates to NULL
|||	    if the CCB_LAST flag is set, or if the ccb_NextPtr field is NULL or zero.
|||
|||	    This macro evaluates its argument more than once; be careful of side
|||	    effects.  It is usable in expression context.
|||
|||	  Arguments
|||
|||	    ccb                          A pointer to a cel.
|||
|||	  Return Value
|||
|||	    A pointer to the next cel, a CCB* type.  NULL if there is no next cel.
|||
|||	  Implementation
|||
|||	    Macro
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/cel_plutptr
|||	CEL_PLUTPTR - Get the pointer to a cel's PLUT.
|||
|||	  Synopsis
|||
|||	    uint16 * CEL_PLUTPTR(ccb)
|||
|||	  Description
|||
|||	    This macro evaluates to a pointer to the cel's PLUT.  It takes the
|||	    CCB_PPABS flag into account and always returns an absolute pointer, even
|||	    when the CCB contains a relative offset in the ccb_PLUTPtr field.
|||
|||	    This macro evaluates its argument more than once; be careful of side
|||	    effects.  It is usable in expression context.
|||
|||	  Arguments
|||
|||	    ccb                          A pointer to a cel.
|||
|||	  Return Value
|||
|||	    A pointer the cel's PLUT, a uint16* type
|||
|||	  Implementation
|||
|||	    Macro
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/cel_pre0word
|||	CEL_PRE0WORD - Get the Preamble Word 0 value from a cel.
|||
|||	  Synopsis
|||
|||	    uint32 CEL_PRE0WORD(ccb)
|||
|||	  Description
|||
|||	    This macro evaluates to the Preamble Word 0 value for the cel.  It takes
|||	    the CCB_CCBPRE flag into account and retrieves the preamble word from the
|||	    CCB or the cel data buffer as appropriate.
|||
|||	    This macro evaluates its argument more than once; be careful of side
|||	    effects.  It is usable in expression context.
|||
|||	  Arguments
|||
|||	    ccb                          A pointer to a cel.
|||
|||	  Return Value
|||
|||	    The cel's preamble word 0 value, a uint32 type
|||
|||	  Implementation
|||
|||	    Macro
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/cel_pre1word
|||	CEL_PRE1WORD - Get the Preamble Word 1 value from a cel.
|||
|||	  Synopsis
|||
|||	    uint32 CEL_PRE1WORD(ccb)
|||
|||	  Description
|||
|||	    This macro evaluates to the Preamble Word 1 value for the cel.  It takes
|||	    the CCB_CCBPRE flag into account and retrieves the preamble word from the
|||	    CCB or the cel data buffer as appropriate.
|||
|||	    This macro evaluates its argument more than once; be careful of side
|||	    effects.  It is usable in expression context.
|||
|||	  Arguments
|||
|||	    ccb                          A pointer to a cel.
|||
|||	  Return Value
|||
|||	    The cel's preamble word 1 value, a uint32 type.  The ccb_PRE1 or
|||	    second celdata word are mindlessly returned regardless of whether the cel
|||	    actually requires a PRE1 control word.
|||
|||	  Implementation
|||
|||	    Macro
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centercrectincrect
|||	CenterCRectInCRect - Center a CRect over or within another CRect.
|||
|||	  Synopsis
|||
|||	    CRect * CenterCRectInCRect(CRect *dst, CRect *src)
|||
|||	  Description
|||
|||	    This function centers the dst rectangle over or within the src rectangle.
|||	    If the dst rectangle is smaller than the src rectangle, its values are
|||	    modified to center it within the src rectangle.  If the dst rectangle is
|||	    larger than the src rectangle, its values are modified to center it over
|||	    the src rectangle.  In either case, the resulting rectangle retains its
|||	    original sizes; only the position changes.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the CRect to be centered.
|||
|||	    src                          A pointer to the CRect within which dst is
|||	                                 to be centered.
|||
|||	  Return Value
|||
|||	    Returns dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterSRectInSRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centercrectindisplay
|||	CenterCRectInDisplay - Center a CRect within the display.
|||
|||	  Synopsis
|||
|||	    CRect * CenterCRectInDisplay(CRect *dst)
|||
|||	  Description
|||
|||	    This function centers the dst rectangle over or within the display.
|||	    This routine assumes your display is 320 * 240.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to a CRect where the results are
|||	                                 to be stored.
|||
|||	  Return Value
|||
|||	    Returns dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterSRectInDisplay()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centercrectoveripoint
|||	CenterCRectOverIPoint - Center a CRect over a point.
|||
|||	  Synopsis
|||
|||	    CRect * CenterCRectOverIPoint(CRect *dst, IPoint *point)
|||
|||	  Description
|||
|||	    This function centers the dst rectangle over the specified point.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to a CRect where the results are
|||	                                 to be stored.
|||
|||	    point                        A pointer to an IPoint over which the dst is
|||	                                 to be centered.
|||
|||	  Return Value
|||
|||	    Returns dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterSRectOverIPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerfpointindisplay
|||	CenterFPointInDisplay - Return the center point of the display.
|||
|||	  Synopsis
|||
|||	    FPoint* CenterFPointInDisplay(void)
|||
|||	  Description
|||
|||	    This function returns the center point of the display.  Assumes your
|||	    display is 320 * 240.
|||
|||	    The return value points to a static structure which must not be modified.
|||
|||	  Return Value
|||
|||	    Returns a pointer to an FPoint describing the center point of the display.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterIPointInDisplay()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centeripointincrect
|||	CenterIPointInCRect - Calculate the center point of a CRect.
|||
|||	  Synopsis
|||
|||	    IPoint* CenterIPointInCRect(IPoint *dst, CRect *rect)
|||
|||	  Description
|||
|||	    This function calculates the center point of the specified rectangle and
|||	    stores the result at *dst.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to an IPoint where the results are
|||	                                 to be stored.
|||
|||	    rect                         A pointer to the CRect for which the center
|||	                                 point is to be calculated.
|||
|||	  Return Value
|||
|||	    Returns dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterIPointInSRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centeripointinsrect
|||	CenterIPointInSRect - Calculate the center point of an SRect.
|||
|||	  Synopsis
|||
|||	    IPoint* CenterIPointInSRect(IPoint *dst, SRect *rect)
|||
|||	  Description
|||
|||	    This function calculates the center point of the specified rectangle and
|||	    stores the result at *dst.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to an IPoint where the results are
|||	                                 to be stored.
|||
|||	    rect                         A pointer to the SRect for which the center
|||	                                 point is to be calculated.
|||
|||	  Return Value
|||
|||	    Returns dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterIPointInCRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centeripointindisplay
|||	CenterIPointInDisplay - Return the center point of the display.
|||
|||	  Synopsis
|||
|||	    IPoint* CenterIPointInDisplay(void)
|||
|||	  Description
|||
|||	    This function returns the center point of the display.
|||
|||	    The return value points to a static structure which must not be modified.
|||
|||	  Return Value
|||
|||	    Returns a pointer to an FPoint describing the center point of the display.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterFPointInDisplay()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerrectaacelincrect
|||	CenterRectAACelInCRect - Center an anti-aliased cel in a CRect.
|||
|||	  Synopsis
|||
|||	    void CenterRectAACelInCRect(CCB *cel, CRect *rect)
|||
|||	  Description
|||
|||	    This function centers an anti-aliased cel over or within the specified
|||	    rectangle.
|||
|||	    The ccb_XPos and ccb_YPos fields are modified in both the data and alpha-
|||	    channel CCBs; other CCB fields are not modified.  The centering is based
|||	    on the cel's projected size, not its source data size.  If the cel is
|||	    projected to a non-rectangular shape, the result will be invalid.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel to be centered.
|||
|||	    rect                         A pointer to a CRect describing the area
|||	                                 within which the cel is to be centered.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterRectAACelInSRect(), CenterRectCelInCRect(),
|||	    CenterRectCelListInCRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerrectaacelinsrect
|||	CenterRectAACelInSRect - Center an anti-aliased cel in an SRect.
|||
|||	  Synopsis
|||
|||	    void CenterRectAACelInSRect(CCB *cel, SRect *rect)
|||
|||	  Description
|||
|||	    This function centers an anti-aliased cel over or within the specified
|||	    rectangle.
|||
|||	    The ccb_XPos and ccb_YPos fields are modified in both the data and alpha-
|||	    channel CCBs; other CCB fields are not modified.  The centering is based
|||	    on the cel's projected size, not its source data size.  If the cel is
|||	    projected to a non-rectangular shape, the result will be invalid.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel to be centered.
|||
|||	    rect                         A pointer to an SRect describing the area
|||	                                 within which the cel is to be centered.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterRectAACelInCRect(), CenterRectCelInSRect(),
|||	    CenterRectCelListInSRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerrectaacelindisplay
|||	CenterRectAACelInDisplay - Center an anti-aliased cel in the display.
|||
|||	  Synopsis
|||
|||	    void CenterRectAACelInDisplay(CCB *cel)
|||
|||	  Description
|||
|||	    This function centers an anti-aliased cel over or within the the
|||	    display.  The display is assumed to be 320 * 240.
|||
|||	    The ccb_XPos and ccb_YPos fields are modified in both the data and alpha-
|||	    channel CCBs; other CCB fields are not modified.  The centering is based
|||	    on the cel's projected size, not its source data size.  If the cel is
|||	    projected to a non-rectangular shape, the result will be invalid.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel to be centered.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterRectCelInDisplay(), CenterRectCelListInDisplay()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerrectaaceloverfpoint
|||	CenterRectAACelOverFPoint - Center an anti-aliased cel over a point.
|||
|||	  Synopsis
|||
|||	    void CenterRectAACelOverFPoint(CCB *cel, FPoint *point)
|||
|||	  Description
|||
|||	    This function centers an anti-aliased cel over the specified point.
|||
|||	    The ccb_XPos and ccb_YPos fields are modified in both the data and alpha-
|||	    channel CCBs; other CCB fields are not modified.  The centering is based
|||	    on the cel's projected size, not its source data size.  If the cel is
|||	    projected to a non-rectangular shape, the result will be invalid.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel to be centered.
|||
|||	    point                        A pointer to an FPoint over which the cel is
|||	                                 to be centered.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterRectAACelOverIPoint(), CenterRectCelOverFPoint(),
|||	    CenterCelListOverFPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerrectaaceloveripoint
|||	CenterRectAACelOverIPoint - Center an anti-aliased cel over a point.
|||
|||	  Synopsis
|||
|||	    void CenterRectAACelOverIPoint(CCB *cel, IPoint *point)
|||
|||	  Description
|||
|||	    This function centers an anti-aliased cel over the specified point.
|||
|||	    The ccb_XPos and ccb_YPos fields are modified in both the data and alpha-
|||	    channel CCBs; other CCB fields are not modified.  The centering is based
|||	    on the cel's projected size, not its source data size.  If the cel is
|||	    projected to a non-rectangular shape, the result will be invalid.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel to be centered.
|||
|||	    point                        A pointer to an IPoint over which the cel is
|||	                                 to be centered.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterRectAACelOverFPoint(), CenterRectCelOverIPoint(),
|||	    CenterCelListOverIPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerrectcelincrect
|||	CenterRectCelInCRect - Center a cel within a CRect.
|||
|||	  Synopsis
|||
|||	    void CenterRectCelInCRect(CCB *cel, CRect *rect)
|||
|||	  Description
|||
|||	    This function centers a single cel over or within the specified rectangle.
|||
|||	    The ccb_XPos and ccb_YPos fields are modified in the CCB; other CCB fields
|||	    are not modified.  The centering is based on the cel's projected size, not
|||	    its source data size.  If the cel is projected to a non-rectangular shape,
|||	    the result will be invalid.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel to be centered.
|||
|||	    rect                         A pointer to a CRect within which the cel is
|||	                                 to be centered.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterRectCelInSRect(), CenterRectAACelInCRect(),
|||	    CenterRectCelListInCRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/chaincelsathead
|||	ChainCelsAtHead - Chain together lists of cels.
|||
|||	  Synopsis
|||
|||	    CCB * ChainCelsAtHead(CCB *existingCels, CCB *newCels)
|||
|||	  Description
|||
|||	    This function chains together two lists of cels, puting the new list ahead
|||	    of the existing cels.  The return value is a pointer to the first cel in
|||	    the resulting list.  Either pointer can be NULL, eliminating the need for
|||	    special-case code when iteratively building a list of cels.  For example,
|||	    the following code will work:
|||
|||	    CCB *list = NULL;
|||	    CCB *cels;
|||	    do {
|||	        cels = get_next_antialiased_cel();
|||	        list = ChainCelsAtHead(list, cels);
|||	    } while (cels != NULL);
|||
|||	    This function works properly with anti-aliased cels and similar constructs
|||	    where a list of related cels makes up a single logical entity.
|||
|||	  Arguments
|||
|||	    existingCels                 A pointer to the existing list of cels; may
|||	                                 be NULL.
|||
|||	    newCels                      A pointer to the new list of cels to be
|||	                                 added ahead of the existing list; may be
|||	                                 NULL.
|||
|||	  Return Value
|||
|||	    A pointer to the head of the resulting list of cels.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    ChainCelsAtTail()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/chaincelsattail
|||	ChainCelsAtTail - Chain together lists of cels.
|||
|||	  Synopsis
|||
|||	    CCB * ChainCelsAtTail(CCB *existingCels, CCB *newCels)
|||
|||	  Description
|||
|||	    This function chains together two lists of cels, puting the new list
|||	    behind the existing cels.  The return value is a pointer to the last cel
|||	    in the resulting list.  Either pointer can be NULL, in which case the
|||	    function is essentially a no-op that just returns a pointer to the last
|||	    cel in the list pointed to by the other pointer.
|||
|||	    The most effecient way to iteratively build a list of cels is to use the
|||	    return value from the prior call as the existingCels pointer for the
|||	    current call.  This eliminates long searches for the end of the
|||	    ever-growing list of cels.  For example:
|||
|||	    CCB *list = NULL;
|||	    CCB *tail = NULL;
|||	    CCB *cels;
|||	    do {
|||	        cels = get_next_antialiased_cel();
|||	        tail = ChainCelsAtTail(tail, cels);
|||	        if (list == NULL) {
|||	            list = cels;  // remember head-of-list pointer first time through.
|||	        }
|||	    } while (cels != NULL);
|||
|||	    This function works properly with anti-aliased cels and similar constructs
|||	    where a list of related cels makes up a single logical entity.
|||
|||	  Arguments
|||
|||	    existingCels                 A pointer to the existing list of cels; may
|||	                                 be NULL.
|||
|||	    newCels                      A pointer to the new list of cels to be
|||	                                 added at the end of the existing list; may
|||	                                 be NULL.
|||
|||	  Return Value
|||
|||	    A pointer to the last cel in the resulting list of cels.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    ChainCelsAtHead()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/clonecel
|||	CloneCel - Create a copy of a cel.
|||
|||	  Synopsis
|||
|||	    CCB * CloneCel(CCB *src, int32 options)
|||
|||	  Description
|||
|||	    This function makes a copy of a CCB, and optionally of the pixels and/or
|||	    PLUT.  The values in the cloned CCB are identical to the src CCB, except
|||	    that the CCB_LAST flag will be set and the ccb_NextPtr field will be NULL
|||	    in the clone.
|||
|||	    The options parameter is one or more of the following values, ORed
|||	    together as necessary: CLONECEL_CCB_ONLY, CLONECEL_COPY_PIXELS,
|||	    CLONECEL_COPY_PLUT.
|||
|||	    When COPY_PIXELS is specified, a new cel data buffer is allocated, and the
|||	    pixels in the src cel's buffer are copied to the new buffer.  When
|||	    COPY_PIXELS is not specified, the new cel's ccb_SourcePtr field points to
|||	    the src cel's data buffer.
|||
|||	    When COPY_PLUT is specified, a new PLUT is allocated, and the values from
|||	    the src cel's PLUT are copied to the new PLUT.  When COPY_PLUT is not
|||	    specified, the new cel's ccb_PLUTPtr field points to the src cel's PLUT.
|||	    If the cel is uncoded, the COPY_PLUT option has no effect.
|||
|||	    You may change any of the values in the CCB fields of the newly created
|||	    cel.  Be careful about changing preamble words when the CCB_CCBPRE flag is
|||	    not set and the CLONECEL_COPY_PIXELS option was not used.  In that
|||	    situation, the preamble words you change would be those belonging to the
|||	    original cel, not the copy.
|||
|||	    Use DeleteCel() to release all resources allocated for the clone.
|||
|||	  Arguments
|||
|||	    src                          A pointer to a cel to copy.
|||
|||	    options                      Bitmapped options, as described above.
|||
|||	  Return Value
|||
|||	    A pointer to the new CCB, or NULL if there is not enough memory to make
|||	    the clone.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CreateCel(), DeleteCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/clonetextcel
|||	CloneTextCel - Clone an existing text cel.
|||
|||	  Synopsis
|||
|||	    TextCel * CloneTextCel(TextCel *templateTextCel,
|||	                           Boolean clonePixels)
|||
|||	  Description
|||
|||	    Creates a new TextCel using a TextCel that already exists as a template.
|||	    If clonePixels is FALSE, the new TextCel contains no text; if TRUE, the
|||	    pixels from the template TextCel are copied into the new TextCel.
|||
|||	    If the template cel has a format buffer attached to it, and that buffer
|||	    was dynamically allocated by the call to SetTextCelFormatBuffer(), a new
|||	    buffer of the same size is allocated and attached to the new cel. If the
|||	    template cel has a format buffer that you provided explicitly (a static
|||	    buffer), the new cel will use the same static buffer.
|||
|||	    This function is useful when you want to create a number of identical
|||	    TextCels.  You can call CreateTextCel(), followed by whatever
|||	    SetTextCel...() functions you need to create a template cel with a given
|||	    size and attributes, then call CloneTextCel() as often as necessary to
|||	    create a number of cels with identical size and attributes.
|||
|||	    Use DeleteTextCel() to dispose of the TextCel when you are finished with
|||	    it.
|||
|||	  Arguments
|||
|||	    templateTextCel              Pointer to the TextCel structure you want to
|||	                                 clone.
|||
|||	    clonePixels                  TRUE if the new TextCel is to contain the
|||	                                 same pixels as those currently in the
|||	                                 template TextCel.
|||
|||	  Return Value
|||
|||	    A pointer to the newly created TextCel structure; NULL if there isn't
|||	    enough memory to create the new TextCel.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
|||	  See Also
|||
|||	    CreateTextCel(), DeleteTextCel
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerrectcelinsrect
|||	CenterRectCelInSRect - Center a cel within an SRect
|||
|||	  Synopsis
|||
|||	    void CenterRectCelInSRect(CCB *cel, SRect *rect)
|||
|||	  Description
|||
|||	    This function centers a single cel over or within the specified rectangle.
|||
|||	    The ccb_XPos and ccb_YPos fields are modified in the CCB; other CCB fields
|||	    are not modified.  The centering is based on the cel's projected size, not
|||	    its source data size.  If the cel is projected to a non-rectangular shape,
|||	    the result will be invalid.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel to be centered.
|||
|||	    rect                         A pointer to an SRect within which the cel
|||	                                 is to be centered.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterRectCelInCRect(), CenterRectAACelInSRect(),
|||	    CenterRectCelListInSRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerrectcelindisplay
|||	CenterRectCelInDisplay - Center a cel in the display.
|||
|||	  Synopsis
|||
|||	    void CenterRectCelInDisplay(CCB *cel)
|||
|||	  Description
|||
|||	    This function centers a single cel over or within the the display.
|||	    The display is assumed to be 320 * 240.
|||
|||	    The ccb_XPos and ccb_YPos fields are modified in the CCB; other CCB fields
|||	    are not modified.  The centering is based on the cel's projected size, not
|||	    its source data size.  If the cel is projected to a non-rectangular shape,
|||	    the result will be invalid.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel to be centered.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterRectAACelInDisplay(), CenterRectCelListInDisplay()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerrectcellistincrect
|||	CenterRectCelListInCRect - Center a list of cels within a CRect.
|||
|||	  Synopsis
|||
|||	    void CenterRectCelListInCRect(CCB *cel, CRect *rect)
|||
|||	  Description
|||
|||	    This function centers a list of one or more cels over or within the
|||	    specified rectangle.  The list of cels is treated as if it were a single
|||	    logical entity.  The first cel in the list is centered, then the X/Y
|||	    coordinates of all remaining cels in the list are modified so that all
|||	    cels maintain the same positional relationship to each other as they had
|||	    originally.  This is especially useful when the first cel in the list
|||	    encompases the area occupied by all the other cels; the effect is to
|||	    center the group of cels as if they were all one cel.
|||
|||	    The ccb_XPos and ccb_YPos fields are modified in the CCBs; other CCB
|||	    fields are not modified.  The centering is based on the first cel's
|||	    projected size, not its source data size.  If the first cel is projected
|||	    to a non-rectangular shape, the result will be invalid.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to a list of one or more cels to
|||	                                 be centered.
|||
|||	    rect                         A pointer to an SRect within which the cels
|||	                                 are to be centered.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterRectCelListInSRect(), CenterRectAACelInCRect(),
|||	    CenterRectCelInCRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerrectcellistinsrect
|||	CenterRectCelListInSRect - Center a list of cels within an SRect.
|||
|||	  Synopsis
|||
|||	    void CenterRectCelListInSRect(CCB *cel, SRect *rect)
|||
|||	  Description
|||
|||	    This function centers a list of one or more cels over or within the
|||	    specified rectangle.  The list of cels is treated as if it were a single
|||	    logical entity.  The first cel in the list is centered, then the X/Y
|||	    coordinates of all remaining cels in the list are modified so that all
|||	    cels maintain the same positional relationship to each other as they had
|||	    originally.  This is especially useful when the first cel in the list
|||	    encompases the area occupied by all the other cels; the effect is to
|||	    center the group of cels as if they were all one cel.
|||
|||	    The ccb_XPos and ccb_YPos fields are modified in the CCBs; other CCB
|||	    fields are not modified.  The centering is based on the first cel's
|||	    projected size, not its source data size.  If the first cel is projected
|||	    to a non-rectangular shape, the result will be invalid.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to a list of one or more cels to
|||	                                 be centered.
|||
|||	    rect                         A pointer to an SRect within which the cels
|||	                                 are to be centered.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterRectCelListInCRect(), CenterRectAACelInSRect(),
|||	    CenterRectCelInSRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerrectcellistindisplay
|||	CenterRectCelListInDisplay - Center a list of cels in the display.
|||
|||	  Synopsis
|||
|||	    void CenterRectCelListInDisplay(CCB *cel)
|||
|||	  Description
|||
|||	    This function centers a list of one or more cels over or within the
|||	    display.  The list of cels is treated as if it were a single logical
|||	    entity.  The first cel in the list is centered, then the X/Y coordinates
|||	    of all remaining cels in the list are modified so that all cels maintain
|||	    the same positional relationship to each other as they had originally.
|||	    This is especially useful when the first cel in the list encompases the
|||	    area occupied by all the other cels; the effect is to center the group of
|||	    cels as if they were all one cel.
|||
|||	    The display is assumed to be 320 * 240.
|||
|||	    The ccb_XPos and ccb_YPos fields are modified in the CCBs; other CCB
|||	    fields are not modified.  The centering is based on the first cel's
|||	    projected size, not its source data size.  If the first cel is projected
|||	    to a non-rectangular shape, the result will be invalid.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to a list of one or more cels to
|||	                                 be centered.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterRectAACelInDisplay(), CenterRectCelInDisplay()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerrectcellistoverfpoint
|||	CenterRectCelListOverFPoint - Center a list of cels over a point.
|||
|||	  Synopsis
|||
|||	    void CenterRectCelListOverFPoint(CCB *cel, FPoint *point)
|||
|||	  Description
|||
|||	    This function centers a list of one or more cels over the specified point.
|||	    The list of cels is treated as if it were a single logical entity.  The
|||	    first cel in the list is centered, then the X/Y coordinates of all
|||	    remaining cels in the list are modified so that all cels maintain the same
|||	    positional relationship to each other as they had originally.  This is
|||	    especially useful when the first cel in the list encompases the area
|||	    occupied by all the other cels; the effect is to center the group of cels
|||	    as if they were all one cel.
|||
|||	    The ccb_XPos and ccb_YPos fields are modified in the CCBs; other CCB
|||	    fields are not modified.  The centering is based on the first cel's
|||	    projected size, not its source data size.  If the first cel is projected
|||	    to a non-rectangular shape, the result will be invalid.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to a list of one or more cels to
|||	                                 be centered.
|||
|||	    point                        A pointer to an FPoint over which the cels
|||	                                 are to be centered.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterRectCelListOverIPoint(), CenterRectAACelOverFPoint(),
|||	    CenterRectCelOverFPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerrectcellistoveripoint
|||	CenterRectCelListOverIPoint - Center a list of cels over a point.
|||
|||	  Synopsis
|||
|||	    void CenterRectCelListOverIPoint(CCB *cel, IPoint *point)
|||
|||	  Description
|||
|||	    This function centers a list of one or more cels over the specified point.
|||	    The list of cels is treated as if it were a single logical entity.  The
|||	    first cel in the list is centered, then the X/Y coordinates of all
|||	    remaining cels in the list are modified so that all cels maintain the same
|||	    positional relationship to each other as they had originally.  This is
|||	    especially useful when the first cel in the list encompases the area
|||	    occupied by all the other cels; the effect is to center the group of cels
|||	    as if they were all one cel.
|||
|||	    The ccb_XPos and ccb_YPos fields are modified in the CCBs; other CCB
|||	    fields are not modified.  The centering is based on the first cel's
|||	    projected size, not its source data size.  If the first cel is projected
|||	    to a non-rectangular shape, the result will be invalid.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to a list of one or more cels to
|||	                                 be centered.
|||
|||	    point                        A pointer to an IPoint over which the cels
|||	                                 are to be centered.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterRectCelListOverFPoint(), CenterRectAACelOverIPoint(),
|||	    CenterRectCelOverIPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerrectceloverfpoint
|||	CenterRectCelOverFPoint - Center a cel over a point.
|||
|||	  Synopsis
|||
|||	    void CenterRectCelOverFPoint(CCB *cel, FPoint *point)
|||
|||	  Description
|||
|||	    This function centers a single cel over the specified point.
|||
|||	    The ccb_XPos and ccb_YPos fields are modified in the CCB; other CCB fields
|||	    are not modified.  The centering is based on the cel's projected size, not
|||	    its source data size.  If the cel is projected to a non-rectangular shape,
|||	    the result will be invalid.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to a cel to be centered.
|||
|||	    point                        A pointer to an FPoint over which the cels
|||	                                 are to be centered.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterRecCelOverIPoint(), CenterRectAACelOverFPoint(),
|||	    CenterRectCelListOverFPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centerrectceloveripoint
|||	CenterRectCelOverIPoint - Center a cel over a point.
|||
|||	  Synopsis
|||
|||	    void CenterRectCelOverIPoint(CCB *cel, IPoint *point)
|||
|||	  Description
|||
|||	    This function centers a single cel over the specified point.
|||
|||	    The ccb_XPos and ccb_YPos fields are modified in the CCB; other CCB fields
|||	    are not modified.  The centering is based on the cel's projected size, not
|||	    its source data size.  If the cel is projected to a non-rectangular shape,
|||	    the result will be invalid.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to a cel to be centered.
|||
|||	    point                        A pointer to an IPoint over which the cels
|||	                                 are to be centered.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterRecCelOverFPoint(), CenterRectAACelOverIPoint(),
|||	    CenterRectCelListOverIPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centersrectinsrect
|||	CenterSRectInSRect - Center an SRect within an SRect.
|||
|||	  Synopsis
|||
|||	    SRect * CenterSRectInSRect(SRect *dst, SRect *rect)
|||
|||	  Description
|||
|||	    This function centers the dst rectangle over or within the src rectangle.
|||	    If the dst rectangle is smaller than the src rectangle, its position
|||	    values are modified to center it within the src rectangle.  If the dst
|||	    rectangle is larger than the src rectangle, its position values are
|||	    modified to center it over the src rectangle.  In either case, the
|||	    resulting rectangle retains its original sizes; only the position changes.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the SRect to be centered.
|||
|||	    src                          A pointer to the SRect within which dst is
|||	                                 to be centered.
|||
|||	  Return Value
|||
|||	    Returns dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterCRectInCRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centersrectindisplay
|||	CenterSRectInDisplay - Center an SRect in the display.
|||
|||	  Synopsis
|||
|||	    SRect * CenterSRectInDisplay(SRect *dst)
|||
|||	  Description
|||
|||	    This function centers the dst rectangle over or within the display
|||	    (assumed 320 * 240).
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the SRect to be centered.
|||
|||	  Return Value
|||
|||	    Returns dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterCRectInDisplay()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/centersrectoveripoint
|||	CenterSRectOverIPoint - Center an SRect over a point.
|||
|||	  Synopsis
|||
|||	    SRect * CenterSRectOverIPoint(SRect *dst, IPoint *point)
|||
|||	  Description
|||
|||	    This function centers the dst rectangle over the specified point.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the SRect to be centered.
|||
|||	    point                        A pointer to an IPoint over which dst is to
|||	                                 be centered.
|||
|||	  Return Value
|||
|||	    Returns dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CenterCRectOverIPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/checkasyncloadfile
|||	CheckAsyncLoadFile - Check for completion of an async file load operation.
|||
|||	  Synopsis
|||
|||	    Err CheckAsyncLoadFile(LoadFileInfo *lf)
|||
|||	  Description
|||
|||	    This function checks the status of an AsyncLoadFile() started earlier.  A
|||	    zero return value means I/O is still in progress.  A negative return value
|||	    indicates error, and a positive return value indicates successful
|||	    completion.
|||
|||	    You must call FinishAsyncLoadFile() after receiving a non-zero status from
|||	    this function.
|||
|||	  Arguments
|||
|||	    lf                           A pointer to the LoadFileInfo structure for
|||	                                 the load operation.
|||
|||	  Return Value
|||
|||	    Returns zero if I/O is still in progress, negative if the I/O failed, or
|||	    positive if the has I/O compeleted successfully.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, blockfile.h
|||
|||	  See Also
|||
|||	    AsyncLoadFile()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/cleanupmsevents
|||	CleanupMSEvents - Release resources acquired by SetupMSEvents.
|||
|||	  Synopsis
|||
|||	    void CleanupMSEvents(MSEventHandle mseHandle)
|||
|||	  Description
|||
|||	    This function releases all resources acquired internally by
|||	    SetupMSEvents().  Any ports or signals which were allocated by
|||	    SeuptMSEvents() are deleted.  If mseHandle is less than or equal to zero 0
|||	    this function quietly returns without doing anything.
|||
|||	  Arguments
|||
|||	    mseHandle                    An MSEventHandle returned by
|||	                                 SetupMSEvents().
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, msgutils.h
|||
|||	  See Also
|||
|||	    SetupMSEvents(), DispatchMSEvents()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/clearbitmap
|||	ClearBitmap - Clear a screen or bitmap.
|||
|||	  Synopsis
|||
|||	    Err ClearBitmap(Item ioreq, Item scr_bm,
|||	                    Bitmap *bmPtr, int32 rgbValue)
|||
|||	  Description
|||
|||	    This function sets all pixels in the screen or bitmap to the specified
|||	    rgbValue.  It uses SetVRAMPages() for fast operation.
|||
|||	    The rgbValue is a pair of RGB555s packed into a 32-bit word.  If you don't
|||	    want simple black (zero), use the MakeRGB15Pair() macro to create this
|||	    value from separate red, green, and blue values.
|||
|||	    For best performance, pass a non-zero ioreq parm and a bitmap pointer
|||	    instead of a screen or bitmap item.  This will avoid LookupItem() and
|||	    CreateIOReq() calls that would otherwise be made internally.
|||
|||	  Arguments
|||
|||	    ioreq                        An IOReq item obtained from GetVRAMIOReq(),
|||	                                 or zero to have an IOReq created and deleted
|||	                                 internally during the call.
|||
|||	    scr_bm                       A Screen or Bitmap item number.  May be zero
|||	                                 if a Bitmap pointer is provided.
|||
|||	    bmPtr                        A Bitmap pointer.  May be NULL if a Screen
|||	                                 or Bitmap item is provided.
|||
|||	    rgbValue                     The color value to store into all screen or
|||	                                 bitmap pixels.
|||
|||	  Return Value
|||
|||	    Returns zero on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, displayutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/createbackdropcel
|||	CreateBackdropCel - Create a single-color cel.
|||
|||	  Synopsis
|||
|||	    CCB * CreateBackdropCel(int32 width, int32 height,
|||	                            int32 color, int32 opacityPercent)
|||
|||	  Description
|||
|||	    This function creates a single-colored cel which is useful in serving as a
|||	    backdrop and/or control point for other objects.
|||
|||	    The color is an RGB555-format value.  Use the MakeRGB15() macro to create
|||	    this value from separate red, green, and blue values.
|||
|||	    The cel can be opaque or translucent as specified by opacityPercent.  100
|||	    percent is opaque, smaller values give translucent cels where the cel's
|||	    color intensity is the specified percentage of the projected result.  For
|||	    example, 75% means the projected pixels are 75% of the cel's color, 25% of
|||	    the existing pixel's color.  There are actually only 8 levels of
|||	    translucency available; the percentage you request is mapped to the
|||	    closest available level.  The opacity and color of the cel cannot be
|||	    changed after the cel is created.  (More specifically, you're on your own
|||	    in attempting the PIXC and source pixel manipulations needed to change
|||	    color and opacity; it won't harm anything to try.)
|||
|||	    An opacityPercent of zero results in a 'virtual' cel, which does not
|||	    project any pixels when drawn.  The CCB_SKIP flag is set when
|||	    opacityPercent is zero.  A cel of this sort is useful in anchoring a list
|||	    of related cels.  It can serve as a single point of control for moving,
|||	    centering, mapping, and other projection-related manipulations when used
|||	    with the Library call implemented in lib3do.lib.s that manipulate lists of cels as if they were a
|||	    single entity.
|||
|||	    Regardless of the sizes you specify, the cel requires only sizeof(CCB)
|||	    bytes of memory.  The cel is created with a source size of 1x1 pixel, and
|||	    the HDX and VDY fields are set to project the pixel to the specified width
|||	    and height.  Note that the ccb_Width and ccb_Height fields reflect the
|||	    cel's source data size of 1x1 pixel.  Library call implemented in lib3do.lib.s that are sensitive
|||	    to cel size, such as centering functions, use SRectFromCel() to get the
|||	    projected cel size and thus work correctly.  You should do the same if you
|||	    need to obtain the projected cel size programmatically.
|||
|||	    You may remap the cel to project to any position, size, and shape you
|||	    desire after it is created.
|||
|||	    Use DeleteCel() to release all resources allocated for the cel.
|||
|||	  Arguments
|||
|||	    width                        The width of the cel as it is to appear when
|||	                                 projected.
|||
|||	    height                       The height of the cel as it is to appear
|||	                                 when projected.
|||
|||	    color                        The color of the cel, in RGB555 format.
|||
|||	    opacityPercent               The opacity percentage, from 0 (invisible)
|||	                                 to 100 (opaque).
|||
|||	  Return Value
|||
|||	    A pointer to the new CCB, or NULL if there is not enough memory to make
|||	    the cel.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CreateCel(), DeleteCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/createbasicdisplay
|||	CreateBasicDisplay - Do everything needed to create a simple
|||	                     display.
|||
|||	  Synopsis
|||
|||	    Item CreateBasicDisplay (ScreenContext *sc, uint32 displayType,
|||	                             uint32 numScreens)
|||
|||	  Description
|||
|||	    This function does all the work needed to prepare a screen display
|||	    on 3DO. It open the graphics folio, creates a screen group,
|||	    and adds the screen group. All you need to view the screen display
|||	    on the monitor is to call DisplayScreen() when you are ready.
|||
|||	    Using this function, you can create a display suitable for NTSC
|||	    or PAL. This is controlled using the displayType argument. The
|||	    different display types are defined in graphics.h and currently
|||	    include:
|||
|||	      DI_TYPE_DEFAULT
|||	      Specifying this value will give you a display which matches the
|||	      current defaults for the system. If the system is PAL, it will
|||	      open a PAL display, and if the machine is NTSC, it will open an
|||	      NTSC display. Be warned that it is very possible that in the
|||	      future, there will be additional display types that can be the
|||	      default. If your code can only cope with either NTSC or PAL,
|||	      you should specify the modes explicitly.
|||
|||	      DI_TYPE_NTSC
|||	      Opens a 320x240 display running at 60Hz.
|||
|||	      DI_TYPE_PAL1
|||	      Opens a 320x240 display running at 50Hz.
|||
|||	      DI_TYPE_PAL2
|||	      Opens a 384x288 display running at 50Hz.
|||
|||	    If your application includes both NTSC and PAL artwork, you
|||	    can use the following approach to determine which set of artwork
|||	    to use:
|||
|||	      QueryGraphics(QUERYGRAF_TAG_DEFAULTDISPLAYTYPE,&displayType);
|||           if ((displayType == DI_TYPE_PAL1) || (displayType == DI_TYPE_PAL2))
|||	      {
|||	          displayType = DI_TYPE_PAL2;
|||	      }
|||	      else
|||	      {
|||	          displayType = DI_TYPE_NTSC;
|||	      }
|||
|||	    You can then supply the displayType parameter to
|||	    CreateBasicDisplay(). What the above code does is protect
|||	    you against any new display types that can be added to the
|||	    system. Essentially, if the system says the default is PAL,
|||	    you will use the PAL artwork. Any other condition will yield
|||	    an NTSC display.
|||
|||	  Arguments
|||
|||	    sc                           This structure is used for output
|||	                                 only. It contains various fields
|||	                                 that describe the display. This
|||	                                 includes the sc_DisplayType field
|||	                                 which indicates the actual display
|||	                                 type of the display. If this pointer
|||	                                 is NULL, this function returns an
|||	                                 error code.
|||
|||	    displayType                  The type of display to open. See the
|||	                                 explanation above.
|||
|||	    numScreens                   Number of screen buffers to allocate.
|||	                                 This is typically 1 or 2. You allocate
|||	                                 two screen buffers when you wish
|||	                                 to do double-buffering.
|||
|||	  Return Value
|||
|||	    Returns the item number of the screen group that was created, or
|||	    a negative error code upon failure.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, displayutils.h
|||
|||	  See Also
|||
|||	    DeleteBasicDisplay()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/createcel
|||	CreateCel - Create a cel from scratch.
|||
|||	  Synopsis
|||
|||	    CCB * CreateCel(int32 width, int32 height, int32 bitsPerPixel,
|||	                    int32 options, void *dataBuf)
|||
|||	  Description
|||
|||	    This function creates a cel of any arbitrary type.  The parameters provide
|||	    just enough information to create the basic cel.  After the cel is
|||	    created, you are free to modify the CCB in any way you'd like to achieve
|||	    effects not directly implied by the creation parameters.
|||
|||	    If you pass a NULL dataBuf pointer an appropriately-sized buffer is
|||	    allocated when the cel is created.  If you pass a non-NULL dataBuf pointer
|||	    it is stored into the ccb_SourcePtr field instead of allocating a buffer
|||	    internally.
|||
|||	    The options parameter is one or more of the following values, ORed
|||	    together as necessary:
|||	    CREATECEL_UNCODED-
|||	        Create a coded cel.  bitsPerPixel must be <= 8.
|||	        CREATECEL_CODED-
|||	                Create an uncoded cel.  bitsPerPixel must be 8 or 16.
|||
|||	    If the bitsPerPixel value is less than 8, a PLUT is always allocated when
|||	    the cel is created.  When bitsPerPixel is 8 or 16, a PLUT is allocated
|||	    only if the CREATECEL_CODED option is set.  When a PLUT is allocated all
|||	    its entries are initialized to zero.
|||
|||	    The cel is created with a position of 0,0 and size and perspective values
|||	    for a 1:1 mapping.  The Preamble Word 1 Blue LSB handling is set to
|||	    PRE1_TLLSB_PDC0.  The PIXC word is set for standard opaque, 0x1F001F00.
|||	    The cel flags are set to:CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_PPABS |
|||	    CCB_LDSIZE | CCB_LDPRS | CCB_LDPPMP | CCB_CCBPRE | CCB_YOXY | CCB_USEAV  |
|||	    CCB_NOBLK | CCB_ACE | CCB_ACW | CCB_ACCW
|||
|||	    Use DeleteCel() to release all resources allocated for the cel.  The cel
|||	    data buffer and PLUT are released only if they were allocated by
|||	    CreateCel().  If you provided your own dataBuf pointer, you must free the
|||	    buffer yourself.
|||
|||	  Arguments
|||
|||	    width                        The width of the cel, and its data buffer,
|||	                                 in pixels.
|||
|||	    height                       The height of the cel, and its data buffer,
|||	                                 in pixels.
|||
|||	    bitsPerPixel                 The bits per pixel value for the cel.
|||
|||	    options                      Bitmapped options, as described above.
|||
|||	    dataBuf                      Option data buffer pointer; may be NULL to
|||	                                 have a buffer allocated automatically.
|||
|||	  Return Value
|||
|||	    A pointer to the new CCB, or NULL if there is not enough memory to make
|||	    the cel.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CreateBackdropCel(), DeleteCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/createlrformcel
|||	CreateLRFormCel - Create an LRForm cel.
|||
|||	  Synopsis
|||
|||	    CCB * CreateLRFormCel(CCB *cel, Item screenItem, SRect *subRect)
|||
|||	  Description
|||
|||	    This function creates an LRForm cel from a screen or portion of a screen.
|||	    LRForm cels can be used to 'capture' a screen's pixels and project them to
|||	    another screen (or another portion of the same screen).
|||
|||	    If the cel pointer is NULL, a new cel is allocated for you.  If non-NULL
|||	    it must be an LRForm cel created by an earlier call to this function.
|||	    Passing a non-NULL cel pointer allows you to repeatedly extract different
|||	    subrects from the same screen without having to create a new cel each
|||	    time.
|||
|||	    If the subRect pointer is NULL, the cel will be the same size as the
|||	    source screen.  If it is non-NULL, the cel will access the sub-rectangle
|||	    of pixels described by subRect.
|||
|||	    Unlike other Library call implemented in lib3do.lib.s, you cannot pass a Bitmap item in place of
|||	    the Screen item.  This is because the GetPixelAddress() function used to
|||	    locate the pixels in the screen will not accept a Bitmap item.
|||
|||	    The cel created by this function will use the pixels in the screen's
|||	    bitmap buffer as the cel source pixels.  The pixels are not copied, and if
|||	    you render new pixels into the screen, subsequent projections of the cel
|||	    will use the new pixels.
|||
|||	    The cel is created with a position of 0,0 and size and perspective values
|||	    for a 1:1 mapping.  The Preamble Word 1 Blue LSB handling is set to
|||	    PRE1_TLLSB_PDC0.  The PIXC word is set for standard opaque, 0x1F001F00.
|||	    The cel flags are set to:CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_PPABS |
|||	    CCB_LDSIZE | CCB_LDPRS | CCB_LDPPMP | CCB_CCBPRE | CCB_YOXY | CCB_USEAV |
|||	    CCB_NOBLK | CCB_ACE | CCB_ACW | CCB_ACCW | CCB_BGND | PMODE_ONE
|||
|||	    Use DeleteCel() to release the resources allocated for the cel.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to a cel created earlier by this
|||	                                 function, or NULL to create a new cel.
|||
|||	    screenItem                   The item number of the Screen whose pixels
|||	                                 this cel is to project.
|||
|||	    subRect                      A pointer to an SRect describing the sub-
|||	                                 rectangle to be extracted from the Screen,
|||	                                 or NULL for full-screen.
|||
|||	  Return Value
|||
|||	    The input parameter dst, if dst was non-NULL on input.  A pointer to the
|||	    newly created cel or NULL if there is not enough memory to create the cel,
|||	    if dst was NULL on input.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CreateSubrectCel(), DeleteCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/createsubrectcel
|||	CreateSubrectCel - Create a cel that projects a sub-rectangle of another
|||	                   cel's pixels.
|||
|||	  Synopsis
|||
|||	    CCB * CreateSubrectCel(CCB *dst, CCB *src, SRect *subRect)
|||
|||	  Description
|||
|||	    This function creates a cel from a portion of another cel's data.  The new
|||	    cel projects a subrectangle of the src cel's pixels.
|||
|||	    If the dst pointer is NULL this function allocates a new cel by doing a
|||	    CloneCel(src, CLONECEL_CCB_ONLY).  The CCB_CCBPRE flag is forced on in the
|||	    new cel so that preamble word changes can be done without affecting the
|||	    source cel.  The new cel's preamble words and ccb_SourcePtr are modified
|||	    to access the specified subrectangle of the src cel's pixels.  The pixels
|||	    are not copied, the dst cel's ccb_SourcePtr will point into the src cel's
|||	    data buffer.
|||
|||	    If the src cel is CODED, the dst cel's ccb_PLUTPtr will point to the src
|||	    cel's PLUT, but you are free to change ccb_PLUTPtr in the dst cel if you
|||	    want.  You are also free to change the dst cel's Flags, PIXC word, and in
|||	    general any CCB values.   Be careful about changing the preamble word
|||	    values that describe the subrectangle of the src cel's data.
|||
|||	    If the dst pointer is non-NULL, the allocation of a new cel is bypassed,
|||	    and only the preamble and ccb_SourcePtr calcs are done.  This option
|||	    provides improved performance; you can allocate the subrect cel just once,
|||	    then continually call this function to have that cel access different
|||	    portions of the src cel's pixels.
|||
|||	    Using a non-NULL dst pointer is an implicit request for high performance,
|||	    so this function does no error or sanity checking.  In particular, it is
|||	    your responsibility to ensure that the dst cel you supply is compatible
|||	    with the src cel you want to extract data from, in terms of bit depth and
|||	    other CCB parameters.  The recommended method is to use this function with
|||	    a NULL dst pointer once to create a subrect cel for a given source cel,
|||	    then only reuse that dst pointer in association with that source cel.
|||
|||	    This function works with all types of cels of any bit depth, except that
|||	    you cannot extract a subrectangle from a PACKED source cel due to way
|||	    packed data is stored.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to a cel created earlier by this
|||	                                 function, or NULL to create a new cel.
|||
|||	    src                          A pointer to an existing cel; NULL is not
|||	                                  allowed.
|||
|||	    subRect                      A pointer to an SRect describing the sub-
|||	                                 rectangle to be extracted from the src cel;
|||	                                 NULL is not allowed.
|||
|||	  Return Value
|||
|||	    The input parameter dst, if dst was non-NULL on input.  A pointer to the
|||	    newly created cel or NULL if there is not enough memory to create the cel,
|||	    if dst was NULL on input.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CreateLRFormCel(), DeleteCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/createtextcel
|||	CreateTextCel - Create a text cel.
|||
|||	  Synopsis
|||
|||	    TextCel * CreateTextCel(FontDescriptor *fDesc,
|||	                            uint32 formatFlags,
|||	                            int32 width,
|||	                            int32 height)
|||
|||	  Description
|||
|||	    Creates a TextCel using the specified sizes and formatting options.
|||	    Initially, the cel data buffer is empty; until you place text into the cel
|||	    all pixels are zero.
|||
|||	    If width and/or height are zero, the cel is initially created with a 1 x 1
|||	    pixel size. Each time text is placed into the cel, the size automatically
|||	    changes to reflect the size of the text in the cel.
|||
|||	    The formatFlags parameter contains one or more formatting options, ORed
|||	    together.  Available options are:
|||	    TC_FORMAT_LEFT_JUSTIFY
|||	        left-justify text
|||	    TC_FORMAT_RIGHT_JUSTIFY
|||	        right -justify text
|||	    TC_FORMAT_CENTER_JUSTIFY
|||	        center-justify text
|||	    TC_FORMAT_WORDWRAP
|||	        auto-word-wrap text
|||
|||	    Use DeleteTextCel() to dispose of the cel when you are finished with it.
|||
|||	  Arguments
|||
|||	    fDesc                        Pointer to a FontDescriptor structure.
|||
|||	    formatFlags                  Text-formatting options.
|||
|||	    width                        Width of the cel.
|||
|||	    height                       Height of the cel.
|||
|||	  Return Value
|||
|||	    A pointer to the newly created TextCel structure; NULL if there isn\xd5 t
|||	    enough memory to create the new TextCel.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
|||	  See Also
|||
|||	    CloneTextCel(), DeleteTextCel(), taCreateTextCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/crectfromcelreturn
|||	CRectFromCelReturn  - CRect describing cel as currently projected.
|||
|||	  Synopsis
|||
|||	    CRect * CRectFromCel(CRect *dst, CCB *cel)
|||
|||	  Description
|||
|||	    This function calculates a CRect that describes the cel's projection, and
|||	    stores the results into *dst.  If the cel is projected to a
|||	    non-rectangular shape, the result will be invalid.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to a CRect where the results are
|||	                                 to be stored.
|||
|||	    cel                          A pointer to the cel.
|||
|||	  Return Value
|||
|||	    Returns dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    SRectFromCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/crectfromival
|||	CRectFromIVal - Fill in a CRect from inline values.
|||
|||	  Synopsis
|||
|||	    CRect * CRectFromIVal(CRect *dst, int32 tlx, int32 tly,
|||	                          int32 brx, int32 bry)
|||
|||	  Description
|||
|||	    This function stores the four specified corner values into the CRect at
|||	    *dst.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to a CRect where the results are
|||	                                 to be stored.
|||
|||	    tlx                          The value to assign to dst->tl.x.
|||
|||	    tly                          The value to assign to dst->tl.y.
|||
|||	    brx                          The value to assign to dst->br.x.
|||
|||	    bry                          The value to assign to dst->br.y.
|||
|||	  Return Value
|||
|||	    Returns dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CRectFromSRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/crectfromsrect
|||	CRectFromSRect - Convert an SRect to a CRect.
|||
|||	  Synopsis
|||
|||	    CRect * CRectFromSRect(CRect *dst, SRect *src)
|||
|||	  Description
|||
|||	    This function converts an SRect into a CRect that describes the same area.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to a CRect where the results are
|||	                                 to be stored.
|||
|||	    src                          A pointer to an SRect to convert to a CRect.
|||
|||	  Return Value
|||
|||	    Returns dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CRectFromIVal(), SRectFromCRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/crectintersection
|||	CRectIntersection - Calculate the intersection of two CRects.
|||
|||	  Synopsis
|||
|||	    CRect * CRectIntersection(CRect *dst, CRect *rect1, CRect *rect2)
|||
|||	  Description
|||
|||	    This function calculates the intersection of two rectangles, returning a
|||	    rectangle that describes the common area.  The dst rectangle can be the
|||	    same as either source rectangle.  If there is no common area between the
|||	    two source rectangles, the function return value is NULL, but the values
|||	    in *dst are still modified.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to a CRect where the results are
|||	                                 to be stored.
|||
|||	    rect1                        A pointer to one of the source CRects.
|||
|||	    rect2                        A pointer to the other source CRect.
|||
|||	  Return Value
|||
|||	    Returns dst, or NULL if there is no common area between the source
|||	    rectangles.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CRectBounds(), SRectIntersection()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/crectbounds
|||	CRectBounds - Calculate the bounding box of two CRects.
|||
|||	  Synopsis
|||
|||	    CRect * CRectBounds(CRect *dst, CRect *rect1, CRect *rect2)
|||
|||	  Description
|||
|||	    This function calculates the bounding box of two rectangles, returning a
|||	    rectangle that encompases all the area described by the two source
|||	    rectangles.  The dst rectangle can be the same as either source rectangle.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to a CRect where the results are
|||	                                 to be stored.
|||
|||	    rect1                        A pointer to one of the source CRects.
|||
|||	    rect2                        A pointer to the other source CRect.
|||
|||	  Return Value
|||
|||	    Returns dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CRectIntersection(), SRectBounds()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/crossfadecels
|||	CrossFadeCels - Crossfade between two cels.
|||
|||	  Synopsis
|||
|||	    CCB * CrossFadeCels(Item screen, int32 step,
|||	                        CCB *oldCel, CCB *newCel)
|||
|||	  Description
|||
|||	    This function calculates and optionally draws a 16-level crossfade between
|||	    two cels.  The crossfade is accomplished in 16 equal steps by 16 calls to
|||	    this function with the step parameter varying between 0 and 15 on each
|||	    subsequent call.  Step 0 draws oldCel at 15/16 intensity and newCel at
|||	    1/16 intensity, step 1 draws 14/16 oldCel and 2/16 newCel, and so on up to
|||	    step 15 which is 0/16 oldCel and 16/16 newCel (IE, oldCel is not drawn and
|||	    newCel is drawn as opaque).
|||
|||	    The screen parameter can be either a Screen or Bitmap item, in which case
|||	    the cels are drawn on each call. It can also be zero, in which case the
|||	    calculations are done but the cels are not drawn.  In the latter case, the
|||	    return value is a pointer to the cels you must draw; the oldCel and newCel
|||	    CCBs will be linked together in the right order for drawing.  As the
|||	    crossfade progresses the order in which the cels are drawn changes, so if
|||	    you pass zero for the screen item you must draw the cels based on this
|||	    function's return value.
|||
|||	    This function make certain assumptions about the cels.  If the cels don't
|||	    match the assumptions your mileage may vary, visually.  Both cels should
|||	    be single cels, not lists of cels and not anti-aliased cels.  Both cels
|||	    should project to the same screen area.  If the cels contain transparency
|||	    in differing locations and there are non-black pixels under the
|||	    transparency area in the bitmap, the results may not be what you expect in
|||	    that area.  Coded-8 cels won't work because their AMV-based scaling can't
|||	    be mixed with the PIXC-based scaling set up by this function.  The
|||	    ccb_NextPtr, PIXC word, and CCB_LAST flag values in both cels are modified
|||	    on each call.  After the last call (step=15) both cels' PIXC words are
|||	    reset to 0x1F001F00, both cels' CCB_LAST flags are set, and both cels'
|||	    ccb_NextPtr links are NULL.  If this doesn't work for your needs, save and
|||	    restore these values yourself, or clone the CCBs and pass the clones to
|||	    this function.
|||
|||	    Other than the limitations and assumptions mentioned above, any type of
|||	    cels should work just fine (E.G., coded/uncoded, packed/not-packed, any
|||	    flags settings, any PLUT values, any size/perspective mapping, etc).  The
|||	    two cels can be different types (E.G., oldCel is coded6/newCel is
|||	    uncoded16, oldCel is LRFORM/newCel isn't, etc).
|||
|||	    Each cel is drawn just once; no intermediate prescale-the-bitmap drawing
|||	    is done.
|||
|||	    This function is coded so that you can start at any fade level, and skip
|||	    intermediate levels if you want.  The last step should not be skipped,
|||	    because it's the one that restores the PIXC words.  You can also call any
|||	    step multiple times.
|||
|||	  Arguments
|||
|||	    screen                       A Screen or Bitmap item to which the cels
|||	                                 are drawn, or zero to bypass the internal
|||	                                 drawing step.
|||
|||	    step                         A value between 0 and 15 inclusive.
|||
|||	    oldCel                       A pointer to the old (outgoing) cel.
|||
|||	    newCel                       A pointer to the new (incoming) cel.
|||
|||	  Return Value
|||
|||	    A pointer to the cels to be drawn, useful only if the screen parameter was
|||	    zero to bypass internal drawing.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CrossFadeCels8()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/crossfadecels8
|||	CrossFadeCels8 - Crossfade between two cels.
|||
|||	  Synopsis
|||
|||	    CCB * CrossFadeCels8(Item screen, int32 step,
|||	                         CCB *oldCel, CCB *newCel)
|||
|||	  Description
|||
|||	    This function calculates and optionally draws an 8-level crossfade between
|||	    two cels.  The crossfade is accomplished in 8 equal steps by 8 calls to
|||	    this function with the step parameter varying between 0 and 7 on each
|||	    subsequent call.  Step 0 draws oldCel at 7/8 intensity and newCel at 1/8
|||	    intensity, step 1 draws 6/8 oldCel and 2/8 newCel, and so on up to step 7
|||	    which is 0/8 oldCel and 8/8 newCel (IE, oldCel is not drawn and newCel is
|||	    drawn as opaque).
|||
|||	    The screen parameter can be either a Screen or Bitmap item in which case
|||	    the cels are drawn on each call, or it can be zero in which case the
|||	    calculations are done but the cels are not drawn.  In the latter case, the
|||	    return value is a pointer to the cels you must draw; the oldCel and newCel
|||	    CCBs will be linked together in the right order for drawing.  As the
|||	    crossfade progresses, the order in which the cels are drawn changes, so if
|||	    you pass zero for the screen item you must draw the cels based on this
|||	    function's return value.
|||
|||	    This function make certain assumptions about the cels.  If the cels don't
|||	    match the assumptions your mileage may vary, visually.  Both cels should
|||	    be single cels, not lists of cels and not anti-aliased cels.  Both cels
|||	    should project to the same screen area.  If the cels contain transparency
|||	    in differing locations and there are non-black pixels under the
|||	    transparency area in the bitmap, the results may not be what you expect in
|||	    that area.  Coded-8 cels won't work because their AMV-based scaling can't
|||	    be mixed with the PIXC-based scaling set up by this function.  The
|||	    ccb_NextPtr, PIXC word, and CCB_LAST flag values in both cels are modified
|||	    on each call.  After the last call (step=15) both cels' PIXC words are
|||	    reset to 0x1F001F00, both cels' CCB_LAST flags are set, and both cels'
|||	    ccb_NextPtr links are NULL.  If this doesn't work for your needs, save and
|||	    restore these values yourself, or clone the CCBs and pass the clones to
|||	    this function.
|||
|||	    Other than the limitations/assumptions mentioned above, any type of cels
|||	    should work just fine (EG, coded/uncoded, packed/not-packed, any flags
|||	    settings, any PLUT values, any size/perspective mapping, etc).  The two
|||	    cels can be different types (EG, oldCel is coded6/newCel is uncoded16,
|||	    oldCel is LRFORM/newCel isn't, etc).
|||
|||	    Each cel is drawn just once; no intermediate pre-scale-the-bitmap drawing
|||	    is done.
|||
|||	    This function is coded so that you can start at any fade level, and skip
|||	    intermediate levels if you want.  The last step should not be skipped,
|||	    because it's the one that restores the PIXC words.  You can also call any
|||	    step multiple times.
|||
|||	  Arguments
|||
|||	    screen                       A Screen or Bitmap item to which the cels
|||	                                 are drawn, or zero to bypass the internal
|||	                                 drawing step.
|||
|||	    step                         A value between 0 and 15 inclusive.
|||
|||	    oldCel                       A pointer to the old (outgoing) cel.
|||
|||	    newCel                       A pointer to the new (incoming) cel.
|||
|||	  Return Value
|||
|||	    A pointer to the cels to be drawn, useful only if the screen parameter was
|||	    zero to bypass internal drawing.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CrossFadeCels()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/deletebasicdisplay
|||	DeleteBasicDisplay - Deletes a display created with
|||	                     CreateBasicDisplay()
|||
|||	  Synopsis
|||
|||	    Err DeleteBasicDisplay(ScreenContext *sc)
|||
|||	  Description
|||
|||	    This function releases all resources acquired internally by
|||	    CreateBasicDisplay(), and it closes the graphics folio.
|||
|||	  Arguments
|||
|||	    sc                           A pointer to the ScreenContext structure
|||	                                 which CreateBasicDisplay() filled in.
|||	                                 If this pointer is NULL, this function
|||	                                 returns an error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, displayutils.h
|||
|||	  See Also
|||
|||	    CreateBasicDisplay()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/deletecel
|||	DeleteCel - Delete a cel created by any Library call implemented in lib3do.lib..
|||
|||	  Synopsis
|||
|||	    CCB * DeleteCel(CCB *cel)
|||
|||	  Description
|||
|||	    This function deletes any type of cel created by Lib3DO library functions.
|||	    A NULL pointer is valid as input, and is quietly treated as a no-op.  If
|||	    extra resources, such as a data buffer or PLUT, were allocated by the
|||	    creating function they are also released.
|||
|||	    This function correctly handles anti-aliased cels, cels loaded as a group
|||	    from a single cel file, and other such constructs where several related
|||	    cels were created and treated as if they were a single cel.  If a Lib3DO
|||	    function gives you back a single CCB* return value even when it creates
|||	    several linked cels at once, then you need pass only that single CCB* to
|||	    DeleteCel() to release everything allocated when the group of cels was
|||	    created.
|||
|||	    This function works correctly with cels loaded from a file via LoadCel(),
|||	    and is equivelent to UnloadCel().  It also works on TextCels created by
|||	    the TextLib functions; DeleteCel() on the tc_CCB value of a TextCel is
|||	    equivelent to DeleteTextCel().
|||
|||	    If you have custom functions for creating cels and those functions do not
|||	    use the standard library CreateCel() function as their basis, you can
|||	    provide  DeleteCel() support for your custom cels.  Doing so is a somewhat
|||	    complex procedure which is fully documented in the Implementation Notes
|||	    section of the source code file DeleteCelMagic.c in the Lib3DO source
|||	    code.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to a cel created by any Lib3DO
|||	                                 function; may be NULL.
|||
|||	  Return Value
|||
|||	    NULL, always.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    DeleteCelList()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/deletecellist
|||	DeleteCelList - Delete a list of cels created by any Library call implemented in lib3do.lib.
|||
|||	  Synopsis
|||
|||	    CCB * DeleteCelList(CCB *celList)
|||
|||	  Description
|||
|||	    This function deletes a list of one or more cels.  It walks the
|||	    ccb_NextPtr links calling DeleteCel() on each cel, implying that all the
|||	    cels in the list must be ones created by DeleteCel-compatible functions.
|||	    A NULL pointer is valid as input, and is quietly treated as a no-op.
|||
|||	    This function contains special logic for handling cels where the library
|||	    function creates a related group of cels and returns a single CCB* value
|||	    to you.  In this sense, it is not exactly equivelent to walking the
|||	    ccb_NextPtr links.  When a group of related cels is linked into a larger
|||	    list, you need to call DeleteCel() only for the first cel in the group of
|||	    related cels.  This function contains logic to call DeleteCel() only on
|||	    the first cel in such a related group, then skip over the related cels
|||	    that get deleted all at once, and continue processing with any other cels
|||	    linked after the related group.  For this reason, it is best to use
|||	    DeleteCelList() if you have acquired cels from various Library call implemented in lib3do.lib.s
|||	    and linked them together into a larger list.  If you attempt to walk the
|||	    list yourself calling DeleteCel() on each cel, you have to keep track of
|||	    whether any of the cels need to be skipped as you go.
|||
|||	  Arguments
|||
|||	    celList                      A pointer to a list of one or more cels
|||	                                 created by Library call implemented in lib3do.lib.s; may be NULL.
|||
|||	  Return Value
|||
|||	    NULL, always.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    DeleteCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/deletetextcel
|||	DeleteTextCel - Delete a text cel and associated resources.
|||
|||	  Synopsis
|||
|||	    void DeleteTextCel(TextCel *tCel)
|||
|||	  Description
|||
|||	    Deletes the specified text cel and all resources it acquired internally
|||	    when it was created.  If SetTextCelFormatBuffer()dynamically allocates a
|||	    format buffer for the cel, that buffer is also released.
|||
|||	    Text cels are also compatible with the Lib3DO DeleteCel() and
|||	    DeleteCelList() functions.  Those functions require a pointer to a CCB.
|||	    When they receive a pointer to a CCB associated with a text cel,
|||	    processing is automatically handed off to DeleteTextCel() internally.
|||
|||	  Arguments
|||
|||	    tCel                           Pointer to a TextCel structure (okay to
|||	                                 pass NULL).
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
|||	  See Also
|||
|||	    CloneTextCel(), CreateTextCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/detachtextcelccb
|||	DetachTextCelCCB - Release text-related resources, retaining rendered
|||	                   pixels.
|||
|||	  Synopsis
|||
|||	    CCB * DetachTextCelCCB(TextCel *tCel)
|||
|||	  Description
|||
|||	    Releases resources required for rendering new text into a cel, but retains
|||	    pixels already rendered.  In addition to freeing resources, this function
|||	    removes the reference to the FontDescriptor.  You can safely unload a font
|||	    after calling this function, and still retain text you've already
|||	    rendered for drawing to the screen later when needed.
|||
|||	    Use DeleteCel()orDeleteCelList() to dispose of the CCB and cel data buffer
|||	    when you no longer need them.  You cannot  use DeleteTextCel() on the cel
|||	    after calling this function, since the TextCel structure has been
|||	    discarded.
|||
|||	  Arguments
|||
|||	    tCel                         pointer to a TextCel structure.
|||
|||	  Return Value
|||
|||	    A pointer to the CCB, the value that was originally in tCel->tc_CCB.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/disablemsevent
|||	DisableMSEvent - Halt dispatching for one event.
|||
|||	  Synopsis
|||
|||	    void DisableMSEvent(MSEventData *theEvent, int32 reserved)
|||
|||	  Description
|||
|||	    This function temporarily disables dispatching of the specified event.  If
|||	    a message or signal is pending at the time this function is called that
|||	    fact is remembered and the event will be dispatched after it is
|||	    re-enabled.  You can safely call this from within the handler for the
|||	    event being disabled, or from a handler for another event being controlled
|||	    by the same MSEventData array.
|||
|||	  Arguments
|||
|||	    theEvent                     A pointer to the element of the MSEventData
|||	                                 array that describes the event to disable.
|||
|||	    reserved                     Reserved for future expansion; pass zero.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, msgutils.h
|||
|||	  See Also
|||
|||	    EnableMSEvent()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/dispatchmsevents
|||	DispatchMSEvents - Dispatch message and signal events to their handler
|||	                   routines.
|||
|||	  Synopsis
|||
|||	    int32 DispatchMSEvents(MSEventHandle mseHandle,
|||	                           void *userContext,
|||	                           int32 reserved)
|||
|||	  Description
|||
|||	    This function waits for events and dispatches processing to the events'
|||	    handlers.  It loops internally, waiting for messages and signals,
|||	    dispatching the handlers when they are received, then looping back to wait
|||	    again after the handlers return.  As long as the handlers keep returning
|||	    zero status this function loops internally.  As soon as a handler returns
|||	    a non-zero value this function returns that value to the caller.
|||
|||	    By convention, a negative return value is an error.  This may be an error
|||	    returned by a handler function, or an error detected in waiting for and
|||	    decoding an event.  The latter should be very rare; the process of waiting
|||	    for a message or signal, looking up the message, and so on, is relatively
|||	    foolproof.  It's likely to fail only if a message or port item is
|||	    DeleteItem()'d out from under the dispatcher (so don't do that!).
|||
|||	  Arguments
|||
|||	    mseHandle                    An MSEventHandle returned by
|||	                                 SetupMSEvents().
|||
|||	    userContext                  Any value you want; it is passed to the
|||	                                 handler functions as they are dispatched.
|||
|||	    reserved                     Reserved for future expansion; pass zero.
|||
|||	  Return Value
|||
|||	    Returns negative on error, or any non-zero status returned by your event
|||	    handler routines.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, msgutils.h
|||
|||	  See Also
|||
|||	    SetupMSEvents()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/drawanimcel
|||	DrawAnimCel - Draw a cel in an animation and give it a hotspot
|||
|||	  Synopsis
|||
|||	    void DrawAnimCel (ANIM *pAnim, Item bitmapItem,
|||	                      long xPos, long YPos,
|||	                      frac16 frameIncrement,
|||	                      long hotSpot)
|||
|||	  Description
|||
|||	    Determines the next cel to draw in the animation specified by pAnim and
|||	    draws it. Lets you draw a hotspot on that frame.
|||
|||	  Arguments
|||
|||	    pAnim                        Pointer to an ANIM structure returned by
|||	                                 LoadAnim().
|||
|||	    An ANIM structure, defined in parse3do.h, looks as follows:
|||
|||	                typedef struct tag_ANIM {
|||	                        long    num_Frames;     // max number of PDATs or CCBs in file
|||	                        frac16  cur_Fram        // allows fract values for smooth speed
|||	                        long    num_Alloced_Frames;
|||	                        AnimFrame *pentries;
|||	                } ANIM;
|||
|||	    bitmapItem                   item  describing the bitmap into which the
|||	                                 cel will be drawn.
|||
|||	    xPos                          x (horizontal) position for a hotspot in
|||	                                 the cel.
|||
|||	    YPos                          y (vertical) position for a hotspot in the
|||	                                 cel.
|||
|||	    frameIncrement               a frac16, 16.16 fixed point number to add to
|||	                                 the current frame count. This is rounded to
|||	                                 an integer to determine the next frame in
|||	                                 the animation. If frameIncrement increments
|||	                                 the current frame past the end of the
|||	                                 animation, cur_Frame will be reset to the
|||	                                 beginning. Note that this works backwards as
|||	                                 well.
|||
|||	    hotSpot                       A integer (enum) equal to one of the
|||	                                 following (defined in utils3do.h).
|||	                        #define CenterHotSpot           1
|||	                        #define UpperLeftHotSpot        2
|||	                        #define LowerLeftHotSpot        3
|||	                        #define UpperRightHotSpot       4
|||	                        #define LowerRightHotSpot       5
|||
|||	    The cel is positioned with the hotspot at xPos, YPos.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, animutils.h
|||
|||	  See Also
|||
|||	    GetAnimCel(), LoadAnim()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/drawimage
|||	DrawImage - Use CopyVRAMPages() to draw an image
|||
|||	  Synopsis
|||
|||	    bool DrawImage (Item iScreen, ubyte *pbImage,
|||	                    ScreenContext *sc)
|||
|||	  Description
|||
|||	    Uses the fast SPORT mechanism, CopyVRAMPages(), to project an image into a
|||	    frame buffer. The image must be stored in page aligned VRAM. The function
|||	    executes during a vertical blank.
|||
|||	    Note that this function creates and deletes an IOReq item to
|||	    communicate with the SPORT device every single time it is called.
|||	    This is not efficient and decreases performance. It is better
|||	    to allocate the IOReq once at the beginning of a program, and
|||	    reuse it throughout. Therefore, you're better off not using
|||	    DrawImage(), and just call CopyVRAMPages() yourself. This
|||	    function is useful only in a quicky do-it-just-once type of
|||	    situation.
|||
|||	  Arguments
|||
|||	    iScreen                      Item for the screen into which to project
|||	                                 the image.
|||
|||	    pbImage                      Pointer to a page-aligned buffer in VRAM
|||	                                 containing a full-screen 16-bit image.
|||
|||	    sc                           Pointer to a screen context.
|||
|||	  Return Value
|||
|||	    TRUE if successful, FALSE if not.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, displayutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/drawtextchar
|||	DrawTextChar - Draw a character into a bitmap.
|||
|||	  Synopsis
|||
|||	    void DrawTextChar (FontDescriptor *fDesc,
|||	                       GrafCon *gcon,
|||	                       Item bitmapItem,
|||	                       uint32 character)
|||
|||	  Description
|||
|||	    Draws a character directly into a bitmap, using the colors and x/y
|||	    coordinates specified in the GrafCon.
|||
|||	    The GrafCon's pen coordinates are interpreted as integer (not Frac16)
|||	    values. The call updates the GrafCon's gc_PenX and gc_PenY fields to
|||	    reflect the space the character pixels occupy within the bitmap. A newline
|||	    (\n) character moves the gc_PenY value down by the font's
|||	    height+leading value and sets the gc_PenX value to zero.
|||
|||	    The foreground and background colors follow the usual rules for cels: A
|||	    color of 000 results in transparent output, for opaque black use 001.
|||
|||	    This function is an extremely inefficient way to get characters onto the
|||	    screen.  To draw more than one character, use the DrawTextString() or
|||	    TextCel functions.
|||
|||	  Arguments
|||
|||	    fDesc                        Pointer to a FontDescriptor structure.
|||
|||	    gcon                         Pointer to a GrafCon structure containing
|||	                                 the foreground and background colors and pen
|||	                                 x/y coordinates to use in rendering the
|||	                                 character.
|||
|||	    bitmapItem                   The bitmap into which the character is
|||	                                 rendered.
|||
|||	    character                    The character to render.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
|||	  See Also
|||
|||	    DrawTextString()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/drawtextstring
|||	DrawTextString - Draw a text string into a bitmap.
|||
|||	  Synopsis
|||
|||	    void DrawTextString(FontDescriptor *fDesc,
|||	                        GrafCon *gcon,
|||	                        Item bitmapItem,
|||	                        char *text, ...)
|||
|||	  Description
|||
|||	    Draws a text string directly into a bitmap, using the colors and x/y
|||	    coordinates specified in the GrafCon. The text string can be a simple
|||	    string of characters or a printf-style format string followed by
|||	    additional arguments. If you use printf-style formatting, the resulting
|||	    formatted string cannot exceed 1024 characters. A maximum of ten
|||	    printf-style % formatting directives can appear in the string.  <<This
|||	    limitation will be lifted when proper sprintf() support is added to the
|||	    runtime library.>>
|||
|||	    The GrafCon's pen coordinates are interpreted as integer (not Frac16)
|||	    values. After the call, the GrafCon's gc_PenX and gc_PenY fields have been
|||	    updated to reflect the space the character pixels occupy within the
|||	    bitmap. A newline (\n) character moves the gc_PenY value down by the
|||	    font's height + leading value, and sets gc_PenX to its entry-time value.
|||	    The net effect is that when there are multiple lines of text, each line
|||	    begins in the column specified by the initial value of gc_PenX.
|||
|||	    The foreground and background colors follow the usual rules for cels: a
|||	    color of 000 results in transparent output, for opaque black use 001.
|||
|||	    If you use typical double-buffering screen logic, it is more efficient to
|||	    use a TextCel to hold your text string and redraw it on each frame with
|||	    DrawScreenCels() than to rerender the string directly into the screen
|||	    bitmap on each frame update.
|||
|||	  Arguments
|||
|||	    fDesc                        Pointer to a FontDescriptor structure.
|||
|||	    gcon                         Pointer to a GrafCon structure containing
|||	                                 the foreground and background colors and pen
|||	                                 x/y coordinates to use in rendering the
|||	                                 character.
|||
|||	    bitmapItem                   The bitmap into which the character is
|||	                                 rendered.
|||
|||	    text                         The string of characters to be rendered,
|||	                                 optionally including printf-style %
|||	                                 formatting commands.
|||
|||	    ...                          Optional arguments for printf-style
|||	                                 formatting.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
|||	  See Also
|||
|||	    DrawTextChar()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/enablemsevent
|||	EnableMSEvent - Enable dispatching for one event.
|||
|||	  Synopsis
|||
|||	    void EnableMSEvent(MSEventData *theEvent, int32 reserved)
|||
|||	  Description
|||
|||	    This function re-enables dispatching of an event previously disabled by
|||	    DisableMSEvent().  If a message or signal was pending at the time you
|||	    called DisableMSEvent(), or if an event arrived while the handler was
|||	    disabled, the event is dispatched to the handler as soon as it is
|||	    re-enabled by this function.
|||
|||	  Arguments
|||
|||	    theEvent                     A pointer to the element of the MSEventData
|||	                                 array that describes the event to disable.
|||
|||	    reserved                     Reserved for future expansion; pass zero.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, msgutils.h
|||
|||	  See Also
|||
|||	    DisableMSEvent()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/erasetextincel
|||	EraseTextInCel - Clear pixels in a text cel.
|||
|||	  Synopsis
|||
|||	    void EraseTextInCel(TextCel *tCel)
|||
|||	  Description
|||
|||	    Clears existing pixels from a text cel's data buffer.  All pixels are set
|||	    to zero.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/fadefromblack
|||	FadeFromBlack - Fade in the screen from black
|||
|||	  Synopsis
|||
|||	    void FadeFromBlack (ScreenContext *sc, int32 frameCount)
|||
|||	  Description
|||
|||	    Fades all the screens in the ScreenContext from black over frameCount.
|||
|||	  Arguments
|||
|||	    sc                           Pointer to a ScreenContext
|||
|||	    frameCount                   Number of frames over which the fade will
|||	                                 occur. After nFrames, all the color entries
|||	                                 in the CLUT have their correct color value.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, displayutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/fadetoblack
|||	FadeToBlack - Fade the screen to black
|||
|||	  Synopsis
|||
|||	    void FadeToBlack (ScreenContext *sc, int32 nFrames)
|||
|||	  Description
|||
|||	    Fades all the screens in the nFrames to black over nFrames.
|||
|||	  Arguments
|||
|||	    sc                           Pointer to a ScreenContext
|||
|||	    nFrames                      Number of frames over which the fade will
|||	                                 occur. After nFrames, all the color entries
|||	                                 in the CLUT will be zero, or black.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, displayutils.h
|||
|||	  See Also
|||
|||	    FadeFromBlack()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/findchunk
|||	FindChunk - Find a chunk of specified chunk ID
|||
|||	  Synopsis
|||
|||	    char *FindChunk (ulong chunk_ID, char **buffer, long *bufLen)
|||
|||	  Description
|||
|||	    Searches buffer until it finds chunk_ID. On exit, updates the buffer
|||	    pointer that is to point to the next chunk and bufLen to contain the
|||	    number of bytes remaining in the buffer.
|||
|||	  Arguments
|||
|||	    chunk_ID                     ID of the chunk you are looking for.
|||
|||	    *buffer                      Pointer to the buffer you are looking for.
|||
|||	    bufLen                       Number of bytes in the buffer that follow
|||	                                 the chunk.
|||
|||	  Return Value
|||
|||	    Returns NULL or a pointer to the chunk.
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/finishasyncloadfile
|||	FinishAsyncLoadFile - Finalize an async file load operation.
|||
|||	  Synopsis
|||
|||	    Err FinishAsyncLoadFile(LoadFileInfo *lf, Err loadStatus)
|||
|||	  Description
|||
|||	    This function releases resources no longer needed after completion of an
|||	    async file load, leaving just the file data in the buffer (if the load was
|||	    successful).
|||
|||	    If you received I/O completion nofitication via message, call this
|||	    function passing ioreqPtr->io_Error as the loadStatus value and
|||	    ioreqPtr->io_User as the LoadFileInfo pointer.  If you detected I/O
|||	    completion using CheckAsyncLoadFile(), pass the return value from that
|||	    function as the loadStatus value.
|||
|||	    If the loadStatus value is negative and a buffer was automatically
|||	    allocated by AsyncLoadFile() this function frees the buffer.  It release
|||	    other resources (IOReq, etc) in all cases.
|||
|||	  Arguments
|||
|||	    lf                           A pointer to the LoadFileInfo structure for
|||	                                 the load operation.
|||
|||	    loadStatus                   The status returned by CheckAsyncLoadFile()
|||	                                 or contained in the I/O completion message
|||	                                 you received.
|||
|||	  Return Value
|||
|||	    Returns the loadStatus value you passed in.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, blockfile.h
|||
|||	  See Also
|||
|||	    AsyncLoadFile()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/fpointfromfval
|||	FPointFromFVal - Create an FPoint from inline values.
|||
|||	  Synopsis
|||
|||	    FPoint * FPointFromFVal(FPoint *dst, frac16 x, frac16 y)
|||
|||	  Description
|||
|||	    This function stores the specified x and y values into *dst.  It is useful
|||	    for writing inline calls such as:
|||
|||	    FPoint    wrk;
|||	    OffsetCelListByFDelta (cel, FPointFromFVal (&wrk, my_x, my_y));
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the FPoint where the results
|||	                                 are to be stored.
|||
|||	    x                            The value to assign to dst->x.
|||
|||	    y                            The value to assign to dst->y.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    FPointFromIVal()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/fpointfromipoint
|||	FPointFromIPoint - Convert an IPoint to an FPoint.
|||
|||	  Synopsis
|||
|||	    FPoint * FPointFromIPoint(FPoint *dst, IPoint *src)
|||
|||	  Description
|||
|||	    This function converts an IPoint to an FPoint.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the FPoint where the results
|||	                                 are to be stored.
|||
|||	    src                          A pointer to the IPoint to be converted.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    IPointFromFPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/fpointfromival
|||	FPointFromIVal - Create an FPoint from inline values.
|||
|||	  Synopsis
|||
|||	    FPoint * FPointFromIVal(FPoint *dst, int32 x, int32 y)
|||
|||	  Description
|||
|||	    This function converts the specified values from integer to frac16 and
|||	    stores them in *dst.  It is useful for writing inline calls such as:
|||
|||	    FPoint      wrk;
|||	    OffsetCelListByFDelta (cel, FPointFromIVal (&wrk, 2, 2));
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the FPoint where the results
|||	                                 are to be stored.
|||
|||	    x                            The value to be converted into dst->x.
|||
|||	    y                            The value to be converted into dst->y.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    FPointFromFVal()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/getanimcel
|||	GetAnimCel - Return a pointer to the CCB of the next animation cel
|||
|||	  Synopsis
|||
|||	    CCB * GetAnimCel (ANIM *pAnim, frac16 frameIncrement)
|||
|||	  Description
|||
|||	    Returns a pointer to the CCB of the next cel in an animation sequence.
|||	    Adds frameIncrement to the current frame counter within the ANIM. Does not
|||	    draw the cel.
|||
|||	  Arguments
|||
|||	    pAnim                        pointer to an ANIM structure returned by
|||	                                 LoadAnim().  The ANIM structure is defined as
|||	                                 follows:
|||
|||	                typedef struct tag_ANIM {
|||	                        long    num_Frames;     // max number of PDATs or CCBs in file
|||	                        frac16  cur_Frame;      // allows fract values for smooth speed
|||	                        long    num_Alloced_Frames;
|||	                        AnimFrame *pentries;
|||	                } ANIM;
|||
|||	    frameIncrement               a frac16 (16.16) fixed point number.
|||
|||	  Return Value
|||
|||	    Pointer to the CCB that was found at the specified position in the
|||	    animation.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, animutils.h
|||
|||	  See Also
|||
|||	    DrawAnimCel(), LoadAnim()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/getcelbitsperpixel
|||	GetCelBitsPerPixel - Return a cel's bits per pixel value.
|||
|||	  Synopsis
|||
|||	    int32 GetCelBitsPerPixel(CCB *cel)
|||
|||	  Description
|||
|||	    This function returns the bits-per-pixel value for the cel, determined by
|||	    examining the cel's preamble data.
|||
|||	    This function exists primarily to support library internals, but may be
|||	    useful for applications as well.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to a cel.
|||
|||	  Return Value
|||
|||	    Bits per pixel, one of 1, 2, 4, 6, 8, or 16.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/getcelbytesperrow
|||	GetCelBytesPerRow - Return a cel's bytes per row value.
|||
|||	  Synopsis
|||
|||	    int32 GetCelBytesPerRow(CCB *cel)
|||
|||	  Description
|||
|||	    This function returns the bytes-per-row value for the cel, determined by
|||	    examing the cel's preamble data.  The calculations take into account the
|||	    cel's bit per pixel and source width values, and padding for the cel
|||	    engine's alignment and lookahead requirements.
|||
|||	    This function exists primarily to support library internals, but may be
|||	    useful for applications as well.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to a cel.
|||
|||	  Return Value
|||
|||	    Bytes per row.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/getceldatabuffersize
|||	GetCelDataBufferSize - Return a cel\xd5 s data buffer size.
|||
|||	  Synopsis
|||
|||	    int32 GetCelDataBufferSize(CCB *cel)
|||
|||	  Description
|||
|||	    This function calculates the size needed for a cel data buffer based on
|||	    the values found in the cel's preamble words.  The calculations take into
|||	    account the cel's bit per pixel and source width and height values, and
|||	    padding for the cel engine's alignment and lookahead requirements.
|||
|||	    The return value is the size needed for the buffer, and not necessarily
|||	    the size of the buffer currently attached to the cel.  For example, using
|||	    this function on a SubRect cel would return the size needed for the
|||	    sub-rectangle, not the size currently used by the full-size source data
|||	    attached to the cel.
|||
|||	    This function can be called when the cel has no buffer yet, providing that
|||	    the CCB_CCBPRE flag is set.  In this sense, it can be useful for
|||	    calculating how big a data buffer to allocate.
|||
|||	    This function exists primarily to support library internals, but may be
|||	    useful for applications as well.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to a cel.
|||
|||	  Return Value
|||
|||	    Bytes needed for the buffer.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/getfontcharinfo
|||	GetFontCharInfo - Retrieve character information.
|||
|||	  Synopsis
|||
|||	    int32 GetFontCharInfo(FontDescriptor *fDesc,
|||	                          int32 character, void **blitInfo)
|||
|||	  Description
|||
|||	    Returns information about the specified character. The return value is the
|||	    width of the character (not including intercharacter spacing). The void
|||	    pointer at *blitInfo will contain a special value to speed up processing
|||	    within BlitFontChar().
|||
|||	    GetFontCharInfo() supports the internals of the text library; it should
|||	    never be necessary to call this function directly from an application
|||	    program. If you are implementing your own replacement text library, use
|||	    GetFontcharInfo() to obtain information about font character pixels before
|||	    blitting them into your own memory buffer. You can use the standard text
|||	    library source code as a guide in creating your own text library.
|||
|||	  Arguments
|||
|||	    fDesc                        Pointer to a FontDescriptor structure.
|||
|||	    character                    The specific character you're looking for.
|||
|||	    blitInfo                     A void pointer updated by this call and then
|||	                                 passed to the BlitFontChar() call.
|||
|||	  Return Value
|||
|||	    The width of the character; zero if the character is not present in the
|||	    font.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, fontlib.h
|||
|||	  See Also
|||
|||	    BlitFontChar(), GetFontCharWidth()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/getfontcharwidest
|||	GetFontCharWidest - Retrieve width of the widest character in a string.
|||
|||	  Synopsis
|||
|||	    int32 GetFontCharWidest(FontDescriptor *fDesc, char *string)
|||
|||	  Description
|||
|||	    Returns the width in pixels of the widest character in the string.
|||
|||	  Arguments
|||
|||	    fDesc                          Pointer to a FontDescriptor structure.
|||
|||	    string                         The string to be scanned for the widest
|||	                                 character width.
|||
|||	  Return Value
|||
|||	    The width of the widest character.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, fontlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/getfontcharwidth
|||	GetFontCharWidth - Retrieve character width.
|||
|||	  Synopsis
|||
|||	    int32 GetFontCharWidth(FontDescriptor *fDesc, int32 character)
|||
|||	  Description
|||
|||	    Retrieves the width of the specified character in the font. The return
|||	    value is the width of the character (not including intercharacter
|||	    spacing).
|||
|||	  Arguments
|||
|||	    fDesc                          Pointer to a FontDescriptor structure.
|||
|||	    character                      The specific character you're looking for.
|||
|||	  Return Value
|||
|||	    The width of the character; zero if the character is not present in the
|||	    font.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, fontlib.h
|||
|||	  See Also
|||
|||	    GetFontStringWidth()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/getfontstringwidth
|||	GetFontStringWidth - Calculate width of a string.
|||
|||	  Synopsis
|||
|||	    int32 GetFontStringWidth(FontDescriptor *fDesc, char *string)
|||
|||	  Description
|||
|||	    Calculates the  width of the specified string.  The return value is the
|||	    sum of the widths of the characters in the string.  The sum includes the
|||	    spacing between each character, but not following the final character.
|||
|||	  Arguments
|||
|||	    fDesc                          Pointer to a FontDescriptor structure.
|||
|||	    string                         The character string.
|||
|||	  Return Value
|||
|||	    The width of the string.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, fontlib.h
|||
|||	  See Also
|||
|||	    GetFontCharWidth()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gethsectime
|||	GetHSecTime - Obtain the current time in hundreths of a second.
|||
|||	  Synopsis
|||
|||	    int32 GetHSecTime(Item ioreq)
|||
|||	  Description
|||
|||	    This function obtains the current value of the system microsecond timer,
|||	    converts the seconds and microseconds into a single hundreths-of-a-second
|||	    value, and returns the converted value.
|||
|||	    The return value will overflow an int32 representation after the machine
|||	    has been powered on for about 250 days.  In other words, this function is
|||	    probably appropriate for debugging and performance measurement, but may
|||	    NOT be appropriate for a final application, since some application
|||	    environments (such as cable TV) are almost certain to run on a machine
|||	    powered on for months at a time.
|||
|||	  Arguments
|||
|||	    ioreq                        An IOReq item obtained from GetTimerIOReq(),
|||	                                 or zero to have an IOReq dynamically
|||	                                 created/deleted during the call.
|||
|||	  Return Value
|||
|||	    Returns positive number of hundreths-of-a-second or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    GetTimerIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gettextcelcolor
|||	GetTextCelColor - Retrieve background and foreground colors when cel is
|||	                  drawn.
|||
|||	  Synopsis
|||
|||	    void GetTextCelColor(TextCel *tCel,  int32 *bgColor,
|||	                         int32 *fgColor0)
|||
|||	  Description
|||
|||	    Retrieves the background and foreground colors used when the cel is drawn.
|||
|||	  Arguments
|||
|||	    t                            Cel  Pointer to a TextCel structure.
|||
|||	    bgColor                      Pointer to a variable to receive the
|||	                                 background color value.  May be NULL.
|||
|||	    fgColor                      Pointer to a variable to receive the
|||	                                 foreground color value.  May be NULL.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
|||	  See Also
|||
|||	    GetTextCelColors()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gettextcelcolors
|||	GetTextCelColors - Retrieve background and all four foreground colors.
|||
|||	  Synopsis
|||
|||	    void GetTextCelColors(TextCel *tCel, int32 bgColor,
|||	                          int32 fgColors[4])
|||
|||	  Description
|||
|||	    Retrieves the background and foreground colors used when the cel is drawn.
|||	    This function gets all four foreground colors; use it with the features
|||	    that allow multiple text colors within a single cel. The fgColors
|||	    parameter must be a pointer to an array of exactly four int32 values.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    bgColor                      Pointer to a variable to receive the
|||	                                 background color value.  May be NULL.
|||
|||	    fgColor                      Pointer to an array to receive the
|||	                                 foreground color values.  May be NULL.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
|||	  See Also
|||
|||	    GetTextCelColor()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gettextcelcoords
|||	GetTextCelCoords - Retrieve text cel's CCB coordinates.
|||
|||	  Synopsis
|||
|||	    void GetTextCelCoords(TextCel *tCel, Coord *ccbX, Coord *ccbY)
|||
|||	  Description
|||
|||	    Retrieves the TextCel CCB coordinates (ccb_XPos and ccb_YPos).
|||
|||	    If you find it convenient to do so, work with the CCB's position,
|||	    size, and perspective fields directly; you don't have to use this
|||	    function to safely obtain the on screen position.
|||
|||	    The ccbX and ccbY values are interpreted as frac16 values (the same as the
|||	    ccb_XPos and ccb_YPos fields themselves).
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    ccbX                         Pointer to a variable to receive the x
|||	                                 coordinate for the text cel CCB.
|||
|||	    ccbY                         Pointer to a variable to receive the y
|||	                                 coordinate for the text cel CCB.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gettextcelformatbuffer
|||	GetTextCelFormatBuffer - Retrieve text cel format buffer and size.
|||
|||	  Synopsis
|||
|||	    void GetTextCelFormatBuffer(TextCel *tCel, char **buffer,
|||	                                uint32 *bufSize)
|||
|||	  Description
|||
|||	    Retrieves the cel's formatting buffer and size. The format buffer is
|||	    used for printf-style formatting of text during rendering. If the buffer
|||	    size is zero, the buffer pointer is not valid.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    buffer                       Pointer to a variable to receive the pointer
|||	                                 to the format buffer.
|||
|||	    bufsize                      Pointer to a variable to receive the size of
|||	                                 the format buffer.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gettextcelformatflags
|||	GetTextCelFormatFlags - Retrieve text cel format flags.
|||
|||	  Synopsis
|||
|||	    uint32 GetTextCelFormatFlags(TextCel *tCel, uint32 *flags)
|||
|||	  Description
|||
|||	    Retrieves the cel's formatting flags.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    flags                        If non-NULL, the format flags are stored at
|||	                                 *flags.
|||
|||	  Return Value
|||
|||	    The format flags.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gettextcelleadingadjust
|||	GetTextCelLeadingAdjust - Retrieve text cel leading adjustment.
|||
|||	  Synopsis
|||
|||	    void GetTextCelLeadingAdjust(TextCel *tCel, int32 *adjustLeading)
|||
|||	  Description
|||
|||	    Retrieves the leading adjustment for text rendered in the cel. (Leading is
|||	    the extra vertical space between lines of text.)
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    adjustLeading                Pointer to a variable to receive the value
|||	                                 added to the base leading value for the
|||	                                 font.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gettextcelmargins
|||	GetTextCelMargins - Retrieve top and left margins.
|||
|||	  Synopsis
|||
|||	    void GetTextCelMargins(TextCel *tCel, int32 *leftMargin,
|||	                           int32 *topMargin)
|||
|||	  Description
|||
|||	    Retrieves the top and left margins within the cel data buffer.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    leftMargin                   Pointer to a variable to receive the left
|||	                                 margin for the cel.
|||
|||	    topMargin                    Pointer to a variable to receive the top
|||	                                 margin for the cel.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gettextcelpennumber
|||	GetTextCelPenNumber - Retrieve text cel pen number.
|||
|||	  Synopsis
|||
|||	    void GetTextCelPenNumber(TextCel *tCel, int32 *penNumber)
|||
|||	  Description
|||
|||	    Retrieves pen number used for text rendering.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    penNumber                    Pointer to a variable to receive the pen
|||	                                 number.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gettextcelsize
|||	GetTextCelSize - Retrieve text cel's current size.
|||
|||	  Synopsis
|||
|||	    void GetTextCelSize(TextCel *tCel, int32 *width, int32 *height)
|||
|||	  Description
|||
|||	    Retrieves the current size for the text cel. You can also obtain the
|||	    current size of a cel by reading the ccb_Width and ccb_Height fields in
|||	    the cel's CCB. Note that this retrieves the on screen size of the cel,
|||	    not the size of the text currently in the cel. (The text currently in the
|||	    cel may not fill the complete width or height of the cel.)
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    width                        Pointer to a variable to receive the width
|||	                                 for the cel.
|||
|||	    height                       Pointer to a variable to receive the height
|||	                                 for the cel.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gettextcelspacingadjust
|||	GetTextCelSpacingAdjust - Retrieve spacing adjustment.
|||
|||	  Synopsis
|||
|||	    void GetTextCelSpacingAdjust(TextCel *tCel, int32 *adjustSpacing)
|||
|||	  Description
|||
|||	    Retrieves the spacing adjustment for text rendered in the cel. Spacing is
|||	    the extra horizontal space between characters in the text.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    adjustSpacing                Pointer to a variable to receive the value
|||	                                 added to the base spacing value for the
|||	                                 font.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gettextceltabstops
|||	GetTextCelTabStops - Retrieve tab stops table.
|||
|||	  Synopsis
|||
|||	    void GetTextCelTabStops(TextCel *tCel, uint16 tabStops[16])
|||
|||	  Description
|||
|||	    Retrieves the tab stops table for the text cel.  A full table of 16 values
|||	    is stored even if some of the values are unused.  The end of the valid
|||	    entries in the table is marked with a zero.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    tabStops                     Pointer to an array of uint16 that receives
|||	                                 the tab stops table.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gettextextent
|||	GetTextExtent - Retrieve width and height for formatted text.
|||
|||	  Synopsis
|||
|||	    char * GetTextExtent(TextCel *tCel, int32 *pWidth,
|||	                         int32 *pHeight, char *fmtString, ...)
|||
|||	  Description
|||
|||	    Calculates the on-screen extent that would be used by the formatted text.
|||	    GetTextExtent formats text (if necessary), calculates the pixel width and
|||	    height required to display the text, and returns width/height values via
|||	    the pointers you provide.  The function return value is a pointer to the
|||	    formatted text (with printf-style formatting already resolved), a feature
|||	    that is mainly a convenience for the text library internals.
|||
|||	    This function does not take clipping into account; the returned width and
|||	    height may be larger than the current size of the text cel specified. As a
|||	    result, you can use this function to determine whether a cel needs to be
|||	    expanded by comparing the width and height it returns with the ccb_Width
|||	    and ccb_Height fields in the current cel.
|||
|||	    The related vGetTextExtent() function works exactly the same, but takes a
|||	    va_list type instead of the ... parameters.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    pWidth                       Pointer to the variable into which the width
|||	                                 is returned.
|||
|||	    pHeight                      Pointer to the variable into which the
|||	                                 height is returned.
|||
|||	    fmtString                    The string of characters to be rendered,
|||	                                 optionally including printf-style %
|||	                                 formatting commands.
|||
|||	    ...                          Arguments for printf-style formatting
|||	                                 (optional).
|||
|||	  Return Value
|||
|||	    A pointer to the formatted text.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
|||	  See Also
|||
|||	    vGetTextExtent()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gettimerioreq
|||	GetTimerIOReq - Get an IOReq for use with timer utilities.
|||
|||	  Synopsis
|||
|||	    Item GetTimerIOReq(void)
|||
|||	  Description
|||
|||	    This function obtains an IOReq item for use with the other timer utilties
|||	    that take an IOReq parm.  Use DeleteItem() to release the IOReq item when
|||	    you no longer need it.
|||
|||	    The timer-relatated functions which take an IOReq parm will perform much
|||	    faster if you allocate an IOReq using this function and pass it to the
|||	    other functions.  Letting the other functions create and delete an IOReq
|||	    on each call is appropriate only if you're planning to call the function
|||	    just a couple times throughout the run of your program.
|||
|||	  Return Value
|||
|||	    An IOReq item number, or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gettsectime
|||	GetTSecTime - Obtain the current time in tenths of a second.
|||
|||	  Synopsis
|||
|||	    int32 GetTSecTime(Item ioreq)
|||
|||	  Description
|||
|||	    This function obtains the current value of the system microsecond timer,
|||	    converts the seconds and microseconds into a single tenths-of-a-second
|||	    value, and returns the converted value.
|||
|||	    The return value will overflow an int32 representation after the machine
|||	    has been powered on for about 7 years.  Chances are good that this is safe
|||	    even in a cable TV or other long-running environment.
|||
|||	  Arguments
|||
|||	    ioreq                        An IOReq item obtained from GetTimerIOReq(),
|||	                                 or zero to have an IOReq dynamically
|||	                                 created/deleted during the call.
|||
|||	  Return Value
|||
|||	    Positive number of tenths-of-a-second or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    GetTimerIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/gettime
|||	GetTime - Obtain the current time in seconds.
|||
|||	  Synopsis
|||
|||	    int32 GetTime(Item ioreq)
|||
|||	  Description
|||
|||	    This function obtains the current value of the system microsecond timer
|||	    and returns just the seconds portion.
|||
|||	  Arguments
|||
|||	    ioreq                        An IOReq item obtained from GetTimerIOReq(),
|||	                                 or zero to have an IOReq dynamically
|||	                                 created/deleted during the call.
|||
|||	  Return Value
|||
|||	    Positive number of seconds or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    GetTimerIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/getvbltime
|||	GetVBLTime - Obtain the current VBL timer values.
|||
|||	  Synopsis
|||
|||	    int32 GetVBLTime(Item ioreq, uint32 *hiorder, uint32 *loworder)
|||
|||	  Description
|||
|||	    This function obtains the current value of the system VBL timer and stores
|||	    the results at *hiorder and *loworder.  Either pointer can be NULL if you
|||	    don't need that part of the result.  The low-order part of the VBL timer
|||	    is generally of the most interest; it doesn't roll over into the
|||	    high-order part until after roughly 800 days of continuous running without
|||	    a reboot.
|||
|||	  Arguments
|||
|||	    ioreq                        An IOReq item obtained from GetTimerIOReq(),
|||	                                 or zero to have an IOReq dynamically
|||	                                 created/deleted during the call.
|||
|||	    hiorder                      A pointer to a variable into which the
|||	                                 high-order portion of the VBL time is
|||	                                 stored.
|||
|||	    loworder                     A pointer to a variable into which the
|||	                                 low-order portion of the VBL time is stored.
|||
|||	  Return Value
|||
|||	    The low-order portion of the VBL time (a positive value), or a negative
|||	    error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    GetTimerIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/getusectime
|||	GetUSecTime - Obtain the current time in microseconds.
|||
|||	  Synopsis
|||
|||	    int32 GetUSecTime(Item ioreq, uint32 *seconds, uint32 *useconds)
|||
|||	  Description
|||
|||	    This function obtains the current value of the system microsecond timer
|||	    and stores the results at *seconds and *useconds.  Either pointer can be
|||	    NULL if you don't need that part of the result.
|||
|||	  Arguments
|||
|||	    ioreq                        An IOReq item obtained from GetTimerIOReq(),
|||	                                 or zero to have an IOReq dynamically
|||	                                 created/deleted during the call.
|||
|||	    seconds                      A pointer to a variable into which the
|||	                                 high-order (full seconds) portion of the
|||	                                 time is stored
|||
|||	    useconds                     A pointer to a variable into which the
|||	                                 low-order (microseconds) portion of the time
|||	                                 is stored.
|||
|||	  Return Value
|||
|||	    The high-order portion of the time (full seconds, a positive value), or a
|||	    negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    GetTimerIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/hz_to_usec
|||	HZ_TO_USEC - Convert hertz to microseconds.
|||
|||	  Synopsis
|||
|||	    int32 HZ_TO_USEC(value)
|||
|||	  Description
|||
|||	    This macro converts a value expressed in hertz (cycles or ticks per
|||	    second) to the equivelent number of microseconds for each tick.  True
|||	    floating point constant values are legal, for example HZ_TO_USEC(2.5).
|||	    The compiler will resolve the constant at compile time without needing
|||	    runtime floating point support.  The macro is usable in expression context
|||	    and only evaluates its argument once.
|||
|||	  Arguments
|||
|||	    value                        An integer constant or variable, or a
|||	                                 floating point constant to be converted to
|||	                                 microseconds.
|||
|||	  Return Value
|||
|||	    The number of microseconds.
|||
|||	  Implementation
|||
|||	    Macro
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/icornerfromsrect
|||	ICornerFromSRect - Calculate the opposite corner in an SRect.
|||
|||	  Synopsis
|||
|||	    IPoint * ICornerFromSRect(IPoint *dst, SRect *src)
|||
|||	  Description
|||
|||	    This function calculates the opposite corner of the area described by the
|||	    src rectangle, and stores the results at *dst.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the IPoint where the results
|||	                                 are to be stored.
|||
|||	    src                          A pointer to an SRect.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    XCORNERFROMSRECT(), YCORNERFROMSRECT(), ISizeFromCRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/ipointfromfpoint
|||	IPointFromFPoint - Convert an FPoint to an IPoint.
|||
|||	  Synopsis
|||
|||	    IPoint * IPointFromFPoint(IPoint *dst, FPoint *src)
|||
|||	  Description
|||
|||	    This function converts an FPoint to an IPoint.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the IPoint where the results
|||	                                 are to be stored.
|||
|||	    src                          A pointer to the FPoint to be converted.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    FPointFromIPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/ipointfromfval
|||	IPointFromFVal - Create an IPoint from inline values.
|||
|||	  Synopsis
|||
|||	    IPoint * IPointFromFVal(IPoint *dst, frac16 x, frac16 y)
|||
|||	  Description
|||
|||	    This functions converts the specified values from frac16 to integer and
|||	    stores them at *dst.  It is useful for writing inline calls such as:
|||
|||	    IPoint      wrk;
|||	    OffsetCelListByIDelta (cel, IPointFromFVal (&wrk, my_x, my_y));
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the IPoint where the results
|||	                                 are to be stored.
|||
|||	    x                            The value to be converted into dst->x.
|||
|||	    y                            The value to be converted into dst->y.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    IPointFromIVal(), FPointFromFVal()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/ipointfromival
|||	IPointFromIVal - Create an IPoint from inline values.
|||
|||	  Synopsis
|||
|||	    IPoint * IPointFromIVal(IPoint *dst, int32 x, int32 y)
|||
|||	  Description
|||
|||	    This functions stores the specified values at *dst.  It is useful for
|||	    writing inline calls such as:
|||
|||	    IPoint    wrk;
|||	    OffsetCelListByIDelta (cel, IPointFromFVal(&wrk, my_x, my_y));
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the IPoint where the results
|||	                                 are to be stored.
|||
|||	    x                            The value to be stored into dst->x.
|||
|||	    y                            The value to be stored into dst->y.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    IPointFromFVal(), FPointFromIVal()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/ipointisincrect
|||	IPointIsInCRect - Determine whether a point is within a rectangle.
|||
|||	  Synopsis
|||
|||	    Boolean IPointIsInCRect(IPoint *point, CRect *rect)
|||
|||	  Description
|||
|||	    This function determines whether the point is within the rectangle.
|||	    Points on the edge are considered within the rectange.
|||
|||	  Arguments
|||
|||	    point                        A pointer to an IPoint.
|||
|||	    rect                         A pointer to a CRect.
|||
|||	  Return Value
|||
|||	    TRUE if the point is in the rectangle, FALSE if it is not.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    IPointIsInSRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/ipointisinsrect
|||	IPointIsInSRect - Determine whether a point is within a rectangle.
|||
|||	  Synopsis
|||
|||	    Boolean IPointIsInSRect(IPoint *point, SRect *rect)
|||
|||	  Description
|||
|||	    This function determines whether the point is within the rectangle.
|||	    Points on the edge are considered within the rectange.
|||
|||	  Arguments
|||
|||	    point                        A pointer to an IPoint.
|||
|||	    rect                         A pointer to an SRect.
|||
|||	  Return Value
|||
|||	    TRUE if the point is in the rectangle, FALSE if it is not.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    IPointIsInCRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/is_lastcel
|||	IS_LASTCEL - Determine whether a cel is the last in a list.
|||
|||	  Synopsis
|||
|||	    Boolean IS_LASTCEL(CCB *ccb)
|||
|||	  Description
|||
|||	    This macro evaluates to TRUE if no cels are linked to ccb, or FALSE if
|||	    other cels are linked to ccb.
|||
|||	    This macro evaluates its argument more than once; be careful of side
|||	    effects.   It is usable in expression context.
|||
|||	  Arguments
|||
|||	    ccb                          A pointer to a cel.
|||
|||	  Return Value
|||
|||	    TRUE if no cels are linked to this one; FALSE if cels are linked to this
|||	    one.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    LastCelInList()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/isizefromcrect
|||	ISizeFromCRect - Calculate the size of a CRect.
|||
|||	  Synopsis
|||
|||	    IPoint * ISizeFromCRect(IPoint *dst, CRect *src)
|||
|||	  Description
|||
|||	    This function calculates the width and height of the area described by the
|||	    src rectangle and stores the results at *dst.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the IPoint where the results
|||	                                 are to be stored.
|||
|||	    src                          A pointer to a CRECT.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    XSIZEFROMCRECT(), YSIZEFROMCRECT(), ICornerFromSRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/insetcrect
|||	InsetCRect - Shrink or enlarge a CRect.
|||
|||	  Synopsis
|||
|||	    CRect * InsetCRect(CRect *dst, IPoint *delta)
|||
|||	  Description
|||
|||	    This function shrinks or enlarges the amount of area described by the dst
|||	    rectangle.  Positive delta values shrink the area, negative values enlarge
|||	    the area.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the CRect to manipulate.
|||
|||	    delta                        A pointer to the delta values to apply to
|||	                                 dst.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    InsetSRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/insetsrect
|||	InsetSRect - Shrink or enlarge an SRect.
|||
|||	  Synopsis
|||
|||	    SRect * InsetSRect(SRect *dst, IPoint *delta)
|||
|||	  Description
|||
|||	    This function shrinks or enlarges the amount of area described by the dst
|||	    rectangle.  Positive delta values shrink the area, negative values enlarge
|||	    the area.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the SRect to manipulate.
|||
|||	    delta                        A pointer to the delta values to apply to
|||	                                 dst.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    InsetCRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/lastcelinlist
|||	LastCelInList - Find the last cel in a list.
|||
|||	  Synopsis
|||
|||	    CCB * LastCelInList(CCB *list)
|||
|||	  Description
|||
|||	    This function returns a pointer to the last cel in a list of one or more
|||	    cels.
|||
|||	  Arguments
|||
|||	    list                         A pointer to a list of cels; may be NULL.
|||
|||	  Return Value
|||
|||	    A pointer to the last cel in the list, or NULL if the list pointer is
|||	    NULL.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    IS_LASTCEL()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/linkcel
|||	LinkCel - Links two cels
|||
|||	  Synopsis
|||
|||	    void LinkCel( CCB *ccb, CCB *nextCCB )
|||
|||	  Description
|||
|||	    Links nextCCB to ccb by setting ccb's NextPtr field to nextCCB turning
|||	    off the CCB_LAST flag in ccb's ccb_Flags field by setting CCB_LAST to
|||	    NULL
|||
|||	    This function does not splice nextCCB into an existing list. It only
|||	    replaces value of ccb_NextPtr in ccb to point to nextCCB.
|||
|||	  Arguments
|||
|||	    ccb                          Pointer to the CCB being linked.
|||
|||	    nextCCB                      Pointer to the CCB being added to the list.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/loadanim
|||	LoadAnim - Load an animation
|||
|||	  Synopsis
|||
|||	    ANIM * LoadAnim (char *fileName, uint32 memTypeBits)
|||
|||	  Description
|||
|||	    Loads an animation file from disk. Parses the cels within the animation
|||	    into an ANIM structure and returns a pointer to the ANIM.
|||
|||	    The animation file can be single-CCB or multi-CCB format. An optional ANIM
|||	    chunk may appear at the start of the file. The file can contain pixel and
|||	    color chunks in either PDAT, PLUT or PLUT, PDAT order, but the chunks must
|||	    appear in the same order throughout the file. (3DO Animator creates PLUT,
|||	    PDAT files; PDAT, PLUT files result from concatenating cel files using
|||	    MPW.)
|||
|||	    When you are finished with the animation, use UnloadAnim() to free all
|||	    resources acquired by LoadAnim() for a given animation file.
|||
|||	    The frames within an animation are allocated incrementally. Initially a
|||	    block large enough to hold N_FRAMES_PER_CHUNK frames is allocated. If this
|||	    is exceeded while reading the ANIM then a block with space for
|||	    N_FRAMES_PER_CHUNK additional frames is allocated, the previously computed
|||	    frames are copied into this new space, and the old full buffer space is
|||	    deallocated
|||
|||	    The following structures are used by LoadAnim().
|||
|||	                typedef struct tag_AnimFrame {
|||	                        CCB             *af_CCB;// Pointer to CCB for this frame
|||	                        char    *af_PLUT;       // Pointer to PLUT for this frame
|||	                        char    *af_pix;        // Pointer to pixels for this frame
|||	                        int32   reserved;
|||	                } AnimFrame;
|||
|||	                typedef struct tag_ANIM {
|||	                        long    num_Frames;     // max number of PDATs or CCBs in file
|||	                        frac16  cur_Frame;      // allows fract values for smooth speed
|||	                        long    num_Alloced_Frames;
|||	                        AnimFrame *pentries;
|||	                } ANIM;
|||
|||	    The frames within an ANIM are allocated incrementally. Initially a block
|||	    large enough to hold N_FRAMES_PER_CHUNK frames is allocated. If this is
|||	    exceeded while reading the ANIM then a block with space for
|||	    N_FRAMES_PER_CHUNK additional AnimFrames is allocated, the previously
|||	    computed frames are copied into this new space, and the old full AnimFrame
|||	     buffer space is deallocated
|||
|||	  Arguments
|||
|||	    fileName                     Pointer to the file name string.
|||
|||	    memTypeBits                  Type of memory to use when loading the file;
|||	                                 usually MEMTYPE_ANY.
|||
|||	  Return Value
|||
|||	    A pointer to an ANIM structure, or NULL if an error occurred.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, animutils.h
|||
|||	  See Also
|||
|||	    UnloadAnim(), GetAnimCel(), DrawAnimCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/loadcel
|||	LoadCel  - Load one or more cels from a file.
|||
|||	  Synopsis
|||
|||	    CCB * LoadCel (char *filename, uint32 memTypeBits)
|||
|||	  Description
|||
|||	    This function loads a cel or related group of  cels from a file.  Memory
|||	    for the file is allocated according to memTypeBits, then the entire file
|||	    is loaded into the buffer using fast block I/O.  After the file is loaded,
|||	    the cel(s) in the buffer are parsed by the ParseCel() function.
|||
|||	    If the file contains more than one cel, the CCBs are strung together via
|||	    their ccb_NextPtr fields in the same order as they are encountered in the
|||	    buffer.  This allows you to store a group of related cels in one file, and
|||	    treat them as a single logical entity using the other CelUtils library
|||	    functions that work with lists of cels.
|||
|||	    This function also contains special handling for anti-aliased cels.
|||	    AACels are two cels stored together in the same file, but the CCBs are
|||	    stored in reverse order of the way they must be drawn.  AACels are
|||	    recognized by a characteristic signature (exactly two cels, one is 4-bit
|||	    coded the other 16-bit uncoded, same size and coordinates, etc), and when
|||	    detected the CCB links are automatically reversed so the return value is a
|||	    list of two cels linked in the proper order for drawing.  One drawback to
|||	    the signature-detection logic is that it prevents the storage of multiple
|||	    related anti-aliased cels in the same file.
|||
|||	    Use DeleteCel() or UnloadCel() to release the resources acquired when the
|||	    cel file is loaded.  When the file contains several cels, or an
|||	    anti-aliased cel, use DeleteCel() or UnloadCel() only on the first cel in
|||	    the file.  That is, pass the pointer returned by this function.
|||	    DeleteCel() will delete all the cels at once.
|||
|||	    DeleteCelList() contains special logic to treat a group of cels loaded
|||	    from the same file as if they were one cel.  If more cels were linked in
|||	    the list following a group loaded from the same file, DeleteCelList()
|||	    calls DeleteCel() for the first cel in the group, then skips to the end of
|||	    the group and resumes walking the ccb_NextPtr links as normal for the rest
|||	    of the cels.  This implies that you cannot load a group of cels from a
|||	    file and then break apart the links built by LoadCel() and relink the cels
|||	    into different lists.  Doing so would confuse the logic in
|||	    DeleteCelList().
|||
|||	  Arguments
|||
|||	    filename                     desc
|||
|||	    memTypeBits                  desc
|||
|||	  Return Value
|||
|||	    A pointer to the cel or list of cels loaded from the file.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    ParseCel(), DeleteCel, DeleteCelList, UnloadCel
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/loadfile
|||	LoadFile - Loads a cel file from disk
|||
|||	  Synopsis
|||
|||	    void * LoadFile (char *fileName, long *fileSizePtr,
|||	                     uint32 memTypeBits)
|||
|||	  Description
|||
|||	    Loads an entire file into a buffer in memory. This function allocates a
|||	    buffer, loads the file into it, and returns a pointer to the allocated
|||	    buffer. The size of the file is returned into a longword via the
|||	    fileSizePtr parameter.
|||
|||	    Note that the buffer is allocated to an even multiple of the device's
|||	    blocksize (2 KB for CD-ROM and the Mac link), and thus may be slightly
|||	    larger than the actual size of the file. The file size returned via
|||	    fileSizePtr, however, reflects the actual size of the file without the
|||	    extra padding required to achieve an integral device block size.
|||
|||	    This function is approximately 3-4 times as fast as the old method of
|||	    loading a file using GetFileSize(), allocating a buffer, calling
|||	    ReadFile(). When you are finished with the file, use UnloadFile() to free
|||	    the file buffer.
|||
|||	    If an error returns, the function return value is NULL and the longword at
|||	    the *fileSizePtr contains the error status. A return value of NULL with an
|||	    error status of zero indicates that the file exists but contains no data;
|||	    in this cas, no buffer was allocated.
|||
|||	  Arguments
|||
|||	    fileName                     Pointer to the file name string.
|||
|||	    fileSizePtr                  Pointer to a longword which receives the
|||	                                 length of file (in bytes). If you don\xd5 t
|||	                                 need the length, a NULL pointer can be
|||	                                 passed for this parameter.
|||
|||	    memTypeBits                  Type of memory to use when loading the file;
|||	                                 usually MEMTYPE_ANY or MEMTYPE_VRAM.
|||
|||	  Return Value
|||
|||	    A pointer to loaded file in memory, or NULL if an error occurred.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, blockfile.h
|||
|||	  See Also
|||
|||	    UnloadFile(), LoadFileHere(), AsyncLoadFile()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/loadfilehere
|||	LoadFileHere - Load a file into a pre-allocated buffer.
|||
|||	  Synopsis
|||
|||	    void * LoadFileHere(char *fname, int32 *pfsize,
|||	                        void *buffer, int32 bufsize)
|||
|||	  Description
|||
|||	    This function loads a file into a buffer you allocate.  It is identical to
|||	    LoadFile() except that it lets you allocate the buffer.  If the bufsize
|||	    parameter is non-zero, this function checks to ensure that the file will
|||	    fit in the specified buffer.  The actual size of the file, which may be
|||	    smaller than the buffer, is stored at *pfsize.  If an error occurs the
|||	    word at *pfsize will contain a negative error code.
|||
|||	    When allocating a buffer, remember that the buffer has to be an integral
|||	    multiple of the file device's blocksize.  That is, loading a 700 byte file
|||	    from a CD still requires a 2k buffer.  A useful space-saving technique
|||	    involves loading a small file into the inactive screen buffer in VRAM then
|||	    copying it (or perhaps just a small portion of it that you need) to a
|||	    right-sized buffer elsewhere in memory.
|||
|||	  Arguments
|||
|||	    fname                        The name of the file to load.
|||
|||	    pfsize                       A pointer to a word which, after the call,
|||	                                 holds the actual size of the file.
|||
|||	    buffer                       A pointer to a buffer large enough to hold
|||	                                 the file.
|||
|||	    bufsize                      The size of the buffer, or zero to disable
|||	                                 overflow checking.
|||
|||	  Return Value
|||
|||	    Returns a pointer to the buffer, or NULL on error.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, blockfile.h
|||
|||	  See Also
|||
|||	    LoadFile(), AsyncLoadFile()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/loadfont
|||	LoadFont - Load 3DO font file.
|||
|||	  Synopsis
|||
|||	    FontDescriptor * LoadFont(char *fileName, uint32 memTypeBits)
|||
|||	  Description
|||
|||	    Loads a font from the specified file and prepares it for use.
|||
|||	    Use UnloadFont() to release all resources acquired by LoadFont().
|||
|||	  Arguments
|||
|||	    fileName                     The font file to load.
|||
|||	    memTypeBits                  The type of memory to load the font into
|||	                                 (usually MEMTYPE_ANY).
|||
|||	  Return Value
|||
|||	    A pointer to a FontDescriptor structure that describes the font; returns
|||	    NULL if the file is not found, is not a valid font file, or there is not
|||	    enough memory to process the file.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, fontlib.h
|||
|||	  See Also
|||
|||	    UnloadFont(), ParseFont()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/loadimage
|||	LoadImage - Loads an image
|||
|||	  Synopsis
|||
|||	    ubyte * LoadImage (char *name, ubyte *dest, VdlChunk **rawVDLPtr,
|||	                       ScreenContext *sc)
|||
|||	  Description
|||
|||	    Loads a 3DO file format image file specified by name into the buffer
|||	    pointed to by dest. May optionally return a VDL (Video Display List) if
|||	    the image file specifies per line CLUTs (Color Lookup Tables).
|||
|||	  Arguments
|||
|||	    name                         Pointer to char string containing a path
|||	                                 name of an image file in 3DO file format.
|||
|||	    dest                         Pointer to destination buffer. If dest is
|||	                                 NULL, a page aligned VRAM buffer large
|||	                                 enough to hold an image is allocated. If
|||	                                 dest is not NULL, it is assumed to point to
|||	                                 a page aligned VRAM buffer large enough to
|||	                                 hold a screen image.
|||
|||	    rawVDLPtr                    Pointer to a pointer to a VDL structure.
|||	                                 LoadImage() fills this structure if the
|||	                                 image file contains a custom VDL .
|||
|||	    sc                           Pointer to a screen context.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, displayutils.h
|||
|||	  Return Value
|||
|||	    Returns a pointer to the destination buffer if successful, NULL if not.
|||	    Note that the returned buffer may have been allocated by LoadImage() if
|||	    the dest input parameter was NULL.
|||
|||	  See Also
|||
|||	    UnloadImage()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/mapaaceltocquad
|||	MapAACelToCQuad - Map an anti-aliased cel to a quad.
|||
|||	  Synopsis
|||
|||	    void MapAACelToCQuad(CCB *aacel, CQuad *quad)
|||
|||	  Description
|||
|||	    This function maps an anti-aliased cel to the specified quad.  It is
|||	    effectively a standard MapCel() call that stores its results into both the
|||	    data and alpha-channel CCBs.  It also works correctly if aacel points to a
|||	    single standalone cel.
|||
|||	  Arguments
|||
|||	    aacel                        A pointer to the cel to be mapped.
|||
|||	    quad                         A pointer to the CQuad to which the cel is
|||	                                 to be mapped.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    MapCelToCQuad(), MapCelListToCQuad()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/mapaaceltocrect
|||	MapAACelToCRect - Map an anti-aliased cel to a CRect.
|||
|||	  Synopsis
|||
|||	    void MapAACelToCRect(CCB *aacel, CRect *rect)
|||
|||	  Description
|||
|||	    This function maps an anti-aliased cel to the specified rectangle.  It is
|||	    much faster than a standard MapCel() call because it only has to calculate
|||	    a simple rectangular projection.  It stores its results into both the data
|||	    and alpha-channel CCBs.  It also works correctly if aacel points to a
|||	    single standalone cel.
|||
|||	  Arguments
|||
|||	    aacel                        A pointer to the cel to be mapped.
|||
|||	    rect                         A pointer to the CRect to which the cel is
|||	                                 to be mapped.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    MapAACelToSRect(), MapCelToCRect(), MapCelListToCRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/mapaaceltofpoint
|||	MapAACelToFPoint - Set an anti-aliased cel's location to a point.
|||
|||	  Synopsis
|||
|||	    void MapAACelToFPoint(CCB *aacel, FPoint *newPosition)
|||
|||	  Description
|||
|||	    This function sets an anti-aliased cel's location to the given
|||	    coordinates.  It stores the coordinates into both the data and
|||	    alpha-channel CCBs.  It also works correctly if aacel points to a single
|||	    standalone cel.  Only the ccb_XPos and ccb_YPos values in the CCB are
|||	    modified; other CCB fields are not touched.
|||
|||	  Arguments
|||
|||	    aacel                        A pointer to the cel to be mapped.
|||
|||	    newPosition                  A pointer to the FPoint to which the cel is
|||	                                 to be mapped.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    MapAACelToIPoint(), MapCelToFPoint(), MapCelListToFPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/mapaaceltoipoint
|||	MapAACelToIPoint - Set an anti-aliased cel's location to a point.
|||
|||	  Synopsis
|||
|||	    void MapAACelToIPoint(CCB *aacel, IPoint *newPosition)
|||
|||	  Description
|||
|||	    This function sets an anti-aliased cel's location to the given
|||	    coordinates.  It stores the coordinates into both the data and
|||	    alpha-channel CCBs.  It also works correctly if aacel points to a single
|||	    standalone cel.  Only the ccb_XPos and ccb_YPos values in the CCB are
|||	    modified; other CCB fields are not touched.
|||
|||	  Arguments
|||
|||	    aacel                        A pointer to the cel to be mapped.
|||
|||	    newPosition                  A pointer to the FPoint to which the cel is
|||	                                 to be mapped.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    MapAACelToFPoint(), MapCelToIPoint(), MapCelListToIPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/mapaaceltosrect
|||	MapAACelToSRect - Map an anti-aliased cel to an SRect.
|||
|||	  Synopsis
|||
|||	    void MapAACelToSRect(CCB *aacel, SRect *rect)
|||
|||	  Description
|||
|||	    This function maps an anti-aliased cel to the specified rectangle.  It is
|||	    much faster than a standard MapCel() call because it only has to calculate
|||	    a simple rectangular projection.  It stores its results into both the data
|||	    and alpha-channel CCBs.  It also works correctly if aacel points to a
|||	    single standalone cel.
|||
|||	  Arguments
|||
|||	    aacel                        A pointer to the cel to be mapped.
|||
|||	    rect                         A pointer to the SRect to which the cel is
|||	                                 to be mapped.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    MapAACelToCRect(), MapCelToSRect(), MapCelListToSRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/mapcellisttocquad
|||	MapCelListToCQuad - Map a list of cels to a quad.
|||
|||	  Synopsis
|||
|||	    void MapCelListToCQuad(CCB *list, CQuad *quad)
|||
|||	  Description
|||
|||	    This function maps a list of cels to the specified quad.  It performs a
|||	    standard MapCel() on the first cel in the list.  It then adjusts the X/Y
|||	    coordinates of the rest of the cels in the list such that all the cels
|||	    maintain the same relative X/Y relationships to each other.  It also
|||	    copies the size and perspective fields from the first cel into all the
|||	    rest of the cels in the list.  The net effect is that the list of cels
|||	    moves as if it were a single unit, although the operation doesn't include
|||	    true 3D parallax calculations.
|||
|||	  Arguments
|||
|||	    list                         A pointer to a list of one or more cels to
|||	                                 be mapped.
|||
|||	    quad                         A pointer to the CQuad to which the cels are
|||	                                 to be mapped.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    MapAACelToCQuad(), MapCelToCQuad()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/mapcellisttocrect
|||	MapCelListToCRect - Map a list of cels to a CRect.
|||
|||	  Synopsis
|||
|||	    void MapCelListToCRect(CCB *list, CRect *rect)
|||
|||	  Description
|||
|||	    This function maps a list of cels to the specified rectangle.  It is much
|||	    faster than a standard MapCel() call because it only has to calculate a
|||	    simple rectangular projection.  It performs a MapCelToCRect() on the first
|||	    cel in the list.  It then adjusts the X/Y coordinates of the rest of the
|||	    cels in the list such that all the cels maintain the same relative X/Y
|||	    relationships to each other.  It also copies the size and perspective
|||	    fields from the first cel into all the rest of the cels in the list.  The
|||	    net effect is that the list of cels moves and scales as if it were a
|||	    single unit.
|||
|||	  Arguments
|||
|||	    list                         A pointer to a list of one or more cels to
|||	                                 be mapped.
|||
|||	    rect                         A pointer to the CRect to which the cels are
|||	                                 to be mapped.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    MapAACelToCRect(), MapCelToCRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/mapcellisttofpoint
|||	MapCelListToFPoint - Set a list of cels' locations to a point.
|||
|||	  Synopsis
|||
|||	    void MapCelListToFPoint(CCB *list, FPoint *newPosition,
|||	                            Boolean copyPerspective)
|||
|||	  Description
|||
|||	    This function sets a list of cels to the given coordinates.  It sets the
|||	    X/Y coordinates of the first cel to the specified values.  It then adjusts
|||	    the X/Y coordinates of the rest of the cels in the list such that the cels
|||	    maintain the same relative X/Y relationships to each other.  The net
|||	    effect is that the list of cels moves as if it were a single unit.
|||
|||	    This function can optionally propagate the size and perspective values
|||	    already present in the first cel to the rest of the cels in the list.
|||	    This is useful if you are manipulating size and perspective values
|||	    manually.  To propagate the size and perspective without changing the
|||	    positions, use one of the OffsetCelList() functions with delta values of
|||	    zero.
|||
|||	  Arguments
|||
|||	    list                         A pointer to a list of one or more cels to
|||	                                 be mapped.
|||
|||	    newPosition                  A pointer to the FPoint to which the cel is
|||	                                 to be mapped.
|||
|||	    copyPerspective              TRUE to copy the size and perspective fields
|||	                                 from the first cel to the rest of the list.
|||	                                 FALSE to leave all size and perspective
|||	                                 fields as is.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    MapCelListToIPoint(), MapAACelToFPoint(), MapCelToFPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/mapcellisttoipoint
|||	MapCelListToIPoint - Set a list of cels' locations to a point.
|||
|||	  Synopsis
|||
|||	    void MapCelListToIPoint(CCB *list, IPoint *newPosition,
|||	                            Boolean copyPerspective)
|||
|||	  Description
|||
|||	    This function sets a list of cels to the given coordinates.  It then
|||	    adjusts the X/Y coordinates of the rest of the cels in the list such that
|||	    the cels maintain the same relative X/Y relationships to each other.  The
|||	    net effect is that the list of cels moves as if it were a single unit.
|||
|||	    This function can optionally propagate the size and perspective values
|||	    already present in the first cel to the rest of the cels in the list.
|||	    This is useful if you are manipulating size and perspective values
|||	    manually.  To propagate the size and perspective without changing the
|||	    positions, use one of the OffsetCelList() functions with delta values of
|||	    zero.
|||
|||	  Arguments
|||
|||	    list                         A pointer to a list of one or more cels to
|||	                                 be mapped.
|||
|||	    newPosition                  A pointer to the FPoint to which the cel is
|||	                                 to be mapped.
|||
|||	    copyPerspective              TRUE to copy the size and perspective fields
|||	                                 from the first cel to the rest of the list.
|||	                                 FALSE to leave all size and perspective
|||	                                 fields as is.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    MapCelListToFPoint(), MapAACelToIPoint(), MapCelToIPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/mapcellisttosrect
|||	MapCelListToSRect - Map a list of cels to a CRect.
|||
|||	  Synopsis
|||
|||	    void MapCelListToSRect(CCB *list, SRect *rect)
|||
|||	  Description
|||
|||	    This function maps a list of cels to the specified rectangle.  It is much
|||	    faster than a standard MapCel() call because it only has to calculate a
|||	    simple rectangular projection.  It performs a MapCelToCRect() on the first
|||	    cel in the list.  It then adjusts the X/Y coordinates of the rest of the
|||	    cels in the list such that all the cels maintain the same relative X/Y
|||	    relationships to each other.  It also copies the size and perspective
|||	    fields from the first cel into all the rest of the cels in the list.  The
|||	    net effect is that the list of cels moves and scales as if it were a
|||	    single unit.
|||
|||	  Arguments
|||
|||	    list                         A pointer to a list of one or more cels to
|||	                                 be mapped.
|||
|||	    rect                         A pointer to the CRect to which the cels are
|||	                                 to be mapped.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    MapCelListToCRect(), MapAACelToSRect(), MapCelToSRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/mapceltocquad
|||	MapCelToCQuad - Map a cel to a quad.
|||
|||	  Synopsis
|||
|||	    void MapCelToCQuad(CCB *cel, CQuad *quad)
|||
|||	  Description
|||
|||	    This function performs a standard MapCel() on a single cel.  It differs
|||	    from the standard MapCel() call only in that it accepts a CQuad* datatype.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel to be mapped.
|||
|||	    quad                         A pointer to the CQuad to which the cel is
|||	                                 to be mapped.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    MapAACelToCQuad(), MapCelListToCQuad()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/mapceltocrect
|||	MapCelToCRect - Map a cel to a CRect.
|||
|||	  Synopsis
|||
|||	    void MapCelToCRect(CCB *cel, CRect *rect)
|||
|||	  Description
|||
|||	    This function maps a single cel to the specified rectangle.  It is much
|||	    faster than a standard MapCel() call because it only has to calculate a
|||	    simple rectangular projection.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel to be mapped.
|||
|||	    rect                         A pointer to the CRect to which the cel is
|||	                                 to be mapped.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    MapCelToSRect(), MapAACelToCRect(), MapCelListToCRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/mapceltofpoint
|||	MapCelToFPoint - Set a cel's location to a point.
|||
|||	  Synopsis
|||
|||	    void MapCelToFPoint(CCB *cel, FPoint *newPosition)
|||
|||	  Description
|||
|||	    This function sets a single cel to the specified coordinates.  It simply
|||	    stores the coordinates in the CCB X/Y position fields.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel to be mapped.
|||
|||	    newPosition                  A pointer to the FPoint to which the cel is
|||	                                 to be mapped.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    MapCelToIPoint(), MapAACelToFPoint(), MapCelListToFPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/mapceltoipoint
|||	MapCelToIPoint - Set a cel's location to a point.
|||
|||	  Synopsis
|||
|||	    void MapCelToIPoint(CCB *cel, IPoint *newPosition)
|||
|||	  Description
|||
|||	    This function sets a single cel to the specified coordinates.  It converts
|||	    the coordinates to frac16 and stores them in the CCB X/Y position fields.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel to be mapped.
|||
|||	    newPosition                  A pointer to the FPoint to which the cel is
|||	                                 to be mapped.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    MapCelToFPoint(), MapAACelToIPoint(), MapCelListToIPoint()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/mapceltosrect
|||	MapCelToSRect - Map a cel to an SRect.
|||
|||	  Synopsis
|||
|||	    void MapCelToSRect(CCB *cel, SRect *rect)
|||
|||	  Description
|||
|||	    This function maps a single cel to the specified rectangle.  It is much
|||	    faster than a standard MapCel() call because it only has to calculate a
|||	    simple rectangular projection.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel to be mapped.
|||
|||	    rect                         A pointer to the CRect to which the cel is
|||	                                 to be mapped.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    MapCelToCRect(), MapAACelToSRect(), MapCelListToSRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/msec_to_usec
|||	MSEC_TO_USEC - Convert milliseconds to microseconds.
|||
|||	  Synopsis
|||
|||	    int32 MSEC_TO_USEC(value)
|||
|||	  Description
|||
|||	    This macro converts a number of milliseconds to the equivelent number of
|||	    microseconds.  True floating point constant values are legal, for example
|||	    MSEC_TO_USEC(2730.52).  The compiler will resolve the constant at compile
|||	    time without needing runtime floating point support.  Values up to
|||	    2,147,000 milliseconds (about 35 minutes) can be expressed without
|||	    overflowing an int32.  The macro is usable in expression context and only
|||	    evaluates its argument once.
|||
|||	  Arguments
|||
|||	    value                        An integer constant or variable, or a
|||	                                 floating point constant to be converted to
|||	                                 microseconds.
|||
|||	  Return Value
|||
|||	    The number of microseconds.
|||
|||	  Implementation
|||
|||	    Macro
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/offsetaacelbyfdelta
|||	OffsetAACelByFDelta - Move an anti-aliased cel.
|||
|||	  Synopsis
|||
|||	    void OffsetAACelByFDelta(CCB *aacel, FPoint *deltaXY)
|||
|||	  Description
|||
|||	    This function adjusts an anti-aliased cel's location by the specified
|||	    delta.  It adds the delta values to the X/Y coordinates in both the data
|||	    and alpha-channel CCBs.  It also works correctly if aacel points to a
|||	    single standalone cel.
|||
|||	  Arguments
|||
|||	    aacel                        A pointer to the cel to be moved.
|||
|||	    deltaXY                      A point to an FPoint containing the movement
|||	                                 delta values.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    OffsetAACelByIDelta(), OffsetCelByFDelta(), OffsetCelListByFDelta()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/offsetaacelbyidelta
|||	OffsetAACelByIDelta - Move an anti-aliased cel.
|||
|||	  Synopsis
|||
|||	    void OffsetAACelByIDelta(CCB *aacel, IPoint *deltaXY)
|||
|||	  Description
|||
|||	    This function adjusts an anti-aliased cel's location by the specified
|||	    delta.  It adds the delta values to the X/Y coordinates in both the data
|||	    and alpha-channel CCBs.  It also works correctly if aacel points to a
|||	    single standalone cel.
|||
|||	  Arguments
|||
|||	    aacel                        A pointer to the cel to be moved.
|||
|||	    deltaXY                      A pointer to an IPoint containing the
|||	                                 movement delta values.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    OffsetAACelByFDelta(), OffsetCelByIDelta(), OffsetCelListByIDelta()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/offsetcel
|||	OffsetCel - Add offsets to a CCB's position fields
|||
|||	  Synopsis
|||
|||	    void OffsetCel (CCB *ccb, int32 xOffset, int32 yOffset)
|||
|||	  Description
|||
|||	    Adds the specified offsets to the position fields of the CCB identified by
|||	    ccb.
|||
|||	  Arguments
|||
|||	    ccb                          Pointer to the CCB of the cel.
|||
|||	    xOffset                      Integer x offset.
|||
|||	    yOffset                      Integer y offset.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/offsetcrect
|||	OffsetCRect - Move a rectangle.
|||
|||	  Synopsis
|||
|||	    CRect * OffsetCRect(CRect *dst, IPoint *deltaXY)
|||
|||	  Description
|||
|||	    This function adjusts the location described by the dst rectangle by
|||	    adding the specified delta values to the rectangle's corner values.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the CRect to move.
|||
|||	    deltaXY                      A pointer to an IPoint containing the
|||	                                 movement delta values.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    OffsetSRect(), InsetCRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/offsetcelbyfdelta
|||	OffsetCelByFDelta - Move a cel.
|||
|||	  Synopsis
|||
|||	    void OffsetCelByFDelta(CCB *cel, FPoint *deltaXY)
|||
|||	  Description
|||
|||	    This function adjusts a single cel's location by the specified delta.  It
|||	    adds the delta values to the X/Y coordinates in the CCB.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel to be moved.
|||
|||	    deltaXY                      A pointer to an FPoint containing the
|||	                                 movement delta values.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    OffsetCelByIDelta(), OffsetAACelByFDelta(), OffsetCelListByFDelta()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/offsetcelbyidelta
|||	OffsetCelByIDelta - Move a cel.
|||
|||	  Synopsis
|||
|||	    void OffsetCelByIDelta(CCB *cel, IPoint *deltaXY)
|||
|||	  Description
|||
|||	    This function adjusts a single cel's location by the specified delta.  It
|||	    adds the delta values to the X/Y coordinates in the CCB.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel to be moved.
|||
|||	    deltaXY                      A pointer to an IPoint containing the
|||	                                 movement delta values.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    OffsetCelByFDelta(), OffsetAACelByIDelta(), OffsetCelListByIDelta()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/offsetcellistbyfdelta
|||	OffsetCelListByFDelta - Move a list of cels.
|||
|||	  Synopsis
|||
|||	    void OffsetCelListByFDelta(CCB *list, FPoint *deltaXY,
|||	                               Boolean copyPerspective)
|||
|||	  Description
|||
|||	    This function adjusts the location of all the cels in a list by adding the
|||	    delta values to the X/Y coordinates in cels' CCBs.
|||
|||	    This function can optionally propagate the size and perspective values
|||	    already present in the first cel to the rest of the cels in the list.
|||	    This is useful if you are manipulating size and perspective values
|||	    manually.  If you want to just copy the size and perspective values from
|||	    the first cel to the rest of the cels without changing the cel positions,
|||	    use a NULL deltaXY pointer and TRUE for copyPerspective.
|||
|||	  Arguments
|||
|||	    list                         A pointer to the list of cels to be moved.
|||
|||	    deltaXY                      A pointer to an FPoint containing the
|||	                                 movement delta values.
|||
|||	    copyPerspective              TRUE to copy the size and perspective fields
|||	                                 from the first cel to the rest of the list.
|||	                                 FALSE to leave all size and perspective
|||	                                 fields as is.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    OffsetCelListByIDelta(), OffsetAACelByFDelta(), OffsetCelByFDelta()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/offsetcellistbyidelta
|||	OffsetCelListByIDelta - Move a list of cels.
|||
|||	  Synopsis
|||
|||	    void OffsetCelListByIDelta(CCB *list, IPoint *deltaXY,
|||	                               Boolean copyPerspective)
|||
|||	  Description
|||
|||	    This function adjusts the location of all the cels in a list by adding the
|||	    delta values to the X/Y coordinates in cels' CCBs.
|||
|||	    This function can optionally propagate the size and perspective values
|||	    already present in the first cel to the rest of the cels in the list.
|||	    This is useful if you are manipulating size and perspective values
|||	    manually.  If you want to just copy the size and perspective values from
|||	    the first cel to the rest of the cels without changing the cel positions,
|||	    use a NULL deltaXY pointer and TRUE for copyPerspective.
|||
|||	  Arguments
|||
|||	    list                         A pointer to the list of cels to be moved.
|||
|||	    deltaXY                      A pointer to an IPoint containing the
|||	                                 movement delta values.
|||
|||	    copyPerspective              TRUE to copy the size and perspective fields
|||	                                 from the first cel to the rest of the list.
|||	                                 FALSE to leave all size and perspective
|||	                                 fields as is.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    OffsetCelListByFDelta(), OffsetAACelByIDelta(), OffsetCelByIDelta()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/offsetsrect
|||	OffsetSRect - Move an SRect.
|||
|||	  Synopsis
|||
|||	    SRect * OffsetSRect(SRect *dst, IPoint *deltaXY)
|||
|||	  Description
|||
|||	    This function adjusts the location described by the dst rectangle by
|||	    adding the specified delta values to the rectangle's position values.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to the CRect to move.
|||
|||	    deltaXY                      A pointer to an IPoint containing the
|||	                                 movement delta values.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    OffsetCRect(), InsetSRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/parsecel
|||	ParseCel - Parse a cel file image in memory.
|||
|||	  Synopsis
|||
|||	    CCB * ParseCel(void *inBuf, long inBufSize)
|||
|||	  Description
|||
|||	    This function parses cel(s) in a memory buffer.  It is useful when you
|||	    have loaded a cel file using some method other than LoadCel() and you need
|||	    to parse the cels in the buffer.
|||
|||	    If the buffer contains more than one cel, the CCBs are strung together via
|||	    their ccb_NextPtr fields in the same order as they are encountered in the
|||	    buffer.  This allows you to store a group of related cels in one file, and
|||	    treat them as a single logical entity using the other CelUtils library
|||	    functions that work with lists of cels.
|||
|||	    This function also contains special handling for anti-aliased cels.
|||	    AACels are two cels stored together in the same file, but the CCBs are
|||	    stored in reverse order of the way they must be drawn.  AACels are
|||	    recognized by a characteristic signature (exactly two cels, one is 4-bit
|||	    coded the other 16-bit uncoded, same size and coordinates, etc), and when
|||	    detected the CCB links are automatically reversed so the return value is a
|||	    list of two cels linked in the proper order for drawing.  One drawback to
|||	    the signature-detection logic is that it prevents the storage of multiple
|||	    related anti-aliased cels in the same file.
|||
|||	  Arguments
|||
|||	    inBuf                        A pointer to a buffer containing the cel
|||	                                 file image.
|||
|||	    inBufSize                    The size of cel file image in the buffer.
|||
|||	  Return Value
|||
|||	    A pointer to the list of cels parsed from the buffer.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    LoadCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/parsefont
|||	ParseFont - Parse a font and prepare it for use.
|||
|||	  Synopsis
|||
|||	    FontDescriptor * ParseFont(void *fontImage)
|||
|||	  Description
|||
|||	    Parses an in-memory image of a font file, creating a FontDescriptor for it
|||	    and preparing it for use. Use this function when you've loaded a font
|||	    file yourself (using data streaming or some other custom I/O method) and
|||	    you want to prepare the font for use by the font and text API routines.
|||
|||	    Use UnloadFont() to release resources acquired by ParseFont(). You must
|||	    release the font image memory yourself; UnloadFont() only releases
|||	    resources acquired internally by ParseFont().
|||
|||	  Arguments
|||
|||	    fontImage                    A pointer to a font file image that you have
|||	                                 loaded into memory.
|||
|||	  Return Value
|||
|||	    A pointer to a FontDescriptor structure that describes the font; NULL if
|||	    the image is not a valid font file, or there is not enough memory to
|||	    create a FontDescriptor structure.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, fontlib.h
|||
|||	  See Also
|||
|||	    LoadFont(), UnloadFont()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/readfile
|||	ReadFile - Read a file or a portion of one into memory
|||
|||	  Synopsis
|||
|||	    int ReadFile (char *filename, int32 size, long *buffer,
|||	                  long offset)
|||
|||	  Description
|||
|||	    ReadFile() uses the file system to read a file or a portion of a file into
|||	    memory. It performs up to four retries on each block read if the file
|||	    system returns an error.
|||
|||	    ReadFile() is useful for loading a portion of a file, optionally starting
|||	    at some offset into the file. For loading an entire file starting at
|||	    offset zero, the LoadFile() function is three to four times faster.
|||
|||	    ReadFile()fills the buffer with size bytes, beginning at offset.
|||
|||	  Arguments
|||
|||	    filename                     Pointer to a string containing a valid
|||	                                 pathname for a file in the currently mounted
|||	                                 file system.
|||
|||	    size                         Number of bytes to read.
|||
|||	    buffer                       Pointer to a buffer into which the data will
|||	                                 be read. Must be large enough to hold size
|||	                                 bytes.
|||
|||	    offset                       Offset from the beginning of the file at
|||	                                 which the read begins.
|||
|||	  Return Value
|||
|||	    The number of bytes read, or -1 if an error occurred.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, parse3do.h
|||
|||	  See Also
|||
|||	    LoadFile()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/rendercelfillrect
|||	RenderCelFillRect - Render a filled rectangle into a cel's data buffer.
|||
|||	  Synopsis
|||
|||	    void RenderCelFillRect(CCB *cel, int32 pixel,
|||	                           int32 x, int32 y, int32 w, int32 h)
|||
|||	  Description
|||
|||	    This function renders a filled rectangle into the cel's data buffer.
|||
|||	    This function performs no clipping internally.  You must be careful not to
|||	    pass coordinates that would result in rendering outside the cel's data
|||	    buffer.
|||
|||	    This function supports all cel formats and bit depths except:
|||
|||	         Packed cels.
|||
|||	         LRForm cels.
|||
|||	         6-bit coded cels.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel.
|||
|||	    pixel                        The value to store into each pixel in the
|||	                                 area.
|||
|||	    x                            The X coordinate within the cel data buffer.
|||
|||	    y                            The Y coordinate within the cel data buffer.
|||
|||	    w                            The width of the area to fill in the cel
|||	                                 data buffer.
|||
|||	    h                            The height of the area to fill in the cel
|||	                                 data buffer.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/rendercelhline
|||	RenderCelHLine - Render a horizontal line into a cel's data buffer.
|||
|||	  Synopsis
|||
|||	    void RenderCelHLine(CCB *cel, int32 pixel,
|||	                        int32 x, int32 y, int32 w)
|||
|||	  Description
|||
|||	    This function renders a horizontal line into the cel's data buffer.
|||
|||	    This function performs no clipping internally.  You must be careful not to
|||	    pass coordinates that would result in rendering outside the cel's data
|||	    buffer.
|||
|||	    This function supports all cel formats and bit depths except:
|||
|||	         Packed cels.
|||
|||	         LRForm cels.
|||
|||	         6-bit coded cels.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel.
|||
|||	    pixel                        The value to store into each pixel in the
|||	                                 line.
|||
|||	    x                            The X coordinate within the cel data buffer.
|||
|||	    y                            The Y coordinate within the cel data buffer.
|||
|||	    w                            The width of the line to render in the cel
|||	                                 data buffer.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/renderceloutlinerect
|||	RenderCelOutlineRect - Render an unfilled rectangle into a cel's data
|||	                       buffer.
|||
|||	  Synopsis
|||
|||	    void RenderCelOutlineRect(CCB *cel, int32 pixel,
|||	                              int32 x, int32 y, int32 w, int32 h)
|||
|||	  Description
|||
|||	    This function renders an outline (unfilled) rectangle into the cel's data
|||	    buffer.
|||
|||	    This function performs no clipping internally.  You must be careful not to
|||	    pass coordinates that would result in rendering outside the cel's data
|||	    buffer.
|||
|||	    This function supports all cel formats and bit depths except packed
|||	    cels, LRForm cels, and 6-bit coded cels.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel.
|||
|||	    pixel                        The value to store into each pixel in the
|||	                                 outline.
|||
|||	    x                            The X coordinate within the cel data buffer.
|||
|||	    y                            The Y coordinate within the cel data buffer.
|||
|||	    w                            The width of the area to outline in the cel
|||	                                 data buffer.
|||
|||	    h                            The height of the area to outline in the cel
|||	                                 data buffer.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/rendercelpixel
|||	RenderCelPixel - Render a single pixel into a cel's data buffer.
|||
|||	  Synopsis
|||
|||	    void RenderCelPixel(CCB *cel, int32 pixel, int32 x, int32 y)
|||
|||	  Description
|||
|||	    This function renders a single pixel into the cel's data buffer.
|||
|||	    This function performs no clipping internally.  You must be careful not to
|||	    pass coordinates that would result in rendering outside the cel's data
|||	    buffer.
|||
|||	    This function supports all cel formats and bit depths except packed
|||	    cels, LRForm cels, and 6-bit coded cels.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel.
|||
|||	    pixel                        The value to store into the pixel.
|||
|||	    x                            The X coordinate within the cel data buffer.
|||
|||	    y                            The Y coordinate within the cel data buffer.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/rendercelvline
|||	RenderCelVLine - Render a vertical line within a cel's data buffer.
|||
|||	  Synopsis
|||
|||	    void RenderCelVLine(CCB *cel, int32 pixel,
|||	                        int32 x, int32 y, int32 h)
|||
|||	  Description
|||
|||	    This function renders a vertical line into the cel's data buffer.
|||
|||	    This function performs no clipping internally.  You must be careful not to
|||	    pass coordinates that would result in rendering outside the cel's data
|||	    buffer.
|||
|||	    This function supports all cel formats and bit depths except packed
|||	    cels, LRForm cels, and 6-bit coded cels.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel.
|||
|||	    pixel                        The value to store into each pixel in the
|||	                                 line.
|||
|||	    x                            The X coordinate within the cel data buffer.
|||
|||	    y                            The Y coordinate within the cel data buffer.
|||
|||	    h                            The height of the line to render in the cel
|||	                                 data buffer.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/returncelpixel
|||	ReturnCelPixel - Return the value of a pixel from a cel's data buffer.
|||
|||	  Synopsis
|||
|||	    int32 ReturnCelPixel(CCB *cel, int32 x, int32 y)
|||
|||	  Description
|||
|||	    This function returns the value of the specified pixel from the cel's data
|||	    buffer.
|||
|||	    This function performs no clipping internally.  You must be careful not to
|||	    pass coordinates that would result in reading from outside the cel's data
|||	    buffer.
|||
|||	    This function supports all cel formats and bit depths except:
|||
|||	         Packed cels.
|||
|||	         LRForm cels.
|||
|||	         6-bit coded cels.
|||
|||	  Arguments
|||
|||	    cel                          A pointer to the cel.
|||
|||	    x                            The X coordinate within the cel data buffer.
|||
|||	    y                            The Y coordinate within the cel data buffer.
|||
|||	  Return Value
|||
|||	    The value of the specified pixel.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/savefile
|||	SaveFile - Save a file to NVRAM.
|||
|||	  Synopsis
|||
|||	    Err SaveFile(char *filename, void *buffer, int32 bufsize,
|||	                 int32 extrabytes)
|||
|||	  Description
|||
|||	    This function stores the contents of a buffer to the named file.
|||	    Currently the file has to be in NVRAM (IE, the filename must start with
|||	    /NRVRAM/) but if other writable filesystems become available this function
|||	    will work with them too.
|||
|||	    If the file doesn't already exist it is created.  If it does exist, its
|||	    contents are completely replaced by the contents of the specified buffer.
|||	    When the file is created it is pre-sized to bufsize+extrabytes.  If the
|||	    file exists but needs to be expanded to hold the output it is expanded to
|||	    the size needed+extrabytes.
|||
|||	    The point of the extrabytes parm is that expanding a file is relatively
|||	    expensive. By specifying a non-zero extrabytes parameter you can ensure
|||	    that the file will grow enough to handle a few more slightly-growing
|||	    outputs before an expansion has to be done again.  If the nature of the
|||	    data you're saving is that it never grows, use an extrabytes value of
|||	    zero.  Remember that NVRAM is a precious limited resource; please don't
|||	    needlessly usurp it with large extrabytes values.
|||
|||	  Arguments
|||
|||	    filename                     The name of the file to save.
|||
|||	    buffer                       A pointer to a buffer full of data to be
|||	                                 saved.
|||
|||	    bufsize                      The size of the data in the buffer.
|||
|||	    extrabytes                   The amount of extra space to allocate if and
|||	                                 when the file is expanded.
|||
|||	  Return Value
|||
|||	    Returns zero on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, blockfile.h
|||
|||	  See Also
|||
|||	    LoadFile()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/sec_to_use
|||	SEC_TO_USE - CConvert seconds to microseconds.
|||
|||	  Synopsis
|||
|||	    int32 SEC_TO_USEC(value)
|||
|||	  Description
|||
|||	    This macro converts a number of seconds to the equivelent number of
|||	    microseconds.  True floating point constant values are legal, for example
|||	    SEC_TO_USEC(2.5).  The compiler will resolve the constant at compile time
|||	    without needing runtime floating point support.  Values up to 2147 seconds
|||	    (about 35 minutes) can be expressed without overflowing an int32.  The
|||	    macro is usable in expression context and only evaluates its argument
|||	    once.
|||
|||	  Arguments
|||
|||	    value                        An integer constant or variable, or a
|||	                                 floating point constant to be converted to
|||	                                 microseconds.
|||
|||	  Return Value
|||
|||	    The number of microseconds.
|||
|||	  Implementation
|||
|||	    Macro
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/settextcelcolor
|||	SetTextCelColor - Set background and foreground colors for text cel.
|||
|||	  Synopsis
|||
|||	    void SetTextCelColor(TextCel *tCel, int32 bgColor, int32 fgColor0)
|||
|||	  Description
|||
|||	    Sets the background and foreground colors to use when the cel is drawn.
|||	    Because of special internal logic that calculates scaled colors for
|||	    anti-aliasing, you must use this function to change the colors, rather
|||	    than manipulating the cel's PLUT directly. After you use this call to
|||	    change the colors, the next DrawCels() call renders the existing contents
|||	    of the cel using the new colors.
|||
|||	    Colors follow the usual rules for 3DO cels. The bgColor and fgColor values
|||	    are interpreted as RGB555 values.  A color of 0,0,0 is transparent. To get
|||	    opaque black,  use MakeRGB15(0,0,1).  If bgColor or fgColor0 is passed as
|||	    -1L, the corresponding values in the text cel remain unchanged.
|||
|||	    When created, a text cel has default colors of transparent for background,
|||	    and pure white for foreground.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    bgColor                      The background color, used by zero-value
|||	                                 pixels in the character bitmap.
|||
|||	    fgColor                      The foreground color, used by non-zero-value
|||	                                 pixels in the character bitmap.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
|||	  See Also
|||
|||	    SetTextCelColors()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/settextcelcolors
|||	SetTextCelColors - Set background and foreground colors for a cel.
|||
|||	  Synopsis
|||
|||	    void SetTextCelColors(TextCel *tCel, int32 bgColor,
|||	                          int32 fgColors[4])
|||
|||	  Description
|||
|||	    Sets the background and foreground colors to  use when the cel is drawn.
|||	    This Function sets all four foreground colors; use it with features that
|||	    allow multiple text colors within a single cel. Because of special
|||	    internal logic that calculates scaled colors for anti-aliasing, you must
|||	    use this function to change the colors, rather than manipulating the
|||	    cel's PLUT directly. After you use this call to change the colors,
|||	    the next DrawCels() call renders the contents already in the cel in  new
|||	    colors.
|||
|||	    Colors follow the usual rules for 3DO cels.  The bgColor and fgColor
|||	     values are interpreted as RGB555 values.  A color of 0,0,0 is
|||	    transparent.  To get opaque black,  use MakeRGB15(0,0,1). If bgColoror any
|||	    of the fgColorsare passed as -1L, the corresponding values in the text cel
|||	    remain unchanged.
|||
|||	    When created, a TextCel has default colors of transparent for background,
|||	    pure white for foreground pen 0, and no color (transparent) for foreground
|||	    pens 1-3.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    bgColor                      The background color, used by zero-value
|||	                                 pixels in the character bitmap.
|||
|||	    fgColor                      The foreground colors, used by
|||	                                 non-zero-value pixels in the character
|||	                                 bitmap, according the TextCel\xd5 s pen
|||	                                 color value when the pixels were rendered.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/settextcelcoords
|||	SetTextCelCoords - Set x and y coordinates for a cel.
|||
|||	  Synopsis
|||
|||	    void SetTextCelCoords(TextCel *tCel, Coord ccbX, Coord ccbY)
|||
|||	  Description
|||
|||	    Sets the TextCel CCB coordinates (ccb_XPos and ccb_YPos).  Use this call
|||	    as a convenience so that you don't need to dereference through two
|||	    levels of pointers to update the CCB's on-screen position.
|||
|||	    If it's convenient, you can manipulate the CCB's position, size,
|||	    and perspective fields directly; you don't have to use this function
|||	    to safely modify the on-screen position. Other utility routines for
|||	    positioning cels, such as MapCel(), may also be safely used.
|||
|||	    The ccbX and ccbY values are interpreted as integer values if, when viewed
|||	    as a longword, they are in the range of -1024<value<1024.  (That is, if
|||	    interpreting the values as frac16 types would result in a very small
|||	    fraction  almost zero they are assumed to be integers.)  In this case,
|||	    SetTextCelCoords() converts the values to frac16 before storing them into
|||	    the CCB. Values outside that range are considered to be frac16 values
|||	    already, and are stored into the CCB without conversion.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    ccbX                         The x coordinate for the text cel CCB.
|||
|||	    ccbY                         The y coordinate for the text cel CCB.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/settextcelformatbuffer
|||	SetTextCelFormatBuffer - Attach a format buffer to a text cel.
|||
|||	  Synopsis
|||
|||	    Err SetTextCelFormatBuffer(TextCel *tCel, char *buffer,
|||	                               uint32 bufSize)
|||
|||	  Description
|||
|||	    Attaches a text-formatting buffer to a TextCel. The buffer is used for
|||	    printf-style formatting of text during rendering. If a text cel has a
|||	    format buffer, text rendered into the cel passes first through sprintf()
|||	    for formatting. If there is no format buffer attached to the cel (the
|||	    default), sprintf() is not invoked at render time.
|||
|||	    If the bufSize parameter is zero, any buffer already in the cel is
|||	    removed, and if the buffer was acquired internally, it is released.
|||	    (DeleteTextCel() also automatically releases an internally acquired
|||	    buffer.)
|||
|||	    If the bufSize parameter is non-zero, a format buffer is attached to the
|||	    cel. In this case, if the buffer parameter is NULL, a buffer of the
|||	    specified size is dynamically allocated internally. If the buffer
|||	    parameter is non-NULL, it is a pointer to a buffer that you have either
|||	    dynamically allocated or a static buffer within your application. When you
|||	    specify your own buffer, you are responsible for releasing it; the library
|||	    internals will not attempt to release a buffer you provided via a non-NULL
|||	    buffer parameter.
|||
|||	    A format buffer must be large enough to hold the largest possible
|||	    formatted string that you try to render. Internally, at rendering time,
|||	    the printf-style format string and arguments you supply are passed to
|||	    sprintf() for formatting. If the resulting string is larger than the
|||	    format buffer, the Debugger window displays a message, but memory beyond
|||	    the end of the buffer has already been corrupted at that point. (This is a
|||	    typical limitation of sprintf() processing.)
|||
|||	    Many text cels can share the same format buffer to save memory. The buffer
|||	    is only used while text is being rendered into the cel. Once
|||	    UpdateTextInCel() returns, the buffer is no longer in use by that cel, and
|||	    can be used in formatting the contents of another cel. The only time it is
|||	    not safe to use the same format buffer for multiple text cels is in a
|||	    multithreaded application where more than one thread might be calling
|||	    UpdateTextInCel() simultaneously.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    buffer                       Pointer to a buffer to use for text
|||	                                 formatting, or NULL to have a buffer
|||	                                 dynamically allocated for you.
|||
|||	    bufsize                      Size of the format buffer.
|||
|||	  Return Value
|||
|||	    Zero if successful, or -1 if dynamic buffer allocation fails; that is, if
|||	    there is not enough memory.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/settextcelformatflags
|||	SetTextCelFormatFlags - Set text formatting flags.
|||
|||	  Synopsis
|||
|||	    void SetTextCelFormatFlags(TextCel *tCel, uint32 formatFlags)
|||
|||	  Description
|||
|||	    Set the text formatting options used for subsequent rendering.
|||
|||	    The formatFlags parameter contains one or more formatting options, ORed
|||	    together.   Available options are:
|||
|||	    TC_FORMAT_LEFT_JUSTIFY - left-justify text
|||	    TC_FORMAT_RIGHT_JUSTIFY - right-justify text
|||	    TC_FORMAT_CENTER_JUSTIFY - center-justify text
|||	    TC_FORMAT_WORDWRAP - auto-word-wrap text
|||
|||	    Changing the formatting options does not affect pixels already in the cel.
|||	    You can make a series of calls to UpdateTextInCel()with the
|||	    replaceExisting flag set to FALSE, intermixed with calls to
|||	    SetTextCelFormatFlags().   This allows you to create, for example, a
|||	    paragraph of word-wrapped text, followed by a line of centered text,
|||	    followed by another word-wrapped paragraph.  Turning word wrap on and off
|||	    at locations other than a paragraph boundary might produce undesirable
|||	    results.  Word wrapping should only be changed immediately following a
|||	    newline.
|||
|||	  Arguments
|||
|||	    tCel                           Pointer to a TextCel structure.
|||
|||	    formatFlags                    The new formatting options.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/settextcelleadingadjust
|||	SetTextCelLeadingAdjust - Set leading adjustment for text cel.
|||
|||	  Synopsis
|||
|||	    void SetTextCelLeadingAdjust(TextCel *tCel, int32 adjustLeading)
|||
|||	  Description
|||
|||	    Sets a leading adjustment for text rendered in the cel. Leading is the
|||	    extra vertical space between lines of text. The base leading value comes
|||	    from theFontDescriptor attached to the cel. The value you specify in this
|||	    call is a signed adjustment to the leading. Negative values render lines
|||	    of text closer together; positive values render lines farther apart.
|||
|||	  Arguments
|||
|||	    tCel                           Pointer to a TextCel structure.
|||
|||	    adjustLeading                  The value (in pixels) to add to the base
|||	                                 leading value for the font.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/settextcelmargins
|||	SetTextCelMargins - Set top and left margin in cel data buffer.
|||
|||	  Synopsis
|||
|||	    void SetTextCelMargins(TextCel *tCel, int32 leftMargin,
|||	                           int32 topMargin)
|||
|||	  Description
|||
|||	    Sets the top and left margins within the cel data buffer. When you erase
|||	    the text in a cel, the pen returns to the top and left margin values. When
|||	    the pen encounters a newline in the text, the pen returns to the
|||	    left-margin value (and down to the next text line).
|||
|||	    If you set a margin value greater than the width and/or height of the cel,
|||	    all text rendered into the cel will be clipped.
|||
|||	  Arguments
|||
|||	    tCel                           Pointer to a TextCel structure.
|||
|||	    leftMargin                     New left margin for the cel.
|||
|||	    topMargin                      New top margin for the cel.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/settextcelpennumber
|||	SetTextCelPenNumber - Set pen number for text rendering.
|||
|||	  Synopsis
|||
|||	    void SetTextCelPenNumber(TextCel *tCel, int32 penNumber)
|||
|||	  Description
|||
|||	    Sets the pen number used for subsequent text rendering. The pen number,
|||	    which must be in the range of 0 through 3, indexes to one of the four
|||	    possible foreground colors.
|||
|||	    Whenyou create a text cel, only pen 0 has a default color assigned; other
|||	    pens are initialized to no color (transparent). Therefore you must use
|||	    SetTextCelColors() and specify nontransparent colors for pens 1-3 before
|||	    text you render with those pens will appear.
|||
|||	  Arguments
|||
|||	    tCel                           Pointer to a TextCel structure.
|||
|||	    penNumber                      Pen number to use for text subsequently
|||	                                 rendered into the cel.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/settextcelsize
|||	SetTextCelSize - Set new size for text in cel.
|||
|||	  Synopsis
|||
|||	    Err SetTextCelSize(TextCel *tCel, int32 width, int32 height)
|||
|||	  Description
|||
|||	    Sets a new size for the text cel. If width and/or height are zero, the cel
|||	    is set to a 1 x 1 pixel size initially, and each time text is placed into
|||	    the cel the size automatically changes to reflect the size of the
|||	    characters actually in the cel.
|||
|||	    Whenever you change a cel's size, you lose the cel's  current
|||	    contents, because the data buffer is discarded and reallocated at the new
|||	    size, and after allocation all pixels in the buffer are set to zero.
|||
|||	  Arguments
|||
|||	    tCel                           Pointer to a TextCel structure.
|||
|||	    width                          New width for the cel.
|||
|||	    height                         New height for the cel.
|||
|||	  Return Value
|||
|||	    Non-negative if successful, or negative if a new cel data buffer can't
|||	    be allocated.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/settextcelspacingadjust
|||	SetTextCelSpacingAdjust - Set spacing adjustment for text in text cel.
|||
|||	  Synopsis
|||
|||	    void SetTextCelSpacingAdjust(TextCel *tCel, int32 adjustSpacing)
|||
|||	  Description
|||
|||	    Sets a spacing adjustment for text rendered in the cel. Spacing is the
|||	    extra horizontal space between characters in the text. The base spacing
|||	    value comes from the FontDescriptor attached to the cel. The value you
|||	    specify in this call is a signed adjustment to the spacing. Negative
|||	    values render characters in the text closer together; positive values
|||	    render characters farther apart.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    adjustSpacing                The value (in pixels) to add to the base
|||	                                 spacing value for the font.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/settextceltabstops
|||	SetTextCelTabStops - Specify tab stops for a text cel.
|||
|||	  Synopsis
|||
|||	    void SetTextCelTabStops(TextCel *tCel, uint16 tabStops[16], ...)
|||
|||	  Description
|||
|||	    Sets the tab stops table for the text cel.  The tabStops parameter is a
|||	    pointer to an array of up to 16 uint16 values, or it is NULL to indicate
|||	    that up to 15 tab stops are available as a comma-delimited list in the ...
|||	    args.  In either case, the list of tab stops must be in ascending numeric
|||	    order.  Entering a zero value terminates the list.  If more than 15 tab
|||	    stops are read without encountering a terminating zero, a zero is forced
|||	    into the 16th array element in the text cel's internal tabs table.
|||
|||	    Tab stops are specified in terms of pixels from the left edge of the cel.
|||	    The tab stops are relative to zero, not to the left margin.
|||
|||	  Arguments
|||
|||	    t                            Cel  Pointer to a TextCel structure.
|||
|||	    tabStops                     Pointer to an array of uint16 that contains
|||	                                 the new tab stops table.  NULL means read
|||	                                 tab stops from ... args.
|||
|||	    ...                          List of tab stops, terminated with a zero.
|||	                                 Used if tabStops parameter is NULL.
|||	                                 (Optional.)
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/setupmsevents
|||	SetupMSEvents - Set up message and signal event handling.
|||
|||	  Synopsis
|||
|||	    MSEventHandle SetupMSEvents(MSEventData eventData[],
|||	                                int32 numEvents, int32 reserved)
|||
|||	  Description
|||
|||	    This function creates resources needed to dispatch message and signal
|||	    events.  It uses the array of MSEventData structures you pass in to decide
|||	    what type of setups to do.  It allocates signal and port items that you
|||	    didn't allocate yourself, and also builds internal control data to assist
|||	    in waiting for and dispatching the events.
|||
|||	    The MSEventData structure is defined in msgutils.h as follows:
|||
|||	                typedef struct MSEventData {
|||	                        char    *name;
|||	                        int32   (*handler)(MSEventData *eventData, void *userContext);
|||	                        void    *userData;
|||	                        int32   signal;
|||	                        Item    port;
|||	                        Item    msgItem;
|||	                        Message *msgPtr;
|||	                        MsgValueTypes   *msgValues;
|||	                        MSEventHandle   backLink;
|||	                } MSEventData;
|||
|||	    The values you place in the MSEventData structures before calling this
|||	    function control resource allocation, as described in the following
|||	    paragraphs.
|||
|||	    If you provide a non-NULL name pointer the MSEventData describes a message
|||	    port.  If the name field is NULL it describes a signal.  For message
|||	    ports, the port is created using the name you provide; if you don't need a
|||	    name just provide an empty string ("") for the name.
|||
|||	    The handler field is a pointer to your function to handle events for the
|||	    message port or signal.  Every event must have a non-NULL handler pointer.
|||
|||	    If you set the signal field to zero and the name field is NULL,
|||	    SetupMSEvents() will allocate a signal for you and store it in this field.
|||	    If the signal field is non-zero and the name field is NULL the signal
|||	    field is used as the signal(s) to be waited on for this event.  If the
|||	    name field is non-NULL (IE, this is a message event) any value already
|||	    existing in the signal field is replaced with the port's signal mask
|||	    during setup.
|||
|||	    If you set the port field to zero and the name field is non-NULL,
|||	    SetupMSEvents() will allocate a message port for you and store it in this
|||	    field.  If the port field is non-zero, it is assumed to be a message port
|||	    item number for a port you allocated yourself.  If the name field is NULL
|||	    (IE, this is a signal event) the port field is ignored.
|||
|||	    The remaining fields, (msgItem, msgPtr, msgValues, and backLink) are
|||	    read-only fields from your point of view.  When a message event arrives,
|||	    the dispatcher fills in the msgItem, msgPtr, and msgValues fields before
|||	    calling your handler function.  Your handler can refer to these values
|||	    when processing the message.
|||
|||	    The MSEventData array can contain some elements for which you've allocated
|||	    your own signal or port and other elements with zero values that request
|||	    automatic allocation at setup time.  The internals keep track of who
|||	    allocated which resources and at cleanup time only automatically allocated
|||	    resources are freed automatically.
|||
|||	  Arguments
|||
|||	    eventData                    A pointer to an array of MSEventData
|||	                                 structures which describe the event(s).
|||
|||	    numEvents                    The number of elements in the MSEventData
|||	                                 array.
|||
|||	    reserved                     Reserved for future expansion; pass zero.
|||
|||	  Return Value
|||
|||	    Returns negative on error, or non-negative on success.  The non-negative
|||	    return value is a handle which must be passed to the Dispatch and Cleanup
|||	    routines.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, msgutils.h
|||
|||	  See Also
|||
|||	    CleanupMSEvents(), DispatchMSEvents()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/sleep
|||	Sleep - Sleep for a number of seconds.
|||
|||	  Synopsis
|||
|||	    Err Sleep(Item ioreq, uint32 seconds)
|||
|||	  Description
|||
|||	    This function puts the calling task in a wait state for the specified
|||	    number of seconds.  The function does not return until the time has
|||	    passed.
|||
|||	  Arguments
|||
|||	    ioreq                        An IOReq item obtained from GetTimerIOReq(),
|||	                                 or zero to have an IOReq dynamically
|||	                                 created/deleted during the call.
|||
|||	    seconds                      The number of seconds to sleep.
|||
|||	  Return Value
|||
|||	    Zero for success, or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    GetTimerIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/sleephsec
|||	SleepHSec - Sleep for a number of hundreths of a second.
|||
|||	  Synopsis
|||
|||	    Err SleepHSec(Item ioreq, uint32 hseconds)
|||
|||	  Description
|||
|||	    This function puts the calling task in a wait state for the specified
|||	    number of hundreths-of-a-second. The function does not return until the
|||	    time has passed.
|||
|||	  Arguments
|||
|||	    ioreq                        An IOReq item obtained from GetTimerIOReq(),
|||	                                 or zero to have an IOReq dynamically
|||	                                 created/deleted during the call.
|||
|||	    hseconds                     The number of hundredths-of-a-second to
|||	                                 sleep.  This value can be larger than 100.
|||
|||	  Return Value
|||
|||	    Zero for success, or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    GetTimerIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/sleepmsec
|||	SleepMSec - Sleep for a number of milliseconds.
|||
|||	  Synopsis
|||
|||	    Err SleepMSec(Item ioreq, uint32 mseconds)
|||
|||	  Description
|||
|||	    This function puts the calling task in a wait state for the specified
|||	    number of milliseconds. The function does not return until the time has
|||	    passed.
|||
|||	  Arguments
|||
|||	    ioreq                        An IOReq item obtained from GetTimerIOReq(),
|||	                                 or zero to have an IOReq dynamically
|||	                                 created/deleted during the call.
|||
|||	    mseconds                     The number of milliseconds to sleep.  This
|||	                                 value can be larger than 1000.
|||
|||	  Return Value
|||
|||	    Zero for success, or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    GetTimerIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/sleeptsec
|||	SleepTSec - Sleep for a number of tenths of a second.
|||
|||	  Synopsis
|||
|||	    Err SleepTSec(Item ioreq, uint32 tseconds)
|||
|||	  Description
|||
|||	    This function puts the calling task in a wait state for the specified
|||	    number of tenths-of-a-second.  The function does not return until the time
|||	    has passed.
|||
|||	  Arguments
|||
|||	    ioreq                        An IOReq item obtained from GetTimerIOReq(),
|||	                                 or zero to have an IOReq dynamically
|||	                                 created/deleted during the call.
|||
|||	    tseconds                     The number of tenths-of-a-second to sleep.
|||	                                 This value can be larger than 10.
|||
|||	  Return Value
|||
|||	    Zero for success, or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    GetTimerIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/sleepusec
|||	SleepUSec - Sleep for a number of microseconds.
|||
|||	  Synopsis
|||
|||	    Err SleepUSec(Item ioreq, uint32 seconds, uint32 useconds)
|||
|||	  Description
|||
|||	    This function puts the calling task in a wait state for the specified
|||	    period of time.  The function does not return until the time has passed.
|||
|||	  Arguments
|||
|||	    ioreq                        An IOReq item obtained from GetTimerIOReq(),
|||	                                 or zero to have an IOReq dynamically
|||	                                 created/deleted during the call.
|||
|||	    seconds                      The number of seconds to sleep.
|||
|||	    useconds                     The number of microseconds to sleep.
|||
|||	  Return Value
|||
|||	    Zero for success, or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    GetTimerIOReq()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/srectfromcrect
|||	SRectFromCRect - Convert a CRect to an SRect.
|||
|||	  Synopsis
|||
|||	    SRect * SRectFromCRect(SRect *dst, CRect *src)
|||
|||	  Description
|||
|||	    This function converts a CRect to an SRect that describes the same area.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to an SRect where the results are
|||	                                 to be stored.
|||
|||	    src                          A pointer to a CRect to be converted to an
|||	                                 SRect.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CRectFromSRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/srectfromcel
|||	SRectFromCel - Return SRect describing cel as currently projected.
|||
|||	  Synopsis
|||
|||	    SRect * SRectFromCel(SRect *dst, CCB *cel)
|||
|||	  Description
|||
|||	    This function calculates the rectangle occupied by a cel as projected and
|||	    stores the results into *dst.  If the cel is projected to a
|||	    non-rectangular shape the results will be invalid.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to an SRect where the results are
|||	                                 to be stored.
|||
|||	    cel                          A pointer to the cel.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    CRectFromCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/srectfromival
|||	SRectFromIVal - Fill in an SRect from inline values.
|||
|||	  Synopsis
|||
|||	    SRect * SRectFromIVal(SRect *dst, int32 x, int32 y,
|||	                          int32 w, int32 h)
|||
|||	  Description
|||
|||	    This function stores the specified position and size value into *dst.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to an SRect where the results are
|||	                                 to be stored.
|||
|||	    x                            The value to assign to dst->pos.x.
|||
|||	    y                            The value to assign to dst->pos.y.
|||
|||	    w                            The value to assign to dst->size.x.
|||
|||	    h                            The value to assign to dst->size.y.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    SRectFromCRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/srectintersection
|||	SRectIntersection - Calculate the intersection of two SRects.
|||
|||	  Synopsis
|||
|||	    SRect * SRectIntersection(SRect *dst, SRect *rect1, SRect *rect2)
|||
|||	  Description
|||
|||	    This function calculates the intersection of two rectangles, returning a
|||	    rectangle that describes the common area.  The dst rectangle can be the
|||	    same as either source rectangle.  If there is no common area between the
|||	    two source rectangles, the function return value is NULL, but the values
|||	    in *dst are still modified.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to an SRect where the results are
|||	                                 to be stored.
|||
|||	    rect1                        A pointer to one of the source SRects.
|||
|||	    rect2                        A pointer to the other source SRect.
|||
|||	  Return Value
|||
|||	    The input parameter dst, or NULL if there is no common area between the
|||	    source rectangles.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    SRectBounds(), CRectIntersection()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/srectbounds
|||	SRectBounds - Calculate the union of two SRects.
|||
|||	  Synopsis
|||
|||	    SRect * SRectBounds(SRect *dst, SRect *rect1, SRect *rect2)
|||
|||	  Description
|||
|||	    This function calculates the bounding box of two rectangles, returning a
|||	    rectangle that encompases all the area described by the two source
|||	    rectangles.  The dst rectangle can be the same as either source rectangle.
|||
|||	  Arguments
|||
|||	    dst                          A pointer to an SRect where the results are
|||	                                 to be stored.
|||
|||	    rect1                        A pointer to one of the source SRects.
|||
|||	    rect2                        A pointer to the other source SRect.
|||
|||	  Return Value
|||
|||	    The input parameter dst.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    SRectIntersection(), CRectBounds()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/tacreatetextcel
|||	taCreateTextCel - Create a text cel from tag args.
|||
|||	  Synopsis
|||
|||	    TextCel * taCreateTextCel(TagArg *tags)
|||
|||	  Description
|||
|||	    Creates a new text cel.  Specifying appropriate tags and values during cel
|||	    creation sets all configurable attributes of the text cel.   If you
|||	    include TCEL_TAG_UPDATE_TEXT_STRING in the tags, the specified text is
|||	    rendered into the cel after processing all other tags.
|||
|||	    The only required tag is TCEL_TAG_FONT.  All unspecified attributes are
|||	    set to default values as per CreateTextCel().  If width and/or height tags
|||	    are not specified, the associated values are set to zero, resulting in an
|||	    auto size cel.  Tags are documented in Appendix A of this document.
|||
|||	  Arguments
|||
|||	    tags                         Pointer to an array of tags and associated
|||	                                 values.
|||
|||	  Return Value
|||
|||	    A pointer to the newly created TextCel structure; returns NULL if there
|||	    isn't enough memory to create the new TextCel, which indicates that
|||	    invalid tags are present.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
|||	  See Also
|||
|||	    CreateTextCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/tamodifytextcel
|||	taModifyTextCel - Change text cel attributes from tag args.
|||
|||	  Synopsis
|||
|||	    TextCel * taModifyTextCel(TextCel *tCel, TagArg *tags)
|||
|||	  Description
|||
|||	    Changes attributes already in a text cel.  You can set all configurable
|||	    attributes of the text cel by specifying appropriate tags and values.   If
|||	    you include TCEL_TAG_UPDATE_TEXT_STRING  in the tags, the specified text
|||	    is rendered into the cel after processing all other tags.
|||
|||	    A negative return value is possible when the text cel size or format
|||	    buffer attributes are changed and a memory allocation failure occurs.  A
|||	    positive non-zero return value is possible if TCEL_TAG_UPDATE_TEXT_STRING
|||	    is specified and characters are clipped.  Otherwise, the return value is
|||	    zero.
|||
|||	    Tags are documented in Appendix A of this document.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    tags                         Pointer to an array of tags and associated
|||	                                 values.
|||
|||	  Return Value
|||
|||	    Negative on error, non-negative if successful.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
|||	  See Also
|||
|||	    SetTextCel...() functions
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timercancel
|||	TimerCancel - Cancel and delete an existing timer.
|||
|||	  Synopsis
|||
|||	    Err TimerCancel(TimerHandle thandle)
|||
|||	  Description
|||
|||	    This function cancels a timer (if active) and releases all resources
|||	    acquired when it was created.  The timer's handle is no longer valid after
|||	    this call.
|||
|||	    If an event had occurred for this timer and a notification message was
|||	    sent, this function withdraws the notification message from your port.  If
|||	    you had already received the message but not yet replied to it, the
|||	    timer's resources are released after you reply to the message.  In other
|||	    words, you can respond to a timer event message by cancelling the timer
|||	    that sent the message, and you can safely do so before replying to the
|||	    message.
|||
|||	    If the timer notification was signal-based and a notification signal was
|||	    sent, you will still receive it since there is no way to withdraw a signal
|||	    once it has been sent.
|||
|||	  Arguments
|||
|||	    thandle                      The handle for a timer created by one of the
|||	                                 TimerMsg or TimerSignal functions.
|||
|||	  Return Value
|||
|||	    Zero on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerSuspend()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timerchangeuserdata
|||	TimerChangeUserdata - Change user data values for a timer.
|||
|||	  Synopsis
|||
|||	    Err TimerChangeUserdata(TimerHandle thandle,
|||	                            uint32 userdata1, uint32 userdata2)
|||
|||	  Description
|||
|||	    This function changes the user data associated with a timer.  The user
|||	    data is delivered to you as part of the notification message for
|||	    message-based timers.  The user data is not meaningful for signal-based
|||	    timers.
|||
|||	  Arguments
|||
|||	    thandle                      The handle for a timer created by one of the
|||	                                 TimerMsg or TimerSignal functions.
|||
|||	    userdata1                    A value passed back to you in the
|||	                                 msg_DataPtr field of the notification
|||	                                 message.
|||
|||	    userdata2                    A value passed back to you in the
|||	                                 msg_DataSize field of the notification
|||	                                 message.
|||
|||	  Return Value
|||
|||	    Zero on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timerreset
|||	TimerReset - Restart a timer using new values.
|||
|||	  Synopsis
|||
|||	    Err TimerReset(TimerHandle thandle,
|||	                   uint32 seconds, uint32 usecs_or_fields)
|||
|||	  Description
|||
|||	    This function restarts a timer using new time values.  If the timer was
|||	    suspended, this reactivates it.  If the timer was active, this stops the
|||	    current action and restarts the timer with the new time values.  This can
|||	    be used to start a new delay even if the old one hadn't triggered yet.
|||
|||	    For microsecond-based timers, pass both seconds and microseconds values.
|||	    For VBL-based timers, pass zero for seconds and put the new field count in
|||	    the usecs_or_fields parm.
|||
|||	    If an event had occurred for this timer and a notification message was
|||	    sent, this function withdraws the notification message from your port.  If
|||	    you had already received the message but not yet replied to it, the timer
|||	    is still restarted properly.  In other words, you can respond to a timer
|||	    event message by resetting the timer that sent the message, and you can
|||	    safely do so before replying to the message.
|||
|||	  Arguments
|||
|||	    thandle                      The handle for a timer created by one of the
|||	                                 TimerMsg or TimerSignal functions.
|||
|||	    seconds                      The new seconds value for the timer.
|||
|||	    usecs_or_fields              The new microseconds or field-count value
|||	                                 for the timer.
|||
|||	  Return Value
|||
|||	    Zero on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerRestart()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timerrestart
|||	TimerRestart - Restart a timer using current values.
|||
|||	  Synopsis
|||
|||	    Err TimerRestart(TimerHandle thandle)
|||
|||	  Description
|||
|||	    This function restarts a timer using the same time values as the last time
|||	    the timer was active.  If the timer was suspended, this reactivates it.
|||	    If the timer was active, this stops the current action and restarts the
|||	    timer.  This can be used to start a new delay even if the old one hadn't
|||	    triggered yet.  It can also be used to re-sync a heartbeat timer to some
|||	    external event.
|||
|||	    If an event had occurred for this timer and a notification message was
|||	    sent, this function withdraws the notification message from your port.  If
|||	    you had already received the message but not yet replied to it, the timer
|||	    is still restarted properly.  In other words, you can respond to a timer
|||	    event message by restarting the timer that sent the message, and you can
|||	    safely do so before replying to the message.
|||
|||	  Arguments
|||
|||	    thandle                      The handle for a timer created by one of the
|||	                                 TimerMsg or TimerSignal functions.
|||
|||	  Return Value
|||
|||	    Zero on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerReset()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timerservicesclose
|||	TimerServicesClose - Close the connection to a running TimerServices
|||	                     thread.
|||
|||	  Synopsis
|||
|||	    void TimerServicesClose(void)
|||
|||	  Description
|||
|||	    This function closes your task's connection to a TimerServices thread
|||	    started by some other task.  Before calling this function, ensure that
|||	    your task and all its child threads have called TimerCancel() to release
|||	    any timers they were using.
|||
|||	  Return Value
|||
|||	    Zero on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerServicesShutdown()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timerservicesopen
|||	TimerServicesOpen - Open a connection to a running TimerServices thread.
|||
|||	  Synopsis
|||
|||	    Err TimerServicesOpen(void)
|||
|||	  Description
|||
|||	    This function establishes your task's connection to a TimerServices thread
|||	    started by some other task.  After a successful return from this function
|||	    your task and all its child threads can use timer services.
|||
|||	  Return Value
|||
|||	    Zero on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerServicesStartup()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timerservicesshutdown
|||	TimerServicesShutdown - Shut down the TimerServices thread.
|||
|||	  Synopsis
|||
|||	    void TimerServicesShutdown(void)
|||
|||	  Description
|||
|||	    This function shuts down the TimerServices thread.  Before calling this
|||	    function, ensure that your task and all its child threads have called
|||	    TimerCancel() to release any timers they were using.
|||
|||	    Only the task which started the TimerServices thread may shut it down.
|||
|||	  Return Value
|||
|||	    Zero on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerServicesClose()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timerservicesstartup
|||	TimerServicesStartup - Start the TimerServices thread.
|||
|||	  Synopsis
|||
|||	    Err TimerServicesStartup(int32 delta_priority)
|||
|||	  Description
|||
|||	    This function starts the timer services thread.  The thread's priority is
|||	    set to your task's priority plus the delta_priority.  After calling this
|||	    function your task and all its child threads can use timer services.
|||
|||	    The service thread doesn't use many CPU cycles, and it uses them only when
|||	    you make requests for services and when time events occur.  Generally, you
|||	    should run the service thread at a slightly higher priority than the rest
|||	    of your tasks/threads, especially if timely notifications are critical.
|||
|||	    If this function returns positive 1, that indicates that someone else has
|||	    already started the service thread.  In this case, you should respond by
|||	    calling TimerServicesOpen() to connect with the already-running thread.
|||
|||	  Arguments
|||
|||	    delta_priority               A value added to your task's priority, the
|||	                                 result becomes the thread's priority.
|||
|||	  Return Value
|||
|||	    Zero on success, one if the thread is already running, or a negative error
|||	    code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerServicesOpen()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timerservicesverify
|||	TimerServicesVerify - Verify timer owners.
|||
|||	  Synopsis
|||
|||	    Err TimerServicesVerify(void)
|||
|||	  Description
|||
|||	    This function causes the TimerServices thread to check all outstanding
|||	    timers to make sure the tasks and threads that created them are still
|||	    running.  If any tasks or threads have died without releasing their
|||	    timers, this function releases them.
|||
|||	    This function is intended to be called by the task that started the
|||	    service thread.  If a main task starts the service thread and then spawns
|||	    other tasks that may use the services, it can call this function after
|||	    each child task has completed.  This function forces a cleanup of any
|||	    timers a child task left outstanding, which can help prevent the leaking
|||	    away of resources from the main task's memory/item pools.  The cleanup
|||	    will not affect any timers belonging to child tasks or threads which are
|||	    still running.
|||
|||	    When the library is compiled with DEBUG=1 in effect, this function reports
|||	    in the debugger terminal window any cleanup actions it has to take .  This
|||	    can help you identify tasks or threads which are leaving timers
|||	    outstanding when they terminate.
|||
|||	  Return Value
|||
|||	    Zero on success or a negative error code.  If some timers had to be
|||	    released because their owners died, that is NOT considered to be an error.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timersuspend
|||	TimerSuspend - Suspend a timer.
|||
|||	  Synopsis
|||
|||	    Err TimerSuspend(TimerHandle thandle)
|||
|||	  Description
|||
|||	    This function suspends an active timer but does not release resources
|||	    associated with the timer.  If the timer was already suspended this
|||	    function acts as a no-op.
|||
|||	    If an event had occurred for this timer and a notification message was
|||	    sent, this function withdraws the notification message from your port.  If
|||	    you had already received the message but not yet replied to it, the timer
|||	    is still properly suspended.  In other words, you can respond to a timer
|||	    event message by suspending the timer that sent the message, and you can
|||	    safely do so before replying to the message.
|||
|||	    Use TimerRestart() to reactivate the timer using the old time values.  Use
|||	    TimerReset() to reactivate it using new time values.   If the timer is
|||	    delay-based, suspending then restarting the timer does not resume the
|||	    delay count from the point it was suspended, it starts a new delay of the
|||	    specified period.
|||
|||	  Arguments
|||
|||	    thandle                      The handle for a timer created by one of the
|||	                                 TimerMsg or TimerSignal functions.
|||
|||	  Return Value
|||
|||	    Zero on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerCancel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timermsgafterdelay
|||	TimerMsgAfterDelay - Send a message after a delay.
|||
|||	  Synopsis
|||
|||	    TimerHandle TimerMsgAfterDelay(Item msgport,
|||	                                   uint32 seconds, uint32 microseconds,
|||	                                   uint32 userdata1, uint32 userdata2)
|||
|||	  Description
|||
|||	    This function creates a timer that notifies you by message after the
|||	    specified delay.  The timer is a one-shot; after sending a notification
|||	    message the timer enters a suspended state.  Use TimerReset() to
|||	    reactivate the timer using new time values, TimerRestart() to start
|||	    another delay period of the same length as last time, or TimerCancel() to
|||	    delete the timer.
|||
|||	    The TimerServices thread must be active before calling this function.
|||
|||	  Arguments
|||
|||	    msgport                      The port to which the notification message
|||	                                 is sent.
|||
|||	    seconds                      The seconds portion of the delay.
|||
|||	    microseconds                 The microseconds portion of the delay.  This
|||	                                 value can be larger than 1 million.
|||
|||	    userdata1                    A value passed back to you in the
|||	                                 msg_DataPtr field of the notification
|||	                                 message.
|||
|||	    userdata2                    A value passed back to you in the
|||	                                 msg_DataSize field of the notification
|||	                                 message.
|||
|||	  Return Value
|||
|||	    A TimerHandle (a positive value) on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerMsgAtTime(), TimerMsgHeartbeat()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timermsgafterdelayvbl
|||	TimerMsgAfterDelayVBL - Send a message after the specified number of VBLs
|||	                        occur.
|||
|||	  Synopsis
|||
|||	    TimerHandle TimerMsgAfterDelayVBL(Item msgport, uint32 fields,
|||	                                      uint32 userdata1,
|||	                                      uint32 userdata2)
|||
|||	  Description
|||
|||	    This function creates a timer that notifies you by message after the
|||	    specified number of VBLs have occurred.  The timer is a one-shot; after
|||	    sending a notification message the timer enters a suspended state.  Use
|||	    TimerReset() to reactivate the timer using new time values, TimerRestart()
|||	    to start another delay period of the same length as last time, or
|||	    TimerCancel() to delete the timer.
|||
|||	    The TimerServices thread must be active before calling this function.
|||
|||	  Arguments
|||
|||	    msgport                      The port to which the notification message
|||	                                 is sent.
|||
|||	    fields                       The number of VBLs to occur before sending
|||	                                 the message.
|||
|||	    userdata1                    A value passed back to you in the
|||	                                 msg_DataPtr field of the notification
|||	                                 message.
|||
|||	    userdata2                    A value passed back to you in the
|||	                                 msg_DataSize field of the notification
|||	                                 message.
|||
|||	  Return Value
|||
|||	    A TimerHandle (a positive value) on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerMsgAtTimeVBL(), TimerMsgHeartbeatVBL()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timermsgattime
|||	TimerMsgAtTime - Send a message at a specific time.
|||
|||	  Synopsis
|||
|||	    TimerHandle TimerMsgAtTime(Item msgport,
|||	                               uint32 seconds, uint32 microseconds,
|||	                               uint32 userdata1, uint32 userdata2)
|||
|||	  Description
|||
|||	    This function creates a timer that notifies you by message at the
|||	    specified time.  The timer is a one-shot; after sending a notification
|||	    message the timer enters a suspended state.  Use TimerReset() to
|||	    reactivate the timer using new time values, or TimerCancel() to delete the
|||	    timer.
|||
|||	    The notification message is a SMALL message containing as values the
|||	    userdata1 and userdata2 values specified in this call.
|||
|||	    AtTime messages are useful primarily for synchronizing with real-world
|||	    events.  For a one-shot delay use TimerMsgDelay().  For a periodic series
|||	    of messages, use TimerMsgHeartbeat().
|||
|||	    The TimerServices thread must be active before calling this function.
|||
|||	  Arguments
|||
|||	    msgport                      The port to which the notification message
|||	                                 is sent.
|||
|||	    seconds                      The seconds portion of the time.
|||
|||	    microseconds                 The microseconds portion of the time.  This
|||	                                 value can be larger than 1 million.
|||
|||	    userdata1                    A value passed back to you in the
|||	                                 msg_DataPtr field of the notification
|||	                                 message.
|||
|||	    userdata2                    A value passed back to you in the
|||	                                 msg_DataSize field of the notification
|||	                                 message.
|||
|||	  Return Value
|||
|||	    A TimerHandle (a positive value) on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerMsgAfterDelay(), TimerMsgHeartbeat()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timermsgattimevbl
|||	TimerMsgAtTimeVBL - Send a message at the specified VBL time.
|||
|||	  Synopsis
|||
|||	    TimerHandle TimerMsgAtTimeVBL(Item msgport, uint32 fields,
|||	                                  uint32 userdata1, uint32 userdata2)
|||
|||	  Description
|||
|||	    This function creates a timer that notifies you by message at the
|||	    specified VBL time.  The timer is a one-shot; after sending a notification
|||	    message the timer enters a suspended state.  Use TimerReset() to
|||	    reactivate the timer using new time values, or TimerCancel() to delete the
|||	    timer.
|||
|||	    The notification message is a SMALL message containing as values the
|||	    userdata1 and userdata2 values specified in this call.
|||
|||	    The TimerServices thread must be active before calling this function.
|||
|||	  Arguments
|||
|||	    msgport                      The port to which the notification message
|||	                                 is sent.
|||
|||	    fields                       The VBL field count at which the event
|||	                                 occurs.
|||
|||	    userdata1                    A value passed back to you in the
|||	                                 msg_DataPtr field of the notification
|||	                                 message.
|||
|||	    userdata2                    A value passed back to you in the
|||	                                 msg_DataSize field of the notification
|||	                                 message.
|||
|||	  Return Value
|||
|||	    A TimerHandle (a positive value) on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerMsgAfterDelayVBL(), TimerMsgHeartbeatVBL()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timermsgheartbeat
|||	TimerMsgHeartbeat - Send a message periodically.
|||
|||	  Synopsis
|||
|||	    TimerHandle TimerMsgHeartbeat(Item msgport,
|||	                                  uint32 seconds, uint32 microseconds,
|||	                                  uint32 userdata1, uint32 userdata2)
|||
|||	  Description
|||
|||	    This function creates a timer that repeatedly notifies you by message at
|||	    the specified interval.  The timer continues sending notifications on a
|||	    regular interval until you use TimerSuspend() to temporarily halt it or
|||	    TimerCancel() to delete it.
|||
|||	    The Heartbeat support in the TimerServices thread contains logic to
|||	    prevent drift in the frequency of timer events.  Messages are delivered at
|||	    the specified frequency even if it takes you significant time to respond
|||	    to each message event.  If an entire frequency interval elapses and you
|||	    still haven't replied to the prior message you will miss one timer event,
|||	    but the frequency of subsequent events is not disturbed.
|||
|||	    The TimerServices thread must be active before calling this function.
|||
|||	  Arguments
|||
|||	    msgport                      The port to which the notification message
|||	                                 is sent.
|||
|||	    seconds                      The seconds portion of the frequency.
|||
|||	    microseconds                 The microseconds portion of the frequency.
|||	                                 This value can be larger than 1 million.
|||
|||	    userdata1                    A value passed back to you in the
|||	                                 msg_DataPtr field of the notification
|||	                                 message.
|||
|||	    userdata2                    A value passed back to you in the
|||	                                 msg_DataSize field of the notification
|||	                                 message.
|||
|||	  Return Value
|||
|||	    A TimerHandle (a positive value) on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerMsgAtTime(), TimerMsgAfterDelay(), HZ_TO_USEC()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timermsgheartbeatvbl
|||	TimerMsgHeartbeatVBL - Send a message periodically based on VBLs.
|||
|||	  Synopsis
|||
|||	    TimerHandle TimerMsgHeartbeatVBL(Item msgport, uint32 fields,
|||	                                     uint32 userdata1,
|||	                                     uint32 userdata2)
|||
|||	  Description
|||
|||	    This function creates a timer that repeatedly notifies you by message
|||	    after the specified number of VBLs have occurred.  The timer continues
|||	    sending notifications on a regular interval until you use TimerSuspend()
|||	    to temporarily halt it or TimerCancel() to delete it.
|||
|||	    The Heartbeat support in the TimerServices thread contains logic to
|||	    prevent drift in the frequency of timer events.  Signals are delivered at
|||	    the specified frequency even if it takes you significant time to respond
|||	    to each signal event.
|||
|||	    The TimerServices thread must be active before calling this function.
|||
|||	  Arguments
|||
|||	    msgport                      The port to which the notification message
|||	                                 is sent.
|||
|||	    fields                       The number of VBLs to occur between each
|||	                                 event.
|||
|||	    userdata1                    A value passed back to you in the
|||	                                 msg_DataPtr field of the notification
|||	                                 message.
|||
|||	    userdata2                    A value passed back to you in the
|||	                                 msg_DataSize field of the notification
|||	                                 message.
|||
|||	  Return Value
|||
|||	    A TimerHandle (a positive value) on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerMsgAtTimeVBL(), TimerMsgAfterDelayVBL()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timersignalafterdelay
|||	TimerSignalAfterDelay - Send a signal after a delay.
|||
|||	  Synopsis
|||
|||	    TimerHandle TimerSignalAfterDelay(int32 signal, uint32 seconds,
|||	                                      uint32 microseconds)
|||
|||	  Description
|||
|||	    This function creates a timer that notifies you by signal after the
|||	    specified delay.  The timer is a one-shot; after sending a notification
|||	    message the timer enters a suspended state.  Use TimerReset() to
|||	    reactivate the timer using new time values, TimerRestart() to start
|||	    another delay period of the same length as last time, or TimerCancel() to
|||	    delete the timer.
|||
|||	    The TimerServices thread must be active before calling this function.
|||
|||	  Arguments
|||
|||	    signal                       The signal to send to your task.
|||
|||	    seconds                      The seconds portion of the delay.
|||
|||	    microseconds                 The microseconds portion of the delay.  This
|||	                                 value can be larger than 1 million.
|||
|||	  Return Value
|||
|||	    A TimerHandle (a positive value) on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerSignalAtTime(), TimerSignalHeartbeat()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timersignalafterdelayvbl
|||	TimerSignalAfterDelayVBL - Send a signal after the specified number of
|||	                           VBLs occur .
|||
|||	  Synopsis
|||
|||	    TimerHandle TimerSignalAfterDelayVBL(int32 signal, uint32 fields)
|||
|||	  Description
|||
|||	    This function creates a timer that notifies you by signal after the
|||	    specified number of VBLs have occurred.  The timer is a one-shot; after
|||	    sending a notification message the timer enters a suspended state.  Use
|||	    TimerReset() to reactivate the timer using new time values, TimerRestart()
|||	    to start another delay period of the same length as last time, or
|||	    TimerCancel() to delete the timer.
|||
|||	    The TimerServices thread must be active before calling this function.
|||
|||	  Arguments
|||
|||	    signal                       The signal to send to your task.
|||
|||	    fields                       The number of VBLs to occur before sending
|||	                                 the message.
|||
|||	  Return Value
|||
|||	    A TimerHandle (a positive value) on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerSignalAtTimeVBL(), TimerSignalHeartbeatVBL()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timersignalattime
|||	TimerSignalAtTime - Send a signal at the specified time.
|||
|||	  Synopsis
|||
|||	    TimerHandle TimerSignalAtTime(int32 signal, uint32 seconds,
|||	                                  uint32 microseconds)
|||
|||	  Description
|||
|||	    This function creates a timer that notifies you by signal at the specified
|||	    time.  The timer is a one-shot; after sending a notification message the
|||	    timer enters a suspended state.  Use TimerReset() to reactivate the timer
|||	    using new time values, or TimerCancel() to delete the timer.
|||
|||	    AtTime signals are useful primarily for synchronizing with real-world
|||	    events.  For a one-shot delay use TimerSignalDelay().  For a periodic
|||	    series of signals, use TimerSignalHeartbeat().
|||
|||	    The TimerServices thread must be active before calling this function.
|||
|||	  Arguments
|||
|||	    signal                       The signal to send to your task.
|||
|||	    seconds                      The seconds portion of the time.
|||
|||	    microseconds                 The microseconds portion of the time.  This
|||	                                 value can be larger than 1 million.
|||
|||	  Return Value
|||
|||	    A TimerHandle (a positive value) on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerSignalAfterDelay(), TimerSignalHeartbeat()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timersignalattimevbl
|||	TimerSignalAtTimeVBL - Send a signal at the specified VBL.
|||
|||	  Synopsis
|||
|||	    TimerHandle TimerSignalAtTimeVBL(int32 signal, uint32 fields)
|||
|||	  Description
|||
|||	    This function creates a timer that notifies you by signal at the specified
|||	    VBL time.  The timer is a one-shot; after sending a notification message
|||	    the timer enters a suspended state.  Use TimerReset() to reactivate the
|||	    timer using new time values, or TimerCancel() to delete the timer.
|||
|||	    The TimerServices thread must be active before calling this function.
|||
|||	  Arguments
|||
|||	    signal                       The signal to send to your task.
|||
|||	    fields                       The VBL field count at which the event
|||	                                 occurs.
|||
|||	  Return Value
|||
|||	    A TimerHandle (a positive value) on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerSignalAfterDelayVBL(), TimerSignalHeartbeatVBL()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timersignalheartbeat
|||	TimerSignalHeartbeat - Send a signal periodically.
|||
|||	  Synopsis
|||
|||	    TimerHandle TimerSignalHeartbeat(int32 signal, uint32 seconds,
|||	                                     uint32 microseconds)
|||
|||	  Description
|||
|||	    This function creates a timer that repeatedly notifies you by signal at
|||	    the specified intervals..  The timer continues sending notifications on a
|||	    regular interval until you use TimerSuspend() to temporarily halt it or
|||	    TimerCancel() to delete.
|||
|||	    The Heartbeat support in the TimerServices thread contains logic to
|||	    prevent drift in the frequency of timer events.  Signals are delivered at
|||	    the specified frequency even if it takes you significant time to respond
|||	    to each signal event.
|||
|||	    The TimerServices thread must be active before calling this function.
|||
|||	  Arguments
|||
|||	    signal                       The signal to send to your task.
|||
|||	    seconds                      The seconds portion of the frequency.
|||
|||	    microseconds                 The microseconds portion of the frequency.
|||	                                 This value can be larger than 1 million.
|||
|||	  Return Value
|||
|||	    A TimerHandle (a positive value) on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerSignalAtTime(), TimerSignalAfterDelay(), HZ_TO_USEC()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/timersignalheartbeatvbl
|||	TimerSignalHeartbeatVBL - Send a signal periodically based on VBLs.
|||
|||	  Synopsis
|||
|||	    TimerHandle TimerSignalHeartbeatVBL(int32 signal, uint32 fields)
|||
|||	  Description
|||
|||	    This function creates a timer that repeatedly notifies you by signal after
|||	    the specified number of VBLs have occurred.  The timer continues sending
|||	    notifications on a regular interval until you use TimerSuspend() to
|||	    temporarily halt it or TimerCancel() to delete it.
|||
|||	    The Heartbeat support in the TimerServices thread contains logic to
|||	    prevent drift in the frequency of timer events.  Signals are delivered at
|||	    the specified frequency even if it takes you significant time to respond
|||	    to each signal event.
|||
|||	    The TimerServices thread must be active before calling this function.
|||
|||	  Arguments
|||
|||	    signal                       The signal to send to your task.
|||
|||	    fields                       The number of VBLs to occur between each
|||	                                 event.
|||
|||	  Return Value
|||
|||	    A TimerHandle (a positive value) on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, timerutils.h
|||
|||	  See Also
|||
|||	    TimerSignalAtTimeVBL(), TimerSignalAfterDelayVBL()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/unloadanim
|||	UnloadAnim - Unload an animation
|||
|||	  Synopsis
|||
|||	    void UnloadAnim (ANIM * animPtr)
|||
|||	  Description
|||
|||	    Unloads an animation file previously loaded with LoadAnim(), and frees all
|||	    resources acquired during the load process.
|||
|||	  Arguments
|||
|||	    animPtr                      Pointer to an ANIM structure which was
|||	                                 returned from the LoadAnim() function.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, parse3do.h
|||
|||	  See Also
|||
|||	    LoadAnim()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/unloadcel
|||	UnloadCel - Unload a cel file loaded by LoadCel().
|||
|||	  Synopsis
|||
|||	    void UnloadCel(CCB *cel)
|||
|||	  Description
|||
|||	    This function releases all resources acquired when a cel was loaded via
|||	    LoadCel().  A NULL pointer is valid as input, and is quietly treated as a
|||	    no-op.
|||
|||	    UnloadCel() just calls DeleteCel(); it differs only in that it doesn't
|||	    return a NULL pointer.
|||
|||	    DeleteCel() and DeleteCelList() may also be used on files loaded via
|||	    LoadCel().
|||
|||	  Arguments
|||
|||	    cel                          A pointer to a cel loaded via LoadCel().
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    LoadCel(), DeleteCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/unloadfile
|||	UnloadFile - Unload a file
|||
|||	  Synopsis
|||
|||	    void UnloadFile (void *buffer)
|||
|||	  Description
|||
|||	    Unloads a file previously loaded with LoadFile(), and frees all resources
|||	    acquired during the load process.
|||
|||	  Arguments
|||
|||	    buffer                       Pointer to a file buffer which was returned
|||	                                 from the LoadFile() function.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, blockfile.h
|||
|||	  See Also
|||
|||	    LoadFile()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/unloadfont
|||	UnloadFont - Release font resources.
|||
|||	  Synopsis
|||
|||	    void UnloadFont(FontDescriptor * fDesc)
|||
|||	  Description
|||
|||	    Releases resources acquired during LoadFont() or ParseFont() processing.
|||	    If the font was loaded via LoadFont(), releases the FontDescriptor and
|||	    unloads the file image, releasing the memory it occupied. If you loaded
|||	    the font file image yourself and called ParseFont(),UnloadFont() releases
|||	    the FontDescriptor. In that case, you must unload/release the font file
|||	    image yourself.
|||
|||	    Because TextCel structures contain a reference to a FontDescriptor, you
|||	    must delete all TextCels that reference a FontDescriptor before you call
|||	    UnloadFont()for that descriptor.
|||
|||	  Arguments
|||
|||	    FontDescriptor               Pointer to a FontDescriptor structure.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, fontlib.h
|||
|||	  See Also
|||
|||	    LoadFont(), ParseFont()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/unloadimage
|||	UnloadImage - Unload an image
|||
|||	  Synopsis
|||
|||	    void UnloadImage (void *imageBuffer)
|||
|||	  Description
|||
|||	    Unloads an image file previously loaded with LoadImage(), and frees all
|||	    resources acquired during the load process.
|||
|||	    Call this function only if you allowed LoadImage() to allocate the
|||	    destination buffer for you by passing a NULL buffer pointer to
|||	    LoadImage(). If you allocated your own image buffer and passed it to
|||	    LoadImage(), you must free the buffer yourself.
|||
|||	  Arguments
|||
|||	    imageBuffer                  Pointer to a buffer which was allocated
|||	                                 previously by the LoadImage() function.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, parse3do.h
|||
|||	  See Also
|||
|||	    LoadImage()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/updatetextincel
|||	UpdateTextInCel - Render text into a text cel.
|||
|||	  Synopsis
|||
|||	    Err UpdateTextInCel(TextCel *tCel, Boolean replaceExisting,
|||	                        char *fmtString, ...)
|||
|||	  Description
|||
|||	    Renders text into a cel.  If the cel has a format buffer attached to it,
|||	    the fmtString and related arguments pass first through sprintf() for
|||	    formatting.
|||
|||	    If replaceExisting is TRUE, the new text replaces any text already in the
|||	    cel. The cel data buffer is zeroed, the pen position within the cel data
|||	    buffer is set back to the top- and left-margin values, and the new text is
|||	    rendered.
|||
|||	    If replaceExisting is FALSE, the new text is added to the text already in
|||	    the cel, using the cel's current pen position.  This effectively adds
|||	    new characters to the end of the text already in the cel.
|||
|||	    If you created the cel with a width or height of zero (an autosize cel),
|||	    its size is adjusted to fit the formatted text exactly, if the
|||	    replaceExisting flag is TRUE. If the replaceExisting flag is FALSE for an
|||	    autosize cel, the net effect is that no characters are rendered.  This
|||	    happens  because the cel was sized exactly right for the characters
|||	    rendered last time replaceExisting was TRUE. Attempting to add more
|||	    characters to the cel always results in those characters falling outside
|||	    the cel's boundaries, where they will be clipped.
|||
|||	    The related vUpdateTextInCel() function works exactly the same, but takes
|||	    a va_list type instead of the ... parameters.
|||
|||	  Arguments
|||
|||	    tCel                         Pointer to a TextCel structure.
|||
|||	    replaceExisting              Flag indicating whether the new text is
|||	                                 added to or replaces text already in the
|||	                                 cel.
|||
|||	    fmtString                    The string of characters to be rendered,
|||	                                 optionally including printf-style %
|||	                                 formatting commands.
|||
|||	    ...                          Arguments for printf-style formatting.
|||	                                 (Optional.)
|||
|||	  Return Value
|||
|||	    Zero if all characters were rendered, positive if any characters were
|||	    clipped, negative if a bigger data buffer could not be allocated for an
|||	    autosize cel. If the cel is not an autosize type, it can\xd5 t return a
|||	    negative value.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, textlib.h
|||
|||	  See Also
|||
|||	    vUpdateTextInCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/vgettextextent
|||	vGetTextExtent - Get text width and height if formatted using format
|||	                 arguments.
|||
|||	  Synopsis
|||
|||	    char * vGetTextExtent(TextCel *tCel,
|||	                          int32 *pWidth, int32 *pHeight,
|||	                          char *fmtString, va_list fmtArgs)
|||
|||	  Description
|||
|||	    Identical to GetTextExtent() except that it takes a va_list rather than a
|||	    variable argument list for printf-style formatting.
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/vupdatetextincel
|||	vUpdateTextInCel - Render into text cel using printf-style formatting.
|||
|||	  Synopsis
|||
|||	    Err vUpdateTextInCel(TextCel *tCel, Boolean replaceExisting,
|||	                         char *fmtString, va_list fmtArgs)
|||
|||	  Description
|||
|||	    Identical to UpdateTextInCel() except that it takes a va_list rather than
|||	    a variable argument list for printf-style formatting.
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/waitasyncloadfile
|||	WaitAsyncLoadFile - Wait for an async file load operation to complete.
|||
|||	  Synopsis
|||
|||	    Err WaitAsyncLoadFile(LoadFileInfo *lf)
|||
|||	  Description
|||
|||	    This function waits for completion of an async file load started earlier.
|||
|||	    Do not call FinishAsyncLoadFile() after calling this function; this
|||	    function performs the Finish actions internally.
|||
|||	  Arguments
|||
|||	    lf                           A pointer to the LoadFileInfo structure for
|||	                                 the load operation.
|||
|||	  Return Value
|||
|||	    Returns zero on success or a negative error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in lib3do.lib.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, blockfile.h
|||
|||	  See Also
|||
|||	    AsyncLoadFile()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/writemacfile
|||	WriteMacFile - Write from buffer to Mac file
|||
|||	  Synopsis
|||
|||	    long WriteMacFile (char *filename, char *buf, long count)
|||
|||	  Description
|||
|||	    Writes count bytes from the specified buffer into the file named by
|||	    filename, creating the MAC file, writing the specified number of bytes,
|||	    and closing the Macintosh file. Returns the number of bytes written to the
|||	    Macintosh file.
|||
|||	  Arguments
|||
|||	    filename                     pointer to char string specifying the name
|||	                                 of the file to write on the Macintosh.
|||
|||	    buf                          pointer to the data bytes to write.
|||
|||	    count                         number of bytes to write.
|||
|||	  Associated Files
|||
|||	    lib3do.lib, utils3do.h
|||
|||	  Return Value
|||
|||	    The number of bytes written to the Macintosh file.
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/xcornerfromsrect
|||	XCORNERFROMSRECT - Calculate the opposite corner X location for an SRect.
|||
|||	  Synopsis
|||
|||	    int32 XCORNERFROMSRECT(SRect *rect)
|||
|||	  Description
|||
|||	    This macro evaluates to the X location for the corner opposite to the
|||	    corner described by rect->pos.
|||
|||	    This macro evaluates its argument more than once; be careful of side
|||	    effects.  It is usable in expression context.
|||
|||	  Arguments
|||
|||	    rect                         A pointer to an SRect.
|||
|||	  Return Value
|||
|||	    The X location of the opposite corner.
|||
|||	  Implementation
|||
|||	    Macro
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    ICornerFromSRect(), YCORNERFROMSRECT()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/xsizefromcrect
|||	XSIZEFROMCRECT - Calculate the X coordinate size for a CRect.
|||
|||	  Synopsis
|||
|||	    CCB * XSIZEFROMCRECT(CRect *rect)
|||
|||	  Description
|||
|||	    This macro evaluates to the size in the X dimension of the area described
|||	    by rect.
|||
|||	    This macro evaluates its argument more than once; be careful of side
|||	    effects.  It is usable in expression context.
|||
|||	  Arguments
|||
|||	    rect                         A pointer to a CRect.
|||
|||	  Return Value
|||
|||	    The size of the area in the X dimension.
|||
|||	  Implementation
|||
|||	    Macro
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    ISizeFromCRect(), YSIZEFROMCRECT()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/ycornerfromsrect
|||	YCORNERFROMSRECT - Calculate the opposite corner Y location for an SRect.
|||
|||	  Synopsis
|||
|||	    CCB * YCORNERFROMSRECT(SRect *rect)
|||
|||	  Description
|||
|||	    This macro evaluates to the Y location for the corner opposite to the
|||	    corner described by rect->pos.
|||
|||	    This macro evaluates its argument more than once; be careful of side
|||	    effects.  It is usable in expression context.
|||
|||	  Arguments
|||
|||	    rect                         A pointer to an SRect.
|||
|||	  Return Value
|||
|||	    The Y location of the opposite corner.
|||
|||	  Implementation
|||
|||	    Macro
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    ICornerFromSRect(), XCORNERFROMSRECT()
|||
**/

/**
|||	AUTODOC PUBLIC spg/lib3do/ysizefromcrect
|||	YSIZEFROMCRECT - Calculate the Y coordinate size for a CRect.
|||
|||	  Synopsis
|||
|||	    CCB * YSIZEFROMCRECT(CRect *rect)
|||
|||	  Description
|||
|||	    This macro evaluates to the size in the Y dimension of the area described
|||	    by rect.
|||
|||	    This macro evaluates its argument more than once; be careful of side
|||	    effects.  It is usable in expression context.
|||
|||	  Arguments
|||
|||	    rect                         A pointer to an SRect.
|||
|||	  Return Value
|||
|||	    The size of the area in the Y dimension.
|||
|||	  Implementation
|||
|||	    Macro
|||
|||	  Associated Files
|||
|||	    lib3do.lib, celutils.h
|||
|||	  See Also
|||
|||	    ISizeFromCRect(), XSIZEFROMCRECT()
|||
**/

/* keep the compiler happy... */
extern int foo;
