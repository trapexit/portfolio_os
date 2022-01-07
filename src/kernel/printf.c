/* $Id: printf.c,v 1.13 1994/08/12 23:42:27 vertex Exp $ */
/* printf.c: ANSI draft (X3J11 Oct 86) part of section 4.9 code
 *
 * Copyright (C) Codemist Ltd., 1988
 * Copyright (C) Advanced Risc Machines Ltd., 1991
 * Copyright (C) The 3DO Company, 1993
 *
 * Based on version 0.05b from ARM
 *
 * printf() and its friends return the number of characters planted. In
 * the case of sprintf() this does not include the terminating '\0'.
 */

#include "types.h"
#include "stdio.h"
#include "stdarg.h"
#include "ctype.h"
#include "string.h"


/*****************************************************************************/


/*#define FLOATING_POINT */
/*#define HOST_HAS_BCD_FLT */


/*****************************************************************************/


#define isnumber(ch) isdigit(ch)


/*****************************************************************************/


/* parsing state flag bits... */
#define _LJUSTIFY         01
#define _SIGNED           02
#define _BLANKER          04
#define _VARIANT         010
#define _PRECGIVEN       020
#define _LONGSPECIFIER   040
#define _SHORTSPEC      0100
#define _PADZERO        0200
#define _FPCONV         0400


/*****************************************************************************/


/* some handy dandy macros... */

#define intofdigit(x) ((x)-'0')

#define pr_padding(ch, n, p)  while(--n>=0) charcount++, (*putc)(ch, p);

#define pre_padding(p)                                                    \
        if (!(flags & _LJUSTIFY))                                           \
        {   char padchar = flags & _PADZERO ? '0' : ' ';                  \
            pr_padding(padchar, width, p); }

#define post_padding(p)                                                   \
        if (flags & _LJUSTIFY)                                              \
        {   pr_padding(' ', width, p); }

#define get_arg(arg,mode) (argSlot ? ((mode *)argArray)[argSlot-1] : va_arg(arg,mode))


/*****************************************************************************/


typedef int (*fp_print)(int ch, double *d, char buff[], int flags,
                        char **lvprefix, int *lvprecision, int *lvbefore_dot,
                        int *lvafter_dot);


/*****************************************************************************/


static int printf_display(FILE *p, int flags, int ch, int precision, int width,
                          unsigned int v, fp_print fp_display_fn, char *prefix,
                          char *hextab, double *d, int32 (*putc)(char,FILE *))
{
int  charcount  = 0;
int  len        = 0;
int  before_dot = -1;
int  after_dot  = -1;
char buff[32];       /* used to accumulate value to print    */

    if ((flags & _FPCONV+_PRECGIVEN) == 0)
        precision = 1;

    switch (ch)
    {
        case 'p':
        case 'X':
        case 'x': while (v)
                  {
                      buff[len++] = hextab[v & 0xf];
                      v = v >> 4;
                  }
                  break;

        case 'o': while (v)
                  {
                      buff[len++] = '0' + (v & 07);
                      v = v >> 3;
                  }
                  break;

        case 'u':
        case 'i':
        case 'd': while (v)
                  {
                      unsigned int vDiv10 = v/10;
                      buff[len++] = '0' + v - vDiv10 * 10U;
                      v = vDiv10;
                  }
                  break;

#ifdef FLOATING_POINT
        case 'f':
        case 'g':
        case 'G':
        case 'e':
        case 'E': if (fp_display_fn)
                  {
                      len = fp_display_fn(ch, d, buff, flags,
                                          &prefix, &precision,
                                          &before_dot, &after_dot);
                  }
                  else
                  {
                      len = 0;
                  }
                  break;

#else
        /* floating-point values always come out as 0.0 */
        default: buff[0] = '0';
                 buff[1] = '.';
                 buff[2] = '0';
                 len = 3;
                 break;
#endif
    }

    /* Now work out how many leading '0's are needed for precision specifier.
     * _FPCONV is the case of FP printing in which case extra digits to make
     * up the precision come within the number as marked by characters '<'
     * and '>' in the buffer.
     */

    if (flags & _FPCONV)
    {
        precision = 0;

        if (before_dot > 0)
            precision = before_dot-1;

        if (after_dot > 0)
            precision += after_dot-1;
    }
    else if ((precision -= len) < 0)
    {
        precision = 0;
    }

    /* figure out how much padding is needed */
    width -= (precision + len + (int)strlen(prefix));

    /* ANSI appears to suggest that the padding (even if with '0')
     * occurs before the possible sign! Treat this as fatuous.
     */
    if (!(flags & _PADZERO))
        pre_padding(p);

    /* handle prefix */
    {
    int c;

        while ((c = *prefix++) != 0)
        {
            (*putc)(c, p);
            charcount++;
        }
    }

    pre_padding(p);

    /* floating point numbers are in buff[] the normal way around, while
     * integers have been pushed in with the digits in reverse order.
     */
    if (flags & _FPCONV)
    {
    int i, c;

        for (i = 0; i<len; i++)
        {
            switch (c = buff[i])
            {
                case '<': pr_padding('0', before_dot, p);
                          break;

                case '>': pr_padding('0', after_dot, p);
                          break;

                default : (*putc)(c, p);
                          charcount++;
                          break;
            }
        }
    }
    else
    {
        pr_padding('0', precision, p);
        charcount += len;

        while (len-- > 0)
            putc(buff[len], p);
    }

    /* If the padding has already been printed width will be zero */
    post_padding(p);

    return charcount;
}


