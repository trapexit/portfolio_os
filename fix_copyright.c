/*
 * $Id: fix_copyright.c,v 1.6 1995/02/10 19:55:35 limes Exp $
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>

/*
 * fix_copyright: look through a bunch of files and replace the
 * first ID string in the file with a copyright message.
 *
 * NOTE: only looks in the first FILECHUNK bytes of the file ...
 */

char                   *copyright[] =
{
    "Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.",
    "This material contains confidential information that is the property of The 3DO Company.",
    "Any unauthorized duplication, disclosure or use is prohibited."
};

#define	COPYR_LINES	((sizeof copyright) / (sizeof copyright[0]))

int                     verbose = 0;
int                     debug = 0;
int                     dryrun = 0;

char			id_head[] = "$Id";
char			id_tail[] = "$";

/*
 * The file input buffer needs to be one byte larger than the number
 * of bytes we read from the file at a time, so we can terminate the
 * data with a NUL character if we need to.
 */
#define	FILECHUNK	8192
char                    fbuf[FILECHUNK + 1];

/*
 * MAXPATHLEN is generally defined in a system header file; if we did
 * not get it, set up a sane default.
 */

#ifndef	MAXPATHLEN
#define	MAXPATHLEN	1024
#endif

int
dofile (char *ifn, int ifd)
{
    int                     i;
    int                     isz;
    int                     ofd;
    int                     osz;
    char                   *efbuf;
    char                   *id;
    char                   *eid;
    char                   *sol;
    char                   *eol;
    char                    ofn[MAXPATHLEN];
    char                    tfn[MAXPATHLEN];

    if (debug > 0)
	fprintf (stderr, "dofile: ifn='%s' ifd=%d\n", ifn, ifd);

/*
 * Get the first (up to) FILECHUNK bytes of the file, which should
 * contain the ID string.
 */
    isz = read (ifd, fbuf, FILECHUNK);
    if (isz < 0)
    {
	perror (ifn);
	return 0;
    }
    if (debug > 0)
	fprintf (stderr, "dofile: got %d chars\n", isz);

/*
 * Since we will be using string manipulation routines to search for
 * our key patterns, terminate the data we got with a NUL character.
 */
    fbuf[isz] = 0;

/*
 * Do not whack binary files. Easiest way to see if we have a binary
 * file is to check for a NUL character in the data we got; if there
 * is one, then strlen() will return its index in the string. If not,
 * it will find the sentinal NUL we planted above.
 */
    if (strlen (fbuf) != isz)
    {
	if (debug > 0)
	    fprintf (stderr, "dofile: binary file (has a NULL)\n");
	return 0;
    }

/*
 * keep a pointer to the end of the data,
 * it makes some of the later code simpler.
 */
    efbuf = fbuf + isz;

/*
 * Scan the buffer looking for the ID string.
 */
    id = strstr (fbuf, id_head);
    if (id == (char *) 0)
    {
	if (debug > 0)
	    fprintf (stderr, "dofile: no opening '$Id'\n");
	return 0;
    }

/*
 * See if the copyright string is already there; if it is,
 * then do not modify this file.
 */
    id[0] = 0;
    if (strstr (fbuf, copyright[0]))
    {
	if (debug > 0)
	    fprintf (stderr, "dofile: this file has already been converted\n");
	return 0;
    }
    id[0] = id_head[0];

/*
 * Locate the start and end of the ID line.
 *
 * "sol" points to the first character on the ID line. So we drop
 * a NUL byte at the ID and find the "rightmost" newline before
 * it; if we find one, bump sol to the next byte, else the first byte
 * on the line is the first byte of the buffer.
 */
    *id = 0;
    sol = strrchr (fbuf, '\n');
    if (sol == (char *) 0)
	sol = fbuf;
    else
	sol++;
    *id = id_head[0];
/*
 * "eol" points to the newline at the end of the ID line.
 */
    eol = strchr (id, '\n');
    if (eol == (char *) 0)
    {
	if (debug > 0)
	    fprintf (stderr, "dofile: unable to find end of ID line\n");
	return 0;
    }

/*
 * Locate the end of the ID string, which will be the first instance
 * of id_tail after id_head; this string must be before the end of the
 * line. If the string is found, point "eid" to the next character
 * after the closing string; otherwise, point it at the *newline*.
 */
    *eol = 0;
    eid = strstr (id + sizeof id_head, id_tail);
    if (eid == (char *) 0)
	eid = eol;
    else
	eid += sizeof id_tail - 1;
    *eol = '\n';

/*
 * If we are doing a "dry run" we now have enough information to tell
 * the user what we would have actually done: we would have attempted
 * to modify the file.
 */
    if (dryrun)
	return 1;

/*
 * If we are reading stdin, send the output to stdout.
 */
    if (ifd == 0)
	ofd = 1;
    else
    {
/*
 * Otherwise, generate a temporary file.
 * XXX- may fail on silly file systems that limit the length of
 * filename components.
 */
	sprintf (ofn, "%s.fco.%d", ifn, getpid ());
	sprintf (tfn, "%s.fct.%d", ifn, getpid ());
	if (debug > 0)
	    fprintf (stderr, "dofile: creating file '%s'\n", ofn);
	ofd = open (ofn, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (ofd < 0)
	{
	    perror (ofn);
	    return 0;
	}
    }
    if (debug > 0)
	fprintf (stderr, "dofile: ofd is %d\n", ofd);

/*
 * Construct the output file:
 * - everything before the ID line
 * - the copyright message (replacing the ID): for each line,
 *   - data on the ID line before the ID
 *   - a line of copyright message
 *   - data on the ID line after the ID
 * - the original ID line, and the remainder of the file.
 */

    if (sol > fbuf)
    {
	osz = write (ofd, fbuf, sol - fbuf);
	if (osz < 0)
	    goto badwrite;
    }

    for (i = 0; i < COPYR_LINES; ++i)
    {
	osz = write (ofd, sol, id - sol);
	if (osz < 0)
	    goto badwrite;
	osz = write (ofd, copyright[i], strlen (copyright[i]));
	if (osz < 0)
	    goto badwrite;
	if (eol >= eid) {
	    osz = write (ofd, eid, eol - eid + 1);
	    if (osz < 0)
		goto badwrite;
	}
    }

    osz = write (ofd, sol, efbuf - sol);
    if (osz < 0)
	goto badwrite;

/*
 * Copy the remainder of the file.
 */
    while ((isz = read (ifd, fbuf, FILECHUNK)) > 0)
    {
	osz = write (ofd, fbuf, isz);
	if (osz < 0)
	    goto badwrite;
    }
    if (debug > 0)
	fprintf (stderr, "dofile: output file created\n");

    if (ifd)
    {
	close (ofd);
/*
 * move files around so the new data appears in place of the old data.
 * NOTE: hardlinks to the original data will retain their old content
 * NOTE: softlinks to the original data now refer to the new data
 */
	if (rename (ifn, tfn) < 0)		/* save original file */
	{
	    perror (tfn);
	    return -1;
	}
	if (rename (ofn, ifn) < 0)		/* move new file into place */
	{
	    perror (ofn);
	    (void) rename (tfn, ifn);		/* try to recover original data */
	    (void) unlink (ofn);		/* try to discard temp file */
	    return -1;
	}
	if (unlink (tfn) < 0)			/* original data no longer needed */
	{
	    perror (tfn);
	    return -1;
	}
	if (debug > 0)
	    fprintf (stderr, "dofile: moved new file into place\n");
    }
    return 1;

badwrite:
    perror (ofn);
nocando:
    close (ofd);
    unlink (ofn);
    unlink (tfn);
    return -1;
}

int
doname (char *ifn)
{
    int                     ifd;
    int                     rv;

    if (debug > 0)
	fprintf (stderr, "doname: ifn is '%s'\n", ifn);
    ifd = open (ifn, O_RDONLY);
    if (ifd < 0)
    {
	perror (ifn);
	return -1;
    }
    rv = dofile (ifn, ifd);
    close (ifd);
    return rv;
}

int
main (int ac, char **av)
{
    int                     dostdin = 1;
    char                   *ap;
    int                     r;

    if (ac > 1)
    {
	while (ap = *++av)
	{
	    if (*ap == '-')
	    {
		while (ac = *++ap)
		{
		    switch (ac)
		    {
		    case 'v':
			++verbose;
			break;
		    case 'd':
			++debug;
			fprintf (stderr, "debug level %d\n", debug);
			break;
		    case 'n':
			++dryrun;
			break;
		    }
		}
	    }
	    else
	    {
		r = doname (ap);
		if ((dryrun > 0) || (verbose > 0))
		{
		    fprintf (stderr, "fix_copyright: %s '%s'\n",
			     r < 0 ? "unable to convert" :
			     r > 0 ? "converted" :
			     "no need to convert",
			     ap);
		}
		dostdin = 0;
	    }
	}
    }

    if (dostdin)
    {
	r = dofile ("stdin", 0);
	if ((dryrun > 0) || (verbose > 0))
	{
	    fprintf (stderr, "fix_copyright: %s stdin\n",
		     r < 0 ? "unable to convert" :
		     r > 0 ? "converted" :
		     "no need to convert");
	}
    }
    return 0;
}
