/* $Id: dumpfile.c,v 1.3 1994/07/26 01:50:38 stan Exp $ */
/****************************************************************
*                                                               *
* General routine to dump a file                                *
*                                                               *
* By:  Stan Shepard                                             *
*                  	                                            *
* Last update:  12-May-93                                        *
*                                                               *
* Copyright (c) 1993, The 3DO Company                           *
*                                                               *
* This program is proprietary and confidential                  *
*                                                               *
*****************************************************************/


#define DBUG(x)	{ printf x ; }
#define FULLDBUG(x) /* { printf x ; } */

#include "types.h"

#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "io.h"
#include "list.h"
#include "driver.h"
#include "device.h"

#include "filesystem.h"
#include "filefunctions.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "directory.h"
#include "directoryfunctions.h"

#include "strings.h"
#include "stdlib.h"
#include "stdio.h"

#define	MAX_LARGE	0x7fffffff

void usage(void);



int
main (int argc, char *argv[])
{
    int32 i, j, k;
	int32 bytesperline, linesperblock;
	int32 offset, numbytes, dumpascii;
    Stream *fstream;
	char buffer[40];

	printf("\n");

	bytesperline = 16;		/* Number of bytes per line to dump */
	linesperblock = 16;		/* Number of bytes per line to dump */
	dumpascii = 1;			/* Yes dump file in ascii too */
	offset = 0;				/* Start at beginning of file */
	numbytes = MAX_LARGE;		/* Dump the entire file */

    if (argc < 2 || argc > 4)
	{
        usage();
		return(0);
    }

    if ( (fstream = OpenDiskStream (argv[1], 0)) == NULL)
	{
		printf("ERROR: opening %s\n", argv[1]);
		return(1);
	}

	if (argc > 2)
	{
		offset = strtol(argv[2], (char **)NULL, 0);
		DBUG(("New offset is %x (%d)\n", offset, offset));
	}

	if (argc > 3)
	{
		numbytes = strtol(argv[3], (char **)NULL, 0);
		DBUG(("New numbytes is %x (%d)\n", numbytes, numbytes));
	}

/*
 * Read and dump line by line
 */
	if ((i = SeekDiskStream(fstream, offset, SEEK_SET)) != offset)
	{
		printf("\nERROR: seeking to initial offset\n");
		printf("Status = %x\n", i);
		return(4);
	}

/*
 * Read 0 bytes due to stream bug
 *  (Remove these lines to exihibit bug with certain offsets)
 */
/* 	i = ReadDiskStream(fstream, buffer, 0); */
/* 	printf("Initial read is %d\n", i); */

	while ( (i = ReadDiskStream(fstream, buffer, 
				(bytesperline - (offset % bytesperline))) ) == 
				(bytesperline - (offset % bytesperline))      )
	{
		if (offset % (bytesperline * linesperblock) == 0)
			printf("\nOffset = %x\n", offset);
		DBUG(("Chars read = %2d    ", i));

		for (j = 0; j < offset % bytesperline; j++)
			printf("   ");

		for (j = k = offset % bytesperline; j < bytesperline; j++)
			printf("%02x ", buffer[j - k]);

		if (dumpascii)
		{
			for (j = 0; j < offset % bytesperline; j++)
				printf("  ");

			for (j = k = offset % bytesperline; j < bytesperline; j++)
			{
				if (buffer[j - k] > 31 && buffer[j - k] < 127)
					printf(" %c", buffer[j - k]);
				else
					printf(" .");
			}
		}
		printf("\n");
		offset = offset + bytesperline - (offset % bytesperline);
	}
	DBUG(("Chars read = %2d    ", i));

/*
 * If we stopped due to error
 */
	if (i < 0)
	{
        DBUG (("Error reading file %s\n", argv[1]));
        CloseDiskStream (fstream);
        return(2);
    }

/*
 * Handle rest of line if any
 */
 	if (i > 0)
	{
		for (j = 0; j < i; j++)
			printf("%02x ", buffer[j]);
		
		for (j = i; j < bytesperline; j++)
			printf("   ");
		
		if (dumpascii)
		{
			for (j = 0; j < i; j++)
			{
				if (buffer[j] > 31 && buffer[j] < 127)
					printf(" %c", buffer[j]);
				else
					printf(" .");
			}
		}
		
		offset += i;
		printf("\n");
	}

	printf("Bytes read = %x (%d)\n", offset, offset);
    CloseDiskStream (fstream);
}


/*
 * Tell the world what this program is about
 */
void
usage(void)
{
    printf("\nUsage:\n\tDumpfile <filename> [<offset> [<numbytes>]]\n");
    printf("<filename> is the file to Dump\n");
    printf("<offset>   (optional) is the byte offset to start file Dump at\n");
    printf("<numbytes> (optional) is the number of bytes to Dump\n");
}