/*****************************************************************************/


int __vfprintf(FILE *p, const char *fmt, va_list args, fp_print fp_display_fn,
               int32 (*putc)(char, FILE *))
{
int          ch;
int          charcount = 0;
va_list      argArray = args;
unsigned int argSlot;

    while ((ch = *fmt++) != 0)
    {
        if (ch != '%')
        {
            (*putc)(ch,p);
            charcount++;
        }
        else
        {
        int           flags = 0;
        int           width;
        int           precision = 0;
        char         *prefix;
        char         *hextab = 0;
        const char   *start;
        unsigned int  v;
        double        d;

            /* The initialization of hextab above is spurious in that it will be
             * set to a real string before use, but necessary in that passing
             * unset parameters to functions is illegal in C.
             */

	    v = 0;

            /* first off, let's see if we have an arg slot # */

            start   = fmt;
            ch      = *fmt++;
            argSlot = 0;
            while (isnumber(ch))
            {
                argSlot = argSlot * 10 + intofdigit(ch);
                ch = *fmt++;
            }

            if (ch != '$')
            {
                /* we didn't get an arg slot, so pretend the whole ugly
                 * incident didn't happen...
                 */

                argSlot = 0;
                fmt     = start;
                ch      = fmt[-1];
            }

            /* This decodes all the nasty flags and options associated with an
             * entry in the format list. For some entries many of these options
             * will be useless, but they are parsed all the same.
             */

            while (TRUE)
            {
                switch (ch = *fmt++)
                {
                    /* '-' Left justify converted flag. Only relevant if width is
                     * specified explicitly and converted value is too short to
                     * fill it.
                     */
                    case '-': flags = _LJUSTIFY | (flags & ~_PADZERO);
                              continue;

                    /* '+' Always print either '+' or '-' at start of #s */
                    case '+': flags |= _SIGNED;
                              continue;

                    /* ' ' Print either ' ' or '-' at start of #s */
                    case ' ': flags |= _BLANKER;
                              continue;

                    /* '#' Variant on main print routine (effect varies across
                     *     different styles, but for instance %#x puts 0x on
                     *     the front of displayed numbers.
                     */
                    case '#': flags |= _VARIANT;
                              continue;

                    /* '0' Leading blanks are printed as zeros
                     *     NOTE: This is a *** DEPRECATED FEATURE ***
                     *           (precision subsumes)
                     */
                    case '0': flags |= _PADZERO;
                              continue;
                }
                break;
            }

            /* now look for 'width' */
            {
            int t = 0;

                if (ch == '*')
                {
                    t = va_arg(args, int);

                    /* If a negative width is passed as an argument, its
                     * absolute value is taken and the negativeness is used to
                     * indicate the presence of the '-' flag (left
                     * justification). If '-' was already specified, the flag
                     * is turned off.
                     */
                    if (t < 0)
                    {
                        t = - t;
                        flags ^= _LJUSTIFY;
                    }
                    ch = *fmt++;
                }
                else
                {
                    while (isnumber(ch))
                    {
                        t  = t*10 + intofdigit(ch);
                        ch = *fmt++;
                    }
                }
                width = (t >= 0 ? t : 0); /* disallow -ve arg */
            }

            if (ch == '.') /* precision spec */
            {
            int t = 0;

                ch = *fmt++;

                if (ch == '*')
                {
                    t  = va_arg(args, int);
                    ch = *fmt++;
                }
                else
                {
                    while (isnumber(ch))
                    {
                        t = t*10 + intofdigit(ch);
                        ch = *fmt++;
                    }
                }

                if (t >= 0)
                {
                    flags |= _PRECGIVEN;
                    precision = t;
                }
            }

            /* 'l' Indicate that a numeric argument is 'long'. Here int and
             *     long are the same (32 bits) and so this flag can be
             *     ignored.
             *
             * 'L' Marks floating arguments as being of type long double. Here
             *     this is the same as just double, and so this flag can be
             *     ignored.
             */

            if ((ch == 'l') || (ch == 'L'))
            {
                flags |= _LONGSPECIFIER;
                ch = *fmt++;
            }
            else if (ch == 'h')
            {
                /* 'h' Indicates that an integer value is to be treated as short */
                flags |= _SHORTSPEC;
                ch = *fmt++;
            }

            /* Now the options have been decoded, process the main dispatch */
            switch (ch)
            {
                /* If a '%' occurs at the end of a format string (possibly with
                 * a few width specifiers and qualifiers after it) the code
                 * ends up here with a '\0'. in my hand. Unless something
                 * special is done, the fact that the format string terminated
                 * gets lost...
                 */
                case 0: fmt--;
                        continue;

                /* %n assigns the number of chars printed so far to the next
                 *    arg (which is expected to be of type (int *).
                 */
                case 'n':{
                         int *xp = get_arg(args, int *);

                             *xp = charcount;
                         }
                         continue;

                /* %s prints a string. If a precision is given it can limit
                 *    the number of characters taken from the string, and
                 *    padding and justification behave as usual.
                 */
                case 's': {
                          char *str = get_arg(args, char *);
                          int i, n;

                              if (flags & _PRECGIVEN)
                              {
                                  n = 0;
                                  while ((n < precision) && (str[n] != 0))
                                      n++;
                              }
                              else
                              {
                                  n = (int)strlen(str);
                              }
                              width -= n;
                              pre_padding(p);

                              for (i=0; i<n; i++)
                                  (*putc)(str[i], p);
                              charcount += n;

                              post_padding(p);
                          }
                          continue;

                /* %x prints in hexadecimal. %X does the same, but uses upper
                 *    case when printing things that are not (decimal) digits.
                 *    Decoding logic is shared here with the code that deals
                 *    with octal and decimal output via %o and %d.
                 */
                case 'X': v = get_arg(args, int);
                          if (flags & _SHORTSPEC)
                              v = (unsigned short)v;

                          hextab = "0123456789ABCDEF";
                          prefix = (flags & _VARIANT) ? "0X" : "";
                          if (flags & _PRECGIVEN)
                              flags &= ~_PADZERO;
                          break;

                case 'x': v = get_arg(args, int);
                          if (flags & _SHORTSPEC)
                              v = (unsigned short)v;

                          hextab = "0123456789abcdef";
                          prefix = (flags & _VARIANT) ? "0x" : "";
                          if (flags & _PRECGIVEN)
                              flags &= ~_PADZERO;
                          break;

                /* %p is for printing a pointer. These are printed as hex
                 *    numbers with the precision always forced to 8.
                 */
                case 'p': v = (unsigned int)get_arg(args, void *);
                          hextab     = "0123456789abcdef";
                          prefix     = (flags & _VARIANT) ? "@" : "";
                          flags     |= _PRECGIVEN;
                          precision  = 8;
                          break;

                case 'o': v = get_arg(args, int);
                          if (flags & _SHORTSPEC)
                              v = (unsigned short)v;

                          prefix = (flags & _VARIANT) ? "0" : "";
                          if (flags & _PRECGIVEN)
                              flags &= ~_PADZERO;
                          break;

                case 'u': v = get_arg(args, unsigned int);
                          if (flags & _SHORTSPEC)
                              v = (unsigned short)v;

                          prefix = "";
                          if (flags & _PRECGIVEN)
                              flags &= ~_PADZERO;
                          break;

                case 'i':
                case 'd': {
                          int w;

                              w = get_arg(args, int);
                              if (flags & _SHORTSPEC)
                                  w = (signed short)w;

                              if (w < 0)
                              {
                                  v = 0U-w;
                                  prefix = "-";
                              }
                              else
                              {
                                  v = w;
                                  prefix = (flags&_SIGNED) ? "+" : (flags & _BLANKER) ? " " : "";
                              }
                          }

                          if (flags & _PRECGIVEN)
                              flags &= ~_PADZERO;

                          break;

                case 'f':
                case 'e':
                case 'E':
                case 'g':
                case 'G': flags |= _FPCONV;
                          if (!(flags & _PRECGIVEN))
                              precision = 6;

#ifdef FLOATING_POINT
                          d      = va_arg(args, double);
                          prefix = 0;
                          hextab = 0;
                          v      = 0;
#else
                          /* floats are two ints in size... */
			  (void)get_arg(args, int);
			  (void)get_arg(args, int);

                          prefix = (flags & _SIGNED) ? "+" : (flags & _BLANKER) ? " " : "";
#endif
                          break;

                /* %c causes a single character to be fetched from the
                 *    argument list and printed. This is subject to padding.
                 */
                case 'c': ch = get_arg(args, int);
                          /* drop through */

                /* %? where ? is some character not properly defined as a
                 *    command char for printf() causes ? to be displayed with
                 *    padding and field widths as specified by the various
                 *    modifiers. %% is handled by this general mechanism.
                 */
                default : width--;                        /* char width is 1       */
                          pre_padding(p);
                          (*putc)(ch, p);
                          charcount++;
                          post_padding(p);
                          continue;
            }

            charcount += printf_display(p, flags, ch, precision, width, v,
                                        fp_display_fn, prefix, hextab, &d, putc);
            continue;
        }
    }

    return charcount;
}


