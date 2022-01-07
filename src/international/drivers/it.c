/* $Id: it.c,v 1.4 1994/05/12 19:19:55 vertex Exp $ */

/* Italian language driver for the International folio */

#include "types.h"
#include "langdrivers.h"
#include "intl.h"
#include "debug.h"


/*****************************************************************************/


static const char *dateStrings[]=
{
    "Domenica",
    "Lunedì",
    "Martedì",
    "Mercoledì",
    "Giovedì",
    "Venerdì",
    "Sabato",

    "Dom",
    "Lun",
    "Mar",
    "Mer",
    "Gio",
    "Ven",
    "Sab",

    "Gennaio",
    "Febbraio",
    "Marzo",
    "Aprile",
    "Maggio",
    "Giugno",
    "Luglio",
    "Agosto",
    "Settembre",
    "Ottobre",
    "Novembre",
    "Dicembre",
    "Lunario",		/* !!! incorrect, get real word !!! */

    "Gen",
    "Feb",
    "Mar",
    "Apr",
    "Mag",
    "Giu",
    "Lug",
    "Ago",
    "Set",
    "Ott",
    "Nov",
    "Dic",
    "Lun",		/* !!! incorrect, get real word !!! */

    "AM",
    "PM"
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
