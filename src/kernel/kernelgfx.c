/* $Id: kernelgfx.c,v 1.7 1994/02/09 01:22:45 limes Exp $ */

#include "types.h"
/*#include "Hardware.h"*/
#include "nodes.h"
#include "list.h"
#include "task.h"
#include "mem.h"
#include "string.h"

extern void printf(const char *fmt,...);

#include "gfx.h"

Pixmap ScreenPM,*ScreenPMp;

#define RED	(31<<10)
#define GREEN	(31<<5)
#define BLUE	(31)
#define WHITE	(RED|GREEN|BLUE)
#define BLACK	(0)

/*#define TESTCONSOLE*/

extern void (*ptr_firq)();

#define LITTLEENDIAN

void qwetrig(void);

unsigned short *
KernelAdrPixel(pm,x,y)
Pixmap *pm;
int x,y;
{
	unsigned short *p;
	if (x < 0) return 0;
	if (y < 0) return 0;
	if (x >= pm->w ) return 0;
	if (y >= pm->h) return 0;
	p = (unsigned short *)pm->Start;
	p += x*2;
	p += y*pm->w;
	if (y&1)
	{
		/* bump to real word */
#ifdef LITTLEENDIAN
		p -= (pm->w - 1);
#else
		p -= pm->w;
#endif
	}
#ifndef LITTLEENDIAN
	else	p++;
#endif

	return p;
}

#ifdef undef
int
KernelReadPixel(pm,x,y)
Pixmap *pm;
int x,y;
{
	unsigned short *p;
	p = KernelAdrPixel(pm,x,y);
	if (!p)	return -1;
	return *p;
}
#endif

void
KernelWritePixel(pm,c,x,y)
Pixmap *pm;
int x,y,c;
{
	ushort *p;
	p = KernelAdrPixel(pm,x,y);
	if (p) *p = (short)c;
	else qwetrig();
}

void
foomyWritePixel(c,x,y)
int x,y,c;
{
	ushort *p;
	p = KernelAdrPixel(ScreenPMp,x,y);
	if (p) *p = (short)c;
}

#ifdef undef
void
vertical_line(c,x,y,h)
int c,x,y,h;
{
	while (h--)	KernelWritePixel(ScreenPMp,c,x,y++);
}
#endif

void
horizontal_line(c,x,y,w)
int c,x,y,w;
{
	while (w--)	KernelWritePixel(ScreenPMp,c,x++,y);
}

void
RectFill(c,x,y,w,h)
int c,x,y,w,h;
{
	while (h--)
		horizontal_line(c,x,y++,w);
}

void
qwetrig(void)
{
	while (1);
}

void
ScrollRaster(x,y,w,h,dx,dy)
int x,y,w,h,dx,dy;
{
	/* presume w full */
	unsigned short *dst = KernelAdrPixel(ScreenPMp,x,y);
	unsigned short *src = KernelAdrPixel(ScreenPMp,x,y-dy);
	int size = w*(h+dy)*2;
	memcpy(dst,src,size);
}

#ifdef undef
int
Inside(p,r)
Position *p;
Rectangle *r;
{
	if (p->x < r->x)	return 0;
	if (p->y < r->y)	return 0;
	if (p->x > r->x+r->w)	return 0;
	if (p->y > r->y+r->h)	return 0;
	return 1;
}
#endif

void
InitScreen(p,w,h)
char *p;
int w,h;
{
	/*printf("InitScreen(%lx)\n",(ulong)p);*/
	ScreenPM.w = w;
	ScreenPM.h = h;
	ScreenPM.Start = (ulong *)p;
	ScreenPMp = &ScreenPM;
}
