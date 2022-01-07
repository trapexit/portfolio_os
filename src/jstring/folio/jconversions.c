/* $Id: jconversions.c,v 1.11 1994/12/19 17:36:10 vertex Exp $ */

#include "types.h"
#include "string.h"
#include "kernel.h"
#include "mem.h"
#include "jstring.h"
#include "intl.h"
#include "tables.h"


/****************************************************************************/


#define __IGNORE_TO_CONVERT__	1

#if __IGNORE_TO_CONVERT__
#define	SKIP_CHAR				((char)0xA5)
#endif


/****************************************************************************/


#define isKanji(c)      (0x81 <= (c) && (c) <= 0x9F || 0xE0 <= (c) && (c) <= 0xFC)
#define isKanji2(c)     (0x40 <= (c) && (c) <= 0xFC && (c) != 0x7F )
#define isAlpha(c)      ((0x41 <= (c) && (c) <= 0x5A) || (0x61 <= (c) && (c) <= 0x7A))
#define isUpper(c)      (0x41 <= (c) && (c) <= 0x5A)
#define isLower(c)      (0x61 <= (c) && (c) <= 0x7A)
#define isNumber(c)     ((0x30 <= (c)) && ((c) <= 0x39))
#define isAlnum(c)        (isAlpha(c) || isNumber(c))
#define isPunctuation(c)  (((0x20 <= (c)) && ((c) <= 0x40)) || \
						  ((0x5B <= (c)) && ((c) <= 0x60)) || \
						  ((0x7B <= (c)) && ((c) <= 0x7E)))
#define isPunctuation2(c) (((0x8140 <= (c)) && ((c) <= 0x817E)) || \
                          ((0x8180 <= (c)) && ((c) <= 0x81AC)) || \
                          ((0x81B8 <= (c)) && ((c) <= 0x81BF)) || \
                          ((0x81C8 <= (c)) && ((c) <= 0x81CE)) || \
                          ((0x81DA <= (c)) && ((c) <= 0x81E8)) || \
                          ((0x81F0 <= (c)) && ((c) <= 0x81F7)) || \
                          ((c) == 0x81FC))
#define isNumber2(c)    (0x824F <= (c) && (c) <= 0x8258)
#define isAlpha2(c)     ((0x8260 <= (c) && (c) <= 0x8279) || (0x8281 <= (c) && (c) <= 0x829A))
#define isUpper2(c)     (0x8260 <= (c) && (c) <= 0x8279)
#define isLower2(c)     (0x8281 <= (c) && (c) <= 0x829A)
#define isAlNum2(c)     (isNumber2(c) || isAlpha2(c) || (c) == 0x8140)
#define isKana(c)       (0xA0 <= (c) && (c) <= 0xDF)
#define isKana2(c)      ((0x8340 <= (c)) && ((c) <= 0x8396) && ((c) != 0x837F))
#define isHiragana(c)   ((0x829F <= (c)) && ((c) <= 0x82F1))
#define isLatin1(c)     ((0xa0 <= (c)) && ((c) <= 0xff))


/****************************************************************************/


/* substitute for ligatures */
static const char *gLigatures[] =
{
    "<<", ">>", "1/4", "1/2", "3/4", "AE", "ae"
};


/*****************************************************************************/


static SJIS_UniCodeTable *SearchUniCodeTable(SJIS_UniCodeTable *which)
{
uint32 i;

    i = 0;
    while (uniCodeTable[i].SJISCode)
    {
        if ((uniCodeTable[i].SJISCode == which->SJISCode) ||
           (uniCodeTable[i].UniCode == which->UniCode))
            return &uniCodeTable[i];

        i++;
    }

    return NULL;
}


/****************************************************************************/


static int32 ParseRomaji(char *srcStr, char *dstStr )
{
int				i;
char*	src;
char*	dst;
char	c;
char	numChar;

	i = 0;
	src = (char*)srcStr;

	numChar = dstStr[0];
	dst = &dstStr[1];

	c = *(src+numChar-1);
	/* to lower */
	if( isAlpha( c ) )
		c |= 0x20;
	else
		goto	NormalExitThisFunc;

	/*
	*	if last char is unmatched.( for speed )
	*/
	if( c != dst[numChar-1] )
		goto	NormalExitThisFunc;

	for( i=0; i<(numChar-1); i++ )
	{
		/* first char is already matched */
		src++;
		dst++;

		c = *src;
		/* to lower */
		if( isAlpha( c ) )
			c |= 0x20;
		else
			break;

		if( *dst == c )
			continue;
		else
			break;
	}

NormalExitThisFunc:

	return	i + 1;

}


