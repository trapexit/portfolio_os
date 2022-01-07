# $Id: GNUmakefile,v 1.3 1994/09/20 22:01:22 vertex Exp $
# GNUmakefile for the International folio link library.
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All Rights Reserved.
# This document is proprietary and confidential

default: all


###############################################################################
# Describe our files
#
LOCAL_INCS     = international_lib.h

INCS_HERE      =

LOCAL_INCLUDES = ../folio

LIB_DATE       = builddate

DLIBS          = international.dlib
RLIBS          = international.rlib


###############################################################################
# Stuff for the link library...
#
internationallib_SSRC   =
internationallib_CSRC   = closefolio.c comparestrings.c convertstring.c formatdate.c\
	                  formatnumber.c getcharattrs.c openfolio.c \
	                  transliteratestring.c

international.dlib_OBJS = ${internationallib_CSRC:%.c=%.do} ${internationallib_SSRC:%.s=%.do}
international.rlib_OBJS = ${internationallib_CSRC:%.c=%.ro} ${internationallib_SSRC:%.s=%.ro}

international.dlib: ${international.dlib_OBJS}
	            ${linklib}

international.rlib: ${international.rlib_OBJS}
	            ${linklib}


###############################################################################
#
#
RCSFILES   = ${internationallib_CSRC}

AUTODEPEND = ${international.dlib_OBJS} ${international.rlib_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
