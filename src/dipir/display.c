/*	$Id: display.c,v 1.3 1994/10/07 20:57:45 markn Exp $ */
/*
 * Code to put a picture up on the display.
 */
#define	APPSPLASH 1
#include "types.h"
#include "dipir.h"

/*
 * These constants are fixed by the graphics hardware.
 */
#define	ScreenWidth		320
#define	ScreenHeight		240
#define	ScreenDepth		16

#define	BitsPerByte		8
#define	BytesPerScreenPixel	(ScreenDepth / BitsPerByte)
#define	BytesPerScreenLine	(ScreenWidth * ScreenDepth / BitsPerByte)
#define	MaxScreenColor		((1<<ScreenDepth)-1)

/*
 * Extract the i-th bitfield from buf.  
 * Buf is treated as an array of elements, each "sz" bits long.
 * sz may be 1 to 32.
 */
#define	GET_FIELD8(buf,i,sz) \
	(((uint8)(((uint8*)buf)[(i)*(sz)/8L]) >> \
		((8L-(sz))-(((uint32)(i)*(uint32)(sz))%8))) & ((1L<<(sz))-1))

#define	GET_FIELD16(buf,i,sz) \
	(((uint16)(((uint16*)buf)[(i)*(sz)/16L]) >> \
		((16L-(sz))-(((uint32)(i)*(uint32)(sz))%16))) & ((1L<<(sz))-1))

#define	GET_FIELD(buf,i,sz) \
	(((sz) <= 8) ? GET_FIELD8(buf,i,sz) : GET_FIELD16(buf,i,sz))

extern void FindMemSize(uint32 *pDramSize, uint32 *pVramSize);

static void 
memset16(uint16 *p, uint16 v, uint32 count)
{
	while (count > 0)
	{
		*p++ = v;
		count -= sizeof(uint16);
	}
}

static uint8 *
FindVideoBuf(void)
{
	uint32 dramSize, vramSize;

	/* Figure out where the current video buffer is.
	 * We "just know" that it is 256K from the top of VRAM */
	FindMemSize(&dramSize, &vramSize);
	return (uint8*) (dramSize + vramSize - (256*1024));
}

void
ClearDisplay(int16 color)
{
	memset16((uint16*)FindVideoBuf(), color, 
		BytesPerScreenLine * ScreenHeight);
}

static int 
u_memcmp(uint8 *a, uint8 *b, uint32 n)
{
	while (n-- > 0) {
		if (*a++ != *b++)
			return 1;
	}
	return 0;
}

int32
DefaultDisplayImage(void *image0, char *pattern)
{
	struct VideoImage *image = (struct VideoImage *)image0;
	uint8 *picture = ((uint8*)image0) + sizeof(struct VideoImage);
	/* Point to video buffer which has already been set up by 
	 * the boot ROM. */
	uint8 *videobuf;
	uint32 ColorScale;
	int32 leftmargin;
	int32 topmargin;
	int32 lines;

#define	BytesPerPictureLine \
	((uint32)image->vi_Width * (uint32)image->vi_Depth / BitsPerByte)

	videobuf = FindVideoBuf();

	/* Clear the screen to black */
	memset16((uint16*)videobuf, 0, BytesPerScreenLine * ScreenHeight);

	if (u_memcmp((uint8*)image->vi_Pattern, (uint8*)pattern, 
			sizeof(image->vi_Pattern))) 
	{
		/* Image has invalid pattern field */
		return 0;
	}
	if (image->vi_Depth > ScreenDepth)
	{
		/* Image is deeper than the screen!  Not supported. */
		return 0;
	}

	/* Figure out how to scale the colors */
	ColorScale = (1L << ScreenDepth) / ((1L << image->vi_Depth) -1);

	/* Figure out left and top margins */
	leftmargin = (ScreenWidth - (int32)image->vi_Width) / 2;
	topmargin = (ScreenHeight - (int32)image->vi_Height) / 2;
	/* Make sure top margin is even, because our loop below
	 * does two lines at a time. */
	if ((topmargin % 2) != 0)
		topmargin++;
	/* Figure out number of lines we must do (implies bottom margin) */
	if (topmargin >= 0)
	{
		videobuf += topmargin * BytesPerScreenLine;
		lines = image->vi_Height;
	} else
	{
		picture -= topmargin * BytesPerPictureLine;
		lines = ScreenHeight;
	}

	/*
	 * Now copy the picture, line by line.
	 * Because of the screwy graphics hardware, we actually do two
	 * lines at a time.
	 */
	for ( ;  lines > 0;  lines -= 2)
	{
		uint32 pix;
		uint32 color;
		uint32 ln;

		/* For each pixel that is supposed to go to the screen, 
		 * extract the pixel from the image, scale the color, 
		 * and store it on the screen. */
		if (leftmargin >= 0)
		{
			for (pix = 0;  pix < image->vi_Width;  pix++)
			for (ln = 0;  ln <= 1;  ln++)
			{
				color = GET_FIELD(picture, 2*pix+ln, 
						image->vi_Depth); 
				color *= ColorScale;
				if (color > MaxScreenColor)
					color = MaxScreenColor;
				((uint16*)videobuf)[2*(pix+leftmargin)+ln] = 
					(uint16)color;
			}
		} else
		{
			for (pix = 0;  pix < ScreenWidth;  pix++)
			for (ln = 0;  ln <= 1;  ln++)
			{
				color = GET_FIELD(picture, 
						2*(pix-leftmargin)+ln,
						image->vi_Depth);
				color *= ColorScale;
				if (color > MaxScreenColor)
					color = MaxScreenColor;
				((uint16*)videobuf)[2*pix+ln] = (uint16)color;
			}
		}
		/* Advance to the next line (actually, next pair of lines)
		 * in both the screen and the picture. */
		videobuf += BytesPerScreenLine * 2;
		picture += BytesPerPictureLine * 2;
	}
	return 1;
}