/*****************************************************************************/


#ifdef FLOATING_POINT


/*****************************************************************************/


#include <math.h>

#ifdef IEEE
#define FLOATING_WIDTH 17
#else
#define FLOATING_WIDTH 18       /* upper bound for sensible precision    */
#endif


/*****************************************************************************/


static int fp_round(char buff[], int len)
/* Round (char form of) FP number - return 1 if carry, 0 otherwise
 * Note that 'len' should be <= 20 - see fp_digits()
 * The caller ensures that buff[0] is always '0' so that carry is simple
 * However, beware that this routine does not re-ensure this if carry!!
 */
{
int ch;
char *p = &buff[len];

    if ((ch = *p)==0)
        return 0;                      /* at end of string */

    if (ch < '5')
        return 0;                      /* round downwards  */

    if (ch == '5')                                   /* the dodgy case   */
    {
    char *p1;

        for (p1 = p; (ch = *++p1)=='0';);
        if (ch == 0)
            return 0;                         /* .5 ulp exactly   */
    }

    for (;;)
    {
        ch = *--p;
        if (ch=='9')
        {
            *p = '0';
        }
        else
        {
            *p = ch + 1;
            break;
        }
    }

    if (buff[0]!='0')           /* caused by rounding                    */
    {
    int w;                  /* renormalize the number                */

        for (w=len; w>=0; w--)
            buff[w+1] = buff[w];

        return 1;
    }

    return 0;
}


