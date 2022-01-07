/* $Id: gencountrydb.c,v 1.18 1995/01/06 18:30:13 vertex Exp $ */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include "intl.h"
#include "jstring.h"
#include "countrydb.h"

/* This is a horrible kludge to get the shift-JIS conversion routines
 * from the JString folio. Since this program runs under UNIX and not
 * Portfolio, we can't use the actual folio itself.
 */
#include "../../jstring/folio/jconversions.c"
#include "../../jstring/folio/tables.c"


/*****************************************************************************/


/* convert a "binary" byte into a usable form */
#define BINBYTE(x) ((uint32)(((x & 1) ? 1 : 0)\
                   | ((x & 0x10) ? 2 : 0)\
                   | ((x & 0x100) ? 4 : 0)\
                   | ((x & 0x1000) ? 8 : 0)\
                   | ((x & 0x10000) ? 16 : 0)\
                   | ((x & 0x100000) ? 32 : 0)\
                   | ((x & 0x1000000) ? 64 : 0)\
                   | ((x & 0x10000000) ? 128 : 0)))

/* convert 4 "binary" bytes into usable form */
#define BIN(a,b,c,d) ((BINBYTE(a) << 24) | (BINBYTE(b) << 16) | (BINBYTE(c) << 8) | BINBYTE(d))


/*****************************************************************************/


static Locale UnitedStates =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_UNITED_STATES,                 /* loc_Country         */
    -5*60,                                    /* loc_GMTOffset       */
    INTL_MS_AMERICAN,                         /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_SUNDAY,                 /* loc_CalendarType    */
    INTL_DT_RIGHTHAND,                        /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)",",                           /* ns_PosGroupSep            */
    (unichar *)".",                           /* ns_PosRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosFractionalGroups    */
    (unichar *)",",                           /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)",",                           /* ns_NegGroupSep            */
    (unichar *)".",                           /* ns_NegRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegFractionalGroups    */
    (unichar *)",",                           /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)",",                           /* ns_PosGroupSep            */
    (unichar *)".",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"$%s",                         /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)",",                           /* ns_NegGroupSep            */
    (unichar *)".",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-$%s",                        /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)",",                           /* ns_PosGroupSep            */
    NULL,                                     /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%s¢",                         /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)",",                           /* ns_NegGroupSep            */
    NULL,                                     /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%s¢",                        /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%W %N %D %04Y",

    /* loc_ShortDate       */
    (unichar *)"%02O/%02D/%02Y",

    /* loc_Time            */
    (unichar *)"%h:%02M:%02S %P",

    /* loc_ShortTime       */
    (unichar *)"%h:%02M %P"
};

static Locale Japan =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_JAPAN,                         /* loc_Country         */
    9*60,                                     /* loc_GMTOffset       */
    INTL_MS_METRIC,                           /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_SUNDAY,                 /* loc_CalendarType    */
    INTL_DT_LEFTHAND,                         /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)",",                           /* ns_PosGroupSep            */
    (unichar *)".",                           /* ns_PosRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosFractionalGroups    */
    (unichar *)",",                           /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)",",                           /* ns_NegGroupSep            */
    (unichar *)".",                           /* ns_NegRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegFractionalGroups    */
    (unichar *)",",                           /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)",",                           /* ns_PosGroupSep            */
    (unichar *)".",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"\\%s",                        /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)",",                           /* ns_NegGroupSep            */
    (unichar *)".",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-\\%s",                       /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)",",                           /* ns_PosGroupSep            */
    NULL,                                     /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)",",                           /* ns_NegGroupSep            */
    NULL,                                     /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%04Y”N%N%D“ú %W",

    /* loc_ShortDate       */
    (unichar *)"%02Y/%02O/%02D",

    /* loc_Time            */
    (unichar *)"%P %hŽž%02M•ª%02S•b",

    /* loc_ShortTime       */
    (unichar *)"%P %h:%02M"
};

