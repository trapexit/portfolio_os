/*****

$Id: LoadFile.c,v 1.40 1994/09/20 19:02:40 vertex Exp $

$Log: LoadFile.c,v $
 * Revision 1.40  1994/09/20  19:02:40  vertex
 * Clarified the issue of priorities in the LoadProgram() and LoadProgramPrio()
 * autodocs
 *
 * Revision 1.39  1994/09/14  22:46:58  vertex
 * No longer checks io_Error in the IO request itself, since the new
 * DoIO() does that automagically.
 *
 * Revision 1.38  1994/09/13  16:48:42  vertex
 * Tweak to the UnloadCode() autodoc
 *
 * Revision 1.37  1994/09/10  02:31:31  vertex
 * Updated autodoc headers per Yvonne's request.
 *
 * Revision 1.36  1994/09/08  23:17:33  vertex
 * Updated for new release numbering scheme.
 *
 * Revision 1.35  1994/08/26  19:07:09  vertex
 * When a task fails to be created, we were calling ControlMem() to
 * return pages to the system. This was incorrect. Instead, FreeMem()
 * the memory, and call ScavengeMem().
 *
 * Revision 1.34  1994/08/17  17:11:23  vertex
 * ExecuteAsThread() now fetches the version and revision from
 * the AIF and passes that along to CreateItem() so that the
 * created thread will have a version and revision consistent
 * with what the AIF says. This will give the eventbroker a
 * valid version number once again.
 *
 * Revision 1.33  1994/08/11  23:47:50  vertex
 * Per Dale's recommendation, we now call ScavengeMem() prior to
 * calling AllocMemBlocks()
 *
 * Revision 1.32  1994/08/11  20:50:59  vertex
 * Switched the NOMEM error codes to use the correct MakeFErr()
 * macro instead.
 *
 * Revision 1.31  1994/04/29  16:40:04  vertex
 * More autodoc improvements
 *
 * Revision 1.30  1994/04/27  01:03:39  vertex
 * Autodoc clarification, hope it's good enough
 *
 * Revision 1.29  1994/04/22  20:16:25  vertex
 * Autodoc fix to mention subroutinestartup.o
 *
 * Revision 1.28  1994/04/20  18:52:22  vertex
 * Fixed mis-handling of files < sizeof(AIFHeader).
 *
 * Revision 1.27  1994/04/07  23:34:35  vertex
 * Put real release numbers in the autodocs
 *
 * Revision 1.26  1994/04/01  18:35:36  vertex
 * Removed another excess error message
 *
 * Revision 1.25  1994/03/31  02:16:09  vertex
 * Commented out over-zealous error reporting code
 *
 * Revision 1.24  1994/03/30  19:00:23  vertex
 * Small changes to keep the compiler from complaining pointlessly
 *
 * Revision 1.23  1994/03/30  18:52:46  vertex
 * Fixed bug with ExecuteAsThread() not setting the stack pointer correctly
 * when launching the thread, which would result in nasty memory
 * trashing of the caller's memory.
 *
 * Revision 1.22  1994/03/26  00:49:34  dplatt
 * Fix AllocMemBlocks problem.
 *
 * Revision 1.21  1994/03/25  23:04:16  dplatt
 * Jack up the radiator cap, and drive a new car in underneath.
 * Thanks much, Martin!
 *
 * Revision 1.20  1993/10/25  23:18:21  limes
 * Fix a compile warning.
 *
 * Revision 1.19  1993/09/13  19:19:23  dplatt
 * Turn off all printf calls when compiled in production mode.
 *
 * Revision 1.18  1993/08/09  22:08:58  dplatt
 * Don't set the task priority and stack size based on values from the
 * AIF header... these values are obsolete.
 *
 * Revision 1.17  1993/07/28  02:30:50  dplatt
 * Zero out IOReqs;  do delete-item cleanup
 *
 * Revision 1.16  1993/05/28  21:43:11  dplatt
 * Cardinal3 changes, get ready for Dragon
 *
 * Revision 1.15  1993/03/16  06:36:37  dplatt
 * Functional Freeze release
 *
 * Revision 1.14  1993/02/21  04:01:40  dplatt
 * Could not launch privileged tasks due to failure to mask the privilege
 * bit off of the stack-size word.
 *
 * Revision 1.13  1993/02/11  19:39:37  dplatt
 * Developer-release and new-kernel changes
 *
 * Revision 1.12  1993/01/05  21:06:58  dplatt
 * No, we don't want to debug all launches...
 *
 * Revision 1.11  1993/01/05  20:57:47  dplatt
 * CES changes redux
 *
 * Revision 1.10  1993/01/04  02:19:26  dplatt
 * CES Changes
 *
 * Revision 1.9  1992/12/22  09:38:50  dplatt
 * Fix CD-ROM aborts and timer trouble
 *
 * Revision 1.8  1992/12/22  07:58:10  dplatt
 * Magneto 3 changes and CD-ROM support
 *
 * Revision 1.7  1992/12/08  05:59:52  dplatt
 * Magenta changes
 *
 * Revision 1.6  1992/11/06  01:12:39  dplatt
 * Make LoadProgram() into a library routine
 *
 * Revision 1.3  1992/10/16  01:22:24  dplatt
 * First cut at bluebird release changes
 *
 * Revision 1.2  1992/10/01  23:36:21  dplatt
 * Switch to int32/uint32
 *
 * Revision 1.1  1992/09/11  00:42:29  dplatt
 * Initial revision
 *

 *****/

