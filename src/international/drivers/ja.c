/* $Id: ja.c,v 1.11 1994/12/21 18:02:11 vertex Exp $ */

/* Japanese language driver for the International folio.
 * Written by Junji Kanemaru of 3DO Japan
 */

#include "types.h"
#include "langdrivers.h"
#include "intl.h"
#include "debug.h"
#include "ja.h"


/*****************************************************************************/


#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif


/*****************************************************************************/


/* so we can call intlTransliterateString() */
void *InternationalBase;


/*****************************************************************************/


/* The following table is to convert FULL-WIDTH symbol-chars to HALF-WIDTH.
 * Some characters are exchanged most closest HALF-SIZE character.
 */
static ExchangeTable punctuationTable[] =
{
    { 0x3000, 0x0020 },             /* IDEOGRAPHIC SPACE */
    { 0x3001, 0x002C },             /* IDEOGRAPHIC COMMA */
    { 0x3002, 0xFF61 },             /* IDEOGRAPHIC FULL STOP */
    { 0xFF0C, 0x002C },             /* COMMA */
    { 0xFF0E, 0x002E },             /* FULL STOP */
    { 0xFF1A, 0x003A },             /* COLON */
    { 0xFF1B, 0x003B },             /* SEMICOLON */
    { 0xFF1F, 0x003F },             /* QUESTION MARK */
    { 0xFF01, 0x0021 },             /* EXCLAMATION MARK */
    { 0xFF40, 0x0027 },             /* GRAVE ACCENT */
    { 0xFF3E, 0x005E },             /* CIRCUMFLEX ACCENT */
    { 0xFFE3, 0x007E },             /* MACRON */
    { 0xFF3F, 0x005F },             /* LOW LINE */
    { 0x2015, 0xFF70 },             /* HORIZONTAL BAR */
    { 0x2010, 0xFF70 },             /* HYPHEN */
    { 0xFF0F, 0x002F },             /* SOLIDUS */
    { 0xFF5C, 0x007C },             /* VERTICAL LINE */
    { 0x2018, 0x0027 },             /* LEFT SINGLE QUOTATION MARK */
    { 0x2019, 0x0027 },             /* RIGHT SINGLE QUOTATION MARK */
    { 0x201C, 0x0022 },             /* LEFT DOUBLE QUOTATION MARK */
    { 0x201D, 0x0022 },             /* RIGHT DOUBLE QUOTATION MARK */
    { 0xFF08, 0x0028 },             /* LEFT PARENTHESIS */
    { 0xFF09, 0x0029 },             /* RIGHT PARENTHESIS */
    { 0x3014, 0x0028 },             /* LEFT TORTOISE SHELL BRACKET */
    { 0x3015, 0x0029 },             /* RIGHT TORTOISE SHELL BRACKET */
    { 0xFF3B, 0x005B },             /* LEFT SQUARE BRACKET */
    { 0xFF3D, 0x005D },             /* RIGHT SQUARE BRACKET */
    { 0xFF5B, 0x007B },             /* LEFT CURLY BRACKET */
    { 0xFF5D, 0x007D },             /* RIGHT CURLY BRACKET */
    { 0x3008, 0x003C },             /* LEFT ANGLE BRACKET */
    { 0x3009, 0x003E },             /* RIGHT ANGLE BRACKET */
    { 0x300C, 0xFF62 },             /* LEFT CORNER BRACKET */
    { 0x300D, 0xFF63 },             /* RIGHT CORNER BRACKET */
    { 0x300E, 0xFF62 },             /* LEFT WHITE CORNER BRACKET */
    { 0x300F, 0xFF63 },             /* RIGHT WHITE CORNER BRACKET */
    { 0xFF0B, 0x002B },             /* PLUS SIGN */
    { 0x2212, 0x002D },             /* MINUS SIGN */
    { 0xFF1D, 0x003D },             /* EQUALS SIGN */
    { 0xFF1C, 0x003C },             /* LESS-THAN SIGN */
    { 0xFF1E, 0x003E },             /* GREATER-THAN SIGN */
    { 0xFF04, 0x0024 },             /* DOLLAR SIGN */
    { 0xFF05, 0x0025 },             /* PERCENT SIGN */
    { 0xFF03, 0x0023 },             /* NUMBER SIGN */
    { 0xFF06, 0x0026 },             /* AMPERSAND */
    { 0xFF0A, 0x002A },             /* ASTERISK */
};

