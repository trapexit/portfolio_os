/* $Id: autodocs.c,v 1.4 1994/10/20 19:03:32 vertex Exp $ */

/**
|||	AUTODOC PUBLIC spg/file/changedirectory
|||	ChangeDirectory - Changes the current directory.
|||
|||	  Synopsis
|||
|||	    Item ChangeDirectory( char *path )
|||
|||	  Description
|||
|||	    Changes the current task's working directory to the absolute or
|||	    relative location specified by the path, and returns the item number for
|||	    the directory.
|||
|||	  Arguments
|||
|||	    path                         An absolute or relative pathname for
|||	                                 the new current directory.
|||
|||	  Return Value
|||
|||	    The function returns the item number of the new directory or a
|||	    negative error code if an error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
|||	  See Also
|||
|||	    GetDirectory()
|||
**/

/**
|||	AUTODOC PUBLIC spg/file/closedirectory
|||	CloseDirectory - Closes a directory.
|||
|||	  Synopsis
|||
|||	    void CloseDirectory( Directory *dir )
|||
|||	  Description
|||
|||	    This function closes a directory that was previously opened using
|||	    OpenDirectoryItem() or OpenDirectoryPath(). All resources get
|||	    released.
|||
|||	  Arguments
|||
|||	    dir                          A pointer to the Directory structure
|||	                                 for the directory to close.
|||
|||	  Implementation
|||
|||	    Folio call implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    directoryfunctions.h, filesystem.lib
|||
|||	  See Also
|||
|||	    OpenDirectoryItem(), OpenDirectoryPath(), ReadDirectory()
|||
**/

/**
|||	AUTODOC PUBLIC spg/file/closediskfile
|||	CloseDiskFile - Closes a file.
|||
|||	  Synopsis
|||
|||	    int32 CloseDiskFile( Item fileItem )
|||
|||	  Description
|||
|||	    Closes a disk file that was opened with a call to OpenDiskFile() or
|||	    OpenDiskFileInDir(). The specified item may not be used after
|||	    successful completion of this call.
|||
|||	  Arguments
|||
|||	    fileItem                     The item number of the disk file to
|||	                                 close.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or a negative error code
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
|||	  See Also
|||
|||	    OpenDiskFile(), OpenDiskFileInDir()
|||
**/

/**
|||	AUTODOC PUBLIC spg/file/closediskstream
|||	CloseDiskStream - Closes a disk stream.
|||
|||	  Synopsis
|||
|||	    void CloseDiskStream( Stream *theStream )
|||
|||	  Description
|||
|||	    This function closes a disk stream that was opened with a call to
|||	    OpenDiskStream().  It closes the stream's file, deallocates the
|||	    buffer memory, and releases the stream structure.
|||
|||	  Arguments
|||
|||	    theStream                    A pointer to the Stream structure for
|||	                                 an open file.
|||
|||	  Implementation
|||
|||	    Folio call implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filestreamfunctions.h, filesystem.lib
|||
|||	  See Also
|||
|||	    OpenDiskStream(), ReadDiskStream(), SeekDiskStream()
|||
**/

/**
|||	AUTODOC PUBLIC spg/file/createalias
|||	CreateAlias - Creates an alias for a file.
|||
|||	  Synopsis
|||
|||	    Item CreateAlias( char *aliasPath, char *realPath )
|||
|||	  Description
|||
|||	    This function creates an alias for a file.  The alias can be used in
|||	    place of the full pathname for the file.  Note that the file system
|||	    maintins separate alias entries for each task.
|||
|||	  Arguments
|||
|||	    aliasPath                    The alias pathname for the file.
|||
|||	    realPath                     The actual pathname for the file.
|||
|||	  Return Value
|||
|||	    The function returns the item number for the file alias that is
|||	    created, or a negative error code if an error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
|||	  See Also
|||
|||	    OpenDiskFile(), OpenDiskFileInDir()
|||
**/

