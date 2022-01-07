# $Id: GNUmakefile,v 1.10 1994/10/07 22:05:23 vertex Exp $
# GNUmakefile for commonly used routines
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

# Just basicly checks everything out, and makes .o files.

INCS_HERE=		loadfile.h writefile.h gettime.h
FUNCS_CSRC=		loadfile.c writefile.c gettime.c
LOCALS=			${FUNCS_CSRC:%.c=%.o}

RCSFILES=		${INCS_HERE} ${FUNCS_CSRC}

AUTODEPEND=		${FUNCS_CSRC:%.c=%.o}

DEPFILE=	.autodepends
include GNUmakefile.master