/* Full-Kana -> half-Kana conversion table */
static uint16 kanaTable[] =
{
    0x30FB, /* KATAKANA MIDDLE DOT */
    0x30F2, /* LETTER WO */
    0x30A1, /* LETTER SMALL A */
    0x30A3, /* LETTER SMALL I */
    0x30A5, /* LETTER SMALL U */
    0x30A7, /* LETTER SMALL E */
    0x30A9, /* LETTER SMALL O */
    0x30E3, /* LETTER SMALL YA */
    0x30E5, /* LETTER SMALL YU */
    0x30E7, /* LETTER SMALL YO */
    0x30C3, /* LETTER SMALL TU */
    0x30FC, /* KATAKANA-HIRAGANA PROLONGED SOUND MARK */
    0x30A2, /* LETTER A */
    0x30A4, /* LETTER I */
    0x30A6, /* LETTER U */
    0x30A8, /* LETTER E */
    0x30AA, /* LETTER O */
    0x30AB, /* LETTER KA */
    0x30AD, /* LETTER KI */
    0x30AF, /* LETTER KU */
    0x30B1, /* LETTER KE */
    0x30B3, /* LETTER KO */
    0x30B5, /* LETTER SA */
    0x30B7, /* LETTER SI */
    0x30B9, /* LETTER SU */
    0x30BB, /* LETTER SE */
    0x30BD, /* LETTER SO */
    0x30BF, /* LETTER TA */
    0x30C1, /* LETTER TI */
    0x30C4, /* LETTER TU */
    0x30C6, /* LETTER TE */
    0x30C8, /* LETTER TO */
    0x30CA, /* LETTER NA */
    0x30CB, /* LETTER NI */
    0x30CC, /* LETTER NU */
    0x30CD, /* LETTER NE */
    0x30CE, /* LETTER NO */
    0x30CF, /* LETTER HA */
    0x30D2, /* LETTER HI */
    0x30D5, /* LETTER HU */
    0x30D8, /* LETTER HE */
    0x30DB, /* LETTER HO */
    0x30DE, /* LETTER MA */
    0x30DF, /* LETTER MI */
    0x30E0, /* LETTER MU */
    0x30E1, /* LETTER ME */
    0x30E2, /* LETTER MO */
    0x30E4, /* LETTER YA */
    0x30E6, /* LETTER YU */
    0x30E8, /* LETTER YO */
    0x30E9, /* LETTER RA */
    0x30EA, /* LETTER RI */
    0x30EB, /* LETTER RU */
    0x30EC, /* LETTER RE */
    0x30ED, /* LETTER RO */
    0x30EF, /* LETTER WA */
    0x30F0, /* LETTER WI */
    0x30F1, /* LETTER WE */
    0x30F3, /* LETTER N */

    /* containing sound-mark(#59) */
    /* equivalece KA(#17)… & Sound Mark */
    0x30AC, /* KATAKANA LETTER GA */
    0x30AE, /* KATAKANA LETTER GI */
    0x30B0, /* KATAKANA LETTER GU */
    0x30B2, /* KATAKANA LETTER GE */
    0x30B4, /* KATAKANA LETTER GO */

    /* equivalece SA(#24)… & Sound Mark */
    0x30B6, /* KATAKANA LETTER ZA */
    0x30B8, /* KATAKANA LETTER ZI */
    0x30BA, /* KATAKANA LETTER ZU */
    0x30BC, /* KATAKANA LETTER ZE */
    0x30BE, /* KATAKANA LETTER ZO */

    /* equivalece TA(#29)… & Sound Mark */
    0x30C0, /* KATAKANA LETTER DA */
    0x30C2, /* KATAKANA LETTER DI */
    0x30C5, /* KATAKANA LETTER DU */
    0x30C7, /* KATAKANA LETTER DE */
    0x30C9, /* KATAKANA LETTER DO */

    /* equivalece HA(#40)… & Sound Mark */
    0x30D0, /* KATAKANA LETTER BA */
    0x30D3, /* KATAKANA LETTER BI */
    0x30D6, /* KATAKANA LETTER BU */
    0x30D9, /* KATAKANA LETTER BE */
    0x30DC, /* KATAKANA LETTER BO */

    /* containing semi-sound-mark(#79) */
    /* equivalece HA(#40)… & Semi-Sound Mark */
    0x30D1, /* KATAKANA LETTER PA */
    0x30D4, /* KATAKANA LETTER PI */
    0x30D7, /* KATAKANA LETTER PU */
    0x30DA, /* KATAKANA LETTER PE */
    0x30DD, /* KATAKANA LETTER PO */

    /* equivalece FULL VU(#4) */
    0x30F4, /* KATAKANA LETTER VU */

    /* equivalece FULL KA(#17) */
    0x30F5, /* LETTER SMALL KA */

    /* equivalece FULL KE(#20) */
    0x30F6, /* LETTER SMALL KE */

    /* equivalece FULL WA(#58) */
    0x30EE, /* LETTER SMALL WA */
};

