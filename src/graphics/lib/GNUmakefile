# $Id: GNUmakefile,v 1.7 1994/09/24 03:34:54 ewhac Exp $
# GNUmakefile for the Graphics folio link library.
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All Rights Reserved.
# This document is proprietary and confidential

default: all


###############################################################################
# Describe our files
#
LOCAL_INCS     =

INCS_HERE      =

LOCAL_INCLUDES = ../includes

LIB_DATE       = builddate

BLIBS          = graphics.blib
DLIBS          = graphics.dlib
RLIBS          = graphics.rlib

###############################################################################
# Stuff for the link library...
#
graflib_SSRC   = fastmapcel.s supergrafdo.s supervectors.s
graflib_CSRC   = addscreengroup.c clonevram.c clonevramdefer.c \
		 closefolio.c copyvram.c copyvramdefer.c createscreengroup.c \
		 createbitmapva.c createvdlva.c deletescreengroup.c \
		 disablehavg.c disablevavg.c displayoverlay.c \
		 displayscreen.c drawcels.c drawchar.c drawscreencels.c \
		 drawtext16.c drawtext8.c drawto.c enablehavg.c enablevavg.c \
		 fillrect.c getcurrentfont.c getfirstdisplayinfo.c \
		 getpixeladdress.c getvblattrs.c getvblioreq.c \
		 getvramioreq.c mapcel.c modifyvdl.c moveto.c openfolio.c \
		 querygraphics.c querygraphicslist.c readclutcolor.c \
		 readpixel.c removescreengroup.c resetcurrentfont.c \
		 resetreadaddress.c resetscreencolors.c setbgpen.c \
		 setcecontrol.c setcewatchdog.c setclipheight.c \
		 setcliporigin.c setclipwidth.c setcurrentfontccb.c \
		 setfgpen.c setreadaddress.c setscreencolor.c \
		 setscreencolors.c setvblattrs.c setvdl.c setvram.c \
		 setvramdefer.c submitvdl.c waitvbl.c waitvbldefer.c \
		 writepixel.c

graphics.blib_OBJS = ${graflib_CSRC:%.c=%.bo} ${graflib_SSRC:%.s=%.bo}
graphics.dlib_OBJS = ${graflib_CSRC:%.c=%.do} ${graflib_SSRC:%.s=%.do}
graphics.rlib_OBJS = ${graflib_CSRC:%.c=%.ro} ${graflib_SSRC:%.s=%.ro}

graphics.blib: ${graphics.blib_OBJS}
	       ${linklib}

graphics.dlib: ${graphics.dlib_OBJS}
	       ${linklib}

graphics.rlib: ${graphics.rlib_OBJS}
	       ${linklib}


###############################################################################
#
#
RCSFILES   = ${graflib_CSRC} ${graflib_SSRC}

AUTODEPEND = ${graphics.dlib_OBJS} ${graphics.rlib_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