/****************************************************************************/


static int32 Romaji2Japanese(CharacterSets srcType,
                             CharacterSets dstType,
                             const char* srcStr,
                             char *dstStr,
                             uint32 resultSize,
                             uint8 filler )
{
short			i, j;
short			idx;
char*	src;
char*	dst;
char	numChar;
char*	transStr1;
char*	transStr2;
int32			rc, nChar, nrc;
int32			sz;
Boolean			number, punctuation;
#if		__IGNORE_TO_CONVERT__
Boolean			shipFlag = FALSE;
#endif

	rc = nrc = 0;
	sz = resultSize - 1;
	src = (char*)srcStr;
	dst = (char*)dstStr;

	while( 1 )
	{
		/*
		*	punctuation and number is able to be get from conversion func,
		*	but I do not do it for speed.
		*/
		number = isNumber( *src ) ? TRUE : FALSE;
		if( !number )
			punctuation = isPunctuation( *src ) ? TRUE : FALSE;
		else
			punctuation = FALSE;

		if( *src == '\0' )
			break;

		if( sz <= rc )
			break;

#if		__IGNORE_TO_CONVERT__
		if( *src == SKIP_CHAR )				 /* 94/12/15 (2) */
		{
			shipFlag = shipFlag ? FALSE : TRUE;
			src++; nrc++;
			if( *src != shipFlag )
				continue;
		}

		if( shipFlag == TRUE || *src==shipFlag )
		{
			*dst++ = *src++;
			rc++; nrc++;
			continue;
		}
#endif

		if( isLatin1( *src ) )
		{
			*dst++ = filler;
			src++;
			rc++; nrc++;
			continue;
		}

		if( !punctuation && !number && !isLower( *src | 0x20 ) )
		{
			*dst = *src;
			rc++; nrc++;
			src++;
			dst++;
			continue;
		}
		else
		{
			if( punctuation )
				idx = gRomajiTableIndex[ 26 ];
			else	if( number )
				idx = gRomajiTableIndex[ 52 ];
			else
				idx = gRomajiTableIndex[ (*src | 0x20) - 0x61 ];
		}

		for( i=0; ; i++ )
		{
		char srcChr;		/* temp */
			transStr1 = (char*)gRomajiTable[idx+i].romaji;
			if( transStr1[0]==0 )	/* unkown panctuation *//* 94/12/15 (1) */
			{
				*dst++ = filler;
				src++;
				rc++; nrc++;
				continue;
			}
			if( !punctuation && !number && transStr1[1] != (*src | 0x20) )
			{
				*dst = *src; /* may be a alphabet */
				rc++; nrc++;
				src++;
				dst++;
				break;
			}
			srcChr = *src | 0x20;
			
			
			
			if( !punctuation &&
				!number &&
				srcChr != 'n' &&
				srcChr == (*(src+1) | 0x20) &&
				srcChr != 'a' && srcChr != 'o'&& srcChr != 'u' &&
				srcChr != 'i' && srcChr != 'e')			 /* 94/12/15 (3) */
			{
				src++;
				numChar = dstType == INTL_CS_HALF_KANA ? 1 : 2;
				if( (sz - numChar) < 0 )
				{
					*dst = '\0';
					goto	ExitThisFunc;
				}

				switch( dstType )
				{
				case	INTL_CS_HALF_KANA:
					transStr2 = (char*)gRomajiTable[199].halfKana;
					break;
				case	INTL_CS_FULL_KANA:
					transStr2 = (char*)gRomajiTable[199].fullKana;
					break;
				case	INTL_CS_HIRAGANA:
					transStr2 = (char*)gRomajiTable[199].hiragana;
					break;
				}

				for( j=0; j<numChar; j++ )				/* Such like "makku" (1st k is proccessed here) */
				{
					*dst++ = (char)transStr2[j+1];
				}
				rc += numChar;
				nrc++;	/* 1 character in japanese. So don't put this instruction into for loop. */
			}

			nChar = ParseRomaji( (char*)src, (char*)transStr1 );

			if(	(punctuation && *src == transStr1[1]) ||
				(number && *src == transStr1[1]) ||
				((!punctuation && !number) && nChar == transStr1[0]) )
			{
				switch( dstType )
				{
				case	INTL_CS_HALF_KANA:
					transStr2 = (char*)gRomajiTable[idx+i].halfKana;
					break;
				case	INTL_CS_FULL_KANA:
					transStr2 = (char*)gRomajiTable[idx+i].fullKana;
					break;
				case	INTL_CS_HIRAGANA:
					transStr2 = (char*)gRomajiTable[idx+i].hiragana;
					break;
				}

				numChar = (char)transStr2[0];
				if( (sz - numChar) < 0 )
				{
					*dst = '\0';
					goto	ExitThisFunc;
				}
				for( j=0; j<numChar; j++ )
					*dst++ = (char)transStr2[j+1];

				src += transStr1[0];
				rc += numChar;
				if( dstType != INTL_CS_HALF_KANA )	/* 1 character in japanese. */
					nrc += numChar/2;
				else
					nrc += numChar;
				break;
			}
		}
	}

	*dst = '\0';

ExitThisFunc:
	return	nrc;
}


