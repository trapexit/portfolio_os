# $Id: GNUmakefile,v 1.3 1994/11/04 18:41:47 vertex Exp $
# GNUmakefile for the jstring folio link library.
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All Rights Reserved.
# This document is proprietary and confidential

default: all


###############################################################################
# Describe our files
#
LOCAL_INCS     = jstring_lib.h

INCS_HERE      =

LOCAL_INCLUDES = ../folio

LIB_DATE       = builddate

DLIBS          = jstring.dlib
RLIBS          = jstring.rlib


###############################################################################
# Stuff for the link library...
#
jstringlib_SSRC   =
jstringlib_CSRC   = closefolio.c \
		    convertshiftjis2unicode.c \
	            convertascii2shiftjis.c \
	            convertfullkana2halfkana.c \
	            convertfullkana2hiragana.c \
	            convertfullkana2romaji.c \
	            converthalfkana2fullkana.c \
	            converthalfkana2hiragana.c \
	            converthalfkana2romaji.c \
	            converthiragana2fullkana.c \
	            converthiragana2halfkana.c \
	            converthiragana2romaji.c \
	            convertromaji2fullkana.c \
	            convertromaji2halfkana.c \
	            convertromaji2hiragana.c \
	            convertshiftjis2ascii.c \
	            convertunicode2shiftjis.c \
	            openfolio.c

jstring.dlib_OBJS = ${jstringlib_CSRC:%.c=%.do} ${jstringlib_SSRC:%.s=%.do}
jstring.rlib_OBJS = ${jstringlib_CSRC:%.c=%.ro} ${jstringlib_SSRC:%.s=%.ro}

jstring.dlib: ${jstring.dlib_OBJS}
	      ${linklib}

jstring.rlib: ${jstring.rlib_OBJS}
	      ${linklib}


###############################################################################
#
#
RCSFILES   = ${jstringlib_CSRC}

AUTODEPEND = ${jstring.dlib_OBJS} ${jstring.rlib_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
