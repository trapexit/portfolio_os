/* $Id: audio_samples.c,v 1.88 1994/12/16 23:35:46 phil Exp $ */
/****************************************************************
**
** Audio Internals to support Samples
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/

/*
** 921116 PLB Do not error out on unrecognized AIFF chunks.
** 921118 PLB Move AIFF sample loading to User Level
**              Load entire sample then pass to CreateItem for blessing.
** 921203 PLB Fixed error trapping and reporting.
** 921207 PLB Free sample memory in User Mode.
** 921214 PLB Use Block I/O for reading sound file.
** 930127 PLB Return actual error in LoadSample.
** 930308 PLB Use new IFF parser to load AIFF
** 930415 PLB Track connections between Items
** 930504 PLB Add Sample Rate, and AF_TAG_SAMPLE_RATE
** 930506 PLB Allow sample Address changes.
** 930510 PLB Added AF_TAG_COMPRESSIONRATIO, TYPE and NUMBITS
** 930524 PLB Add LoadSampleHere
** 930527 PLB internalSetSampleInfo was returning a 1
** 930628 PLB Fixed reading of ReleaseEnd from Sample
** 930715 PLB Added asmp_SuperFlags, DelayLine
** 930824 PLB Changed debug and trace text.
** 930825 PLB Allow attached samples to use Custom Allocate and Free functions.
** 930830 PLB Disallow negative number of channels, free mem if delay line creation fails.
** 930907 PLB Don't attempt to free mem for delay lines in user mode.  Causes fence violation.
** 931129 PLB Use Detune in CalcSampleBaseFreq
** 931215 PLB Added CreateSample()
** 940304 PLB Set NumBytes before calling internalSetSampleInfo so Lemmings doesn't crash.
** 940429 PLB Allow illegal call to MakeSample() so MadDog McCree doesn't lose soundtrack.
** 940506 PLB Added support for changeable DAC sample rates.
** 940513 PLB DebugSample() now uses decimal for reporting most numbers.
** 940606 PLB Added \n to an error message.
** 940614 PLB Added miserable kludge for Mad Dog McCree's bug.  If they call MakeSample()
**            and request both an allocation and a specific address, then we do both
**            and then only use the allocated memory if they call GetAudioItemInfo()
**            and ask for the address back.  Otherwise we can assume the caller really
**            wanted the specified address.  Note the allocated memory does not get freed
**            unless they use it.  This is as before.  This change was made for ROM over CD
**            so that Mad Dog won't break under Anvil OS.
** 940727 WJB Added a quickie fix to permit ScanSample() to parse the header of an
*             AIFF with markers. [REMOVED] 
** 940809 PLB Reject a CompressionRatio<1 in SetSampleInfo, avoid zero divide
**            Removed quickie fix from 940727 because it allowed markers outside legal memory.
** 940811 PLB Used %.4s to print ChunkTypes instead of scratch array kludge.
** 940812 PLB Implement LEAVEINPLACE, Make LEAVEINPLACE illegal for files.
** 940817 PLB Shift Frequency by Shift Rate to compensate for execution rate.
** 940907 PLB Frame to byte conversions moved from dspp_touch.c 
** 940912 PLB Use Read16 to read uint16 values for NuPup
** 940921 PLB Handle AIFF files where MARK is after INST
** 941121 PLB In MakeSample(), set number of frames to match number of
**            bytes and default format.
** 941128 PLB Set UpdateSize if CompressionRatio set.
** 941128 PLB Recompute NumBits if width set.
** 941212 WJB Tweaked printed messages for AF_TAG_LEAVE_IN_PLACE.  
** 941213 WJB Added debug line in UnloadSample()
** 941216 PLB Fix double freemem for AF_TAG_LEAVE_IN_PLACE
*****************************************************************/

#include "audio_internal.h"
#include "filefunctions.h"

/* Macros for debugging. */
#define DBUG(x)    /* PRT(x) */
#define NODBUG(x)  /* */

/* Load AIFF sample file and fill fields */

static int32 SearchArray(int32 *Pnts, int32 NumPnts, int32 Value);

/*****************************************************************/
/***** USER Level Folio Calls ************************************/
/*****************************************************************/

 /**
 |||	AUTODOC PUBLIC spg/items/sample
 |||	Sample - A digital recording of a sound.
 |||
 |||	  Description
 |||
 |||	    A Sample Item is a handle to a digital recording of a sound in memory.
 |||	    Samples come in two kinds:
 |||	        . Ordinary Samples - sample loaded from a sample file, or
 |||	          created from scratch. User memory is used for samples. CreateSample()
 |||	          and a myriad of special purpose functions create ordinary samples.
 |||	        . Delay Lines - special samples suitable for receiving the DMA
 |||	          output from delay instruments (e.g. delaymono.dsp). The memory
 |||	          for delay lines is allocated in system memory. Use CreateDelayLine()
 |||	          to create a delay line.
 |||
 |||	    Most sample operations can be performed on both kinds of samples.
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_SAMPLE_NODE
 |||
 |||	  Create
 |||
 |||	    CreateDelayLine()
 |||
 |||	    CreateItem()
 |||
 |||	    CreateSample()
 |||
 |||	    DefineSampleHere()
 |||
 |||	    LoadSample()
 |||
 |||	    LoadSampleHere()
 |||
 |||	    ScanSample()
 |||
 |||	  Delete
 |||
 |||	    DeleteDelayLine()
 |||
 |||	    DeleteItem()
 |||
 |||	    UnloadSample()
 |||
 |||	  Query
 |||
 |||	    GetAudioItemInfo()
 |||
 |||	  Modify
 |||
 |||	    SetAudioItemInfo()
 |||
 |||	  Use
 |||
 |||	    AttachSample()
 |||
 |||	    DebugSample()
 |||
 |||	  Tags
 |||
 |||	    Loading:
 |||
 |||	    AF_TAG_NAME                  (const char *) Create. Name of sample file to
 |||	                                 load.
 |||
 |||	    AF_TAG_IMAGE_ADDRESS         (const char *) Create. Specifies a memory
 |||	                                 location containing a sample file image from
 |||	                                 which to read sample. Must use in conjunction
 |||	                                 with AF_TAG_IMAGE_LENGTH.
 |||
 |||	    AF_TAG_IMAGE_LENGTH          (uint32) Create. Specifies number of bytes in
 |||	                                 sample file image pointed to by
 |||	                                 AF_TAG_IMAGE_ADDRESS.
 |||
 |||	    AF_TAG_SCAN                  (int32) Create. Specifies the maximum number
 |||	                                 of bytes of sound data to read from the file.
 |||	                                 This can be used to cause the audio folio
 |||	                                 to simply load sample parameters without
 |||	                                 loading sample data.
 |||
 |||	    AF_TAG_ALLOC_FUNCTION        (void *(*)(uint32 memsize, uint32 memflags))
 |||	                                 Create. Sets custom memory allocation function
 |||	                                 to be called to allocate sample memory while
 |||	                                 loading sample file. Defaults to AllocMem().
 |||	                                 If you supply a custom allocation function you
 |||	                                 must also provide a custom free function with
 |||	                                 AF_TAG_FREE_FUNCTION.
 |||
 |||	    AF_TAG_FREE_FUNCTION         (void (*)(void *memptr, uint32 memsize))
 |||	                                 Create. Sets custom memory free function to
 |||	                                 free sample memory to be called during sample
 |||	                                 deletion. Defaults to FreeMem(). If you supply
 |||	                                 a custom free function you must also provide a
 |||	                                 custom allocation function with
 |||	                                 AF_TAG_ALLOC_FUNCTION.
 |||
 |||	    AF_TAG_LEAVE_IN_PLACE        (bool) When TRUE, causes sample being read
 |||	                                 from AF_TAG_IMAGE_ADDRESS to be used in-place
 |||	                                 instead of allocating more memory to hold
 |||	                                 the sample data. Mutually exclusive with
 |||	                                 AF_TAG_ALLOC_FUNCTION. See CreateSample()
 |||	                                 for more details and caveats.
 |||
 |||	    AF_TAG_DATA_OFFSET           (uint32) Query. Byte offset in sample file
 |||	                                 of the beginning of the sample data.
 |||
 |||	    AF_TAG_DATA_SIZE             (uint32) Query. Size of sample data in
 |||	                                 sample file in bytes. Note that this may
 |||	                                 differ from the length of the sample as loaded
 |||	                                 into memory (as returned by AF_TAG_NUMBYTES).
 |||
 |||	    Data:
 |||
 |||	    AF_TAG_ADDRESS               (void *) Create, Query, Modify. Address of
 |||	                                 sample data.
 |||
 |||	                                 This tag, and the all the other Data tags, can
 |||	                                 be used to query or modify the data
 |||	                                 address/length of ordinary samples. They can
 |||	                                 only be used to query the address/length of a
 |||	                                 delay line.
 |||
 |||	    AF_TAG_FRAMES                (uint32) Create, Query, Modify. Length of
 |||	                                 sample data expressed in frames. In a stereo
 |||	                                 sample, this would be the number of stereo
 |||	                                 pairs.
 |||
 |||	    AF_TAG_NUMBYTES              (uint32) Create, Query, Modify. Length of
 |||	                                 sample data expressed in bytes.
 |||
 |||
 |||	    Format:
 |||
 |||	    AF_TAG_CHANNELS              (uint32) Create, Query, Modify. Number of
 |||	                                 channels (or samples per sample frame). For
 |||	                                 example: 1 for mono, 2 for stereo. Valid range
 |||	                                 is 1..255.
 |||
 |||	    AF_TAG_WIDTH                 (uint32) Create, Query, Modify. Number of
 |||	                                 bytes per sample (uncompressed). Valid range
 |||	                                 is 1..2.
 |||
 |||	    AF_TAG_NUMBITS               (uint32) Create, Query, Modify. Number of bits
 |||	                                 per sample (uncompressed). Valid range is
 |||	                                 1..32. Width is rounded up to the next byte
 |||	                                 when computed from this tag.
 |||
 |||	    AF_TAG_COMPRESSIONTYPE       (uint32) Create, Query, Modify. 32-bit ID
 |||	                                 representing AIFC compression type of sample
 |||	                                 data (e.g. ID_SDX2). 0 for no compression.
 |||
 |||	    AF_TAG_COMPRESSIONRATIO      (uint32) Create, Query, Modify. Compression
 |||	                                 ratio of sample data. Uncompressed data
 |||	                                 has a value of 1. 0 is illegal.
 |||
 |||	    Loops:
 |||
 |||	    AF_TAG_SUSTAINBEGIN          (int32) Create, Query, Modify. Frame index of
 |||	                                 the first frame of the sustain loop. Valid
 |||	                                 range is 0..NumFrames-1. -1 for no sustain
 |||	                                 loop. Use in conjunction with
 |||	                                 AF_TAG_SUSTAINEND.
 |||
 |||	    AF_TAG_SUSTAINEND            (int32) Create, Query, Modify. Frame index of
 |||	                                 the first frame after the last frame in the
 |||	                                 sustain loop. Valid range is 1..NumFrames. -1
 |||	                                 for no sustain loop. Use in conjunction with
 |||	                                 AF_TAG_SUSTAINBEGIN.
 |||
 |||	    AF_TAG_RELEASEBEGIN          (int32) Create, Query, Modify. Frame index of
 |||	                                 the first frame of the release loop. Valid
 |||	                                 range is 0..NumFrames-1. -1 for no release
 |||	                                 loop. Use in conjunction with
 |||	                                 AF_TAG_RELEASEEND.
 |||
 |||	    AF_TAG_RELEASEEND            (int32) Create, Query, Modify. Frame index of
 |||	                                 the first frame after the last frame in the
 |||	                                 release loop. Valid range is 1..NumFrames. -1
 |||	                                 for no release loop. Use in conjunction with
 |||	                                 AF_TAG_RELEASEBEGIN.
 |||
 |||	    Tuning:
 |||
 |||	    AF_TAG_BASENOTE              (uint32) Create, Query, Modify. MIDI note
 |||	                                 number for this sample when played at the
 |||	                                 original sample rate (as set by
 |||	                                 AF_TAG_SAMPLE_RATE). Defaults to middle C
 |||	                                 (60). This defines the frequency conversion
 |||	                                 reference note for the StartInstrument()
 |||	                                 AF_TAG_PITCH tag.
 |||
 |||	    AF_TAG_DETUNE                (int32) Create, Query, Modify. Amount to
 |||	                                 detune the MIDI base note in cents to reach
 |||	                                 the original pitch. Must be in the range of
 |||	                                 -100 to 100.
 |||
 |||	    AF_TAG_SAMPLE_RATE           (ufrac16) Create, Query, Modify. Original
 |||	                                 sample rate for sample expressed in 16.16
 |||	                                 fractional Hz. Defaults to 44,100 Hz.
 |||
 |||	    AF_TAG_BASEFREQ              (ufrac16) Query. The frequency of the sample,
 |||	                                 expressed in 16.16 Hz, when played at the
 |||	                                 original sample rate. This value is computed
 |||	                                 from the other tuning tag values.
 |||
 |||	    Multisample:
 |||
 |||	    AF_TAG_LOWNOTE               (uint32) Create, Query, Modify. Lowest MIDI
 |||	                                 note number at which to play this sample when
 |||	                                 part of a multisample. StartInstrument()
 |||	                                 AF_TAG_PITCH tag is used to perform selection.
 |||	                                 Valid range is 0 to 127. Defaults to 0.
 |||
 |||	    AF_TAG_HIGHNOTE              (uint32) Create, Query, Modify. Highest MIDI
 |||	                                 note number at which to play this sample when
 |||	                                 part of a multisample. Valid range is 0 to
 |||	                                 127. Defaults to 127.
 |||
 |||	  Notes
 |||
 |||	    Sample creation tags have a lot mutual interaction. See CreateSample() for
 |||	    a complete explanation of this.
 |||
 |||	  Caveats
 |||
 |||	    There's currently no way to enforce that memory pointed to by
 |||	    AF_TAG_ADDRESS or file image memory used with AF_TAG_LEAVE_IN_PLACE is
 |||	    actually of MEMTYPE_AUDIO. Be careful.
 |||
 |||	    All sample data, loop points, and lengths must be quad-byte aligned. For
 |||	    example, a 1-channel, 8-bit sample (which has 1 byte per frame) is only
 |||	    legal when the lengths and loop points are at multiples of 4 frames. For
 |||	    mono ADPCM samples (which have only 4 bits per frame), lengths and loop
 |||	    points must be at multiples of 8 frames. The audio folio, however, does not
 |||	    report any kind of an error for using non-quad-byte aligned sample data.
 |||	    Sample addresses, lengths, and loop points, are internally truncated to
 |||	    quad-byte alignment when performing the DMA without warning. Misalignments
 |||	    may result in noise at loop points, or slight popping at the beginning and
 |||	    ending of sound playback. It is recommended that you pay very close
 |||	    attention to sample lengths and loop points when creating, converting, and
 |||	    compressing samples.
 |||
 |||	    Setting AF_TAG_WIDTH does not set the AF_TAG_NUMBITS attribute (e.g. if you
 |||	    create a sample with AF_TAG_WIDTH set to 1, GetAudioItemInfo()
 |||	    AF_TAG_NUMBITS will return 16). Setting AF_TAG_NUMBITS does however
 |||	    correctly update the AF_TAG_WIDTH field.
 |||
 |||	  See Also
 |||
 |||	    Attachment, Instrument, Template, StartInstrument(), sampler.dsp,
 |||	    delaymono.dsp
 |||
 **/
 /**
 |||	AUTODOC PRIVATE spg/items/sample-private
 |||	Additional private documentation for Sample Items
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_DELAY_LINE            (uint32) Create. Creates a delay line
 |||	                                 consisting of ta_Arg bytes (not frames).
 |||	                                 Mutually exclusive with AF_TAG_IMAGE_ADDRESS
 |||	                                 and AF_TAG_NAME. Not publicly documented because
 |||	                                 there's nothing to be gained by using this tag
 |||	                                 instead of CreateDelayLine().
 |||
 |||	    AF_TAG_INTERNAL_1            (void *) Create. Used to patch sample data
 |||	                                 address to fix an application that abused
 |||	                                 MakeSample(). See notes on BadDogData.
 |||
 |||	    AF_TAG_HIGHVELOCITY          (uint32) Create, Modify, Query. Highest MIDI
 |||	                                 attack velocity at which to play this sample
 |||	                                 when part of a multisample. Range is 0 to 127.
 |||	                                 Defaults to 127 on creation.
 |||
 |||	    AF_TAG_LOWVELOCITY           (uint32) Create, Modify, Query. Lowest MIDI
 |||	                                 attack velocity at which to play this sample
 |||	                                 when part of a multisample. Range is 0 to 127.
 |||	                                 Defaults to 0 on creation.
 |||
 |||	                                 Ideally, this tag and AF_TAG_HIGHVELOCITY
 |||	                                 would be used in conjunction with the
 |||	                                 StartInstrument() AF_TAG_VELOCITY tag, but
 |||	                                 StartInstrument() currently does not support
 |||	                                 velocity selection of multiple samples.
 |||
 |||	                                 These tags remain private for this reason.
 |||
 |||	    AF_TAG_SAMPLE                (const AudioSample *) Create. AudioSample
 |||	                                 structure containing initial sample parameters
 |||	                                 to use instead of defaults. Additional
 |||	                                 parameter tags can override these settings.
 |||	                                 This is used by Sample file loaders for the
 |||	                                 sake of efficiency to pass in sample
 |||	                                 parameters derived from the sample file.
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/loadsample
 |||	LoadSample - Loads an AIFF or AIFC sample from file.
 |||
 |||	  Synopsis
 |||
 |||	    Item LoadSample( char *Name )
 |||
 |||	  Description
 |||
 |||	    This procedure allocates task memory and creates a sample item there that
 |||	    contains the digital-audio recording from the specified file.  The file
 |||	    must be either an AIFF file or an AIFC file.  These can be created using
 |||	    almost any sound development tool including AudioMedia, Alchemy, CSound,
 |||	    and SoundHack.
 |||
 |||	    AIFC files contain compressed audio data.  These can be created from an
 |||	    AIFF file using the SquashSound MPW  tool from The 3DO Company.
 |||
 |||	    A single loop is supported for sustained repetitive waveforms.
 |||
 |||	    When you finish with the sample, you should call UnloadSample() to
 |||	    deallocate the resources.
 |||
 |||	  Arguments
 |||
 |||	    Name                         The name of the AIFF or AIFC file.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns an item number if successful or an error code (a
 |||	    negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    AttachSample(), DetachSample(), SetAudioItemInfo(), UnloadSample(),
 |||	    DebugSample(), CreateSample(), ScanSample(), GetAudioItemInfo(),
 |||	    LoadSampleHere()
 |||
 **/
