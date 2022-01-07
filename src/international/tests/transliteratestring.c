/* $Id: transliteratestring.c,v 1.5 1994/08/26 23:52:00 vertex Exp $ */

#include "types.h"
#include "debug.h"
#include "stdio.h"
#include "string.h"
#include "operror.h"
#include "intltest.h"


/*****************************************************************************/


typedef struct BadPointers
{
    uint32 bp_Ptr;
    Err    bp_ExpectedResult;
} BadPointers;


/*****************************************************************************/


static BadPointers badSources[] =
{
    {-999,       INTL_ERR_BADSOURCEBUFFER},
    {-1,         INTL_ERR_BADSOURCEBUFFER},
    {0,          INTL_ERR_BADSOURCEBUFFER},
    {0x10000000, INTL_ERR_BADSOURCEBUFFER},
};

#define NUM_BADSOURCES (sizeof(badSources) / sizeof(BadPointers))


/*****************************************************************************/


static BadPointers badResults[] =
{
    {-999,       INTL_ERR_BADRESULTBUFFER},
    {-1,         INTL_ERR_BADRESULTBUFFER},
    {0,          INTL_ERR_BADRESULTBUFFER},
    {0x10000000, INTL_ERR_BADRESULTBUFFER},
};

#define NUM_BADRESULTS (sizeof(badResults) / sizeof(BadPointers))


/*****************************************************************************/


static BadPointers badResultSizes[] =
{
    {-999,       INTL_ERR_BADRESULTBUFFER},
    {-1,         INTL_ERR_BADRESULTBUFFER},
    {0,          INTL_ERR_BADRESULTBUFFER},
    {0x10000000, INTL_ERR_BADRESULTBUFFER},
};

#define NUM_BADRESULTSIZES (sizeof(badResultSizes) / sizeof(BadPointers))


/*****************************************************************************/


typedef struct TransTest
{
    void          *tt_String;
    CharacterSets  tt_StringSet;
    void          *tt_Result;
    CharacterSets  tt_ResultSet;
    int32          tt_ExpectedResult;
    uint8          tt_Filler;
} TransTest;

static TransTest transTests[] =
{
    {"abcd",INTL_CS_ASCII,"\000a\000b\000c\000d\000",INTL_CS_UNICODE,4,'Z'},
    {"\000a\000b\000c\000d\000",INTL_CS_UNICODE,"abcd",INTL_CS_ASCII,4,'Z'},
    {"\000a\000b\000c\010d\000",INTL_CS_UNICODE,"abcZ",INTL_CS_ASCII,4,'Z'},
};


#define NUM_TESTS (sizeof(transTests) / sizeof(TransTest))


/*****************************************************************************/


void TestTransliterateString(Item locItem)
{
Err      result;
unichar  buffer[200];
uint32   i;
unichar *uni1;
unichar *uni2;

    /* check bad source buffer handling */
    for (i = 0; i < NUM_BADSOURCES; i++)
    {
        result = intlTransliterateString((void *)badSources[i].bp_Ptr,INTL_CS_ASCII,buffer,INTL_CS_ASCII,sizeof(buffer),' ');
        CHECKRESULT("TestTransliterateString (sources)",i,badSources[i].bp_ExpectedResult,result);
    }

    /* check bad result buffer handling */
    for (i = 0; i < NUM_BADRESULTS; i++)
    {
        result = intlTransliterateString("",INTL_CS_ASCII,(void *)badResults[i].bp_Ptr,INTL_CS_ASCII,sizeof(buffer),' ');
        CHECKRESULT("TestTransliterateString (result)",i,badResults[i].bp_ExpectedResult,result);
    }

    /* check bad result buffer size handling */
    for (i = 0; i < NUM_BADRESULTSIZES; i++)
    {
        result = intlTransliterateString("",INTL_CS_ASCII,buffer,INTL_CS_ASCII,badResultSizes[i].bp_Ptr,' ');
        CHECKRESULT("TestTransliterateString (result sizes)",i,badResultSizes[i].bp_ExpectedResult,result);
    }

    /* check various conversions */
    for (i = 0; i < NUM_TESTS; i++)
    {
        result = intlTransliterateString(transTests[i].tt_String,transTests[i].tt_StringSet,
                                         buffer,transTests[i].tt_ResultSet,sizeof(buffer),
                                         transTests[i].tt_Filler);

        CHECKRESULT("TestTransliterateString (strings - 1)",i,transTests[i].tt_ExpectedResult,result);

        if (transTests[i].tt_ResultSet == INTL_CS_ASCII)
        {
            if (strcmp((char *)buffer,(char *)transTests[i].tt_Result))
            {
                printf("INTLTEST: TestTransliterateString (strings - 2), test %d, expected '%s', got '%s'\n",i,transTests[i].tt_Result,buffer);
            }
        }
        else if (transTests[i].tt_ResultSet == INTL_CS_UNICODE)
        {
            uni1 = (unichar *)transTests[i].tt_Result;
            uni2 = buffer;
            while (*uni1 == *uni2)
            {
                if (*uni1 == 0)
                    break;

                uni1++;
                uni2++;
            }

            if (*uni1 != *uni2)
            {
                printf("INTLTEST: TestTransliterateString (strings - 2), test %d, expected '",i);
                PrintUniCode((unichar *)transTests[i].tt_Result);
                printf("', got '");
                PrintUniCode(buffer);
                printf("'\n");
            }
        }
    }
}
