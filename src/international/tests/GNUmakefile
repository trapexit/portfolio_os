# $Id: GNUmakefile,v 1.9 1994/10/11 19:12:55 limes Exp $
# GNUmakefile for the International folio test programs.
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All Rights Reserved.
# This document is proprietary and confidential

default: all


###############################################################################
# Describe our files
#
LOCAL_INCLUDES = ../folio

LOCAL_INCS     = formatdate.h formatnumber.h intltest.h transliteratestring.h\
		 comparestrings.h convertstring.h

LOCALS	       = intltest


###############################################################################
# Stuff for the test program
#
intltest_CSRC   = intltest.c formatdate.c formatnumber.c transliteratestring.c\
		  comparestrings.c convertstring.c

intltest_OBJS   = ${intltest_CSRC:%.c=%.o}
intltest_MODBIN = -pri 20 -stack 10240
intltest_LIBS   = ../lib/international.dlib filesystem.lib kernel.lib clib.lib

intltest:	${intltest_OBJS}
		${linkdev}


###############################################################################
# Files under RCS
#
RCSFILES   = ${intltest_CSRC}

AUTODEPEND = ${intltest_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