/*
  Copyright The 3DO Company Inc., 1991 - 1994.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  LoadFile.c - code to load the contents of a program into memory
*/

#undef SUPER


#include "types.h"
#include "device.h"
#include "io.h"
#include "mem.h"
#include "aif.h"
#include "string.h"
#include "filesystem.h"
#include "filefunctions.h"
#include "operror.h"
#include "stdio.h"
#include "debug.h"



/*****************************************************************************/


/* ASM stub */
extern int32 ExecuteAsSub(void *code, uint32 argc, char **argv);

#define PATHBUFSIZE 256

/* number of extra bytes at the beginning of the memory block used to hold
 * a thread's code
 */
#define THREAD_PAD  16


/*****************************************************************************/

#define BIGGULP

/*
   Set USE_AIF_VALS if the priority and stacksize fields in the AIF
   header are to be honored.  They shouldn't be, now that the 3DO header
   is being used.
*/

/* #define USE_AIF_VALS */

#undef DEBUG

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x) /* x */
#endif

#ifdef NOTDEF
extern int32 bcopy (void *s, void *d, int32 n);
#endif

static Err internalLoadCode(char *name, int32 priority, bool spawnTask,
                            CodeHandle *code)
{
  Item       fileItem;
  OpenFile  *file;
  Item       fileIOItem;
  IOReq     *fileIO;
  IOInfo     ioInfo;
  void      *program;
  int32      fileSize;
  int32      blockSize;
  int32      allocSize;
  int32      imageSize;
  int32      blocksInFile;
  int32      blocksToRead;
  void      *ioBuffer;
  int32      ioBufferSize;
  int32      numHeaderBlocks;
  bool       readPartialLastBlock;
  AIFHeader *header;
  Err        result;
  TagArg     tags[8];
  File      *programDirectory;
  char       pathBuffer[PATHBUFSIZE];
  char      *fileName;
  uint32     i;

    if (spawnTask)
    {
        /* if we're spawning a task, isolate the filename part of the
         * supplied command string, since the string may contain arguments
         * for the task
         */

        i = 0;
        while ((i < PATHBUFSIZE-1) && name[i] && (name[i] != ' '))
        {
            pathBuffer[i] = name[i];
            i++;
        }
        pathBuffer[i] = 0;
        fileName = pathBuffer;
    }
    else
    {
        /* when not spawning a task, the supplied name is just a filename
         * and can be used as is
         */

        fileName = name;
    }

    program   = NULL;
    imageSize = 0;
    allocSize = 0;
    fileSize  = 0;

    DBUG(("Loading %s\n", fileName));

    fileItem = OpenDiskFile(fileName);
    if (fileItem >= 0)
    {
        file = (OpenFile *) LookupItem(fileItem);

        fileIOItem = CreateIOReq(NULL,0,fileItem,0);
        if (fileIOItem >= 0)
        {
            fileIO          = (IOReq *) LookupItem(fileIOItem);
            blocksInFile    = (int32) file->ofi_File->fi_BlockCount;
            blockSize       = (int32) file->ofi_File->fi_BlockSize;
            fileSize        = (int32) file->ofi_File->fi_ByteCount;
            numHeaderBlocks = 1;

            if (fileSize >= sizeof(AIFHeader))
            {
                DBUG(("fi_BlockCount %ld, fi_BlockSize %ld, fi_FileSize %ld\n",blocksInFile,blockSize,fileSize));

                if (blockSize < sizeof(AIFHeader))
                    numHeaderBlocks = ((sizeof(AIFHeader) + blockSize - 1)) / blockSize;

                ioBufferSize = numHeaderBlocks * blockSize;

                ioBuffer = AllocMem(ioBufferSize, MEMTYPE_DMA);

                if (ioBuffer)
                {
                  DBUG(("Reading first block into buffer at 0x%X\n", ioBuffer));
                    /* Read the first block of the file */
                    memset(&ioInfo, 0, sizeof(ioInfo));
                    ioInfo.ioi_Command         = CMD_READ;
                    ioInfo.ioi_Recv.iob_Buffer = ioBuffer;
                    ioInfo.ioi_Recv.iob_Len    = ioBufferSize;

                    result = DoIO(fileIOItem, &ioInfo);
                    if (result >= 0)
                    {
                        header             = (AIFHeader *)ioBuffer;
                        ioInfo.ioi_Offset += numHeaderBlocks;

                        /* validate AIF header */
                        if ((fileIO->io_Actual >= sizeof(AIFHeader))
                         && (header->aif_SWIexit == 0xef000011)  /* SWI $11 */
                         && (header->aif_ImageROsize >= 128)
                         && (header->aif_ImageRWsize >= 0)
                         && (header->aif_ZeroInitSize >= 0))
                        {
                            /* Allocate sufficient memory to hold the program...
                             * It's the larger of the file size and the size
                             * indicated by the header.
                             *
                             * RSA signed files will be larger than what the
                             * header indicates. Non-RSA signed files should have a
                             * matching size.
                             */

                            imageSize = header->aif_ImageROsize +
                                        header->aif_ImageRWsize +
                                        (~15 & (header->aif_ZeroInitSize+15));

                            allocSize = imageSize;
                            if (allocSize < fileSize)
                                allocSize = fileSize;

                            if (spawnTask)
                            {
                                ScavengeMem();

                                /* for a task, allocate page-aligned */
                                program = AllocMemBlocks(allocSize, MEMTYPE_DMA | MEMTYPE_TASKMEM);
                            }
                            else
                            {
                                /* no need for page-aligned, but we do need an extra uint32
                                 * to hold the size of the allocation so UnloadCode()
                                 * can free the allocation. We want to keep the
                                 * loaded code on a 16-byte boundary, so add a total
                                 * of 16 bytes to the allocation.
                                 */
                                program = AllocMem(allocSize + THREAD_PAD, MEMTYPE_DMA);
                            }

                            if (program)
                            {
                                if (spawnTask)
                                {
                                    /* get the actual size of the allocation */
                                    allocSize = *((int32 *)program);
                                }
                                else
                                {
                                    /* Skip over the first 16 bytes of the allocation.
                                     * The size of the allocation is stored by AllocMem()
                                     * in the first uint32 of the memory. The rest is
                                     * unused padding.
                                     */
                                    program = (void *)((uint32)program + THREAD_PAD);
                                }

                                /* There are 3 cases to handle:
                                 *
                                 * 1. The program area is shorter than the IO buffer.
                                 *    Simply move the data from the IO buffer to the
                                 *    program area.
                                 *
                                 * 2. The program area is large enough to hold all blocks
                                 *    of the file. Move the chunk of data from the
                                 *    IO buffer to the program area, and read all
                                 *    remaining blocks of the file into the program
                                 *    area.
                                 *
                                 * 3. The program area cannot hold the entire last
                                 *    block of the file. Move the chunk of data from
                                 *    the IO buffer into the program area. Read
                                 *    the remaining blocks of the file, except for
                                 *    the last block, into the program area. Then
                                 *    read the last block of the file into the IO
                                 *    buffer, and transfer the needed amount of data
                                 *    from the IO buffer to the program area.
                                 */

                                result = 0;
                                if (allocSize <= ioBufferSize)
                                {
                                  DBUG(("Short copy\n"));
                                    memcpy(program, ioBuffer, allocSize);
                                }
                                else
                                {
                                  DBUG(("Copying first %d bytes to 0x%X from 0x%X\n", ioBufferSize, program, ioBuffer));
                                    memcpy(program, ioBuffer, ioBufferSize);
                                    if (blocksInFile * blockSize <= allocSize)
                                    {
                                        blocksToRead         = blocksInFile - numHeaderBlocks;
                                        readPartialLastBlock = FALSE;
                                    }
                                    else
                                    {
                                        blocksToRead         = blocksInFile - numHeaderBlocks - 1;
                                        readPartialLastBlock = TRUE;
                                    }

                                    if (blocksToRead > 0)
                                    {
                                      DBUG(("Reading bulk of file\n"));
                                        ioInfo.ioi_Recv.iob_Buffer = ((char *) program) + ioBufferSize;
                                        ioInfo.ioi_Recv.iob_Len    = blocksToRead * blockSize;

                                        result = DoIO(fileIOItem, &ioInfo);

                                        ioInfo.ioi_Offset += blocksToRead;
                                    }

                                    if (readPartialLastBlock && (result == 0))
                                    {
                                      DBUG(("Reading last block\n"));
                                        ioInfo.ioi_Recv.iob_Buffer = ioBuffer;
                                        ioInfo.ioi_Recv.iob_Len    = blockSize;

                                        result = DoIO(fileIOItem, &ioInfo);

                                      DBUG(("Copying last %d bytes\n", allocSize % blockSize));

                                        memcpy(((char *) program) + blockSize * (blocksInFile - 1), ioBuffer, allocSize % blockSize);
                                    }
                                }

                                if (result == 0)
                                {
                                    /* if the file's data didn't fill the whole
                                     * buffer, zap the rest to DEADFOOD
                                     */
                                    if (fileSize < allocSize) {
                                      DBUG(("Zapping DEADF00D\n"));
                                        memset((void *)((uint32)program + fileSize), 0xDEADF00D, allocSize - fileSize);
                                    }
                                }
                                else
                                {
                                    PrintError(NULL,"DoIO() of CMD_READ",fileName,result);
                                }
                            }
                            else
                            {
                                result = MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
                                PrintError(NULL,"AllocMem()","program code area",result);
                            }
                        }
                        else
                        {
                            /* don't be anal about it.... stop complaining already
                             *
                             * PrintError(NULL,"validate AIF Header of",fileName,BADAIF);
                             */
                            result = BADAIF;
                        }
                    }
                    else
                    {
                        PrintError(NULL,"DoIO() of CMD_READ",fileName,result);
                    }
                  DBUG(("Releasing %d bytes at 0x%X\n", ioBufferSize, ioBuffer));
                    FreeMem(ioBuffer,ioBufferSize);
                }
                else
                {
                    result = MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
                    PrintError(NULL,"AllocMem()","IO buffer",result);
                }
            }
            else
            {
                result = BADAIF;
            }
            DeleteItem(fileIOItem);
        }
        else
        {
            PrintError(NULL,"CreateIOReq()",NULL,fileIOItem);
            result = fileIOItem;
        }

        if ((result >= 0) && spawnTask)
        {
            programDirectory = file->ofi_File->fi_ParentDirectory;

            tags[0].ta_Tag = TAG_ITEM_NAME;
            tags[0].ta_Arg = (void *)file->ofi_File->fi_FileName;
            tags[1].ta_Tag = (priority >= 0) ? TAG_ITEM_PRI : TAG_NOP;
            tags[1].ta_Arg = (void *)priority;
            tags[2].ta_Tag = CREATETASK_TAG_AIF;
            tags[2].ta_Arg = (void *)program;
            tags[3].ta_Tag = CREATETASK_TAG_IMAGESZ;
            tags[3].ta_Arg = (void *)fileSize;
            tags[4].ta_Tag = CREATETASK_TAG_CMDSTR;
            tags[4].ta_Arg = (void *)name;
            tags[5].ta_Tag = (FILEFOLIO<<16) + FILETASK_TAG_CURRENTDIRECTORY;
            tags[5].ta_Arg = (void *)programDirectory->fi.n_Item;
            tags[6].ta_Tag = (FILEFOLIO<<16) + FILETASK_TAG_PROGRAMDIRECTORY;
            tags[6].ta_Arg = (void *)programDirectory->fi.n_Item;
            tags[7].ta_Tag = TAG_END;

	    DBUG(("Launching task\n"));

            result = CreateItem(MKNODEID(KERNELNODE,TASKNODE), tags);
	    DBUG(("Task launch returns 0x%X\n", result));
        }

        if ((result < 0) && program)
        {
            if (spawnTask)
            {
                /* return the allocated memory blocks to the system */
	      DBUG(("Giving back pages\n"));
                FreeMem(program,allocSize);
            }
            else
            {
	      DBUG(("Freeing pages\n"));
                program = (void *)((uint32)program - THREAD_PAD);
                FreeMem(program,allocSize + THREAD_PAD);
            }
            ScavengeMem();
            program = NULL;
        }

	DBUG(("Closing file\n"));

        CloseDiskFile(fileItem);
    }
    else
    {
        /* here too, don't be anal about it.... stop complaining already
         *
         * PrintError(NULL,"OpenDiskFile()",fileName,fileItem);
         */
        result = fileItem;
    }

    if (code)
        *code = program;

  DBUG(("Returning 0x%X\n", result));

    return (result);
}


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/file/loadcode
|||	LoadCode - Load a binary image into memory, and obtain a handle to it.
|||
|||	  Synopsis
|||
|||	    Err LoadCode(char *fileName, CodeHandle *code);
|||
|||	  Description
|||
|||	    This function loads an executable file from disk into
|||	    memory. Once loaded, the code can be spawned as a
|||	    thread, or executed as a subroutine.
|||
|||	    In order to work correctly with this and associated functions,
|||	    the executable file being loaded must have been linked
|||	    with threadstartup.o or subroutinestartup.o instead of the
|||	    usual cstartup.o
|||
|||	    You give this function the name of the executable file to
|||	    load, as well as a pointer to a CodeHandle variable, where
|||	    the handle for the loaded code will be stored. Note, "code"
|||	    must point to a valid CodeHandle variable, that's where
|||	    LoadCode() will put a pointer to the loaded code.
|||
|||	    Once you are done using the loaded code, you can remove it
|||	    from memory by passing the code handle to the UnloadCode()
|||	    function.
|||
|||	    To execute the loaded code, you must call either the
|||	    ExecuteAsThread() function or the ExecuteAsSubroutine()
|||	    function. Note that if the loaded code is reentrant, the
|||	    same loaded code can be spawned multiple times simultaneously
|||	    as a thread.
|||
|||	  Arguments
|||
|||	    fileName                    A NULL-terminated string indicating
|||	                                the executable file to load.
|||
|||	    code                        A pointer to a CodeHandle variable,
|||	                                where a handle to the loaded code
|||	                                can be put.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code if the file
|||	    could not be loaded.
|||
|||	  Implementation
|||
|||	    Folio call implemented in file folio V21.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
|||	  See Also
|||
|||	    LoadProgram(), UnloadCode(), ExecuteAsThread(),
|||	    ExecuteAsSubroutine()
**/

