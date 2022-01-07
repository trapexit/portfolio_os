/* $Id: autodocs.c,v 1.6 1995/02/14 00:53:19 vertex Exp $ */

/**
|||	AUTODOC PUBLIC spg/compression/createcompressor
|||	CreateCompressor - Create a compression engine
|||
|||	  Synopsis
|||
|||	    Err CreateCompressor(Compressor **comp, CompFunc cf,
|||	                         const TagArg *tags);
|||
|||	    Err CreateCompressorVA(Compressor **comp, CompFunc cf,
|||	                           uint32 tags, ...);
|||
|||	  Description
|||
|||	    Creates a compression engine. Once the engine is created, you
|||	    can call FeedCompressor() to have the engine compress the
|||	    data you supply.
|||
|||	  Arguments
|||
|||	    comp                        A pointer to a Compressor variable,
|||	                                where a handle to the compression
|||	                                engine can be put.
|||
|||	    cf                          A data output function. Every word of
|||	                                compressed data is sent to this
|||	                                function. This function is called with
|||	                                two parameters: one is a user-data
|||	                                value as supplied with the
|||	                                COMP_TAG_USERDATA tag. The other is the
|||	                                word of compressed data being output by
|||	                                the compressor.
|||
|||	    tags                        A pointer to an array of tag arguments
|||	                                containing extra data for this
|||	                                function. See below for a description
|||	                                of the tags supported.
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function. The array must be terminated with TAG_END.
|||
|||	    COMP_TAG_WORKBUFFER (void *)
|||	                                A pointer to a work buffer. This buffer
|||	                                is used by the compressor to store
|||	                                state information. If this tag is not
|||	                                supplied, the buffer is allocated and
|||	                                freed by the folio automatically. To
|||	                                obtain the required size for the
|||	                                buffer, call the
|||	                                GetCompressorWorkBufferSize() function.
|||	                                The buffer you supply must be aligned
|||	                                on a 4-byte boundary, and must
|||	                                remain valid until DeleteCompressor()
|||	                                is called. When you supply a work
|||	                                buffer, this routine allocates no
|||	                                memory of its own.
|||
|||	    COMP_TAG_USERDATA (void *)
|||	                                A value that the compressor will pass
|||	                                to 'cf' when it is called. This value
|||	                                can be anything you want. For example,
|||	                                it can be a pointer to a private data
|||	                                structure containing some context such
|||	                                as a file handle. If this tag is
|||	                                not supplied, then NULL is passed
|||	                                to 'cf' when it is called.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code if the
|||	    compression engine could not be created. Possible error codes
|||	    include:
|||
|||	    COMP_ERR_BADPTR             An invalid output function pointer,
|||	                                or work buffer was supplied.
|||
|||	    COMP_ERR_BADTAG             An unknown tag was supplied.
|||
|||	    COMP_ERR_NOMEM              There was not enough memory to
|||	                                initialize the compressor.
|||
|||	  Implementation
|||
|||	    Folio call implemented in compression folio V24.
|||
|||	  Associated Files
|||
|||	    compression.h
|||
|||	  See Also
|||
|||	    FeedCompressor(), DeleteCompressor(), GetCompressorWorkBufferSize(),
|||	    CreateDecompressor()
|||
**/

/**
|||	AUTODOC PUBLIC spg/compression/deletecompressor
|||	DeleteCompressor - Delete a compression engine
|||
|||	  Synopsis
|||
|||	    Err DeleteCompressor(Compressor *comp);
|||
|||	  Description
|||
|||	    Deletes a compression engine previously created by
|||	    CreateCompressor(). This flushes any data left to be output
|||	    by the compressor, and generally cleans things up.
|||
|||	  Arguments
|||
|||	    comp                        An active compression handle, as
|||	                                obtained from CreateCompressor(). Once
|||	                                this call is made, the compression
|||	                                handle becomes invalid and can no
|||	                                longer be used.
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative error code. Possible
|||	    error codes include:
|||
|||	    COMP_ERR_BADPTR             An invalid compression handle was
|||	                                supplied.
|||
|||	  Implementation
|||
|||	    Folio call implemented in compression folio V24.
|||
|||	  Associated Files
|||
|||	    compression.h
|||
|||	  See Also
|||
|||	    CreateCompressor()
|||
**/

