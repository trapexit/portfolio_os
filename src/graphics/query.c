/* $Id: query.c,v 1.5 1994/12/13 22:20:50 ewhac Exp $ */

/* ************************************************************************
 *
 * Graphics routines for the Opera Hardware
 *
 * Copyright (C) 1992, New Technologies Group, Inc.
 * NTG Trade Secrets  -  Confidential and Proprietary
 *
 * The contents of this file were designed with tab stops of 4 in mind
 *
 * DATE   NAME             DESCRIPTION
 * ------ ---------------- -------------------------------------------------
 * 940414 SHL              Created this file
 *
 * ********************************************************************** */


#include "types.h"

#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "io.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include <tags.h>

#include "intgraf.h"


uint32 _fieldcount=0;


Err
QueryGraphics ( int32 tag, void *ret )
{

#ifdef DEVELOPMENT
  Err	e;

  if ((e = SuperValidateMem (CURRENTTASK, ret, sizeof (uint32))) < 0)
    return (e);
#endif

  switch (tag) {
  case QUERYGRAF_TAG_END:
    break;
  case QUERYGRAF_TAG_FIELDFREQ:
    *((int32*)ret) = GrafBase->gf_VBLFreq;
    break;
  case QUERYGRAF_TAG_FIELDTIME:
    *((int32*)ret) = GrafBase->gf_VBLTime;
    break;
  case QUERYGRAF_TAG_FIELDCOUNT:
    *((int32*)ret) = _fieldcount;
    break;
  case QUERYGRAF_TAG_DEFAULTWIDTH:
    *((int32*)ret) = GrafBase->gf_DefaultDisplayWidth;
    break;
  case QUERYGRAF_TAG_DEFAULTHEIGHT:
    *((int32*)ret) = GrafBase->gf_DefaultDisplayHeight;
    break;
  case QUERYGRAF_TAG_DEFAULTDISPLAYTYPE:
    *((int32*)ret) = GrafBase->gf_DefaultDisplayType;
    break;
  default:
    return GRAFERR_BADTAG;
  }
  return 0;
}


/*
 * This is not a fully protected walk of the tag list.  However, since this
 * routine executes in user mode, this isn't a security issue.
 */
Err
QueryGraphicsList ( TagArg *ta )
{
  TagArg	*state;
  Err		err;

  err = 0;
  if (!(state = ta))
    return (err);

  while (ta = NextTagArg ((const TagArg **) &state)) {
    if ((err = QueryGraphics (ta->ta_Tag, ta->ta_Arg)) < 0)
      break;
  }
  return (err);
}