Item LoadSample ( char *Name )
{
	Item Result;
	TagArg Tags[2];

TRACEE(TRACE_INT,TRACE_SAMPLE,("LoadSample( %s )\n", Name));

	Tags[0].ta_Tag = AF_TAG_NAME;
	Tags[0].ta_Arg = (void *) Name;
	Tags[1].ta_Tag = TAG_END;
	
	Result = CreateSample( Tags );
	
TRACER(TRACE_INT, TRACE_SAMPLE, ("LoadSample returns 0x%08x\n", Result));
	return Result;
}

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/loadsamplehere
 |||	LoadSampleHere - Loads an AIFC sample from file into RAM allocated and
 |||	                 freed  by a custom procedure.
 |||
 |||	  Synopsis
 |||
 |||	    Item LoadSampleHere( char *Name, void*(*CustomAllocMem)(),
 |||	    void (*CustomFreeMem)() )
 |||
 |||	  Description
 |||
 |||	    This procedure, like LoadSample(), loads and creates a sample item in RAM
 |||	     from a sample stored in the specified file.  The major difference is that
 |||	    LoadSampleHere() allows a task to use its own memory allocation and
 |||	    deallocation calls to provide RAM for the sample.
 |||
 |||	    To use this task, you must provide a custom allocation procedure and a
 |||	    custom deallocation procedure and then specify them in the call.  When
 |||	    LoadSampleHere() executes, it passes these arguments to the allocation
 |||	    procedure:
 |||
 |||	    ( int32 NumBytes, uint32 MemType )
 |||
 |||	    The allocation call should should return a DataPtr to the allocated memory
 |||	    if successful or NULL if unsuccessful.
 |||
 |||	    When the sample is later unloaded using UnloadSample(), the deallocation
 |||	    procedure receives these arguments:
 |||
 |||	    ( ivoid *DataPtr, int32 NumBytes)
 |||
 |||	    The deallocation procedure must use these arguments to free the sample's
 |||	    memory.
 |||
 |||	  Arguments
 |||
 |||	    Name                         Name of an AIFF or AIFC file to be loaded.
 |||
 |||	    CustomAllocMem()             A custom memory-allocation procedure.
 |||
 |||	    CustomFreeMem()              A custom memory-freeing procedure.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number of the sample if successful or an
 |||	    error code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    AttachSample(), DetachSample(), SetAudioItemInfo(), UnloadSample(),
 |||	    DebugSample(), CreateSample(), ScanSample(), GetAudioItemInfo(),
 |||	    LoadSample()
 |||
 **/
Item LoadSampleHere ( char *Name, void *(*CustomAllocMem)(), void (*CustomFreeMem)())
{
	Item Result;
	TagArg Tags[4];

TRACEE(TRACE_INT,TRACE_SAMPLE,("LoadSampleHere( %s, 0x%x, 0x%x)\n", Name, CustomAllocMem, CustomFreeMem));

	Tags[0].ta_Tag = AF_TAG_NAME;
	Tags[0].ta_Arg = (void *) Name;
	Tags[1].ta_Tag = AF_TAG_ALLOC_FUNCTION;
	Tags[1].ta_Arg = (void *) CustomAllocMem;
	Tags[2].ta_Tag = AF_TAG_FREE_FUNCTION;
	Tags[2].ta_Arg = (void *) CustomFreeMem;
	Tags[3].ta_Tag = TAG_END;
	
	Result = CreateSample( Tags );
	
TRACER(TRACE_INT, TRACE_SAMPLE, ("LoadSampleHere returns 0x%08x\n", Result));
	return Result;
}

