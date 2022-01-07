# $Id: GNUmakefile,v 1.30 1994/10/12 21:08:21 limes Exp $
# GNUmakefile for the Portfolio Shell.
#
# Copyright (c) 1992-1994, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default: all


###############################################################################
# Describe our files
#
LOCAL_INCS = AccessButtons.h AccessDelete.h AccessFileGet.h \
	     AccessLoad.h AccessSave.h AccessShell.h \
	     AccessText.h AccessTwoButton.h AccessUtility.h \
	     DirectoryHelper.h FilesView.h MsgStrings.h \
	     doaccess.h intcmds.h shelldebug.h errors.h \
	     launchapp.h shell.h

INCS_HERE  = access.h

DATE       = BuildDate

DPGMS      = shell.dev access.tuner.dev
RPGMS      = shell.rom access.tuner.rom
DOCS       = ShellNotes.doc

CCOPTIMIZE = -Ospace

CDDEFS     = -DDEVELOPMENT -DDEBUG=1 -DMODE="developer"
CRDEFS     = -DROMBUILD    -DDEBUG=0 -DMODE="runtime"


###############################################################################
# Stuff for the shell proper
#
shell_CSRC        = shell.c intcmds.c errors.c launchapp.c autodocs.c

shell.dev_OBJS    = ${shell_CSRC:%.c=%.do}
shell.dev_MODBIN  = -pri 100 -stack 3072 -MaxUsecs 10000
shell.dev_LIBS    = filesystem.lib graphics.lib clib.lib music.lib\
		    memdebug.lib

shell.rom_OBJS    = ${shell_CSRC:%.c=%.ro}
shell.rom_MODBIN  = -pri 100 -stack 3072 -MaxUsecs 10000
shell.rom_LIBS    = filesystem.lib graphics.lib clib.lib

shell.dev: ${shell.dev_OBJS}
	   ${linkdev}
	   ${compress}
	   ${setrev}

shell.rom: ${shell.rom_OBJS}
	   ${linkrom}
	   ${compress}
	   ${setrev}


###############################################################################
# Stuff for Access
#
access.tuner_CSRC       = AccessUtility.c AccessTwoButton.c AccessSave.c \
		          AccessText.c doaccess.c AccessDelete.c \
		          AccessFileGet.c DirectoryHelper.c FilesView.c \
		          AccessLoad.c

access.tuner_STARTUPCODE=	subroutinestartup.o

access.tuner.dev_OBJS   = ${access.tuner_CSRC:%.c=%.do}
access.tuner.dev_MODBIN = -pri 100 -stack 3072 -MaxUsecs 10000
access.tuner.dev_LIBS   = graphics.lib input.lib filesystem.lib clib.lib

access.tuner.rom_OBJS   = ${access.tuner_CSRC:%.c=%.ro}
access.tuner.rom_MODBIN = -pri 100 -stack 3072 -MaxUsecs 10000
access.tuner.rom_LIBS   = graphics.lib input.lib filesystem.lib clib.lib

access.tuner.dev: ${access.tuner.dev_OBJS}
	          ${linkdev}
	   	  ${compress}
	   	  ${setrev}

access.tuner.rom: ${access.tuner.rom_OBJS}
	          ${linkrom}
	          ${compress}
	   	  ${setrev}


###############################################################################
#
#
RCSFILES   = ${shell_CSRC} ${access.tuner_CSRC}

AUTODEPEND = ${shell.dev_OBJS} ${shell.rom_OBJS} \
	     ${access.tuner.dev_OBJS} ${access.tuner.rom_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
