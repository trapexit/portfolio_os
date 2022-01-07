/* $Id: transliterate.c,v 1.6 1994/11/04 18:58:55 vertex Exp $ */

/* #define TRACING */

#include "types.h"
#include "string.h"
#include "international_folio.h"
#include "kernel.h"
#include "mem.h"
#include "intl.h"
#include "international_folio.h"
#include "usermodeserver.h"
#include "jstring.h"


/****************************************************************************/


static Err OpenSubFolioFunc(void *dummy)
{
    return OpenJStringFolio();
}

static Err OpenSubFolio(void)
{
extern void *JStringBase;

    if (JStringBase)
        return 0;

    /* if the JString folio hasn't been loaded yet, we ask the user
     * mode server to do it for us. This will bring in the folio under the
     * thread of the folio, keeping it around until the folio itself is
     * expunged.
     */
    return CallUserModeFunc((USERMODEFUNC)OpenSubFolioFunc,NULL);
}


/****************************************************************************/


/* max character set currently defined and supported in intl.h */
#define MAX_CHAR_SET INTL_CS_HIRAGANA


/*****************************************************************************/


int32 ConvertASCII2UniCode(const char *string, unichar *result,
                           uint32 resultSize, uint8 filler)
{
int32  i;
uint32 maxIndex;

    if (resultSize < sizeof(unichar))
        return (INTL_ERR_BUFFERTOOSMALL);

    maxIndex = (resultSize / sizeof(unichar)) - 1;

    i = 0;
    while (TRUE)
    {
        result[i] = string[i];

        if (!string[i])
            return (i);

        if (i == maxIndex)
        {
            result[i] = 0;
            return (INTL_ERR_BUFFERTOOSMALL);
        }

        i++;
    }
}


/****************************************************************************/


int32 ConvertUniCode2ASCII(const unichar *string, char *result,
                           uint32 resultSize, uint8 filler)
{
int32  i;
uint32 maxIndex;

    if (!resultSize)
        return (INTL_ERR_BUFFERTOOSMALL);

    maxIndex = resultSize - 1;

    i = 0;
    while (TRUE)
    {
        if (string[i] > 0x00ff)
        {
            /* We should be more discriminate here. There are characters,
             * many in fact, in UniCode characters above 255 which can
             * be succesfully mapped to ASCII. However, I expect that
             * we will never encounter such characters in our environment.
             *
             * In any case, this is easy to fix if a bug report ever comes
             * in complaining about incorrect UniCode to ASCII conversions
             * of > 255 UniCode characters. It just requires a LOT of reading
             * of the gigantic UniCode tables....
             */

            result[i] = filler;
        }
        else
        {
            result[i] = string[i];
        }

        if (!string[i])
            return (i);

        if (i == maxIndex)
        {
            result[i] = 0;
            return (INTL_ERR_BUFFERTOOSMALL);
        }

        i++;
    }
}


/****************************************************************************/


typedef int32 (* CONVERSIONFUNC)(const void *, void *, uint32, uint8);

typedef struct ConversionInfo
{
    bool           ci_SubFolio;
    CONVERSIONFUNC ci_Func;
} ConversionInfo;

static ConversionInfo converters[]=
{
    /* convert from ASCII to x */
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)ConvertASCII2UniCode},
    {TRUE,  (CONVERSIONFUNC)ConvertASCII2ShiftJIS},
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},

    /* convert from UniCode to x */
    {FALSE, (CONVERSIONFUNC)ConvertUniCode2ASCII},
    {FALSE, (CONVERSIONFUNC)NULL},
    {TRUE,  (CONVERSIONFUNC)ConvertUniCode2ShiftJIS},
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},

    /* convert from SJIS to x */
    {TRUE,  (CONVERSIONFUNC)ConvertShiftJIS2ASCII},
    {TRUE,  (CONVERSIONFUNC)ConvertShiftJIS2UniCode},
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},

    /* convert from Romaji to x */
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {TRUE,  (CONVERSIONFUNC)ConvertRomaji2HalfKana},
    {TRUE,  (CONVERSIONFUNC)ConvertRomaji2FullKana},
    {TRUE,  (CONVERSIONFUNC)ConvertRomaji2Hiragana},

    /* convert from half-kana to x */
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {TRUE,  (CONVERSIONFUNC)ConvertHalfKana2Romaji},
    {FALSE, (CONVERSIONFUNC)NULL},
    {TRUE,  (CONVERSIONFUNC)ConvertHalfKana2FullKana},
    {TRUE,  (CONVERSIONFUNC)ConvertHalfKana2Hiragana},

    /* convert from full-kana to x */
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {TRUE,  (CONVERSIONFUNC)ConvertFullKana2Romaji},
    {TRUE,  (CONVERSIONFUNC)ConvertFullKana2HalfKana},
    {FALSE, (CONVERSIONFUNC)NULL},
    {TRUE,  (CONVERSIONFUNC)ConvertFullKana2Hiragana},

    /* convert from hiragana to x */
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {FALSE, (CONVERSIONFUNC)NULL},
    {TRUE,  (CONVERSIONFUNC)ConvertHiragana2Romaji},
    {TRUE,  (CONVERSIONFUNC)ConvertHiragana2HalfKana},
    {TRUE,  (CONVERSIONFUNC)ConvertHiragana2FullKana},
    {FALSE, (CONVERSIONFUNC)NULL}
};


/****************************************************************************/


int32 intlTransliterateString(const void *string, CharacterSets stringSet,
                              void *result, CharacterSets resultSet,
                              uint32 resultSize,
                              uint8 unknownFiller)
{
ConversionInfo *conv;
CONVERSIONFUNC  cf;
Err             err;

    TRACE(("INTLTRANSLITERATESTRING: entering\n"));

#ifdef DEVELOPMENT
    if (((uint32)stringSet > MAX_CHAR_SET) || ((uint32)resultSet > MAX_CHAR_SET))
        return (INTL_ERR_BADCHARACTERSET);

    if (!IsRamAddr(string,2))
        return (INTL_ERR_BADSOURCEBUFFER);

    if (ValidateMem(KernelBase->kb_CurrentTask,result,resultSize) < 0)
        return (INTL_ERR_BADRESULTBUFFER);
#endif

    conv = &converters[stringSet * (MAX_CHAR_SET+1) + resultSet];
    cf   = conv->ci_Func;

    if (conv->ci_SubFolio)
    {
        err = OpenSubFolio();
        if (err < 0)
            return err;
    }

#ifdef DEVELOPMENT
    if (!cf)
        return (INTL_ERR_NOTSUPPORTED);
#endif

    TRACE(("INTLTRANSLITERATESTRING: calling conversion routine\n"));

    return (*cf)(string,result,resultSize,unknownFiller);
}
