# $Id: GNUmakefile,v 1.1 1994/11/14 23:17:04 vertex Exp $
# GNUmakefile for the C++ support library
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all


###############################################################################
# Describe our files
#

LIB_DATE   = builddate
LIBS       = cpluslib.lib


###############################################################################
# Stuff for the link library...
#
cpluslib_CSRC     = cplusruntime.c
cpluslib.lib_OBJS = ${cpluslib_CSRC:%.c=%.o}

cpluslib.lib: ${cpluslib.lib_OBJS}
	      ${linklib}


###############################################################################
#
#
RCSFILES   = ${cpluslib_CSRC}

AUTODEPEND = ${cpluslib.dlib_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