/****************************************************************************/


static int32 ParseJapanese(char *srcStr, char *dstStr)
{
int				i;
char*	src;
char*	dst;
char	c;
char	numChar;

	i = 0;
	src = (char*)srcStr;

	numChar = dstStr[0];
	dst = &dstStr[1];

	c = *(src+numChar-1);

	/*
	*	if last char is unmatched.( for speed )
	*/
	if( c != dst[numChar-1] )
		goto	NormalExitThisFunc;

	for( i=0; i<(numChar-1); i++ )
	{
		/* first char is already matched */
		src++;
		dst++;

		c = *src;
		if( *dst == c )
			continue;
		else
			break;
	}

NormalExitThisFunc:

	return	i + 1;

}


/****************************************************************************/


static int32 Japanese2_X(CharacterSets srcType,
                         CharacterSets dstType,
                         const char* srcStr,
                         char *dstStr,
                         uint32 resultSize,
                         uint8 filler )
{
short			j;
short			idx;
char*	src;
char*	dst;
char	numChar;
char*	transStr1;
char*	transStr2;
int32			rc, nChar, nrc;
int32			sz;

	rc = nrc = 0;
	sz = resultSize - 1;
	src = (char*)srcStr;
	dst = (char*)dstStr;

	while( 1 )
	{
		if( *src == '\0' )
			break;

		if( sz <= rc )
			break;

		for( idx=0; ; idx++ )
		{
			switch( srcType )
			{
			case	INTL_CS_HALF_KANA:
				transStr1 = (char*)gRomajiTable[idx].halfKana;
				break;
			case	INTL_CS_FULL_KANA:
				transStr1 = (char*)gRomajiTable[idx].fullKana;
				break;
			case	INTL_CS_HIRAGANA:
				transStr1 = (char*)gRomajiTable[idx].hiragana;
				break;
			}

			if( transStr1[0] == '\0' )
			{
				*dst = *src;
				rc++; nrc++;
				src++;
				dst++;
				break;
			}

			nChar = ParseJapanese( (char*)src, (char*)transStr1 );

			if(	*src == transStr1[1] &&
				nChar == transStr1[0] )
			{
				switch( dstType )
				{
				case	INTL_CS_ROMAJI:
					transStr2 = (char*)gRomajiTable[idx].romaji;
					break;
				case	INTL_CS_HALF_KANA:
					transStr2 = (char*)gRomajiTable[idx].halfKana;
					break;
				case	INTL_CS_FULL_KANA:
					transStr2 = (char*)gRomajiTable[idx].fullKana;
					break;
				case	INTL_CS_HIRAGANA:
					transStr2 = (char*)gRomajiTable[idx].hiragana;
					break;
				}
				numChar = (char)transStr2[0];
				if( (sz - numChar) < 0 )
				{
					*dst = '\0';
					goto	ExitThisFunc;
				}
				for( j=0; j<numChar; j++ )
					*dst++ = (char)transStr2[j+1];

				src += transStr1[0];
				rc += numChar;
				if( dstType == INTL_CS_FULL_KANA || dstType == INTL_CS_HIRAGANA )	/* 1 character in japanese. */
					nrc += numChar/2;
				else
					nrc += numChar;
				break;
			}
		}
	}

	*dst = '\0';

ExitThisFunc:
	return	nrc;
}


/*****************************************************************************/


