/* file: Hollywood.h */
/* Hollywood FMV card definitions */
/* 4/16/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#ifndef HOLLYWOOD_HEADER
#define HOLLYWOOD_HEADER

#define HLWD_GCLK			40000000L		/* 40 Mhz */
#define HLWD_VCLCK			HLWD_GCLK		/* 40 Mhz */
#define HLWD_ACLCK			20000000L		/* 20 Mhz */
//#define HLWD_ACLCK			18000000L		/* 18 Mhz */
#define HLWD_AUDIOFREQ		44100L			/* 44.1 Khz initial audio sample frequency */
//#define HLWD_AUDIOFREQ		60000L			/* 60 Khz initial audio sample frequency */

#define HLWD_ASYSCLKRATIO	32L				/* PCM output is 32 * sampling rate */

#endif
