/* $Id: autodocs.c,v 1.7 1994/11/05 00:44:53 vertex Exp $ */

/**
|||	AUTODOC PUBLIC spg/international/intlopenlocale
|||	intlOpenLocale - Gain access to a Locale Item.
|||
|||	  Synopsis
|||
|||	    Item intlOpenLocale(const TagArg *tags);
|||
|||	  Description
|||
|||	    This function returns a Locale Item. You can then use
|||	    intlLookupLocale() to gain access to the localization
|||	    information within the Locale Item. This information enables
|||	    software to adapt to different languages and customs
|||	    automatically at run-time, thus creating truly international
|||	    software.
|||
|||	  Arguments
|||
|||	    tags                        A pointer to an array of tag arguments
|||	                                containing extra data for this
|||	                                function. This must currently always
|||	                                be NULL.
|||
|||	  Return Value
|||
|||	    The function returns the Item number of a Locale.
|||	    You can use intlLookupLocale() to get a pointer to the
|||	    Locale structure, which contains localization information.
|||
|||	  Implementation
|||
|||	    Macro implemented in intl.h V24.
|||
|||	  Associated Files
|||
|||	    intl.h
|||
|||	  Notes
|||
|||	    Once you are done with the Locale Item, you should call
|||	    intlCloseLocale().
|||
|||	  See Also
|||
|||	    intlCloseLocale(), intlLookupLocale()
**/

/**
|||	AUTODOC PUBLIC spg/international/intlcloselocale
|||	intlCloseLocale - Tells the system you are done using a given Locale
|||	                  Item.
|||
|||	  Synopsis
|||
|||	    Err intlCloseLocale(Item locItem);
|||
|||	  Description
|||
|||	    This function concludes the title's use of the given Locale
|||	    Item. After this call is made, the Locale Item may no longer
|||	    be used.
|||
|||	  Arguments
|||
|||	    locItem                     The Locale Item, as obtained from
|||	                                intlOpenLocale().
|||
|||	  Return Value
|||
|||	    0                           Item successfully closed.
|||
|||	    INTL_ERR_BADITEM            locItem was not an existing Locale
|||	                                Item.
|||
|||	    INTL_ERR_CANTCLOSEITEM      At attempt was made to close this
|||	                                Locale Item more often than it was
|||	                                opened.
|||
|||	  Implementation
|||
|||	    Macro implemented in intl.h V24.
|||
|||	  Associated Files
|||
|||	    intl.h
|||
|||	  See Also
|||
|||	    intlOpenLocale(), intlLookupLocale()
**/

/**
|||	AUTODOC PUBLIC spg/international/intllookuplocale
|||	intlLookupLocale - Returns a pointer to a Locale structure.
|||
|||	  Synopsis
|||
|||	    Locale *intlLookupLocale(Item locItem);
|||
|||	  Description
|||
|||	    This macro returns a pointer to a Locale structure. The
|||	    structure can then be examined and its contents used to
|||	    localize titles.
|||
|||	  Arguments
|||
|||	    locItem                     A Locale Item, as obtained from
|||	                                intlOpenLocale().
|||
|||	  Return Value
|||
|||	    The macro returns a pointer to a Locale structure, which
|||	    contains localization information, or NULL if the supplied Item
|||	    was not a valid Locale Item.
|||
|||	  Implementation
|||
|||	    Macro implemented in intl.h V24.
|||
|||	  Associated Files
|||
|||	    intl.h
|||
|||	  Example
|||
|||	    {
|||	    Item    |t;
|||	    Locale *loc;
|||
|||	        it = intlOpenLocale(NULL);
|||	        if (it >= 0)
|||	        {
|||	            loc = intlLookupLocale(it);
|||
|||	            // you can now read fields in the Locale structure.
|||	            printf("Language is %ld\n",loc->loc_Language);
|||
|||	            intlCloseLocale(it);
|||	        }
|||	    }
|||
|||	  See Also
|||
|||	    intlOpenLocale(), intlCloseLocale()
**/

