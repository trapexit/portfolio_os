/* File : doaccess.c */
/*
** 930819 PLB Always Init and Kill events to nest focus properly.
** 930820 PLB Hacked return codes for Save to make them consistent with
**            Load and Delete
*/
#include "types.h"
#include "shelldebug.h"
#include "access.h"
#include "doaccess.h"
#include "AccessUtility.h"
#include "AccessText.h"
#include "AccessLoad.h"
#include "AccessSave.h"
#include "AccessDelete.h"
#include "filesystem.h"
#include "filefunctions.h"
#include "msgport.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))

//???
int32 AccessFlags = 0;


#define BUF_SIZE 256
static char tmp_buffer[BUF_SIZE];


int32
DoAccessLoad(aa_args)
AccessArgs	*aa_args;
{
#ifdef DISGNOSTICFLAG
	char	selectfilename[40];
#endif
	int32	result;

	DIAGNOSTIC("DoAccessLoad");

	result = AccessLoad(aa_args);

	if (result == OK)
	  DIAGNOSTIC("OK");
	else
	  DIAGNOSTIC("CANCEL");

	DIAGNOSTIC(selectfilename);

	return(result);
}



int32
DoAccessSave(aa_args)
AccessArgs	*aa_args;
{
	int32	result;

	DIAGNOSTIC("DoAccessSave");

	result = AccessSave(aa_args);

	if (result == RETURN)
	{
		DIAGNOSTIC("RETURN");
		result = CANCEL;
	}
	else
	{
		DIAGNOSTIC("SAVE");
		result = OK;
	}

	DIAGNOSTIC(aa_args->aa_StringBuffer);

	return(result);
}



int32
DoAccessTwoButton(aa_args)
AccessArgs	*aa_args;
{
	int32		result;

	DIAGNOSTIC("DoAccessTwoButton");

	result = AccessButtons(aa_args, 2);

	if (result == BUTTON1)
	  DIAGNOSTIC("BUTTON1");
	else
	  DIAGNOSTIC("BUTTON2");

	return(result);
}



int32
DoAccessOneButton(aa_args)
AccessArgs	*aa_args;
{

	DIAGNOSTIC("DoAccessOneButton");

	return(AccessButtons(aa_args, 1));
}



int32
DoAccessDelete( AccessArgs *aa_args )
{
#ifdef DISGNOSTICFLAG
	char	selectfilename[40];
#endif
	int32	result;

	DIAGNOSTIC("DoAccessDelete");

	result = AccessDelete(aa_args);

	if (result == OK)
	  DIAGNOSTIC("OK");
	else
	  DIAGNOSTIC("CANCEL");

	DIAGNOSTIC(selectfilename);

	return(result);

}

Item vblItem;

