/* $Id: mmu.c,v 1.9 1994/02/09 01:22:45 limes Exp $ */

extern void fuckarmcc(void);	/* Why do I need this here? */


#ifdef ARM600

#include "types.h"
#include "kernel.h"
#include "internalf.h"
#include "mem.h"
#include "task.h"
#include "mmu.h"
#include "strings.h"

extern void printf(char *fmt, ...);

extern void mmuTLBFlush(void);
extern void LoadFenceBits(Task *);
extern uint32 mmuReadFaultStatus(void);
extern uint32 mmuReadFaultAddress(void);
extern void mmuWriteControl(uint32);
extern void mmuWriteDomainAccessControl(uint32);
extern void mmuWriteTranslationTable(uint8 *);

extern int32 Arm600;
extern uint32 myMSysMemBits;

/* ST_ADDR must be on a 16K boundary according to arm manual */
/* We are leaving 16k of memory at top of SRAM for debugger */
#define ST_ADDR	((256-32)*1024)		/* just below debugger */
#define ST_SIZE 4096	/* size in int32s */

#define PS	4096		/* page size */
#define PS_SHIFT	12
#define MEG	(1024*1024)
#define VRAM_MEG	16
#define VRAM_START	(VRAM_MEG*MEG)

#define STATIC_PT	/* define this to use SRAM for Page Tables */
		/* enough for 1M, how convenient */
#define PT_SIZE	(2*256)	/* 256 entries */

/* maximum number of 16 tasks supported */
#define PTS_MAX		16
#define PTS_SIZE	(PTS_MAX*PT_SIZE*sizeof(int32))
#define PTS_BASE	(ST_ADDR-PTS_SIZE)

#ifdef undef
uint32 kernel_pt[8];	/* first 32k of VRAM */
#endif

extern char *Ptr_Addr_SRAM;
#define SRAM_MEG	((uint32)Ptr_Addr_SRAM>>20)

#define DBUG(x)	/*printf x*/

void
ComputePageTable(t)
Task *t;
{
	/* need to process each of the MemList hdrs for this task */
	/* For now we just look at the first MemList */
	MemList *ml;
	int32 i;
	uint32 *pt = t->t_PageTable;

	DBUG(("ComputePageTable(0x%lx) pt=%lx\n",t,pt));

    for (ml = (MemList *)FIRSTNODE(t->t_FreeMemoryLists) ;
	ISNODE(t->t_FreeMemoryLists,ml) ;
		ml = (MemList *)NEXTNODE(ml) )
    {
	MemHdr *mh = ml->meml_MemHdr;
	uint32 entry =  (uint32)mh->memh_MemBase | 2;	/* small pages */
	pd_set *pbits = (pd_set *)ml->meml_WriteBits;
	DBUG(("ml=%lx mh=%lx pbits=%lx",ml,mh,pbits));
	DBUG((" name=%s\n",mh->memh_n.n_Name));
	if (pt)
	{
	    for (i = 0; i < 256; i++)
	    {
		if (PD_ISSET(i>>1,pbits))	*pt++ = entry | 0xff0; /* read/write */
		else			*pt++ = entry | 0xaa0; /* read only */
		entry += PS;
	    }
	}
    }
}

uint32 *
AllocatePageTable(void)
{
	/* MEMTYPE_STARTPAGE is actually more than needed */
	/* all we need really is to have it start on a 1k page */
	/* boundary */
	uint32 *pt;
#ifdef STATIC_PT
	/* search for available page table */
	/* in static ram */
	int32 i;
	pt = (uint32 *)PTS_BASE;
	for (i = 0;
		i < PTS_MAX;
			i++)
	{
		if (*pt == 0)
		{
		    /* set up default VRAM */
		    /*memcpy(pt+256,kernel_pt,sizeof(kernel_pt));*/
		    return pt;
		}
		pt += PT_SIZE;
	}
	pt = 0;
#else
	pt = ALLOCMEM(PT_SIZE * sizeof(uint32),MEMTYPE_STARTPAGE);
	if (pt) memcpy(pt,kernel_pt,sizeof(kernel_pt));
#endif
	/* initialize first 8 entries (32k) */
	return pt;
}

