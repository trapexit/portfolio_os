#ifndef _CSHAPE_H_
#define _CSHAPE_H_

/******************************************************************************
**
**  $Id: cshape.h,v 1.3 1994/11/23 23:58:13 vertex Exp $
**
**  Simple graphic shape class.
**
******************************************************************************/

#include "portfolio.h"
#include "utils3do.h"

class CShape
{
	public:
		CShape (ScreenContext *sc);
		CShape (ScreenContext *sc, Rect& r, Color c = 0xffffffff);

		virtual void Draw( void ) {}
		virtual void Erase( void );

		virtual void Move(long x, long y);
		virtual void MoveTo(long x, long y);

		Color GetColor( void );
		Rect *GetBBox(Rect *pBBox = 0L);
		ScreenContext *GetScreen( void );

		void MoveUp(long y);
		void MoveRight(long x);
		void MoveDown(long y);
		void MoveLeft(long x);

		void SetColor(Color c);
		void SetColor(uchar r, uchar g, uchar b);

		void SetScreen(ScreenContext *pSC);
		void SetBBox(Rect& r);

	protected:
		Rect			fBBox;
		GrafCon			fGrafCon;
		ScreenContext	*fScreenContext;
};

class CRect : public CShape
{
	public:
		CRect(ScreenContext *pSC, Rect &r, Color c  = 0xffffffff);

		void Draw( void );
		void SetFrameFill(Boolean frameFill);

	private:
		Boolean fFrameFill;
};

class CFont;

class CText : public CShape
{
	public:
		CText(ScreenContext *pSC, char *pText);
		CText(ScreenContext *pSC, char *pText, CFont *pFont);

		void Draw( void );

		long GetCharWidth (long lChar);
		long GetStringWidth (char *pString);

		void SetFont(CFont *pFont);
		void SetText(char *pText);

	private:
		char *fText;
		CFont *fFont;
};

class CLine : public CShape
{
	public:
		CLine(ScreenContext *pSC, Coord sx, Coord sy, Coord ex, Coord ey, Color c = 0xffffffff);

		void Draw( void );

		void SetLine(Coord sx, Coord sy, Coord ex, Coord ey);

	private:
		Coord fStartPtX;
		Coord fStartPtY;
		Coord fEndPtX;
		Coord fEndPtY;
		long fSlope;
};

#endif