/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/definesamplehere
 |||	DefineSampleHere - Defines a sample from a data stream.
 |||
 |||	  Synopsis
 |||
 |||	    Item DefineSampleHere( uint8 *AIFFImage, int32 NumBytes,
 |||	    void *(*CustomAllocMem)(), void (*CustomFreeMem)() )
 |||
 |||	  Description
 |||
 |||	    This call uses a custom allocation procedure to allocate memory for a
 |||	    sample buffer.  It then finds an image of an AIFF file contained within a
 |||	    data stream and creates a sample using that image, placing the sample in
 |||	    the allocated memory.  The procedure specifies a custom
 |||	    memory-deallocation procedure to be called when the sample is unloaded.
 |||
 |||	    See the description of LoadSampleHere() for the parameters passed to the
 |||	    custom memory calls.
 |||
 |||	    When you finish with the created sample, you should call UnloadSample() to
 |||	    deallocate its resources.
 |||
 |||	  Arguments
 |||
 |||	    AIFFImage                    Pointer to the AIFF image in RAM.
 |||
 |||	    NumBytes                     Value specifying the  number of bytes in the
 |||	                                 sample.
 |||
 |||	    CustomAllocMem               Pointer to a custom memory-allocation
 |||	                                 procedure.
 |||
 |||	    CustomFreeMem                Pointer to a custom memory-freeing
 |||	                                 procedure.
 |||
 |||	  Return Value
 |||
 |||	    This returns the item number of the sample or an error code (a negative
 |||	    value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    DefineInsTemplate()
 |||
 **/
Item DefineSampleHere ( char *Image, int32 NumBytes, void *(*CustomAllocMem)(), void (*CustomFreeMem)())
{
	Item Result;
	TagArg Tags[5];

TRACEE(TRACE_INT,TRACE_SAMPLE,("DefineSampleHere( 0x%x, %d, 0x%x, 0x%x )\n",
	Image, NumBytes, CustomAllocMem, CustomFreeMem));

	Tags[0].ta_Tag = AF_TAG_IMAGE_ADDRESS;
	Tags[0].ta_Arg = (void *) Image;
	Tags[1].ta_Tag = AF_TAG_IMAGE_LENGTH;
	Tags[1].ta_Arg = (void *) NumBytes;
	Tags[2].ta_Tag = AF_TAG_ALLOC_FUNCTION;
	Tags[2].ta_Arg = (void *) CustomAllocMem;
	Tags[3].ta_Tag = AF_TAG_FREE_FUNCTION;
	Tags[3].ta_Arg = (void *) CustomFreeMem;
	Tags[4].ta_Tag = TAG_END;
	
	Result = CreateSample( Tags );

TRACER(TRACE_INT, TRACE_SAMPLE, ("DefineSampleHere returns 0x%08x\n", Result));
	return Result;
}

/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/makesample
 |||	MakeSample - Makes a sample from memory (OBSOLETE)
 |||
 |||	  Synopsis
 |||
 |||	    Item MakeSample( uint32 NumBytes, TagArg *TagList )
 |||
 |||	  Description
 |||
 |||	    OBSOLETE - use CreateSample() instead.
 |||
 |||	    This procedure makes a sample without loading data from disk. It
 |||	    allocates a sample data buffer of the specified size and then defines that
 |||	    buffer as a sample using the parameters specified in the tag arg list. A
 |||	    task can write any data it cares to in the sample buffer; a sampled-sound
 |||	    instrument can then play that data from the buffer.
 |||
 |||	    When you finish with the sample, you should call UnloadSample() to
 |||	    deallocate its resources.
 |||
 |||	    See the SetAudioItemInfo() and the Music Programmer's Guide for a
 |||	    list of supported tags.
 |||
 |||	    See the test program ta_makesample.c for an example.
 |||
 |||	  Arguments
 |||
 |||	    NumBytes                     Amount of memory to be allocated. Set to
 |||	                                 zero if you already have a sample in memory
 |||	                                 that you want to define using this call.
 |||
 |||	    TagList                      A pointer to an array of tag arguments. The
 |||	                                 last element of the array must be the value
 |||	                                 TAG_END. If there are no tag arguments,
 |||	                                 this argument must be NULL. For a list of
 |||	                                 possible tag arguments for samples, see
 |||	                                 SetAudioItemInfo() and the description of
 |||	                                 MakeSample() in the Music Programmer's Guide.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a Sample Item number (a positive value) if successful
 |||	    or an error code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Notes
 |||
 |||	    THIS FUNCTION IS CONSIDERED OBSOLETE. Use CreateSample() instead.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    UnloadSample(), CreateSample()
 |||
 **/
Item MakeSample ( uint32 NumBytes , TagArg *TagList)
{
	AudioSample ASample;
	TagArg Tags[2];
	Item it;
	int32 Result;
	void *AllocatedMem = NULL;

TRACEE(TRACE_INT,TRACE_SAMPLE,("MakeSample( %d , 0x%lx)\n", NumBytes, TagList));

	DefaultSample ( &ASample );
	
/* 940304 Moved internalSetSampleInfo after allocation of data.
** This is because MakeSample could fail if loops were set.
**
** There could be a problem if the caller both passed an address and count as tags,
** and requested an allocation via NumBytes > 0.
*/

/* Allocate MEMTYPE_AUDIO memory. */
	if (NumBytes > 0)
	{

		ASample.asmp_Data = AllocatedMem = EZMemAlloc(NumBytes, MEMTYPE_AUDIO);
		if (ASample.asmp_Data == NULL)
		{
			it = (Item) AF_ERR_NOMEM;
			goto error;
		}
		ASample.asmp_SuperFlags |= AF_SAMPF_FOLIO_OWNS;
		ASample.asmp_NumBytes = NumBytes;
/* Set number of frames to match number of bytes and default format. 941121 */
		ASample.asmp_NumFrames = CvtByteToFrame( NumBytes, &ASample );
	}
	
/* Parse tags to modify default. */
	Result = internalSetSampleInfo(&ASample, TagList);
	if (Result < 0)
	{
		it = (Item) Result;
		goto error;
	}
DBUG(("MakeSample: AllocatedMem = 0x%x , ASample.asmp_Data = 0x%lx)\n",
	AllocatedMem, ASample.asmp_Data));

/* Check for conflicts between taglist and allocation request. */
	if(AllocatedMem)
	{
		if(AllocatedMem != ASample.asmp_Data)
		{
			PRT(("MakeSample: memory allocated by NumBytes>0 and address passed by tag!\n"));

/* 940429
** Mad Dog has several problems.  It calls this routine with NumBytes set
** which causes sample memory to be allocated, and it passes an address
** using AF_TAG_ADDRESS which causes the memory we allocated to be lost.
** It then asks for the allocated memory using GetAudioItemInfo().
** Most Apps with this bug will expect to use the passed in address.
** But we will save the allocated memory and then use it if they
** call GetAudioItemInfo().
** Applications that call this routine correctly will not be affected by this.
*/
/* Disable error return code because it breaks Mad Dog McCree. 940429 */
/*			it = (Item) AF_ERR_BADTAG; */
/*			goto error; */
			ASample.asmp_BadDogData = AllocatedMem; /* 940614 */
			AllocatedMem = NULL;
DBUG(("ASample.asmp_Data = 0x%x\n", ASample.asmp_Data ));
DBUG(("ASample.asmp_BadDogData = 0x%x\n", ASample.asmp_BadDogData ));
		}
	}
	
	if ( (NumBytes > 0)  && ( ASample.asmp_NumBytes != NumBytes ) )
	{
		ERR(("MakeSample: NumBytes set twice!\n"));
		it = (Item) AF_ERR_BADTAG;
		goto error;
	}

	Tags[0].ta_Tag = AF_TAG_SAMPLE;
	Tags[0].ta_Arg = (void *) &ASample;
	Tags[1].ta_Tag =  0;
    it = CreateItem( MKNODEID(AUDIONODE,AUDIO_SAMPLE_NODE), Tags );
    
TRACER(TRACE_INT, TRACE_SAMPLE, ("MakeSample returns 0x%08x\n", it));
	return it;
    
error:
/* Deallocate memory because it will be lost if we return a non-item. 940614 */
	if( AllocatedMem ) EZMemFree( AllocatedMem );
TRACER(TRACE_INT, TRACE_SAMPLE, ("MakeSample returns 0x%08x\n", it));
	return it;
 }
 
/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/createsample
 |||	CreateSample - Generic sample creation function.
 |||
 |||	  Synopsis
 |||
 |||	    Item CreateSample (TagArg *TagList)
 |||
 |||	    Item CreateSampleVA (uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    This function is used by all the other sample creation functions (e.g.
 |||	    LoadSample(), ScanSample(), DefineSampleHere(), etc.) to actually perform
 |||	    the sample creation. Use it if you need to do something that these
 |||	    other more specific functions don't provide (e.g. creating an empty sample
 |||	    item to which to attach your own sample memory).
 |||
 |||	    This function is extremely flexible and has many different sets of tags to
 |||	    perform different types of Sample Item creation. Sample Item creation falls
 |||	    into these categories:
 |||	        . Loading a Sample File from disc
 |||	        . Parsing a Sample File image in memory
 |||	        . Creating a Sample Item from scratch
 |||
 |||	    The currently supported sample file formats are:
 |||	        . AIFF
 |||	        . AIFC
 |||
 |||	    The tag sets are described below for each category of sample item creation.
 |||
 |||	  Tags to Load Sample File
 |||
 |||	    Required:
 |||
 |||	    AF_TAG_NAME                  (const char *) Name of sample file to load.
 |||
 |||	    Optional:
 |||
 |||	    AF_TAG_SCAN                  (int32) Specifies the maximum number of bytes
 |||	                                 of sound data to read from the file. This can
 |||	                                 be used to cause the audio folio to simply
 |||	                                 load sample parameters without loading sample
 |||	                                 data. (see ScanSample()).
 |||
 |||	    AF_TAG_ALLOC_FUNCTION        (void *(*)(uint32 memsize, uint32 memflags))
 |||	                                 Sets custom memory allocation function to be
 |||	                                 called to allocate sample memory while loading
 |||	                                 sample file. Defaults to AllocMem(). If you
 |||	                                 supply a custom allocation function you must
 |||	                                 also provide a custom free function with
 |||	                                 AF_TAG_FREE_FUNCTION.
 |||
 |||	    AF_TAG_FREE_FUNCTION         (void (*)(void *memptr, uint32 memsize)) Sets
 |||	                                 custom memory free function to free sample
 |||	                                 memory to be called during sample deletion.
 |||	                                 Defaults to FreeMem(). If you supply a custom
 |||	                                 free function you must also provide a custom
 |||	                                 allocation function with AF_TAG_ALLOC_FUNCTION.
 |||
 |||	  Tags to Parse Sample File Image in Memory
 |||
 |||	    Required:
 |||
 |||	    AF_TAG_IMAGE_ADDRESS         (const char *) Specifies a memory location
 |||	                                 containing a sample file image from which to
 |||	                                 read sample. Unless AF_TAG_LEAVE_IN_PLACE is
 |||	                                 set, this memory does not need to remain valid
 |||	                                 after calling CreateSample(). Must use in
 |||	                                 conjunction with AF_TAG_IMAGE_LENGTH.
 |||
 |||	    AF_TAG_IMAGE_LENGTH          (uint32) Specifies number of bytes in sample
 |||	                                 file image pointed to by AF_TAG_IMAGE_ADDRESS.
 |||
 |||	    Optional:
 |||
 |||	    AF_TAG_SCAN                  (int32) See above.
 |||
 |||	    AF_TAG_ALLOC_FUNCTION        (void *(*)(uint32 memsize, uint32 memflags))
 |||	                                 See above. Mutually exclusive with
 |||	                                 AF_TAG_LEAVE_IN_PLACE.
 |||
 |||	    AF_TAG_FREE_FUNCTION         (void (*)(void *memptr, uint32 memsize)) See
 |||	                                 above.
 |||
 |||	    AF_TAG_LEAVE_IN_PLACE        (bool) Memory is normally allocated for the
 |||	                                 sample data to be extracted from the sample
 |||	                                 file, even when parsing a sample file image in
 |||	                                 memory. When this parameter is set to TRUE, the
 |||	                                 Sample Item uses the sample data contained
 |||	                                 within the sample file image in-place rather
 |||	                                 than allocating more memory to hold the sample
 |||	                                 data.
 |||
 |||	                                 This tag assumes that the file has been
 |||	                                 buffered in memory of MEMTYPE_AUDIO and that
 |||	                                 the memory will remain valid for the life of
 |||	                                 the Sample Item.
 |||
 |||	                                 Also, because the audio folio requires that
 |||	                                 sample data be quad-byte aligned, the sample
 |||	                                 data image in memory may be moved within the
 |||	                                 file image. This corrupts the file image for
 |||	                                 further parsing, but leaves the sample data
 |||	                                 intact. The audio folio prints a warning
 |||	                                 message if it needed to move the sample data,
 |||	                                 but does not return any special code.
 |||	                                 Therefore it's best to assume that the file
 |||	                                 image is always corrupted when this option is
 |||	                                 enabled.
 |||
 |||	                                 Defaults to FALSE. Mutually exclusive with
 |||	                                 AF_TAG_ALLOC_FUNCTION.
 |||
 |||	  Tags to Create Sample from Scratch
 |||
 |||	    Data:
 |||
 |||	    AF_TAG_ADDRESS               (void *) Specifies pointer to sample data to
 |||	                                 construct Sample Item for. This parameter can
 |||	                                 be changed at a later time with
 |||	                                 SetAudioItemInfo(), so it is not required for
 |||	                                 creation. For best results, must be quad-byte
 |||	                                 aligned (see caveats). In general, this
 |||	                                 parameter should be used in conjunction with
 |||	                                 AF_TAG_FRAMES or AF_TAG_NUMBYTES. Defaults to
 |||	                                 NULL.
 |||
 |||	    AF_TAG_FRAMES                (uint32) Length of sample data expressed in
 |||	                                 frames. In a stereo sample, this would be the
 |||	                                 number of stereo pairs. For best results, must
 |||	                                 be quad-byte aligned (see caveats). Mutually
 |||	                                 exlusive with AF_TAG_NUMBYTES. If neither
 |||	                                 AF_TAG_FRAMES or AF_TAG_NUMBYTES is specified,
 |||	                                 length defaults to 0.
 |||
 |||	    AF_TAG_NUMBYTES              (uint32) Length of sample data expressed in
 |||	                                 bytes. For best results, must be quad-byte
 |||	                                 aligned (see caveats). Mutually exlusive with
 |||	                                 AF_TAG_FRAMES. If neither AF_TAG_FRAMES or
 |||	                                 AF_TAG_NUMBYTES is specified, length defaults
 |||	                                 to 0.
 |||
 |||	    Format:
 |||
 |||	    AF_TAG_CHANNELS              (uint32) Number of channels (or samples per
 |||	                                 sample frame). For example: 1 for mono, 2 for
 |||	                                 stereo. Valid range is 1..255. Defaults to 1.
 |||
 |||	    AF_TAG_WIDTH                 (uint32) Number of bytes per sample
 |||	                                 (uncompressed). Valid range is 1..2. Mutually
 |||	                                 exclusive with AF_TAG_NUMBITS. If neither
 |||	                                 AF_TAG_WIDTH or AF_TAG_NUMBIS is specified,
 |||	                                 width defaults to 2 bytes (16 bits).
 |||
 |||	    AF_TAG_NUMBITS               (uint32) Number of bits per sample
 |||	                                 (uncompressed). Valid range is 1..32. Width
 |||	                                 is rounded up to the next byte when computed
 |||	                                 from this tag. Mutually exclusive with
 |||	                                 AF_TAG_WIDTH. If neither AF_TAG_WIDTH or
 |||	                                 AF_TAG_NUMBIS is specified, width defaults to
 |||	                                 2 bytes (16 bits).
 |||
 |||	    AF_TAG_COMPRESSIONTYPE       (uint32) 32-bit ID representing AIFC
 |||	                                 compression type of sample data (e.g.
 |||	                                 ID_SDX2). 0 for no compression, which is the
 |||	                                 default.
 |||
 |||	    AF_TAG_COMPRESSIONRATIO      (uint32) Compression ratio of sample data.
 |||	                                 Uncompressed data has a value of 1, which is
 |||	                                 the default. 0 is illegal.
 |||
 |||	    Loops:
 |||
 |||	    AF_TAG_SUSTAINBEGIN          (int32) Frame index of the first frame of the
 |||	                                 sustain loop. Valid range is 0..NumFrames-1.
 |||	                                 For best results, must be quad-byte aligned
 |||	                                 (see caveats). -1 for no sustain loop, which
 |||	                                 is the default. Must be used in conjunction
 |||	                                 with AF_TAG_SUSTAINEND.
 |||
 |||	    AF_TAG_SUSTAINEND            (int32) Frame index of the first frame after
 |||	                                 the last frame in the sustain loop. Valid
 |||	                                 range is 1..NumFrames. For best results, must
 |||	                                 be quad-byte aligned (see caveats). -1 for no
 |||	                                 sustain loop, which is the default. Must be
 |||	                                 used in conjunction with AF_TAG_SUSTAINBEGIN.
 |||
 |||	    AF_TAG_RELEASEBEGIN          (int32) Frame index of the first frame of the
 |||	                                 release loop. Valid range is 0..NumFrames-1.
 |||	                                 For best results, must be quad-byte aligned
 |||	                                 (see caveats). -1 for no release loop, which
 |||	                                 is the default. Must be used in conjunction
 |||	                                 with AF_TAG_RELEASEEND.
 |||
 |||	    AF_TAG_RELEASEEND            (int32) Frame index of the first frame after
 |||	                                 the last frame in the release loop. Valid
 |||	                                 range is 1..NumFrames. For best results, must
 |||	                                 be quad-byte aligned (see caveats). -1 for no
 |||	                                 release loop, which is the default. Must be
 |||	                                 used in conjunction with AF_TAG_RELEASEBEGIN.
 |||
 |||	    Tuning:
 |||
 |||	    AF_TAG_BASENOTE              (uint32) MIDI note number for this sample when
 |||	                                 played at the original sample rate (as set by
 |||	                                 AF_TAG_SAMPLE_RATE). Defaults to middle C
 |||	                                 (60). This defines the frequency conversion
 |||	                                 reference note for the StartInstrument()
 |||	                                 AF_TAG_PITCH tag.
 |||
 |||	    AF_TAG_DETUNE                (int32) Amount to detune the MIDI base note in
 |||	                                 cents to reach the original pitch. Must be in
 |||	                                 the range of -100 to 100. Defaults to 0.
 |||
 |||	    AF_TAG_SAMPLE_RATE           (ufrac16) Original sample rate for sample
 |||	                                 expressed in 16.16 fractional Hz. Defaults to
 |||	                                 44,100 Hz.
 |||
 |||	    Multisample:
 |||
 |||	    AF_TAG_LOWNOTE               (uint32) Lowest MIDI note number at which to
 |||	                                 play this sample when part of a multisample.
 |||	                                 StartInstrument() AF_TAG_PITCH tag is used
 |||	                                 to perform selection. Valid range is 0 to 127.
 |||	                                 Defaults to 0.
 |||
 |||	    AF_TAG_HIGHNOTE              (uint32) Highest MIDI note number at which to
 |||	                                 play this sample when part of a multisample.
 |||	                                 Valid range is 0 to 127. Defaults to 127.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a Sample Item number (a positive value) if successful
 |||	    or an error code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    There's currently no way to enforce that memory pointed to by
 |||	    AF_TAG_ADDRESS or file image memory used with AF_TAG_LEAVE_IN_PLACE is
 |||	    actually of MEMTYPE_AUDIO. Be careful.
 |||
 |||	    All sample data, loop points, and lengths must be quad-byte aligned. For
 |||	    example, a 1-channel, 8-bit sample (which has 1 byte per frame) is only
 |||	    legal when the lengths and loop points are at multiples of 4 frames. For
 |||	    mono ADPCM samples (which have only 4 bits per frame), lengths and loop
 |||	    points must be at multiples of 8 frames. The audio folio, however, does not
 |||	    report any kind of an error for using non-quad-byte aligned sample data.
 |||	    Sample addresses, lengths, and loop points, are internally truncated to
 |||	    quad-byte alignment when performing the DMA without warning. Misalignments
 |||	    may result in noise at loop points, or slight popping at the beginning and
 |||	    ending of sound playback. It is recommended that you pay very close
 |||	    attention to sample lengths and loop points when creating, converting, and
 |||	    compressing samples.
 |||
 |||	    Setting AF_TAG_WIDTH does not set the AF_TAG_NUMBITS attribute (e.g. if you
 |||	    create a sample with AF_TAG_WIDTH set to 1, GetAudioItemInfo()
 |||	    AF_TAG_NUMBITS will return 16). Setting AF_TAG_NUMBITS does however
 |||	    correctly update the AF_TAG_WIDTH field.
 |||
 |||	  Examples
 |||
 |||	        // Create a Sample Item to be filled out later with SetAudioItemInfo()
 |||	    sample = CreateSample (NULL);
 |||
 |||	        // Make a Sample Item for raw sample data in memory
 |||	        // (8-bit monophonic 22KHz sample in this case)
 |||	    sample = CreateSampleVA (
 |||	        AF_TAG_ADDRESS,     data,
 |||	        AF_TAG_FRAMES,      NUMFRAMES,
 |||	        AF_TAG_CHANNELS,    1,
 |||	        AF_TAG_WIDTH,       1,
 |||	        AF_TAG_SAMPLE_RATE, Convert32_F16 (22050),
 |||	        TAG_END);
 |||
 |||	        // Parse a sample file image and use the sample data in place
 |||	    sample = CreateSampleVA (
 |||	        AF_TAG_IMAGE_ADDRESS,  sampleFileImage,
 |||	        AF_TAG_IMAGE_LENGTH,   sampleFileLength,
 |||	        AF_TAG_LEAVE_IN_PLACE, TRUE,
 |||	        TAG_END);
 |||
 |||	        // Identical to LoadSample()
 |||	    sample = CreateSampleVA (
 |||	        AF_TAG_NAME, "sinewave.aiff",
 |||	        TAG_END);
 |||
 |||	        // Identical to DefineSampleHere()
 |||	    sample = CreateSampleVA (
 |||	        AF_TAG_IMAGE_ADDRESS,  sampleFileImage,
 |||	        AF_TAG_IMAGE_LENGTH,   sampleFileLength,
 |||	        AF_TAG_ALLOC_FUNCTION, MyAlloc,
 |||	        AF_TAG_FREE_FUNCTION,  MyFree,
 |||	        TAG_END);
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    UnloadSample(), LoadSample(), ScanSample(), Sample, AttachSample()
 |||
 **/
Item CreateSample ( TagArg *Tags)
{
	Item Result;
	AudioSample ASample, *asmp;
	uint32 tagc, *tagp, temp;
	TagArg MyTags[3];
	iff_control iffcb;
	SampleParserContext LSContext;
	int32 i;

	char *FileName = NULL;
	void *(*CustomAllocMem)() = NULL;
	void (*CustomFreeMem)() = NULL;
	int32 IfLeaveInPlace = FALSE;
	char *ImageAddress = NULL;
	int32 ImageLength = 0;
	int32 IfLoadingSample = FALSE;
	int32 IfOtherTags = FALSE;
	int32 IfReadData = TRUE;
	int32 ScanDataSize = 0;
	
	Result = 0;
TRACEE(TRACE_INT,TRACE_SAMPLE,("CreateSample( 0x%lx)\n", Tags));

	asmp = &ASample;
	DefaultSample ( asmp );
	
	tagp = (uint32 *) Tags;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
			temp = *tagp++;
DBUG(("CreateSample: Tag = %d, Arg = $%x\n", tagc, temp));
			
			switch (tagc)
			{
			case AF_TAG_NAME:
				FileName = (char *)temp;
				IfLoadingSample = TRUE;
				break;
				
			case AF_TAG_IMAGE_ADDRESS:
				ImageAddress = (char *) temp;
				IfLoadingSample = TRUE;
				break;
			case AF_TAG_IMAGE_LENGTH:
				ImageLength = temp;
				IfLoadingSample = TRUE;
				break;
			
			case AF_TAG_LEAVE_IN_PLACE:
				IfLeaveInPlace = temp;
				IfLoadingSample = TRUE;
				break;
				
			case AF_TAG_ALLOC_FUNCTION:
				CustomAllocMem = (void *(*)()) temp;
				IfLoadingSample = TRUE;
				break;
				
			case AF_TAG_FREE_FUNCTION:
				CustomFreeMem = (void (*)()) temp;
				IfLoadingSample = TRUE;
				break;
				
			case AF_TAG_SCAN:
				ScanDataSize = (int32) temp;
				IfReadData = FALSE;
				break;
				
			default:
				IfOtherTags = TRUE;
				break;
			}
		}
	}

