/***************************************************************\
*								*
* Operamath folio                                               *
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  16-Jul-93					*
*								*
* Copyright (c) 1992, 1993, The 3DO Company                     *
* All rights reserved						*
* This program is proprietary and confidential			*
*								*
\***************************************************************/

/* $Id: mathfolio.c,v 1.24 1994/08/29 18:19:39 sdas Exp $ */


#define DBUG(x) { printf x; }
#define SDBUG(x) { Superkprintf x; }

#include "mathfolio.h"
#include "ctype.h"
#include "sysinfo.h"
#include "debug.h"

#include "stdio.h"


extern int32 isUser (void);

void MathError (void);
void halt (void);

void *(*MathSWIFuncs[])() = {
  (void *(*)())MathError,	/* 24 */
  (void *(*)())MathError,	/* 23 */
  (void *(*)())MathError,	/* 22 */
  (void *(*)())MathError,	/* 21 */
  (void *(*)())MathError,	/* 20 */
  (void *(*)())MathError,	/* 19 */
  (void *(*)())sMulManyVec3Mat33DivZ_F16,	/* 18 */
  (void *(*)())sMulVec3Mat33DivZ_F16,	/* 17 */
  (void *(*)())sAbsVec4_F16,	/* 16 */
  (void *(*)())sAbsVec3_F16,	/* 15 */
  (void *(*)())sCross3_F16,	/* 14 */
  (void *(*)())sDot4_F16,	/* 13 */
  (void *(*)())sDot3_F16,	/* 12 */
  (void *(*)())sMulObjectMat44_F16,	/* 11 */
  (void *(*)())sMulObjectVec4Mat44_F16,	/* 10 */
  (void *(*)())sMulManyVec4Mat44_F16,	/* 9 */
  (void *(*)())sMulMat44Mat44_F16,	/* 8 */
  (void *(*)())sMulVec4Mat44_F16,	/* 7 */
  (void *(*)())MulScalarF16,	/* 6 */
  (void *(*)())MulManyF16,	/* 5 */
  (void *(*)())sMulObjectMat33_F16,	/* 4 */
  (void *(*)())sMulObjectVec3Mat33_F16,	/* 3 */
  (void *(*)())sMulManyVec3Mat33_F16,	/* 2 */
  (void *(*)())sMulMat33Mat33_F16,	/* 1 */
  (void *(*)())sMulVec3Mat33_F16,	/* 0 */
};


/* routines added just for debugging purposes */
struct ccbin {
  int32 w, h;
  int32 x01, y01;
  int32 x03, y03;
  int32 x123, y123;
  int64 n;
};

struct ccbout {
  int32 hdx, hdy;
  int32 vdx, vdy;
  int32 ddx, ddy;
};

/* int32 qdiv (int64 *n, int32 d); */
/* int32 getnz (void); */
/* void doccb (struct ccbin, struct ccbout); */


#ifndef _STUB
void *(*GreenMathSWIFuncs[])() = {
  (void *(*)())MathError,	/* 24 */
  (void *(*)())MathError,	/* 23 */
  (void *(*)())MathError,	/* 22 */
  (void *(*)())MathError,	/* 21 */
  (void *(*)())MathError,	/* 20 */
  (void *(*)())MathError,	/* 19 */
  (void *(*)())gMulManyVec3Mat33DivZ_F16,	/* 18 */
  (void *(*)())gMulVec3Mat33DivZ_F16,	/* 17 */
  (void *(*)())gAbsVec4_F16,	/* 16 */
  (void *(*)())gAbsVec3_F16,	/* 15 */
  (void *(*)())gCross3_F16,	/* 14 */
  (void *(*)())gDot4_F16,	/* 13 */
  (void *(*)())gDot3_F16,	/* 12 */
  (void *(*)())gMulObjectMat44_F16,	/* 11 */
  (void *(*)())gMulObjectVec4Mat44_F16,	/* 10 */
  (void *(*)())gMulManyVec4Mat44_F16,	/* 9 */
  (void *(*)())gMulMat44Mat44_F16,	/* 8 */
  (void *(*)())gMulVec4Mat44_F16,	/* 7 */
  (void *(*)())MulScalarF16,	/* 6 */
  (void *(*)())MulManyF16,	/* 5 */
  (void *(*)())gMulObjectMat33_F16,	/* 4 */
  (void *(*)())gMulObjectVec3Mat33_F16,	/* 3 */
  (void *(*)())gMulManyVec3Mat33_F16,	/* 2 */
  (void *(*)())gMulMat33Mat33_F16,	/* 1 */
  (void *(*)())gMulVec3Mat33_F16,	/* 0 */
};
#else
void *(*GreenMathSWIFuncs[])() = {
  (void *(*)())MathError,	/* 24 */
  (void *(*)())MathError,	/* 23 */
  (void *(*)())MathError,	/* 22 */
  (void *(*)())MathError,	/* 21 */
  (void *(*)())MathError,	/* 20 */
  (void *(*)())MathError,	/* 19 */
  (void *(*)())halt,	/* 18 */
  (void *(*)())halt,	/* 17 */
  (void *(*)())halt,	/* 16 */
  (void *(*)())halt,	/* 15 */
  (void *(*)())halt,	/* 14 */
  (void *(*)())halt,	/* 13 */
  (void *(*)())halt,	/* 12 */
  (void *(*)())halt,	/* 11 */
  (void *(*)())halt,	/* 10 */
  (void *(*)())halt,	/* 9 */
  (void *(*)())halt,	/* 8 */
  (void *(*)())halt,	/* 7 */
  (void *(*)())MulScalarF16,	/* 6 */
  (void *(*)())MulManyF16,	/* 5 */
  (void *(*)())halt,	/* 4 */
  (void *(*)())halt,	/* 3 */
  (void *(*)())halt,	/* 2 */
  (void *(*)())halt,	/* 1 */
  (void *(*)())halt,	/* 0 */
};
#endif


