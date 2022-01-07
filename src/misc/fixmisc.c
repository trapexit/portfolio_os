/* $Id: fixmisc.c,v 1.2 1994/10/17 19:51:50 limes Exp $ */

#include <unistd.h>
#include <fcntl.h>

typedef unsigned long	uint32;
typedef long		int32;
typedef unsigned long	Item;
typedef unsigned char	uint8;

typedef struct AIFHeader {
    uint32	    aif_blDecompress;	/* NOP if image not compressed */
    uint32	    aif_blSelfReloc;	/* NOP if image not self-relocating */
    uint32	    aif_blZeroInit;	/* NOP if image has none */
    uint32	    aif_blEntry;
    uint32	    aif_SWIexit;	/* in case of return from main() */
    int32	    aif_ImageROsize;	/* includes header */
    int32	    aif_ImageRWsize;	/* exact size */
    int32	    aif_DebugSize;	/* exact size */
    int32	    aif_ZeroInitSize;	/* exact size, (right...) */
    int32	    aif_ImageDebugType; /* 0, 1, 2, or 3 */
    uint32	    aif_ImageBase;	/* addr image linked at */
    int32	    aif_WorkSpace;	/* initial stack size recommended */
    uint32	    aif_AddressMode;
    uint32	    aif_DataBaseAddr;	/* Addr image data linked at */
    uint32	    aif_Reserved[2];
    uint32	    aif_DebugInit;	/* NOP if unused */
    uint32	    aif_ZeroInitCode[15];
}		AIFHeader;


typedef struct ItemNode {
    struct ItemNode   *n_Next;	/* pointer to next itemnode in list */
    struct ItemNode   *n_Prev;	/* pointer to previous itemnode in list */
    uint8	    n_SubsysType;	/* what folio manages this node */
    uint8	    n_Type;	/* what type of node for the folio */
    uint8	    n_Priority; /* queueing priority */
    uint8	    n_Flags;	/* misc flags, see below */
    int32	    n_Size;	/* total size of node including hdr */
    char	   *n_Name;	/* ptr to null terminated string or NULL */
    uint8	    n_Version;	/* version of of this itemnode */
    uint8	    n_Revision; /* revision of this itemnode */
    uint8	    n_FolioFlags;	/* flags for this item's folio */
    uint8	    n_ItemFlags;/* additional system item flags */
    Item	    n_Item;	/* ItemNumber for this data structure */
    Item	    n_Owner;	/* creator, present owner, disposer */
    void	   *n_ReservedP;/* Reserved pointer */
}		ItemNode, *ItemNodeP;

typedef struct _3DOBinHeader {
    ItemNode	    _3DO_Item;
    uint8	    _3DO_Flags;
    uint8	    _3DO_OS_Version;	/* compiled for this OS release */
    uint8	    _3DO_OS_Revision;
    uint8	    _3DO_Reserved;
    uint32	    _3DO_Stack; /* stack requirements */
    uint32	    _3DO_FreeSpace;	/* preallocate bytes for FreeList */
    uint32	    _3DO_Signature;	/* if privileged, offset to beginning of sig */
    uint32	    _3DO_SignatureLen;	/* length of signature */
    uint32	    _3DO_MaxUSecs;	/* max usecs before task switch */
    uint32	    _3DO_Reserved0;	/* must be zero */
    char	    _3DO_Name[32];	/* optional task name on startup */
    uint32	    _3DO_Time;	/* seconds since 1/1/93 00:00:00 GMT */
    uint32	    _3DO_Reserved1[7];	/* must be zero */
}		_3DOBinHeader;


typedef struct {
    AIFHeader		    aif;
    _3DOBinHeader	    thdo;
}			filehead;


filehead		buf;

main ()
{
    int                     fd;

    fd = open ("misc", O_RDWR);
    read (fd, &buf, sizeof buf);
    buf.aif.aif_WorkSpace = 0;
    lseek(fd, 0L, SEEK_SET);
    write (fd, &buf, sizeof buf);
    return 0;
}
