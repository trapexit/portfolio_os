	;; $Id: FileDaemonStartup.s,v 1.3 1994/02/09 01:27:17 limes Exp $

	AREA	ASMCODE, CODE, READONLY

	EXPORT	FileDaemonStartup
FileDaemonStartup
	swi	&00030002
	b	FileDaemonStartup
	END














