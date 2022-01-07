/* $Id: bsafe2.h,v 1.2 1994/02/09 02:04:35 limes Exp $ */
/* Copyright (C) RSA Data Security, Inc. created 1990.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#ifndef _BSAFE_H_
#define _BSAFE_H_ 1

#ifndef T_CALL
#define T_CALL
#endif

#include "atypes.h"

/* Include these definitions until RSA Labs gives an atypes.h which does.
 */
typedef struct {
  unsigned int primeBits;
  unsigned int exponentBits;
} A_DH_PARAM_GEN_PARAMS;

typedef struct {
  unsigned char *key;                                     /* 8 byte DES key */
  unsigned char *inputWhitener;                    /* 8 byte input whitener */
  unsigned char *outputWhitener;                  /* 8 byte output whitener */
} A_DESX_KEY;

#define BE_ALGORITHM_ALREADY_SET 0x0200
#define BE_ALGORITHM_INFO 0x0201
#define BE_ALGORITHM_NOT_INITIALIZED 0x0202
#define BE_ALGORITHM_NOT_SET 0x0203
#define BE_ALGORITHM_OBJ 0x0204
#define BE_ALG_OPERATION_UNKNOWN 0x0205
#define BE_ALLOC 0x0206
#define BE_CANCEL 0x0207
#define BE_DATA 0x0208
#define BE_EXPONENT_EVEN 0x0209
#define BE_EXPONENT_LEN 0x020a
#define BE_HARDWARE 0x020b
#define BE_INPUT_DATA 0x020c
#define BE_INPUT_LEN 0x020d
#define BE_KEY_ALREADY_SET 0x020e
#define BE_KEY_INFO 0x020f
#define BE_KEY_LEN 0x0210
#define BE_KEY_NOT_SET 0x0211
#define BE_KEY_OBJ 0x0212
#define BE_KEY_OPERATION_UNKNOWN 0x0213
#define BE_MEMORY_OBJ 0x0214
#define BE_MODULUS_LEN 0x0215
#define BE_NOT_INITIALIZED 0x0216
#define BE_NOT_SUPPORTED 0x0217
#define BE_OUTPUT_LEN 0x0218
#define BE_OVER_32K 0x0219
#define BE_RANDOM_NOT_INITIALIZED 0x021a
#define BE_RANDOM_OBJ 0x021b
#define BE_SIGNATURE 0x021c
#define BE_WRONG_ALGORITHM_INFO 0x021d
#define BE_WRONG_KEY_INFO 0x021e

typedef POINTER B_KEY_OBJ;
typedef POINTER B_ALGORITHM_OBJ;

typedef int (T_CALL *B_INFO_TYPE) PROTO_LIST ((POINTER, int, POINTER));

typedef struct {
  B_INFO_TYPE algorithmInfoType;
  int encryptFlag;
  B_INFO_TYPE keyInfoType;
  POINTER alga;
} B_ALGORITHM_METHOD;
typedef B_ALGORITHM_METHOD **B_ALGORITHM_CHOOSER;

/* Version information.
 */
extern char *BSAFE_VERSION;

/* Routines supplied by the implementor.
 */
void T_CALL T_memset PROTO_LIST ((POINTER, int, unsigned int));
void T_CALL T_memcpy PROTO_LIST ((POINTER, POINTER, unsigned int));
void T_CALL T_memmove PROTO_LIST ((POINTER, POINTER, unsigned int));
int T_CALL T_memcmp PROTO_LIST ((POINTER, POINTER, unsigned int));
POINTER T_CALL T_malloc PROTO_LIST ((unsigned int));
POINTER T_CALL T_realloc PROTO_LIST ((POINTER, unsigned int));
void T_CALL T_free PROTO_LIST ((POINTER));

/* The key object.
 */
int T_CALL B_CreateKeyObject PROTO_LIST ((B_KEY_OBJ *));
void T_CALL B_DestroyKeyObject PROTO_LIST ((B_KEY_OBJ *));
int T_CALL B_SetKeyInfo PROTO_LIST ((B_KEY_OBJ, B_INFO_TYPE, POINTER));
int T_CALL B_GetKeyInfo PROTO_LIST ((POINTER *, B_KEY_OBJ, B_INFO_TYPE));

