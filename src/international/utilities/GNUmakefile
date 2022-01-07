# $Id: GNUmakefile,v 1.1 1994/12/09 01:22:12 vertex Exp $
# GNUmakefile for the International folio utility programs.
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All Rights Reserved.
# This document is proprietary and confidential

default: all


###############################################################################
# Describe our files
#
LOCAL_INCLUDES = ../folio

PGMS	       = intl


###############################################################################
# Stuff for the utility programs
#
intl_CSRC   = intl.c
intl_OBJS   = ${intl_CSRC:%.c=%.o}
intl_MODBIN = -pri 20 -stack 10240
intl_LIBS   = ../lib/international.dlib filesystem.lib clib.lib

intl:   ${intl_OBJS}
	${linkdev}


###############################################################################
# Files under RCS
#
RCSFILES   = ${intl_CSRC}

AUTODEPEND = ${intl_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
