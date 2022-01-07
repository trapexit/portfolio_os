# $Id: GNUmakefile,v 1.3 1994/09/20 21:58:50 vertex Exp $
# GNUmakefile for the file folio link library.
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All Rights Reserved.
# This document is proprietary and confidential

default: all


###############################################################################
# Describe our files
#
LOCAL_INCS     = filefolio_lib.h

INCS_HERE      =

LOCAL_INCLUDES = ../includes

LIB_DATE       = builddate

BLIBS          = filesystem.blib
DLIBS          = filesystem.dlib
RLIBS          = filesystem.rlib


###############################################################################
# Stuff for the link library...
#
filelib_SSRC   =
filelib_CSRC   = closedirectory.c closediskstream.c closefolio.c\
		 executeassubroutine.c executeasthread.c getfilefolio.c\
		 initfilefolioglue.c loadcode.c loadprogram.c\
		 loadprogramprio.c opendirectoryitem.c opendirectorypath.c\
		 opendiskstream.c openfolio.c readdirectory.c\
		 readdiskstream.c seekdiskstream.c unloadcode.c

filesystem.blib_OBJS = ${filelib_CSRC:%.c=%.bo} ${filelib_SSRC:%.s=%.bo}
filesystem.dlib_OBJS = ${filelib_CSRC:%.c=%.do} ${filelib_SSRC:%.s=%.do}
filesystem.rlib_OBJS = ${filelib_CSRC:%.c=%.ro} ${filelib_SSRC:%.s=%.ro}

filesystem.blib: ${filesystem.blib_OBJS}
	         ${linklib}

filesystem.dlib: ${filesystem.dlib_OBJS}
	         ${linklib}

filesystem.rlib: ${filesystem.rlib_OBJS}
	         ${linklib}


###############################################################################
#
#
RCSFILES   = ${filelib_CSRC} ${filelib_SSRC}

AUTODEPEND = ${filesystem.blib_OBJS} ${filesystem.dlib_OBJS} ${filesystem.rlib_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