void
FreePageTable(t)
Task *t;
{
	uint32 *pt = t->t_PageTable;
	uint32 *st = (uint32 *)(Ptr_Addr_SRAM + ST_ADDR);
	if (pt)
	{
	    if (t == KernelBase->kb_CurrentTask)
	    {
		st[VRAM_MEG] = VRAM_START | 0x802; /* set VRAM meg to default */
		st[SRAM_MEG] = (int32)Ptr_Addr_SRAM | 0x802; /* set SRAM meg to default */
		mmuTLBFlush();
	    }
#ifdef STATIC_PT
	    *pt = 0;
#else
		FREEMEM(pt,PT_SIZE*sizeof(uint32));
#endif
	}
}

/* fine control of memory access priveleges */

uint8 mem_ctrl_table[64] =
{
	1,	/* 0x0000000 1M of SRAM, (really only 256k) */
	0,0,0,	/* the next 15M are off limits */
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	/* */
	1,	/* 0x1000000 1M of VRAM ok */
	0,0,0,	/* the next 15M are off limits */
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	/* */
	1,	/* 0x2000000 1M ROM, Joystick, some unused */
	0,	/* 0x2100000 1M unused */
	1,	/* 0x2200000 1M MAC, PLAYER, unused, PHYS SRAM, Unused */
	1,	/* 0x2300000 1M Hardware registers */
	1,	/* 0x2400000 1M Sport access */
	0,0,0,	/* 0x2500000 27M unused */
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
};

void
InitMMU(RO_size)
int32 RO_size;
{
	uint32 *pt = (uint32 *)(Ptr_Addr_SRAM + ST_ADDR);
	uint8 *ok = mem_ctrl_table;
	int32 i;
	int32 entry;
#ifdef undef
	uint32 mask;
#endif

	/* set up first level table with 1M sections */
	/* super r/w, user r, section */
	/* nothing cached, buffered, or updatable */
	/* disable all accesses first */
	DBUG(("st=%lx size=%d\n",(long)pt,ST_SIZE*sizeof(uint32)));
	memset(pt,0,ST_SIZE*sizeof(uint32));
	entry = 0x802;
	pt = (uint32 *)(Ptr_Addr_SRAM + ST_ADDR);
	/* Selectively enable the pieces we need */
	for (i = 0; i < 64 ; i++)
	{
	    if (*ok++)	*pt = entry;
	    pt++;
	    entry += MEG;	/* next physical address */
	}
	/* Initialize PageTable array */
	memset((uint8 *)PTS_BASE,0,PTS_SIZE);
	mmuWriteTranslationTable(Ptr_Addr_SRAM + ST_ADDR);
	mmuWriteDomainAccessControl(0x55555555);	/* all client */
	mmuWriteControl(MMU_STANDARD|MMU_MMUON);
	/* set up default 32k base of VRAM */
	entry = VRAM_START | 2 | 0xaa0;
#ifdef undef
	pt = kernel_pt;
	/* set up default first 32k is r/w super, r-only user */
	for (i = 0; i < 8 ; i++)
	{
	    *pt++ = entry;
	    entry += PS;
	}
#endif
#ifdef undef
	/* can't do this because of stupid arm600 mmu */
	/* does not allow readonly for super and user concurrently */
	/* now make super-read only as much code as we can */
	i = 0;
	pt = kernel_pt;
	mask = 0x30;
	RO_size -= 1024;
	while (RO_size>0)
	{
	    *pt &= ~mask;	/* make super read only */
	    mask <<= 2;
	    i++;
	    if (i == 4)
	    {
		mask = 0x30;
		i = 0;
		pt++;	/* bump to next 4k descriptor */
	    }
	    RO_size -= 1024;
	}
#endif
}

void
NewPageTable(pt)
uint32 *pt;
{
	uint32 *st = (uint32 *)(Ptr_Addr_SRAM + ST_ADDR);
	/*printf("Setting New Page Table pt=%lx st=%lx\n",pt,st);*/
	/*if ((int32)pt == 0x80000010)	while (1);*/
	st[SRAM_MEG] = (uint32)pt | 0x001;	/* set second meg translation */
	st[VRAM_MEG] = (uint32)(pt+256) | 0x001;
	mmuTLBFlush();
}

void
dumpmmu(void)
{
	int32 stat = mmuReadFaultStatus();
	int32 addr = mmuReadFaultAddress();
	printf("Fault Status = %lx Address= %lx\n",stat,addr);
}

#endif
