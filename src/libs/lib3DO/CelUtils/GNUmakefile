# $Id: GNUmakefile,v 1.2 1994/11/22 19:20:20 vertex Exp $
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

local_CSRC = CenterPoint.c \
	     CenterRect.c \
	     CenterRectCelFPoint.c \
	     CenterRectCelIPoint.c \
	     CenterRectCelRect.c \
	     ChainCels.c \
	     CloneCel.c \
	     CreateBackdropCel.c \
	     CreateCel.c \
	     CreateLRFormCel.c \
	     CreateSubrectCel.c \
	     CrossFadeCels.c \
	     DeleteCelMagic.c \
	     GetCelBufferSize.c \
	     InitCel.c \
	     InterUnionRect.c \
	     LinkCel.c \
	     LoadCel.c \
	     MapCelToPoint.c \
	     MapCelToQuad.c \
	     MapCelToRect.c \
	     OffsetCel.c \
	     OffsetInsetRect.c \
	     ParseCel.c \
	     PointConversions.c \
	     RectConversions.c \
	     RectFromCel.c \
	     RenderCelPixel.c

local.dlib_OBJS = ${local_SSRC:%.s=%.do} ${local_CSRC:%.c=%.do}
local.rlib_OBJS = ${local_SSRC:%.s=%.ro} ${local_CSRC:%.c=%.ro}

local.dlib: ${local.dlib_OBJS}
	    ${linklib}

local.rlib: ${local.rlib_OBJS}
	    ${linklib}

# ==========

EXAMPLE_SRCS  = ${local_CSRC} ${local_SSRC}
EXAMPLE_CLASS = Lib3DO/CelUtils
RCSFILES      = ${local_CSRC} ${local_SSRC}
AUTODEPEND    =	${local.dlib_OBJS} ${local.rlib_OBJS}
DEPFILE       = .autodepends
include GNUmakefile.master