/* Detect Tag errors. */
	if( IfLoadingSample && IfOtherTags )
	{
		ERR(("CreateSample: mixed loading tags with make from scratch tags.\n"));
		return AF_ERR_BADTAG;
	}
	
	if( (FileName != NULL) && (ImageAddress != NULL) )
	{
		ERR(("CreateSample: Can't load from file and image!\n"));
		return AF_ERR_BADTAG;
	}
	if((CustomAllocMem==NULL) ^ (CustomFreeMem==NULL))
	{
		ERR(("CreateSample: Both custom functions must be passed!\n"));
		return AF_ERR_OUTOFRANGE;
	}

/* Load from file or image. */
	if( IfLoadingSample )
	{

/* Setup Parser. */
		iffcb.iffc_ChunkHandler = 0;
		iffcb.iffc_FormHandler = ParseAIFFForm;
		memset( (char *) &LSContext, 0, sizeof(SampleParserContext) );
		LSContext.spc_Sample = asmp;
		LSContext.spc_CustomAllocMem = CustomAllocMem;
		LSContext.spc_IfLeaveInPlace = IfLeaveInPlace;
		iffcb.iffc_UserContext = (void *) &LSContext;
		iffcb.iffc_LastChanceDir = AudioBase->af_StartupDirectory;
		ASample.asmp_NumBytes = ScanDataSize;
		LSContext.spc_IfReadData = IfReadData;

	 	if(FileName != NULL)
	 	{
/* Disable this for files.  940812 */
	 		if( IfLeaveInPlace )
	 		{
	 			ERR(("AF_TAG_LEAVE_IN_PLACE not legal for files.\n"));
	 			Result = AF_ERR_BADTAGVAL;
				goto error;
	 		}
	 		
	 		Result = (int32) iffParseFile(&iffcb, FileName);
		}
		else
		{
			Result = (int32) iffParseImage(&iffcb, ImageAddress, ImageLength );
		}
		if (Result < 0)
		{
			ERR(("CreateSample: Could not parse AIFF.\n"));
			goto error;
		}
			
  	 	ASample.asmp_CustomFreeMem = CustomFreeMem;
  	 	i = 0;
  	 	if(FileName != NULL)
		{
			MyTags[i].ta_Tag = AF_TAG_NAME;
			MyTags[i++].ta_Arg = (void *) FileName;
		}
		MyTags[i].ta_Tag = AF_TAG_SAMPLE;
		MyTags[i++].ta_Arg = (void *) asmp;
		MyTags[i].ta_Tag =  TAG_END;
  	  	Result = CreateItem( MKNODEID(AUDIONODE,AUDIO_SAMPLE_NODE), MyTags );
	}
	else
	{
  	  	Result = CreateItem( MKNODEID(AUDIONODE,AUDIO_SAMPLE_NODE), Tags );
	}
	
TRACER(TRACE_INT, TRACE_SAMPLE, ("LoadSample returns 0x%08x\n", Result));
error:
	return Result;
}

/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/deletedelayline
 |||	DeleteDelayLine - Deletes a delay line.
 |||
 |||	  Synopsis
 |||
 |||	    Err DeleteDelayLine( Item DelayLine )
 |||
 |||	  Description
 |||
 |||	    This procedure deletes a delay line item and frees its resources.
 |||
 |||	  Arguments
 |||
 |||	    DelayLine                    The item number of the delay line to delete.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    CreateDelayLine()
 |||
 **/
int32 DeleteDelayLine ( Item DelayLine )
{
	return DeleteItem( DelayLine );
}

/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/createdelayline
 |||	CreateDelayLine - Creates a delay for echoes and reverberations.
 |||
 |||	  Synopsis
 |||
 |||	    Item CreateDelayLine( int32 NumBytes, int32 NumChannels,
 |||	    int32 IfLoop )
 |||
 |||	  Description
 |||
 |||	    This procedure creates a delay line item for echo, reverberation, and
 |||	    oscilloscope applications.  The delay line item can be attached to any
 |||	    instrument that provides DMA OUT from DSP.  When finished with the delay
 |||	    line, delete it using DeleteDelayLine() to free its resources.
 |||
 |||	    CreateDelayLine() calls CreateItem() using the tag argument
 |||	    AF_TAG_DELAY_LINE.
 |||
 |||	  Arguments
 |||
 |||	    NumBytes                     The number of bytes in the delay line's
 |||	                                 sample buffer.
 |||
 |||	    NumChannels                  The number of channels stored in each sample
 |||	                                 frame.
 |||
 |||	    IfLoop                       A value indicating whether or not this is a
 |||	                                 loop.  Set to TRUE (1) or FALSE (0).
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number of the delay line or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    DeleteDelayLine()
 |||
 **/
Item CreateDelayLine ( int32 NumBytes , int32 NumChannels, int32 IfLoop)
{
	TagArg Tags[3];
	Item it;
	int32 Result;
	int32 NumFrames;
	
TRACEE(TRACE_INT,TRACE_SAMPLE,("CreateDelayLine( %d , %d, %d)\n", NumBytes, NumChannels, IfLoop));
	Tags[0].ta_Tag = AF_TAG_DELAY_LINE;
	Tags[0].ta_Arg = (void *) NumBytes;
	Tags[1].ta_Tag = AF_TAG_CHANNELS;
	Tags[1].ta_Arg = (void *) NumChannels;
	Tags[2].ta_Tag =  TAG_END;
    it = CreateItem( MKNODEID(AUDIONODE,AUDIO_SAMPLE_NODE), Tags );
    if( it < 0 ) return it;

	if(IfLoop)
	{
		Tags[0].ta_Tag = AF_TAG_FRAMES;
		Tags[1].ta_Tag =  TAG_END;
		Result = GetAudioItemInfo( it, Tags );
		if( Result < 0) return Result;
		NumFrames = (int32) Tags[0].ta_Arg;
	
		Tags[0].ta_Tag = AF_TAG_SUSTAINBEGIN;
		Tags[0].ta_Arg = (void *) 0;
		Tags[1].ta_Tag = AF_TAG_SUSTAINEND;
		Tags[1].ta_Arg = (void *) NumFrames;
		Tags[2].ta_Tag =  TAG_END;
		Result = SetAudioItemInfo( it, Tags );
		if( Result < 0) return Result;
	}
	
TRACER(TRACE_INT, TRACE_SAMPLE, ("CreateDelayLine returns 0x%08x\n", it));
	return it;
 }