/**
|||	AUTODOC PUBLIC spg/international/intlformatdate
|||	intlFormatDate - Format a date in a localized manner.
|||
|||	  Synopsis
|||
|||	    int32 intlFormatDate(Item locItem,
|||	                         DateSpec spec,
|||	                         const GregorianDate *date,
|||	                         unichar *result, uint32 resultSize);
|||
|||	  Description
|||
|||	    This function formats a date according to a template and
|||	    to the rules specified by the provided Locale Item. The
|||	    formatting string works in a manner similar to how printf()
|||	    formatting strings are handled, except using some specialized
|||	    format commands, tailored to date generation. The following
|||	    format commands are supported:
|||
|||	       %D - day of month
|||	       %H - hour using 24-hour style
|||	       %h - hour using 12-hour style
|||	       %M - minutes
|||	       %N - month name
|||	       %n - abbreviated month name
|||	       %O - month number
|||	       %P - AM or PM strings
|||	       %S - seconds
|||	       %W - weekday name
|||	       %w - abbreviated weekday name
|||	       %Y - year
|||
|||	    In addition to straight format commands, the formatting
|||	    string can also specify a field width, a field limit, and a
|||	    field pad character. This is done in a manner identical to
|||	    the way printf() formatting strings specify these values. That
|||	    is:
|||
|||	       %[flags][width][.limit]command
|||
|||	    where flags can be "-" or "0", width is a positive numeric value,
|||	    limit is a positive numeric value, and command is one of the
|||	    format commands mentioned above. Refer to documentation on the
|||	    standard C printf() function for more information on how these
|||	    numbers and flags interact.
|||
|||	    A difference with standard printf() processing is that the limit
|||	    value is applied starting from the rightmost digits, instead of
|||	    the leftmost. So for example:
|||
|||	       %.2Y
|||
|||	    will print the rightmost two digits of the current year.
|||
|||	  Arguments
|||
|||	    locItem                     A Locale Item, as obtained from
|||	                                intlOpenLocale().
|||
|||	    spec                        The formatting template describing the
|||	                                date layout. This value is typically
|||	                                taken from the Locale structure
|||	                                (loc_Date, loc_ShortDate, loc_Time,
|||	                                loc_ShortTime), but it can also be
|||	                                built up by the title for custom
|||	                                formatting.
|||
|||	    date                        The date to convert into a string.
|||
|||	    result                      Where the result of the formatting
|||	                                is put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code. The string copied into
|||	    the result buffer is guaranteed to be NULL-terminated.
|||
|||	    >= 0                        Number of characters copied.
|||
|||	    INTL_ERR_BADRESULTBUFFER    The result buffer pointer was NULL or
|||	                                wasn't in valid writable memory.
|||
|||	    INTL_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	    INTL_ERR_BADDATESPEC        The pointer to the DateSpec array was
|||	                                bad.
|||
|||	    INTL_ERR_BADITEM            locItem was not an existing Locale
|||	                                Item.
|||
|||	    INTL_ERR_BADDATE            The date specified in the GregorianDate
|||	                                structure is not a valid date. For
|||	                                example, if the gd_Month is greater
|||	                                than 12.
|||
|||	  Implementation
|||
|||	    Folio call implemented in international folio V24.
|||
|||	  Associated Files
|||
|||	    intl.h
|||
|||	  See Also
|||
|||	    intlOpenLocale(), intlFormatNumber()
**/