/* The algorithm object.
 */
int T_CALL B_CreateAlgorithmObject PROTO_LIST ((B_ALGORITHM_OBJ *));
void T_CALL B_DestroyAlgorithmObject PROTO_LIST ((B_ALGORITHM_OBJ *));
int T_CALL B_SetAlgorithmInfo PROTO_LIST
  ((B_ALGORITHM_OBJ, B_INFO_TYPE, POINTER));
int T_CALL B_GetAlgorithmInfo PROTO_LIST
  ((POINTER *, B_ALGORITHM_OBJ, B_INFO_TYPE));

/* Algorithm operations.
 */
int T_CALL B_RandomInit PROTO_LIST
  ((B_ALGORITHM_OBJ, B_ALGORITHM_CHOOSER, A_SURRENDER_CTX *));
int T_CALL B_RandomUpdate PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int, A_SURRENDER_CTX *));
int T_CALL B_GenerateRandomBytes PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int, A_SURRENDER_CTX *));

int T_CALL B_DigestInit PROTO_LIST
  ((B_ALGORITHM_OBJ, B_KEY_OBJ, B_ALGORITHM_CHOOSER, A_SURRENDER_CTX *));
int T_CALL B_DigestUpdate PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int, A_SURRENDER_CTX *));
int T_CALL B_DigestFinal PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int *, unsigned int,
    A_SURRENDER_CTX *));

int T_CALL B_EncryptInit PROTO_LIST
  ((B_ALGORITHM_OBJ, B_KEY_OBJ, B_ALGORITHM_CHOOSER, A_SURRENDER_CTX *));
int T_CALL B_EncryptUpdate PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int *, unsigned int,
    unsigned char *, unsigned int, B_ALGORITHM_OBJ, A_SURRENDER_CTX *));
int T_CALL B_EncryptFinal PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int *, unsigned int,
    B_ALGORITHM_OBJ, A_SURRENDER_CTX *));

int T_CALL B_DecryptInit PROTO_LIST
  ((B_ALGORITHM_OBJ, B_KEY_OBJ, B_ALGORITHM_CHOOSER, A_SURRENDER_CTX *));
int T_CALL B_DecryptUpdate PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int *, unsigned int,
    unsigned char *, unsigned int, B_ALGORITHM_OBJ, A_SURRENDER_CTX *));
int T_CALL B_DecryptFinal PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int *, unsigned int,
    B_ALGORITHM_OBJ, A_SURRENDER_CTX *));

int T_CALL B_EncodeInit PROTO_LIST ((B_ALGORITHM_OBJ));
int T_CALL B_EncodeUpdate PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int *, unsigned int,
    unsigned char *, unsigned int));
int T_CALL B_EncodeFinal PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int *, unsigned int));

int T_CALL B_DecodeInit PROTO_LIST ((B_ALGORITHM_OBJ));
int T_CALL B_DecodeUpdate PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int *, unsigned int,
    unsigned char *, unsigned int));
int T_CALL B_DecodeFinal PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int *, unsigned int));

int T_CALL B_SignInit PROTO_LIST
  ((B_ALGORITHM_OBJ, B_KEY_OBJ, B_ALGORITHM_CHOOSER, A_SURRENDER_CTX *));
int T_CALL B_SignUpdate PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int, A_SURRENDER_CTX *));
int T_CALL B_SignFinal PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int *, unsigned int,
    B_ALGORITHM_OBJ, A_SURRENDER_CTX *));

int T_CALL B_VerifyInit PROTO_LIST
  ((B_ALGORITHM_OBJ, B_KEY_OBJ, B_ALGORITHM_CHOOSER, A_SURRENDER_CTX *));
int T_CALL B_VerifyUpdate PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int, A_SURRENDER_CTX *));
int T_CALL B_VerifyFinal PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int, B_ALGORITHM_OBJ,
    A_SURRENDER_CTX *));

int T_CALL B_KeyAgreeInit PROTO_LIST
  ((B_ALGORITHM_OBJ, B_KEY_OBJ, B_ALGORITHM_CHOOSER, A_SURRENDER_CTX *));
int T_CALL B_KeyAgreePhase1 PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int *, unsigned int,
    B_ALGORITHM_OBJ, A_SURRENDER_CTX *));