/**
|||	AUTODOC PUBLIC spg/file/createfile
|||	CreateFile - Creates a file.
|||
|||	  Synopsis
|||
|||	    Item CreateFile( char *path )
|||
|||	  Description
|||
|||	    This function creates a new file.
|||
|||	  Arguments
|||
|||	    path                         The pathname for the file.
|||
|||	  Return Value
|||
|||	    The function returns a positive value if the file is created successfully
|||	    or a negative error code if an error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
|||	  See Also
|||
|||	    DeleteFile(), OpenDiskFile(), OpenDiskFileInDir()
|||
**/

/**
|||	AUTODOC PUBLIC spg/file/deletefile
|||	DeleteFile - Deletes a file.
|||
|||	  Synopsis
|||
|||	    Err DeleteFile( char *path )
|||
|||	  Description
|||
|||	    This function deletes a file.
|||
|||	  Arguments
|||
|||	    path                         The pathname for the file to delete.
|||
|||	  Return Value
|||
|||	    The function returns zero if the file was successfully deleted or
|||	    a negative error code if an error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
|||	  See Also
|||
|||	    CreateFile(), OpenDiskFile(), OpenDiskFileInDir(), CloseDiskFile()
|||
**/

/**
|||	AUTODOC PRIVATE spg/file/dismountfilesystem
|||	DismountFileSystem - Dismounts a file system.
|||
|||	  Synopsis
|||
|||	    Err DismountFileSystem( char *name )
|||
|||	  Description
|||
|||	    This function dismounts a file system.  It can only be executed by
|||	    privledged tasks.
|||
|||	  Arguments
|||
|||	    name                         The name of the file system to dismount.
|||
|||	  Return Value
|||
|||	    The function returns zero if successful or a negative error code
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
|||	  See Also
|||
|||	    MountFileSystem()
|||
**/

/**
|||	AUTODOC PUBLIC spg/file/getdirectory
|||	GetDirectory - Gets the item number and pathname for the current
|||	               directory.
|||
|||	  Synopsis
|||
|||	    Item GetDirectory( char *pathBuf, int32 pathBufLen )
|||
|||	  Description
|||
|||	    This function returns the item number of the calling task's current
|||	    directory.  If pathBuf is non-NULL, it must point to a buffer of writable
|||	    memory whose length is given in pathBufLen;  the absolute pathname of the
|||	    current working directory is stored into this buffer.
|||
|||	  Arguments
|||
|||	    pathBuf                      A pointer to a buffer in which to receive
|||	                                 the absolute pathname for the current
|||	                                 directory.  If you do not want to get the
|||	                                 pathname string, use NULL as the value of
|||	                                 this argument.
|||
|||	    pathBufLen                   The size of the buffer pointed to by the
|||	                                 pathBuf argument, in bytes, or zero if you
|||	                                 don't provide a buffer.
|||
|||	  Return Value
|||
|||	    The function returns the item number of the current directory.
|||
|||	  Implementation
|||
|||	    SWI implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
**/

/**
|||	AUTODOC PRIVATE spg/file/mountfilesystem
|||	MountFileSystem - Mounts a file system.
|||
|||	  Synopsis
|||
|||	    Item MountFileSystem( Item deviceItem, int32 unit,
|||	                          uint32 blockOffset )
|||
|||	  Description
|||
|||	    This function mounts a file system.  It can only be executed by
|||	    privileged tasks.
|||
|||	  Arguments
|||
|||	    deviceItem                   The item number of the raw disk device on
|||	                                 which to mount the file system.
|||
|||	    unit                         The number of the unit on which to mount the
|||	                                 file system.
|||
|||	    blockOffset                  The offset from the beginning of the unit to
|||	                                 the first block of the file system, in
|||	                                 bytes.
|||
|||	  Return Value
|||
|||	    The function returns a positive value if the operation was successful or
|||	    a negative error code if an error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filesystem.h, filefunctions.h
|||
|||	  See Also
|||
|||	    DismountFileSystem(), MountMacFileSystem()
|||
**/

