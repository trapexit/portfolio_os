/* $Id: formatnumber.c,v 1.14 1994/12/08 00:15:53 vertex Exp $ */

/* #define TRACING */

#include "types.h"
#include "item.h"
#include "kernel.h"
#include "string.h"
#include "stdio.h"
#include "locales.h"
#include "utils.h"
#include "international_folio.h"


/*****************************************************************************/


/* put an ASCII character at the end of the UniCode result */
#define PutCh(ch) {if (index < maxIndex) target[index++] = (unichar)ch; else tooLong = TRUE;}

/* put a UniCode string at the end of the UniCode result */
#define PutStr(str) {uint32 i = 0;\
                    while (str[i])\
                    {\
                        if (index >= maxIndex)\
                        {\
                            tooLong = TRUE;\
                            break;\
                        }\
                        target[index++] = str[i++];\
                    }}


/*****************************************************************************/


static int32 internalFormatNumber(Locale *loc,
                                  const NumericSpec *spec,
                                  uint32 whole, uint32 frac,
                                  bool negative, bool doFrac,
                                  unichar *result, uint32 resultSize)
{
GroupingDesc  groups;
unichar      *sep;
unichar      *radix;
GroupingDesc  fracGroups;
unichar      *fracSep;
uint32        minFracs;
uint32        maxFracs;
uint32        maxIndex;
uint32        index;
unichar       wholePart[16];
char          fracPart[16];
unichar       temp[80];
unichar      *format;
bool          tooLong;
uint32        i;
uint32        len;
unichar      *target;
unichar       ch;

    TRACE(("INTERNALFORMATNUMBER: entering with whole %ld, frac %ld\n",whole,frac));

    tooLong  = FALSE;
    maxIndex = (resultSize / sizeof(unichar)) - 1;
    index    = 0;
    target   = result;

    /* if we have a "zero" string */
    if (spec->ns_Zero)
    {
        /* if the whole and frac parts are zero... */
        if ((whole == 0) && ((frac == 0) || !doFrac))
        {
            PutStr(spec->ns_Zero);
            result[index] = 0;

            if (tooLong)
            {
                TRACE(("INTERNALFORMATNUMBER: exiting with 'buffer too small'\n"));

                return (INTL_ERR_BUFFERTOOSMALL);
            }

            TRACE(("INTERNALFORMATNUMBER: exiting with %d\n",index));

            return ((int32)index);
        }
    }

    if (negative)
    {
        /* we're formatting a negative number... */
        groups     = spec->ns_NegGroups;
        sep        = spec->ns_NegGroupSep;
        radix      = spec->ns_NegRadix;
        fracGroups = spec->ns_NegFractionalGroups;
        fracSep    = spec->ns_NegFractionalGroupSep;
        format     = spec->ns_NegFormat;
        minFracs   = spec->ns_NegMinFractionalDigits;
        maxFracs   = spec->ns_NegMaxFractionalDigits;
    }
    else
    {
        /* we're formatting a positive number... */
        groups     = spec->ns_PosGroups;
        sep        = spec->ns_PosGroupSep;
        radix      = spec->ns_PosRadix;
        fracGroups = spec->ns_PosFractionalGroups;
        fracSep    = spec->ns_PosFractionalGroupSep;
        format     = spec->ns_PosFormat;
        minFracs   = spec->ns_PosMinFractionalDigits;
        maxFracs   = spec->ns_PosMaxFractionalDigits;
    }

    /* if there's no separator sequence, disable grouping */
    if (!sep)
        groups = 0;

    /* if there's no frac separator sequence, disable frac grouping */
    if (!fracSep)
        fracGroups = 0;

    /* if we don't want any decimal digits, disable fraction processing */
    if (maxFracs == 0)
        doFrac = FALSE;

    /* if there's no radix character, disable fraction processing */
    if (!radix)
        doFrac = FALSE;

    /* If we're doing post-processing, we generate the initial string
     * into a temporary buffer, and we'll copy it to the result buffer later
     */
    if (format)
    {
        target   = temp;
        maxIndex = sizeof(temp) - 1;
    }

    /* is the number negative? */
    if (negative)
    {
        /* If we don't do post-processing, then put the sign there explicitly.
         * Otherwise, we assume that the formatting string contains the
         * sign symbol itself.
         */

        if (!format)
            PutCh('-');
    }

    /* now format the whole part, adding group separators where needed... */
    len = ConvUnsigned(whole,wholePart);
    i   = 0;
    while (wholePart[i])
    {
        if ((groups & (1L << (len - i - 1))) && i)
            PutStr(sep);

        PutCh(wholePart[i++]);
    }

    if (doFrac)
    {
        /* insert the radix characters... */
        PutStr(radix);

        sprintf(fracPart,"%09u",frac);
        len = 9;
        while (fracPart[len - 1] == '0')
            fracPart[--len] = 0;

        if (len > maxFracs)
        {
            fracPart[maxFracs] = 0;
        }
        else
        {
            while ((len < minFracs) && (len < sizeof(fracPart)-1))
                fracPart[len++] = '0';

            fracPart[len] = 0;
        }

        /* format the fractional part, adding group separators where needed */
        i = 0;
        while (fracPart[i])
        {
            PutCh(fracPart[i++]);

            if (fracPart[i])
            {
                if (fracGroups & 1)
                    PutStr(fracSep);
            }

            fracGroups /= 2;
        }
    }

    target[index] = 0;

    if (format && !tooLong)
    {
        target   = result;
        index    = 0;
        maxIndex = (resultSize / 2) - 1;

        /* process the formatting string */
        while ((ch = *format++) != '\0')
        {
            if (ch != '%')
            {
                PutCh(ch);
            }
            else
            {
                if (*format == 's')
                {
                    PutStr(temp);
                }
                else
                {
                    PutCh(*format);
                }

                if (*format)
                    format++;
            }
        }

        target[index] = 0;
    }

    if (tooLong)
    {
        TRACE(("INTERNALFORMATNUMBER: exiting with 'buffer too small\n"));

        return (INTL_ERR_BUFFERTOOSMALL);
    }

    TRACE(("INTERNALFORMATNUMBER: exiting with %ld\n",index));

    return ((int32)index);
}


