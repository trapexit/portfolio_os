/* $Id: ja.h,v 1.4 1994/11/29 17:42:37 vertex Exp $ */

#ifndef __JA_H
#define __JA_H


/*****************************************************************************/


/* Definitions in support of Japanese */


/*****************************************************************************/


#define HIWORD(n)           ((n)>>16)
#define LOWORD(n)           ((n)&0x0000FFFF)

#define MAX_JA_CODE         7100    /* MAX table count */
#define ASCII_BASE          0x20
#define LATIN1_BASE         0xA0

/* for ASCII chars */
#define isASCIILigatures(c) ((c&0xFF) == 0xAB) || ((c&0xFF) == 0xBB) || ((c&0xFF) == 0xBC) || \
							((c&0xFF) == 0xBD) || ((c&0xFF) == 0xBE) || ((c&0xFF) == 0xC6) || \
							((c&0xFF) == 0xE6))

/* for SJIS chars */
#define isKanji(c)      ((0x81 <= (c&0xFF) && (c&0xFF) <= 0x9F) || \
						 (0xE0 <= (c&0xFF) && (c&0xFF) <= 0xFC))
#define isKanji2(c)     ((0x40 <= (c&0xFF) && (c&0xFF) <= 0xFC) && ((c&0xFF) != 0x7F))
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

#define isKana2WithVoicedSound(c)   (((c) == 0x834B) || ((c) == 0x834D) || ((c) == 0x834F) \
                                     ((c) == 0x8351) || ((c) == 0x8353) || ((c) == 0x8355) \
                                     ((c) == 0x8357) || ((c) == 0x8359) || ((c) == 0x835B) \
                                     ((c) == 0x835D) || ((c) == 0x835F) || ((c) == 0x8361) \
                                     ((c) == 0x8364) || ((c) == 0x8366) || ((c) == 0x8368) \
                                     ((c) == 0x836F) || ((c) == 0x8372) || ((c) == 0x8375) \
                                     ((c) == 0x8378) || ((c) == 0x837B) || ((c) == 0x8394))

#define isKana2WithSemiVoicedSound(c)   (((c) == 0x8370) || ((c) == 0x8373) || ((c) == 0x8376) \
                                         ((c) == 0x8379) || ((c) == 0x837C))

#define isHiraganaWithVoicedSound(c)    (((c) == 0x82AA) || ((c) == 0x82AC) || ((c) == 0x82AE) \
                                         ((c) == 0x82B0) || ((c) == 0x82B2) || ((c) == 0x82B4) \
                                         ((c) == 0x82B6) || ((c) == 0x82B8) || ((c) == 0x82BA) \
                                         ((c) == 0x82BC) || ((c) == 0x82BE) || ((c) == 0x83C0) \
                                         ((c) == 0x82C3) || ((c) == 0x82C5) || ((c) == 0x82C7) \
                                         ((c) == 0x82CE) || ((c) == 0x82D1) || ((c) == 0x82D4) \
                                         ((c) == 0x82D7) || ((c) == 0x82DA))

#define isHiraganaWithSemiVoicedSound(c)    (((c) == 0x82CF) || ((c) == 0x82D2) || ((c) == 0x82D5) \
                                             ((c) == 0x82D8) || ((c) == 0x83DB))

#define isSymbols(c)    (((c) >= 0x8140) && ((c) <= 0x8197))

#define isKanjiL2(c)    ((0x989f <= (c)) && ((c) <= 0xeafc))


/* for FullWidth UniChar */
#define isUniNumber2(c) (((c) >= 0xFF10) && ((c) <= 0xFF19))
#define isUniAlpha2(c)  (((c) >= 0xFF21) && ((c) <= 0xFF3A) || ((c) >= 0xFF41) && ((c) <= 0xFF5A))
#define isUniUpper2(c)  ((c) >= 0xFF21) && ((c) <= 0xFF3A)
#define isUniLower2(c)  ((c) >= 0xFF41) && ((c) <= 0xFF5A)
#define isUniAlnum2(c)  (isUniNumber2(c) || isUniAlpha2(c) || (c) == 0x3000)
#define isUniKana(c)    (0xFF65 <= (c) && (c) <= 0xFF9F)
#define isUniKana2(c)   ((0x30A1 <= (c)) && ((c) <= 0x30F6))
#define isUniHiragana(c)    ((0x3041 <= (c)) && ((c) <= 0x3093))
#define isUniKanji(c)   (((0x4E00 <= (c)) && ((c) <= 0x9FA5)) || ((0xF900 <= (c)) && ((c) <= 0xFA2D)))

#define isUniKana2WithVoicedSound(c)    (((c) == 0x30AC) || ((c) == 0x30AE) || ((c) == 0x30B0) \
                                         ((c) == 0x30B2) || ((c) == 0x30B4) || ((c) == 0x30B6) \
                                         ((c) == 0x30B8) || ((c) == 0x30BA) || ((c) == 0x30BC) \
                                         ((c) == 0x30BE) || ((c) == 0x30C0) || ((c) == 0x30C2) \
                                         ((c) == 0x30C5) || ((c) == 0x30C7) || ((c) == 0x30C9) \
                                         ((c) == 0x30D0) || ((c) == 0x30D3) || ((c) == 0x30D6) \
                                         ((c) == 0x30D9) || ((c) == 0x30DC) || ((c) == 0x30F4))

#define isUniKana2WithSemiVoicedSound(c)    (((c) == 0x30D1) || ((c) == 0x30D4) || ((c) == 0x30D7) \
                                             ((c) == 0x30DA) || ((c) == 0x30DD))

#define isUniHiraganaWithVoicedSound(c) (((c) == 0x304C) || ((c) == 0x304E) || ((c) == 0x3050) \
                                         ((c) == 0x3052) || ((c) == 0x3054) || ((c) == 0x3056) \
                                         ((c) == 0x3058) || ((c) == 0x305A) || ((c) == 0x305C) \
                                         ((c) == 0x305E) || ((c) == 0x3060) || ((c) == 0x3062) \
                                         ((c) == 0x3065) || ((c) == 0x3067) || ((c) == 0x3069) \
                                         ((c) == 0x3070) || ((c) == 0x3073) || ((c) == 0x3076) \
                                         ((c) == 0x3079) || ((c) == 0x307C))

#define isUniHiraganaWithSemiVoicedSound(c) (((c) == 0x3071) || ((c) == 0x3074) || ((c) == 0x3077) \
                                             ((c) == 0x307A) || ((c) == 0x307D))


#define UNI_HALF_KANA_VOICEDSOUND_MARK              0xFF9E
#define UNI_HALF_KANA_SEMI_VOICEDSOUND_MARK         0xFF9F

/* for ASCII chars */
#define isASCII(c)      (0x20 <= (c) && (c) <= 0x7E)
#define isLatin1(c)     (0xA0 <= (c&0xFF) && (c&0xFF) <= 0xFF)


/* for Exchange */
typedef struct ExchangeTable
{
    unichar ch1;
    unichar ch2;
} ExchangeTable;


/*****************************************************************************/


#endif /* __JA_H */
