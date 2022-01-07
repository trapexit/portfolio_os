/* $Id: fr.c,v 1.4 1994/05/12 19:19:55 vertex Exp $ */

/* French language driver for the International folio */

#include "types.h"
#include "langdrivers.h"
#include "intl.h"
#include "debug.h"


/*****************************************************************************/


static const char *dateStrings[]=
{
    "Dimanche",
    "Lundi",
    "Mardi",
    "Mercredi",
    "Jeudi",
    "Vendredi",
    "Samedi",

    "Dim",
    "Lun",
    "Mar",
    "Mer",
    "Jeu",
    "Ven",
    "Sam",

    "Janvier",
    "Février",
    "Mars",
    "Avril",
    "Mai",
    "Juin",
    "Juillet",
    "Août",
    "Septembre",
    "Octobre",
    "Novembre",
    "Décembre",
    "Lunaire"

    "Jan",
    "Fév",
    "Mars",
    "Avr",
    "Mai",
    "Juin",
    "Jull",
    "Août",
    "Sep",
    "Oct",
    "Nov",
    "Déc",
    "Lun",

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