/**
|||	AUTODOC PUBLIC spg/international/intlformatnumber
|||	intlFormatNumber - Format a number in a localized manner.
|||
|||	  Synopsis
|||
|||	    int32 intlFormatNumber(Item locItem,
|||	                           const NumericSpec *spec,
|||	                           uint32 whole, uint32 frac,
|||	                           bool negative, bool doFrac,
|||	                           unichar *result, uint32 resultSize);
|||
|||	  Description
|||
|||	    This function formats a number according to the rules
|||	    contained in the NumericSpec structure. The NumericSpec
|||	    structure is normally taken from a Locale structure.
|||	    The Locale structure contains three initialized NumericSpec
|||	    structures (loc_Numbers, loc_Currency, and loc_SmallCurrency)
|||	    which let you format numbers in an appropriate manner for the
|||	    current system.
|||
|||	    You can create your own NumericSpec structure which lets you
|||	    use intlFormatNumber() to handle custom formatting needs. The
|||	    fields in the structure have the following meaning:
|||
|||	        ns_PosGroups
|||             A GroupingDesc value defining how digits are grouped to the
|||	        left of the decimal character. A GroupingDesc is simply a
|||	        32-bit bitfield. Every ON bit in the bitfield indicates that
|||	        the separator sequence should be inserted after the associated
|||	        digit. So if the third bit (bit #2) is ON, it means that the
|||	        grouping separator should be inserted after the third digit
|||	        of the formatted number.
|||
|||	        ns_PosGroupSep
|||	        A string used to separate groups of digits to the left of the
|||	        decimal character.
|||
|||	        ns_PosRadix
|||	        A string used as a decimal character.
|||
|||	        ns_PosFractionalGroups
|||	        A GroupingDesc value defining how digits are grouped to the
|||	        right of the decimal character.
|||
|||	        ns_PosFractionalGroupSep
|||	        A string used to separate groups of digits to the right of the
|||	        decimal character.
|||
|||	        ns_PosFormat
|||	        This field is used to do post-processing on a formatted
|||	        number. This is typically used to add currency notation around
|||	        a numeric value. The string in this field is used as a
|||	        format string in a sprintf() function call, and the formatted
|||	        numeric value is supplied as a parameter to the same sprintf()
|||	        call. For example, if the ns_PosFormat field is defined as
|||	        "$%s", and the formatted numeric value is "0.02". then the
|||	        result of the post-processing will be "$0.02". When this
|||	        field is NULL, no post-processing occurs.
|||
|||	        ns_PosMinFractionalDigits
|||	        Specifies the minimum number of digits to display to the right
|||	        of the decimal character. If there are not enough digits, then
|||	        the string will be padded on the right with 0s.
|||
|||	        ns_PosMaxFractionalDigits
|||	        Specifies the maximum number of digits to display to the right
|||	        of the decimal character. Any excess digits are just removed.
|||
|||	        ns_NegGroups, ns_NegGroupSep, ns_NegRadix,
|||	        ns_NegFractionalGroups, ns_NegFractionalGroupSep,
|||	        ns_NegFormat, ns_NegMinFractionalDigits,
|||	        ns_NegMaxFractionalDigits
|||		These fields have the same function as the eight fields
|||	        described above, except that they are used to process negative
|||	        amounts, while the previous fields are used for positive
|||	        amounts.
|||
|||	        ns_Zero
|||	        If the number being processed is 0, then this string pointer
|||	        us used 'as-is' and is copied directly into the result buffer.
|||	        If this field is NULL, the number is formatted as if it
|||	        were a positive number.
|||
|||	        ns_Flags
|||	        This is reserved for future use and must always be 0.
|||
|||	  Arguments
|||
|||	    locItem                     A Locale Item, as obtained from
|||	                                intlOpenLocale().
|||
|||	    spec                        The formatting template describing the
|||	                                number layout. This structure is
|||	                                typically taken from a Locale structure
|||	                                (loc_Numbers, loc_Currency,
|||	                                loc_SmallCurrency), but it can also be
|||	                                built up by the title for custom
|||	                                formatting.
|||
|||	    whole                       The whole component of the number to
|||	                                format. (The part of the number to the
|||	                                left of the radix character.)
|||
|||	    frac                        The decimal component of the number to
|||	                                format. (The part of the number to the
|||	                                right of the radix character.). This
|||	                                is specified in number of billionth.
|||	                                For example, to represent .5, you
|||	                                would use 500000000. To represent .0004,
|||	                                you would use 400000.
|||
|||	    negative                    TRUE if the number is negative, and
|||	                                FALSE if the number is positive.
|||
|||	    doFrac                      TRUE if a complete number with a
|||	                                decimal mark and decimal digits is
|||	                                desired. FALSE if only the whole part
|||	                                of the number should be output.
|||
|||	    result                      Where the result of the formatting is
|||	                                put.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code. The string copied into
|||	    the result buffer is guaranteed to be NULL-terminated.
|||
|||	    >= 0                        Number of characters copied.
|||
|||	    INTL_ERR_BADNUMERICSPEC     The pointer to the NumericSpec
|||	                                structure  was bad.
|||
|||	    INTL_ERR_NORESULTBUFFER     "result" was NULL.
|||
|||	    INTL_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	    INTL_ERR_BADITEM            locItem was not an existing Locale
|||	                                Item.
|||
|||	  Implementation
|||
|||	    Folio call implemented in international folio V24.
|||
|||	  Associated Files
|||
|||	    intl.h
|||
|||	  See Also
|||
|||	    intlOpenLocale(), intlFormatDate()
**/

