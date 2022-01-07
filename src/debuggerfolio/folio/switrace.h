/* $Id: switrace.h,v 1.2 1994/08/02 02:41:48 anup Exp $ */

/*
 * $Log: switrace.h,v $
 * Revision 1.2  1994/08/02  02:41:48  anup
 * Added Id and Log keywords
 *
 */

/*
	File:		SWITrace.h

	Contains:	Header for SWI trace mechanism

	Written by:	Anup K Murarka

	Copyright:	©1994 by The 3DO Company, all rights reserved
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

				08.07.94	akm		First Writing

	To Do:
*/

extern Err InstallSWITrace(void);
extern Err RemoveSWITrace(void);

extern Err EnableSWITrace(Boolean on);		// Enable & maybe install

#define SWITraceON()	EnableSWITrace(true)
#define SWITraceOFF()	EnableSWITrace(false)
#define	GetSWITraceEnable()	(DebugFolioPublicFlags->SWITraceEnable)