/**
|||	AUTODOC PUBLIC spg/compression/feedcompressor
|||	FeedCompressor - Give data to a compression engine
|||
|||	  Synopsis
|||
|||	    Err FeedCompressor(Compressor *comp, void *data,
|||	                       uint32 numDataWords);
|||
|||	  Description
|||
|||	    Give data to a compressor engine to have it compress it.
|||	    As data is compressed, the call back function supplied when the
|||	    compressor was created gets called for every word of
|||	    compressed data generated.
|||
|||	  Arguments
|||
|||	    comp                        An active compression handle, as
|||	                                obtained from CreateCompressor().
|||
|||	    data                        A pointer to the data to compress.
|||
|||	    numDataWords                The number of words of data being
|||	                                given to the compressor.
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative error code. Possible
|||	    error codes include:
|||
|||	    COMP_ERR_BADPTR            An invalid compression handle was
|||	                               supplied.
|||
|||	  Implementation
|||
|||	    Folio call implemented in compression folio V24.
|||
|||	  Associated Files
|||
|||	    compression.h
|||
|||	  See Also
|||
|||	    CreateCompressor(), DeleteCompressor()
|||
**/

/**
|||	AUTODOC PUBLIC spg/compression/getcompressorworkbuffersize
|||	GetCompressorWorkBufferSize - Get the size of the work buffer
|||	                              needed by a compression engine.
|||
|||	  Synopsis
|||
|||	    int32 GetCompressorWorkBufferSize(const TagArg *tags);
|||
|||	    int32 GetCompressorWorkBufferSizeVA(uint32 tags, ...);
|||
|||	  Description
|||
|||	    Return the size of the work buffer needed by a compression
|||	    engine. You can then allocate a buffer of that size and supply
|||	    the pointer with the COMP_TAG_WORKBUFFER tag when creating
|||	    a compression engine. If the COMP_TAG_WORKBUFFER tag is not
|||	    supplied when creating a compressor, then the folio automatically
|||	    allocates the memory needed for the compression engine.
|||
|||	  Arguments
|||
|||	    tags                        A pointer to an array of tag arguments
|||	                                containing extra data for this
|||	                                function. This must currently always
|||	                                be NULL.
|||
|||	  Return Value
|||
|||	    A positive value indicates the size of the work buffer needed in
|||	    bytes, while a negative value indicates an error. Possible
|||	    error codes currently include:
|||
|||	    COMP_ERR_BADTAG            An unknown tag was supplied.
|||
|||	  Implementation
|||
|||	    Folio call implemented in compression folio V24.
|||
|||	  Associated Files
|||
|||	    compression.h
|||
|||	  See Also
|||
|||	    CreateCompressor(), DeleteCompressor()
|||
**/

/**
|||	AUTODOC PUBLIC spg/compression/createdecompressor
|||	CreateDecompressor - Create a decompression engine
|||
|||	  Synopsis
|||
|||	    Err CreateDecompressor(Decompressor **comp, CompFunc cf,
|||	                           const TagArg *tags);
|||
|||	    Err CreateDecompressorVA(Decompressor **comp, CompFunc cf,
|||	                             uint32 tags, ...);
|||
|||	  Description
|||
|||	    Creates a decompression engine. Once the engine is created, you
|||	    can call FeedDecompressor() to have the engine decompress the
|||	    data you supply.
|||
|||	  Arguments
|||
|||	    decomp                      A pointer to a Decompressor variable,
|||	                                where a handle to the decompression
|||	                                engine can be put.
|||
|||	    cf                          A data output function. Every word of
|||	                                decompressed data is sent to this
|||	                                function. This function is called with
|||	                                two parameters: one is a user-data
|||	                                value as supplied with the
|||	                                COMP_TAG_USERDATA tag. The other is the
|||	                                word of decompressed data being output
|||	                                by the decompressor.
|||
|||	    tags                        A pointer to an array of tag arguments
|||	                                containing extra data for this
|||	                                function. See below for a description
|||	                                of the tags supported.
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function. The array must be terminated with TAG_END.
|||
|||	    COMP_TAG_WORKBUFFER (void *)
|||	                                A pointer to a work buffer. This buffer
|||	                                is used by the decompressor to store
|||	                                state information. If this tag is not
|||	                                supplied, the buffer is allocated and
|||	                                freed by the folio automatically. To
|||	                                obtain the required size for the
|||	                                buffer, call the
|||	                                GetDecompressorWorkBufferSize()
|||	                                function. The buffer you supply must
|||	                                be aligned on a 4-byte boundary, and
|||	                                must remain valid until
|||	                                DeleteDecompressor() is called.
|||	                                When you supply a work buffer, this
|||	                                routine allocates no memory of its own.
|||
|||	    COMP_TAG_USERDATA (void *)
|||	                                A value that the decompressor will pass
|||	                                to 'cf' when it is called. This value
|||	                                can be anything you want. For example,
|||	                                it can be a pointer to a private data
|||	                                structure containing some context such
|||	                                as a file handle. If this tag is
|||	                                not supplied, then NULL is passed
|||	                                to 'cf' when it is called.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code if the
|||	    decompression engine could not be created. Possible error codes
|||	    include:
|||
|||	    COMP_ERR_BADPTR             An invalid output function pointer,
|||	                                or work buffer was supplied.
|||
|||	    COMP_ERR_BADTAG             An unknown tag was supplied.
|||
|||	    COMP_ERR_NOMEM              There was not enough memory to
|||	                                initialize the compressor.
|||
|||	  Implementation
|||
|||	    Folio call implemented in compression folio V24.
|||
|||	  Associated Files
|||
|||	    compression.h
|||
|||	  See Also
|||
|||	    FeedDecompressor(), DeleteDecompressor(),
|||	    GetDecompressorWorkBufferSize(), CreateCompressor()
|||
**/