/**
|||	AUTODOC PUBLIC spg/international/intlgetcharattrs
|||	intlGetCharAttrs - Returns attributes describing a given
|||	                   character.
|||
|||	  Synopsis
|||
|||	    uint32 intlGetCharAttrs(Item locItem, unichar character);
|||
|||	  Description
|||
|||	    This function examines the provided UniCode character and
|||	    returns general information about it.
|||
|||	  Arguments
|||
|||	    locItem                     A Locale Item, as obtained from
|||	                                intlOpenLocale().
|||
|||	    character                   The character to get the attribute of.
|||
|||	  Return Value
|||
|||	    Returns a bit mask, with bit sets to indicate various
|||	    characteristics as defined by the UniCode standard. The
|||	    possible bits are:
|||
|||	      INTL_ATTRF_UPPERCASE        This character is uppercase.
|||
|||	      INTL_ATTRF_LOWERCASE        This character is lowercase.
|||
|||	      INTL_ATTRF_PUNCTUATION      This character is a punctuation mark.
|||
|||	      INTL_ATTRF_DECIMAL_DIGIT    This character is a numeric digit.
|||
|||	      INTL_NUMBER                 This character represent a numerical
|||	                                  value not representable as a single
|||	                                  decimal digit. For example, a
|||	                                  character 0x00bc represents the
|||	                                  constant 1/2.
|||
|||	      INTL_ATTRF_NONSPACING       This character is a non-spacing mark.
|||
|||	      INTL_SPACE                  This character is a space character.
|||
|||	    If the value returned by this function is negative, then it is
|||	    not a bit mask, and is instead an error code.
|||
|||	    INTL_ERR_BADITEM            locItem was not an existing Locale
|||	                                Item.
|||
|||	  Implementation
|||
|||	    Folio call implemented in international folio V24.
|||
|||	  Associated Files
|||
|||	    intl.h
|||
|||	  Caveats
|||
|||	    This function currently does not report any attributes for many
|||	    upper UniCode characters. Only the ECMA Latin-1 character page
|||	    (0x0000 to 0x00ff) is handled correctly at this time.
|||
|||	  See Also
|||
|||	    intlOpenLocale(), intlConvertString()
**/

/**
|||	AUTODOC PUBLIC spg/international/intlcomparestrings
|||	intlCompareStrings - Compares two strings for collation purposes.
|||
|||	  Synopsis
|||
|||	    int32 intlCompareStrings(Item locItem,
|||	                             const unichar *string1,
|||	                             const unichar *string2);
|||
|||	  Description
|||
|||	    Compares two strings according to the collation rules of the
|||	    Locale item's language.
|||
|||	  Arguments
|||
|||	    locItem                     A Locale Item, as obtained from
|||	                                intlOpenLocale().
|||
|||	    string1                     The first string to compare.
|||
|||	    string2                     The second string to compare.
|||
|||	  Return Value
|||
|||	    -1                          (string1 < string2)
|||
|||	    0                           (string1 == string2)
|||
|||	    1                           (string1 > string2)
|||
|||	    INTL_ERR_BADITEM            locItem was not an existing Locale
|||	                                Item.
|||
|||	  Implementation
|||
|||	    Folio call implemented in international folio V24.
|||
|||	  Associated Files
|||
|||	    intl.h
|||
|||	  See Also
|||
|||	    intlOpenLocale(), intlConvertString()
**/

