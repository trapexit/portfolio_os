/* $Id: sportdev.c,v 1.12 1994/08/26 01:33:40 ewhac Exp $ */

/* *************************************************************************
 *
 * SPORT Device routines for the Opera Hardware
 *
 * Copyright (C) 1992, New Technologies Group, Inc.
 * NTG Trade Secrets  -  Confidential and Proprietary
 *
 * DATE   NAME             DESCRIPTION
 * ------ ---------------- -------------------------------------------------
 * 930617 SHL              Removed assembler wrapper around FIRQ handler
 * 930211 -RJ              Changed the AbortSPORTIO() routine to work with
 *                         IOReq's the new way
 * 920724 -RJ Mical        Start overhaul
 * 920717 Stephen Landrum  Last edits before July handoff
 *
 * ********************************************************************** */


#include "types.h"

#include "debug.h"
#include "item.h"
#include "nodes.h"
#include "interrupts.h"
#include "kernel.h"

#define SUPER
#include "mem.h"
#undef SUPER

#include "list.h"
#include "task.h"
#include "folio.h"
#include "kernelnodes.h"
#include "operror.h"

#include "driver.h"
#include "device.h"
#include "io.h"

#include "graphics.h"
#include "inthard.h"

#include "super.h"
/*#include "stdio.h" */

#include "vbl.h"


#define MINSPORTVCOUNT	10
#define MAXSPORTVCOUNT	13

#define SDEBUGSPORT(x)	/*Superkprintf x*/
#define KDBUG(x) /*kprintf x*/
#define INFO(x)	Superkprintf x

/* this should go into inthard.h */
#define SVF_WCR        0x03202000
#define SVF_FW         0x03204000

extern int32	copypages (vuint32 *dest, vuint32 *src, int32 npages, uint32 mask);
extern int32	clonepages (vuint32 *dest, vuint32 *src, int32 npages, uint32 mask);
extern void	SPORTFirqC(void);

static void	tryopengraf(void);


static int32	defaultsportlines[] = {
	MINSPORTVCOUNT,
	MAXSPORTVCOUNT
};

int32	*sportlines = defaultsportlines;


List SPORTCmdList;	/* Where we put ioreq waiting for time delay */
MemHdr *vram;		/* pts to systems MemHdr for VRAM */
int32 vrampage;		/* vram page size */
Item gfirqItem;

struct GrafFolio	*GrafBase;
Item			GrafFolioNum;


static void AbortSPORTIO(IOReq *ior)
{
	Node *n = (Node *)ior;
	RemNode( n );
	ior->io_Error = ABORTED;
	SuperCompleteIO( ior );
}


TagArg SPORTFirqTags[] =
	{
	TAG_ITEM_PRI,	(void *)0,
	TAG_ITEM_NAME,	(void *)"SPORT FIRQ",
	CREATEFIRQ_TAG_CODE,	(void *)((long)SPORTFirqC),
	CREATEFIRQ_TAG_NUM,	(void *)INT_V1,
	TAG_ITEM_END,	(void *)0,
	};


void SPORTFirqC(void)
/* called by interrupt routine */
{
	IOReq	*ior;
	vuint32	*sport_src, *sport_dst;
	uint32	i, n, m;
	uint8	pageshift;

	ior = (IOReq *)FIRSTNODE(&SPORTCmdList);
	if (NEXTNODE (ior)) {
		/* busy wait until proper line (this is why this firq is at pri 0) */
		pageshift = vram->memh_VRAMPageShift;
		while (((*VCNT & VCNT_MASK) >> VCNT_SHIFT) < sportlines[0])
		    ;
		while (ior = (IOReq *) RemHead (&SPORTCmdList)) {
		    if (((*VCNT & VCNT_MASK) >> VCNT_SHIFT) >= sportlines[1])
		    {
			/* oops, out of time, the rest will have to wait */
			/* we could try to do as many as we can in another */
			/* loop that checks the time and then does the rest */
			/* at the next vertical blank */
			/*
			 * There's also a window of vulnerability here, where
			 * the next transfer may be so large as to overflow
			 * available time.  The current line really needs
			 * checked in the inner loop.  Next pass...  9408.11
			 */
			/*
			 * Window removed (I think).  9408.24
			 */
			AddHead (&SPORTCmdList, (Node *) ior);
			break;
		    }

		    sport_src = SPORT+ior->io_Extension[0];
		    sport_dst = SPORT+ior->io_Extension[1];
		    i = ior->io_Info.ioi_Recv.iob_Len >> pageshift;
		    m = ior->io_Info.ioi_Offset;
		    n = 0;

		    switch( ior->io_Info.ioi_Command ) {
		        case SPORTCMD_CLONE:
#if 0
			    j = *sport_src; /* j = SPORT[s]; */
/*			    for ( ; i > 0; i -= lv) {} */
			    while(i--) {
				*sport_dst++ = m; /* SPORT[d++] = m; */
			    }
			    j = *sport_src; /*    j = SPORT[s]; */
#endif
		            n = clonepages (sport_dst, sport_src, i, m);
			    break;
			case SPORTCMD_COPY:
#if 0
/*			    for ( ; i  > 0; i -= lv ) {} */
			    while(i--) {
				j = *sport_src++; /* j = SPORT[s++]; */
				*sport_dst++ = m; /* SPORT[d++] = m; */
			    }
			    j = *(sport_src-1); /* j = SPORT[s-1]; */
#endif
			    n = copypages (sport_dst, sport_src, i, m);
			    break;
			default:
			    SDEBUGSPORT( ("Invalid SPORT transfer queued command (%d)\n", ior->io_Info.ioi_Command));
			    /*for (;;) {}*/
		    }
		    if (n) {
		    	/*
		    	 * Couldn't finish operation.  Recalculate number of
		    	 * pages to do and push back on queue.
		    	 */
		    	ior->io_Info.ioi_Recv.iob_Len = n << pageshift;
		    	ior->io_Extension[1] += i - n;	/*  Update dest  */
		    	if (ior->io_Info.ioi_Command == SPORTCMD_COPY)
		    		ior->io_Extension[0] += i - n; /*Update src*/
			AddHead (&SPORTCmdList, (Node *) ior);
		    	break;
		    }

		    SuperCompleteIO (ior);
		}
	}
}


