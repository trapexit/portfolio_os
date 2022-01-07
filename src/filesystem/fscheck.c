/*****
$Id: fscheck.c,v 1.3 1994/08/04 17:57:09 shawn Exp $

$Log: fscheck.c,v $
 * Revision 1.3  1994/08/04  17:57:09  shawn
 * Added rcs Log recording to source.
 *
*****/

/*
 *
 *      Copyright The 3DO Company Inc., 1993
 *      All Rights Reserved Worldwide.
 *      Company confidential and proprietary.
 *      Contains unpublished technical data.
 */

/*
 *      fscheck.c - All filesystem type maintainer.
 */

#include	"stdio.h"
#include	"item.h"
#include	"task.h"
#include	"filefunctions.h"

/*
 *	fstab is the table of all filesystem types that need to be
 *	checked/repaired at system startup.
 */
char *fstab[] = {
	"$c/lmadm -^a ram 3 0 nvram",
	NULL		/* last line */
};

#undef	DEBUG
#ifdef	DEBUG
#define	DBUG(x)	printf x ;
#else	/* DEBUG */
#define	DBUG(x)	/* x */;
#endif	/* DEBUG */

#define WAITTASK(itemid)	\
		{ \
			while (LookupItem(itemid)) { \
				WaitSignal(SIGF_DEADTASK); \
			} \
		}

int
main()
{
	int	i;
	char	*cmdline;
	Item	result;

	for (i = 0, cmdline = fstab[0]; cmdline; cmdline = fstab[++i]) {
		DBUG(("DOING \"%s\"\n", cmdline));
		result = LoadProgram(cmdline);
		WAITTASK(result);
	}
	return 0;
}