/* SJIS date string table */
static const char *dateStrings[]=
{
    "ì˙ójì˙",
    "åéójì˙",
    "âŒójì˙",
    "êÖójì˙",
    "ñÿójì˙",
    "ã‡ójì˙",
    "ìyójì˙",

    "ì˙",
    "åé",
    "âŒ",
    "êÖ",
    "ñÿ",
    "ã‡",
    "ìy",

    "ÇPåé",
    "ÇQåé",
    "ÇRåé",
    "ÇSåé",
    "ÇTåé",
    "ÇUåé",
    "ÇVåé",
    "ÇWåé",
    "ÇXåé",
    "ÇPÇOåé",
    "ÇPÇPåé",
    "ÇPÇQåé",
    "åé",

    "ÇP",
    "ÇQ",
    "ÇR",
    "ÇS",
    "ÇT",
    "ÇU",
    "ÇV",
    "ÇW",
    "ÇX",
    "ÇPÇO",
    "ÇPÇP",
    "ÇPÇQ",
    "åé",

    "åﬂëO",
    "åﬂå„"
};


/*****************************************************************************/


STATIC uint32 _isUniKana2(unichar ch)
{
uint32 i;

    if (!isUniKana2(LOWORD(ch)))
        return 0;

    for (i = 0; i < (sizeof(kanaTable)/sizeof(kanaTable[0])); i++)
    {
        if (kanaTable[i] == ch)
        {
            if (i <= 58)
                return (i + (uint32)0xFF65);

            if (i >= 59 && i <= 73)
                return ((((i - 59) + (uint32)0xFF76)) | ((uint32)0xFF9E << 16));

            if (i >= 74 && i <= 78)
                return ((((i - 74) + (uint32)0xFF8A)) | ((uint32)0xFF9E << 16));

            if (i >= 79 && i <= 83)
                return ((((i - 79) + (uint32)0xFF8A)) | ((uint32)0xFF9F << 16));

            if (i == 84)
                return (((uint32)0xFF69) | ((uint32)0xFF9E << 16));

            if (i == 85)
                return (uint32)0xFF76;

            if (i == 86)
                return (uint32)0xFF79;

            /* !!! JUNJI: This is wrong, these two "if" statements are checking the same value! */

            if (i == 86)
                return (uint32)0xFF9C;
        }
    }

    return 0;
}


/*****************************************************************************/


STATIC uint32 _isUniHiragana(unichar ch)
{
    if (!isUniHiragana(LOWORD(ch)))
        return 0;

    return _isUniKana2(ch + 0x0060);
}


/*****************************************************************************/


STATIC uint32 _isUniKana(uint32 ch)
{
unichar	kana, sndMark;

    if (!isUniKana(LOWORD(ch)))
        return 0;

    /* with sound mark ? */
    sndMark = HIWORD(ch);
    if (sndMark)
    {
        kana = (unichar)LOWORD(ch);
        if (sndMark == UNI_HALF_KANA_VOICEDSOUND_MARK)
        {
            if ((kana >= (unichar)0xFF76) && (kana <= (unichar)0xFF84))
                return kanaTable[(kana - (uint32)0xFF76 + 59)];        /* KATAKANA LETTER GA - DO */

            if ((kana >= (unichar)0xFF8A) && (kana <= (unichar)0xFF8E))
                return kanaTable[(kana - (uint32)0xFF8A + 74)];        /* KATAKANA LETTER BA - BO */

            if (kana == (unichar)0xFF73 )
                return (uint32)0x30F4;         /* KATAKANA LETTER VU */
        }
        else
        {
            if (sndMark == UNI_HALF_KANA_SEMI_VOICEDSOUND_MARK)
            {
                if ((kana >= (unichar)0xFF8A) && (kana <= (unichar)0xFF8E) )
                    return kanaTable[(kana - (uint32)0xFF8A + 79)];        /* KATAKANA LETTER PA - PO */
            }
        }
    }
    else
    {
        return (uint32)kanaTable[(ch-(uint32)0xFF65)];
    }

    return 0;
}


