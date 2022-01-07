/* $Id: cd2mac.c,v 1.6 1994/02/18 01:54:55 limes Exp $ */
/*
** Copy a binary file from cd-rom to Mac
** No character conversion.
** By:  Phil Burk
*/

/*
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

#include "types.h"
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "io.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "stdarg.h"
#include "stdio.h"
#include "strings.h"
#include "operror.h"
#include "audio.h"
#include "music.h"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

#define NUMBLOCKS  (32)
#define BLOCKSIZE  (2048)
#define BUFSIZE  (NUMBLOCKS*BLOCKSIZE)

#define NUL ('\0')

#define EOL ('\r')

/*****************************************************************/
/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\Failure in",name,val); \
		goto cleanup; \
	}


/*****************************************************************/
int main (int argc, char *argv[])
{
	Stream *str;
	FILE *fid;
	char *infile, *outfile;
	int32 NumRead, NumWritten;
	char *tbuf;

	str  = NULL;
	tbuf = NULL;
	fid  = NULL;
	
/* Allocate buffer for transfer. */
	tbuf = (char *) malloc(BUFSIZE + 1000);
	printf("malloc successful, tbuf = %x\n", tbuf);
	printf("=> 0x%x\n", tbuf);
	
	if(argc < 3)
	{
		printf("Usage: cd2mac <3dofilename>  <macfilename>\n");
		goto cleanup;
	}
	
/* Get file names. */
	infile = argv[1];
	str =  OpenDiskStream( infile, 0);
	if (str == NULL)
	{
		ERR(("Couldn't open %s\n", infile));
		goto cleanup;
	}
	
	
/* Open Mac file for writing. */
	outfile = argv[2];
	fid = fopen(outfile, "w");
	if (fid == NULL)
	{
		printf("Could not open %s\n", outfile);
		goto cleanup;
	}
	
/* Loop through buffers full until end. */
	do
	{
		NumRead = ReadDiskStream( str, (char *) tbuf, BUFSIZE );
		if(NumRead < 0)
		{
			ERR(("Error read file = 0x%x.\n", NumRead));
			goto cleanup;
		}
DBUG(("Write %d bytes.\n", NumRead ));

		NumWritten = fwrite( tbuf, 1, NumRead, fid );
		if(NumWritten != NumRead)
		{
			ERR(("Error writing file.\n"));
			goto cleanup;
		}
	} while(NumRead == BUFSIZE);
	
printf("%s written.\n", outfile);
	
cleanup:
	if (str) CloseDiskStream(str);
	if (fid) fclose(fid);
	if (tbuf)free(tbuf);
	return 0;
}

