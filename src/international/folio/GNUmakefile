# $Id: GNUmakefile,v 1.21 1994/12/20 18:15:40 vertex Exp $
# GNUmakefile for the International folio.
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All Rights Reserved.
# This document is proprietary and confidential

default: all


###############################################################################
# Describe our files
#
INCS_HERE  = intl.h

DATE       = builddate

DHDW       = international.privfolio.dev
RHDW       = international.privfolio.rom
PGMS       = CountryDatabase
CDEFS      = -DINTLFOLIO_PRIVATE


###############################################################################
# Stuff for the folio
#
international_CSRC        = international_folio.c formatdate.c \
		            formatnumber.c locales.c autodocs.c \
		            transliterate.c externalcode.c englishdriver.c \
		            usermodeserver.c utils.c

international_INCS        = international_folio.h locales.h externalcode.h \
	                    englishdriver.h langdrivers.h usermodeserver.h \
	                    utils.h countrydb.h

international_STARTUPCODE = subroutinestartup.o
international_MODBIN      = -pri 20 -type 4

international.dev_OBJS    = ${international_CSRC:%.c=%.do}
international.dev_LIBS    = filesystem.lib kernel.lib jstring.lib clib.lib

international.rom_OBJS    = ${international_CSRC:%.c=%.ro}
international.rom_LIBS    = filesystem.lib kernel.lib jstring.lib clib.lib

international.dev: ${international.dev_OBJS}
		   ${linkdev}
		   ${compress}
		   ${setrev}
		   ${setpriv}

international.rom: ${international.rom_OBJS}
		   ${linkrom}
		   ${compress}
		   ${setrev}
		   ${setpriv}

international.privfolio.dev: international.dev
	                     ${copyit}

international.privfolio.rom: international.rom
			     ${copyit}


###############################################################################
# Generate the country database
#
gencountrydb: gencountrydb.c
	      gcc gencountrydb.c -o gencountrydb -I.top/includes -funsigned-char

CountryDatabase: gencountrydb
	         ./gencountrydb CountryDatabase


###############################################################################
#
#
RCSFILES   = ${international_CSRC} ${international_INCS} gencountrydb.c

AUTODEPEND = ${international.dev_OBJS} ${international.rom_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