/*****************************************************************************/


#ifdef HOST_HAS_BCD_FLT

static int fp_digits(char *buff, double d)
/* This routine turns a 'double' into a character string representation of
 * its mantissa and returns the exponent after converting to base 10.
 * It guarantees that buff[0] = '0' to ease problems connected with
 * rounding and the like.  See also comment at first call.
 * Use FPE2 convert-to-packed-decimal feature to do most of the work
 * The sign of d is returned in the LSB of x, and x has to be halved to
 * obtain the 'proper' value it needs.
 */
{
unsigned int a[3], w, d0, d1, d2, d3;
int x, i;

    _stfp(d, a);

    w = a[0];

    /* Four digit exponents are allowed even though sensible values can
     * only extend to 3 digits, just being cautious...
     */
    if ((w & 0x0ffff000) == 0x0ffff000)
    {
        x = 999;    /* Infinity will print as 1.0e999 here */
                    /* as will NaNs                        */
        for (i = 0; i<20; i++)
            buff[i] = '0';

        buff[1] = '1';
    }
    else
    {
        d0 = (w>>24) & 0xf;
        d1 = (w>>20) & 0xf;
        d2 = (w>>16) & 0xf;
        d3 = (w>>12) & 0xf;
        x = ((d0*10 + d1)*10 + d2)*10 + d3;

        if (w & 0x40000000)
            x = -x;

        buff[0] = '0';
        for (i = 1; i<4; i++)
            buff[i] = '0' + ((w>>(12-4*i)) & 0xf);

        w = a[1];
        for (i = 4; i<12; i++)
            buff[i] = '0' + ((w>>(44-4*i)) & 0xf);

        w = a[2];
        for (i = 12; i<20; i++)
            buff[i] = '0' + ((w>>(76-4*i)) & 0xf);
    }
    buff[20] = 0;

    x = x<<1;
    if (a[0] & 0x80000000)
        x |= 1;

    return x;
}

