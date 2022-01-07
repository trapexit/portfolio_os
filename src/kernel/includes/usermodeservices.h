#ifndef __USERMODESERVICES_H
#define __USERMODESERVICES_H

#pragma force_top_level
#pragma include_only_once


/*****************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __ITEM_H
#include "item.h"
#endif

#ifndef __AIF_H
#include "aif.h"
#endif

#ifndef __H_FILESYSTEM
#include "filesystem.h"
#endif


/*****************************************************************************/


typedef enum UMS
{
   UMS_LOADMODULE,
   UMS_UNLOADMODULE
} UMS;

typedef struct UMSHeader
{
    UMS ums_Service;
} UMSHeader;

typedef struct UMSLoadModuleArgs
{
    UMSHeader     ums_Hdr;
    int32         ums_Argc;            /* in  */
    char        **ums_Argv;
    char         *ums_ItemPath;
    char         *ums_ItemName;
    uint8         ums_ItemType;
    uint8         ums_ItemSubsysType;

    void         *ums_DemandLoad;      /* out */
    uint8         ums_ItemVersion;
    uint8         ums_ItemRevision;
    bool          ums_ItemPrivileged;
} UMSLoadModuleArgs;

typedef struct UMSUnloadModuleArgs
{
    UMSHeader   ums_Hdr;              /* in */
    int32       ums_Argc;             /* in */
    char      **ums_Argv;             /* in */
    void       *ums_DemandLoad;       /* in */
} UMSUnloadModuleArgs;


/*****************************************************************************/


Err UserModeService(void *umsArgs, uint32 umsArgSize, bool privilegedServer);

Item LoadModule(const char *typeName, const char *itemName,
		int32 cntype, void **context);

Err UnloadModule(ItemNode *it, void *context);


/*****************************************************************************/


#endif /* __USERMODESERVICES_H */
