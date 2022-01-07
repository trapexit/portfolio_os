/* $Id: formatnumber.c,v 1.4 1994/11/05 00:40:36 vertex Exp $ */

/* Check intlFormatNumber()
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
 *
 * Number checking:
 *
 *    Passes a bunch of numbers and validates the returned strings
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


static BadPointers badSpecs[] =
{
    {-999,       INTL_ERR_BADNUMERICSPEC},
    {-1,         INTL_ERR_BADNUMERICSPEC},
    {0,          INTL_ERR_BADNUMERICSPEC},
    {0x10000000, INTL_ERR_BADNUMERICSPEC},
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


typedef struct SpecTest
{
    NumericSpec    st_Spec;
    uint32         st_Whole;
    uint32         st_Frac;
    bool           st_Negative;
    bool	   st_DoFrac;
    char          *st_ExpectedNumber;
    int32          st_ExpectedResult;
} SpecTest;


static SpecTest specs[] =
{
    {{0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\0000"},
     32, 150000000, FALSE, TRUE, "32.15", 5
    },

    {{0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\0000"},
     32, 156780000, FALSE, TRUE, "32.15", 5
    },

    {{0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,4,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\0000"},
     32, 156780000, FALSE, TRUE, "32.1567", 7
    },

    {{0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,7,6,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\0000"},
     32, 156780000, FALSE, TRUE, "", INTL_ERR_BADNUMERICSPEC
    },

    {{0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,6,7,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\0000"},
     32, 156780000, FALSE, TRUE, "32.156780", 9
    },

    {{0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\0000"},
     32, 150000000, FALSE, FALSE, "32", 2
    },

    {{0x24924924,(unichar *)"\000,",NULL,0x24924924,NULL,NULL,2,2,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\0000"},
     32, 150000000, FALSE, TRUE, "32", 2
    },

    {{0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,(unichar *)"\000$\000%\000s",2,2,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\0000"},
     32, 150000000, FALSE, TRUE, "$32.15", 6
    },

    {{0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\0000"},
      32, 150000000, TRUE, FALSE, "-32", 3
    },

    {{0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\0000"},
     3200, 150000000, FALSE, FALSE, "3,200", 5
    },

    {{0x24924924,NULL,(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\0000"},
     3200, 150000000, FALSE, FALSE, "3200", 4
    },

    {{0x24924924,NULL,(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\000X\000X\000X"},
     0, 0, FALSE, FALSE, "XXX", 3
    },

    {{0x24924924,NULL,(unichar *)"\000.",0x24924924,NULL,NULL,0,0,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\0000"},
     3200, 150000000, FALSE, TRUE, "3200", 4
    },

    {{0x24924924,NULL,(unichar *)"\000.",0x24924924,NULL,NULL,5,6,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\0000"},
     3200, 150000000, FALSE, TRUE, "3200.15000", 10
    },

    {{0x24924924,NULL,(unichar *)"\000.",0x24924924,(unichar *)"\000,",NULL,5,6,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\0000"},
     3200, 150000000, FALSE, TRUE, "3200.150,00",11
    },

    {{0x24924924,NULL,(unichar *)"\000.",0x24924924,(unichar *)"\000,",NULL,5,6,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,2,2,
      (unichar *)"\0000"},
     3200, 15000000, FALSE, TRUE, "3200.015,00",11
    },

    {{0x24924924,NULL,(unichar *)"\000.",0x24924924,(unichar *)"\000,",NULL,5,6,
      0x24924924,(unichar *)"\000,",(unichar *)"\000.",0x24924924,NULL,NULL,5,6,
      (unichar *)"\0000"},
     0, 15000000, TRUE, TRUE, "-0.01500",8
    },

};


#define NUM_SPECS (sizeof(specs) / sizeof(SpecTest))


/*****************************************************************************/


void TestFormatNumber(Item locItem)
{
Err         result;
unichar     buffer[100];
uint32      i;
NumericSpec spec;

    spec.ns_PosGroups               = 0;
    spec.ns_PosGroupSep             = (unichar *)"";
    spec.ns_PosRadix                = (unichar *)"";
    spec.ns_PosFractionalGroups     = 0;
    spec.ns_PosFractionalGroupSep   = (unichar *)"";
    spec.ns_PosFormat               = (unichar *)"";
    spec.ns_PosMinFractionalDigits  = 0;
    spec.ns_PosMaxFractionalDigits  = 0;
    spec.ns_NegGroups               = 0;
    spec.ns_NegGroupSep             = (unichar *)"";
    spec.ns_NegRadix                = (unichar *)"";
    spec.ns_NegFractionalGroups     = 0;
    spec.ns_NegFractionalGroupSep   = (unichar *)"";
    spec.ns_NegFormat               = (unichar *)"";
    spec.ns_NegMinFractionalDigits  = 0;
    spec.ns_NegMaxFractionalDigits  = 0;
    spec.ns_Zero                    = (unichar *)"";
    spec.ns_Flags                   = 0;

    /* check item handling */
    for (i = 0; i < NUM_BADITEMS; i++)
    {
        result = intlFormatNumber(badItems[i].it_Item,&spec,0,0,FALSE,FALSE,buffer,sizeof(buffer));
        CHECKRESULT("TestFormatNumber (items)",i,badItems[i].it_ExpectedResult,result);
    }

    /* check bad numeric spec handling */
    for (i = 0; i < NUM_BADSPECS; i++)
    {
        result = intlFormatNumber(locItem,(NumericSpec *)badSpecs[i].bp_Ptr,0,0,FALSE,FALSE,buffer,sizeof(buffer));
        CHECKRESULT("TestFormatNumber (specs)",i,badSpecs[i].bp_ExpectedResult,result);
    }

    /* check bad result buffer handling */
    for (i = 0; i < NUM_BADRESULTS; i++)
    {
        result = intlFormatNumber(locItem,&spec,0,0,FALSE,FALSE,(unichar *)badResults[i].bp_Ptr,sizeof(buffer));
        CHECKRESULT("TestFormatNumber (result buffer)",i,badResults[i].bp_ExpectedResult,result);
    }

    /* check bad result buffer size handling */
    for (i = 0; i < NUM_BADRESULTSIZES; i++)
    {
        result = intlFormatNumber(locItem,&spec,0,0,FALSE,FALSE,buffer,(uint32)badResultSizes[i].bp_Ptr);
        CHECKRESULT("TestFormatNumber (result buffer size)",i,badResultSizes[i].bp_ExpectedResult,result);
    }

    /* check various number specs */
    for (i = 0; i < NUM_SPECS; i++)
    {
        buffer[0] = 0;

        result = intlFormatNumber(locItem,&specs[i].st_Spec,specs[i].st_Whole,specs[i].st_Frac,specs[i].st_Negative,specs[i].st_DoFrac,buffer,sizeof(buffer));
        CHECKRESULT("TestFormatNumber (specs - 1)",i,specs[i].st_ExpectedResult,result);
        CHECKRESULTSTR("TestFormatNumber (specs - 2)",i,specs[i].st_ExpectedNumber,buffer);
    }
}