#endif /* HOST_HAS_BCD_FLT */


/*****************************************************************************/


#ifndef HOST_HAS_BCD_FLT

static void pr_dec(int d, char *p, int n)
/* print d in decimal, field width n and store result at p. arg small & +ve.*/
{
    while ((n--)>0)
    {
    int dDiv10;

        dDiv10 = d / 10;
        *p--   = '0' + d - dDiv10 * 10;
        d      = dDiv10;
    }
}

#endif /* !HOST_HAS_BCD_FLT */


/*****************************************************************************/


#ifndef HOST_HAS_BCD_FLT

static int fp_digits(char *buff, double d)
/* This routine turns a 'double' into a character string representation of
 * its mantissa and returns the exponent after converting to base 10.
 * For this we use one-and-a-half precision done by steam.
 * It guarantees that buff[0] = '0' to ease problems connected with
 * rounding and the like.  See also comment at first call.
 */
{
int hi, mid, lo, dx, sign = 0;

    if (d < 0.0)
    {
        d = -d;
        sign = 1;
    }

    if (d == 0.0)
    {
        hi = mid = lo = 0;
        dx = -5;
    }
    else
    {
    double d1, d2, d2low, d3, d3low, scale;
    int w, bx;

        d1 = frexp(d, &bx);     /* exponent & mantissa   */
        /* fraction d1 is in range 0.5 to 1.0            */
        /* remember log_10(2) = 0.3010!                  */
        dx = (301*bx - 5500)/1000;   /* decimal exponent */
        scale = ldexp(1.0, dx-bx);
        w = dx;
        if (w < 0)
        {
            w = -w;
            d3 = 0.2;
        }
        else
        {
            d3 = 5.0;
        }

        if (w != 0)
        {
            for (;;)      /* scale *= 5**dx        */
            {
                if ((w&1) != 0)
                {
                    scale *= d3;
                    if (w == 1)
                        break;
                }
                d3 *= d3;
                w = w >> 1;
            }
        }
        d2 = d1 / scale;

        /* The initial value selected for dx was computed on the basis of the
         * binary exponent in the argument value - now dx is refined. If the
         * value produced to start with was accurate enough hardly anything
         * will have to be done here...
         */
        while (d2 < 100000.0)
        {
            d2 *= 10.0;
            dx -= 1;
            scale /= 10.0;
        }

        while (d2 >= 1000000.0)
        {
            d2 /= 10.0;
            dx += 1;
            scale *= 10.0;
        }

        hi = (int) d2;
        for (;;)               /* loop to get hi correct                 */
        {
            d2 = ldexp((double) hi, dx-bx);
            /* at worst 24 bits in d2 here                               */
            /* even with IBM fp numbers there is no accuracy lost        */
            d2low = 0.0;
            w = dx;

            if (w < 0)
            {
                w = -w;
                /* the code here needs to set (d3, d3low) to a one-and-a-half precision  */
                /* version of the constant 0.2.                                          */
                d3 = 0.2;
                d3low = 0.0;
                _fp_normalize(d3, d3low);
                d3low = (1.0 - 5.0*d3)/5.0;
            }
            else
            {
                d3 = 5.0;
                d3low = 0.0;
            }

            /* Now I want to compute d2 = d2 * d3**dx in extra precision arithmetic  */
            if (w != 0)
            {
                for (;;)
                {
                    if ((w & 1) != 0)
                    {
                        d2low = d2*d3low + d2low*(d3 + d3low);
                        d2 *= d3;
                        _fp_normalize(d2, d2low);
                        if (w == 1)
                            break;
                    }
                    d3low *= (2.0*d3 + d3low);
                    d3 *= d3;
                    _fp_normalize(d3, d3low);
                    w = w>>1;
                }
            }

            if (d2 <= d1)
                break;

            hi -= 1;          /* hardly ever happens */
        }

        d1 -= d2;
        /* for this to be accurate d2 MUST be less */
        /* than d1 so that d1 does not get shifted */
        /* prior to the subtraction.               */
        d1 -= d2low;
        d1 /= scale;

        /* Now d1 is a respectably accurate approximation for (d - (double)hi)   */
        /* scaled by 10**dx                                                      */

        d1 *= 1000000.0;
        mid = (int) d1;
        d1 = 1000000.0 * (d1 - (double) mid);
        lo = (int) d1;

        /* Now some postnormalization on the integer results                     */
        /* If I do things this way the code will work if (int) d rounds or       */
        /* truncates.                                                            */

        while (lo<0) { lo += 1000000; mid -= 1; }
        while (lo>=1000000) { lo -= 1000000; mid += 1; }
        while (mid<0) { mid += 1000000; hi -= 1; }
        while (mid>=1000000) { mid -= 1000000; hi += 1; }

        if (hi<100000)
        {
        int loDiv100000;
        int midDiv100000 = mid/100000;

            hi = 10*hi + midDiv100000;
            loDiv100000 = lo/100000;
            mid = 10*(mid - midDiv100000 * 100000) + loDiv100000;
            lo = 10*(lo - loDiv100000 * 100000);
            dx -= 1;
        }
        else if (hi >= 1000000)
        {
        int midDiv10;
        int hiDiv10 = hi/10;

            mid += 1000000*(hi - hiDiv10 * 10);
            hi = hiDiv10;
            midDiv10 = mid/10;
            lo += 1000000*(mid - midDiv10 * 10);
            mid = midDiv10;
            lo = (lo + 5)/10;    /* pretence at rounding */
            dx += 1;
        }
    }

    /* Now my result is in three 6-digit chunks (hi, mid, lo)                */
    /* The number of characters put in the buffer here MUST agree with       */
    /* FLOATING_PRECISION. This version is for FLOATING_PRECISION = 18.      */
    buff[0] = '0';
    pr_dec(hi,  &buff[6], 6);
    pr_dec(mid, &buff[12], 6);
    pr_dec(lo,  &buff[18], 6);
    buff[19] = '0';
    buff[20] = 0;

    return ((dx+5)<<1) | sign;
}