/**
|||	AUTODOC PUBLIC spg/compression/deletedecompressor
|||	DeleteCompressor - Delete a decompression engine
|||
|||	  Synopsis
|||
|||	    Err DeleteDecompressor(Decompressor *decomp);
|||
|||	  Description
|||
|||	    Deletes a decompression engine previously created by
|||	    CreateDecompressor(). This flushes any data left to be output
|||	    by the decompressor, and generally cleans things up.
|||
|||	  Arguments
|||
|||	    decomp                      An active decompression handle, as
|||	                                obtained from CreateDecompressor().
|||	                                Once this call is made, the
|||	                                decompression handle becomes invalid
|||	                                and can no longer be used.
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative error code. Current possible
|||	    error codes are:
|||
|||	    COMP_ERR_BADPTR               An invalid decompression handle was
|||	                                  supplied.
|||
|||	    COMP_ERR_DATAREMAINS          The decompressor thinks it is done,
|||	                                  but there remains extra data in its
|||	                                  buffers. This happens when the
|||	                                  compressed data is somehow corrupt.
|||
|||	    COMP_ERR_DATAMISSING          The decompressor thinks that not all
|||	                                  of the compressed data was given to
|||                                       be decompressed. This happens when
|||	                                  the compressed data is somehow
|||	                                  corrupt.
|||
|||	  Implementation
|||
|||	    Folio call implemented in compression folio V24.
|||
|||	  Associated Files
|||
|||	    compression.h
|||
|||	  See Also
|||
|||	    CreateCompressor()
|||
**/

/**
|||	AUTODOC PUBLIC spg/compression/feeddecompressor
|||	FeedDecompressor - Give data to the decompression engine
|||
|||	  Synopsis
|||
|||	    Err FeedDecompressor(Decompressor *decomp, void *data,
|||	                         uint32 numDataWords);
|||
|||	  Description
|||
|||	    Give data to the decompressor engine to have it decompress it.
|||	    As data is decompressed, the call back function supplied when the
|||	    decompressor was created gets called for every word of
|||	    decompressed data generated.
|||
|||	  Arguments
|||
|||	    decomp                      An active decompression handle, as
|||	                                obtained from CreateDecompressor().
|||
|||	    data                        A pointer to the data to decompress.
|||
|||	    numDataWords                The number of words of compressed data
|||	                                being given to the decompressor.
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative error code. Possible
|||	    error codes include:
|||
|||	    COMP_ERR_BADPTR             An invalid decompression handle was
|||	                                supplied.
|||
|||	  Implementation
|||
|||	    Folio call implemented in compression folio V24.
|||
|||	  Associated Files
|||
|||	    compression.h
|||
|||	  See Also
|||
|||	    CreateDecompressor(), DeleteDecompressor()
|||
**/

