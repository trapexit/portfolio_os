/* $Id: sherryvers.h,v 1.6 1994/12/10 00:42:50 limes Exp $ */

/*
 * THIS FUNCTIONALITY IS INTENDED FOR USE ONLY WITHIN 3DO OWNED AND
 * MAINTAINED CODE. Frankly, it is not clear that it *could* be useful
 * for our software licensees ...
 *
 * Version numbers returned by DiscOsVersion
 *
 * These correspond to the Vers/Rev fields of the sherry image used
 * for each version.
 *
 * These values are generally used when some title depends on
 * particluar behaviors of the version of the system with which it was
 * encrypted, and the "current" behavior at the time of construction
 * of a "Rom-over-CD" type image is not compatible; the RoCD image can
 * check MakeDiscOsVersion and provide the older behavior based on
 * this version number.
 *
 * When booting on a DEV station, we can't really get the version
 * number of the disc from dipir as it never looked at it to begin
 * with. So, we return the revision number of the latest version, via
 * the DiscOs_DEV constant.
 *
 * If someone uses DiscOs_DEV in their code, the value they use,
 * forever, will be the version that they were compiled with. This is
 * a feature, not a bug. But then, if the code is being slapped onto a
 * CD, it should never ever make sense for it to want to know
 * DiscOsVersion at runtime.
 */

#ifndef	__SHERRYVERS_H
#define	__SHERRYVERS_H

extern uint32	__swi(KERNELSWI+35)	swiDiscOsVersion(uint32);	/* directly to sherry */
extern uint32				DiscOsVersion(uint32);		/* available from libc */
extern uint32				SuperDiscOsVersion(uint32);	/* available from libkernel */

/*
 * The raw data returned by DiscOsVersion is, at the moment, the
 * Version/Revision codes from the "sherry" module that the dipir got
 * from the disc. This may change, but the value of these constants
 * will not change (ie. if we put it here, we will make sure that
 * DiscOsVersion returns it correctly).
 */
#define	MakeDiscOsVersion(maj,min)	(((maj)<<8)|(min))

#define	DiscOs_Ver(vers)	((vers)>>8)
#define	DiscOs_Rev(vers)	((vers)&255)

/*
 * This list contains many (most? all?) of the releases that have been
 * used to encrypt titles. You may want to use one of the comparison
 * macros further down in the file, instead of comparing against these
 * numbers.
 */
#define	DiscOs_1_0	MakeDiscOsVersion(20,1)
#define	DiscOs_1_01	MakeDiscOsVersion(20,7)
#define	DiscOs_1_1	MakeDiscOsVersion(20,15)
#define	DiscOs_1_2	MakeDiscOsVersion(20,20)
#define	DiscOs_1_2_3	MakeDiscOsVersion(20,21)
#define	DiscOs_1_3	MakeDiscOsVersion(21,10)
#define	DiscOs_1_4	MakeDiscOsVersion(23,10)

/*
 * A value is needed that we can pass into DiscOsVersion that will
 * cause it to re-figure the version by calling Dipir or examining the
 * sherry header again. Here it is.
 */
#define	DiscOsReset	(~0)

/*
 * Best, of course, is to use logical macros, so the right thing
 * happens with intermediate revisions, even though these revisions
 * should never actually be used to cut customer discs ...
 */
#define	DiscOs_Before_1_3(vers)	((vers) < MakeDiscOsVersion(21,0))
#define	DiscOs_Before_1_4(vers)	((vers) < MakeDiscOsVersion(23,0))
#define	DiscOs_Before_2_5(vers)	((vers) < MakeDiscOsVersion(24,0))

#endif	/* __SHERRYVERS_H */