int T_CALL B_KeyAgreePhase2 PROTO_LIST
  ((B_ALGORITHM_OBJ, unsigned char *, unsigned int *, unsigned int,
    unsigned char *, unsigned int, A_SURRENDER_CTX *));

int T_CALL B_GenerateInit PROTO_LIST
  ((B_ALGORITHM_OBJ, B_ALGORITHM_CHOOSER, A_SURRENDER_CTX *));
int T_CALL B_GenerateKeypair PROTO_LIST
  ((B_ALGORITHM_OBJ, B_KEY_OBJ, B_KEY_OBJ, B_ALGORITHM_OBJ,
    A_SURRENDER_CTX *));
int T_CALL B_GenerateParameters PROTO_LIST
  ((B_ALGORITHM_OBJ, B_ALGORITHM_OBJ, B_ALGORITHM_OBJ, A_SURRENDER_CTX *));


/* Information for password-based encryption (PBE) algorithms.
 */
typedef struct {
  unsigned char *salt;                                        /* salt value */
  unsigned int iterationCount;                           /* iteration count */
} B_PBE_PARAMS;

typedef struct {
  unsigned int effectiveKeyBits;                     /* effective key length */
  unsigned char *salt;                                        /* salt value */
  unsigned int iterationCount;                           /* iteration count */
} B_RC2_PBE_PARAMS;

/* Information for MAC algorithm.
 */
typedef struct {
  unsigned int macLen;                               /* length of MAC value */
} B_MAC_PARAMS;

/* Information for BSAFE 1.x compatible encryption algorithms.
 */
#define B_BSAFE1_PAD 1
#define B_BSAFE1_PAD_CHECKSUM 2
#define B_BSAFE1_RAW 3
typedef struct {
  int encryptionType;                /* encryption type: B_BSAFE1_PAD, etc. */
} B_BSAFE1_ENCRYPTION_PARAMS;

typedef struct {
  unsigned char *key;                              /* pointer to 8 byte key */
  unsigned int effectiveKeyBits;           /* effective key length parameter */
} B_RC2_BSAFE1_PARAMS_KEY;

/* Key Info Types.
 */
int T_CALL KI_8Byte PROTO_LIST ((B_KEY_OBJ, int, POINTER));
int T_CALL KI_DES8 PROTO_LIST ((B_KEY_OBJ, int, POINTER));
int T_CALL KI_Item PROTO_LIST ((B_KEY_OBJ, int, POINTER));
int T_CALL KI_PKCS_RSAPrivate PROTO_LIST ((B_KEY_OBJ, int, POINTER));
int T_CALL KI_PKCS_RSAPrivateBER PROTO_LIST ((B_KEY_OBJ, int, POINTER));
int T_CALL KI_RSAPrivate PROTO_LIST ((B_KEY_OBJ, int, POINTER));
int T_CALL KI_RSAPublic PROTO_LIST ((B_KEY_OBJ, int, POINTER));
int T_CALL KI_RSAPublicBER PROTO_LIST ((B_KEY_OBJ, int, POINTER));
int T_CALL KI_RSA_CRT PROTO_LIST ((B_KEY_OBJ, int, POINTER));

/* Key Info Types for BSAFE 1.x Support.
 */
int T_CALL KI_DES_BSAFE1 PROTO_LIST ((B_KEY_OBJ, int, POINTER));
int T_CALL KI_DESX PROTO_LIST ((B_KEY_OBJ, int, POINTER));
int T_CALL KI_DESX_BSAFE1 PROTO_LIST ((B_KEY_OBJ, int, POINTER));
int T_CALL KI_RC2WithBSAFE1Params PROTO_LIST ((B_KEY_OBJ, int, POINTER));
int T_CALL KI_RC2_BSAFE1 PROTO_LIST ((B_KEY_OBJ, int, POINTER));
int T_CALL KI_RSAPrivateBSAFE1 PROTO_LIST ((B_KEY_OBJ, int, POINTER));
int T_CALL KI_RSAPublicBSAFE1 PROTO_LIST ((B_KEY_OBJ, int, POINTER));

/* Algorithm Info Types.
 */
