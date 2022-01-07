# $Id: GNUmakefile,v 1.2 1994/09/20 22:09:24 vertex Exp $
# GNUmakefile for the Debugger folio link library.
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

LOCAL_INCLUDES = ../folio

LIB_DATE       = builddate

ZO             = -zo

DLIBS          = debugger.dlib


###############################################################################
# Stuff for the link library...
#
debuggerlib_SSRC   =
debuggerlib_CSRC   = debuggerfoliolib.c

debugger.dlib_OBJS = ${debuggerlib_CSRC:%.c=%.do} ${debuggerlib_SSRC:%.s=%.do}
debugger.rlib_OBJS = ${debuggerlib_CSRC:%.c=%.ro} ${debuggerlib_SSRC:%.s=%.ro}

debugger.dlib: ${debugger.dlib_OBJS}
	       ${linklib}

debugger.rlib: ${debugger.rlib_OBJS}
	       ${linklib}


###############################################################################
#
#
RCSFILES   = ${debuggerlib_CSRC} ${debuggerlib_SSRC}

AUTODEPEND = ${debugger.dlib_OBJS} ${debugger.rlib_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
