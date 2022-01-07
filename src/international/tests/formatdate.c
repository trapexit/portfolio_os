/* $Id: formatdate.c,v 1.5 1994/12/08 00:12:29 vertex Exp $ */

/* Check intlFormatDate()
 *
 * Bad args:
 *
 *    locItem    - Negative
 *                 0
 *                 random > 0
 *                 known item of a different type
 *
 *    dateSpec   - Negative
 *                 0
 *                 very large positive
 *
 *    date       - Negative
 *                 0
 *                 very large positive
 *
 *    result     - Negative
 *                 0
 *                 very large positive
 *
 *    resultSize - Negative
 *                 0
 *                 1
 *                 very large positive
 *
 * Date checking:
 *
 *    Passes a bunch of dates and validates the returned strings
 *
 */


#include "types.h"
#include "debug.h"
#include "stdio.h"
#include "operror.h"
#include "intltest.h"


/*****************************************************************************/


typedef struct DateTest
{
    GregorianDate dt_Date;
    Err           dt_ExpectedResult;
} DateTest;

static DateTest dates[] =
{
    {{0,0,0,0,0,0},          INTL_ERR_IMPOSSIBLEDATE},
    {{0xfffff,1,1,0,0,0}, 0},
    {{0,1,0,0,0,0},          INTL_ERR_IMPOSSIBLEDATE},
    {{0,0,1,0,0,0},          INTL_ERR_IMPOSSIBLEDATE},
    {{1,1,1,0,0,0},          0},
    {{1,13,1,0,0,0},         INTL_ERR_IMPOSSIBLEDATE},
    {{1,234,1,0,0,0},        INTL_ERR_IMPOSSIBLEDATE},
    {{1,0xffffffff,1,0,0,0}, INTL_ERR_IMPOSSIBLEDATE},
    {{1,1,32,0,0,0},         INTL_ERR_IMPOSSIBLEDATE},
    {{1,1,1,23,0,0},         0},
    {{1,1,1,24,0,0},         INTL_ERR_IMPOSSIBLEDATE},
    {{1,1,1,255,0,0},        INTL_ERR_IMPOSSIBLEDATE},
    {{1,1,1,0,59,0},         0},
    {{1,1,1,0,60,0},         INTL_ERR_IMPOSSIBLEDATE},
    {{1,1,1,0,255,0},        INTL_ERR_IMPOSSIBLEDATE},
    {{1,1,1,0,0,59},         0},
    {{1,1,1,0,0,60},         INTL_ERR_IMPOSSIBLEDATE},
    {{1,1,1,0,0,255},        INTL_ERR_IMPOSSIBLEDATE},
    {{1992,2,29,0,0,0},      0},
    {{1993,2,29,0,0,0},      INTL_ERR_IMPOSSIBLEDATE},

    /* one too many days in each month */
    {{1994,1,32,0,0,0},      INTL_ERR_IMPOSSIBLEDATE},
    {{1994,2,29,0,0,0},      INTL_ERR_IMPOSSIBLEDATE},
    {{1994,3,32,0,0,0},      INTL_ERR_IMPOSSIBLEDATE},
    {{1994,4,31,0,0,0},      INTL_ERR_IMPOSSIBLEDATE},
    {{1994,5,32,0,0,0},      INTL_ERR_IMPOSSIBLEDATE},
    {{1994,6,31,0,0,0},      INTL_ERR_IMPOSSIBLEDATE},
    {{1994,7,32,0,0,0},      INTL_ERR_IMPOSSIBLEDATE},
    {{1994,8,32,0,0,0},      INTL_ERR_IMPOSSIBLEDATE},
    {{1994,9,31,0,0,0},      INTL_ERR_IMPOSSIBLEDATE},
    {{1994,10,32,0,0,0},     INTL_ERR_IMPOSSIBLEDATE},
    {{1994,11,31,0,0,0},     INTL_ERR_IMPOSSIBLEDATE},
    {{1994,12,32,0,0,0},     INTL_ERR_IMPOSSIBLEDATE},

    /* maximum number of days in each month */
    {{1994,1,31,0,0,0},      0},
    {{1994,2,28,0,0,0},      0},
    {{1994,3,31,0,0,0},      0},
    {{1994,4,30,0,0,0},      0},
    {{1994,5,31,0,0,0},      0},
    {{1994,6,30,0,0,0},      0},
    {{1994,7,31,0,0,0},      0},
    {{1994,8,31,0,0,0},      0},
    {{1994,9,30,0,0,0},      0},
    {{1994,10,31,0,0,0},     0},
    {{1994,11,30,0,0,0},     0},
    {{1994,12,31,0,0,0},     0},

    {{1994,12,31,23,59,59},  0},
    {{0xfffff,12,31,23,59,59}, 0},

    /* and make sure my birthday works! */
    {{1967,7,21,6,55,30},    0},
};