static Locale Netherlands =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_NETHERLANDS,                    /* loc_Country         */
    -5*60,                                    /* loc_GMTOffset       */
    INTL_MS_METRIC,                           /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_MONDAY,                 /* loc_CalendarType    */
    INTL_DT_RIGHTHAND,                        /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"fl %s",                       /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"fl %s-",                      /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    NULL,                                     /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"c %s",                        /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    NULL,                                     /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"c %s-",                       /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%W, %D %N,%04Y",

    /* loc_ShortDate       */
    (unichar *)"%D-%02O-%04Y",

    /* loc_Time            */
    (unichar *)"%02H:%02M:%02S",

    /* loc_ShortTime       */
    (unichar *)"%02H:%02M"
};

static Locale Switzerland =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_SWITZERLAND,                   /* loc_Country         */
    1*60,                                     /* loc_GMTOffset       */
    INTL_MS_METRIC,                           /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_MONDAY,                 /* loc_CalendarType    */
    INTL_DT_RIGHTHAND,                        /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)"'",                           /* ns_PosGroupSep            */
    (unichar *)".",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)"'",                           /* ns_NegGroupSep            */
    (unichar *)".",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)"'",                           /* ns_PosGroupSep            */
    (unichar *)".",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"SFr.%s",                      /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)"'",                           /* ns_NegGroupSep            */
    (unichar *)".",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"SFr.-$%s",                    /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)"'",                           /* ns_PosGroupSep            */
    NULL,                                     /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"ct.%s",                       /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)"'",                           /* ns_NegGroupSep            */
    NULL,                                     /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"ct.-%s¢",                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%W %D %N %04Y",

    /* loc_ShortDate       */
    (unichar *)"%D.%02O.%04Y",

    /* loc_Time            */
    (unichar *)"%02H:%02M:%02S",

    /* loc_ShortTime       */
    (unichar *)"%02H:%02M"
};

static Locale Sweden =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_SWEDEN,                        /* loc_Country         */
    1*60,                                     /* loc_GMTOffset       */
    INTL_MS_METRIC,                           /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_MONDAY,                 /* loc_CalendarType    */
    INTL_DT_RIGHTHAND,                        /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosFractionalGroups    */
    (unichar *)".",                           /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegFractionalGroups    */
    (unichar *)".",                           /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%skr",                        /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%skr",                       /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    NULL,                                     /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%s öre",                      /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    NULL,                                     /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%s öre",                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%04Y-%02O-%02D",

    /* loc_ShortDate       */
    (unichar *)"%04Y-%02O-%02D",

    /* loc_Time            */
    (unichar *)"%02H.%02M.%02S",

    /* loc_ShortTime       */
    (unichar *)"%02H.%02M.%02S"
};

static Locale Portugal =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_PORTUGAL,                      /* loc_Country         */
    0*60,                                    /* loc_GMTOffset       */
    INTL_MS_METRIC,                           /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_MONDAY,                 /* loc_CalendarType    */
    INTL_DT_RIGHTHAND,                        /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)"'",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosFractionalGroups    */
    (unichar *)"'",                           /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)"'",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegFractionalGroups    */
    (unichar *)"'",                           /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)"'",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"+$ %s",                       /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)"'",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-$ %s",                       /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)"'",                           /* ns_PosGroupSep            */
    NULL,                                     /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"+$ %s",                       /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)"'",                           /* ns_NegGroupSep            */
    NULL,                                     /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-$ %s",                       /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%W, %D de %N de %04Y",

    /* loc_ShortDate       */
    (unichar *)"%D %N %04Y",

    /* loc_Time            */
    (unichar *)"%02H:%02M:%02S",

    /* loc_ShortTime       */
    (unichar *)"%02H:%02M"
};