static Item
SPORTInit(Driver *dev)
{
	List *l = KernelBase->kb_MemHdrList;
	MemHdr *m;

	SDEBUGSPORT (("Entering SPORTInit\n"));

        vrampage = GetPageSize(MEMTYPE_VRAM);
	InitList(&SPORTCmdList,"SPORT Queue");
	for (m = (MemHdr *)FIRSTNODE(l); ISNODE(l,m); m = (MemHdr *)NEXTNODE(m) ) {
		if (m->memh_Types & MEMTYPE_VRAM) break;
	}
	if (ISNODE(l,m) == 0) {
		SDEBUGSPORT(("SportInit fail, could not find VRAM\n"));
		return NOMEM;
	}

	/* Found VRAM description header, save here */
	vram = m;
	SDEBUGSPORT(("vram hdr=%lx\n",vram));

	gfirqItem = SuperCreateItem(MKNODEID(KERNELNODE,FIRQNODE),SPORTFirqTags);
	SDEBUGSPORT(("gfirItem=%lx\n",gfirqItem));
	if (gfirqItem < 0) return gfirqItem;
	return dev->drv.n_Item;
}

static int32
NoCmd (ior)
IOReq *ior;
{
	ior->io_Error = BADCOMMAND;
	return( 1 );
}


static int32
QueueSPORTCmd (ior)
IOReq *ior;
{
    int32 s1,d1,dl1,com;
    uint32 intSave;

    if (!GrafBase)
	tryopengraf ();

    /* src len (sl1) is not used, only dst len is used */
    s1 = (long)ior->io_Info.ioi_Send.iob_Buffer;
    d1 = (long)ior->io_Info.ioi_Recv.iob_Buffer;
    dl1 = (long)ior->io_Info.ioi_Recv.iob_Len;
    com = (long)ior->io_Info.ioi_Command;


    /* Make sure src and dst are inside of VRAM */
#if 0
    if  ( (s1 < (long)vram->memh_MemBase) ||		/* src below vram? */
        (d1 < (long)vram->memh_MemBase) ||		/* dst below vram? */
	((s1+dl1) > (long)vram->memh_MemTop) ||		/* srcend above vram? */
	((d1+dl1) > (long)vram->memh_MemTop) ||		/* dstend above vram? */
	( (s1|d1|dl1) & vram->memh_VRAMPageMask) )	/* aligned properly? */
#endif
    if( ((d1+dl1) > (long)vram->memh_MemTop) ||		/* dstend above vram? */
      (d1 < (long)vram->memh_MemBase) ||		/* dst below vram? */
      ( (s1|d1|dl1) & vram->memh_VRAMPageMask) ||	/* aligned properly? */
      ( (com != FLASHWRITE_CMD) && (s1 < (long)vram->memh_MemBase)) || /* src below vram? */
      ( (com == SPORTCMD_COPY) && ((s1+dl1) > (long)vram->memh_MemTop) ) ||	/* srcend above vram? */
      ( (com == SPORTCMD_CLONE) && ((s1+vrampage) > (long)vram->memh_MemTop) ))	/* srcend above vram? */
	{
		ior->io_Error = BADIOARG;
		INFO(("QueueSport error: bad args on cmd %ld\n",com));
		INFO(("%lx,%lx\n",s1,d1));
		INFO(("%lx,%lx\n",dl1,vram->memh_VRAMPageMask));
		return( 1 );
	}


	 /* Check for Blue same memory bank */
	 /* this is not a good way to test for memory bank */
	if ((com != FLASHWRITE_CMD) &&
	     ( ((s1&0x700000) != (d1&0x700000)) ||
	       ((com == SPORTCMD_COPY) && (((s1+dl1)&0x700000) != ((d1+dl1)&0x700000))) ||
	       ((com == SPORTCMD_CLONE) && (((s1+vrampage)&0x700000) != ((d1+dl1)&0x700000)))))
	{
		INFO(("QueueSport error on cmd %ld: xfer across 1M boundary\n",com));
		INFO(("s1=%lx d1=%lx len=%lx\n",s1,d1,dl1));
		ior->io_Error = BADIOARG;
		return( 1 );
	}

	/* precompute these values now so we don't have to do it at interrupt time */
	/* normalize to beginning of VRAM */
	s1 -= (long)vram->memh_MemBase;
	d1 -= (long)vram->memh_MemBase;


	/* At this point, Flash can be done IMMEDIATELY, NOT que'd till VBLANK*/
	if (com == FLASHWRITE_CMD) {
		volatile register int32 *FWP;
		register int32 mask, num_pages;
		int32 FWdest, v, *colreg;
		uint8 pageshift;

		/* Currently, shift is 11, for 2K byte pages. */
		pageshift = vram->memh_VRAMPageShift;
		FWdest = d1 >> pageshift-2;
		FWP = (int32 *)(SVF_FW|FWdest);

		mask = ior->io_Info.ioi_CmdOptions;

		v = (int32)ior->io_Info.ioi_Offset;
		colreg = (int32 *)SVF_WCR;
		*colreg = v; /* Load Write Color Register */

/*		num_pages = dl1 / vrampage; */
		num_pages = dl1 >> pageshift;

		/* critical, page at a time write loop */
		while (num_pages--)*FWP++ = mask;
		return(1);
	}

	s1 >>= vram->memh_VRAMPageShift;
	d1 >>= vram->memh_VRAMPageShift;

	/* save them here for the interrupt handler */
	ior->io_Extension[0] = s1;
	ior->io_Extension[1] = d1;

	intSave = Disable ();
	ior->io_Flags &= ~IO_QUICK;
	AddTail(&SPORTCmdList,(Node *)ior);
	Enable (intSave);
	return( 0 );
}


