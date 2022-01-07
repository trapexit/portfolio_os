# $Id: GNUmakefile,v 1.2 1994/11/02 01:43:57 vertex Exp $
# GNUmakefile for ObsoleteLib3DO
#
# Copyright (c) 1992, 1993, 1994 The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

# ==========

CDEFS=	-DEXTERNAL_RELEASE

# put each function in its own code area
ZO    =			-zo

# ==========

DLIBS=			obsoletelib3do.dlib
RLIBS=			obsoletelib3do.rlib

obsoletelib3do_CSRC=		 \
			autodocs.c \
			CenterCelOnScreen.c \
			CenterRectf16.c \
			ChangeInitialDir.c \
			FadeInCel.c \
			FadeOutCel.c \
			FrameBufferToCel.c \
			FreeBuffer.c \
			LoadSoundEffect.c \
			LoadSoundFX.c \
			MakeNewCel.c \
			MakeNewDupCCB.c \
			MapP2Cel.c \
			MoveCel.c \
			OpenAudio.c \
			OpenMacLink.c \
			OpenSPORT.c \
			PreMoveCel.c \
			ReadControlPad.c \
			SetCelScale.c \
			SetChannel.c \
			SetFadeInCel.c \
			SetFadeOutCel.c \
			SetMixer.c \
			SetMixerChannel.c \
			SetQuad.c \
			SetRectf16.c \
			ShutDown.c

obsoletelib3do.dlib_OBJS= ${obsoletelib3do_CSRC:%.c=%.do}
obsoletelib3do.dlib: ${obsoletelib3do.dlib_OBJS}
	             ${linklib}

obsoletelib3do.rlib_OBJS= ${obsoletelib3do_CSRC:%.c=%.ro}
obsoletelib3do.rlib: ${obsoletelib3do.rlib_OBJS}
	             ${linklib}

# ==========

RCSFILES   = ${obsoletelib3do_CSRC}
AUTODEPEND = ${obsoletelib3do.dlib_OBJS} ${obsoletelib3do.rlib_OBJS}
DEPFILE    = .autodepends

include GNUmakefile.master