int32 DoAccess( TagArg *targs , uint8 msgflags)
{
AccessArgs TheArgs;
int32 retvalue;
char *init_string = NULL;
int32 init_string_len=0;

    OpenFileFolio();

    retvalue = OpenGraphicsFolio();
    if (retvalue >= 0)
    {
        vblItem = GetVBLIOReq();
        if (vblItem >= 0)
        {
            retvalue = InitEventUtility(1, 0, LC_FocusListener);
            if (retvalue >= 0)
            {
                memset( &TheArgs, 0, sizeof( AccessArgs ) );
                memset( tmp_buffer,0,sizeof(tmp_buffer));

                TheArgs.aa_BGPen     = 0x7FFF;
                TheArgs.aa_FGPen     = 0x0421;
                TheArgs.aa_HilitePen = 0x17E5;
                TheArgs.aa_ShadowPen = TheArgs.aa_BGPen - 0x4210;

                while (targs->ta_Tag != TAG_END)
                {
                    switch( targs->ta_Tag )
                    {
                        case ACCTAG_REQUEST:
                                TheArgs.aa_Request = (int32)targs->ta_Arg;
                                break;
                        case ACCTAG_SCREEN:
                                TheArgs.aa_Screen = (Item)targs->ta_Arg;
                                break;
                        case ACCTAG_BUFFER:
                                TheArgs.aa_Buffer = (char *)targs->ta_Arg;
                                break;
                        case ACCTAG_BUFFERSIZE:
                                TheArgs.aa_BufferSize = (int32)targs->ta_Arg;
                                break;
                        case ACCTAG_TITLE:
                                TheArgs.aa_Title = (char *)targs->ta_Arg;
                                break;
                        case ACCTAG_TEXT:
                                TheArgs.aa_Text = (char *)targs->ta_Arg;
                                break;
                        case ACCTAG_BUTTON_ONE:
                                TheArgs.aa_ButtonOne = (char *)targs->ta_Arg;
                                break;
                        case ACCTAG_BUTTON_TWO:
                                TheArgs.aa_ButtonTwo = (char *)targs->ta_Arg;
                                break;
                        case ACCTAG_SAVE_BACK:
                                SetFlag( TheArgs.aa_Flags, AAFLAGS_SAVE_BACK );
                                break;
                        case ACCTAG_STRINGBUF:
                                if((msgflags & MESSAGE_PASS_BY_VALUE) != 0) {
                                    init_string = (char *)targs->ta_Arg;
                                }
                                else TheArgs.aa_StringBuffer = (char *)targs->ta_Arg;
                                break;
                        case ACCTAG_STRINGBUF_SIZE:
                                if((msgflags & MESSAGE_PASS_BY_VALUE) != 0)
                                    init_string_len = (int32)targs->ta_Arg;
                                else TheArgs.aa_StringBufferSize = (int32)targs->ta_Arg;
                                break;
                        case ACCTAG_FG_PEN:
                                TheArgs.aa_FGPen = (int32)targs->ta_Arg;
                                break;
                        case ACCTAG_BG_PEN:
                                TheArgs.aa_BGPen = (int32)targs->ta_Arg;
                                break;
                        case ACCTAG_HILITE_PEN:
                                TheArgs.aa_HilitePen = (int32)targs->ta_Arg;
                                break;
                        case ACCTAG_SHADOW_PEN:
                                TheArgs.aa_ShadowPen = (int32)targs->ta_Arg;
                                break;
                    }
                    targs++;
                }

                if (TheArgs.aa_Screen > 0)
                {
                    // open the screen item shared
                    retvalue = OpenItem(TheArgs.aa_Screen,0);
                    if(retvalue >= 0)
                    {
                        //??? If buffer declared but no corresponding size, error
                        if (!(( TheArgs.aa_Buffer && (TheArgs.aa_BufferSize <= 0) )
                        || ( TheArgs.aa_StringBuffer && (TheArgs.aa_StringBufferSize <= 0) )))
                        {
                            // if the message is pass-by-value, set up the string buffer
                            if ((msgflags & MESSAGE_PASS_BY_VALUE) != 0)
                            {
                                TheArgs.aa_StringBuffer = tmp_buffer;
                                TheArgs.aa_StringBufferSize = BUF_SIZE;

                                    // copy in any initial string
                                if (init_string)
                                    strncpy(tmp_buffer,init_string,min(BUF_SIZE,init_string_len));
                            }

                            ResetCurrentFont();

                            switch (TheArgs.aa_Request)
                            {
                                case ACCREQ_LOAD :
                                        if ( TheArgs.aa_StringBuffer == NULL )
                                                {
                                                retvalue = ACCERR_BAD_ARGS;
                                                break;
                                                }
                                        retvalue = DoAccessLoad( &TheArgs );
                                        break;
                                case ACCREQ_SAVE :
                                        if ( TheArgs.aa_StringBuffer == NULL )
                                                {
                                                retvalue = ACCERR_BAD_ARGS;
                                                break;
                                                }
                                        retvalue = DoAccessSave( &TheArgs );
                                        break;
                                case ACCREQ_DELETE :
                                        if ( TheArgs.aa_StringBuffer == NULL )
                                                {
                                                retvalue = ACCERR_BAD_ARGS;
                                                break;
                                                }
                                        retvalue = DoAccessDelete( &TheArgs );
                                        break;
                                case ACCREQ_OK :
                                        TheArgs.aa_ButtonOne = "OK";
                                        retvalue = DoAccessOneButton( &TheArgs );
                                        break;
                                case ACCREQ_OKCANCEL :
                                        TheArgs.aa_ButtonOne = "OK";
                                        TheArgs.aa_ButtonTwo = "CANCEL";
                                        retvalue = DoAccessTwoButton( &TheArgs );
                                        break;
                                case ACCREQ_ONEBUTTON :
                                        if ( TheArgs.aa_ButtonOne == NULL )
                                                {
                                                retvalue = ACCERR_BAD_ARGS;
                                                break;
                                                }
                                        retvalue = DoAccessOneButton( &TheArgs );
                                        break;
                                case ACCREQ_TWOBUTTON :
                                        if ( (TheArgs.aa_ButtonOne == NULL)
                                                        || (TheArgs.aa_ButtonTwo == NULL) )
                                                {
                                                retvalue = ACCERR_BAD_ARGS;
                                                break;
                                                }
                                        retvalue = DoAccessTwoButton( &TheArgs );
                                        break;
                                default :
                                        retvalue = ACCERR_BAD_REQUEST;
                                        break;
                            }

                            ResetCurrentFont();
                        }
                        else
                        {
                            retvalue = ACCERR_BAD_ARGS;
                        }
                        CloseItem(TheArgs.aa_Screen);
                    }
                    else
                    {
                        PrintError(NULL,"OpenItem()","supplied screen",retvalue);
                    }
                }
                else
                {
                    retvalue = ACCERR_NO_SCREEN;
                }
                KillEventUtility();
            }
            DeleteItem(vblItem);
        }
        else
        {
            retvalue = vblItem;
        }
        CloseGraphicsFolio();
    }

    CloseFileFolio();

    return (retvalue);
}


/*****************************************************************************/


int main(Item msgItem)
{
int32  result;
Msg   *msg;

    msg = (Msg *)LookupItem(msgItem);

    result = DoAccess((TagArg *)msg->msg_DataPtr, msg->msg.n_Flags);

    if ((msg->msg.n_Flags & MESSAGE_PASS_BY_VALUE) != 0)
        ReplyMsg(msgItem,result,(void *)tmp_buffer,(int32)strlen(tmp_buffer));
    else
        ReplyMsg(msgItem,result,(void *)NULL,(int32)NULL);

    return (0);
}