/**
|||	AUTODOC PUBLIC spg/international/intlconvertstring
|||	intlConvertString - Changes certain attributes of a string.
|||
|||	  Synopsis
|||
|||	    int32 intlConvertString(Item locItem,
|||	                            const unichar *string,
|||	                            unichar *result, uint32 resultSize,
|||	                            uint32 flags);
|||
|||	  Description
|||
|||	    Converts character attributes within a string. The flags
|||	    argument specifies the type of conversion to perform.
|||
|||	  Arguments
|||
|||	    locItem                     A Locale Item, as obtained from
|||	                                intlOpenLocale().
|||
|||	    string                      The string to convert.
|||
|||	    result                      Where the result of the conversion is
|||	                                put. This area must be at least as
|||	                                large as the number of bytes in the
|||	                                string.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    flags                       Description of the conversion process
|||	                                to apply:
|||
|||	                                INTL_CONVF_UPPERCASE will convert all
|||	                                characters to uppercase if possible.
|||
|||	                                INTL_CONVF_LOWERCASE will convert all
|||	                                characters to lowercase if possible.
|||
|||	                                INTL_CONVF_STRIPDIACRITICALS will
|||	                                remove diacritical marks from all
|||	                                characters.
|||
|||	                                You can also specify:
|||
|||                                     (INTL_CONVF_UPPERCASE|INTL_CONVF_STRIPDIACRITICALS)
|||	                                or
|||	                                (INTL_CONVF_LOWERCASE|INTL_CONVF_STRIPDIACRITICALS) in order
|||
|||	                                to perform two conversions in one call.
|||
|||	                                If flags is 0, then a straight copy
|||	                                occurs.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code. The string copied into
|||	    the result buffer is guaranteed to be NULL-terminated.
|||
|||	    >= 0                        Number of characters copied.
|||
|||	    INTL_ERR_BADSOURCEBUFFER    The string pointer supplied was bad.
|||
|||	    INTL_ERR_BADRESULTBUFFER    The result buffer pointer was NULL or
|||	                                wasn't in valid writable memory.
|||
|||	    INTL_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	    INTL_ERR_BADITEM            locItem was not an existing Locale
|||	                                Item.
|||
|||	  Implementation
|||
|||	    Folio call implemented in international folio V24.
|||
|||	  Associated Files
|||
|||	    intl.h
|||
|||	  Caveats
|||
|||	    This function is not as smart as it could be. Characters
|||	    greater than 0x00ff are always copied as-is with no conversion
|||	    performed on them.
|||
|||	  See Also
|||
|||	    intlOpenLocale(), intlCompareStrings()
**/

/**
|||	AUTODOC PUBLIC spg/international/intltransliteratestring
|||	intlTransliterateString - Converts a string between character sets.
|||
|||	  Synopsis
|||
|||	    int32 intlTransliterateString(const void *string,
|||	                                  CharacterSets stringSet,
|||	                                  void *result,
|||	                                  CharacterSets resultSet,
|||	                                  uint32 resultSize,
|||	                                  uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string between two character sets. This is typically
|||	    used to convert a string from or to UniCode. The conversion
|||	    is done as well as possible. If certain characters from the source
|||	    string cannot be represented in the destination character set,
|||	    the unknownFiller byte will be inserted in the result buffer
|||	    in their place.
|||
|||	  Arguments
|||
|||	    string                      The string to transliterate.
|||
|||	    stringSet                   The character set of the string to
|||	                                transliterate. This describes the
|||	                                interpretation of the bytes in the
|||	                                source string.
|||
|||	    result                      A memory buffer where the result of the
|||	                                transliteration can be put.
|||
|||	    resultSet                   The character set to use for the
|||	                                resulting string.
|||
|||	    resultSize                  The number of bytes available in the
|||	                                result buffer. This limits the number
|||	                                of bytes which are put into the buffer.
|||
|||	    unknownFiller               If a character sequence cannot be
|||	                                adequately converted from the source
|||	                                character set, then this byte will
|||	                                be put into the result buffer in
|||	                                place of the character sequence. When
|||	                                converting to a 16-bit character set,
|||	                                then this byte will be inserted twice
|||	                                in a row within the result buffer.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result
|||	    buffer. If negative, then an error code.
|||
|||	    >= 0                        Number of characters in the result
|||	                                buffer.
|||
|||	    INTL_ERR_BADSOURCEBUFFER    The string pointer supplied was bad.
|||
|||	    INTL_ERR_BADCHARACTERSET    The "stringSet" or "resultSet"
|||	                                arguments did not specify valid
|||	                                character sets.
|||
|||	    INTL_ERR_BADRESULTBUFFER    The result buffer pointer was NULL or
|||	                                wasn't in valid writable memory.
|||
|||	    INTL_ERR_BUFFERTOOSMALL     There was not enough room in the
|||	                                result buffer.
|||
|||	    INTL_ERR_BADITEM            locItem was not an existing Locale
|||	                                Item.
|||
|||	  Implementation
|||
|||	    Folio call implemented in international folio V24.
|||
|||	  Caveats
|||
|||	    This function is not as smart as it could be. When converting from
|||	    UniCode to ASCII, characters not in the first UniCode page (codes
|||	    greater than 0x00ff) are never converted and always replaced with
|||	    the unknownFiller byte. The upper pages of the UniCode set contain
|||	    many characters which could be converted to equivalent ASCII
|||	    characters, but these are not supported at this time.
|||
|||	  Associated Files
|||
|||	    intl.h
**/

/* keep the compiler happy... */
extern int foo;
