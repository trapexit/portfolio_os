/* $Id: vdl.c,v 1.43 1994/03/25 08:07:38 limes Exp $ */
#include "types.h"
#include "internalf.h"

#include "kernel.h"
#include "hardware.h"
#include "inthard.h"
#include "clio.h"
#include "string.h"
#include "gfx.h"

/*#define TEST*/
/*#define GURU*/
/*#define VDL*/

#define DBUG(x)	/*printf x*/
/*#define COLORBARS*/

extern uint8 *vram_start;
extern int32 vram_size;
uint8 *screenstart;

#ifdef VDL

/* There can be maximum of 50 vdl instructions per block */

extern int charcolor;
extern int okguru;

void
Box(color)
int color;
{
	RectFill(color,16,20,320-32,7);
	RectFill(color,16,20,7,60);
	RectFill(color,319-8-16+2,20,7,60);
	RectFill(color,16,80,319-32,7);
}

#ifdef GURU
void Delay(int cnt)
{
	volatile int qwe;
	while (cnt--)
		qwe += qwe + cnt;
}

extern int cx,cy;
extern int kill_kprintf;

void
GcString(s)
char *s;
{
	char c;
	while ( c = *s++)	GCPutChar(c);
}
#endif



void
Guru(int gooroo)
{
#ifdef GURU
    int i;
    int save_kill =  kill_kprintf;
    memset(screenstart,0,320*240*2);
    kill_kprintf = 0;
    cx = 32; cy = 30;
    charcolor = RED;
#ifdef undef
    GcString("            Unknown Application Failure ");
    cx = 32; cy = 30+8+8;
    GcString("       GURU Meditation Number 00:00:00 ");
    cx = 32; cy = 30+8+8+16;
    GcString("     Press Power Button twice to continue");
#else
    GcString("            Uh Uh UUAaahhh! ");
    cx = 32; cy = 30+8+8;
    GcString("       GURU Meditation Number 12:00:00 ");
    cx = 32; cy = 30+8+8+16;
    GcString("     You did not say the magic word");
#endif
    i = 10;
    while (i--)
    {
        cx = 196; cy = 30+8+8;
        charcolor = BLACK;
	GcString("12:00:00");
	Box(BLACK);
	Delay(200000);
        charcolor = CYAN;
        cx = 196; cy = 30+8+8;
	GcString("12:00:00");
	Box(RED);
	Delay(200000);
    }
    charcolor = 31<<5;
    cx = 50; cy = 120;
#ifdef undef
    GcString("   Task error trapped by kernel\n\n");
    Delay(800000);
    cx = 50;
    GcString("   Offensive task being deleted\n\n");
    cx = 50;
    Delay(800000);
    GcString("   About to resume normal operation\n");
#else
    GcString("   You have defective Hardware\n\n");
    Delay(800000);
    cx = 50;
    GcString("   Operating system needs Green\n\n");
    while (1)
    {
        cx = 196; cy = 30+8+8;
        charcolor = BLACK;
	GcString("12:00:00");
	Box(BLACK);
	Delay(200000);
        charcolor = CYAN;
        cx = 196; cy = 30+8+8;
	GcString("12:00:00");
	Box(RED);
	Delay(200000);
    }
    while (1);
    cx = 50;
    Delay(800000);
    GcString("   About to resume normal operation\n");
#endif
    Delay(800000);
    RectFill(0,0,0,320,200);
    cx = 50; cy = 200;
    GcString("Nothing like protected multitasking!\n");
    Delay(800000);
    RectFill(0,0,199,320,20);
    kill_kprintf = save_kill;
    charcolor = 0x7fff;
#endif
}

#endif

extern void EnableVDLVideo(void);

void
MakeKernelDisplay(int quiet)
{
#ifdef COLORBARS
    int i;
    uint32 oldints;
#endif
    /* put screen at end of vram now */
    DBUG(("MakeKernelDisplay quiet=%d\n",quiet));
    DBUG(("vram_start=%lx\n",vram_start));
#ifdef VDL
    screenstart = (uint8 *)(vram_start+vram_size-5*32*1024);
#endif
    /*screenstart = vram_start;*/
    screenstart = vram_start + vram_size - 256*1024;
    DBUG(("Calling InitScreen screenstart=%lx\n",screenstart));
    InitScreen(screenstart,320,240);
    DBUG(("Calling VDLOn\n"));
    /*MyMCTL |= CLUTXEN|VSCTXEN;*/
    EnableVDLVideo();
    /*RectFill(-1,0,0,320,240);*/

#ifdef COLORBARS
    if (quiet == 0)
	for (i = 0; i < 8; i++)
	{
		int r = i & 4;
		int g = i & 2;
		int b = i & 1;
		int c;
		if (r) c |= 31<<10;	/* want left and right color bars */
		if (g) c |= 31<<5;
		if (!b) c |= 31;
		RectFill(c,i*40,0,40,240);
		/*while (1);*/
	}
    DBUG(("after Calling RectFills\n"));
#ifdef undef
    horizontal_line(0x7fff,0,0,320);
    horizontal_line(0x7fff,239,0,320);
    vertical_line(0x7fff,0,0,240);
    vertical_line(0x7fff,319,0,240);
#endif
#endif
}