/*****************************************************************************/


STATIC uint32 _isPunctuation(unichar ch)
{
uint32 i;

    for (i = 0; i < (sizeof(punctuationTable)/sizeof(punctuationTable[0])); i++)
    {
        if ((punctuationTable[i].ch1) == ch || (punctuationTable[i].ch2 == ch))
            return  (((uint32)punctuationTable[i].ch1 << 16) | ((uint32)punctuationTable[i].ch2 & 0x0000FFFF));
    }

    return 0;
}



/*****************************************************************************/


STATIC bool GetDateStr(DateComponents dc, unichar *result, uint32 resultSize)
{
int32 nRet;

    if (dc > PM)
        return FALSE;

    nRet = intlTransliterateString(dateStrings[dc], INTL_CS_SHIFT_JIS,
                                   result, INTL_CS_UNICODE,
                                   resultSize,
                                   ' ');

    if (nRet >= 0)
        return TRUE;

    return FALSE;
}


/*****************************************************************************/


STATIC int32 CompareStrings(const unichar *str1, const unichar *str2)
{
uint32 i, j;

    /* I don't assume ligatures in Japanese */

    i = 0;
    j = 0;
    while (str1[i] && str2[j])
    {
        if (str1[i] != str2[j])
        {
            /* comma */
            if ((str1[i] == 0x3001 && str2[j] == 0xFF0C) ||
                (str1[i] == 0xFF0C && str2[j] == 0x3001))
                goto CONTINUE;

            /* middle dot */
            if ((str1[i] == 0x30FB && str2[j] == 0xFF65) ||
                (str1[i] == 0xFF65 && str2[j] == 0x30FB))
                goto CONTINUE;

            /* prolonged and horiz-bar */
            if ((str1[i] == 0x30FC && str2[j] == 0x2015) ||
                (str1[i] == 0x2015 && str2[j] == 0x30FC))
                goto CONTINUE;

            return (str1[i] < str2[j] ? -1 : 1);
        }

CONTINUE:
        i++;
        j++;
    }
	if( str1[i] || str2[j] )
    	return (str1[i] < str2[j] ? -1 : 1);
	else
		return 0;
}


/*****************************************************************************/


STATIC int32 ConvertString(const unichar *string,
                           unichar *result,
                           uint32 resultSize,
                           uint32 flags)
{
unichar src;
int32   i, j;
uint32  rc, maxIndex;

    if (!resultSize)
        return (INTL_ERR_BUFFERTOOSMALL);

    maxIndex = (resultSize / sizeof(unichar)) - 1;

    i = 0;
    j = 0;
    if (flags & INTL_CONVF_HALF_WIDTH)
    {
        while (TRUE)
        {
            src = string[i];
            if (!src)
            {
                result[j] = 0;
                break;
            }

            if (isUniAlnum2(src))        /* if full-width AlNum char */
            {
                result[j] = src - 0xFEE0;       /* convert to half-width char */
            }
            else
            {
                rc = _isPunctuation(src);
                if (rc)
                {
                    result[j] = (unichar)LOWORD( rc );
                }
                else
                {
                    rc = _isUniKana2(src);
                    if (!rc)
                        rc = _isUniHiragana(src);

                    if (rc)
                    {
                        result[j] = (unichar)LOWORD(rc);      /* convert to half-width char */
                        if (j >= maxIndex)
                        {
                            result[j] = 0;
                            return INTL_ERR_BUFFERTOOSMALL;
                        }

                        if (HIWORD(rc))
                            result[++j] = (unichar)HIWORD(rc);
                    }
                    else
                    {
                        result[j] = src;
                    }
                }
            }

            if (j >= maxIndex)
            {
                result[j] = 0;
                return INTL_ERR_BUFFERTOOSMALL;
            }
            i++;
            j++;
        }
    }
    else
    {
        if (flags & INTL_CONVF_FULL_WIDTH)
        {
            while (TRUE)
            {
                src = string[i];
                if (!src)
                {
                    result[j] = 0;
                    break;
                }

                if (isAlnum(src))    /* if half-width AlNum char */
                {
                    result[j] = src + 0xFEE0;       /* convert to full-width char */
                }
                else
                {
                    if (isUniKana(src))
                    {
                        rc = (uint32)src;
                        if (string[i+1] &&
                          ((string[i+1] == UNI_HALF_KANA_VOICEDSOUND_MARK) ||
                           (string[i+1] == UNI_HALF_KANA_SEMI_VOICEDSOUND_MARK)) )
                        {
                            /* put the sound-mark to HiWord */
                            rc |= (((uint32)string[++i]) << 16);
                        }

                        rc = _isUniKana(rc);
                        if (rc)
                            result[j] = (unichar)rc;
                        else
                            result[j] = src;
                    }
                    else
                    {
                        rc = _isPunctuation(src);
                        if (rc)
                            result[j] = HIWORD( src );
                        else
                            result[j] = src;
                    }
                }

                if (j >= maxIndex)
                {
                    result[j] = 0;
                    return INTL_ERR_BUFFERTOOSMALL;
                }
                i++;
                j++;
            }
        }
        else
        {
            while (TRUE)
            {
                result[i] = string[i];
                if (!result[i])
                    break;

                if (i >= maxIndex)
                {
                    result[i] = 0;
                    return INTL_ERR_BUFFERTOOSMALL;
                }
                i++;
            }
        }
    }

    if (flags & (INTL_CONVF_UPPERCASE | INTL_CONVF_LOWERCASE))
    {
       i = 0;
       while (TRUE)
        {
            src = result[i];
            if (!src)
                break;

            if (flags & INTL_CONVF_UPPERCASE)
            {
                if ((isAlpha(src) && ((src >= 0x0061) && (src <= 0x007A))) ||
                    (isUniAlpha2(src) && ((src >= 0xFF41) && (src <= 0xFF5A))))
                    src -= 0x0020;
            }
            else
            {
                if (flags & INTL_CONVF_LOWERCASE)
                {
                    if ((isAlpha(src) && ((src >= 0x0041) && (src <= 0x005A))) ||
                        (isUniAlpha2(src) && ((src >= 0xFF21) && (src <= 0xFF3A))))
                        src += 0x0020;
                }
            }

            result[i++] = src;
        }
    }

    return i;
}


