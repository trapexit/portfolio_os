# $Id: GNUmakefile,v 1.6 1994/10/11 19:12:53 limes Exp $

# Makefile for HWControl driver

# $Log: GNUmakefile,v $
# Revision 1.6  1994/10/11  19:12:53  limes
# eliminate unnecessary ${modbin} calls
#
# Revision 1.5  1994/10/01  05:28:13  deborah
# Add BuildDate support to print out version info on startup
#
# Revision 1.4  1994/09/17  02:36:33  stan
# Added the 3DO card driver to this makefile
#
# Revision 1.3  1994/09/02  20:51:30  stan
# Include file to all lower case.
#
# Revision 1.2  1994/08/29  19:04:18  vertex
# Removed a bunch of useless link lib from the link process,
# including swi.lib
#
# Revision 1.1  1994/04/18  22:51:57  limes
# Initial revision
#

default:	all

# Yes, we have a datestamp file.
DATE=                   BuildDate

INCS_HERE=	hwcontroldriver.h

LINK_LIBS= \
		filesystem.lib \
		graphics.lib \
		clib.lib \
		kernel.lib

PGMS=		InstallHWControlDriver StorageDriver

InstallHWControlDriver_CSRC=		InstallHWControlDriver.c \
					HWControlDriver.c

InstallHWControlDriver_OBJS=		${InstallHWControlDriver_CSRC:%.c=%.o}
InstallHWControlDriver_MODBIN=	-stack 4096
InstallHWControlDriver: ${InstallHWControlDriver_OBJS}
	${linkdev}
	${setpriv}

StorageDriver_CSRC=	StorageDriver.c
StorageDriver_OBJS=	${StorageDriver_CSRC:%.c=%.o}
StorageDriver_MODBIN=	-stack 4096
StorageDriver:		${StorageDriver_OBJS}
	${linkdev}
	${setpriv}

RCSFILES=		${InstallHWControlDriver_CSRC} ${StorageDriver_CSRC}
AUTODEPEND=		${InstallHWControlDriver_OBJS} ${StorageDriver_OBJS}
DEPFILE=		.autodepends

include GNUmakefile.master

