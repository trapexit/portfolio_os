/* $Id: gfx.h,v 1.3 1994/02/09 02:04:35 limes Exp $ */

/* file gfx.h */

typedef struct Console
{
	int cons_x,cons_y;	/* upper left corner */
	int cons_w,cons_h;	/* width and height */
	int cons_curx,cons_cury; /* current text position */
	int	cons_color;	/* back ground color */
	int	cons_textcolor;
	long	cons_flags;
} Console;

typedef struct Pixmap
{
	Node	pm;
	int	w,h;
	unsigned long *Start;
} Pixmap;

typedef struct Position
{
	int x, y;
} Position;

typedef struct Rectangle
{
	int x, y;
	int w, h;
} Rectangle;

typedef struct Button
{
	Node	b;
	Rectangle pos;
	char *text;
	int	c;
	void (*func)();
} Button;


#define RED	(31<<10)
#define GREEN	(31<<5)
#define BLUE	(31)
#define CYAN	(BLUE|GREEN)
#define MAGENTA	(BLUE|RED)
#define YELLOW	(GREEN|RED)
#define WHITE	(RED|GREEN|BLUE)
#define BLACK	(0)
#define DARKBLUE	(BLUE>>1)
#define DARKRED	(RED>>1)
#define DARKGREEN	(GREEN>>1)
#define GRAY	(DARKRED|DARKGREEN|DARKBLUE)
#define GREY	GRAY
#define PUKE	(GREEN|DARKRED|DARKBLUE)

#define TOBLUE(x)	(x>>3)
#define TOGREEN(x)	((x&0xf8)<<2)
#define TORED(x)	((x&0xf8)<<7)

#define CHARTREUSE	(DARKRED|GREEN)
#define ORCHID		(TORED(218)|TOGREEN(112)|TOBLUE(214))

#define WIDTH	320
#define HEIGHT	240

void InitScreen(char *,int,int);
int KernelReadPixel(Pixmap *,int,int);
void KernelWritePixel(Pixmap *,int,int,int);
int KernelPlotChar(int,int,int, int);
int Inside(Position *, Rectangle *);
int CharWidth(char);
int TextLength(char *);

extern void MakeKernelDisplay(int quiet);
extern void ScrollRaster(int x,int y,int w,int h,int dx,int dy);
extern void RectFill(int c,int x,int y,int w,int h);
extern void foomyWritePixel(int x,int y,int c);