static Locale Denmark =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_DENMARK,                       /* loc_Country         */
    1*60,                                     /* loc_GMTOffset       */
    INTL_MS_METRIC,                           /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_MONDAY,                 /* loc_CalendarType    */
    INTL_DT_RIGHTHAND,                        /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosFractionalGroups    */
    (unichar *)".",                           /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegFractionalGroups    */
    (unichar *)".",                           /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%sKr",                        /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%sKr",                       /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    NULL,                                     /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%s øre",                      /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    NULL,                                     /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%s øre",                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%D. %N %04Y",

    /* loc_ShortDate       */
    (unichar *)"%04Y/%02O/%02D",

    /* loc_Time            */
    (unichar *)"%02H:%02M:%02S",

    /* loc_ShortTime       */
    (unichar *)"%02H:%02M:%02S"
};

static Locale Norway =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_NORWAY,                        /* loc_Country         */
    1*60,                                     /* loc_GMTOffset       */
    INTL_MS_METRIC,                           /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_MONDAY,                 /* loc_CalendarType    */
    INTL_DT_RIGHTHAND,                        /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosFractionalGroups    */
    (unichar *)".",                           /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegFractionalGroups    */
    (unichar *)".",                           /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%sKr.",                       /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%sKr.",                      /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    NULL,                                     /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%sKr.",                       /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    NULL,                                     /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%sKr.",                      /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%D %N %04Y",

    /* loc_ShortDate       */
    (unichar *)"%D.%n.%02Y",

    /* loc_Time            */
    (unichar *)"%02H.%02M.%02S",

    /* loc_ShortTime       */
    (unichar *)"%02H.%02M.%02S"
};

static Locale Austria =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_AUSTRIA,                       /* loc_Country         */
    1*60,                                     /* loc_GMTOffset       */
    INTL_MS_METRIC,                           /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_MONDAY,                 /* loc_CalendarType    */
    INTL_DT_RIGHTHAND,                        /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"ÖS%s",                         /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-ÖS%s",                       /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    NULL,                                     /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%sg",                         /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    NULL,                                     /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%sg",                        /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%W, %D. %N %04Y",

    /* loc_ShortDate       */
    (unichar *)"%04Y-%02O-%02D",

    /* loc_Time            */
    (unichar *)"%02H:%02M:%02S",

    /* loc_ShortTime       */
    (unichar *)"%02H:%02M"
};

static Locale Spain =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_SPAIN,                         /* loc_Country         */
    1*60,                                     /* loc_GMTOffset       */
    INTL_MS_METRIC,                           /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_MONDAY,                 /* loc_CalendarType    */
    INTL_DT_RIGHTHAND,                        /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)",",                           /* ns_PosGroupSep            */
    (unichar *)"'",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)",",                           /* ns_NegGroupSep            */
    (unichar *)"'",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%s Pesetas",                  /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%s Pesetas",                        /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%s Pesetas",                  /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%s Pesetas",                 /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%W %N %D %04Y",

    /* loc_ShortDate       */
    (unichar *)"%02O/%02D/%02Y",

    /* loc_Time            */
    (unichar *)"%h:%02M:%02S %P",

    /* loc_ShortTime       */
    (unichar *)"%h:%02M %P"
};

static Locale Belgium =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_BELGIUM,                       /* loc_Country         */
    1*60,                                     /* loc_GMTOffset       */
    INTL_MS_METRIC,                           /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_MONDAY,                 /* loc_CalendarType    */
    INTL_DT_RIGHTHAND,                        /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%sFB",                        /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%sFB",                       /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%sFB",                        /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%sFB",                       /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%D-%n-%04Y",

    /* loc_ShortDate       */
    (unichar *)"%02D/%02O/%02Y",

    /* loc_Time            */
    (unichar *)"%02Hh%02M",

    /* loc_ShortTime       */
    (unichar *)"%02Hh%02M"
};

