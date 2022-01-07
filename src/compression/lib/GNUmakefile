# $Id: GNUmakefile,v 1.3 1994/09/20 21:59:44 vertex Exp $
# GNUmakefile for the compression folio link library.
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All Rights Reserved.
# This document is proprietary and confidential

default: all


###############################################################################
# Describe our files
#
LOCAL_INCS     = compression_lib.h

INCS_HERE      =

LOCAL_INCLUDES = ../folio

LIB_DATE       = builddate

DLIBS          = compression.dlib
RLIBS          = compression.rlib


###############################################################################
# Stuff for the link library...
#
compressionlib_SSRC   =
compressionlib_CSRC   = closefolio.c createcompressor.c createdecompressor.c \
			deletecompressor.c deletedecompressor.c\
			feedcompressor.c feeddecompressor.c \
			getcompressorworkbuffersize.c \
			getdecompressorworkbuffersize.c \
			openfolio.c compress.c decompress.c \
			createcompressorva.c createdecompressorva.c \
			getcompressorworkbuffersizeva.c \
			getdecompressorworkbuffersizeva.c

compression.dlib_OBJS = ${compressionlib_CSRC:%.c=%.do} ${compressionlib_SSRC:%.s=%.do}
compression.rlib_OBJS = ${compressionlib_CSRC:%.c=%.ro} ${compressionlib_SSRC:%.s=%.ro}

compression.dlib: ${compression.dlib_OBJS}
	          ${linklib}

compression.rlib: ${compression.rlib_OBJS}
	          ${linklib}


###############################################################################
#
#
RCSFILES   = ${compressionlib_CSRC} ${compressionlib_SSRC}

AUTODEPEND = ${compression.dlib_OBJS} ${compression.rlib_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