int32 ConvertShiftJIS2UniCode(const char *string, unichar *result,
                              uint32 resultSize, uint8 filler)
{
int32               i, j;
uint32              maxIndex;
SJIS_UniCodeTable   tbl;
SJIS_UniCodeTable  *tblFound;

    if (resultSize == 0)
        return JSTR_ERR_BUFFERTOOSMALL;

    maxIndex = (resultSize / sizeof(unichar)) - 1;

    tbl.UniCode = 0;
    for (i = 0, j = 0; i < maxIndex; i++, j++)
    {
        tbl.SJISCode = (string[j] & 0xFF);
        if (isKanji(string[j]))
        {
            tbl.SJISCode <<= 8;
            j++;
            if (isKanji2(string[j]))
                tbl.SJISCode |= (string[j] & 0xFF);
            else
                tbl.SJISCode = (unichar)0;
        }
        else
        {
            if (string[j] && !isAlnum(string[j]) && !isKana(string[j]) && !isPunctuation(string[j]))
                tbl.SJISCode = (unichar)filler;
        }

        tblFound = SearchUniCodeTable( &tbl );
		if (tblFound)
			result[i] = tblFound->UniCode;
		else
        	result[i] = (string[j] != 0) ? (unichar)filler : 0;

        if (!result[i])
            return i;

        if (i >= maxIndex)
            return JSTR_ERR_BUFFERTOOSMALL;
    }

    result[i] = 0;

    return i;
}


/*****************************************************************************/


int32 ConvertUniCode2ShiftJIS(const unichar *string, char *result,
                              uint32 resultSize, uint8 filler)
{
int32              i, j;
uint32             maxIndex;
SJIS_UniCodeTable  tbl;
SJIS_UniCodeTable *tblFound;
char               sjisChar1, sjisChar2;

    if (resultSize == 0)
        return JSTR_ERR_BUFFERTOOSMALL;

    maxIndex = resultSize - 1;

    tbl.SJISCode = 0;
    for (i = 0, j = 0; ; i++, j++)
    {
        tbl.UniCode = string[j];
        tblFound = SearchUniCodeTable(&tbl);
        if (tblFound)
        {
            sjisChar1 = (tblFound->SJISCode >> 8) & 0x00FF;
            sjisChar2 = tblFound->SJISCode & 0x00FF;
            if (sjisChar1 && isKanji( sjisChar1))
            {
                result[i++] = sjisChar1;
                /* If there is no space to store and the last character is
                 * the first byte of Kanji
                 */
                if (i > maxIndex)
                {
                    result[--i] = 0;
                    return JSTR_ERR_BUFFERTOOSMALL;
                }

                if (isKanji2( sjisChar2 ) )
                    result[i] = sjisChar2;
                else
                    result[--i] = filler;
            }
            else
            {
                result[i] = sjisChar2;
                if (sjisChar2 && !isAlnum(sjisChar2) && !isKana(sjisChar2) && !isPunctuation(sjisChar2))
                    result[i] = filler;
            }
        }
        else
        {
            result[i] = (string[j] != 0) ? filler : 0;
        }

        if (!result[i])
            return i;

        if (i >= maxIndex)
            return JSTR_ERR_BUFFERTOOSMALL;
    }

    result[i] = 0;

    return i;
}


/****************************************************************************/


int32 ConvertASCII2ShiftJIS(const char *string, char *result,
                            uint32 resultSize, uint8 filler)
{
int32	i, j, k;
uint32	maxIndex;
char	c;
int32	substituteIdx, subCnt;

    if (resultSize == 0)
        return JSTR_ERR_BUFFERTOOSMALL;

    maxIndex = resultSize - 1;
    substituteIdx = 0;

    i = 0;
    j = 0;
    k = 0;
    subCnt = 0;
    while (TRUE)
    {
        if (subCnt)
        {
            result[i] = gLigatures[ substituteIdx ][ j++ ];
            subCnt--;
        }
        else
        {
            j = 0;
            c = (char)string[k++];

            if (c == 0xAB || c == 0xBB)
            {
                substituteIdx = (c == 0xAB) ? 0 : 1;
                subCnt = 2;
                continue;
            }
            else if (c == 0xBC || c == 0xBD || c == 0xBE)
            {
                substituteIdx = (c - 0xBC + 2);
                subCnt = 3;
                continue;
            }
            else if (c == 0xC6 || c == 0xE6)
            {
                substituteIdx = (c == 0xC6) ? 5 : 6;
                subCnt = 2;
                continue;
            }

            result[i] = asciiTable[c];
            if (!result[i])
            {
                if (!c)
                    return i;

                result[i] = (char)filler;
            }
        }

        if (i >= maxIndex)
        {
            result[maxIndex] = 0;
            return (JSTR_ERR_BUFFERTOOSMALL);
        }

        i++;
    }
}


