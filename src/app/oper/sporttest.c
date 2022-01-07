/*  :ts=8 bk=0
 *
 * sporttest.c:	Test SPORT device functionality.
 *
 * Leo L. Schwab					9407.25
 ***************************************************************************
 * Copyright 1994 The 3DO Company.  All Rights Reserved.
 *
 * 3DO Trade Secrets  -  Confidential and Proprietary
 ***************************************************************************
 *			     --== RCS Log ==--
 *
 * $Id: sporttest.c,v 1.2 1994/10/24 23:34:32 ewhac Exp $
 *
 * $Log: sporttest.c,v $
 * Revision 1.2  1994/10/24  23:34:32  ewhac
 * Added usage line.  Also added printing of absolute address where
 * pages differ so one may look at them in the debugger.
 *
 * Revision 1.1  1994/08/26  01:33:40  ewhac
 * Initial revision
 *
 */
#include <types.h>
#include <graphics.h>	/*  You have *GOT* to be kidding...  */
#include <mem.h>
#include <io.h>
#include <stdio.h>


#define	MAXREPS		256


/* sporttest.c */
int main(int ac, char **av);
void opensport(void);
void testsport(void);
void parseargs(int ac, char **av);
void die(char *);




Item	sportdev;
Item	sportio[MAXREPS];

uint32	mask = ~0;
int32	npages = 16;
int32	cmd = -1;
int32	reps = 1;


static char	*usage = "\
Usage: sporttest {clone | copy} [-n<#pages>] [-r<#reps>]\n";


int
main (ac, av)
int	ac;
char	**av;
{
	parseargs (ac, av);

	opensport ();

	if (cmd < 0)
		die (usage);

	testsport ();
}



void
opensport ()
{
	register int	i;
	Item		foo;

	if ((foo = FindNamedItem
		    (MKNODEID (KERNELNODE, DEVICENODE), "SPORT")) < 0)
		die ("Can't find SPORT device.\n");

	if ((sportdev = OpenItem (foo, NULL)) < 0)
		die ("Can't open SPORT device.\n");

	for (i = reps;  --i >= 0; )
		if ((sportio[i] = CreateIOReq (NULL, 100, sportdev, 0)) < 0)
			die ("Can't create SPORT I/O.\n");
}