static Locale Germany =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_GERMANY,                       /* loc_Country         */
    1*60,                                    /* loc_GMTOffset       */
    INTL_MS_METRIC,                           /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_MONDAY,                 /* loc_CalendarType    */
    INTL_DT_RIGHTHAND,                        /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosFractionalGroups    */
    (unichar *)".",                           /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegFractionalGroups    */
    (unichar *)".",                           /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%s DM",                         /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%s DM",                        /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    NULL,                                     /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%s Pf",                         /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    NULL,                                     /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%s Pf",                      /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%W, %D. %N %04Y",

    /* loc_ShortDate       */
    (unichar *)"%02D.%02O.%02Y",

    /* loc_Time            */
    (unichar *)"%02H:%02M:%02S",

    /* loc_ShortTime       */
    (unichar *)"%02H:%02M:%02S"
};

static Locale Italy =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_ITALY,                         /* loc_Country         */
    1*60,                                     /* loc_GMTOffset       */
    INTL_MS_METRIC,                           /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_SUNDAY,                 /* loc_CalendarType    */
    INTL_DT_RIGHTHAND,                        /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    NULL,                                     /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"Lire %s",                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    NULL,                                     /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"Lire -%s",                    /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)".",                           /* ns_PosGroupSep            */
    NULL,                                     /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"Lire %s",                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)".",                           /* ns_NegGroupSep            */
    NULL,                                     /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"Lire -%s",                    /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%W %D %N %04Y",

    /* loc_ShortDate       */
    (unichar *)"%D-%N-%04Y",

    /* loc_Time            */
    (unichar *)"%H:%02M:%02S",

    /* loc_ShortTime       */
    (unichar *)"%H:%02M:%02S"
};

static Locale France =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_FRANCE,                        /* loc_Country         */
    1*60,                                     /* loc_GMTOffset       */
    INTL_MS_METRIC,                           /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_MONDAY,                 /* loc_CalendarType    */
    INTL_DT_RIGHTHAND,                        /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)" ",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosFractionalGroups    */
    (unichar *)" ",                           /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)" ",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegFractionalGroups    */
    (unichar *)" ",                           /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)" ",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%sF",                         /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)" ",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%sF",                        /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)" ",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%sF",                         /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)" ",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%sF",                        /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%W %D %N %04Y",

    /* loc_ShortDate       */
    (unichar *)"%02D/%02O/%02Y",

    /* loc_Time            */
    (unichar *)"%02Hh%02M",

    /* loc_ShortTime       */
    (unichar *)"%02Hh%02M"
};

static Locale UnitedKingdom =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_UNITED_KINGDOM,                /* loc_Country         */
    0,                                        /* loc_GMTOffset       */
    INTL_MS_IMPERIAL,                         /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_MONDAY,                 /* loc_CalendarType    */
    INTL_DT_LEFTHAND,                         /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)",",                           /* ns_PosGroupSep            */
    (unichar *)".",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)",",                           /* ns_NegGroupSep            */
    (unichar *)".",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)",",                           /* ns_PosGroupSep            */
    (unichar *)".",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"£%s",                         /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)",",                           /* ns_NegGroupSep            */
    (unichar *)".",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"(£%s)",                       /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)",",                           /* ns_PosGroupSep            */
    NULL,                                     /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%sp",                         /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)",",                           /* ns_NegGroupSep            */
    NULL,                                     /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"(%sp)",                       /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%W %D %N %04Y",

    /* loc_ShortDate       */
    (unichar *)"%02D/%02O/%02Y",

    /* loc_Time            */
    (unichar *)"%02H:%02M:%02S",

    /* loc_ShortTime       */
    (unichar *)"%02H:%02M"
};

static Locale Australia =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_AUSTRALIA,                     /* loc_Country         */
    10*60,                                    /* loc_GMTOffset       */
    INTL_MS_METRIC,                           /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_SUNDAY,                 /* loc_CalendarType    */
    INTL_DT_LEFTHAND,                         /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)" ",                           /* ns_PosGroupSep            */
    (unichar *)".",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)" ",                           /* ns_NegGroupSep            */
    (unichar *)".",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)" ",                           /* ns_PosGroupSep            */
    (unichar *)".",                           /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"$ %s",                        /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)" ",                           /* ns_NegGroupSep            */
    (unichar *)".",                           /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"($%s)",                       /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)",",                           /* ns_PosGroupSep            */
    NULL,                                     /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%sc",                         /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)",",                           /* ns_NegGroupSep            */
    NULL,                                     /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%sc",                        /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%W %N %D %04Y",

    /* loc_ShortDate       */
    (unichar *)"%2D/%02O/%02.2Y",

    /* loc_Time            */
    (unichar *)"%02h:%02M:%02S%P",

    /* loc_ShortTime       */
    (unichar *)"%02h:%02M%P"
};

