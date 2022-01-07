/* $Id: comparestrings.c,v 1.2 1994/05/11 21:23:54 vertex Exp $ */

/* Check intlCompareStrings()
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


typedef struct CompTest
{
    char *ct_String1;
    char *ct_String2;
    Err   ct_ExpectedResult;
} CompTest;

static CompTest compTests[] =
{
    {"abc",  "abc", 0},
    {"ab",   "abc", -1},
    {"abc",  "ab",  1},
    {"abcc", "abc", 1},
    {"Abc",  "abc", -1},
    {"ABc",  "ABC", 1},
    {"ábc",  "abc", 1},
    {"Æbc",  "aebc", -1},
    {"æbc",  "AEbc", 1},
    {"bæc",  "baed", -1},
    {"bæc",  "baeb", 1},
    {"ab-c", "abd", -1},
    {"ab-c", "abc", -1},
    {"ab-c", "aba", 1},
};

#define NUM_COMPTESTS (sizeof(compTests) / sizeof(CompTest))


/*****************************************************************************/


void TestCompareStrings(Item locItem)
{
Err     result;
uint32  i;
unichar buf1[200];
unichar buf2[200];

    /* check item handling */
    for (i = 0; i < NUM_BADITEMS; i++)
    {
        result = intlCompareStrings(badItems[i].it_Item,(unichar *)"",(unichar *)"");
        CHECKRESULT("TestCompareStrings (items)",i,badItems[i].it_ExpectedResult,result);
    }

    /* check bad source buffer handling */
    for (i = 0; i < NUM_BADSOURCES; i++)
    {
        result = intlCompareStrings(locItem,(unichar *)badSources[i].bp_Ptr,(unichar *)"");
        CHECKRESULT("TestCompareStrings (string1)",i,badSources[i].bp_ExpectedResult,result);
    }

    /* check bad source buffer handling */
    for (i = 0; i < NUM_BADSOURCES; i++)
    {
        result = intlCompareStrings(locItem,(unichar *)"",(unichar *)badSources[i].bp_Ptr);
        CHECKRESULT("TestCompareStrings (string2)",i,badSources[i].bp_ExpectedResult,result);
    }

    /* check comparisons... */
    for (i = 0; i < NUM_COMPTESTS; i++)
    {
        ASCIIToUniCode(compTests[i].ct_String1,buf1);
        ASCIIToUniCode(compTests[i].ct_String2,buf2);

        result = intlCompareStrings(locItem,buf1,buf2);
        CHECKRESULT("TestCompareStrings (compares)",i,compTests[i].ct_ExpectedResult,result);
    }
}
