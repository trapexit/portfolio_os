# $Id: GNUmakefile,v 1.2 1994/10/11 19:12:39 limes Exp $
# GNUmakefile for the Debugger folio test programs.
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All Rights Reserved.
# This document is proprietary and confidential

default: all


###############################################################################
# Describe our files
#
LOCAL_INCLUDES = ../folio

LOCAL_INCS     =

LOCALS	       = debugtest


###############################################################################
# Stuff for the test program
#
debugtest_CSRC   = debugtest.c

debugtest_OBJS   = ${debugtest_CSRC:%.c=%.o}
debugtest_MODBIN = -pri 20 -stack 10240
debugtest_LIBS   = ../lib/debugger.dlib filesystem.lib kernel.lib clib.lib

debugtest: ${debugtest_OBJS}
	   ${linkdev}


###############################################################################
# Files under RCS
#
RCSFILES   = ${debugtest_CSRC}

AUTODEPEND = ${debugtest_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
