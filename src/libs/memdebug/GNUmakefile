# $Id: GNUmakefile,v 1.12 1994/12/17 02:16:47 vertex Exp $
# GNUmakefile for the Portfolio memory debugging library
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all


###############################################################################
# Describe our files
#

LIB_DATE   = builddate

DLIBS      = memdebug.dlib
RLIBS      = memdebug.rlib
DOCS       = MemDebugNotes.doc


###############################################################################
# Stuff for the link library...
#
memdebuglib_SSRC   = allocvectors.s
memdebuglib_CSRC   = memdebug.c createthread.c errors.c autodocs.c

memdebug.dlib_OBJS = ${memdebuglib_CSRC:%.c=%.do} ${memdebuglib_SSRC:%.s=%.do}
memdebug.rlib_OBJS = ${memdebuglib_CSRC:%.c=%.ro} ${memdebuglib_SSRC:%.s=%.ro}

memdebug.dlib: ${memdebug.dlib_OBJS}
	       ${linklib}

memdebug.rlib: ${memdebug.rlib_OBJS}
	       ${linklib}


###############################################################################
#
#
RCSFILES   = ${memdebuglib_CSRC} ${memdebuglib_SSRC}

AUTODEPEND = ${memdebug.dlib_OBJS} ${memdebug.rlib_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