/**
|||	AUTODOC PRIVATE spg/file/mountmacfilesystem
|||	MountMacFileSystem - Mounts a file system located on a Macintosh host computer.
|||
|||	  Synopsis
|||
|||	    Item MountMacFileSystem( char *path )
|||
|||	  Description
|||
|||	    This function mounts a file system on a Macintosh host computer.  When
|||	    mounted, the file system is identical to the file system on a 3DO system,
|||	    and you use the same file-system functions to gain access to it.
|||
|||	    This function is supported only on development systems, and is not
|||	    implemented in field systems.  It can only be executed by privileged
|||	    tasks.
|||
|||	  Arguments
|||
|||	    path                         A relative pathname (a null-terminated text
|||	                                 string) that specifies the Macintosh
|||	                                 directory to mount.  The pathname is
|||	                                 relative to the root directory of the
|||	                                 Macintosh Portfolio Driver.
|||
|||	  Return Value
|||
|||	    The function returns the item number of the new file system or a
|||	    negative error code if an error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
|||	  See Also
|||
|||	    MountFileSystem()
|||
**/

/**
|||	AUTODOC PUBLIC spg/file/opendirectoryitem
|||	OpenDirectoryItem - Opens a directory specified by an Item.
|||
|||	  Synopsis
|||
|||	    Directory *OpenDirectoryItem( Item openFileItem )
|||
|||	  Description
|||
|||	    This function opens a directory.  It allocates a new Directory
|||	    structure, opens the directory, and prepares for a traversal of
|||	    the contents of the directory.  Unlike OpenDirectoryPath(), you
|||	    specify the file for the directory by its item number rather than
|||	    by its pathname.
|||
|||	  Arguments
|||
|||	    openFileItem                 The item number of the open file to
|||	                                 use for the directory. When you
|||	                                 later call CloseDirectory(), this
|||	                                 file item will automatically be
|||	                                 deleted for you, you do not need to
|||	                                 call CloseDiskFile().
|||
|||	  Return Value
|||
|||	    The function returns a pointer to the Directory structure that is
|||	    created or NULL if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in file folio V20.
|||
|||	  Caveats
|||
|||	    When you are done scanning the directory and call
|||	    CloseDirectory(), the item you gave to OpenDirectoryItem()
|||	    will automatically be closed for you. In essense, when you
|||	    call OpenDirectoryItem(), you are giving away the File item
|||	    to the file folio, which will dispose of it when the directory
|||	    is closed.
|||
|||	  Associated Files
|||
|||	    directoryfunctions.h, filesystem.lib
|||
|||	  See Also
|||
|||	    OpenDiskFile(), OpenDiskFileInDir(), OpenDirectoryPath(),
|||	    CloseDirectory()
|||
**/

/**
|||	AUTODOC PUBLIC spg/file/opendirectorypath
|||	OpenDirectoryPath - Opens a directory specified by a pathname.
|||
|||	  Synopsis
|||
|||	    Directory *OpenDirectoryPath( char *thePath )
|||
|||	  Description
|||
|||	    This function opens a directory.  It allocates a new Directory
|||	    structure, opens the directory, and prepares for a traversal of the
|||	    contents of the directory.  Unlike OpenDirectoryItem(), you specify
|||	    the file for the directory by its pathname rather than by its item
|||	    number.
|||
|||	  Arguments
|||
|||	    thePath                      An absolute or relative pathname (a null-
|||	                                 terminated text string) for the file to
|||	                                 use for the directory, or an alias for the
|||	                                 pathname.
|||
|||	  Return Value
|||
|||	    The function returns a pointer to a Directory structure that is created
|||	    or NULL if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    directoryfunctions.h, filesystem.lib
|||
|||	  See Also
|||
|||	    OpenDiskFile(), OpenDiskFileInDir(), OpenDirectoryItem(),
|||	    CloseDirectory()
|||
**/

