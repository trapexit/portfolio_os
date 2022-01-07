/* $Id: launchapp.c,v 1.8.1.1 1994/12/06 22:07:50 vertex Exp $ */

#include "types.h"
#include "nodes.h"
#include "list.h"
#include "task.h"
#include "device.h"
#include "mem.h"
#include "string.h"
#include "io.h"
#include "aif.h"
#include "operror.h"
#include "cdrom.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "filesystem.h"
#include "filefunctions.h"
#include "stdio.h"
#include "shell.h"
#include "shelldebug.h"
#include "launchapp.h"


/*****************************************************************************/


static bool videoCD;
static bool photoCD;
static bool checkedISO;


/*****************************************************************************/


/* root directory block on an ISO-9660 disc */
typedef struct VolumeDescriptor
{
    uint8 type;                         /* type of this descriptor                                      */
    uint8 formatStandard[5];            /* identifies the format of this volume                         */
    uint8 version;                      /* version of this descriptor                                   */
    uint8 filler1;                      /* reserved                                                     */
    uint8 systemName[32];               /* name of the host system                                      */
    uint8 volumeName[32];               /* volume name                                                  */
    uint8 filler2[8];                   /* reserved                                                     */
    uint8 volumeSize_le[4];             /* volume size (little-endian)                                  */
    uint8 volumeSize[4];                /* volume size (big-endian)                                     */
    uint8 filler3[32];                  /* reserved                                                     */
    uint8 volumeSetCount_le[2];         /* number of volumes in volume set (little-endian)              */
    uint8 volumeSetCount[2];            /* number of volumes in volume set (big-endian)                 */
    uint8 volumeSetIndex_le[2];         /* index of this volume in volume set (little-endian)           */
    uint8 volumeSetIndex[2];            /* index of this volume in volume set (big-endian)              */
    uint8 logicalBlockSize_le[2];       /* size of a block on this volume (little-endian)               */
    uint8 logicalBlockSize[2];          /* size of a block on this volume (big-endian)                  */
    uint8 pathTableSize_le[4];          /* size of path table in bytes (little-endian)                  */
    uint8 pathTableSize[4];             /* size of path table in bytes (big-endian)                     */
    uint8 pathTabDiskAddr_le[4];        /* disk address of L-type path table (little-endian)            */
    uint8 pathTabDiskAddr2_le[4];       /* disk address of redundant L-type path table (little-endian)  */
    uint8 pathTabDiskAddr[4];           /* disk address of M-type path table (big-endian)               */
    uint8 pathTabDiskAddr2[4];          /* disk address of redundant M-type path table (big-endian)     */
    uint8 rootDirectoryEntry[34];       /* root directory information                                   */
    uint8 volumeSetName[128];           /* volume set identification                                    */
    uint8 publisherName[128];           /* publisher identification                                     */
    uint8 dataPreparerName[128];        /* data preparer identification                                 */
    uint8 applicationName[128];         /* application identification                                   */
    uint8 copyrightFilePathName[37];    /* path name of file containing copyright                       */
    uint8 abstractFilePathName[37];     /* path name of file containing abstract                        */
    uint8 bibliographicFilePathName[37];/* path name of file containing bibliography                    */
    uint8 creationTime[17];             /* volume creation time                                         */
    uint8 modificationTime[17];         /* volume modification time                                     */
    uint8 expirationTime[17];           /* volume information expiration date                           */
    uint8 effectiveTime[17];            /* volume information effective after date                      */
    uint8 formatVersion;                /* version of volume format                                     */
    uint8 filler4;                      /* reserved                                                     */
} VolumeDescriptor;

/* an entry in a ISO-9660 path table */
typedef struct PathTableEntry
{
    uint8 nameLength;           /* length of directory name             */
    uint8 extendedLength;       /* length of extended-attribute record  */
    uint8 discAddr[4];          /* first block of extent of directory   */
    uint8 parentDirectoryID[2]; /* path table index of parent directory */
} PathTableEntry;


/*****************************************************************************/


