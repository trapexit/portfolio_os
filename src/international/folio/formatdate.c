/* $Id: formatdate.c,v 1.10 1994/12/08 00:15:53 vertex Exp $ */

#include "types.h"
#include "item.h"
#include "kernel.h"
#include "locales.h"
#include "utils.h"
#include "langdrivers.h"
#include "international_folio.h"


/*****************************************************************************/


/* convert parms to number of days... */
#define DMYToDays(day,month,year) (((year)-1+((month)+9)/12)*365 \
                                   +((year)-1+((month)+9)/12)/4 \
                                   -((year)-1+((month)+9)/12)/100 \
                                   +((year)-1+((month)+9)/12)/400 \
                                   +((((month)+9)%12)*306+5)/10 \
                                   +(day) - 1)


/*****************************************************************************/


#define STR_SIZE 100

uint32 GetDateStr(Locale *loc, DateComponents dc, unichar *result)
{
uint32 len;

    (*loc->loc_GetDateStr)(dc,result,STR_SIZE);
    len = 0;
    while (result[len])
        len++;

    return (len);
}


/*****************************************************************************/


static void DaysToDMY(uint32 days, GregorianDate *date)
{
uint32 year;
uint32 month;
uint32 day;
uint32 x;

    /* find year */
    x     = days;
    x    -= (days + 1) / 146097;     /* subtract quadcentury leap days */
    x    += x / 36524;               /* add century leap days          */
    x    -= (x + 1) / 1461;          /* subtract all leap days         */
    year  = x / 365;

    /* find day of year */
    x = days - (year * 365) - (year / 4) + (year / 100) - (year / 400);

    /* find month */
    month  = x / 153;
    month *= 5;
    month += 10 * (x % 153) / 305;

    /* find day of month */
    day  = 1 + x;
    day -= (int32)((month * 306 + 5) / 10);

    /* final adjustments... */
    month += 2;
    year  += month / 12;
    month %= 12;
    month++;

    date->gd_Day   = (uint8)day;
    date->gd_Month = (uint8)month;
    date->gd_Year  = (uint32)year;
}


/*****************************************************************************/


/* put a character at the end of the UniCode result */
#define PutCh(ch) {if (index < maxIndex) result[index++] = (unichar)ch; else tooLong = TRUE;}


/****************************************************************************/


