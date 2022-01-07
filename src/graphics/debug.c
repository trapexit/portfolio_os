/* $Id: debug.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */

/* *************************************************************************
 *
 * Debugging routines for the Opera Hardware
 *
 * Copyright (C) 1992, New Technologies Group, Inc.
 * NTG Trade Secrets  -  Confidential and Proprietary
 *
 * The contents of this file were designed with tab stops of 4 in mind
 *
 * DATE   NAME             DESCRIPTION
 * ------ ---------------- -------------------------------------------------
 * 921010 -RJ Mical        Created this file!
 *
 * ********************************************************************** */


/***************************************************************\
* Header files                                                 *
\***************************************************************/


#define INTERNALDEBUG
#define NOERASE

#include "types.h"

/*???#include "stdlib.h"*/

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
#include "super.h"

#include "intgraf.h"

#include "stdarg.h"
#include "strings.h"




void printgraffolio()
{
/*???	SDEBUGVDL(("GRAFFOLIO:(%x,0x%x) vdl5:0x%x ",*/
/*???  		GrafBase->gf.fn.n_Item,GrafBase,GrafBase->gf_VDL5));*/
/*???		*/
/*???	SDEBUGVDL(("vdl20:0x%x vdlblank:0x%x nullval:0x%x\n",*/
/*???		GrafBase->gf_VDL20,GrafBase->gf_VDLBlank,GrafBase->gf_NullVDLValue));*/
/*???*/
/*???	*/
/*???	SDEBUGVDL(("  switch %d lastswitch %d ",*/
/*???		GrafBase->gf_VDLSwitch,GrafBase->gf_LastVDLSwitch));*/
/*???		*/
/*???	if(EMPTYLIST(GrafBase->gf_ScreenGroupListPtr))*/
/*???	{*/
/*???		SDEBUGVDL(("ScreenGroupList:Empty "));*/
/*???	}*/
/*???	else*/
/*???	{*/
/*???		SDEBUGVDL(("ScreenGroupList:0x%x ",*/
/*???				FIRSTNODE(GrafBase->gf_ScreenGroupListPtr)));*/
/*???	}*/
/*???	*/
/*???	if(EMPTYLIST(GrafBase->gf_ScreenListPtr))*/
/*???	{*/
/*???		SDEBUGVDL(("ScreenList:Empty\n"));*/
/*???	}*/
/*???	else*/
/*???	{*/
/*???		SDEBUGVDL(("ScreenList:0x%x\n",*/
/*???				FIRSTNODE(GrafBase->gf_ScreenListPtr)));*/
/*???	}*/
/*???	SDEBUGVDL(("  firstdisplays:0x%x,0x%x ",*/
/*???		GrafBase->gf_FirstDisplay[0],GrafBase->gf_FirstDisplay[1]));*/
/*???		*/
/*???	SDEBUGVDL(("lastblanks:0x%x,0x%x\n",*/
/*???		GrafBase->gf_LastBlankStart[0],GrafBase->gf_LastBlankStart[1]));*/
/*???		*/
}

void printscreengroup(Item i)
{
/*???	ScreenGroup *sg;*/
/*???*/
/*???	sg = (ScreenGroup *)LocateItem(i);*/
/*???		*/
/*???	if(!sg)*/
/*???	{*/
/*???		SDEBUGVDL(("SCREENGROUP: bad item %x\n",i));*/
/*???		return;*/
/*???	}*/
/*???	SDEBUGVDL(("SCREENGROUP:(%x,0x%x) nextnode:0x%x ",*/
/*???		sg->sg.n_Item,sg,NEXTNODE(sg)));*/
/*???		*/
/*???	SDEBUGVDL(("Y:0x%x maxh:0x%x currenth:0x%x\n",*/
/*???		sg->sg_Y,sg->sg_MaxHeight,sg->sg_CurrentHeight));*/
/*???*/
/*???	SDEBUGVDL(("  nextdisp:0x%x 0x%x ",*/
/*???		sg->sg_NextDisplay[0],sg->sg_NextDisplay[1]));*/
/*???		*/
/*???	SDEBUGVDL(("evscr:0x%x odscr:0x%x\n",*/
/*???		sg->sg_ScreenPtr[0],sg->sg_ScreenPtr[1]));*/
/*???		*/
/*???	SDEBUGVDL(("  0 evvdl:0x%x odvdl:0x%x",*/
/*???		sg->sg_VDLPtr[0][0],sg->sg_VDLPtr[0][1]));*/
/*???*/
/*???	SDEBUGVDL(("  1 evvdl:0x%x odvdl:0x%x\n",*/
/*???		sg->sg_VDLPtr[1][0],sg->sg_VDLPtr[1][1]));*/
/*???*/
/*???	SDEBUGVDL(("  invdl:0x%x blankst:0x%x 0x%x ",*/
/*???		sg->sg_VDLInUse,sg->sg_BlankStart[0],sg->sg_BlankStart[1]));*/
/*???		*/
/*???	SDEBUGVDL(("blankpat:(0x%x,0x%x),",*/
/*???		sg->sg_BlankPatchPtr[0],sg->sg_BlankPatchDMACtrl[0]));*/
/*???		*/
/*???	SDEBUGVDL(("(0x%x,0x%x)\n",*/
/*???		sg->sg_BlankPatchPtr[1],sg->sg_BlankPatchDMACtrl[1]));*/
/*???*/
/*???	SDEBUGVDL(("  pat 0e(0x%x,0x%x) ",*/
/*???		sg->sg_PatchPtr[0][0],sg->sg_PatchDMACtrl[0][0]));*/
/*???*/
/*???	SDEBUGVDL(("o(0x%x,0x%x) ",*/
/*???		sg->sg_PatchPtr[0][1],sg->sg_PatchDMACtrl[0][1]));*/
/*???		*/
/*???	SDEBUGVDL(("1e(0x%x,0x%x) ",*/
/*???		sg->sg_PatchPtr[1][0],sg->sg_PatchDMACtrl[1][0]));*/
/*???		*/
/*???	SDEBUGVDL(("o(0x%x,0x%x)\n",*/
/*???		*/
/*???		sg->sg_PatchPtr[1][1],sg->sg_PatchDMACtrl[1][1]));*/
/*???		*/
}
	
void printscreen(Item i)
{
/*???	Screen *scr = (Screen *)LocateItem(i);*/
/*???	*/
/*???	if(!scr)*/
/*???	{*/
/*???		SDEBUGVDL(("SCREEN: bad item %x\n",i));*/
/*???		return;*/
/*???	}*/
/*???	SDEBUGVDL(("SCREEN:(%x,0x%x) next:0x%x ",*/
/*???		scr->scr.n_Item,scr,NEXTNODE(scr)));*/
/*???		*/
/*???	SDEBUGVDL(("parent:0x%x vdl:0x%xx\n",*/
/*???		scr->scr_ScreenGroupPtr,scr->scr_VDLPtr));*/
}

void printvdl(Item i)
{
/*???	VDL *vdl = (VDL *)LocateItem(i);*/
/*???	if(!vdl)*/
/*???	{*/
/*???		SDEBUGVDL(("VDL: bad item %x\n",i));*/
/*???		return;*/
/*???	}*/
/*???	*/
/*???	SDEBUGVDL(("VDL:(%x,0x%x) ",*/
/*???		vdl->vdl.n_Item,vdl));*/
/*???		*/
/*???	SDEBUGVDL(("screen:0x%x data:0x%x\n",*/
/*???			vdl->vdl_ScreenPtr,vdl->vdl_DataPtr));*/
}