Err LoadCode(char *fileName, CodeHandle *code)
{
    return (internalLoadCode(fileName, -1, FALSE, code));
}


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/file/unloadcode
|||	UnloadCode - Unload a binary image previously loaded with LoadCode().
|||
|||	  Synopsis
|||
|||	    Err UnloadCode(CodeHandle code);
|||
|||	  Description
|||
|||	    This function frees any resources allocated by LoadCode().
|||	    Once UnloadCode() has been called, the code handle supplied
|||	    becomes invalid and cannot be used again.
|||
|||	  Arguments
|||
|||	    code                        A code handle filled in by a previous
|||	                                call to LoadCode().
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code if the CodeHandle
|||	    supplied is somehow invalid.
|||
|||	  Implementation
|||
|||	    Folio call Implemented in file folio V21.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
|||	  See Also
|||
|||	    LoadCode()
**/

Err UnloadCode(CodeHandle code)
{
  uint32 *ptr;

  if (code)
    {
      ptr = (uint32 *)((uint32)code - THREAD_PAD);
      FreeMem(ptr,*ptr);
    }
  return 0;
}


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/file/loadprogramprio
|||	LoadProgramPrio - Load a binary image and spawn it as a task, with
|||	                  priority.
|||
|||	  Synopsis
|||
|||	    Item LoadProgramPrio(char *cmdLine, int32 priority);
|||
|||	  Description
|||
|||	    This function loads an executable file from disk and
|||	    launches a new task to execute the code.
|||
|||	    You give this function a command-line to interpret.
|||	    The first component of the command-line is taken as the
|||	    name of the file to load. The entirety of the command-line
|||	    is passed to the new task as argc and argv in the main()
|||	    function. The filename component of the command-line specifies
|||	    either a fully-qualified pathname, or a pathname relative
|||	    to the current directory.
|||
|||	    The priority argument specifies the task priority to use
|||	    for the new task. If you simply want the new task to have
|||	    the same priority as the current task, use the
|||	    LoadProgram() function instead. Alternatively, passing a
|||	    negative priority to this function will also give the new
|||	    task the same priority as the current task.
|||
|||	    If a priority was given to the executable being launched using
|||	    the modbin utility, then the priority you give to this function
|||	    is ignored, and the priority that was specified to modbin will be
|||	    used instead.
|||
|||	  Arguments
|||
|||	    cmdLine                     A NULL-terminated string indicating
|||	                                the file to load as a task. The
|||	                                string may also contain arguments for
|||	                                the task being started.
|||
|||	    priority                    The task priority for the new task.
|||	                                For user code, this can be in the
|||	                                range 10 to 199.
|||
|||	  Return Value
|||
|||	    The Item number of the newly created task, or a negative error
|||	    code if the task could not be created.
|||
|||	  Implementation
|||
|||	    Folio call implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
|||	  See Also
|||
|||	    LoadProgram(), LoadCode(), ExecuteAsThread()
**/

