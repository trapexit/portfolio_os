/* $Id: nl.c,v 1.3 1994/05/12 19:19:55 vertex Exp $ */

/* Dutch language driver for the International folio */

#include "types.h"
#include "langdrivers.h"
#include "intl.h"
#include "debug.h"


/*****************************************************************************/


static const char *dateStrings[]=
{
    "zondag",
    "maandag",
    "dinsdag",
    "woensdag",
    "donderdag",
    "vrijdag",
    "zaterdag",

    "zo",
    "ma",
    "di",
    "wo",
    "do",
    "vr",
    "za",

    "januari",
    "februari",
    "maart",
    "april",
    "mei",
    "juni",
    "juli",
    "augustus",
    "september",
    "oktober",
    "november",
    "december",
    "lunar",		/* !!! incorrect, get real word !!! */

    "jan",
    "feb",
    "maa",
    "apr",
    "mei",
    "jun",
    "jul",
    "aug",
    "sep",
    "oct",
    "nov",
    "dec",
    "lun",		/* !!! incorrect, get real word !!! */

    "VM",
    "NM"
};


/*****************************************************************************/


static bool GetDateStr(DateComponents dc, unichar *result, uint32 resultSize)
{
uint32 i;

    if (dc > PM)
        return (FALSE);

    i = 0;
    while (dateStrings[i] && (i < resultSize - 1))
    {
        result[i] = dateStrings[dc][i];
        i++;
    }
    result[i] = 0;

    return (TRUE);
}


/*****************************************************************************/


static LanguageDriverInfo driverInfo =
{
    sizeof(LanguageDriverInfo),

    NULL,
    NULL,
    NULL,
    GetDateStr
};


LanguageDriverInfo *main(void)
{
#ifdef DEVELOPMENT
    print_vinfo();
#endif

    return (&driverInfo);
}
