/* $Id: autodocs.c,v 1.4 1994/11/02 20:37:11 vertex Exp $ */

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/centercelonscreen
|||	CenterCelOnScreen - Center a cel on the screen
|||
|||	  Synopsis
|||
|||	    void CenterCelOnScreen (CCB *ccb)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases. Use
|||	    CenterRectCelInDisplay() as a replacement.
|||
|||	    Centers the cel named by ccb.  This function calls MapCel() to set
|||	    the relevant ccb fields.  This function also assumes your screen
|||	    dimensions are 320 * 240.
|||
|||	  Arguments
|||
|||	    ccb                          Pointer to the CCB you want to center.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
|||	  See Also
|||
|||	    SetQuad(), SetRectf16(), CenterRectf16(), CenterRectCelInDiplay()
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/centerrectf16
|||	CenterRectf16 - Center a rectangle in a frame
|||
|||	  Synopsis
|||
|||	    void CenterRectf16 (Point *q, Rectf16 *rect, Rectf16 *frame)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Sets the coordinates of the four points q[0] through q[3]. Determines
|||	    these coordinates by centering the Rectf16 rectangle within the Rectf16
|||	    frame. The Rectf16 structure has the following definition:
|||
|||	    typedef struct tag_Rectf16 {
|||	        frac16 rectf16_XLeft;
|||	        frac16 rectf16_YTop;
|||	        frac16 rectf16_XRight;
|||	        frac16 rectf16_YBottom;
|||	    } Rectf16;
|||
|||	  Arguments
|||
|||	    q                            Pointer to an array of 4 points.
|||
|||	    rect                         Pointer to a Rectf16 structure.
|||
|||	    frame                        Pointer to a Rectf16 structure.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
|||	  See Also
|||
|||	    SetQuad, SetRectf16, CenterCelOnScreen, CenterCRectInCRect,
|||	    CenterCRectInDisplay, CenterCRectOverIPoint, CenterSRectInSRect,
|||	    CenterSRectInDisplay, CenterSRectOverIPoint
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/fadeincel
|||	FadeInCel - Fade in a cel
|||
|||	  Synopsis
|||
|||	    bool FadeInCel (CCB *ccb, CCB *maskccb, int32 *stepValue)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Calls SetCelScale() to set up the parameters for the next step in the
|||	    fade-in of the cel and then increments stepValue. To fade in a cel, call
|||	    SetFadeInCel() to initialize the CCBs, then iteratively call FadeInCel()
|||	    and DrawCels() MAX_SCALE times. MAX_SCALE is defined in utils3do.h.
|||
|||	  Arguments
|||
|||	    ccb                          Pointer to the CCB of the cel to fade in.
|||
|||	    maskccb                      Pointer to the scratch CCB already
|||	                                 initialized by SetFadeInCel().
|||
|||	    stepValue                    Pointer to an int32 which will be
|||	                                 incremented.
|||
|||	  Return Value
|||
|||	    TRUE if successful, FALSE if not. Returns FALSE when stepValue reaches
|||	    MAX_SCALE.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
|||	  See Also
|||
|||	    SetCelScale(), SetFadeInCel(), SetFadeOutCel(), FadeOutCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/fadeoutcel
|||	FadeOutCel - Fade out a cel
|||
|||	  Synopsis
|||
|||	    bool FadeOutCel (CCB *ccb, CCB *maskccb, int32 *stepValue)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Calls SetCelScale() to set up the parameters for the next step in the
|||	    fade-out of the cel and then increments stepValue. To fade out a cel, call
|||	    SetFadeInCel() to initialize the CCBs, then iteratively call FadeOutCel()
|||	    and DrawCels() MAX_SCALE times.
|||
|||	  Arguments
|||
|||	    ccb                          Pointer to a CCB.
|||
|||	    maskccb                      Pointer to the scratch CCB already
|||	                                 initialized by SetFadeOutCel().
|||
|||	    stepValue                    Pointer to an int32 which will be
|||	                                 incremented.
|||
|||	  Return Value
|||
|||	    TRUE if successful, FALSE if not.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
|||	  See Also
|||
|||	    SetCelScale(), SetFadeInCel(), FadeInCel(), SetFadeOutCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/framebuffertocel
|||	FrameBufferToCel - Convert a frame buffer image to a cel
|||
|||	  Synopsis
|||
|||	    bool FrameBufferToCel (Item iScreen, CCB* cel)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||	    Use CreateLRFormCel() as a replacement.
|||
|||	  Return Value
|||
|||	    TRUE if successful, FALSE if not.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/free
|||	Free - Free memory allocated by Malloc
|||
|||	  Synopsis
|||
|||	    void * Free (void *block)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Frees a block of memory previously allocated via Malloc(). Do not use this
|||	    function to free blocks of memory allocated by any method other than
|||	    Malloc().
|||
|||	  Arguments
|||
|||	    block                        Pointer to the memory block to be freed.
|||
|||	  Return Value
|||
|||	    Always returns NULL.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, umemory.h
|||
|||	  See Also
|||
|||	    Malloc(), MemBlockSize()
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/freebuffer
|||	FreeBuffer - Free a buffer allocated by LoadAnim(), LoadImage(), or
|||	             LoadCel()
|||
|||	  Synopsis
|||
|||	    void FreeBuffer (char *filename, long *fileBuffer)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    This function is obsolete
|||
|||	  Arguments
|||
|||	    filename                     Address of a character string which
|||	                                 specifies a filename.
|||
|||	    filebuffer                   Pointer to the memory buffer being released.
|||	                                 Assumes the buffer is the same size as the
|||	                                 file specified by filename.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
|||	  See Also
|||
|||	    LoadAnim(), LoadCel(), LoadImage()
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/loadsoundeffect
|||	LoadSoundEffect - Load a sound effect
|||
|||	  Synopsis
|||
|||	    Item LoadSoundEffect (char* sFilename, int nNumVoices)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Loads a sound effect from the specified file and assigns it nNumVoices. It
|||	    does this by loading the Instrument template "sampler.ofx",
|||	    allocating an instrument for the sound effect, loading the sample
|||	    specified by sFilename, and then attaching the sample to the allocated
|||	    instrument.
|||
|||	    This function is being pased out and will not appear in future releases of
|||	    the library. It uses global variables and inappropriate audio folio
|||	    techniques wwhich result in audio pops and clicks. The tsc_soundfx source
|||	    code in the Examples folder demonstrates proper techniques for managing
|||	    sound effects.
|||
|||	  Arguments
|||
|||	    sFilename                    pointer to character string containing the
|||	                                 path name of an audio sample file in AIFF
|||	                                 format.
|||
|||	    nNumVoices                   number of voices to assign to this sound
|||	                                 effect. 1 for Mono, 2 for stereo.
|||
|||	  Return Value
|||
|||	    The item representing the allocated instrument.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, parse3do.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/makenewcel
|||	MakeNewCel - Create a new cel of specified size
|||
|||	  Synopsis
|||
|||	    CCB * MakeNewCel (Rectf16 *r)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases. Use
|||	    CreateCel() or CreateBackdropCel() as a replacement.
|||
|||	    Creates a new cel with default CCB values and fills it with white
|||	    pixels.
|||
|||	  Arguments
|||
|||	    r                            Pointer to a Rectf16 which specifies the
|||	                                 size of the cel.
|||
|||	  Return Value
|||
|||	    A pointer to the CCB of the cel that was created.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/malloc
|||	Malloc - Allocate a block of memory
|||
|||	  Synopsis
|||
|||	    void * Malloc (uint32 size, uint32 memTypeBits)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Allocates a block of memory of the requested size and type.
|||
|||	    This function supports allocation of all memory types defined in the
|||	    system header file mem.h, including valid combinations of types (for
|||	    example, MEMTYPE_CEL|MEMTYPE_FILL|0xFF to allocate cel memory which is
|||	    pre-initialized to contain 0xFF in all bytes). The umemory.h header file
|||	    automatically includes the mem.h header file for you.
|||
|||	    This function correctly handles page-aligned VRAM allocations, returning a
|||	    pointer to the beginning of a VRAM memory page.
|||
|||	    A block of memory allocated with this function must be freed with the
|||	    Free() -- not free() or FreeMem() -- function.
|||
|||	  Arguments
|||
|||	    size                         Size (in bytes) of the block to be
|||	                                 allocated.
|||
|||	    memTypeBits                  Type of memory to allocate.
|||
|||	  Return Value
|||
|||	    A pointer to the allocated block, or NULL if the memory couldn't be
|||	    allocated.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, umemory.h
|||
|||	  See Also
|||
|||	    Free(), MemBlockSize()
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/mapp2cel
|||	MapP2Cel - Optimized version of MapCel()
|||
|||	  Synopsis
|||
|||	    void MapP2Cel (Point *q, CCB *ccb)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Sets the CCB fields of a cel to draw that cel with its four corners
|||	    specified by the array of four points addressed by q. Assumes that both
|||	    the width and height of the cel are integral powers of 2. This is a
|||	    special case of the more general MapCel()call in the graphics folio,
|||	    optimized for power of 2 cels.
|||
|||	    This routine sets the CCB fields ccb_X, ccb_Y, ccb_HDX, ccb_HDY, ccb_VDX,
|||	    ccb_VDY, ccb_DDX, and ccb_DDY. MapP2Cel() achieves its optimizations by
|||	    performing right shifts instead of divides.
|||
|||	  Arguments
|||
|||	    q                            Pointer to an array of four points which
|||	                                 specify the destination coordinates of the
|||	                                 cel.
|||
|||	    ccb                          Pointer to the CCB for the cel. The cel\xd5
|||	                                 s width and height must both be integral
|||	                                 powers of 2 for this function to work
|||	                                 properly.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
|||	  See Also
|||
|||	    MapCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/movecel
|||	MoveCel - Move a cel
|||
|||	  Synopsis
|||
|||	    void MoveCel (CCB *ccb, MoveRec *pMove)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Moves a cel from beginQuad to endQuad over numberOfFrames, using the
|||	    MoveRec structure defined by PreMoveCel(). Each time MoveCel() is called,
|||	    the increments calculated in PreMoveCel() are added to the current corner
|||	    values and the results are applied to the corners of the cel using
|||	    MapCel().
|||
|||	    MoveCel() does not draw the cel into a frame buffer.
|||
|||	  Arguments
|||
|||	    ccb                          Pointer to the CCB for the cel you want to
|||	                                 move. This CCB's position fields will be
|||	                                 updated
|||
|||	    pMove                        Pointer to the MoveRec structure that
|||	                                 contains the increments and to the current
|||	                                 quad.
|||
|||	        The MoveRec structure is defined as:
|||
|||	                typedef struct tag_MoveRec {
|||	                        MoveVect        curQuadf16[4];  // the current coords for the cel
|||	                        MoveVect        quadIncr[4];    // x and y increments for the corners
|||	                } MoveRec
|||
|||	        This structure, in turn, uses the MoveVect structure, which is defined as
|||	        follows:
|||
|||	                typedef struct tag_MoveVect {
|||	                        frac16 xVector;
|||	                        frac16 yVector;
|||	                } MoveVect
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
|||	  See Also
|||
|||	    PreMoveCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/openaudio
|||	OpenAudio - Opens the audio folio and loads a default mixer
|||
|||	  Synopsis
|||
|||	    bool OpenAudio( void )
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases. Use
|||	    OpenAudioFolio() instead.
|||
|||	    OpenAudio() initializes the audio folio, opening it and loading a default
|||	    mixer.It initializes the global variable TheMixer to make it accessible by
|||	    other applications. TheMixer provides four "voices," each with two
|||	    separate audio channels.
|||
|||	  Return Value
|||
|||	    TRUE if successful, FALSE if not.
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/openmaclink
|||	OpenMacLink - Open the MAC device
|||
|||	  Synopsis
|||
|||	    bool OpenMacLink (void)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases. The
|||	    function does absolutely nothing.
|||
|||	  Return Value
|||
|||	    Returns TRUE.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, init3do.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/opensport
|||	OpenSPORT - Set up items for the SPORT device
|||
|||	  Synopsis
|||
|||	    bool OpenSPORT (void)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases. The
|||	    function does absolutely nothing.
|||
|||	  Return Value
|||
|||	    Returns TRUE.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, init3do.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/premovecel
|||	PreMoveCel - Set up a cel for a MoveCel() call
|||
|||	  Synopsis
|||
|||	    void PreMoveCel (CCB *ccb,
|||	                     Point *beginQuad, Point *endQuad,
|||	                     int32 numberOfFrames, MoveRec *pMove)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Calculates the increments for each corner of a cel you want to move from
|||	    the quadrilateral defined by beginQuad to the quadrilateral defined by
|||	    endQuad. The numberOfFrames parameter specifies how many iterations are
|||	    needed to complete the movement.
|||
|||	  Arguments
|||
|||	    ccb                          The CCB to be moved.
|||
|||	    beginQuad                    Four coordinates that define the starting
|||	                                 position of the cel.
|||
|||	    endQuad                      Four coordinates that define the ending
|||	                                 position of the cel.
|||
|||	    numberOfFrames               Number of iterations to go from beginning to
|||	                                 end. Should be between 1 and 32767.
|||
|||	    pMove                        Pointer to the MoveRec to contain increments
|||	                                 and the quadrilateral defined by beginQuad
|||	                                 and endQuad.
|||
|||	    A MoveRec structure is defined as follows:
|||
|||	                typedef struct tag_MoveRec {
|||	                        MoveVect        curQuadf16[4];  // the current coords for the cel
|||	                        MoveVect        quadIncr[4];    // x and y increments for the corners
|||	                } MoveRec
|||
|||	    This structure, in turn, uses the MoveVect structure, which is defined as
|||	    follows:
|||
|||	                typedef struct tag_MoveVect {
|||	                        frac16 xVector;
|||	                        frac16 yVector;
|||	                } MoveVect
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
|||	  See Also
|||
|||	    MoveCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/readcontrolpad
|||	ReadControlPad - Return the state of the first controlpad
|||
|||	  Synopsis
|||
|||	    long ReadControlPad (long lControlMask)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Returns the state of the first control pad:Returns the current state of
|||	    the button for all bits that have a 1 in the corresponding position of the
|||	    control mask. Reports only down transitions for bits which have a 0 in the
|||	    control mask.
|||
|||	    The mask functions as a report continuous mask. Relevant constants are
|||	    defined in hardware.h
|||
|||	        // === JOYSTICK/JOYSTICK1 flags ===
|||	        #define JOYSTART        0x00000080
|||	        #define JOYFIREC        0x00000040
|||	        #define JOYFIREA        0x00000020
|||	        #define JOYFIREB        0x00000010
|||	        #define JOYDOWN         0x00000008
|||	        #define JOYUP           0x00000004
|||	        #define JOYRIGHT        0x00000002
|||	        #define JOYLEFT         0x00000001
|||	        #define JOYSELECT       JOYFIREC
|||	        #define JOYMOVE         (JOYLEFT+JOYRIGHT+JOYUP+JOYDOWN)
|||	        #define JOYBUTTONS      (JOYFIREA+JOYFIREB+JOYFIREC+JOYSTART)
|||
|||	  Arguments
|||
|||	    lControlMask                 A long word control mask.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/reportmemoryusage
|||	ReportMemoryUsage - Free memory allocated by Malloc  #### SAY WHAT?
|||
|||	  Synopsis
|||
|||	    void ReportMemoryUsage (void)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases. Use
|||	    AvailMem() as a replacement.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, umemory.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/setcelscale
|||	SetCelScale - Called by FadeInCel() and FadeOutCel()
|||
|||	  Synopsis
|||
|||	    void SetCelScale (CCB *ccb, CCB *maskccb, int32 stepValue)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    SetCelScale() sets up the multipliers and dividers of both ccb and maskccb
|||	    to the level determined by stepValue. This provides a finer grained fade
|||	    in by drawing the cel twice, once with ccb and once with maskccb.
|||
|||	    This function assumes that the cel has a checkerboard or random dither
|||	    P-Mode mask. The high and low words of the PPMPC are updated alternately.
|||	    If the cel does not have a per pixel P-Mode mask, it fades in on even
|||	    steps only.
|||
|||	    This function is used by FadeInCel() and FadeOutCel(), and is not normally
|||	    called by the user.
|||
|||	  Arguments
|||
|||	    ccb                          Pointer to the CCB to be faded in.
|||
|||	    maskccb                      Pointer to a CCB. This CCB should be
|||	                                 previously initialized by SetFadeInCel() to
|||	                                 point to the same pixel data as ccb.
|||
|||	    stepValue                    Determines which scalar values entry to
|||	                                 apply to the cel. The value is between 0 and
|||	                                 22.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
|||	  See Also
|||
|||	    SetFadeInCel(), SetFadeInCel(), SetFadeOutCel(), FadeOutCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/setchannel
|||	SetChannel - Assign an instrument to an audio channel
|||
|||	  Synopsis
|||
|||	    bool SetChannel (Item iInstrument, int nChannel)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Assigns an instrument to an audio channel.
|||
|||	  Arguments
|||
|||	    iInstrument                  Item descriptor for an instrument. Usually
|||	                                 obtained from LoadSoundEffect().
|||
|||	    nChannel                     The channel to use.
|||
|||	  Return Value
|||
|||	    TRUE if successful, FALSE if not.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
|||	  See Also
|||
|||	    LoadSoundEffect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/setfadeincel
|||	SetFadeInCel - Set up a cel for a FadeInCel() call
|||
|||	  Synopsis
|||
|||	    void SetFadeInCel (CCB *ccb, CCB *maskccb, int32 *stepValue)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Sets up a cel for a FadeInCel() call as follows:copies the CCB data from
|||	    ccb into maskccbinitializes the relevant CCB control fields in ccb and
|||	    maskccbinitializes stepValue to 0
|||
|||	    SetFadeInCel() also links maskccb to cbb so that drawing ccb is
|||	    automatically followed by drawing maskccb. The two CCBs together allow a
|||	    finer grained fade-in than is possible with a single CCB.
|||
|||	  Arguments
|||
|||	    ccb                          Pointer to the CCB of a cel to be faded in.
|||
|||	    maskccb                      Pointer to a scratch CCB.
|||
|||	    stepValue                    Pointer to an Int32 used to hold the current
|||	                                 step value index.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
|||	  See Also
|||
|||	    SetCelScale(), FadeInCel(), SetFadeOutCel(), FadeOutCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/setfadeoutcel
|||	SetFadeOutCel - Set up a cel for a FadeOutCel() call
|||
|||	  Synopsis
|||
|||	    void SetFadeOutCel (CCB *ccb, CCB *maskccb, int32 *stepValue)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Sets up the cel ccb points to for the FadeOutCel() function:
|||
|||	        copies the CCB data from ccb into maskccb
|||	        sets up the proper CCB control fields in the two CCBs
|||	        initializes stepValue to 0
|||
|||	    This function also links maskccb to cbb, so that drawing cbb will
|||	    automatically draw maskccb in succession. The two CCBs together allow a
|||	    finer grained fade-in than a single CCB would allow.
|||
|||	  Arguments
|||
|||	    ccb                          Pointer to the CCB of a cel to be faded out.
|||
|||	    maskccb                      Pointer to a scratch CCB.
|||
|||	    stepValue                    Pointer to an Int32 used to hold the current
|||	                                 stepValue index.
|||
|||	  Return Value
|||
|||	    TRUE if successful, FALSE if not.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
|||	  See Also
|||
|||	    SetCelScale(), SetFadeInCel(), FadeInCel(), FadeOutCel()
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/setmixer
|||	SetMixer - Set volume and balance of an audio channel
|||
|||	  Synopsis
|||
|||	    bool SetMixer (int nChannel, int32 nVolume, int32 nBalance)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Sets the overall volume and balance of a given audio channel.
|||
|||	  Arguments
|||
|||	    nChannel                     Audio channel to set.
|||
|||	    nVolume                      Overall volume for this channel. Ranges from
|||	                                 0 to 0x7FFF. The volume passed in is
|||	                                 actually divided by four, assuming a 4x2
|||	                                 mixer.
|||
|||	    nBalance                     Balance setting for this channel. 0x4000 is
|||	                                 the mid point. 0x7FFF is full right.
|||
|||	  Return Value
|||
|||	    TRUE if successful, FALSE if not.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/setquad
|||	SetQuad - Set a Quad defined by the specified coordinates
|||
|||	  Synopsis
|||
|||	    void SetQuad (Point *r, Coord left, Coord top,
|||	                  Coord right, Coord bottom)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Set an array of four points (a quad) to conform to the rectangle defined
|||	    by the values of left, top, right, and bottom.
|||
|||	  Arguments
|||
|||	    r                            pointer to an array of four Point
|||	                                 structures.
|||
|||	    left                         Coordinate of the left edge of the
|||	                                 rectangle.
|||
|||	    top                          Coordinate of the top edge of the rectangle.
|||
|||	    right                        Coordinate of the right edge of the
|||	                                 rectangle.
|||
|||	    bottom                       Coordinate of the bottom edge of the
|||	                                 rectangle.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
|||	  See Also
|||
|||	    SetRect()
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/setrectf16
|||	SetRectf16 - Convert integer coordinates to 16.16 coordinates
|||
|||	  Synopsis
|||
|||	    void SetRectf16 (Rectf16 *r, Coord left, Coord top,
|||	                     Coord right, Coord bottom)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases.
|||
|||	    Sets the four corners of the specified rectangle to be fixed point 16.16
|||	    conversions of the four integer coordinates passed to the function. The
|||	    Rectf16 structure is defined as follows:
|||
|||	                typedef struct tag_Rectf16 {
|||	                        frac16  rectf16_XLeft;
|||	                        frac16  rectf16_YTop;
|||	                        frac16  rectf16_XRight;
|||	                        frac16  rectf16_YBottom;
|||	                } Rectf16;
|||
|||	  Arguments
|||
|||	    r                            Pointer to a Rectf16 structure.
|||
|||	    left                         Integer Coord.
|||
|||	    top                          Integer Coord.
|||
|||	    right                        Integer Coord.
|||
|||	    bottom                       Integer Coord.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, utils3do.h
|||
|||	  See Also
|||
|||	    SetQuad()
|||
**/

/**
|||	AUTODOC PUBLIC spg/obsoletelib3do/shutdown
|||	ShutDown - Shut down the system cleanly
|||
|||	  Synopsis
|||
|||	    void ShutDown (void)
|||
|||	  Description
|||
|||	    This function is obsolete and should not be used by new code.
|||	    It will be eliminated for future Portfolio releases. Use
|||	    CloseAudioFolio() as a replacement.
|||
|||	  Associated Files
|||
|||	    obsoletelib3do.lib, init3do.h
|||
|||	  See Also
|||
|||	    CloseAudioFolio()
|||
**/

/* keep the compiler happy... */
extern int foo;