/* some helpful macros for ISO-9660 parsing */
#define FRAME_SIZE         2048
#define FRAMES_PER_SECOND  75
#define GetFrameNum(m,s,f) (((uint32)m * 60 + (uint32)s) * FRAMES_PER_SECOND + (uint32)f)


/*****************************************************************************/


static Err GetCDStatus(Item cdromio, DeviceStatus *ds)
{
IOInfo ioInfo;

    memset(&ioInfo,0,sizeof(ioInfo));
    ioInfo.ioi_Command         = CMD_STATUS;
    ioInfo.ioi_Recv.iob_Buffer = ds;
    ioInfo.ioi_Recv.iob_Len    = sizeof(DeviceStatus);

    return (DoIO(cdromio,&ioInfo));
}


/*****************************************************************************/


static Err GetCDData(Item cdromio, struct CDROM_Disc_Data *cddd)
{
IOInfo ioInfo;

    memset(&ioInfo, 0, sizeof(IOInfo));

    ioInfo.ioi_Command         = CDROMCMD_DISCDATA;
    ioInfo.ioi_Unit            = 0;
    ioInfo.ioi_Flags           = 0;
    ioInfo.ioi_Send.iob_Buffer = 0;
    ioInfo.ioi_Send.iob_Len    = 0;
    ioInfo.ioi_Recv.iob_Buffer = cddd;
    ioInfo.ioi_Recv.iob_Len    = sizeof(struct CDROM_Disc_Data);

    return DoIO(cdromio, &ioInfo);
}


/*****************************************************************************/


static Err GetFrames(Item cdromio, uint32 startFrame, uint32 numFrames,
                     void *destination)
{
IOInfo ioInfo;

    memset(&ioInfo, 0, sizeof(IOInfo));

    ioInfo.ioi_Command         = CMD_READ;
    ioInfo.ioi_Unit            = 0;
    ioInfo.ioi_Flags           = 0;
    ioInfo.ioi_Offset          = startFrame;
    ioInfo.ioi_CmdOptions      = 0;
    ioInfo.ioi_Send.iob_Buffer = 0;
    ioInfo.ioi_Send.iob_Len    = 0;
    ioInfo.ioi_Recv.iob_Buffer = destination;
    ioInfo.ioi_Recv.iob_Len    = numFrames * FRAME_SIZE;

    return DoIO(cdromio, &ioInfo);
}


/*****************************************************************************/


/* compare a length limited string with a NULL-terminated one */
static bool CompareStr(char *chars, int32 count, char *string)

{
    while (count--)
    {
        if (*string++ != *chars++)
            return FALSE;
    }

    /* All the characters matched, now make sure the strings have
     * the same length
     */

    if (*string)
        return FALSE;

    return TRUE;
}


/*****************************************************************************/


/* convert 4 bytes into a uint32 */
static uint32 Int32Cast(uint8 raw[4])

{
    return ((uint32) raw[0] << 24) |
           ((uint32) raw[1] << 16) |
           ((uint32) raw[2] << 8)  |
            (uint32) raw[3];
}


/*****************************************************************************/


/* convert 2 bytes into a uint32 */
static uint32 Int16Cast(uint8 raw[2])

{
    return ((uint32) raw[0] << 8) | (uint32) raw[1];
}


/*****************************************************************************/


/* search for a name within a path table */
static bool FindPathEntry(PathTableEntry *pte, char *name)

{
    /* Visit all entries until an empty entry is found, unless the
     * desired entry is found first
     */

    while (pte->nameLength)
    {
        /* we only want entries in the root directory */
        if ((Int16Cast(pte->parentDirectoryID) == 1)
          && CompareStr((char *)pte + sizeof(PathTableEntry),(int32)pte->nameLength, name))
        {
            return TRUE;
        }

        pte = (PathTableEntry *) ((char *)pte + sizeof(PathTableEntry) + pte->nameLength);

        /* PathTableEntry structures are always 16-bit aligned */
        if ((uint32)pte & 1)
            pte = (PathTableEntry*) ((char*) pte + 1);
    }

    return FALSE;
}