Item LoadProgramPrio(char *path, int32 prio)
{
    return ((Item)internalLoadCode(path, prio, TRUE, NULL));
}


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/file/loadprogram
|||	LoadProgram - Load a binary image and spawn it as a task.
|||
|||	  Synopsis
|||
|||	    Item LoadProgram(char *cmdLine);
|||
|||	  Description
|||
|||	    This function loads an executable file from disk and
|||	    launches a new task to execute the code.
|||
|||	    You give this function a command-line to interpret.
|||	    The first component of the command-line is taken as the
|||	    name of the file to load. The entirety of the command-line
|||	    is passed to the new task as argc and argv in the main()
|||	    function. The filename component of the command-line specifies
|||	    either a fully-qualified pathname, or a pathname relative
|||	    to the current directory.
|||
|||	    The priority of the new task will be identical to the
|||	    priority of the current task. If you wish the task to have
|||	    a different priority, you must use the LoadProgramPrio()
|||	    function instead.
|||
|||	    If a priority was given to the executable being launched using
|||	    the modbin utility, then the priority that was specified to modbin
|||	    will be used for the new task, and not the current priority.
|||
|||	  Arguments
|||
|||	    cmdLine                     A NULL-terminated string indicating
|||	                                the file to load as a task. The string
|||	                                may also contain arguments for the task
|||	                                being started.
|||
|||	  Return Value
|||
|||	    The Item number of the newly created task, or a negative error
|||	    code if the task could not be created.
|||
|||	  Implementation
|||
|||	    Folio call implemented in file folio V20.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
|||	  See Also
|||
|||	    LoadProgramPrio(), LoadCode(), ExecuteAsThread()
**/