/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/scansample
 |||	ScanSample - Loads a truncated sample file.
 |||
 |||	  Synopsis
 |||
 |||	    Item ScanSample( char *Name, int32 BufferSize )
 |||
 |||	  Description
 |||
 |||	    This procedure is similar to LoadSample(), but allocates only as much
 |||	    memory as specified by BufferSize.  It then loads as much of the sample
 |||	    data as will fit.  This is handy for loading the first part of a very
 |||	    large sound file that won't fit in memory.
 |||
 |||	  Arguments
 |||
 |||	    Name                         Name of an AIFF or AIFC file.
 |||
 |||	    BufferSize                   Amount of memory to allocate (in bytes).
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a Sample Item number if successful or an error code (a
 |||	    negative value) if an error occurs.
 |||
 |||	  Caveats                       
 |||                       
 |||	    Any loop points in an AIFF or AIFC file beyond the BufferSize cause                   
 |||	    this function to return an error.
 |||                       
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||                       
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    AttachSample(), DetachSample(), SetAudioItemInfo(), LoadSample(),
 |||	    UnloadSample(), DebugSample(), CreateSample(), GetAudioItemInfo()
 |||
 **/
Item ScanSample ( char *Name , int32 DataSize )
{
	Item Result;
	TagArg Tags[3];

TRACEE(TRACE_INT,TRACE_SAMPLE,("ScanSample( %s, %d)\n", Name, DataSize));

	Tags[0].ta_Tag = AF_TAG_NAME;
	Tags[0].ta_Arg = (void *) Name;
	Tags[1].ta_Tag = AF_TAG_SCAN;
	Tags[1].ta_Arg = (void *) DataSize;
	Tags[2].ta_Tag = TAG_END;
	
	Result = CreateSample( Tags );
	
TRACER(TRACE_INT, TRACE_SAMPLE, ("ScanSample returns 0x%08x\n", Result));
	return Result;
}


/*****************************************************************/
void DefaultSample ( AudioSample *asmp)
{
/* Setup Sample field defaults. */
	asmp->asmp_OpenCount = 0;
	asmp->asmp_Data = NULL;
	asmp->asmp_NumBytes = 0;
	asmp->asmp_Bits = 16;
	asmp->asmp_Width = 2;
	asmp->asmp_CompressionRatio = 1;
	asmp->asmp_SuperFlags = 0;
	asmp->asmp_Channels = 1;
	asmp->asmp_NumFrames = 0;
	asmp->asmp_BaseNote = 60;   /* Middle C */
	asmp->asmp_Detune = 0;
	asmp->asmp_LowNote = 0;
	asmp->asmp_HighNote = 127;
	asmp->asmp_LowVelocity = 0;
	asmp->asmp_HighVelocity = 127;
	asmp->asmp_BaseFreq = Convert32_F16(440);  /* Concert A */
	asmp->asmp_SustainBegin = -1;
	asmp->asmp_ReleaseBegin = -1;
	asmp->asmp_SustainEnd = -1;
	asmp->asmp_ReleaseEnd = -1;
	asmp->asmp_CompressionType = 0;
	asmp->asmp_OwnerItem = 0;
	asmp->asmp_DataOffset = 0;
	asmp->asmp_DataSize = 0;
	asmp->asmp_SampleRate = Convert32_F16( DEFAULT_SAMPLERATE ); /* 940506 */
	asmp->asmp_CustomFreeMem = NULL;
	asmp->asmp_BadDogData = NULL;  /* 940614 */
}

/*****************************************************************/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/debugsample
 |||	DebugSample - Prints sample information for debugging.
 |||
 |||	  Synopsis
 |||
 |||	    Err DebugSample( Item SampleItem )
 |||
 |||	  Description
 |||
 |||	    This procedure dumps all available sample information to the 3DO Debugger
 |||	    screen for your delight and edification.  This procedure will probably be
 |||	    eliminated from the production version of the software.
 |||
 |||	  Arguments
 |||
 |||	    SampleItem                   Item number of a sample.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    AttachSample(), DetachSample(), SetAudioItemInfo(), LoadSample(),
 |||	    UnloadSample(), CreateSample(), ScanSample(), GetAudioItemInfo()
 |||
 **/
int32 DebugSample( Item SampleItem )
{
/* User level dump of Sample contents. */
	AudioSample *asmp;
	
	asmp = (AudioSample *)CheckItem(SampleItem,  AUDIONODE, AUDIO_SAMPLE_NODE);
	if (asmp == NULL) return AF_ERR_BADITEM;
	return internalDumpSample (asmp);
}

/*****************************************************************/
#ifdef DEVELOPMENT
#define REPORT_SAMPLE(name,member) PRT(("SAMPLE.%s = %d\n", name, asmp->member));
#define REPORT_SAMPLE_HEX(name,member) PRT(("SAMPLE.%s = 0x%x\n", name, asmp->member));
#else
#define REPORT_SAMPLE(name,member) /* PRT(("SAMPLE.%s = 0x%x\n", name, asmp->member)); */
#define REPORT_SAMPLE_HEX(name,member) /* PRT(("SAMPLE.%s = 0x%x\n", name, asmp->member)); */
#endif

int32 internalDumpSample ( AudioSample *asmp )
{
	PRT(("--------------------------\nSample Structure Dump\n"));
    REPORT_SAMPLE("asmp_OpenCount", asmp_OpenCount);
    REPORT_SAMPLE_HEX("asmp_Data", asmp_Data);
    REPORT_SAMPLE("asmp_NumBytes", asmp_NumBytes);
    REPORT_SAMPLE("asmp_SuperFlags", asmp_SuperFlags);
    REPORT_SAMPLE("asmp_Bits", asmp_Bits);
    REPORT_SAMPLE("asmp_Width", asmp_Width);
    REPORT_SAMPLE("asmp_Channels", asmp_Channels);
    REPORT_SAMPLE("asmp_NumFrames", asmp_NumFrames);
    REPORT_SAMPLE("asmp_BaseFreq", asmp_BaseFreq);
    REPORT_SAMPLE("asmp_BaseNote", asmp_BaseNote);
    REPORT_SAMPLE("asmp_Detune", asmp_Detune);
    REPORT_SAMPLE("asmp_LowNote", asmp_LowNote);
    REPORT_SAMPLE("asmp_HighNote", asmp_HighNote);
    REPORT_SAMPLE("asmp_LowVelocity", asmp_LowVelocity);
    REPORT_SAMPLE("asmp_HighVelocity", asmp_HighVelocity);
    REPORT_SAMPLE("asmp_SustainBegin", asmp_SustainBegin);
    REPORT_SAMPLE("asmp_SustainEnd", asmp_SustainEnd);
    REPORT_SAMPLE("asmp_ReleaseBegin", asmp_ReleaseBegin);
    REPORT_SAMPLE("asmp_ReleaseEnd", asmp_ReleaseEnd);
    REPORT_SAMPLE("asmp_DataOffset", asmp_DataOffset);
    REPORT_SAMPLE("asmp_DataSize", asmp_DataSize);
    REPORT_SAMPLE_HEX("asmp_CompressionType", asmp_CompressionType);
    REPORT_SAMPLE_HEX("asmp_CompressionRatio", asmp_CompressionRatio);
    REPORT_SAMPLE("asmp_OwnerItem", asmp_OwnerItem);
    REPORT_SAMPLE_HEX("asmp_SampleRate", asmp_SampleRate);
	return 0;
}

/******************************************************************/
/* Make Sample Item, return in ATTParserContext context. ********/
/******************************************************************/
int32 HandleATSMForm ( iff_control *iffc, uint32 FormType , uint32 FormSize )
{
	int32 Result = 0;
	TagArg Tags[2];
	SampleParserContext LSContext;
	TemplateParserContext *tmplpc;
	AudioSample ASample;
	int32 (*OldHandler)();
	
DBUG(("HandleATSMForm: %.4s, %d\n",  &FormType, FormSize));

/* Setup Parser. */
	OldHandler = iffc->iffc_ChunkHandler;
/* 940921 REDUNDANT	iffc->iffc_ChunkHandler = HandleAIFFChunk; */
	tmplpc = (TemplateParserContext *) iffc->iffc_UserContext;

	memset( (char *) &LSContext, 0, sizeof(SampleParserContext) );

	LSContext.spc_IfReadData = TRUE;
	LSContext.spc_Sample = &ASample;
	LSContext.spc_CustomAllocMem = tmplpc->tmplpc_CustomAllocMem; /* 931216 */
	LSContext.spc_IfLeaveInPlace = tmplpc->tmplpc_IfLeaveInPlace; /* 931216 */
DBUG(("HandleATSMForm: set LSContext.spc_IfLeaveInPlace to %d\n", LSContext.spc_IfLeaveInPlace));
	iffc->iffc_UserContext = (void *) &LSContext;

	DefaultSample ( &ASample );
	 
	Result = ParseAIFFForm( iffc, FormType, FormSize );

	ASample.asmp_CustomFreeMem = tmplpc->tmplpc_CustomFreeMem;
	Tags[0].ta_Tag = AF_TAG_SAMPLE;
	Tags[0].ta_Arg = (void *) &ASample;
	Tags[1].ta_Tag =  0;
	tmplpc->tmplpc_SlaveItem = CreateItem( MKNODEID(AUDIONODE,AUDIO_SAMPLE_NODE), Tags );

/* restore iff parser */
	iffc->iffc_ChunkHandler = OldHandler;
	iffc->iffc_UserContext = (void *) tmplpc;
	
	return Result;
}

/******************************************************************/
int32 HandleATSMChunk ( iff_control *iffc, uint32 ChunkType , uint32 ChunkSize )
{
	int32 Result = 0;
	char *name;
	TemplateParserContext *tmplpc;
	
	tmplpc = (TemplateParserContext *) iffc->iffc_UserContext;
	
DBUG(("HandleATSMChunk: %.4s, %d\n",  &ChunkType, ChunkSize));

	switch(ChunkType)
	{
		case ID_HOOK:
			name = (char *) &tmplpc->tmplpc_HookName;
			if (ChunkSize < AF_MAX_NAME_SIZE)
			{
				Result = iffReadChunkData(iffc, name, ChunkSize);
				CHECKRSLT(("Error reading CHUNK data = %d\n", Result));
				name[ChunkSize] = '\0';
				DBUG(("Hook = %s\n", name));
			}
			else
			{
				ERR(("HandleATSMChunk: HOOK name too large = %d\n", ChunkSize));
				Result = AF_ERR_BADOFX;
			}
			break;
	}
	
error:
	return Result;
}


/*****************************************************************/
int32 ParseAIFFForm( iff_control *iffc, int32 FormType, int32 FormSize )
{
	int32 Result;
	int32 (*OldHandler)();
	SampleParserContext *spc;
	AudioSample *asmp;
	
	OldHandler = iffc->iffc_ChunkHandler;
	spc = (SampleParserContext *) iffc->iffc_UserContext;
	
	iffc->iffc_ChunkHandler = HandleAIFFChunk;
	
	switch(FormType)
	{
		case ID_AIFF:
			spc->spc_IfCompressed = FALSE;
			break;
		case ID_AIFC:
			spc->spc_IfCompressed = TRUE;
			break;
					
		default:
			ERR(("Invalid IFF FORM type = 0x%x\n", FormType));
			Result = AF_ERR_BADFILETYPE;
			goto error;
	}
	
	Result = iffScanChunks( iffc, FormSize );
	iffc->iffc_ChunkHandler = OldHandler;
	CHECKRSLT(("HandleAIFFForm: Error scanning sample = 0x%x\n", Result));


/* Loop up loop markers in MARK chunk info. 940921 */
DBUG(("ParseAIFFForm: NumMarkers = %d\n", spc->spc_NumMarkers));
DBUG(("ParseAIFFForm: SustainBeginMark = %d\n", spc->spc_SustainBeginMark));

	asmp = spc->spc_Sample;
	if (spc->spc_SustainLoopType)
	{
		int32 indx;
		indx = SearchArray (spc->spc_MarkerIDs, spc->spc_NumMarkers, spc->spc_SustainBeginMark);
		if (indx < 0) goto badindex;
		asmp->asmp_SustainBegin = spc->spc_MarkerPositions[indx];

		indx = SearchArray (spc->spc_MarkerIDs, spc->spc_NumMarkers, spc->spc_SustainEndMark);
		if (indx < 0) goto badindex;
		asmp->asmp_SustainEnd = spc->spc_MarkerPositions[indx];
	}
	else
	{
		asmp->asmp_SustainBegin = -1;
		asmp->asmp_SustainEnd = -1;
	}
	if (spc->spc_ReleaseLoopType)
	{
		int32 indx;
		indx = SearchArray (spc->spc_MarkerIDs, spc->spc_NumMarkers, spc->spc_ReleaseBeginMark);
		if (indx < 0) goto badindex;
		asmp->asmp_ReleaseBegin = spc->spc_MarkerPositions[indx];

		indx = SearchArray (spc->spc_MarkerIDs, spc->spc_NumMarkers, spc->spc_ReleaseEndMark);
		if (indx < 0) goto badindex;
		asmp->asmp_ReleaseEnd = spc->spc_MarkerPositions[indx]; /* Was Begin 930628*/
	}
	else
	{
		asmp->asmp_ReleaseBegin = -1;
		asmp->asmp_ReleaseEnd = -1;
	}
	
error:
	return Result;

badindex:
	ERR(("ParseAIFFForm: bad index in Mark\n"))
	return(AF_ERR_BADOFX);
}