/*****************************************************************************/


/* check if we've got a VideoCD or a PhotoCD */
static void CheckISOCD(Item cdromio)
{
VolumeDescriptor       *vd;
uint32                  track1Frame;
uint32                  sessionFrame;
uint32                  pteBytes;
uint32                  pteFrames;
PathTableEntry         *pte;
struct CDROM_Disc_Data  cddd;

    /* only do this code once */
    if (!checkedISO)
    {
        checkedISO = TRUE;

        vd = (VolumeDescriptor *)AllocMem(FRAME_SIZE,MEMTYPE_DMA);
        if (vd)
        {
            if (GetCDData(cdromio,&cddd) >= 0)
            {
                sessionFrame = 0;
                track1Frame  = GetFrameNum(cddd.TOC[1].minutes,
                                           cddd.TOC[1].seconds,
                                           cddd.TOC[1].frames);

                /* Determine the address of the last session, if
                 * this is a multi-session disc
                 */

                if (cddd.session.valid)
                {
                   /* retain session address relative to beginning of track one */
                   sessionFrame = GetFrameNum(cddd.session.minutes,
                                              cddd.session.seconds,
                                              cddd.session.frames) - track1Frame;
                }

                /* read the root block */
                if (GetFrames(cdromio, 16 + sessionFrame + track1Frame, 1, vd) >= 0)
                {
                    /* is this an ISO-9660 format? */
                    if (CompareStr(vd->formatStandard, 5, "CD001"))
                    {
                        pteBytes  = Int32Cast(vd->pathTableSize);
                        pteFrames = ((pteBytes + FRAME_SIZE - 1) / FRAME_SIZE);

                        pte = (PathTableEntry *)AllocMem(pteFrames * FRAME_SIZE,MEMTYPE_DMA);
                        if (pte)
                        {
                            /* read the path table for the root */
                            if (GetFrames(cdromio,Int32Cast(vd->pathTabDiskAddr) + track1Frame,pteFrames,pte) >= 0)
                            {
                                /* try and find interesting directories... */
                                videoCD = FindPathEntry(pte,"MPEGAV");
                                photoCD = FindPathEntry(pte,"PHOTO_CD");
                            }

                            FreeMem(pte,pteFrames * FRAME_SIZE);
                        }
                    }
                }
            }
            FreeMem(vd, FRAME_SIZE);
        }
    }
}


/*****************************************************************************/


static bool IsAudioCD(Item cdromio, DeviceStatus *ds)
{
    if (ds->ds_DeviceBlockSize == 2352)
    {
        return TRUE;
    }

    return FALSE;
}


/*****************************************************************************/


static bool Is3DOCD(Item cdromio, DeviceStatus *ds)
{
Item file;

    if (ds->ds_DeviceBlockSize == 2048)
    {
        file = OpenDiskFile("$app/LaunchMe");
        if (file >= 0)
        {
            CloseDiskFile(file);
            return TRUE;
        }
    }

    return FALSE;
}


/*****************************************************************************/


static bool IsNoCD(Item cdromio, DeviceStatus *ds)
{
    if ((ds->ds_DeviceFlagWord & CDROM_STATUS_DISC_IN) == 0)
        return TRUE;

    return FALSE;
}


/*****************************************************************************/


static bool IsVideoCD(Item cdromio, DeviceStatus *ds)
{
    if (ds->ds_DeviceBlockSize == 2048)
    {
        CheckISOCD(cdromio);
        return videoCD;
    }

    return FALSE;
}


/*****************************************************************************/


static bool IsPhotoCD(Item cdromio, DeviceStatus *ds)
{
    if (ds->ds_DeviceBlockSize == 2048)
    {
        CheckISOCD(cdromio);
        return photoCD;
    }

    return FALSE;
}


/*****************************************************************************/


typedef struct NavikenLabel
{
    char  nl_Reserved1[9];
    char  nl_VolumeStrings[5];
    uint8 nl_VolumeVersion;
    char  nl_Reserved2;
    char  nl_SystemID[32];
} NavikenLabel;