void
testsport ()
{
	register int	i;
	IOInfo		ioi;
	IOReq		*ior;
	int32		srcsize, destsize, rdestsize, pagesize;
	int32		*src, *dest, *rdest;
	int32		*srcp, *destp;
	int32		err;

	memset (&ioi, 0, sizeof (ioi));

	pagesize = GetPageSize (MEMTYPE_VRAM);
	destsize = npages * pagesize;
	rdestsize = destsize + pagesize * 2;
	if (cmd == SPORTCMD_COPY)
		srcsize = destsize;
	else
		srcsize = pagesize;

	if (!(src = AllocMem (srcsize, MEMTYPE_VRAM | MEMTYPE_STARTPAGE)))
		die ("Can't allocate source pages.\n");

	if (!(rdest = AllocMem (rdestsize, MEMTYPE_VRAM | MEMTYPE_STARTPAGE)))
		die ("Can't allocate destination pages.\n");

	dest = (int32 *) ((char *) rdest + pagesize);


	/*
	 * Initialize source and destination regions.
	 */
	for (srcp = src, i = srcsize / sizeof (int32);  --i >= 0; )
		*srcp++ = 0xDEADBEEF;

	for (destp = rdest, i = rdestsize / sizeof (int32);  --i >= 0; )
		*destp++ = 0xC0EDBABE;

	/*
	 * Perform the SPORT operation.
	 */
	ioi.ioi_Command		= cmd;
	ioi.ioi_Offset		= mask;
	ioi.ioi_Send.iob_Buffer	= src;
	ioi.ioi_Send.iob_Len	= srcsize;
	ioi.ioi_Recv.iob_Buffer	= dest;
	ioi.ioi_Recv.iob_Len	= destsize;
	for (i = reps;  --i >= 0; ) {
		if ((err = SendIO (sportio[i], &ioi)) < 0) {
			printf ("SPORT SendIO() failed: ");
			PrintfSysErr (err);
			die ("Aborting.\n");
		}
	}

	for (i = reps;  --i >= 0; ) {
		if ((err = WaitIO (sportio[i])) < 0) {
			printf ("SPORT WaitIO() failed: ");
			PrintfSysErr (err);
			die ("Aborting.\n");
		}
		if (!(ior = LookupItem (sportio[i])))
			die ("What I/O request?\n");
		if (ior->io_Error) {
			printf ("I/O Error: %d (0x%08lx)\n", ior->io_Error);
			die ("Aborting.\n");
		}
	}


	/*
	 * Validate results.
	 * Check that guard pages weren't touched.
	 */
	for (destp = rdest, i = pagesize / sizeof (int32);  --i >= 0; ) {
		if (*destp++ != 0xC0EDBABE) {
			printf ("Forward guard trashed!  dest offset = 0x%08lx\n",
				destp - rdest);
			die ("Keeling over...\n");
		}
	}

	for (destp = (int32 *) ((char *) rdest + rdestsize - pagesize),
	      i = pagesize / sizeof (int32);
	     --i >= 0;
	     )
	{
		if (*destp++ != 0xC0EDBABE) {
			printf ("Rear guard trashed!  dest offset = 0x%08lx\n",
				destp - rdest);
			die ("Keeling over...\n");
		}
	}

	/*
	 * Check that pages actually got modified.
	 */
	if (cmd == SPORTCMD_COPY) {
		for (srcp = src, destp = dest, i = destsize / sizeof (int32);
		     --i >= 0;
		     )
		{
			if (*destp++ !=
			    ((*srcp++ & mask) | (0xC0EDBABE & ~mask)))
			{
/*- - - - - - - - - - - - - - -*/

srcp--;  destp--;
printf ("Mismatch!\nsrc = 0x%08lx, dest = 0x%08lx, mask = 0x%08lx\n",
	*srcp, *destp, mask);
printf ("offsets: src = 0x%08lx, dest = 0x%08lx\n",
	srcp - src, destp - dest);
printf ("srcp @ 0x%08lx, destp @ 0x%08lx\n", srcp, destp);
die ("Keeling over...\n");

/*- - - - - - - - - - - - - - -*/
			}
		}
	} else {
		for (destp = dest, i = npages;  --i >= 0; ) {
			for (srcp = src, i = pagesize / sizeof (int32);
			     --i >= 0;
			     )
			{
				if (*destp++ !=
				    ((*srcp++ & mask) | (0xC0EDBABE & ~mask)))
				{
/*- - - - - - - - - - - - - - - - - - -*/

srcp--;  destp--;
printf ("Mismatch!\nsrc = 0x%08lx, dest = 0x%08lx, mask = 0x%08lx\n",
	*srcp, *destp, mask);
printf ("offsets: src = 0x%08lx, dest = 0x%08lx\n",
	srcp - src, destp - dest);
die ("Keeling over...\n");

/*- - - - - - - - - - - - - - - - - - -*/
				}
			}
		}
	}

	printf ("Huzzah!\n");
}


void
parseargs (ac, av)
int	ac;
char	**av;
{
	char	*arg;

	while (++av, --ac) {
		arg = *av;
		while (*arg) {
			*arg = tolower (*arg);
			arg++;
		}

		arg = *av;
		if (*arg == '-') {
			switch (*++arg) {
			case 'n':
				npages = atoi (arg + 1);
				break;
			case 'm':
				mask = atoi (arg + 1);
				break;
			case 'r':
				if ((reps = atoi (arg + 1)) > MAXREPS)
					die ("-r: Too many (>256) reps.\n");
				break;
			default:
				printf ("Unknown switch -%c; ignored.\n",
					*arg);
				break;
			}
		} else {
			if (!strcmp (arg, "copy"))
				cmd = SPORTCMD_COPY;
			else if (!strcmp (arg, "clone"))
				cmd = SPORTCMD_CLONE;
			else {
				printf ("Unknown command: %s\n", arg);
				die ("Aborting.\n");
			}
		}
	}
}

void
die (char *str)
{
	printf (str);
	exit (1);
}
