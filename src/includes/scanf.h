#ifndef __SCANF_H
#define __SCANF_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: scanf.h,v 1.4 1994/09/10 01:22:35 peabody Exp $
**
******************************************************************************/


#define NOSTORE      01
#define LONG	     02
#define SHORT	     04
#define FIELDGIVEN  010
#define LONGDOUBLE  020
#define ALLOWSIGN   040    /* + or - acceptable to rd_int  */
#define DOTSEEN    0100    /* internal to rd_real */
#define NEGEXP	   0200    /* internal to rd_real */
#define NUMOK	   0400    /* ditto + rd_int */
#define NUMNEG	  01000    /* ditto + rd_int */


/*****************************************************************************/


#endif /* SCANF_H */
