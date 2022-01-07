
static char rcsid[] = "$Id: readfile.c,v 1.1 1994/05/09 20:10:58 limes Exp $";

/*
 * File Read and Compare test
 */

#include "types.h"
#include "stdio.h"

#include "filestream.h"
#include "filestreamfunctions.h"

#include "strings.h"

#include "mem.h"

extern int              errno;

#define	REPS	1000

#define	TriggerLogan()	do { unsigned junk; junk = *((vuint32 *)0x100); } while (0)

int
main (int ac, char **av)
{
    uint32                 *rbuf;
    uint32                 *tbuf;
    Stream                 *fstr;
    int32                   fsz;
    int32                   chk;
    uint32                  tec, rep, ec, i;
    uint32                  rc, nc;
    uint32                  rc2, nc2;

    fstr = OpenDiskStream (av[1], 0);
    if (fstr == (Stream *) 0)
    {
	printf ("unable to open file '%s'; errno=%d\n", av[1], errno);
	return 1;
    }
    fsz = 512 << 10;			/* max half a meg */

    rbuf = (uint32 *) AllocMem ((1<<20), 0);
    tbuf = (uint32 *) AllocMem (fsz, MEMTYPE_STARTPAGE | MEMTYPE_SYSTEMPAGESIZE);
    if (tbuf == (uint32 *)0) {
	printf ("unable to allocdate initial read buffer\n");
	return 1;
    }

    chk = ReadDiskStream (fstr, (char *) tbuf, fsz);
    if (chk < 1)
    {
	printf ("unable to read '%s': got %d (0x%8X) bytes, errno=%d\n",
		av[1], chk, chk, errno);
	return 1;
    }
    CloseDiskStream (fstr);
    FreeMem (tbuf, fsz);
    if (rbuf != (uint32 *)0)
	FreeMem (rbuf, 1<<20);

    fsz = chk;

    printf ("file fsz is %d (0x%08X) bytes\n", fsz);

    rbuf = (uint32 *) AllocMem (fsz, MEMTYPE_STARTPAGE | MEMTYPE_SYSTEMPAGESIZE);
    if (rbuf == (uint32 *) 0)
    {
	printf ("unable to allocate %d (0x%08X) bytes for rbuf buffer\n",
		fsz, fsz);
	return 1;
    }
    printf ("rbuf buffer at 0x%08X\n", rbuf);

    tbuf = (uint32 *) AllocMem (fsz, MEMTYPE_STARTPAGE | MEMTYPE_SYSTEMPAGESIZE);
    if (tbuf == (uint32 *) 0)
    {
	printf ("unable to allocate %d (0x%08X) bytes for tbuf buffer\n",
		fsz, fsz);
	return 1;
    }
    printf ("tbuf buffer at 0x%08X\n", tbuf);

    fstr = OpenDiskStream (av[1], 0);
    if (fstr == (Stream *) 0)
    {
	printf ("unable to open '%s' for rbuf read\n", av[1]);
	return 1;
    }
    chk = ReadDiskStream (fstr, (char *) rbuf, fsz);
    CloseDiskStream (fstr);
    if (chk != fsz)
    {
	printf ("rbuf read failed; expected %d (0x%08X) bytes, got %d (0x%08X).\n",
		fsz, fsz, chk, chk);
	return 1;
    }

    tec = 0;
    for (rep = 0; rep < REPS; ++rep)
    {
	ec = 0;
	fstr = OpenDiskStream (av[1], 0);
	if (fstr == (Stream *) 0)
	{
	    printf ("unable to open '%s' for tbuf read %d\n", av[1], rep);
	    return 1;
	}
	chk = ReadDiskStream (fstr, (char *) tbuf, fsz);
	CloseDiskStream (fstr);
	if (chk != fsz)
	{
	    printf ("tbuf read %d failed; expected %d (0x%08X) bytes, got %d (0x%08X).\n",
		    rep, fsz, fsz, chk, chk);
	    return 1;
	}
	for (chk = 0; chk < (fsz / 4); ++chk)
	{
	    rc = rbuf[chk];
	    nc = tbuf[chk];
	    if (rc != nc)
	    {
TriggerLogan ();
		if (ec < 16)
		    printf ("offset %06X: rbuf=0x%02X tbuf=0x%02X xor=0x%02X\n",
			    chk << 2, rc, nc, rc ^ nc);
		rc2 = rbuf[chk];
		if (rc != rc2)
		{
		    printf ("reread error from rbuf; 1st=0x%08X 2nd=0x%08X\n",
			    rc, rc2);
		}
		nc2 = tbuf[chk];
		if (nc != nc2)
		{
		    printf ("reread error from tbuf; 1st=0x%08X 2nd=0x%08X\n",
			    nc, nc2);
		}
		ec++;
		tbuf[chk] = 0xDEADBEEF;
	    }
	}
	printf ("%8d errors (of %d bytes) in read %d\n", ec, fsz, rep);
	tec += ec;
    }
    printf ("%8d errors in %d passes of %d bytes\n", tec, REPS, fsz);
    return 0;
}