#define NAVIKEN_LABEL_MINUTES 0
#define NAVIKEN_LABEL_SECONDS 2
#define NAVIKEN_LABEL_OFFSET  16
#define NAVIKEN_LABEL_FRAME   (((NAVIKEN_LABEL_MINUTES * 60 + NAVIKEN_LABEL_SECONDS) * FRAMES_PER_SECOND) + NAVIKEN_LABEL_OFFSET)


static bool IsNavikenCD(Item cdromio, DeviceStatus *ds)
{
NavikenLabel *nl;
bool          result;

    result = FALSE;

    if (ds->ds_DeviceBlockSize == 2048)
    {
        nl = (NavikenLabel *)AllocMem(FRAME_SIZE,MEMTYPE_DMA);
        if (nl)
        {
            if (GetFrames(cdromio,NAVIKEN_LABEL_FRAME,1,nl) >= 0)
            {
                if (strncmp(nl->nl_VolumeStrings,"NSRAJ",5) == 0)
                {
                    if (nl->nl_VolumeVersion == 0x02)
                    {
                        if (strcmp(nl->nl_SystemID,"NAVIGATION SYSTEM") == 0)
                        {
                            result = TRUE;
                        }
                    }
                }
            }
            FreeMem(nl,FRAME_SIZE);
        }
    }

    return result;
}


/*****************************************************************************/


typedef bool (* CheckCDTypeFunc)(Item cdromio, DeviceStatus *ds);

typedef struct CDType
{
    CheckCDTypeFunc  cdt_CheckFunc;
    char            *cdt_ROMApp;
} CDType;

static const CDType cdTypes[] =
{
    {IsNoCD,      "$RunNoCD"},
    {Is3DOCD,     "$app/LaunchMe"},
    {IsAudioCD,   "$RunAudioCD"},
    {IsPhotoCD,   "$RunPhotoCD"},
    {IsVideoCD,   "$RunVideoCD"},
    {IsNavikenCD, "$RunNavikenCD"},
};

#define NUM_CDTYPES     (sizeof(cdTypes) / sizeof(CDType))
#define UnknownCDROMApp "$RunUnknownCD"


/*****************************************************************************/


void LaunchApp(void)
{
Item          cdromitem;
Item          cdromio;
DeviceStatus  ds;
char         *romApp;
uint32        i;

    TRACE(("DOCD: entering\n"));

    cdromitem = OpenNamedDevice("cd-rom",0);
    cdromio = CreateIOReq(0,0,cdromitem,0);
#ifndef ROMBUILD
/*
   Work around compiler bug by not compiling the following empty "if"
   block.  dcp
*/
    if (cdromio < 0)
    {
        PrintError(NULL,"OpenNamedDevice() or CreateIOReq() for","cd-rom",cdromitem);

        /* from this point on, there's no CD, so we'll end up just
         * running $RunNoCD for the rest of eternity.
         */
    }
#endif

    while (TRUE)
    {
#ifdef DEVELOPMENT
        romApp = cdTypes[0].cdt_ROMApp;
#else
	if ((KernelBase->kb_CPUFlags & KB_NODBGR) == 0)
	{
	    /* If we're in the production shell, and there is a debugger,
	     * don't try to run NoCD, and skip directly to LaunchMe. This is
	     * in support of catapult stuff.
	     */
            romApp = cdTypes[1].cdt_ROMApp;
	}
        else
        {
            romApp = cdTypes[0].cdt_ROMApp;
        }
#endif

        if (GetCDStatus(cdromio,&ds) >= 0)
        {
            romApp = UnknownCDROMApp;
            for (i = 0; i < NUM_CDTYPES; i++)
            {
                if ((*cdTypes[i].cdt_CheckFunc)(cdromio,&ds))
                {
                    romApp = cdTypes[i].cdt_ROMApp;
                    break;
                }
            }
        }

        ExecuteString(romApp);
    }
}