/*****************************************************************/
int32 HandleAIFFChunk( iff_control *iffc, uint32 ChunkType , uint32 ChunkSize )
{
	int32 Result=0, i;
	int32 memsize;
	AudioSample *asmp;
	uint32 scratch[2];
	int32 *ip;
	SampleParserContext *spc;
#define AIFF_PAD_SIZE 250
	uint16 pad[AIFF_PAD_SIZE];  /* For reading structures */
	void *tmp;
	char *cp;
	int32 ChunkOffset;
	
	spc = (SampleParserContext *) iffc->iffc_UserContext;
	
	asmp = spc->spc_Sample;

TRACEB(TRACE_INT, TRACE_SAMPLE, ("HandleAIFFChunk: Chunk= %.4s, %d\n", &ChunkType, ChunkSize));

/* If not sample data, just read into scratch pad. */
	if ((ChunkType != ID_SSND) && (ChunkSize > 0))
	{
		if (ChunkSize > (AIFF_PAD_SIZE * sizeof(uint16)))
		{
			ERR(("AIFF Pad too small! "));
			ERR(("Chunk = %.4s, %d\n", &ChunkType, ChunkSize));
			Result = iffSkipChunkData(iffc, ChunkSize);
		}
		else
		{
			Result = iffReadChunkData(iffc, pad, ChunkSize);
		}
		if (Result < 0)
		{
			ERR(("Error reading CHUNK data = 0x%x\n", Result));
			goto error;
		}
	}
		
	switch(ChunkType)
	{
		case ID_SSND:
	
/* Read Offset and Block Alignment. */
			Result = iffReadChunkData(iffc, scratch, 2*sizeof(uint32));
			ChunkOffset = scratch[0];
DBUG(("SSND: ChunkOffset = %d\n", ChunkOffset ));
			asmp->asmp_DataSize = ChunkSize - 8 - ChunkOffset;  /* Account for offset in file. */
			if (spc->spc_IfReadData)
			{
				memsize = asmp->asmp_DataSize; 
				asmp->asmp_NumBytes = memsize;
DBUG(("SSND: NumBytes = %d\n", asmp->asmp_NumBytes ));
			}
			else
			{
				memsize = asmp->asmp_NumBytes; /* Just a scan. */
			}
/* Save position in File for later reads. */
			asmp->asmp_DataOffset = iffSkipChunkData(iffc, ChunkOffset);

			if (memsize > 0)
			{
				if(spc->spc_IfLeaveInPlace)
				{
					tmp = (char *) TellFlexStreamAddress( &iffc->iffc_FlexStream );
PRT(("HandleAIFFChunk: use spc_IfLeaveInPlace at 0x%08x\n", tmp));
/* Move data down if not quad aligned.  This will destroy AIFF image in RAM. */
/* !!! magic # - will need to change for M2 */
					if( (((uint32) tmp) & 3) != 0)
					{
						uint8 *NewTmp;
						
						NewTmp = (char *) (((uint32) tmp) & ~3);
						PRT(("AF_TAG_LEAVE_IN_PLACE had to quad align data:\n"));
						PRT(("  Move from 0x%x to 0x%x\n", tmp, NewTmp));
						memmove( NewTmp, tmp, memsize );
						tmp = NewTmp;
					}
				}
				else
				{   /* Allocate data using custom or regular allocator. */
					if (spc->spc_CustomAllocMem)
					{
						tmp = (*spc->spc_CustomAllocMem)(memsize+sizeof(int32), MEMTYPE_AUDIO);
						if (tmp)
						{
							ip = (int32 *) tmp;
							*ip++ = memsize;   /* Save size at beginning of data. */
							tmp = (void *) ip;
						}
DBUG(("HandleAIFFChunk: use spc_CustomAllocMem at 0x%08x\n", tmp));
					}
					else
					{
						tmp = EZMemAlloc(memsize, MEMTYPE_AUDIO);
DBUG(("HandleAIFFChunk: allocate %d bytes  at 0x%08x\n", memsize, tmp));
					}
				
					if (tmp == NULL)
					{
						ERR(("HandleAIFFChunk: Could not allocate sample memory, size = %d\n", memsize));
						Result = AF_ERR_NOMEM;
						goto error;
				
					}
					asmp->asmp_SuperFlags |= AF_SAMPF_FOLIO_OWNS; /* 941216 */
				}
				asmp->asmp_Data = tmp;
				
				if (spc->spc_IfReadData && !spc->spc_IfLeaveInPlace)
				{
					Result = iffReadChunkData(iffc, tmp, memsize);
					if (Result < 0)
					{
						ERR(("Error reading SSND data = 0x%x\n", Result));
						goto error;
					}
				}
				else
				{
					iffSkipChunkData(iffc, asmp->asmp_DataSize); /* Skip any remaining data. */
				}
			}
			else
			{
				iffSkipChunkData(iffc, asmp->asmp_DataSize); /* Skip any remaining data. */
			}
			break;

/* Read values from COMM chunk */
		case ID_COMM:
			asmp->asmp_Channels = (uint8) Read16(&pad[0]);
			asmp->asmp_NumFrames =
				((int32)(Read16(&pad[1])) << 16) + (int32) Read16(&pad[2]);
DBUG(("COMM: NumFrames = %d\n", asmp->asmp_NumFrames ));

			asmp->asmp_Bits = (uint8) Read16(&pad[3]);
			asmp->asmp_Width = (uint8) (((uint32)asmp->asmp_Bits) + 7)>>3;
			asmp->asmp_SampleRate = ufrac16FromIeee(&pad[4]);

			if (spc->spc_IfCompressed) asmp->asmp_CompressionType =
				(((uint32)(Read16(&pad[9]))) << 16) + (uint32)Read16(&pad[10]);
/* Convert sample rate. */
			switch(asmp->asmp_CompressionType)
			{
				case 0:
					asmp->asmp_CompressionRatio = 1;
					break;
				case ID_SDX2:
					asmp->asmp_CompressionRatio = 2;
					break;
				case ID_SDX3:
					asmp->asmp_CompressionRatio = 3;
					break;
				case ID_ADP4:
					asmp->asmp_CompressionRatio = 4;
					break;
				default:
					ERR(("Unrecognized compression type = 0x%x\n",
						asmp->asmp_CompressionType));
	/* Try to guess at compression ratio based on last char. */
					cp = (char *) &asmp->asmp_CompressionType;
					asmp->asmp_CompressionRatio = cp[3] - '0';
					if ((asmp->asmp_CompressionRatio < 1) ||
						(asmp->asmp_CompressionRatio > 9))
					{
						 asmp->asmp_CompressionRatio = 2;
					}
			}
			break;
			
		case ID_FVER:
		/* %Q Do something with version. */
			break;
			
		case ID_MARK:
			spc->spc_NumMarkers = (int32) Read16(&pad[0]);
TRACEB(TRACE_INT, TRACE_SAMPLE, ("HandleAIFFChunk: NumMarkers = %d\n", spc->spc_NumMarkers));
			if (spc->spc_NumMarkers > AIFF_MAX_MARKERS) return AF_ERR_OVERMARKERS;
/* Pull out markers for Loop identification */
			cp = (char *) &pad[1];
			for (i=0; i<spc->spc_NumMarkers; i++)
			{
				uint16 *sp;
				sp = (uint16 *)cp;
				spc->spc_MarkerIDs[i] = Read16(&sp[0]);
TRACEB(TRACE_INT, TRACE_SAMPLE, ("HandleAIFFChunk: MarkerID = $%x, cp = $%x\n",
					spc->spc_MarkerIDs[i], cp));
				spc->spc_MarkerPositions[i] = (((int32)(Read16(&sp[1])))<<16)
				                            + (int32)(Read16(&sp[2]));
TRACEB(TRACE_INT, TRACE_SAMPLE, ("HandleAIFFChunk: Pos = $%x\n",
		spc->spc_MarkerPositions[i]));
				cp += 6;
				cp += *cp + 1; /* Skip Name */
				if ((int32) cp & 0x1) cp++; /* Even up */
			}
			break;
				
		case ID_INST:
			cp = (char *) pad;
			asmp->asmp_BaseNote = cp[0];
			asmp->asmp_Detune = cp[1];
			asmp->asmp_LowNote = cp[2];
			asmp->asmp_HighNote = cp[3];
			asmp->asmp_LowVelocity = cp[4];
			asmp->asmp_HighVelocity = cp[4];
/* Save Loop info until MARK chunk read. */
			spc->spc_SustainLoopType = (int32) Read16(&pad[4]);
			spc->spc_SustainBeginMark = (int32) Read16(&pad[5]);
			spc->spc_SustainEndMark = (int32) Read16(&pad[6]);
			spc->spc_ReleaseLoopType = (int32) Read16(&pad[7]);
			spc->spc_ReleaseBeginMark = (int32) Read16(&pad[8]);
			spc->spc_ReleaseEndMark = (int32) Read16(&pad[9]);
		break;
			
		case ID_APPL:
/* Ignore it. */
			break;
			
		default:
			ERR(("Unrecognized AIFF chunk = 0x%lx\n", ChunkType ));
			
	} 
			


	
TRACEB(TRACE_INT, TRACE_SAMPLE, ("HandleAIFFChunk: %d bytes remaining.\n", iffc->iffc_length));
TRACEB(TRACE_INT, TRACE_SAMPLE, ("HandleAIFFChunk at end: NumMarkers = %d\n", spc->spc_NumMarkers));

	return(0);
	
error:
/* %Q cleanup */
	return(Result);

}

/*****************************************************************/
int32 internalSetSampleInfo (AudioSample *asmp, TagArg *args )
{
  	int32 Result = 0;  	
	uint32 tagc, *tagp, temp;
	int32 stemp;
	int32 ChangedNFrames, ChangedNBytes, UpdateSize;
	uint32 NumFrames, NumBytes;
	int32 UpdateBaseFreq;
	
  	ChangedNFrames = ChangedNBytes = UpdateSize = UpdateBaseFreq = FALSE;
  	NumFrames = 0;
  	NumBytes = 0;
  	
	tagp = (uint32 *)args;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
DBUG(("internalSetSampleInfo: Asmp = 0x%x, Tag = %d, Arg = $%x\n", asmp, tagc, *tagp));
			temp = *tagp++;
			
			switch (tagc)
			{
/* NAME and SAMPLE are handled by internalCreateAudioSample and so are allowed to pass. */
			case AF_TAG_NAME:
				break;
			case AF_TAG_SAMPLE:
				break;
			case AF_TAG_DELAY_LINE:
				break;
				
			case AF_TAG_BASENOTE: /* MIDI note when played at 44.1 Khz */
				asmp->asmp_BaseNote = (uint8) temp;	
  				UpdateBaseFreq = TRUE;
				break;
			case AF_TAG_DETUNE:
				stemp = (int32) temp;
				if(( stemp < -100 ) || ( stemp > 100))
				{
					ERR(("internalSetSampleInfo: Detune out of range = %d\n", stemp));
					Result = AF_ERR_OUTOFRANGE;
					goto error;
				}
				asmp->asmp_Detune = (int8) stemp;
  				UpdateBaseFreq = TRUE;
				break;
				
			case AF_TAG_LOWNOTE:  /* lowest note to use when multisampling */
				asmp->asmp_LowNote = (uint8) temp;
				break;
			case AF_TAG_HIGHNOTE:  /* highest note to use when multisampling */
				asmp->asmp_HighNote = (uint8) temp;
				break;
			case AF_TAG_LOWVELOCITY:
				asmp->asmp_LowVelocity = (uint8) temp;
				break;
			case AF_TAG_HIGHVELOCITY:
				asmp->asmp_HighVelocity = (uint8) temp;
				break;
			case AF_TAG_SUSTAINBEGIN:
				asmp->asmp_SustainBegin = temp;
				break;
			case AF_TAG_SUSTAINEND:
				asmp->asmp_SustainEnd = temp;
				break;
			case AF_TAG_RELEASEBEGIN:
				asmp->asmp_ReleaseBegin = temp;
				break;
			case AF_TAG_RELEASEEND:
				asmp->asmp_ReleaseEnd = temp;
				break;
			case AF_TAG_SAMPLE_RATE:
				asmp->asmp_SampleRate = temp;
  				UpdateBaseFreq = TRUE;
				break;
			case AF_TAG_COMPRESSIONRATIO:
				if( temp < 1 )
				{
					ERR(("Compression ratio < 1, = %d\n", temp));
					Result = AF_ERR_OUTOFRANGE;
					goto error;
				}
				asmp->asmp_CompressionRatio = (uint8) temp;
				UpdateSize = TRUE;  /* 941128 CR 3830 */
				break;

			case AF_TAG_COMPRESSIONTYPE:
				asmp->asmp_CompressionType = temp;
				break;
			
			case AF_TAG_WIDTH:  /* bytes per sample UNCOMPRESSED */
				if (temp > 2)
				{
					Result = AF_ERR_BADTAGVAL;
					goto error;
				}
				asmp->asmp_Width = (uint8) temp;
				asmp->asmp_Bits = (uint8) ((uint32)(temp) << 3); /* 941128 CR3818 */
				UpdateSize = TRUE;
				break;
				
			case AF_TAG_NUMBITS:  /* bits per sample UNCOMPRESSED */
				if ((temp < 1) || (temp > 32))
				{
					Result = AF_ERR_BADTAGVAL;
					goto error;
				}
				asmp->asmp_Bits = (uint8) temp;
				asmp->asmp_Width = (uint8) ((uint32)(temp + 7) >> 3);
				UpdateSize = TRUE;
				break;
			
			case AF_TAG_CHANNELS:  /* samples per frame */
				if ((temp < 1) || (temp > 255))
				{
					Result = AF_ERR_BADTAGVAL;
					goto error;
				}
				asmp->asmp_Channels = (uint8) temp;
				UpdateSize = TRUE;
				break;

#define TEST_SUPER_ALLOC(msg) \
	if(asmp->asmp_SuperFlags & AF_SAMPF_SUPER_ALLOC) \
	{ \
		ERR((msg)); \
		Result = AF_ERR_SECURITY; \
		goto error;	\
	}

			case AF_TAG_ADDRESS:
				TEST_SUPER_ALLOC("Cannot change delay line address.\n");
				asmp->asmp_Data = (void *) temp;
				if (asmp->asmp_SuperFlags & AF_SAMPF_FOLIO_OWNS)
				{
					ERR(("Warning: internalSetSampleInfo, Folio owned data pointer lost.\n"));
					asmp->asmp_SuperFlags &= ~AF_SAMPF_FOLIO_OWNS;
				}
				break;
				
/* 940614  Added this tag to SWI to patch sample address. */
			case AF_TAG_INTERNAL_1:
				TEST_SUPER_ALLOC("Cannot change delay line address.\n");
				asmp->asmp_Data = (void *) temp;
				asmp->asmp_BadDogData = (void *) NULL;
				asmp->asmp_SuperFlags |= AF_SAMPF_FOLIO_OWNS;
				break;
				
			case AF_TAG_NUMBYTES:
				TEST_SUPER_ALLOC("Cannot change delay line size.\n");
				NumBytes = (uint32) temp;
				ChangedNBytes = TRUE;
				break;
				
			case AF_TAG_FRAMES: /* number of frames, in stereo a frame is 2 samples */
				TEST_SUPER_ALLOC("Cannot change delay line size.\n");
				NumFrames = (uint32) temp;
				ChangedNFrames = TRUE;
				break;
				
			default:
		ERR (("Warning: Unrecognized Tag ID in internalSetSampleInfo = 0x%x, 0x%x\n",
					tagc, temp));
				Result = AF_ERR_BADTAG;
				goto error;
			}
		}
	}
	