static void
tryopengraf ()
{
	static TagArg vbltags[] = {
		VBL_TAG_REPORTSPORTLINES,	&sportlines,
		TAG_END,			0
	};

	if ((GrafFolioNum =
	      SuperOpenItem (SuperFindNamedItem (MKNODEID (KERNELNODE,
							   FOLIONODE),
						 "Graphics"),
			     0)) < 0)
		return;

	GrafBase = (GrafFolio *) LookupItem (GrafFolioNum);

	if (SuperInternalGetVBLAttrs (vbltags) < 0)
		sportlines = defaultsportlines;  /* In case it got stomped */

	SDEBUGSPORT (("Sportlines are: %d - %d\n",
		      sportlines[0], sportlines[1]));
}



static int32 (*SPORTCmdTable[])() =
{
	NoCmd,		/* CMD_WRITE	*/
	NoCmd,		/* CMD_READ	*/
	NoCmd,		/* CMD_STATUS	*/
	NoCmd,		/* ???		*/
	QueueSPORTCmd,	/* Copy pages command */
	QueueSPORTCmd,	/* Clone pages command */
	QueueSPORTCmd,	/* Clone pages command */
};

static TagArg drvrArgs[] =
{
	TAG_ITEM_PRI,		(void *)1,
	TAG_ITEM_NAME,	"SPORT",
	CREATEDRIVER_TAG_ABORTIO,	(void *)((long)AbortSPORTIO),
	CREATEDRIVER_TAG_MAXCMDS,	(void *)(sizeof(SPORTCmdTable)/sizeof(SPORTCmdTable[0])),
	CREATEDRIVER_TAG_CMDTABLE,	(void *)SPORTCmdTable,
	CREATEDRIVER_TAG_INIT,	(void *)((long)SPORTInit),
	TAG_ITEM_END,		0,
};

static TagArg devArgs[] =
{
	TAG_ITEM_PRI,		(void *)3,
	CREATEDEVICE_TAG_DRVR,	(void *)1,
	TAG_ITEM_NAME,	"SPORT",
	TAG_ITEM_END,		0,
};

Item
createSPORTDriver(void)
{
	Item devItem;
	Item drvrItem;

	KDBUG(("In createSportDriver\n"));

	drvrItem = CreateItem(MKNODEID(KERNELNODE,DRIVERNODE),drvrArgs);
	if (drvrItem < 0)
	{
	    KDBUG(("Creating SPORT driver returns drvrItem=%lx\n",drvrItem));
	    return (drvrItem);			/* 9306.17 SHL */
	}
	devArgs[1].ta_Arg = (void *)drvrItem;
	devItem = CreateItem(MKNODEID(KERNELNODE,DEVICENODE),devArgs);
	KDBUG(("Creating SPORT device returns devItem=%d\n",devItem));

	return( devItem );
}