static int32 internalFormatDate(Locale *loc, DateSpec spec,
                                const GregorianDate *date, uint32 days,
                                unichar *result, uint32 resultSize)
{
unichar        ch;
unichar        str[STR_SIZE];
unichar       *ptr;
uint32         num;
uint32         index;
uint32         maxIndex;
bool           tooLong;
uint32         width;
uint32         limit;
uint32         len;
unichar        pad;
bool           leftJustify;
unichar       *start;

    tooLong  = FALSE;
    index    = 0;
    maxIndex = (resultSize / sizeof(unichar)) - 1;

    /* process the formatting string */

    while ((ch = *spec++) != '\0')
    {
        if (ch != '%')
        {
            PutCh(ch);
        }
        else if (*spec == '%')
        {
            PutCh('%');
            spec++;
        }
        else
        {
            start       = spec;
            width       = 0;
            limit       = 0xffffffff;
            leftJustify = FALSE;
            pad         = ' ';

            /* parse flags... */
            while (TRUE)
            {
                switch (*spec)
                {
                    case '-': leftJustify = TRUE;
                              pad = ' ';
                              spec++;
                              continue;

                    case '0': if (!leftJustify)
                                  pad = '0';      /* left justify prevents 0 padding */
                              spec++;
                              continue;
                }
                break;
            }

            /* gather width argument */
            while ((*spec >= '0') && (*spec <= '9'))
            {
                width = width*10 + (*spec - (unichar)'0');
                spec++;
            }

            if (width >= (sizeof(str) / sizeof(unichar)))
                width = (sizeof(str) / sizeof(unichar)) - 1;

            /* do we have a limit? */
            if (*spec == '.')
            {
                spec++;
                limit = 0;
                while ((*spec >= '0') && (*spec <= '9'))
                {
                    limit = limit*10 + (*spec - (unichar)'0');
                    spec++;
                }
            }

            if (limit >= (sizeof(str) / sizeof(unichar)))
                limit = (sizeof(str) / sizeof(unichar)) - 1;

            switch (*spec++)
            {
                case 'D': len = ConvUnsigned((uint32)date->gd_Day,str);
                          break;

                case 'H': len = ConvUnsigned((uint32)date->gd_Hour,str);
                          break;

                case 'h': if ((num = (uint32)date->gd_Hour % 12) == 0)
                              num = 12;
                          len = ConvUnsigned(num,str);
                          break;

                case 'M': len = ConvUnsigned((uint32)date->gd_Minute,str);
                          break;

                case 'O': len = ConvUnsigned(date->gd_Month,str);
                          break;

                case 'N': len = GetDateStr(loc,(DateComponents)(MONTH_1 + date->gd_Month - 1),str);
                          break;

                case 'n': len = GetDateStr(loc,(DateComponents)(AB_MONTH_1 + date->gd_Month - 1),str);
                          break;

                case 'P': if (date->gd_Hour >= 12)
                              len = GetDateStr(loc,PM,str);
                          else
                              len = GetDateStr(loc,AM,str);
                          break;

                case 'S': len = ConvUnsigned((uint32)date->gd_Second,str);
                          break;

                case 'W': len = GetDateStr(loc,(DateComponents)(DAY_1 + (days+3) % 7),str);
                          break;

                case 'w': len = GetDateStr(loc,(DateComponents)(AB_DAY_1 + (days+3) % 7),str);
                          break;

                case 'Y': len = ConvUnsigned(date->gd_Year,str);
                          break;

                default : len = 0;
                          start--;
                          while (start < spec)
                          {
                              PutCh(*start++);
                              len++;
                          }

                          str[0] = 0;
                          break;
            }

            ptr = str;

            if (limit < len)
            {
                ptr = &str[len - limit];
                len = limit;
            }

            if (!leftJustify)
            {
                while (len < width)
                {
                    PutCh(pad);
                    len++;
                }
            }

            while (*ptr)
                PutCh(*ptr++);

            if (leftJustify)
            {
                while (len < width)
                {
                    PutCh(pad);
                    len++;
                }
            }
        }
    }

    /* null terminate the buffer... */
    result[index] = 0;

    if (tooLong)
        return (INTL_ERR_BUFFERTOOSMALL);

    return ((int32)index);
}


/****************************************************************************/


int32 intlFormatDate(Item locItem, DateSpec spec,
                     const GregorianDate *date,
                     unichar *result, uint32 resultSize)
{
Locale        *loc;
GregorianDate  temp;
uint32         days;

    TRACE(("INTLFORMATDATE: entering\n"));

#ifdef DEVELOPMENT
    /* the size is actually longer, but checking it would cause a fault anyway.... */
    if (!IsRamAddr(spec,2))
        return (INTL_ERR_BADDATESPEC);

    if (!IsRamAddr(date,sizeof(GregorianDate)))
        return (INTL_ERR_BADGREGORIANDATE);

    if (ValidateMem(KernelBase->kb_CurrentTask,result,resultSize) < 0)
        return (INTL_ERR_BADRESULTBUFFER);

    if (resultSize < sizeof(unichar))
        return (INTL_ERR_BUFFERTOOSMALL);
#endif

    /* find ourselves... */
    loc = (Locale *)CheckItem(locItem,(uint8)NST_INTL,INTL_LOCALE_NODE);
    if (!loc)
        return (INTL_ERR_BADITEM);

    /* validate the date... */
    if ((date->gd_Hour   >= 24)
     || (date->gd_Minute >= 60)
     || (date->gd_Second >= 60)
     || (date->gd_Day    >= 32))
    {
        return (INTL_ERR_IMPOSSIBLEDATE);
    }

    /* convert the date into a number of days... */
    days = DMYToDays(date->gd_Day,date->gd_Month,date->gd_Year);

    /* now convert the number of days back into a date */
    DaysToDMY(days,&temp);

    /* if the original and the retranslated dates don't agree, we've got a
     * bogus original date...
     */
    if ((date->gd_Day   != temp.gd_Day)
     || (date->gd_Month != temp.gd_Month)
     || (date->gd_Year  != temp.gd_Year))
    {
        return (INTL_ERR_IMPOSSIBLEDATE);
    }

    return (internalFormatDate(loc,spec,date,days,result,resultSize));
}
