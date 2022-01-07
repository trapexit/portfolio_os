/* File : doaccess.h */
/*
** 930818 PLB Added AAFLAGS_WRITE_ABLE flag for Save and Delete volumes.
**            This is not fully implemented.
**            Disabled diagnostics macros cuz DEBUG is messed up.
*/
#ifndef	__DOACCESS_H
#define	__DOACCESS_H

#include "types.h"
#include "access.h"
#include "filefunctions.h"

#define OK		0
#define	CANCEL	1

#define	BUTTON1	0
#define	BUTTON2	1

#define	SUCCESS					0
#define ERR_GENERIC				-1
#define ERR_NO_SAVER			-2
#define ERR_NO_CHR_IN_FONT		-3
#define	ERR_SERIOUS_MEM_ERR		-4
#define	ERR_ACCESS_DESTROY_FAIL	-98
#define	ERR_ACCESS_INIT_FAIL	-99


#define ACCESS_GRAFINIT  0x00000001
#define ACCESS_PADINIT   0x00000002

#undef DIAGNOSTIC
#undef ERROR
#undef INFORMATION

// diagnosticflag must be set when diagnostics enabled
#undef DIAGNOSTICFLAG

#define DIAGNOSTIC(s)		/* printf("Information : %s\n", s) */
#define INFORMATION(s)		/* printf("Information : %s\n", s) */
#define ERROR(s)		/* printf("*** ERROR : %s\n", s) */

/*???#define ControlAccept (ControlA | ControlB | ControlC)*/
#define ControlAccept (ControlA)
#define ControlMove   (ControlUp | ControlDown | ControlLeft | ControlRight)


typedef struct AccessArgs
	{
	// ACCTAG_REQUEST
	int32 aa_Request;

	// see below for definition of Flags bits
	int32 aa_Flags;

	// ACCTAG_SCREEN
	Item aa_Screen;

	// ACCTAG_BUFFER
	// ACCTAG_BUFFERSIZE
	char *aa_Buffer;
	int32 aa_BufferSize;

	// ACCTAG_TITLE
	char *aa_Title;

	// ACCTAG_TEXT
	char *aa_Text;

	// ACCTAG_BUTTON_ONE
	// ACCTAG_BUTTON_TWO
	char *aa_ButtonOne;
	char *aa_ButtonTwo;

	// ACCTAG_STRINGBUF
	// ACCTAG_STRINGBUF_SIZE
	char *aa_StringBuffer;
	int32 aa_StringBufferSize;

	// ACCTAG_FG_PEN
	// ACCTAG_BG_PEN
	// ACCTAG_HILITE_PEN
	int32 aa_FGPen;
	int32 aa_BGPen;
	int32 aa_HilitePen;
	int32 aa_ShadowPen;		// еее Shadow Pen Value @NICK
	} AccessArgs;

// aa_Flags Definitions
#define AAFLAGS_SAVE_BACK  0x00000001
#define AAFLAGS_EVNT_INIT  0x00000002	// еее EventUtility Flag @NICK
#define AAFLAGS_GRPH_INIT  0x00000004	// еее GraphicsFolio Flag @NICK
#define AAFLAGS_WRITE_ABLE  0x00000008	// Volume must be writeable


int32	DoAccess( TagArg *targs , uint8 msgflags);
int32	DoAccessDispatch(AccessArgs *aa_args);
int32	DoAccessLoad(AccessArgs *aa_args);
int32	DoAccessSave(AccessArgs *aa_args);
int32	DoAccessTwoButton(AccessArgs *aa_args);
int32	DoAccessOneButton(AccessArgs *aa_args);
int32	DoAccessDelete(AccessArgs *aa_args);
int32	AccessButtons(AccessArgs *aa_args, int32 Buttons);

#endif