/* What if #Channels gets changed while servicing NexAttachment? %Q */
	if (ChangedNFrames)
	{
		asmp->asmp_NumFrames = NumFrames;
		asmp->asmp_NumBytes = CvtFrameToByte( asmp->asmp_NumFrames, asmp );
		UpdateSize = FALSE;
	}
	
	if (ChangedNBytes)
	{
		asmp->asmp_NumBytes = NumBytes;
		asmp->asmp_NumFrames = CvtByteToFrame( asmp->asmp_NumBytes, asmp );
		UpdateSize = FALSE;
	}

	if(UpdateSize)
	{
		asmp->asmp_NumFrames = CvtByteToFrame( asmp->asmp_NumBytes, asmp);
	}
	
/* Process changes to data size. */
	if (ChangedNFrames && ChangedNBytes)
	{
		if (NumFrames != asmp->asmp_NumFrames)
		{
			ERR(("Number of Frames and Number of Bytes don't agree!\n")); /* 940606 Add \n. */
			Result = AF_ERR_BADTAGVAL;
			goto error;
		}
	}

DBUG(("SetInfo: addr=0x%x, nb=0x%x,",
	asmp->asmp_Data, asmp->asmp_NumBytes));
DBUG((" nf=0x%x, width=0x%x, cr=%d\n",
	asmp->asmp_NumFrames, asmp->asmp_Width, asmp->asmp_CompressionRatio));
/*
** Validate sample data.
*/
	if ((asmp->asmp_Data != NULL))
	{
		Result = afi_IsRamAddr( (char *) asmp->asmp_Data, asmp->asmp_NumBytes );
		CHECKRSLT(("internalSetSampleInfo: Sample data address not in RAM\n"));
	}
	
	if(asmp->asmp_SustainBegin >= 0)  /* 940203 was >0 */
	{
		if((asmp->asmp_SustainBegin > asmp->asmp_SustainEnd) ||
		   (asmp->asmp_SustainEnd > asmp->asmp_NumFrames))
		{
			ERR(("Sustain loop out of range. %d<>%d, %d\n",
				asmp->asmp_SustainBegin, asmp->asmp_SustainEnd, asmp->asmp_NumFrames));
			Result = AF_ERR_OUTOFRANGE;
			goto error;
		}
	}
	if(asmp->asmp_ReleaseBegin >= 0)  /* 940203 was >0 */
	{
		if((asmp->asmp_ReleaseBegin > asmp->asmp_ReleaseEnd) ||
		   (asmp->asmp_ReleaseEnd > asmp->asmp_NumFrames))
		{
			ERR(("Release loop out of range. %d<>%d, %d\n",
				asmp->asmp_ReleaseBegin, asmp->asmp_ReleaseEnd, asmp->asmp_NumFrames));
			Result = AF_ERR_OUTOFRANGE;
			goto error;
		}
	}

	if(UpdateBaseFreq)
	{
		Result = CalcSampleBaseFreq( asmp );
	}
	
	return Result;
	
error:
	return Result;
}

/******************************************************************
** 940614 Some app has called MakeSample() incorrectly and then queried for
**  the allocated memory.  So it must have intended to allocate memory instead
**  of using the passed data to MakeSample().  So we will use the allocated data.
**  We have to call a SWI to make the BadDogData address the current address.
*******************************************************************/
static void PatchBadDogData( AudioSample *asmp )
{
	TagArg Tags[2];
	
	Tags[0].ta_Tag = AF_TAG_INTERNAL_1;
	Tags[0].ta_Arg = (void *) asmp->asmp_BadDogData;
	Tags[1].ta_Tag =  TAG_END;
	SetAudioItemInfo( asmp->asmp_Item.n_Item, Tags );  /* SWI to do dirty work. */
}

/*****************************************************************/
int32 internalGetSampleInfo (AudioSample *asmp, TagArg *args)
{
  	int32 Result = 0;  	
	uint32 tagc, *tagp;
  	
	tagp = (uint32 *)args;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
DBUG(("internalGetSampleInfo: Tag = %d, Arg = $%x\n", tagc, *tagp));
			switch (tagc)
			{
			case AF_TAG_ADDRESS:
				if( asmp->asmp_BadDogData )
				{
					PRT(("GetAudioItemInfo: address queried so use allocated data.\n"));
					PatchBadDogData( asmp ); /* 940614 */
				}
				*tagp++ = (uint32) asmp->asmp_Data;
				break;
			case AF_TAG_NUMBYTES:
				*tagp++ = (uint32) asmp->asmp_NumBytes;
				break;
			case AF_TAG_WIDTH:  /* bytes per sample */
				*tagp++ = asmp->asmp_Width;
				break;
			case AF_TAG_CHANNELS:  /* samples per frame */
				*tagp++ = (uint32) asmp->asmp_Channels;
				break;
			case AF_TAG_FRAMES: /* number of frames, in stereo a frame is 2 samples */
				*tagp++ = (uint32) asmp->asmp_NumFrames;
				break;
			case AF_TAG_BASENOTE: /* MIDI note when played at 44.1 Khz */
				*tagp++ = (uint32) asmp->asmp_BaseNote;
				break;
			case AF_TAG_DETUNE:
				*tagp++ = (uint32) asmp->asmp_Detune;
				break;
			case AF_TAG_LOWNOTE:  /* lowest note to use when multisampling */
				*tagp++ = (uint32) asmp->asmp_LowNote;
				break;
			case AF_TAG_HIGHNOTE:  /* highest note to use when multisampling */
				*tagp++ = (uint32) asmp->asmp_HighNote;
				break;
			case AF_TAG_LOWVELOCITY:
				*tagp++ = (uint32) asmp->asmp_LowVelocity;
				break;
			case AF_TAG_HIGHVELOCITY:
				*tagp++ = (uint32) asmp->asmp_HighVelocity;
				break;
			case AF_TAG_SUSTAINBEGIN:
				*tagp++ = (uint32) asmp->asmp_SustainBegin;
				break;
			case AF_TAG_SUSTAINEND:
				*tagp++ = (uint32) asmp->asmp_SustainEnd;
				break;
			case AF_TAG_RELEASEBEGIN:
				*tagp++ = (uint32) asmp->asmp_ReleaseBegin;
				break;
			case AF_TAG_RELEASEEND:
				*tagp++ = (uint32) asmp->asmp_ReleaseEnd;
				break;
			case AF_TAG_SAMPLE_RATE:
				*tagp++ = (uint32) asmp->asmp_SampleRate;
				break;
			case AF_TAG_BASEFREQ:
				*tagp++ = (uint32) asmp->asmp_BaseFreq;
				break;
			case AF_TAG_COMPRESSIONRATIO:
				*tagp++ = (uint32) asmp->asmp_CompressionRatio;
				break;
			case AF_TAG_COMPRESSIONTYPE:
				*tagp++ = (uint32) asmp->asmp_CompressionType;
				break;
			case AF_TAG_NUMBITS:
				*tagp++ = (uint32) asmp->asmp_Bits;
				break;
			case AF_TAG_DATA_OFFSET:
				*tagp++ = (uint32) asmp->asmp_DataOffset;
				break;
			case AF_TAG_DATA_SIZE:
				*tagp++ = (uint32) asmp->asmp_DataSize;
				break;
			
			default:
				if(tagc > TAG_ITEM_LAST)
				{
					ERR (("Warning - unrecognized tag in internalGetSampleInfo - 0x%x: 0x%x\n",
					tagc, *tagp));	
					Result = AF_ERR_BADTAG;
					goto DONE;
				}
				tagp++;
			}
		}
	}
DONE:
	return Result;
}

/**************************************************************
** Frame to byte conversions moved from dspp_touch.c 940907
**************************************************************/
uint32 CvtFrameToByte(uint32 Frame, AudioSample *asmp)
{
	 if (asmp->asmp_CompressionRatio == 1)
	 {
	 	return (Frame * asmp->asmp_Channels * asmp->asmp_Width);
	 }
	 if (asmp->asmp_CompressionRatio == 2)
	 {
	 	return ((Frame * asmp->asmp_Channels * asmp->asmp_Width) >> 1);
	 }
	 
	 return (Frame * asmp->asmp_Channels * asmp->asmp_Width /
	 	asmp->asmp_CompressionRatio);
}


uint32 CvtByteToFrame(uint32 Byte, AudioSample *asmp)
{
	 if (asmp->asmp_CompressionRatio == 1)
	 {
	 	return (Byte / (asmp->asmp_Channels * asmp->asmp_Width) );
	 }
	 if (asmp->asmp_CompressionRatio == 2)
	 {
/* This was causing the number of bytes and frames to disagree.  501 => 500. */
/*	 	return ((Byte / (asmp->asmp_Channels * asmp->asmp_Width)) << 1);  940606 Roundoff error! */
	 	return ( (Byte << 1) / (asmp->asmp_Channels * asmp->asmp_Width) );
	 }
	 
	 return ( (Byte * asmp->asmp_CompressionRatio) / (asmp->asmp_Channels * asmp->asmp_Width) );
}

/**************************************************************
** Update sample base frequencies. 940506
**************************************************************/
int32 UpdateAllSampleBaseFreqs( void )
{
	Node *n;
	int32 Result = 0;
	
	n = FirstNode( &AudioBase->af_SampleList );
	while (ISNODE( &AudioBase->af_SampleList, n))
	{
		Result = CalcSampleBaseFreq( (AudioSample *) n );
		if( Result < 0 ) return Result;
		n = NextNode( n );
	}
	
	return Result;
}

/*****************************************************************/
int32 CalcSampleBaseFreq( AudioSample *asmp )
{
	int32 Result;
	ufrac16 PitchFreq;
	frac16 DetuneFrac;
	
/* Calculate samples Base Frequency. */
	Result = PitchToFrequency( AudioBase->af_DefaultTuning, asmp->asmp_BaseNote,
		&PitchFreq );
	if(Result < 0) return Result;
	asmp->asmp_BaseFreq = MulUF16( PitchFreq,
		DivUF16( Convert32_F16(DSPPData.dspp_SampleRate), asmp->asmp_SampleRate));

	if( asmp->asmp_Detune != 0 )
	{
		Result = Convert12TET_F16( 0, -(asmp->asmp_Detune), &DetuneFrac );
		if(Result < 0) return Result;
		asmp->asmp_BaseFreq = MulUF16( asmp->asmp_BaseFreq, (ufrac16) DetuneFrac);
	}
TRACEB(TRACE_INT, TRACE_TUNING|TRACE_SAMPLE,
		("asmp->asmp_BaseNote = 0x%x, asmp->asmp_BaseFreq = 0x%x\n",
		asmp->asmp_BaseNote, asmp->asmp_BaseFreq));

	return Result;
}

/**************************************************************/

uint32 CalcSampleNoteRate ( AudioInstrument *ains, AudioSample *asmp, int32 Note)
{
	int32 Result;
	ufrac16 Fraction, Freq;
	DSPPInstrument *dins;
	
TRACEE(TRACE_INT, TRACE_TUNING|TRACE_SAMPLE, ("CalcSampleNoteRate(ains=0x%lx, asmp=0x%lx, note=%d)\n",
			ains, asmp, Note));
			
	Result = PitchToFrequency( GetInsTuning(ains), Note, &Freq);
	if( Result) return 0x8000;
	
/* Shift Frequency by Shift Rate to compensate for execution rate. 940817 */
	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
	Freq = Freq << dins->dins_RateShift;
	
	Fraction = DivUF16( Freq, asmp->asmp_BaseFreq );
	
TRACEB(TRACE_INT, TRACE_TUNING|TRACE_SAMPLE,
		("CalcSampleNoteRate: Fraction=0x%lx, Freq=0x%lx, BaseFreq=%d\n",
		Fraction, Freq, asmp->asmp_BaseFreq));
		
	Fraction = Fraction >> 1;     /* For 1.15 FP DSP instrument. */
	if(Fraction > 0xFFFF) Fraction = 0xFFFF;
	
TRACER(TRACE_INT, TRACE_TUNING|TRACE_SAMPLE, ("CalcSampleNoteRate returns 0x%lx\n", Fraction ));

	return (uint32) Fraction;
}