Item LoadProgram(char *path)
{
    return ((Item)internalLoadCode(path, -1, TRUE, NULL));
}


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/file/executeassubroutine
|||	ExecuteAsSubroutine - Execute previously loaded code as a subroutine.
|||
|||	  Synopsis
|||
|||	    int32 ExecuteAsSubroutine(CodeHandle code,
|||	                              int32 argc, char **argv);
|||
|||	  Description
|||
|||	    This function lets you execute a chunk of code that was
|||	    previously loaded from disk using LoadCode(). The code will
|||	    run as a subroutine of the current task or thread.
|||
|||	    In order to function correctly, code being run as a subroutine
|||	    should be linked with subroutinestartup.o instead of the usual
|||	    cstartup.o
|||
|||	    The argc and argv parameters are passed directly to the main()
|||	    entry point of the loaded code. The return value of this
|||	    function is the value returned by main() of the code being run.
|||
|||	    The values you supply for argc and argv are irrelevant to
|||	    this function. They are simply passed through to the loaded
|||	    code. Therefore, their meaning must be agreed upon by the
|||	    caller of this function, and by the loaded code.
|||
|||	  Arguments
|||
|||	    code                        A code handle filled in by a previous
|||	                                call to LoadCode().
|||
|||	    argc	                A value that is passed directly as the
|||	                                argc parameter to the loaded code's
|||	                                main() entry point. This function
|||	                                doesn't use the value of this argument,
|||	                                it is simply passed through to the
|||	                                loaded code.
|||
|||	    argv                        A value that is passed directly as the
|||	                                argv parameter to the loaded code's
|||	                                main() entry point. This function
|||	                                doesn't use the value of this argument,
|||	                                it is simply passed through to the
|||	                                loaded code.
|||
|||	  Return Value
|||
|||	    Returns the value that the loaded code's main() function returns.
|||
|||	  Implementation
|||
|||	    Folio call implemented in file folio V21.
|||
|||	  Caveats
|||
|||	    If code being executed as a subroutine calls exit(), the
|||	    parent thread or task will exit, not just the subroutine
|||	    code.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
|||	  See Also
|||
|||	    LoadCode(), UnloadCode(), ExecuteAsThread()
**/