/**
|||	AUTODOC PUBLIC spg/file/opendiskfile
|||	OpenDiskFile - Opens a disk file.
|||
|||	  Synopsis
|||
|||	    Item OpenDiskFile( char *path )
|||
|||	  Description
|||
|||	    This function opens a disk file, given an absolute or relative
|||	    pathname, and returns its Item number.
|||
|||	  Arguments
|||
|||	    path                         An absolute or relative pathname (a null-
|||	                                 terminated text string) for the file to
|||	                                 open, or an alias for the pathname.
|||
|||	  Return Value
|||
|||	    The function returns the item number of the opened file (which can be
|||	    used later to refer to the file), or a negative error code if an
|||	    error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
|||	  See Also
|||
|||	    CloseDiskFile(), OpenDiskFileInDir(), OpenDiskStream()
|||
**/

/**
|||	AUTODOC PUBLIC spg/file/opendiskfileindir
|||	OpenDiskFileInDir - Opens a disk file contained in a specific directory.
|||
|||	  Synopsis
|||
|||	    Item OpenDiskFileInDir( Item dirItem, char *path )
|||
|||	  Description
|||
|||	    Similar to OpenDiskFile(), this function allows the caller to specify
|||	    the Item of a directory that should serve as a starting location for
|||	    the file search.
|||
|||	  Arguments
|||
|||	    dirItem                      The item number of the directory
|||	                                 containing the file.
|||
|||	    path                         A pathname for the file that is relative
|||	                                 to the directory specified by the dirItem
|||	                                 argument.
|||
|||	  Return Value
|||
|||	    The function returns the item number of the opened file (which can be
|||	    used later to refer to the file), or a negative error code if an
|||	    error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
|||	  See Also
|||
|||	    OpenDiskFile(), OpenDirectoryItem(), OpenDirectoryPath(),
|||	    OpenDiskStream(), CloseDiskFile()
|||
**/

/**
|||	AUTODOC PUBLIC spg/file/opendiskstream
|||	OpenDiskStream - Opens a disk stream for stream-oriented I/O.
|||
|||	  Synopsis
|||
|||	    Stream *OpenDiskStream( char *theName, int32 bSize )
|||
|||	  Description
|||
|||	    This routine allocates a new Stream structure, opens the disk file
|||	    identified by an absolute or relative pathname, allocates the specified
|||	    amount of buffer space, and initiates an asynchronous read to fill the
|||	    buffer with the first portion of the file's data.  It returns NULL if
|||	    any of these functions cannot be performed for any reason.
|||
|||	    The bSize field specifies the amount of buffer space to be allocated to
|||	    the file stream.  It may be positive (giving the number of bytes to be
|||	    allocated), zero (indicating that a default allocation of two blocks
|||	    should be used), or negative (giving the two's complement of the
|||	    number of blocks worth of memory that should be allocated).
|||
|||	  Arguments
|||
|||	    theName                      An absolute or relative pathname (a null-
|||	                                 terminated text string) for the file to
|||	                                 open.
|||
|||	    bSize                        The size of the buffer to allocate for the
|||	                                 stream.  This can be (1) a positive value
|||	                                 that specifies the size of the buffer to
|||	                                 allocate, in bytes, (2) zero, which
|||	                                 specifies to allocate the default buffer
|||	                                 (currently two blocks), or (3) a negative
|||	                                 value , the two's complement of which
|||	                                 specifies the number of blocks of memory to
|||	                                 allocate for the buffer.
|||
|||	  Return Value
|||
|||	    The function returns a pointer to the Stream structure for the opened
|||	    file or NULL if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filestreamfunctions.h
|||
|||	  See Also
|||
|||	    CloseDiskStream(), ReadDiskStream(), SeekDiskStream(),
|||	    OpenDiskFile()
|||
**/