/****************************************************************************/


int32 ConvertShiftJIS2ASCII(const char *string, char *result,
                            uint32 resultSize, uint8 filler)
{
int32  i;
uint32 maxIndex;
char   c;

    if (resultSize == 0)
        return JSTR_ERR_BUFFERTOOSMALL;

    maxIndex = resultSize - 1;

    i = 0;
    while (TRUE)
    {
        c = string[i];
        if (!c)
        {
            result[i] = 0;
            return i;
        }

        if (isKanji(c))
        {
            if (string[i + 1] && isKanji2(string[i + 1]))
            {
                result[i++] = filler;
            }
            result[i] = filler;
        }
        else
        {
            if (c & 0x80)
                result[i] = filler;
            else
                result[i] = c;
        }

        if (i >= maxIndex)
        {
            result[maxIndex] = 0;
            return (JSTR_ERR_BUFFERTOOSMALL);
        }

        i++;
    }
}




/****************************************************************************/


int32 ConvertRomaji2Hiragana(const char *string, char *result,
                             uint32 resultSize, uint8 filler)
{
    return Romaji2Japanese( INTL_CS_ROMAJI, INTL_CS_HIRAGANA, string, result, resultSize, filler );
}




/****************************************************************************/


int32 ConvertRomaji2FullKana(const char *string, char *result,
                              uint32 resultSize, uint8 filler)
{
    return Romaji2Japanese( INTL_CS_ROMAJI, INTL_CS_FULL_KANA, string, result, resultSize, filler );
}




/****************************************************************************/


int32 ConvertRomaji2HalfKana(const char *string, char *result,
                              uint32 resultSize, uint8 filler)
{
    return Romaji2Japanese( INTL_CS_ROMAJI, INTL_CS_HALF_KANA, string, result, resultSize, filler );
}




/****************************************************************************/


int32 ConvertHiragana2Romaji(const char *string, char *result,
                              uint32 resultSize, uint8 filler)
{
    return Japanese2_X( INTL_CS_HIRAGANA, INTL_CS_ROMAJI, string, result, resultSize, filler );
}




/****************************************************************************/


int32 ConvertFullKana2Romaji(const char *string, char *result,
                              uint32 resultSize, uint8 filler)
{
    return Japanese2_X( INTL_CS_FULL_KANA, INTL_CS_ROMAJI, string, result, resultSize, filler );
}




/****************************************************************************/


int32 ConvertHalfKana2Romaji(const char *string, char *result,
                              uint32 resultSize, uint8 filler)
{
    return Japanese2_X( INTL_CS_HALF_KANA, INTL_CS_ROMAJI, string, result, resultSize, filler );
}



/****************************************************************************/


int32 ConvertHalfKana2Hiragana(const char *string, char *result,
                              uint32 resultSize, uint8 filler)
{
    return Japanese2_X( INTL_CS_HALF_KANA, INTL_CS_HIRAGANA, string, result, resultSize, filler );
}



/****************************************************************************/


int32 ConvertHalfKana2FullKana(const char *string, char *result,
                              uint32 resultSize, uint8 filler)
{
    return Japanese2_X( INTL_CS_HALF_KANA, INTL_CS_FULL_KANA, string, result, resultSize, filler );
}



/****************************************************************************/


int32 ConvertFullKana2HalfKana(const char *string, char *result,
                              uint32 resultSize, uint8 filler)
{
    return Japanese2_X( INTL_CS_FULL_KANA, INTL_CS_HALF_KANA, string, result, resultSize, filler );
}


/****************************************************************************/


int32 ConvertFullKana2Hiragana(const char *string, char *result,
                              uint32 resultSize, uint8 filler)
{
    return Japanese2_X( INTL_CS_FULL_KANA, INTL_CS_HIRAGANA, string, result, resultSize, filler );
}


/****************************************************************************/


int32 ConvertHiragana2HalfKana(const char *string, char *result,
                              uint32 resultSize, uint8 filler)
{
    return Japanese2_X( INTL_CS_HIRAGANA, INTL_CS_HALF_KANA, string, result, resultSize, filler );
}


/****************************************************************************/


int32 ConvertHiragana2FullKana(const char *string, char *result,
                              uint32 resultSize, uint8 filler)
{
    return Japanese2_X( INTL_CS_HIRAGANA, INTL_CS_FULL_KANA, string, result, resultSize, filler );
}