static Locale Canada =
{
    {0},                                      /* loc_Item            */
    0,                                        /* loc_Language        */
    NULL,                                     /* loc_Dialects        */
    INTL_CNTRY_CANADA,                     /* loc_Country         */
    10*60,                                    /* loc_GMTOffset       */
    INTL_MS_METRIC,                           /* loc_MeasuringSystem */
    INTL_CT_GREGORIAN_SUNDAY,                 /* loc_CalendarType    */
    INTL_DT_RIGHTHAND,                         /* loc_DrivingType     */

    /* loc_Numbers */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)" ",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100),    /* ns_PosFractionalGroups    */
    (unichar *)" ",                                     /* ns_PosFractionalGroupSep  */
    NULL,                                     /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0xffffffff,                               /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)" ",                           /* ns_NegGroupSep            */
    (unichar *)",",                           /* ns_NegRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegFractionalGroups    */
    (unichar *)" ",                           /* ns_NegFractionalGroupSep  */
    NULL,                                     /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0xffffffff,                               /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Currency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)" ",                           /* ns_PosGroupSep            */
    (unichar *)",",                           /* ns_PosRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosFractionalGroups    */
    (unichar *)" ",                           /* ns_PosFractionalGroupSep  */
    (unichar *)"$%s",                         /* ns_PosFormat              */
    2,                                        /* ns_PosMinFractionalDigits */
    2,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)" ",                           /* ns_NegGroupSep            */
    (unichar *)".",                           /* ns_NegRadix               */
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegFractionalGroups    */
    (unichar *)" ",                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-$%s",                        /* ns_NegFormat              */
    2,                                        /* ns_NegMinFractionalDigits */
    2,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_SmallCurrency */
    {
    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_PosGroups              */
    (unichar *)",",                           /* ns_PosGroupSep            */
    NULL,                                     /* ns_PosRadix               */
    0,                                        /* ns_PosFractionalGroups    */
    NULL,                                     /* ns_PosFractionalGroupSep  */
    (unichar *)"%s¢",                         /* ns_PosFormat              */
    0,                                        /* ns_PosMinFractionalDigits */
    0,                                        /* ns_PosMaxFractionalDigits */

    BIN(0x00100100,0x10010010,0x01001001,0x00100100), /* ns_NegGroups              */
    (unichar *)",",                           /* ns_NegGroupSep            */
    NULL,                                     /* ns_NegRadix               */
    0,                                        /* ns_NegFractionalGroups    */
    NULL,                                     /* ns_NegFractionalGroupSep  */
    (unichar *)"-%s¢",                        /* ns_NegFormat              */
    0,                                        /* ns_NegMinFractionalDigits */
    0,                                        /* ns_NegMaxFractionalDigits */
    NULL,                                     /* ns_Zero                   */
    0                                         /* ns_Flags                  */
    },

    /* loc_Date            */
    (unichar *)"%W %N %D %04Y",

    /* loc_ShortDate       */
    (unichar *)"%02D/%02O/%02.2Y",

    /* loc_Time            */
    (unichar *)"%02H:%02M:%02S",

    /* loc_ShortTime       */
    (unichar *)"%02H:%02M"
};


/****************************************************************************/


static Locale *countryInfo[] =
{
    &Australia,
    &Austria,
    &Belgium,
    &Canada,
    &Denmark,
    &France,
    &Germany,
    &Italy,
    &Japan,
    &Netherlands,
    &Norway,
    &Portugal,
    &Spain,
    &Sweden,
    &Switzerland,
    &UnitedKingdom,
    &UnitedStates
};

