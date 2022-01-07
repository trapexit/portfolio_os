# $Id: GNUmakefile,v 1.3 1995/03/02 01:32:40 stan Exp $
# GNUmakefile for Lib3DO
#
# Copyright (c) 1994 The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default: all

# ==========

CDEFS  = -DEXTERNAL_RELEASE
ZO     = -zo
LOCALS = local.dlib local.rlib
LOCAL_INCLUDES = ../includes

# ==========

local_CSRC = Debug3DO.c \
	     GetChunk.c \
	     MSEvents.c \
	     UMemory.c

local.dlib_OBJS = ${local_SSRC:%.s=%.do} ${local_CSRC:%.c=%.do}
local.rlib_OBJS = ${local_SSRC:%.s=%.ro} ${local_CSRC:%.c=%.ro}

local.dlib: ${local.dlib_OBJS}
	    ${linklib}

local.rlib: ${local.rlib_OBJS}
	    ${linklib}

# ==========

EXAMPLE_SRCS  = ${local_CSRC} ${local_SSRC}
EXAMPLE_CLASS = Lib3DO/MiscUtils
RCSFILES      = ${local_CSRC} ${local_SSRC}
AUTODEPEND    =	${local.dlib_OBJS} ${local.rlib_OBJS}
DEPFILE       = .autodepends
include GNUmakefile.master
