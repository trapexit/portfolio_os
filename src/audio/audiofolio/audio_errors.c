/* $Id: audio_errors.c,v 1.14 1994/11/21 20:25:33 phil Exp $ */
/****************************************************************
**
** Error Messages for Audio Folio
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************
** 930504 PLB Added missing comma after string to prevent concatenation.
** 930812 PLB Added AF_ERR_NOTOWNER, and more missing commas.
****************************************************************/

#include "types.h"

#include "nodes.h"
#include "list.h"
#include "kernelnodes.h"
#include "strings.h"
#include "item.h"
#include "folio.h"
#include "kernel.h"

#include "operror.h"
#include "audio_internal.h"

static char *AF_ErrTable[] =
{
	"no error",
	"file not found",       /* AF_ERR_BASE+0 */
	"no FIFO",
	"no sample attached",
	"instrument has no knobs",
	"named thing not found",
	"Item has no instrument",
	"bad calculation type",
	"knob connected to illegal resource",
	"bad file io",
	"external reference not satisfied",
	"illegal resource type",
	"illegal relocation resource type",
	"DSP code relocation error",
	"illegal resource attribute",
	"item is still in use",
	"already freed or detached",
	"too many markers in AIFF file",
	"general OFX file error",
	"couldn't allocate DSP resource",
	"bad IFF file type",
	"file not open",
	"could not allocate a signal",
	"value out of range",
	"unimplemented feature",
	"NULL address",
	"device timeout",
	"security violation",
	"item not owned by caller",
	"OpenAudioFolio() not called by thread",
	"internal dsp instrument error",
	"contradictory tags"
};
#define MAX_AF_ERR_SIZE	sizeof("OpenAudioFolio() not called by thread")

TagArg ErrTags[] =
{
    TAG_ITEM_NAME,	(void *)"Audio Errors",
    ERRTEXT_TAG_OBJID,	(void *)((ER_FOLI<<ERR_IDSIZE)|(ER_ADIO)),
    ERRTEXT_TAG_MAXERR,	(void *)(sizeof(AF_ErrTable)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,	(void *)AF_ErrTable,
    ERRTEXT_TAG_MAXSTR,	(void *)MAX_AF_ERR_SIZE,
    TAG_END,		0
};

Item InitAudioErrors(void)
{
	Item ErrItem;
	
	if ((ErrItem = CreateItem(MKNODEID(KERNELNODE,ERRORTEXTNODE),ErrTags)) < 0) {
		PrintError(0,"create audio error table",0,ErrItem);
	}
	return(ErrItem);
}
