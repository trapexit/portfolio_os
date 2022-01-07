/* $Id: convertstring.c,v 1.2 1994/05/11 22:24:29 vertex Exp $ */

/* Check intlConvertString()
 *
 * Bad args:
 *
 *  locItem    - Negative
 *               0
 *               random > 0
 *               known item of a different type
 *
 *  spec       - Negative
 *               0
 *               very large positive
 *
 *  result     - Negative
 *               0
 *               very large positive
 *
 *  resultSize - Negative
 *               0
 *	         1
 *               very large positive
 */


#include "types.h"
#include "debug.h"
#include "stdio.h"
#include "operror.h"
#include "intltest.h"


/*****************************************************************************/


typedef struct ItemTest
{
    Item  it_Item;
    Err   it_ExpectedResult;
} ItemTest;


static ItemTest badItems[] =
{
    {-999,    INTL_ERR_BADITEM},
    {-1,      INTL_ERR_BADITEM},
    {0,       INTL_ERR_BADITEM},
    {3,       INTL_ERR_BADITEM},  /* file folio's item */
    {0x35465, INTL_ERR_BADITEM},
};

#define NUM_BADITEMS (sizeof(badItems) / sizeof(ItemTest))


/*****************************************************************************/


typedef struct BadPointers
{
    uint32 bp_Ptr;
    Err    bp_ExpectedResult;
} BadPointers;


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
    {1,          INTL_ERR_BUFFERTOOSMALL},
    {0x10000000, INTL_ERR_BADRESULTBUFFER},
};

#define NUM_BADRESULTSIZES (sizeof(badResultSizes) / sizeof(BadPointers))


/*****************************************************************************/


typedef struct ConvTest
{
    char   *ct_String;
    char   *ct_ExpectedConvert;
    uint32  ct_Flags;
    uint32  ct_ExpectedResult;
} ConvTest;

static ConvTest convTests[] =
{
   {"ABC", "abc", INTL_CONVF_LOWERCASE, 3},
   {"abc", "ABC", INTL_CONVF_UPPERCASE, 3},
   {"ABC", "ABC", INTL_CONVF_UPPERCASE, 3},
   {"abc", "abc", INTL_CONVF_LOWERCASE, 3},
   {"ábc", "ÁBC", INTL_CONVF_UPPERCASE, 3},
   {"ÁbC", "abc", INTL_CONVF_STRIPDIACRITICALS | INTL_CONVF_LOWERCASE, 3},
};

#define NUM_CONVTESTS (sizeof(convTests) / sizeof(ConvTest))


/*****************************************************************************/


void TestConvertString(Item locItem)
{
Err     result;
uint32  i;
unichar buffer[200];
unichar unicode[200];

    /* check item handling */
    for (i = 0; i < NUM_BADITEMS; i++)
    {
        result = intlConvertString(badItems[i].it_Item,(unichar *)"",(unichar *)"",2,0);
        CHECKRESULT("TestConvertString (items)",i,badItems[i].it_ExpectedResult,result);
    }

    /* check bad source buffer handling */
    for (i = 0; i < NUM_BADSOURCES; i++)
    {
        result = intlConvertString(locItem,(unichar *)badSources[i].bp_Ptr,(unichar *)"",2,0);
        CHECKRESULT("TestConvertString (source)",i,badSources[i].bp_ExpectedResult,result);
    }

    /* check bad result buffer handling */
    for (i = 0; i < NUM_BADRESULTS; i++)
    {
        result = intlConvertString(locItem,(unichar *)"",(unichar *)badResults[i].bp_Ptr,2,0);
        CHECKRESULT("TestConvertString (results)",i,badResults[i].bp_ExpectedResult,result);
    }

    /* check bad result buffer size handling */
    for (i = 0; i < NUM_BADRESULTSIZES; i++)
    {
        result = intlConvertString(locItem,(unichar *)"",(unichar *)"",badResultSizes[i].bp_Ptr,0);
        CHECKRESULT("TestConvertString (result sizes)",i,badResultSizes[i].bp_ExpectedResult,result);
    }

    /* check conversions... */
    for (i = 0; i < NUM_CONVTESTS; i++)
    {
        ASCIIToUniCode(convTests[i].ct_String,unicode);

        result = intlConvertString(locItem,unicode,buffer,sizeof(buffer),convTests[i].ct_Flags);

        CHECKRESULT("TestConvertString (converts - 1)",i,convTests[i].ct_ExpectedResult,result);
        CHECKRESULTSTR("TestConvertString (converts - 2)",i,convTests[i].ct_ExpectedConvert,buffer);
    }
}