#endif /* !HOST_HAS_BCD_FLT */


/*****************************************************************************/


static int fp_addexp(char *buff, int len, int dx, int ch)
{
int dxDiv10;

    buff[len++] = ch;
    if (dx < 0)
    {
        dx = -dx;
        buff[len++] = '-';
    }
    else
    {
        buff[len++] = '+';
    }

    if (dx >= 1000)
    {
    int dxDiv1000;

        dxDiv1000   = dx / 1000;
        buff[len++] = '0' + dxDiv1000;
        dx          = dx - dxDiv1000 * 1000;
    }

    if (dx >= 100)
    {
    int dxDiv100;

        dxDiv100    = dx / 100;
        buff[len++] = '0' + dxDiv100;
        dx          = dx - dxDiv100 * 100;
    }
    dxDiv10 = dx / 10;

    buff[len++] = '0' + dxDiv10;
    buff[len++] = '0' + dx - dxDiv10 * 10;

    return len;
}


/*****************************************************************************/


#define fp_insert_(buff, pos, c)                    \
    {   int w;                                      \
        for (w=0; w<=pos; w++) buff[w] = buff[w+1]; \
        buff[pos+1] = c; }

static int fpdisplay(int ch, double *lvd, char buff[], int flags,
                     char **lvprefix, int *lvprecision, int *lvbefore_dot,
                     int *lvafter_dot)
{
int    len = 0;
double d   = *lvd;

    switch (ch)
    {
        /* The following code places characters in buff[]
         * to print the floating-point number given as d.
         * It is given flags that indicate what format is required and how
         * many digits precision are needed.
         *
         * Floating-point values are ALWAYS converted into 18 decimal digits
         * (the largest number possible reasonable) to start with, and rounding
         * is then performed on this character representation. This is intended
         * to avoid all possibility of boundary effects when numbers like .9999
         * are being displayed.
         */
        case 'f': {
                  int dx;

                      dx = fp_digits(buff, d);
                      if (dx & 1)
                          *lvprefix = "-";
                      else
                          *lvprefix = (flags&_SIGNED) ? "+" : (flags&_BLANKER) ? " " : "";

                      dx = (dx & ~1) / 2;
                      if (dx < 0)
                      {
                          /* insert leading zeros */
                          dx = -dx;
                          if (dx > *lvprecision+1)
                          {
                              len = 0;       /* prints as zero */
                              buff[len++] = '0';
                              buff[len++] = '.';
                              *lvafter_dot = *lvprecision;
                          }
                          else
                          {
                              len = *lvprecision - dx + 2;
                              if (len > FLOATING_WIDTH + 1)
                              {
                                  *lvafter_dot = len - (FLOATING_WIDTH + 2);
                                  len = FLOATING_WIDTH+2;
                              }

                              if (fp_round(buff, len))
                                  dx--, len++; /* dx-- because of negation */

                              /* unfortunately we may have dx=0 now because of
                               * the rounding
                               */
                              if (dx == 0)
                              {
                                  buff[0] = buff[1];
                                  buff[1] = '.';
                              }
                              else if (dx == 1)
                              {
                              int w;

                                  for (w=len; w>0; w--)
                                      buff[w+1] = buff[w];

                                  len += 1;
                                  buff[0] = '0';
                                  buff[1] = '.';
                              }
                              else
                              {
                              int w;

                                  for (w=len; w>0; w--)
                                      buff[w+2] = buff[w];

                                  len += 2;
                                  buff[0] = '0';
                                  buff[1] = '.';
                                  buff[2] = '<';
                                  *lvbefore_dot = dx - 1;
                              }
                          }

                          if (*lvafter_dot > 0)
                              buff[len++] = '>';
                      }
                      else /* dx >= 0 */
                      {
                          len = dx + *lvprecision + 2;
                          if (len > FLOATING_WIDTH+1)
                          {
                              len = FLOATING_WIDTH+2;

                              /* Seemingly endless fun here making sure that
                               * the number is printed without truncation or
                               * loss even if it is very big & hence needs very
                               * many digits. Only the first few digits will be
                               * significant, of course but the C specification
                               * forces printing lots of insignificant ones
                               * too. Use flag characters '<' and '>' plus
                               * variables (before_dot) and (after_dot) to keep
                               * track of what has happened.
                               */
                              if (fp_round(buff, len))
                              {
                                  dx++;
                                  len++;         /* number extended  */
                              }

                              if (dx < len-1)
                              {
                                  fp_insert_(buff, dx, '.');
                                  *lvafter_dot = dx + *lvprecision - FLOATING_WIDTH;

                                  if (*lvafter_dot!=0)
                                      buff[len++] = '>';
                              }
                              else
                              {
                              int w;

                                  for (w=0; w<len-1; w++)
                                      buff[w] = buff[w+1];

                                  buff[len-1] = '<';
                                  *lvbefore_dot = dx - len + 2;
                                  buff[len++] = '.';
                                  if (*lvprecision != 0)
                                  {
                                      *lvafter_dot = *lvprecision;
                                      buff[len++] = '>';
                                  }
                              }
                          }
                          else
                          {
                              if (fp_round(buff, len))
                              {
                                  dx++;
                                  len++;     /* number extended  */
                              }

                              fp_insert_(buff, dx, '.');
                          }
                      }

                      if ((*lvprecision==0) && ((flags&_VARIANT)==0))
                          len--;
                  }
                  break;

#if 0
	case 'g':
	case 'G':
#endif
        default : {
                  int dx;

                      dx = fp_digits(buff, d);
                      if (dx & 1)
                          *lvprefix = "-";
                      else
                          *lvprefix = (flags&_SIGNED) ? "+" :
                                 (flags&_BLANKER) ? " " : "";
                      dx = (dx & ~1) / 2;

                      if (*lvprecision < 1)
                          *lvprecision = 1;

                      len = (*lvprecision>FLOATING_WIDTH) ? FLOATING_WIDTH+1 : *lvprecision + 1;
                      dx += fp_round(buff, len);

                      /* Now choose either 'e' or 'f' format, depending on
                       * which will lead to the more compact display of the
                       * number.
                       */
                      if ((dx >= *lvprecision) || (dx < -4))
                      {
                          buff[0] = buff[1];          /* e or E format */
                          buff[1] = '.';
                      }
                      else
                      {
                          ch = 'f';                   /* uses f format */
                          if (dx >= 0)
                          {
                              /* Insert a decimal point at the correct place
                               * for 'f' format printing
                               */
                              fp_insert_(buff, dx, '.')
                          }
                          else
                          {
                          int w;

                              /* If the exponent is negative the required
                               * format will be something like 0.xxxx, 0.0xxx
                               * or 0.00xx and the buffer needs to be lengthened
                               */

                              dx = -dx;
                              for (w=len; w>=0; w--)
                                  buff[w+dx] = buff[w];

                              len += dx;
                              for(w=0; w<=dx; w++)
                                  buff[w] = '0';

                              buff[1] = '.';
                          }
                      }

                      if ((flags & _VARIANT) == 0)         /* trailing 0?   */
                      {
                          *lvafter_dot = -1;
                          if (buff[len]!='.')
                              while (buff[len-1]=='0')
                                  len--;

                          if (buff[len-1]=='.')
                              len--;
                      }
                      else
                      {
                          /* Allow for the fact that the specified precision
                           * may be very large in which case trailing zeros
                           * are added via the marker character '>' and a
                           * count (after_dot). Not applicable unless the '#'
                           * flag has been given since without '#' trailing
                           * zeros in the fraction are killed.
                           */
                          if (*lvprecision>FLOATING_WIDTH)
                          {
                              *lvafter_dot = *lvprecision - FLOATING_WIDTH;
                              buff[len++] = '>';
                          }
                      }

                      if (ch != 'f')    /* sets 'f' if it prints in f format and 'e' or 'E' if in e format */
                          len = fp_addexp(buff, len, dx, ch + ('e'-'g'));
                  }
                  break;

        case 'e':
        case 'E': {
                  int dx;

                      dx = fp_digits(buff, d);
                      if (dx & 1)
                          *lvprefix = "-";
                      else
                          *lvprefix = (flags & _SIGNED) ? "+" : (flags & _BLANKER) ? " " : "";

                      dx = (dx & ~1) / 2;
                      if (*lvprecision > FLOATING_WIDTH)
                      {
                          *lvafter_dot = *lvprecision - FLOATING_WIDTH;
                          *lvprecision = FLOATING_WIDTH;
                      }

                      len = *lvprecision + 2;
                      dx += fp_round(buff, len);
                      buff[0] = buff[1];
                      if ((*lvprecision == 0) && !(flags & _VARIANT))
                          len = 1;
                      else
                          buff[1] = '.';

                      /* Deal with trailing zeros for excessive precision requests */
                      if (*lvafter_dot > 0)
                          buff[len++] = '>';

                      len = fp_addexp(buff, len, dx, ch);
                  }
                  break;
    }

    return len;
}


/*****************************************************************************/


#else /* FLOATING_POINT */


/*****************************************************************************/


#define fpdisplay NULL


/*****************************************************************************/


#endif /* FLOATING_POINT */


/*****************************************************************************/


int32 printf(const char *fmt, ...)
{
va_list a;
int     n;

    va_start(a, fmt);
    n = __vfprintf(NULL, fmt, a, fpdisplay, putc);
    va_end(a);

    return n;
}
