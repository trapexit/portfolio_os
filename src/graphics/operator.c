/* *************************************************************************
 *
 * Graphics operator task for the Opera Hardware
 *
 * Copyright (C) 1992, New Technologies Group, Inc.
 * NTG Trade Secrets  -  Confidential and Proprietary
 *
 * The contents of this file were designed with tab stops of 4 in mind
 *
 * $Id: operator.c,v 1.33 1994/09/14 03:07:23 ewhac Exp $
 *
 * DATE   NAME             DESCRIPTION
 * ------ ---------------- -------------------------------------------------
 * 930629 SHL              Prints out version and revision on startup
 * 920724 -RJ Mical        Start overhaul
 * 920717 Stephen Landrum  Last edits before July handoff
 *
 * ********************************************************************** */


/*#define TEST*/
#define DBUG(x) printf x
#define GRAFDBUG(x) /* printf x */

/*#define MEMTEST*/

/*#define DEMOTASK*/
/*#define RAMTEST*/
/*#define TIMERTEST*/
/*#define POLLIO*/
/*#define MACTEST*/
/*#define CDROMTEST*/

#include "types.h"
#include "ctype.h"
#include "stdlib.h"
#include "stdio.h"

#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "strings.h"
#include "semaphore.h"
#include "msgport.h"
#include "io.h"
#include "operror.h"
#include "debug.h"
#include "interrupts.h"

#include "intgraf.h"


extern void __main(void);
/*???#include "rom.h"*/

extern TagArg GrafFolioTags[];

/* passed internally to graphics folio */
uint32 linewidth=320;
uint32 palflag=FALSE, pal2flag=FALSE, _overrideflag=FALSE, slipstream_dispctrl=0;

uint32 _HDelayOverride, _tripaddress;
extern volatile uint32 _firqstate;
extern int32 _timediff;


extern void graphicsFirq (void);

TagArg GraphicsFirqTags[] = {
  TAG_ITEM_PRI,	(void *)250,
  TAG_ITEM_NAME,	(void *)"Graphics FIRQ",
  CREATEFIRQ_TAG_CODE,	(void *)((long)graphicsFirq),
  CREATEFIRQ_TAG_NUM,	(void *)INT_V1,
  TAG_ITEM_END,	(void *)0,
};


void opentimer (void);
void gettime (uint32 timebuffer[2]);



int
InstallGraphicsFolio (int argc, char *argv[])
{ /* This runs in user mode! */
  /* However all of memory is writable! */
  /* Therefore Operator cannot call any routines */
  /* or use any memory that is writable by other */
  /* processes */
  /*struct KernelBase *kb = KernelBase;*/
  Item firq;
  int  i;
  Item result;

  firq = CreateItem(MKNODEID(KERNELNODE,FIRQNODE), GraphicsFirqTags);
  if (firq >= 0 )
  {
      while (_firqstate<3) {
      }

      DBUG (("i = %ld, threshold = %ld\n", _timediff, (16684L+20000L)/2));

      for (i = 1;  i < argc;  i++) {
        if (argv[i][0] != '-') {
          DBUG (("Error - unrecognized parameter %s\n", argv[i]));
          return (-1);
        }
        switch (tolower(argv[i][1])) {
        case 'w':
          linewidth = strtol (argv[i]+2, 0, 0);
          if (!FindValidWidth (linewidth, BMF_DISPLAYABLE)) {
            DBUG(("graphix operator: illegal line width use: 320,384,512,640,1024\n"));
            return (-1);
          }
          break;
        case 'p':
          _overrideflag = TRUE;
          palflag = TRUE;
          if (argv[i][2]=='2') {
            pal2flag = TRUE;
          }
          break;
        case 'n':
          _overrideflag = TRUE;
          palflag = FALSE;
          pal2flag = FALSE;
          break;
        case 's':
          slipstream_dispctrl = VDL_SLPDCEL|VDL_BACKTRANS;
          break;
        case 'h':
          _HDelayOverride = strtoul(argv[i]+2, 0, 0);
          DBUG (("HDelay set to 0x%lx\nOverride stored at 0x%lx\n",
                 _HDelayOverride, (uint32)&_HDelayOverride));
          break;
        case 't':
          DBUG (("tripaddress = %lx\n", (uint32)&_tripaddress));
          break;
        case 'm':   /* Math folio paramater - ignore */
          break;
        default:
          DBUG (("Error - unrecognized parameter %s\n", argv[i]));
          return (-1);
        }
      }

      /* Start up the Graphics folio */
      result = CreateItem(MKNODEID(KERNELNODE,FOLIONODE),GrafFolioTags);
      if (result >= 0)
      {
          InitGraphicsErrors();
          return (int) result;
      }
      PrintError(NULL,"create graphics folio",NULL,result);
      DeleteItem(firq);
  }
  else
  {
      PrintError(NULL,"create graphics FIRQ handler",NULL,firq);
      result = firq;
  }

  return (int) result;
}