#ifndef _STUB
void *(*AnvilMathSWIFuncs[])() = {
  (void *(*)())MathError,	/* 24 */
  (void *(*)())MathError,	/* 23 */
  (void *(*)())MathError,	/* 22 */
  (void *(*)())MathError,	/* 21 */
  (void *(*)())MathError,	/* 20 */
  (void *(*)())MathError,	/* 19 */
  (void *(*)())aMulManyVec3Mat33DivZ_F16,	/* 18 */
  (void *(*)())aMulVec3Mat33DivZ_F16,	/* 17 */
  (void *(*)())gAbsVec4_F16,	/* 16 */
  (void *(*)())gAbsVec3_F16,	/* 15 */
  (void *(*)())gCross3_F16,	/* 14 */
  (void *(*)())gDot4_F16,	/* 13 */
  (void *(*)())gDot3_F16,	/* 12 */
  (void *(*)())gMulObjectMat44_F16,	/* 11 */
  (void *(*)())gMulObjectVec4Mat44_F16,	/* 10 */
  (void *(*)())gMulManyVec4Mat44_F16,	/* 9 */
  (void *(*)())gMulMat44Mat44_F16,	/* 8 */
  (void *(*)())gMulVec4Mat44_F16,	/* 7 */
  (void *(*)())MulScalarF16,	/* 6 */
  (void *(*)())MulManyF16,	/* 5 */
  (void *(*)())gMulObjectMat33_F16,	/* 4 */
  (void *(*)())gMulObjectVec3Mat33_F16,	/* 3 */
  (void *(*)())gMulManyVec3Mat33_F16,	/* 2 */
  (void *(*)())gMulMat33Mat33_F16,	/* 1 */
  (void *(*)())gMulVec3Mat33_F16,	/* 0 */
};
#else
void *(*AnvilMathSWIFuncs[])() = {
  (void *(*)())MathError,	/* 24 */
  (void *(*)())MathError,	/* 23 */
  (void *(*)())MathError,	/* 22 */
  (void *(*)())MathError,	/* 21 */
  (void *(*)())MathError,	/* 20 */
  (void *(*)())MathError,	/* 19 */
  (void *(*)())halt,	/* 18 */
  (void *(*)())halt,	/* 17 */
  (void *(*)())halt,	/* 16 */
  (void *(*)())halt,	/* 15 */
  (void *(*)())halt,	/* 14 */
  (void *(*)())halt,	/* 13 */
  (void *(*)())halt,	/* 12 */
  (void *(*)())halt,	/* 11 */
  (void *(*)())halt,	/* 10 */
  (void *(*)())halt,	/* 9 */
  (void *(*)())halt,	/* 8 */
  (void *(*)())halt,	/* 7 */
  (void *(*)())MulScalarF16,	/* 6 */
  (void *(*)())MulManyF16,	/* 5 */
  (void *(*)())halt,	/* 4 */
  (void *(*)())halt,	/* 3 */
  (void *(*)())halt,	/* 2 */
  (void *(*)())halt,	/* 1 */
  (void *(*)())halt,	/* 0 */
};
#endif

#define NUM_MATHSWIFUNCS (sizeof(MathSWIFuncs)/sizeof(*MathSWIFuncs))

void *(*MathUserFuncs[])() = {
  (void *(*)()) RecipSF16,	/* -8 */
  (void *(*)()) RecipUF16,	/* -7 */
  (void *(*)()) DivRemSF16,	/* -6 */
  (void *(*)()) DivSF16,	/* -5 */
  (void *(*)()) DivRemUF16,	/* -4 */
  (void *(*)()) DivUF16,	/* -3 */
  (void *(*)()) MulSF16,	/* -2 */
  (void *(*)()) MulUF16,	/* -1 */
};

