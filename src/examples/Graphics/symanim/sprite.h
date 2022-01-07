#ifndef __SPRITE_H
#define __SPRITE_H


/******************************************************************************
**
**  $Id: sprite.h,v 1.5 1994/10/19 23:28:03 vertex Exp $
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __OPERAMATH_H
#include "operamath.h"
#endif


extern bool  zscaling;
extern uint8 rotating;


void InitSprites(uint32 bitmapWidth, uint32 bitmapHeight);

Err LoadSprite(const char *fileName,  frac16 xvel, frac16 yvel, frac16 zvel, frac16 tvel);
void UnloadSprites(void);
void DrawSprites(Item bitmapItem);
void MoveSprites(void);
int32 SetSymOrder(int32 nsym);


/*****************************************************************************/


#endif /* __SPRITE_H */