#define NUM_COUNTRIES (sizeof(countryInfo) / sizeof(Locale *))

static CountryEntry ce[NUM_COUNTRIES+1];
static CountryCodes currentCountry;


/*****************************************************************************/


uint32 unilen(unichar* uni)
{
uint32	len;

    if (uni == NULL)
        return 0;

    for (len = 0; *uni; ++uni)
        ++len;

    return len;
}


/*****************************************************************************/


void ASCIIToUniCode(unsigned char *asc, unichar *uni)
{
    while (*asc)
    {
        *uni++ = (unichar)*asc++;
    }

    *uni = 0;
}


/*****************************************************************************/


static void WriteString(FILE *file, unichar *str)
{
uint8    wlen;
uint32   i, len;
char    *ptr;
unichar  unibuf[128];

    ptr = (char *)str;
    if (ptr)
    {
        if (currentCountry == INTL_CNTRY_JAPAN )
        {
            ConvertShiftJIS2UniCode( ptr, unibuf, sizeof(unibuf), ' ');
        }
        else
        {
            ASCIIToUniCode( ptr, unibuf);
        }

        len = unilen(unibuf) + 1;  /* +1 to include terminator bytes */
    }
    else
    {
        len = 0;
    }

    wlen = ((len*2 + 3) / 4);
    fwrite(&wlen,1,1,file);

    for (i = 0; i < len; i++)
    {
        fwrite(&unibuf[i],2,1,file);    /* write a unicode */
    }

    if ((len * 2) / 4 < wlen)
    {
        fwrite("",1,1,file);  /* write a 0 byte */
        fwrite("",1,1,file);  /* write a 0 byte */
    }
}


/*****************************************************************************/


static void WriteInline(FILE *file, void *data, int numWords)
{
    fwrite(data,numWords * 4,1,file);
}


/*****************************************************************************/