int32 ExecuteAsSubroutine(CodeHandle code, uint32 argc, char **argv)
{
AIFHeader *hdr;

    if (!code)
        return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);

    hdr = (AIFHeader *)code;

    /* patch AIF header so it returns to the caller instead
     * of exiting
     */
    hdr->aif_SWIexit = 0xe8bddffe; /* ldmfd sp!,{r1-r12,r14,r15} */

    return (ExecuteAsSub(code,argc,argv));
}


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/file/executeasthread
|||	ExecuteAsThread - Execute previously loaded code as a thread.
|||
|||	  Synopsis
|||
|||	    Item ExecuteAsThread(CodeHandle code,
|||	                         int32 argc, char **argv,
|||	                         char *threadName, int32 priority);
|||
|||	  Description
|||
|||	    This function lets you execute a chunk of code that was
|||	    previously loaded from disk using LoadCode(). The code will
|||	    execute as a thread of the current task.
|||
|||	    In order to function correctly, code being run as a thread
|||	    should be linked with threadstartup.o instead of the usual
|||	    cstartup.o
|||
|||	    The argc and argv parameters are passed directly to the main()
|||	    entry point of the loaded code.
|||
|||	    The values you supply for argc and argv are irrelevant to
|||	    this function. They are simply passed through to the loaded
|||	    code. Therefore, their meaning must be agreed upon by the
|||	    caller of this function, and by the loaded code.
|||
|||	    threadName specifies the name of the thread.
|||
|||	    priority specifies the priority the new thread should have.
|||	    Providing a negative priority makes the thread inherit
|||	    the priority of the current task or thread.
|||
|||	  Arguments
|||
|||	    code                        A code handle filled in by a previous
|||	                                call to LoadCode().
|||
|||	    argc                        A value that is passed directly as the
|||	                                argc parameter to the loaded code's
|||	                                main() entry point. This function
|||	                                doesn't use the value of this argument,
|||	                                it is simply passed through to the
|||	                                loaded code.
|||
|||	    argv                        A value that is passed directly as the
|||	                                argv parameter to the loaded code's
|||	                                main() entry point. This function
|||	                                doesn't use the value of this argument,
|||	                                it is simply passed through to the
|||	                                loaded code.
|||
|||	    threadName                  The name of the thread. Should be a
|||	                                descriptive string that identifies this
|||	                                thread.
|||
|||	    priority                    The priority for the new thread. Supply
|||		                        a negative value to have the thread
|||	                                inherit the priority of the current
|||	                                task or thread. Priorities for user
|||	                                threads can be in the range of 10 to
|||	                                199.
|||
|||	  Return Value
|||
|||	    Returns the Item number of the new thread, or a negative error
|||	    code if the thread could not be created.
|||
|||	  Implementation
|||
|||	    Folio call implemented in file folio V21.
|||
|||	  Associated Files
|||
|||	    filefunctions.h
|||
|||	  See Also
|||
|||	    LoadCode(), UnloadCode(), ExecuteAsSubroutine()
**/

