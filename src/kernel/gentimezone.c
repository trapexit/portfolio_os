/* $Id: gentimezone.c,v 1.1 1994/10/26 01:07:15 limes Exp $ */

/*
 * gentimezone: generate the "timezone.c" file which contains the
 * offset west of GMT in seconds for the local time on the system that
 * compiled sherry, as of the date sherry was compiled. This is used
 * for printing out the "local" time for the system modules, which
 * assumes that they were compiled in the same timezone (and within
 * the same daylight savings time state) as the sherry that is running
 * them. Oh well, it is at least more useful than GMT.
 */

#include <sys/time.h>

main()
{
    struct timezone tz;
    gettimeofday((struct timeval *)0, &tz);
    printf("int timezone = %d;\n", tz.tz_minuteswest * 60);
    return 0;
}