int main(int argc, char **argv)
{
FILE        *file;
FormHdr      form;
ChunkHdr     chunk;
int          i;
Locale      *ctry;
int          startEntries;
int          len;

    if (argc != 2)
    {
        printf("Usage: gencountrydb <filename>\n");
        return 0;
    }

    file = fopen(argv[1],"w");
    if (file)
    {
        form.ID       = ID_FORM;
        form.FormType = ID_INTL;
        chunk.ID      = ID_CTRY;

        for (i = 0; i < NUM_COUNTRIES; i++)
            ce[i].ce_Country = countryInfo[i]->loc_Country;

        fwrite(&form,sizeof(form),1,file);
        fwrite(&chunk,sizeof(chunk),1,file);

        startEntries = ftell(file);
        fwrite(ce,sizeof(ce),1,file);

        for (i = 0; i < NUM_COUNTRIES; i++)
        {
            ctry = countryInfo[i];
            currentCountry = ctry->loc_Country;

            ce[i].ce_SeekOffset = ftell(file) - startEntries - i * sizeof(CountryEntry) - sizeof(CountryEntry);

            /* loc_Dialects                                     */
            WriteString(file,NULL);

            /* loc_Country                                      */
            /* loc_GMTOffset                                    */
            /* loc_MeasuringSystem                              */
            /* loc_CalendarType                                 */
            /* loc_DrivingType                                  */
            WriteInline(file,&ctry->loc_Country,5);

        /* --- */

            /* loc_Numbers.ns_PosGroups                         */
            WriteInline(file,&ctry->loc_Numbers.ns_PosGroups,1);

            /* loc_Numbers.ns_PosGroupSep                       */
            WriteString(file,ctry->loc_Numbers.ns_PosGroupSep);

            /* loc_Numbers.ns_PosRadix                          */
            WriteString(file,ctry->loc_Numbers.ns_PosRadix);

            /* loc_Numbers.ns_PosFractionalGroups               */
            WriteInline(file,&ctry->loc_Numbers.ns_PosFractionalGroups,1);

            /* loc_Numbers.ns_PosFractionalGroupSep             */
            WriteString(file,ctry->loc_Numbers.ns_PosFractionalGroupSep);

            /* loc_Numbers.ns_PosFormat                         */
            WriteString(file,ctry->loc_Numbers.ns_PosFormat);

            /* loc_Numbers.ns_PosMinFractionalDigits            */
            /* loc_Numbers,ns_PosMaxFractionalDigits            */
            /* loc_Numbers.ns_NegGroups                         */
            WriteInline(file,&ctry->loc_Numbers.ns_PosMinFractionalDigits,3);

            /* loc_Numbers.ns_NegGroupSep                       */
            WriteString(file,ctry->loc_Numbers.ns_NegGroupSep);

            /* loc_Numbers.ns_NegRadix                          */
            WriteString(file,ctry->loc_Numbers.ns_NegRadix);

            /* loc_Numbers.ns_NegFractionalGroups               */
            WriteInline(file,&ctry->loc_Numbers.ns_NegFractionalGroups,1);

            /* loc_Numbers.ns_NegFractionalGroupSep             */
            WriteString(file,ctry->loc_Numbers.ns_NegFractionalGroupSep);

            /* loc_Numbers.ns_NegFormat                         */
            WriteString(file,ctry->loc_Numbers.ns_NegFormat);

            /* loc_Numbers.ns_NegMinFractionalDigits            */
            /* loc_Numbers.ns_NegMaxFractionalDigits            */
            WriteInline(file,&ctry->loc_Numbers.ns_NegMinFractionalDigits,2);

            /* loc_Numbers.ns_Zero                              */
            WriteString(file,ctry->loc_Numbers.ns_Zero);

            /* loc_Numbers.ns_Flags                             */
            WriteInline(file,&ctry->loc_Numbers.ns_Flags,1);

        /* --- */

            /* loc_Currency.ns_PosGroups                         */
            WriteInline(file,&ctry->loc_Currency.ns_PosGroups,1);

            /* loc_Currency.ns_PosGroupSep                       */
            WriteString(file,ctry->loc_Currency.ns_PosGroupSep);

            /* loc_Currency.ns_PosRadix                          */
            WriteString(file,ctry->loc_Currency.ns_PosRadix);

            /* loc_Currency.ns_PosFractionalGroups               */
            WriteInline(file,&ctry->loc_Currency.ns_PosFractionalGroups,1);

            /* loc_Currency.ns_PosFractionalGroupSep             */
            WriteString(file,ctry->loc_Currency.ns_PosFractionalGroupSep);

            /* loc_Currency.ns_PosFormat                         */
            WriteString(file,ctry->loc_Currency.ns_PosFormat);

            /* loc_Currency.ns_PosMinFractionalDigits            */
            /* loc_Currency,ns_PosMaxFractionalDigits            */
            /* loc_Currency.ns_NegGroups                         */
            WriteInline(file,&ctry->loc_Currency.ns_PosMinFractionalDigits,3);

            /* loc_Currency.ns_NegGroupSep                       */
            WriteString(file,ctry->loc_Currency.ns_NegGroupSep);

            /* loc_Currency.ns_NegRadix                          */
            WriteString(file,ctry->loc_Currency.ns_NegRadix);

            /* loc_Currency.ns_NegFractionalGroups               */
            WriteInline(file,&ctry->loc_Currency.ns_NegFractionalGroups,1);

            /* loc_Currency.ns_NegFractionalGroupSep             */
            WriteString(file,ctry->loc_Currency.ns_NegFractionalGroupSep);

            /* loc_Currency.ns_NegFormat                         */
            WriteString(file,ctry->loc_Currency.ns_NegFormat);

            /* loc_Currency.ns_NegMinFractionalDigits            */
            /* loc_Currency.ns_NegMaxFractionalDigits            */
            WriteInline(file,&ctry->loc_Currency.ns_NegMinFractionalDigits,2);

            /* loc_Currency.ns_Zero                              */
            WriteString(file,ctry->loc_Currency.ns_Zero);

            /* loc_Currency.ns_Flags                             */
            WriteInline(file,&ctry->loc_Currency.ns_Flags,1);

        /* --- */

            /* loc_SmallCurrency.ns_PosGroups                         */
            WriteInline(file,&ctry->loc_SmallCurrency.ns_PosGroups,1);

            /* loc_SmallCurrency.ns_PosGroupSep                       */
            WriteString(file,ctry->loc_SmallCurrency.ns_PosGroupSep);

            /* loc_SmallCurrency.ns_PosRadix                          */
            WriteString(file,ctry->loc_SmallCurrency.ns_PosRadix);

            /* loc_SmallCurrency.ns_PosFractionalGroups               */
            WriteInline(file,&ctry->loc_SmallCurrency.ns_PosFractionalGroups,1);

            /* loc_SmallCurrency.ns_PosFractionalGroupSep             */
            WriteString(file,ctry->loc_SmallCurrency.ns_PosFractionalGroupSep);

            /* loc_SmallCurrency.ns_PosFormat                         */
            WriteString(file,ctry->loc_SmallCurrency.ns_PosFormat);

            /* loc_SmallCurrency.ns_PosMinFractionalDigits            */
            /* loc_SmallCurrency,ns_PosMaxFractionalDigits            */
            /* loc_SmallCurrency.ns_NegGroups                         */
            WriteInline(file,&ctry->loc_SmallCurrency.ns_PosMinFractionalDigits,3);

            /* loc_SmallCurrency.ns_NegGroupSep                       */
            WriteString(file,ctry->loc_SmallCurrency.ns_NegGroupSep);

            /* loc_SmallCurrency.ns_NegRadix                          */
            WriteString(file,ctry->loc_SmallCurrency.ns_NegRadix);

            /* loc_SmallCurrency.ns_NegFractionalGroups               */
            WriteInline(file,&ctry->loc_SmallCurrency.ns_NegFractionalGroups,1);

            /* loc_SmallCurrency.ns_NegFractionalGroupSep             */
            WriteString(file,ctry->loc_SmallCurrency.ns_NegFractionalGroupSep);

            /* loc_SmallCurrency.ns_NegFormat                         */
            WriteString(file,ctry->loc_SmallCurrency.ns_NegFormat);

            /* loc_SmallCurrency.ns_NegMinFractionalDigits            */
            /* loc_SmallCurrency.ns_NegMaxFractionalDigits            */
            WriteInline(file,&ctry->loc_SmallCurrency.ns_NegMinFractionalDigits,2);

            /* loc_SmallCurrency.ns_Zero                              */
            WriteString(file,ctry->loc_SmallCurrency.ns_Zero);

            /* loc_SmallCurrency.ns_Flags                             */
            WriteInline(file,&ctry->loc_SmallCurrency.ns_Flags,1);

            /* loc_Date                                         */
            WriteString(file,ctry->loc_Date);

            /* loc_ShortDate                                    */
            WriteString(file,ctry->loc_ShortDate);

            /* loc_Time                                         */
            WriteString(file,ctry->loc_Time);

            /* loc_ShortTime                                    */
            WriteString(file,ctry->loc_ShortTime);
        }

        len = ftell(file);
        if (len & 1)  /* align file on 2 byte boundary (IFF spec) */
            fwrite("",1,1,file);

        form.Size  = len - 8;
        chunk.Size = len - 20;
        fseek(file,0,0);
        fwrite(&form,sizeof(form),1,file);
        fwrite(&chunk,sizeof(chunk),1,file);
        fwrite(ce,sizeof(ce),1,file);

        fclose(file);

        return 0;
    }
    else
    {
        printf("%s: Unable to open output file '%s'\n",argv[0],argv[1]);
        return 1;
    }
}