Item ExecuteAsThread(CodeHandle code, uint32 argc, char **argv,
                     char *threadName, int32 priority)
{
TagArg         tags[11];
_3DOBinHeader *hdr;
uint32         stackSize;
char          *stack;

    if (!code)
        return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);

    hdr       = (_3DOBinHeader *)((uint32)code + sizeof(AIFHeader));
    stackSize = hdr->_3DO_Stack;

    stack = (char *)AllocMem(stackSize,MEMTYPE_ANY);
    if (!stack)
        return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);

    tags[0].ta_Tag = TAG_ITEM_NAME;
    tags[0].ta_Arg = (void *)threadName;

    tags[1].ta_Tag = (priority >= 0) ? TAG_ITEM_PRI : TAG_NOP;
    tags[1].ta_Arg = (void *)priority;

    tags[2].ta_Tag = CREATETASK_TAG_PC;
    tags[2].ta_Arg = (void *)code;

    tags[3].ta_Tag = CREATETASK_TAG_STACKSIZE;
    tags[3].ta_Arg = (void *)stackSize;

    tags[4].ta_Tag = CREATETASK_TAG_SP;
    tags[4].ta_Arg = (void *)&stack[stackSize];

    tags[5].ta_Tag = CREATETASK_TAG_ARGC;
    tags[5].ta_Arg = (void *)argc;

    tags[6].ta_Tag = CREATETASK_TAG_ARGP;
    tags[6].ta_Arg = (void *)argv;

    tags[7].ta_Tag = CREATETASK_TAG_ALLOCDTHREADSP;

    tags[8].ta_Tag = TAG_ITEM_VERSION;
    tags[8].ta_Arg = (void *)hdr->_3DO_Item.n_Version;

    tags[9].ta_Tag = TAG_ITEM_REVISION;
    tags[9].ta_Arg = (void *)hdr->_3DO_Item.n_Revision;

    tags[10].ta_Tag = TAG_END;

    return CreateItem(MKNODEID(KERNELNODE,TASKNODE),tags);
}