/*****************************************************************************/


#ifdef DEVELOPMENT
static bool CheckPointers(unichar *pointers, ...)
{
unichar **ptr;
uint32    i;

    ptr = &pointers;
    for (i = 0; i < 9; i++)
    {
        if ((*ptr != NULL) && (!IsRamAddr(*ptr,2)))
            return FALSE;

        ptr++;
    }

    return TRUE;
}
#endif


/*****************************************************************************/


int32 intlFormatNumber(Item locItem,
                       const NumericSpec *spec,
                       uint32 whole, uint32 frac,
                       bool negative, bool doFrac,
                       unichar *result, uint32 resultSize)
{
Locale *loc;

    TRACE(("FORMATNUMBER: entering\n"));

#ifdef DEVELOPMENT
    if (!IsRamAddr(spec,sizeof(NumericSpec)))
        return (INTL_ERR_BADNUMERICSPEC);

    if (!CheckPointers(spec->ns_PosGroupSep,spec->ns_PosRadix,
                       spec->ns_PosFractionalGroupSep,spec->ns_PosFormat,
                       spec->ns_NegGroupSep,spec->ns_NegRadix,
                       spec->ns_NegFractionalGroupSep,spec->ns_NegFormat,
                       spec->ns_Zero))
    {
        return (INTL_ERR_BADNUMERICSPEC);
    }

    if ((spec->ns_PosMinFractionalDigits > spec->ns_PosMaxFractionalDigits)
     || (spec->ns_NegMinFractionalDigits > spec->ns_NegMaxFractionalDigits)
     || (spec->ns_Flags))
    {
        return (INTL_ERR_BADNUMERICSPEC);
    }

    if (ValidateMem(KernelBase->kb_CurrentTask,result,resultSize) < 0)
        return (INTL_ERR_BADRESULTBUFFER);

    if (resultSize < sizeof(unichar))
        return (INTL_ERR_BUFFERTOOSMALL);

    if (frac > 999999999)
        return (INTL_ERR_FRACTOOLARGE);
#endif

    loc = (Locale *)CheckItem(locItem,(uint8)NST_INTL,INTL_LOCALE_NODE);
    if (!loc)
        return (INTL_ERR_BADITEM);

    return (internalFormatNumber(loc,spec,whole,frac,negative,doFrac,result,resultSize));
}