int T_CALL AI_DES_CBC_IV8 PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_DES_CBCPadBER PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_DES_CBCPadIV8 PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_DHKeyAgree PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_DHKeyAgreeBER PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_DHParamGen PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD2 PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD2Random PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD2WithDES_CBCPad PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD2WithDES_CBCPadBER PROTO_LIST
  ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD2WithRC2_CBCPad PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD2WithRC2_CBCPadBER PROTO_LIST
  ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD2WithRSAEncryption PROTO_LIST
  ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD2WithRSAEncryptionBER PROTO_LIST
  ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD2_BER PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD5 PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD5Random PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD5WithDES_CBCPad PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD5WithDES_CBCPadBER PROTO_LIST
  ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD5WithRC2_CBCPad PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD5WithRC2_CBCPadBER PROTO_LIST
  ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD5WithRSAEncryption PROTO_LIST
  ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD5WithRSAEncryptionBER PROTO_LIST
  ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD5_BER PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_PKCS_RSAPrivate PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_PKCS_RSAPrivateBER PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_PKCS_RSAPublic PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_PKCS_RSAPublicBER PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_RC2_CBC PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_RC2_CBCPad PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_RC2_CBCPadBER PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_RC4 PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_RC4_BER PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_RFC1113Recode PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_RSAKeyGen PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_RSAPrivate PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_RSAPublic PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));

/* Algorithm Info Types for BSAFE 1.x Support.
 */
int T_CALL AI_DES_CBC_BSAFE1 PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_DESX_CBC_BSAFE1 PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_DESX_CBC_IV8 PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MAC PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_MD PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_RC2_CBC_BSAFE1 PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_RSAPrivateBSAFE1 PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));
int T_CALL AI_RSAPublicBSAFE1 PROTO_LIST ((B_ALGORITHM_OBJ, int, POINTER));

int T_CALL B_EncodeDigestInfo PROTO_LIST
  ((unsigned char *, unsigned int *, unsigned int, ITEM *, unsigned char *,
    unsigned int));
int T_CALL B_DecodeDigestInfo PROTO_LIST
  ((ITEM *, ITEM *, unsigned char *, unsigned int));

/* Algorithm methods for use int the algorithm chooser.
 */
extern B_ALGORITHM_METHOD AM_DESX_CBC_DECRYPT;
extern B_ALGORITHM_METHOD AM_DESX_CBC_ENCRYPT;
extern B_ALGORITHM_METHOD AM_DES_CBC_DECRYPT;
extern B_ALGORITHM_METHOD AM_DES_CBC_ENCRYPT;
extern B_ALGORITHM_METHOD AM_DH_PARAM_GEN;
extern B_ALGORITHM_METHOD AM_MD2;
extern B_ALGORITHM_METHOD AM_MD2_RANDOM;
extern B_ALGORITHM_METHOD AM_MD5;
extern B_ALGORITHM_METHOD AM_MD5_RANDOM;
extern B_ALGORITHM_METHOD AM_MD;
extern B_ALGORITHM_METHOD AM_RC2_CBC_DECRYPT;
extern B_ALGORITHM_METHOD AM_RC2_CBC_ENCRYPT;
extern B_ALGORITHM_METHOD AM_RC4_DECRYPT;
extern B_ALGORITHM_METHOD AM_RC4_ENCRYPT;
extern B_ALGORITHM_METHOD AM_RSA_CRT_DECRYPT;
extern B_ALGORITHM_METHOD AM_RSA_CRT_ENCRYPT;
extern B_ALGORITHM_METHOD AM_RSA_DECRYPT;
extern B_ALGORITHM_METHOD AM_RSA_ENCRYPT;
extern B_ALGORITHM_METHOD AM_RSA_KEY_GEN;

extern B_ALGORITHM_METHOD AM_DH_PARAM_GEN_86;
extern B_ALGORITHM_METHOD AM_RSA_CRT_DECRYPT_86;
extern B_ALGORITHM_METHOD AM_RSA_CRT_ENCRYPT_86;
extern B_ALGORITHM_METHOD AM_RSA_DECRYPT_86;
extern B_ALGORITHM_METHOD AM_RSA_ENCRYPT_86;
extern B_ALGORITHM_METHOD AM_RSA_KEY_GEN_86;

#endif