#define NUM_MATHUSERFUNCS (sizeof(MathUserFuncs)/sizeof(*MathUserFuncs))


extern void __main (void);
int32 MathInit (void);

TagArg MathFolioTags[] = {
  { CREATEFOLIO_TAG_DATASIZE,	(void *) sizeof (MathFolio) },
  { CREATEFOLIO_TAG_NSWIS,	(void *) NUM_MATHSWIFUNCS },
  { CREATEFOLIO_TAG_NUSERVECS,	(void *) NUM_MATHUSERFUNCS },
  { CREATEFOLIO_TAG_SWIS,	(void *) GreenMathSWIFuncs },
  { CREATEFOLIO_TAG_USERFUNCS,	(void *) MathUserFuncs },
  { CREATEFOLIO_TAG_INIT,	(void *) ((int32)MathInit) },
  { TAG_ITEM_NAME,	(void *) "Operamath" },
  { CREATEFOLIO_TAG_ITEM,	(void *) 5 },
  { TAG_ITEM_PRI,	(void *) 0 },
  { 0,				(void *) 0 },
};

Semaphore *MathSemaphore;

int32 qsi (uint32 a, uint32 b, uint32 c);
int32 ssi (uint32 a, uint32 b, uint32 c);


int
InstallMathFolio (int argc, char *argv[])
{
  Item result;
  int32 j, k;

#if 1
  j = CallBackSuper (ssi, SYSINFO_TAG_MATHDIVOUTFIX, 0, 0);
  j = CallBackSuper (ssi, SYSINFO_TAG_MATHDIVOVERLAPFIX, 0, 0);
  j = CallBackSuper (ssi, SYSINFO_TAG_MATHSWAPFIX, 0, 0);
#else
  j =  ssi (SYSINFO_TAG_MATHDIVOUTFIX, 0, 0);
  j =  ssi (SYSINFO_TAG_MATHDIVOVERLAPFIX, 0, 0);
  j =  ssi (SYSINFO_TAG_MATHSWAPFIX, 0, 0);
#endif

  if (j<0) {
    DBUG (("Green/Preen MADAM chip in system\n"));
    /* MathFolioTags[3].ta_Arg = (void*) GreenMathSWIFuncs; */ /* default */
  } else {
    DBUG (("Anvil MADAM chip in system\n"));
    /* For now - force Green level support all the time */
    /* MathFolioTags[3].ta_Arg = (void*) AnvilMathSWIFuncs; */
  }

  for (k=1; k<argc; k++) {
    if (argv[k][0] != '-') {
      DBUG (("Error - unrecognized parameter %s\n", argv[k]));
      return (-1);
    }
    switch (tolower(argv[k][1])) {
    case 'm':
      switch (tolower(argv[k][2])) {
      case 'g':
	DBUG (("Forcing Green chip support\n"));
	MathFolioTags[3].ta_Arg = (void*) GreenMathSWIFuncs;
	break;
      case 's':
	DBUG (("Forcing software only support\n"));
	MathFolioTags[3].ta_Arg = (void*) MathSWIFuncs;
	break;
      case 'a':
	DBUG (("Forcing Anvil support\n"));
	MathFolioTags[3].ta_Arg = (void*) AnvilMathSWIFuncs;
	break;
      default:
	DBUG (("Warning - Unrecognized parameter %s\n", argv[k]));
	break;
      }
      break;
    default:
      break;
    }
  }

  result = CreateSemaphore ("MathSemaphore", 0);
  if (result >= 0)
  {
      MathSemaphore = (Semaphore *)LookupItem(result);

      result = CreateItem(MKNODEID(KERNELNODE,FOLIONODE),MathFolioTags);
      if (result >= 0)
      {
          return (int)result;
      }
      PrintError(NULL,"create the operamath folio",NULL,result);
      DeleteSemaphore(MathSemaphore->s.n_Item);
  }
#ifdef DEVELOPMENT
  else
  {
      PrintError(NULL,"create the operamath semaphore",NULL,result);
  }
#endif

  return (int) result;
}


int32
MathInit (void)
{
  *MATH_CONTROLCLEAR = 0xffffffff;
  *MATH_CONTROLSET = 0x00000065;
  return 0;
}


void
MathError (void)
{
  if (isUser()) {
    DBUG (("Operamath Error - call to unimplemented function\n"));
  } else {
    Superkprintf (("Operamath Error - call to unimplemented function\n"));
  }
}


#ifdef _STUB
void
halt (void)
{
  Superkprintf (("Matrix function called\n"));
  for (;;);
}
#endif


int32
qsi (uint32 a, uint32 b, uint32 c)
{
  return SuperQuerySysInfo (a, (void *)b, c);
}


int32
ssi (uint32 a, uint32 b, uint32 c)
{
  return SuperSetSysInfo (a, (void *)b, c);
}