/**
|||	AUTODOC PUBLIC spg/file/readdirectory
|||	ReadDirectory - Reads the next entry from a directory.
|||
|||	  Synopsis
|||
|||	    int32 ReadDirectory( Directory *dir, DirectoryEntry *de )
|||
|||	  Description
|||
|||	    This routine reads the next entry from the specified directory. It
|||	    stores the information from the directory entry into the supplied
|||	    DirectoryEntry structure. You can then examine the DirectoryEntry
|||	    structure for information about the entry.
|||
|||	    The most interest fields in the DirectoryEntry structure are:
|||
|||	      de_Flags
|||	      This contains a series of bit flags that describe characteristics
|||	      of the entry. Flags of interest are FILE_IS_DIRECTORY which
|||	      indicates the entry is a nested directory and FILE_IS_READONLY
|||	      which tells you the file cannot be written to.
|||
|||	      de_Type
|||	      This is currently one of FILE_TYPE_DIRECTORY, FILE_TYPE_LABEL, or
|||	      FILE_TYPE_CATAPULT.
|||
|||	      de_BlockSize
|||	      This is the size in bytes of the blocks when reading this entry.
|||
|||	      de_ByteCount
|||	      The logical count of the number of useful bytes within the blocks
|||	      allocated for this file.
|||
|||	      de_BlockCount
|||	      The number of blocks allocated for this file.
|||
|||	    You can use OpenDirectoryPath() and ReadDirectory() to scan
|||	    the list of mounted file systems. This is done by supplying a
|||	    path of "/" to OpenDirectoryPath(). The entries that
|||	    ReadDirectory() will return will then correspond to all of the
|||	    mounted file systems. You can then look at the de_Flags field
|||	    to determine if a file system is readable or not.
|||
|||	  Arguments
|||
|||	    dir                          A pointer to the Directory structure for the
|||	                                 directory.
|||
|||	    de                           A pointer to a DirectoryEntry structure in
|||	                                 which to receive the information about the
|||	                                 next directory entry.
|||
|||	  Return Value
|||
|||	    This function returns zero if successful or a negative error code
|||	    if an error (such as end-of-directory) occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    directoryfunctions.h, filesystem.lib
|||
|||	  See Also
|||
|||	    OpenDirectoryItem(), OpenDirectoryPath()
|||
**/

/**
|||	AUTODOC PUBLIC spg/file/readdiskstream
|||	ReadDiskStream - Reads from a disk stream.
|||
|||	  Synopsis
|||
|||	    int32 ReadDiskStream( Stream *theStream, char *buffer,
|||	                          int32 nBytes )
|||
|||	  Description
|||
|||	    This routine transfers the specified number of bytes (or, as many as are
|||	    left before the end-of-file) from the stream (beginning at the stream'
|||	    current position) to the specified buffer.  It advances the stream's
|||	    position past the bytes that have been transferred, and returns the actual
|||	    number of bytes transferred.  It will initiate an asynchronous read to
|||	    read additional data into the stream's internal buffer, if possible
|||	    and appropriate.
|||
|||	  Arguments
|||
|||	    theStream                    A pointer to the Stream structure from which
|||	                                 to read.
|||
|||	    buffer                       A pointer to the buffer into which to read
|||	                                 the data.
|||
|||	    nBytes                       The number of bytes to read.
|||
|||	  Return Value
|||
|||	    The function returns the actual number of bytes read or a
|||	    negative error code if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filestreamfunctions.h, filesystem.lib
|||
|||	  See Also
|||
|||	    CloseDiskStream(), OpenDiskStream(), SeekDiskStream()
|||
**/