/**
|||	AUTODOC PUBLIC spg/compression/getdecompressorworkbuffersize
|||	GetDecompressorWorkBufferSize - Get the size of the work buffer
|||	                                needed by a decompression engine.
|||
|||	  Synopsis
|||
|||	    int32 GetDecompressorWorkBufferSize(const TagArg *tags);
|||
|||	    int32 GetDecompressorWorkBufferSizeVA(uint32 tags, ...);
|||
|||	  Description
|||
|||	    Return the size of the work buffer needed by a decompression
|||	    engine. You can then allocate a buffer of that size and supply
|||	    the pointer with the COMP_TAG_WORKBUFFER tag when creating
|||	    a decompression engine. If the COMP_TAG_WORKBUFFER tag is not
|||	    supplied when creating a decompressor, then the folio automatically
|||	    allocates the memory needed for the decompression engine.
|||
|||	  Arguments
|||
|||	    tags                        A pointer to an array of tag arguments
|||	                                containing extra data for this
|||	                                function. This must currently always
|||	                                be NULL.
|||
|||	  Return Value
|||
|||	    A positive value indicates the size of the work buffer needed in
|||	    bytes, while a negative value indicates an error. Possible
|||	    error codes currently include:
|||
|||	    COMP_ERR_BADTAG            An unknown tag was supplied.
|||
|||	  Implementation
|||
|||	    Folio call implemented in compression folio V24.
|||
|||	  Associated Files
|||
|||	    compression.h
|||
|||	  See Also
|||
|||	    CreateDecompressor(), DeleteDecompressor()
|||
**/

/**
|||	AUTODOC PUBLIC spg/compression/simplecompress
|||	SimpleCompress - Compress some data in memory
|||
|||	  Synopsis
|||
|||	    Err SimpleCompress(void *source, uint32 sourceWords,
|||	                       void *result, uint32 resultWords);
|||
|||	  Description
|||
|||	    Compress a chunk of memory to a different chunk of
|||	    memory.
|||
|||	  Arguments
|||
|||	    source                      A pointer to memory containing
|||	                                the data to be compressed. This
|||	                                pointer must be on a 4-byte boundary.
|||
|||	    sourceWords                 The number of words of data to
|||	                                compress.
|||
|||	    result                      A pointer to where the compressed
|||	                                data is to be deposited.
|||
|||	    resultWords                 The number of words available in the
|||	                                result buffer. If the compressed data
|||	                                is larger than this size, an
|||	                                overflow will be reported.
|||
|||	  Return Value
|||
|||	    If positive, then indicates the number of words copied to the
|||	    result buffer. If negative, then an error code. Possible error
|||	    codes include:
|||
|||	    COMP_ERR_NOMEM              There was not enough memory to
|||	                                initialize the compressor.
|||
|||	    COMP_ERR_OVERFLOW           There was not enough room in the
|||	                                result buffer to hold all of the
|||	                                compressed data.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in compression.lib V24.
|||
|||	  Associated Files
|||
|||	    compression.h
|||
|||	  See Also
|||
|||	    SimpleDecompress()
**/

/**
|||	AUTODOC PUBLIC spg/compression/simpledecompress
|||	simpleDecompress - Decompress some data in memory
|||
|||	  Synopsis
|||
|||	    Err SimpleDecompress(void *source, uint32 sourceWords,
|||	                         void *result, uint32 resultWords);
|||
|||	  Description
|||
|||	    Decompress a chunk of memory to a different chunk of
|||	    memory.
|||
|||	  Arguments
|||
|||	    source                      A pointer to memory containing
|||	                                the data to be decompressed. This
|||	                                pointer must be on a 4-byte boundary.
|||
|||	    sourceWords                 The number of words of data to
|||	                                decompress.
|||
|||	    result                      A pointer to where the decompressed
|||	                                data is to be deposited.
|||
|||	    resultWords                 The number of words available in the
|||	                                result buffer. If the decompressed data
|||	                                is larger than this size, an
|||	                                overflow will be reported.
|||
|||	  Return Value
|||
|||	    If positive, then indicates the number of words copied to the
|||	    result buffer. If negative, then an error code. Possible error
|||	    codes include:
|||
|||	    COMP_ERR_NOMEM              There was not enough memory to
|||	                                initialize the decompressor.
|||
|||	    COMP_ERR_OVERFLOW           There was not enough room in the
|||	                                result buffer to hold all of the
|||	                                decompressed data.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in compression.lib V24.
|||
|||	  Associated Files
|||
|||	    compression.h
|||
|||	  See Also
|||
|||	    SimpleCompress()
**/

/* keep the compiler happy... */
extern int foo;
