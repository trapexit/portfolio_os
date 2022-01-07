/* $Id: autodocs.c,v 1.4 1994/11/04 19:14:49 vertex Exp $ */

/**
|||	AUTODOC PUBLIC spg/jstring/convertshiftjis2ascii
|||	ConvertShiftJIS2ASCII - Convert a Shift-JIS string to ASCII.
|||
|||	  Synopsis
|||
|||	    int32 ConvertShiftJIS2ASCII(const char *string,
|||	                                char *result, uint32 resultSize,
|||	                                uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from Shift-JIS to ASCII. The conversion
|||	    is done as well as possible. If certain characters from the source
|||	    string cannot be represented in ASCII, the unknownFiller byte will
|||	    be inserted in the result buffer in their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/**
|||	AUTODOC PUBLIC spg/jstring/convertascii2shiftjis
|||	ConvertASCII2ShiftJIS - Convert an ASCII string to Shift-JIS.
|||
|||	  Synopsis
|||
|||	    int32 ConvertASCII2ShiftJIS(const char *string,
|||	                                char *result, uint32 resultSize,
|||	                                uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from ASCII to Shift-JIS. The conversion
|||	    is done as well as possible. If certain characters from the source
|||	    string cannot be represented in ASCII, the unknownFiller byte will
|||	    be inserted in the result buffer in their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/**
|||	AUTODOC PUBLIC spg/jstring/convertshiftjis2unicode
|||	ConvertShiftJIS2UniCode - Convert a Shift-JIS string to UniCode.
|||
|||	  Synopsis
|||
|||	    int32 ConvertShiftJIS2UniCode(const char *string,
|||	                                  unichar *result, uint32 resultSize,
|||	                                  uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from Shift-JIS to UniCode. The conversion
|||	    is done as well as possible. If certain characters from the source
|||	    string cannot be represented in ASCII, the unknownFiller byte will
|||	    be inserted in the result buffer in their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/**
|||	AUTODOC PUBLIC spg/jstring/convertunicode2shiftjis
|||	ConvertUniCode2ShiftJIS - Convert a UniCode string to Shift-JIS.
|||
|||	  Synopsis
|||
|||	    int32 ConvertUniCode2ShiftJIS(const unichar *string,
|||	                                  char *result, uint32 resultSize,
|||	                                  uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from UniCode to Shift-JIS. The conversion
|||	    is done as well as possible. If certain characters from the source
|||	    string cannot be represented in ASCII, the unknownFiller byte will
|||	    be inserted in the result buffer in their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/**
|||	AUTODOC PUBLIC spg/jstring/convertfullkana2halfkana
|||	ConvertFullKana2HalfKana - Convert a full-width kana string to a
|||	                           half-width kana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertFullKana2HalfKana(const char *string,
|||	                                   char *result, uint32 resultSize,
|||	                                   uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from full-width kana to half-width kana. The
|||	    conversion is done as well as possible. If certain characters from
|||	    the source string cannot be represented in the target character set,
|||	    the unknownFiller byte will be inserted in the result buffer in
|||	    their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/**
|||	AUTODOC PUBLIC spg/jstring/convertfullkana2romaji
|||	ConvertFullKana2Romaji - Convert a full-width kana string to a
|||	                         romaji string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertFullKana2Romaji(const char *string,
|||	                                 char *result, uint32 resultSize,
|||	                                 uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from full-width kana to romaji. The
|||	    conversion is done as well as possible. If certain characters from
|||	    the source string cannot be represented in the target character set,
|||	    the unknownFiller byte will be inserted in the result buffer in
|||	    their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/**
|||	AUTODOC PUBLIC spg/jstring/convertfullkana2hiragana
|||	ConvertFullKana2Hiragana - Convert a full-width kana string to a
|||	                           hiragana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertFullKana2Hiragana(const char *string,
|||	                                   char *result, uint32 resultSize,
|||	                                   uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from full-width kana to hiragana. The
|||	    conversion is done as well as possible. If certain characters from
|||	    the source string cannot be represented in the target character set,
|||	    the unknownFiller byte will be inserted in the result buffer in
|||	    their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/**
|||	AUTODOC PUBLIC spg/jstring/converthalfkana2fullkana
|||	ConvertHalfKana2FullKana - Convert a half-width kana string to a
|||	                           full-width kana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertHalfKana2FullKana(const char *string,
|||	                                   char *result, uint32 resultSize,
|||	                                   uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from half-width kana to full-width kana. The
|||	    conversion is done as well as possible. If certain characters from
|||	    the source string cannot be represented in the target character set,
|||	    the unknownFiller byte will be inserted in the result buffer in
|||	    their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/**
|||	AUTODOC PUBLIC spg/jstring/converthalfkana2romaji
|||	ConvertHalfKana2Romaji - Convert a half-width kana string to a
|||	                         romaji string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertHalfKana2Romaji(const char *string,
|||	                                 char *result, uint32 resultSize,
|||	                                 uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from half-width kana to romaji. The
|||	    conversion is done as well as possible. If certain characters from
|||	    the source string cannot be represented in the target character set,
|||	    the unknownFiller byte will be inserted in the result buffer in
|||	    their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/**
|||	AUTODOC PUBLIC spg/jstring/converthalfkana2hiragana
|||	ConvertHalfKana2Hiragana - Convert a half-width kana string to a
|||	                           hiragana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertHalfKana2Hiragana(const char *string,
|||	                                   char *result, uint32 resultSize,
|||	                                   uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from half-width kana to hiragana. The
|||	    conversion is done as well as possible. If certain characters from
|||	    the source string cannot be represented in the target character set,
|||	    the unknownFiller byte will be inserted in the result buffer in
|||	    their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/**
|||	AUTODOC PUBLIC spg/jstring/convertromaji2halfkana
|||	ConvertRomaji2HalfKana - Convert a romaji string to a
|||	                         half-width kana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertRomaji2HalfKana(const char *string,
|||	                                 char *result, uint32 resultSize,
|||	                                 uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from romaji to half-width kana. The
|||	    conversion is done as well as possible. If certain characters from
|||	    the source string cannot be represented in the target character set,
|||	    the unknownFiller byte will be inserted in the result buffer in
|||	    their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/**
|||	AUTODOC PUBLIC spg/jstring/convertromaji2fullkana
|||	ConvertRomaji2FullKana - Convert a romaji string to a
|||	                         full-width kana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertRomaji2FullKana(const char *string,
|||	                                 char *result, uint32 resultSize,
|||	                                 uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from romaji to full-width kana. The
|||	    conversion is done as well as possible. If certain characters from
|||	    the source string cannot be represented in the target character set,
|||	    the unknownFiller byte will be inserted in the result buffer in
|||	    their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/**
|||	AUTODOC PUBLIC spg/jstring/convertromaji2hiragana
|||	ConvertRomaji2Hiragana - Convert a romaji string to a
|||	                         hiragana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertRomaji2Hiragana(const char *string,
|||	                                 char *result, uint32 resultSize,
|||	                                 uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from romaji to hiragana. The
|||	    conversion is done as well as possible. If certain characters from
|||	    the source string cannot be represented in the target character set,
|||	    the unknownFiller byte will be inserted in the result buffer in
|||	    their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/**
|||	AUTODOC PUBLIC spg/jstring/converthiragana2halfkana
|||	ConvertHiragana2HalfKana - Convert a hiragana string to a
|||	                           half-width kana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertHiragana2HalfKana(const char *string,
|||	                                   char *result, uint32 resultSize,
|||	                                   uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from hiragana to half-width kana. The
|||	    conversion is done as well as possible. If certain characters from
|||	    the source string cannot be represented in the target character set,
|||	    the unknownFiller byte will be inserted in the result buffer in
|||	    their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/**
|||	AUTODOC PUBLIC spg/jstring/converthiragana2fullkana
|||	ConvertHiragana2FullKana - Convert a hiragana string to a
|||	                           full-width kana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertHiragana2FullKana(const char *string,
|||	                                   char *result, uint32 resultSize,
|||	                                   uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from hiragana to full-width kana. The
|||	    conversion is done as well as possible. If certain characters from
|||	    the source string cannot be represented in the target character set,
|||	    the unknownFiller byte will be inserted in the result buffer in
|||	    their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/**
|||	AUTODOC PUBLIC spg/jstring/converthiragana2romaji
|||	ConvertHiragana2Romaji - Convert a hiragana string to a
|||	                         romaji string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertHiragana2Romaji(const char *string,
|||	                                 char *result, uint32 resultSize,
|||	                                 uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from hiragana to romaji. The
|||	    conversion is done as well as possible. If certain characters from
|||	    the source string cannot be represented in the target character set,
|||	    the unknownFiller byte will be inserted in the result buffer in
|||	    their place.
|||
|||	  Arguments
|||
|||	    string                      The string to convert.
|||
|||	    result                      A memory buffer where the result of the
|||	                                conversion can be put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted, then this byte
|||	                                will be put into the result buffer in
|||	                                place of the character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    JSTR_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in jstring folio V24.
|||
|||	  Associated Files
|||
|||	    jstring.h
**/

/* keep the compiler happy... */
extern int foo;
