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

local_INCS = timerservicesinternals.h
local_CSRC = TimerServicesAPI.c \
	     TimerServicesAPIU.c \
	     TimerServicesAPIV.c \
	     TimerServicesThread.c \
	     TimerUtilsGetIOReq.c \
	     TimerUtilsGetTime.c \
	     TimerUtilsSleep.c

local.dlib_OBJS = ${local_SSRC:%.s=%.do} ${local_CSRC:%.c=%.do}
local.rlib_OBJS = ${local_SSRC:%.s=%.ro} ${local_CSRC:%.c=%.ro}

local.dlib: ${local.dlib_OBJS}
	    ${linklib}

local.rlib: ${local.rlib_OBJS}
	    ${linklib}

# ==========

EXAMPLE_SRCS  = ${local_CSRC} ${local_SSRC} ${local_INCS}
EXAMPLE_CLASS = Lib3DO/TimerUtils
RCSFILES      = ${local_CSRC} ${local_SSRC} ${local_INCS}
AUTODEPEND    =	${local.dlib_OBJS} ${local.rlib_OBJS}
DEPFILE       = .autodepends
include GNUmakefile.master