/*****************************************************************/
/***** Create Sample Item for Folio ******************************/
/*****************************************************************/
Item internalCreateAudioSample (AudioSample *asmp, TagArg *args)
{
  	char *Name = NULL, *s, *d;
  	int32 Result, NumBytes;
	uint32 tagc, *tagp;
	void *AllocatedMem;
	
	AllocatedMem = NULL;
	
    Result = TagProcessor( asmp, args, afi_DummyProcessor, 0);
    if(Result < 0)
    {
    	ERR(("internalCreateAudioSample: TagProcessor failed.\n"));
    	return Result;
    }

	tagp = (uint32 *)args;

	DefaultSample (asmp);

DBUG(("internalCreateAudioSample\n"));

	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
			switch (tagc)
			{
			case AF_TAG_NAME:
				Name = (char *) *tagp++;  /* %Q Validate, but not currently used. */
				break;
				
			case AF_TAG_DELAY_LINE:
/*
** Allocate delay line from supervisor memory so user can't free.
*/
				NumBytes = *tagp++;
				AllocatedMem = EZMemAlloc( NumBytes, MEMTYPE_AUDIO | MEMTYPE_FILL );
				if( AllocatedMem == NULL )
				{
					Result = AF_ERR_NOMEM;
					goto error;
				}
				asmp->asmp_Data = AllocatedMem;
/* Set flags so this can't be freed by user. */
				asmp->asmp_SuperFlags |= 
					(AF_SAMPF_SUPER_ALLOC | AF_SAMPF_FOLIO_OWNS);
				asmp->asmp_NumBytes = NumBytes;
				break;
				
			case AF_TAG_SAMPLE:
/*
** Copy sample passed in from user mode to super sample.
** Don't clobber ItemNode at beginning
*/
				s = (char *) *tagp++;
				
				if(asmp->asmp_SuperFlags & AF_SAMPF_SUPER_ALLOC)
				{
					ERR(("Cannot use AF_TAG_SAMPLE after AF_TAG_DELAY_LINE!\n"));
					Result = AF_ERR_SECURITY;
					goto error;
				}
				else
				{
					Result = afi_IsRamAddr( s, sizeof(AudioSample) );
					CHECKRSLT(("Sample structure address not in RAM"));
					s += sizeof(ItemNode);
					d  = (char *) asmp;
					d += sizeof(ItemNode);
					bcopy( s, d, (sizeof(AudioSample) - sizeof(ItemNode)) );
				}
				
				if(asmp->asmp_SuperFlags & AF_SAMPF_SUPER_ALLOC)
				{
					ERR(("Cannot set AF_SAMPF_SUPER_ALLOC in AF_TAG_SAMPLE\n"));
					Result = AF_ERR_SECURITY;
					goto error;
				}
				break;
				
			default:
				tagp++;
			}
		}
	}

	asmp->asmp_NumFrames = CvtByteToFrame( asmp->asmp_NumBytes, asmp );
	
/* Parse remaining tags to allow overwriting file info. */
	Result = internalSetSampleInfo(asmp, args);
	CHECKRSLT(("Bad Tag value in internalCreateAudioSample\n"));

/*
** Validate sample data.
*/
	if ((asmp->asmp_Data != NULL))
	{
		Result = afi_IsRamAddr( (char *) asmp->asmp_Data, asmp->asmp_NumBytes );
		CHECKRSLT(("internalCreateAudioSample: Sample data address not in RAM\n"));
	}

/* Init List of attachments. */
	InitList(&asmp->asmp_AttachmentRefs, "SampleRefs");
	
/* Connect Sample to Folio List */
	AddTail(&AudioBase->af_SampleList, (Node *) asmp);
	
	Result = CalcSampleBaseFreq( asmp );
	if(Result < 0) return Result;
	
/* Everything OK so pass back item number. */
	Result = asmp->asmp_Item.n_Item;
	return Result;
	
error:
	if( AllocatedMem ) EZMemFree( AllocatedMem );
	return Result;
}

/******************************************************************/
Item internalOpenAudioSample (AudioSample *asmp, void *args)
{
  DBUG (("OpenAudioSample (0x%x, 0x%lx)\n", args));
	
	asmp->asmp_OpenCount += 1;  /* Bump reference count  */
	return (asmp->asmp_Item.n_Item);
}

/**************************************************************/

int32 internalCloseAudioSample (Item it, AudioSample *asmp)
{
	Item ret;
	
/* Decrement count, if 0 then delete it. */
	if(--(asmp->asmp_OpenCount) <= 0)
	{
		ret = afi_SuperDeleteItem(it);
		return (ret != it); /* %Q?? */
	}
	return 0;
}

/**************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/unloadsample
 |||	UnloadSample - Deletes a sample and frees its resources.
 |||
 |||	  Synopsis
 |||
 |||	    Err UnloadSample( Item SampleItem )
 |||
 |||	  Description
 |||
 |||	    This procedure deletes a sample from memory, freeing its resources.  If
 |||	    the sample was created using a custom memory-allocation call,
 |||	    UnloadSample() calls a corresponding custom memory-deallocation call to
 |||	    free the sample's memory.
 |||
 |||	  Arguments
 |||
 |||	    SampleItem                   Item number of sample to delete.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    AttachSample(), DetachSample(), SetAudioItemInfo(), LoadSample(),
 |||	    DebugSample(), CreateSample(), ScanSample(), GetAudioItemInfo()
 |||
 **/
int32 UnloadSample ( Item SampleItem  )
{
	int32 Result, IfFree;
	void *data;
	AudioSample *asmp;
	int32 *p, size;
	void (*CustomFree)();

TRACEE(TRACE_TOP,TRACE_SAMPLE,("UnloadSample( 0x%x )\n", SampleItem));
	asmp = (AudioSample *)CheckItem(SampleItem, AUDIONODE, AUDIO_SAMPLE_NODE);
	if (asmp == NULL) return AF_ERR_BADITEM;
	
/* Get values from Sample before we delete it. */
	data = asmp->asmp_Data;
	IfFree = ((asmp->asmp_SuperFlags & AF_SAMPF_FOLIO_OWNS) && 
		!(asmp->asmp_SuperFlags & AF_SAMPF_SUPER_ALLOC));  /* Don't if delay line. 930907 */
	CustomFree = asmp->asmp_CustomFreeMem;
	
	Result = DeleteItem ( SampleItem );
	if (data)
	{
		if ((Result == 0)  && IfFree)
		{
TRACEE(TRACE_INT,TRACE_SAMPLE,("UnloadSample: attempt to free 0x%x....", data));
            DBUG(("UnloadSample: attempt to free mem @ $%08lx\n", data));
			if (CustomFree)
			{
				p = (int32 *) data;
				p--;   /* point to saved size */
				size = *p;
				(*CustomFree)((void *) p, size + sizeof(int32));
			}
			else
			{
				EZMemFree( data );
			}
TRACEE(TRACE_INT,TRACE_SAMPLE,("free completed.\n"));
		}
	}
	
	return Result;
}

/**************************************************************/

int32 internalDeleteAudioSample (AudioSample *asmp)
{	
	int32 Result = 0;
	AudioReferenceNode *arnd;
	
TRACEE(TRACE_INT,TRACE_ITEM,("internalDeleteAudioSample( 0x%x)\n", asmp));

/* Stop all attachments to avoid noise or DMA write to free memory. */
	arnd = (AudioReferenceNode *)FirstNode( &asmp->asmp_AttachmentRefs );
	while (ISNODE( &asmp->asmp_AttachmentRefs, (Node *) arnd))
	{
		swiStopAttachment( arnd->arnd_RefItem, NULL);
		arnd = (AudioReferenceNode *) NextNode((Node *) arnd);
	}

/* Free memory allocated for Delay Lines. */
	if(asmp->asmp_SuperFlags & AF_SAMPF_SUPER_ALLOC)
	{
		if(asmp->asmp_Data)
		{
			EZMemFree( asmp->asmp_Data );
			asmp->asmp_Data = NULL;
		}
		asmp->asmp_SuperFlags &= ~( AF_SAMPF_SUPER_ALLOC | AF_SAMPF_FOLIO_OWNS );
	}
		
	Result = afi_DeleteReferencedItems( &asmp->asmp_AttachmentRefs );
	
	if (asmp->asmp_OpenCount > 0)
	{
		ERR(("Cannot Delete a sample that is still open.\n"));
		Result = AF_ERR_INUSE;
	}
	else
	{
/* Remove from list of samples in AudioFolio structure. */
		ParanoidRemNode( (Node *) asmp );
	}
	
TRACER(TRACE_INT,TRACE_ITEM,("internalDeleteAudioSample returns 0x%x)\n", Result));
	return Result;
}

/*****************************************************************/
/***** SUPERVISOR Level ******************************************/
/*****************************************************************/

/*************************************************************************/
static int32 SearchArray(int32 *Pnts, int32 NumPnts, int32 Value)
{
	int32 i;
DBUG(("SearchArray: $%x, $%x, $%x\n", Pnts, NumPnts, Value));
	for (i=0; i<NumPnts; i++)
	{
DBUG(("Pnt[%d] = $%x\n", i, Pnts[i]));
		if (Pnts[i] == Value)
		{
			return i;
		}
	}
	return -1;
}

/**************************************************************************
** PutSampleInfo() is very old and obsolete and was never documented
** in an official release.  There is a remote chance, however,
** that something is using it so we will support it for awhile.
** We must take advantage, however, of the security checks in
** internalSetAudioItemInfo() so we will call that directly.
**************************************************************************/
int32 swiPutSampleInfo( Item SampleItem, TagArg *args )
{
	ERR(("Warning: obsolete function swiPutSampleInfo() called.\n"));
	return swiSetAudioItemInfo( SampleItem, args );
}

/*************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/attachsample
 |||	AttachSample - Connects sample to instrument's FIFO.
 |||
 |||	  Synopsis
 |||
 |||	    Item AttachSample( Item Instrument, Item Sample, char
 |||	    *FIFOName )
 |||
 |||	  Description
 |||
 |||	    This procedure connects a sample to a named input FIFO for an instrument.
 |||	    This sample will be played when the instrument is started.  See the
 |||	    instrument's documentation for the names of its FIFOs.  If the
 |||	    procedure is successful, it returns an attachment item number.
 |||
 |||	    When you finish with the attachment between sample and instrument, you
 |||	    should call DetachSample() to detach and free the attachment's
 |||	    resources.
 |||
 |||	    Multisamples:  If more than one sample is attached to a FIFO and the
 |||	    instrument is started with a specified pitch, then the list of samples is
 |||	    searched for the first sample whose range of notes and velocities matches
 |||	    the desired note index (pitch).  The LowNote and HighNote, and LowVelocity
 |||	    and HighVelocity are read from the AIFF file.  The values from the file
 |||	    can be overwritten using SetAudioItemInfo().
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   The item number of the instrument.
 |||
 |||	    Sample                       The item number of the sample.
 |||
 |||	    FIFOName                     Name of an input FIFO (set to NULL for the
 |||	                                 first or only FIFO).
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number of the attachment or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    DetachSample(), SetAudioItemInfo(), LoadSample(), UnloadSample(),
 |||	    DebugSample(), CreateSample(), ScanSample(), GetAudioItemInfo()
 |||
 **/
Item AttachSample( Item Instrument, Item Sample, char *FIFOName )
{
	Item Result = -1;
	TagArg Tags[4];

TRACEE(TRACE_INT,TRACE_SAMPLE,("AttachSample( 0x%x, 0x%x, %s )\n", Instrument,
		Sample, FIFOName));

	Tags[0].ta_Tag = AF_TAG_INSTRUMENT;
	Tags[0].ta_Arg = (void *) Instrument;
	Tags[1].ta_Tag = AF_TAG_SAMPLE;
	Tags[1].ta_Arg = (void *) Sample;
	if (FIFOName == NULL)
	{
		Tags[2].ta_Tag =  TAG_END;
	}
	else
	{
		Tags[2].ta_Tag = AF_TAG_HOOKNAME;
		Tags[2].ta_Arg = (void *) FIFOName;
		Tags[3].ta_Tag =  TAG_END;
	}
	
    Result = CreateItem( MKNODEID(AUDIONODE,AUDIO_ATTACHMENT_NODE), Tags );
TRACER(TRACE_INT, TRACE_SAMPLE, ("AttachSample returns 0x%08x\n", Result));

	return Result;
}

/*************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/detachsample
 |||	DetachSample - Disconnects sample from instrument.
 |||
 |||	  Synopsis
 |||
 |||	    Err DetachSample( Item Attachment )
 |||
 |||	  Description
 |||
 |||	    This procedure disconnects a  sample from an instrument, deletes the
 |||	    attachment item connecting the two, and frees the attachment's
 |||	    resources.
 |||
 |||	  Arguments
 |||
 |||	    Attachment                   The item number of the attachment.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    AttachSample(), SetAudioItemInfo(), LoadSample(), UnloadSample(),
 |||	    DebugSample(), CreateSample(), ScanSample(), GetAudioItemInfo()
 |||
 **/
int32 DetachSample( Item Attachment )
{
	return DeleteItem( Attachment );
}
