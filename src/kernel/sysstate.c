/* $Id: sysstate.c,v 1.3 1994/11/15 02:22:35 vertex Exp $ */

#include "types.h"
#include "operror.h"
#include "sysinfo.h"
#include "super.h"
#include "sysstate.h"


/*****************************************************************************/


static BootDiscTypes bootDisc = BDT_NotSet;
static setBootDisc            = FALSE;


/*****************************************************************************/


Err internalGetSystemState(uint32 tag, void *info, size_t infosize)
{
    switch (tag)
    {
        case SYSSTATE_TAG_BOOTDISCTYPE: if (infosize > sizeof(uint32))
                                            *(uint32 *)info = (uint32)bootDisc;

                                        return 0;

        default                       : return BADTAG;
    }
}


/*****************************************************************************/


Err internalSetSystemState(uint32 tag, void *info, size_t infosize)
{
    switch (tag)
    {
        case SYSSTATE_TAG_BOOTDISCTYPE: if (setBootDisc)
                                            return -1;  /* can't call twice */

                                        bootDisc    = (BootDiscTypes)((uint32)info);
                                        setBootDisc = TRUE;

                                        SuperSetSysInfo(SYSINFO_TAG_BOOTDISCTYPE,
                                                        (void *)((uint32)bootDisc),0);

                                        return 0;

        default                       : return BADTAG;
    }
}
