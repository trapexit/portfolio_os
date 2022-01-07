# $Id: GNUmakefile,v 1.42 1994/12/01 00:49:54 stan Exp $
# GNUmakefile for Lib3DO
#
# Copyright (c) 1992, 1993, 1994 The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default: all

# ==========

.tree: .includes

# ==========

INCS_DOWN = animutils.h \
	    blockfile.h \
	    celutils.h \
	    debug3do.h \
	    deletecelmagic.h \
	    displayutils.h \
	    fontlib.h \
	    form3do.h \
	    init3do.h \
	    macros3do.h \
	    msgutils.h \
	    parse3do.h \
	    portfolio.h \
	    textlib.h \
	    timerutils.h \
	    umemory.h \
	    utils3do.h

BDIRS = AnimUtils \
	CelUtils \
	DisplayUtils \
	IOUtils \
	MiscUtils \
	TextLib \
	TimerUtils

# ==========

DLIBS =	lib3do.dlib
RLIBS =	lib3do.rlib

lib3do.dlib_OBJS = AnimUtils/*.do \
	           CelUtils/*.do \
	           DisplayUtils/*.do \
	           IOUtils/*.do \
	           MiscUtils/*.do \
	           TextLib/*.do \
	           TimerUtils/*.do

lib3do.rlib_OBJS = AnimUtils/*.ro \
	           CelUtils/*.ro \
	           DisplayUtils/*.ro \
	           IOUtils/*.ro \
	           MiscUtils/*.ro \
	           TextLib/*.ro \
	           TimerUtils/*.ro

lib3do.dlib: AnimUtils/local.dlib \
	     CelUtils/local.dlib \
	     DisplayUtils/local.dlib \
	     IOUtils/local.dlib \
	     MiscUtils/local.dlib \
	     TextLib/local.dlib \
	     TimerUtils/local.dlib
	     ${linklib}

lib3do.rlib: AnimUtils/local.rlib \
	     CelUtils/local.rlib \
	     DisplayUtils/local.rlib \
	     IOUtils/local.rlib \
	     MiscUtils/local.rlib \
	     TextLib/local.rlib \
	     TimerUtils/local.rlib
	     ${linklib}

%/local.dlib:
	${MAKE} -f GNUmakefile -C $* local.dlib

%/local.rlib:
	${MAKE} -f GNUmakefile -C $* local.rlib

# ==========

EXAMPLE_SRCS  = Lib3DO.lib.make
EXAMPLE_CLASS = Lib3DO
RCSFILES      =	${EXAMPLE_SRCS} autodocs.c
DEPFILE       = .autodepends

include GNUmakefile.master