/**
|||	AUTODOC PUBLIC spg/file/seekdiskstream
|||	SeekDiskStream - Performs a seek operation on a disk stream.
|||
|||	  Synopsis
|||
|||	    int32 SeekDiskStream( Stream *theStream, int32 offset,
|||	                          enum SeekOrigin whence )
|||
|||	  Description
|||
|||	    This function does a seek operation on a stream file, thereby changing
|||	    the I/O byte position within the file.  After calling this function, data
|||	    transfers start with the first byte at the new file position.  The whence
|||	    argument specifies whether the operation is to be relative to either the
|||	    beginning or end of the file or to the current position in the file.  The
|||	    offset argument specifies the number of bytes of offset relative to the
|||	    whence position.  The result is the actual absolute file position that
|||	    results from the seek or an error code if anything went wrong.
|||
|||	    The offset is specified in any of three ways: absolute (positive) byte
|||	    offset from the beginning of file (SEEK_SET), relative byte offset from
|||	    the current position in the file (SEEK_CUR), or absolute (negative) byte
|||	    offset from the end of the file (SEEK_END).
|||
|||	    This function doesn't actually perform the seek, or do any I/O, or
|||	    invalidate the data in the stream, or abort a read-ahead-in-progress.  It
|||	    simply stores the desired offset and a "please seek"  request into
|||	    the stream structure.  The actual seeking, if any, occurs on the next call
|||	    to ReadDiskStream().
|||
|||	    Seeks are very efficient if they are backward seeks that don't take
|||	    you outside of the current block, or forward seeks that don't take you
|||	    outside of the amount of data actually read ahead into the buffer.  Short
|||	    seeks of this nature simply adjust the pointers, and then allow the read
|||	    to continue from the appropriate place in the buffer.
|||
|||	    Seeks that move you backward outside of the current block or forward past
|||	    the amount of data in the buffer, will force all of the data in the buffer
|||	    to be flushed.  Data from the sought after portion of the file will be
|||	    read into the buffer (synchronously) and the read will be completed. As a
|||	    result, you'll see some latency when you issue the ReadDiskStream
|||	    call.
|||
|||	    If you wish to do a seek-ahead that is, you wish to seek to a specified
|||	    location in the file but not necessarily read anything immediatelythe best
|||	    thing to do is a SeekDiskStream followed immediately by a zero-byte
|||	    ReadDiskStream.  This is a special- case that will start the read ahead
|||	    for the data you're seeking, and then return without waiting for the
|||	    I/O to complete.  When you issue a nonzero-length ReadDiskStream some time
|||	    later, the data will probably have been read in, and there will be no
|||	    delay in accessing it.
|||
|||	  Arguments
|||
|||	    theStream                    A pointer to an open Stream. "offset"
|||	                                 is a byte offset into the file, and whence
|||	                                 is SEEK_SET, SEEK_CUR, or SEEK_END.
|||
|||	    offset                       Specified in any of three ways:  absolute
|||	                                 (positive) byte offset from the beginning of
|||	                                 file (SEEK_SET);relative byte offset from
|||	                                 the current position in the file (SEEK_CUR);
|||	                                 or absolute (negative) byte offset from the
|||	                                 end of the file (SEEK_END).
|||
|||	    whence                       The anchor point (either the beginning of
|||	                                 the file, the current position, or the end
|||	                                 of the file) to which the offset should be
|||	                                 applied to create the new file position.
|||
|||	  Return Value
|||
|||	    The function returns the actual (absolute) offset to which the seek
|||	    occurs or a negative error code if (1) the offset is outside of
|||	    the legal range, (2) the whence field contains an illegal value, or (3)
|||	    there was any other problem.
|||
|||	  Implementation
|||
|||	    Folio call implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filestreamfunctions.h, filesystem.lib
|||
|||	  Notes
|||
|||	    Seeks that move backward outside of the current block or forward past the
|||	    amount of data in the buffer flush all of the data from the buffer.  Data
|||	    from the sought-after portion of the file is read synchronously into the
|||	    buffer and the read operation is then completed.  As a result, you'll
|||	    see some latency when you call ReadDiskStream().
|||
|||	    To perform a seek-ahead (seeking to an actual physical location in the
|||	    file but not necessarily read anything immediately), call SeekDiskStream()
|||	    followed immediately by a zero-byte ReadDiskStream().  This is a special
|||	    case that starts the read-ahead for the data you're seeking.  You then
|||	    return without waiting for the I/O to complete.  Later, you can call
|||	    ReadDiskStream() with a nonzero read-length value, and if the data has
|||	    been read in, you have immediate access to it.
|||
|||	  See Also
|||
|||	    CloseDiskStream(), OpenDiskStream(), ReadDiskStream()
**/

/* keep the compiler happy... */
extern int foo;