/*****************************************************************************/


STATIC uint32 GetCharAttrs(unichar ch)
{
uint32 rc;

    if (ch == 0x0020)
        return INTL_ATTRF_SPACE;

    if (ch == 0x3000)
        return (INTL_ATTRF_SPACE | INTL_ATTRF_FULL_WIDTH);

    if (isNumber(ch))
        return INTL_ATTRF_DECIMAL_DIGIT;

    if (isUniNumber2(ch))
        return (INTL_ATTRF_DECIMAL_DIGIT | INTL_ATTRF_FULL_WIDTH);

    if (isUpper(ch))
        return INTL_ATTRF_UPPERCASE;

    if (isUniUpper2(ch))
        return (INTL_ATTRF_UPPERCASE | INTL_ATTRF_FULL_WIDTH);

    if (isLower(ch))
        return INTL_ATTRF_LOWERCASE;

    if (isUniLower2(ch))
        return (INTL_ATTRF_LOWERCASE | INTL_ATTRF_FULL_WIDTH);

    rc = _isPunctuation(ch);
    if (rc && (LOWORD(rc) != (uint32)'#') &&
              (LOWORD(rc) != (uint32)'$') &&
              (LOWORD(rc) != (uint32)'@'))
    {
        if (LOWORD(rc) == (uint32)ch)
            return INTL_ATTRF_PUNCTUATION;

        return (INTL_ATTRF_PUNCTUATION | INTL_ATTRF_FULL_WIDTH);
    }

    if (isUniKana(ch))
        return (INTL_ATTRF_KANA | INTL_ATTRF_HALF_WIDTH);

    if (isUniKana2(ch))
        return (INTL_ATTRF_KANA | INTL_ATTRF_FULL_WIDTH);

    if (isUniHiragana(ch))
        return (INTL_ATTRF_HIRAGANA | INTL_ATTRF_FULL_WIDTH);

    if (isUniKanji(ch))
        return (INTL_ATTRF_KANJI | INTL_ATTRF_FULL_WIDTH);

    return 0;
}


/*****************************************************************************/


/* Driver Information Structure */
STATIC LanguageDriverInfo driverInfo =
{
    sizeof(LanguageDriverInfo),

    CompareStrings,
    ConvertString,
    GetCharAttrs,
    GetDateStr
};


/*****************************************************************************/


LanguageDriverInfo *main(void)
{
#ifdef DEVELOPMENT
    print_vinfo();
#endif

    /* get the folio so we can call on it... */
    InternationalBase = LookupItem(FindNamedItem(MKNODEID(KERNELNODE,FOLIONODE),"international"));
    if (InternationalBase)
        return &driverInfo;

    return NULL;
}