#define NUM_DATES (sizeof(dates) / sizeof(DateTest))


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


static BadPointers badSpecs[] =
{
    {-999,       INTL_ERR_BADDATESPEC},
    {-1,         INTL_ERR_BADDATESPEC},
    {0,          INTL_ERR_BADDATESPEC},
    {0x10000000, INTL_ERR_BADDATESPEC},
};

#define NUM_BADSPECS (sizeof(badSpecs) / sizeof(BadPointers))


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


static BadPointers badDates[] =
{
    {-999,       INTL_ERR_BADGREGORIANDATE},
    {-1,         INTL_ERR_BADGREGORIANDATE},
    {0,          INTL_ERR_BADGREGORIANDATE},
    {0x10000000, INTL_ERR_BADGREGORIANDATE},
};

#define NUM_BADDATES (sizeof(badDates) / sizeof(BadPointers))


/*****************************************************************************/


typedef struct SpecTest
{
    char          *st_Spec;
    GregorianDate  st_Date;
    char          *st_ExpectedDate;
    int32          st_ExpectedResult;
} SpecTest;

static SpecTest specs[] =
{
    {"%D",
     {1994,5,6,12,13,14},
     "6",
     1
    },

    {"%H",
     {1994,5,6,12,13,14},
     "12",
     2
    },

    {"%h",
     {1994,5,6,12,13,14},
     "12",
     2
    },

    {"%H",
     {1994,5,6,13,13,14},
     "13",
     2
    },

    {"%h",
     {1994,5,6,13,13,14},
     "1",
     1
    },

    {"%M",
     {1994,5,6,12,13,14},
     "13",
     2
    },

    {"%N",
     {1994,9,6,12,13,14},
     "September",
     9
    },

    {"%n",
     {1994,9,6,12,13,14},
     "Sep",
     3
    },

    {"%O",
     {1994,5,6,12,13,14},
     "5",
     1
    },

    {"%P",
     {1994,5,6,0,13,14},
     "AM",
     2
    },

    {"%P",
     {1994,5,6,11,13,14},
     "AM",
     2
    },

    {"%P",
     {1994,5,6,23,13,14},
     "PM",
     2
    },

    {"%P",
     {1994,5,6,12,13,14},
     "PM",
     2
    },

    {"%S",
     {1994,5,6,12,13,14},
     "14",
     2
    },

    {"%W",
     {1994,5,6,12,13,14},
     "Friday",
     6
    },

    {"%w",
     {1994,5,6,12,13,14},
     "Fri",
     3
    },

    {"%Y",
     {1994,5,6,12,13,14},
     "1994",
     4
    },

    {"%%",
     {1994,5,6,12,13,14},
     "%",
     1
    },

    {"%Z",
     {1994,5,6,12,13,14},
     "%Z",
     2
    },

    {"%0Z",
     {1994,5,6,12,13,14},
     "%0Z",
     3
    },

    {"%6W",
     {1994,5,6,12,13,14},
     "Friday",
     6
    },

    {"%10W",
     {1994,5,6,12,13,14},
     "    Friday",
     10
    },

    {"%-10W",
     {1994,5,6,12,13,14},
     "Friday    ",
     10
    },

    {"%.W",
     {1994,5,6,12,13,14},
     "",
     0
    },

    {"%6.W",
     {1994,5,6,12,13,14},
     "      ",
     6
    },

    {"%.3W",
     {1994,5,6,12,13,14},
     "day",
     3
    },

    {"%-010W",
     {1994,5,6,12,13,14},
     "Friday    ",
     10
    },

    {"%010W",
     {1994,5,6,12,13,14},
     "0000Friday",
     10
    },

    {"%D",
     {1992,2,29,12,13,14},
     "29",
     2
    },
};

#define NUM_SPECS (sizeof(specs) / sizeof(SpecTest))



/*****************************************************************************/


void TestFormatDate(Item locItem)
{
Err           result;
unichar       buffer[100];
uint32        i;
GregorianDate date;
unichar       unicode[200];

    date.gd_Year   = 0;
    date.gd_Month  = 1;
    date.gd_Day    = 1;
    date.gd_Hour   = 0;
    date.gd_Minute = 0;
    date.gd_Second = 0;

    /* check item handling */
    for (i = 0; i < NUM_BADITEMS; i++)
    {
        result = intlFormatDate(badItems[i].it_Item,(unichar *)"",&date,buffer,sizeof(buffer));
        CHECKRESULT("TestFormatDate (items)",i,badItems[i].it_ExpectedResult,result);
    }

    /* check bad date spec handling */
    for (i = 0; i < NUM_BADSPECS; i++)
    {
        result = intlFormatDate(locItem,(unichar *)badSpecs[i].bp_Ptr,&date,buffer,sizeof(buffer));
        CHECKRESULT("TestFormatDate (specs)",i,badSpecs[i].bp_ExpectedResult,result);
    }

    /* check bad result buffer handling */
    for (i = 0; i < NUM_BADRESULTS; i++)
    {
        result = intlFormatDate(locItem,(unichar *)"",&date,(unichar *)badResults[i].bp_Ptr,sizeof(buffer));
        CHECKRESULT("TestFormatDate (result buffer)",i,badResults[i].bp_ExpectedResult,result);
    }

    /* check bad result buffer size handling */
    for (i = 0; i < NUM_BADRESULTSIZES; i++)
    {
        result = intlFormatDate(locItem,(unichar *)"",&date,buffer,(uint32)badResultSizes[i].bp_Ptr);
        CHECKRESULT("TestFormatDate (result buffer size)",i,badResultSizes[i].bp_ExpectedResult,result);
    }

    /* check bad gregorian date handling */
    for (i = 0; i < NUM_BADDATES; i++)
    {
        result = intlFormatDate(locItem,(unichar *)"",(GregorianDate *)badDates[i].bp_Ptr,buffer,sizeof(buffer));
        CHECKRESULT("TestFormatDate (greg. dates)",i,badDates[i].bp_ExpectedResult,result);
    }

    /* check impossible date handling */
    for (i = 0; i < NUM_DATES; i++)
    {
        result = intlFormatDate(locItem,(unichar *)"",&dates[i].dt_Date,buffer,sizeof(buffer));
        if (result >= 0)
            result = 0;

        CHECKRESULT("TestFormatDate (dates)",i,dates[i].dt_ExpectedResult,result);
    }

    /* check various date specs */
    for (i = 0; i < NUM_SPECS; i++)
    {
        ASCIIToUniCode(specs[i].st_Spec,unicode);

        result = intlFormatDate(locItem,unicode,&specs[i].st_Date,buffer,sizeof(buffer));
        CHECKRESULT("TestFormatDate (specs - 1)",i,specs[i].st_ExpectedResult,result);
        CHECKRESULTSTR("TestFormatDate (specs - 2)",i,specs[i].st_ExpectedDate,buffer);
    }
}
