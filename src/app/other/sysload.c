/* $Id: sysload.c,v 1.10 1994/09/24 19:15:32 vertex Exp $ */

/**
|||	AUTODOC PUBLIC tpg/shell/sysload
|||	sysload - Dynamically display the system's CPU/memory load
|||
|||	  Format
|||
|||	    sysload [-top <top edge>]
|||	            [-height <height>]
|||	            [-rate <update rate>]
|||	            [-barcolor <color>]
|||	            [-bordercolor <color>]
|||	            [-textcolor <color>]
|||	            [-bgcolor <color>]
|||	            [-dsp]
|||	            [-nodsp]
|||	            [-ram]
|||	            [-noram]
|||	            [-quit]
|||
|||	  Description
|||
|||	    This command creates an overlay that sits on top of any
|||	    screen displayed on a 3DO. The overlay has a graph showing
|||	    current CPU utilization, as well as four bar graphs showing
|||	    the number of DRAM pages used, VRAM pages used, DSP ticks
|||	    reserved, and DSP code used.
|||
|||	    The overlay hovers on top of all other displays on the 3DO system.
|||	    The overlay is only visible if there are opened screens in the
|||	    system. Closing all screens will close the overlay automatically,
|||	    and it comes back magically the next time a screen opens.
|||
|||	    If sysload is already running in the system, and it is run
|||	    again, any command-line options you give are passed automatically
|||	    to the running version, which responds to them. The second version
|||	    then exits.
|||
|||	  Arguments
|||
|||	    -top <top edge>             Lets you specify the top edge of
|||	                                the overlay on the display. The
|||	                                supplied number specifies the number
|||	                                of pixels from the top of the display
|||	                                where the overlay should start.
|||
|||	    -height <height>            Lets you specify the total height
|||	                                of the overlay.
|||
|||	    -rate <update rate>         Lets you specify the update rate to
|||	                                use in milliseconds. The default rate
|||	                                is 1000 milliseconds (1 second).
|||
|||	    -barcolor <color>           Lets you provide a 16-bit RGB value
|||	                                that is used to render the histograms
|||	                                and the graph.
|||
|||	    -bordercolor <color>        Lets you provide a 16-bit RGB value
|||	                                that is used to render the borders
|||	                                around the various sections of the
|||	                                overlay display.
|||
|||	    -textcolor <color>          Lets you specify a 16-bit RGB value
|||	                                that is used to render any text on
|||	                                the overlay display.
|||
|||	    -bgcolor <color>            Lets you specify a 16-bit RGB value
|||	                                that is used to render the background
|||	                                of the overlay display.
|||
|||	    -dsp                        Specifies that you want the DSP usage
|||	                                line displayed. This is the default.
|||
|||	    -nodsp                      Specifies that you do not want the DSP
|||	                                usage line displayed.
|||
|||	    -ram                        Specifies that you want the RAM usage
|||	                                line displayed. This is the default.
|||
|||	    -noram                      Specifies that you do not want the RAM
|||	                                usage line displayed.
|||
|||	    -quit                       Tells a previously started version of
|||	                                sysload to exit.
|||
|||	  Implementation
|||
|||	    Command implemented in V22.
|||
|||	  Location
|||
|||	    $c/sysload
**/

#include "types.h"
#include "graphics.h"
#include "audio.h"
#include "mem.h"
#include "msgport.h"
#include "io.h"
#include "device.h"
#include "task.h"
#include "stdio.h"
#include "string.h"
#include "time.h"
#include "debug.h"


/*****************************************************************************/


/* command-line options we support */
#define SYSLOAD_OPTF_TOP         (1 << 0)
#define SYSLOAD_OPTF_HEIGHT      (1 << 1)
#define SYSLOAD_OPTF_RATE        (1 << 2)
#define SYSLOAD_OPTF_TEXTCOLOR   (1 << 3)
#define SYSLOAD_OPTF_BARCOLOR    (1 << 4)
#define SYSLOAD_OPTF_BORDERCOLOR (1 << 5)
#define SYSLOAD_OPTF_BGCOLOR     (1 << 6)
#define SYSLOAD_OPTF_DSP         (1 << 7)
#define SYSLOAD_OPTF_RAM         (1 << 8)
#define SYSLOAD_OPTF_QUIT        (1 << 9)

/* packet of options passed from main code to metering loop */
typedef struct OptionPacket
{
    uint32 op_ValidOptions;     /* see SYSLOAD_OPTF_XXX constants */
    uint32 op_Top;
    uint32 op_Height;
    uint32 op_Rate;
    uint32 op_TextColor;
    uint32 op_BarColor;
    uint32 op_BorderColor;
    uint32 op_BgColor;
    bool   op_DSP;
    bool   op_RAM;
    bool   op_Quit;
} OptionPacket;


/*****************************************************************************/


#define Error(x,err)      {printf(x); PrintfSysErr(err);}
#define SYSLOAD_PORTNAME  "SysLoad"
#define HORIZONTAL_MARGIN 12
#define HISTOGRAM_HEIGHT  12
#define FONT_WIDTH        8
#define FONT_HEIGHT       8
#define NUM_DRAM_PAGES    64
#define NUM_VRAM_PAGES    64


/*****************************************************************************/


static uint32         textColor;
static uint32         barColor;
static uint32         borderColor;
static uint32         bgColor;
static struct timeval rate;
static bool           dsp;
static bool           ram;
static bool           quit;

static uint32         busyCounter;
static Item           busyThread;

static Item           optPort;
static CCB           *ccb;

static Item           timer;
static Item           timerIO;
static struct IOInfo  timerIOInfo;

static Item           ovl;
static uint32         ovlWidth;
static uint32         ovlHeight;
static uint32         ovlTop;
static uint32         ovlMaxHeight;
static uint32         ovlGraphHeight;

static Item           bm;
static uint32         bmWidth;
static uint32         bmHeight;
static uint32         bmBufferSize;
static void          *bmBuffer;


/*****************************************************************************/


/* scrolls the bits in a rectangle within a bitmap by a given amount
 * The area vacated by the scroll is filled with the background color.
 */
static Err ScrollBitmap(Item bm, uint32 x0, uint32 y0, uint32 x1, uint32 y1,
                        int32 deltaX, int32 deltaY)
{
Bitmap *bmp;
Point   corners[4];
uint32  srcX0, srcY0, srcX1, srcY1;
uint32  dstX0, dstY0, dstX1, dstY1;
Rect    vacated;
GrafCon gc;

    if (deltaX < 0)
    {
        srcX0 = x0 - deltaX;
        srcX1 = x1;
        dstX0 = x0;
        dstX1 = x1 + deltaX;

        vacated.rect_XLeft  = x1 + deltaX + 1;
        vacated.rect_XRight = x1;
    }
    else if (deltaX > 0)
    {
        srcX0 = x0;
        srcX1 = x1 - deltaX;
        dstX0 = x0 + deltaX;
        dstX1 = x1;

        vacated.rect_XLeft  = x0;
        vacated.rect_XRight = x0 + deltaX - 1;
    }
    else
    {
        srcX0 = x0;
        srcX1 = x1;
        dstX0 = x0;
        dstX1 = x1;

        vacated.rect_XLeft  = x0;
        vacated.rect_XRight = x1;
    }

    if (deltaY < 0)
    {
        srcY0 = y0 - deltaY;
        srcY1 = y1;
        dstY0 = y0;
        dstY1 = y1 + deltaY;

        vacated.rect_YTop    = y1 + deltaY + 1;
        vacated.rect_YBottom = y1;
    }
    else if (deltaY > 0)
    {
        srcY0 = y0;
        srcY1 = y1 - deltaY;
        dstY0 = y0 + deltaY;
        dstY1 = y1;

        vacated.rect_YTop    = y0;
        vacated.rect_YBottom = y0 + deltaY - 1;
    }
    else
    {
        srcY0 = y0;
        srcY1 = y1;
        dstY0 = y0;
        dstY1 = y1;

        vacated.rect_YTop    = y0;
        vacated.rect_YBottom = y1;
    }

    bmp = (Bitmap *)LookupItem(bm);

    ccb->ccb_Flags     = CCB_YOXY | CCB_CCBPRE | CCB_LAST | CCB_SPABS |
                         CCB_LDSIZE | CCB_LDPRS | CCB_LDPPMP | CCB_ACW |
                         CCB_ACCW | CCB_ACE | CCB_BGND;
    ccb->ccb_NextPtr   = NULL;
    ccb->ccb_SourcePtr = (CelData *)GetPixelAddress(bm, srcX0, srcY0);
    ccb->ccb_PLUTPtr   = NULL;
    ccb->ccb_PIXC      = (PPMPC_2S_CCB | PPMPC_MF_8 | PPMPC_SF_8) |
                         ((PPMPC_2S_CCB | PPMPC_MF_8 | PPMPC_SF_8) << 16);

    ccb->ccb_PRE0 = PRE0_BPP_16 |
                    PRE0_LINEAR |
                    PRE0_BGND |
                    ((((srcY1 - srcY0 + 1) / 2) - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT);

    ccb->ccb_PRE1 = ((bmp->bm_Width - PRE1_WOFFSET_PREFETCH) << PRE1_WOFFSET10_SHIFT) |
                    (PRE1_LRFORM) |
                    (PRE1_NOSWAP) |
                    (PRE1_TLLSB_PDC0) |
                    (((srcX1 - srcX0 + 1) - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT);

    ccb->ccb_Width  = srcX1 - srcX0 + 1;
    ccb->ccb_Height = srcY1 - srcY0 + 1;

    corners[0].pt_X = dstX0;
    corners[0].pt_Y = dstY0;
    corners[1].pt_X = dstX1+1;
    corners[1].pt_Y = dstY0;
    corners[2].pt_X = dstX1+1;
    corners[2].pt_Y = dstY1+1;
    corners[3].pt_X = dstX0;
    corners[3].pt_Y = dstY1+1;
    MapCel(ccb,corners);

    DrawCels(bm,ccb);

    SetFGPen(&gc,bgColor);
    FillRect(bm,&gc,&vacated);

    return (0);
}


/*****************************************************************************/


void DrawText(GrafCon *gc, Item bm, char *text)
{
int32 plut[] = {0x18C66318, 0x63186318, 0x63186318, 0x63186318};
CCB   old;
CCB   statCCB = {
                    CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_PPABS | CCB_LDSIZE | CCB_LDPRS
                    | CCB_LDPPMP | CCB_LDPLUT | CCB_YOXY | CCB_ACW | CCB_ACCW
                    | PMODE_ZERO,

                    NULL, NULL, NULL,
                    0,0,
                    ONE_12_20,0, 0,ONE_16_16, 0,0,
                    (PPMP_MODE_NORMAL << PPMP_0_SHIFT)|(PPMP_MODE_AVERAGE << PPMP_1_SHIFT),
                };

    plut[0] = (((int32) gc->gc_FGPen) & 0xFFFF)
              | ((((int32) gc->gc_BGPen) << 16) & 0xFFFF0000);

    old                = *(GetCurrentFont()->font_CCB);
    *ccb               = statCCB;
    ccb->ccb_SourcePtr = old.ccb_SourcePtr;
    ccb->ccb_PLUTPtr   = plut;
    SetCurrentFontCCB(ccb);

    DrawText8(gc, bm, text);

    SetCurrentFontCCB(&old);
}


/*****************************************************************************/


static uint32 ConvertNum(char *str)
{
    if (*str == '$')
    {
        str++;
        return strtoul(str,0,16);
    }

    return strtoul(str,0,0);
}


/*****************************************************************************/


static int32 GetBitMapSize(int32 w, int32 h)
{
    /* Valid only for LRFORM buffers */
    h = (h + 1) >> 1;

    return (int32)(w * h * sizeof(int32));
}


/*****************************************************************************/


static Err CreateOverlay(void)
{
GrafCon gc;
Rect    rect;

    DeleteItem(ovl);

    if ((bmWidth != ovlWidth) || (bmHeight != ovlHeight))
    {
        DeleteItem(bm);

        if (bmBuffer)
            FreeMem(bmBuffer,bmBufferSize);

        bm           = -1;
        ovl          = -1;
        bmWidth      = ovlWidth;
        bmHeight     = ovlHeight;
        bmBufferSize = GetBitMapSize(bmWidth, bmHeight);

        bmBuffer = AllocMem(bmBufferSize, MEMTYPE_VRAM);
        if (bmBufferSize)
        {
            bm = CreateBitmapVA(CBM_TAG_WIDTH,  bmWidth,
                                CBM_TAG_HEIGHT, bmHeight,
                                CBM_TAG_BUFFER, bmBuffer,
                                TAG_END);
        }
    }

    /* make a box, the simple and slow way... */

    SetFGPen(&gc,bgColor);
    rect.rect_XLeft   = 0;
    rect.rect_YTop    = 0;
    rect.rect_XRight  = ovlWidth - 1;
    rect.rect_YBottom = ovlHeight - 1;
    FillRect(bm,&gc,&rect);

    SetFGPen(&gc,borderColor);
    rect.rect_XLeft   = HORIZONTAL_MARGIN;
    rect.rect_YTop    = 0;
    rect.rect_XRight  = ovlWidth - 1 - HORIZONTAL_MARGIN;
    rect.rect_YBottom = ovlHeight - 1 - HORIZONTAL_MARGIN;
    FillRect(bm,&gc,&rect);

    SetFGPen(&gc,bgColor);
    rect.rect_XLeft   = HORIZONTAL_MARGIN + 1;
    rect.rect_YTop    = 1;
    rect.rect_XRight  = ovlWidth - 1 - HORIZONTAL_MARGIN - 1;
    rect.rect_YBottom = ovlHeight - 1 - HORIZONTAL_MARGIN - 1;
    FillRect(bm,&gc,&rect);

    ovl = DisplayOverlay(bm,ovlTop);

    return (ovl);
}


/*****************************************************************************/


static void DeleteOverlay(void)
{
    DeleteItem(ovl);
    DeleteItem(bm);

    if (bmBuffer)
        FreeMem(bmBuffer,bmBufferSize);
}


/*****************************************************************************/


static void ApplyOP(OptionPacket *op)
{
    if (op->op_ValidOptions & SYSLOAD_OPTF_TOP)
        ovlTop = op->op_Top;

    if (op->op_ValidOptions & SYSLOAD_OPTF_HEIGHT)
        ovlHeight = op->op_Height;

    if (op->op_ValidOptions & SYSLOAD_OPTF_RATE)
    {
        rate.tv_sec  = op->op_Rate / 1000;
        rate.tv_usec = (op->op_Rate % 1000) * 1000;
    }

    if (op->op_ValidOptions & SYSLOAD_OPTF_TEXTCOLOR)
        textColor = op->op_TextColor;

    if (op->op_ValidOptions & SYSLOAD_OPTF_BARCOLOR)
        barColor = op->op_BarColor;

    if (op->op_ValidOptions & SYSLOAD_OPTF_BORDERCOLOR)
        borderColor = op->op_BorderColor;

    if (op->op_ValidOptions & SYSLOAD_OPTF_BGCOLOR)
        bgColor = op->op_BgColor;

    if (op->op_ValidOptions & SYSLOAD_OPTF_DSP)
        dsp = op->op_DSP;

    if (op->op_ValidOptions & SYSLOAD_OPTF_RAM)
        ram = op->op_RAM;

    quit = op->op_Quit;

    if (ovlTop > ovlMaxHeight)
        ovlTop = ovlMaxHeight;

    if (ovlHeight > ovlMaxHeight)
        ovlHeight = ovlMaxHeight;

    if (ovlHeight < HISTOGRAM_HEIGHT * 2)
        ovlHeight = HISTOGRAM_HEIGHT * 2;

    if (ovlTop + ovlHeight > ovlMaxHeight)
        ovlTop = ovlMaxHeight - ovlHeight;

    ovlGraphHeight = ovlHeight;

    if (ram)
        ovlGraphHeight -= HISTOGRAM_HEIGHT;

    if (dsp)
        ovlGraphHeight -= HISTOGRAM_HEIGHT;
}


/*****************************************************************************/


static Err WaitFor(struct timeval *tv)
{
    timerIOInfo.ioi_Unit            = TIMER_UNIT_USEC;
    timerIOInfo.ioi_Command         = TIMERCMD_DELAY;
    timerIOInfo.ioi_Send.iob_Len    = sizeof(struct timeval);
    timerIOInfo.ioi_Send.iob_Buffer = tv;

    return (DoIO(timerIO,&timerIOInfo));
}


/*****************************************************************************/


static Err Histo(Item bm, uint32 value, uint32 range,
                 char *label, uint32 x, uint32 y, uint32 width, uint32 height,
                 bool drawBorder)
{
Rect    rect;
char    buffer[40];
uint32  len;
GrafCon gc;

    if (drawBorder)
    {
        SetFGPen(&gc,borderColor);
        rect.rect_XLeft   = x;
        rect.rect_YTop    = y;
        rect.rect_XRight  = x + width - 1;
        rect.rect_YBottom = y + height - 1;
        FillRect(bm,&gc,&rect);
    }

    x      += 1;
    y      += 1;
    width  -= 2;
    height -= 2;

    if (value == 0)
    {
        SetFGPen(&gc,bgColor);
        rect.rect_XLeft   = x;
        rect.rect_YTop    = y;
        rect.rect_XRight  = x + width - 1;
        rect.rect_YBottom = y + height - 1;
        FillRect(bm,&gc,&rect);
    }
    else if (value == range)
    {
        SetFGPen(&gc,barColor);
        rect.rect_XLeft   = x;
        rect.rect_YTop    = y;
        rect.rect_XRight  = x + width - 1;
        rect.rect_YBottom = y + height - 1;
        FillRect(bm,&gc,&rect);
    }
    else
    {
        SetFGPen(&gc,barColor);
        rect.rect_XLeft   = x;
        rect.rect_YTop    = y;
        rect.rect_XRight  = x + (value * width / range);
        rect.rect_YBottom = y + height - 1;
        FillRect(bm,&gc,&rect);

        SetFGPen(&gc,bgColor);
        rect.rect_XLeft  = rect.rect_XRight + 1;
        rect.rect_XRight = x + width - 1;
        FillRect(bm,&gc,&rect);
    }

    sprintf(buffer,label,value);
    len = strlen(buffer) * FONT_WIDTH;

    MoveTo(&gc,(width - len) / 2 + x, (height - FONT_HEIGHT) / 2 + y);
    SetFGPen(&gc,textColor);
    SetBGPen(&gc,0);
    DrawText(&gc,bm,buffer);

    return 0;
}


/*****************************************************************************/


static void BusyThread(void)
{
    while (TRUE)
        busyCounter++;
}


/*****************************************************************************/


static Err MeterLoop(OptionPacket *op)
{
Item     msgItem;
Msg     *msg;
Err      err;
uint32   index;
uint32   plotX;
GrafCon  gc;
bool     first;
bool     histoBorders;
uint32   sub;
MemInfo  memInfo;
uint32   oldDRAMPages, oldVRAMPages;
uint32   oldDSPCode, oldDSPTicks;
uint32   amount;
uint32   nominalCount;
bool     newOpts;

    SetItemPri(CURRENTTASK->t.n_Item,199);

    ovlMaxHeight = 240;
    ovlWidth     = 320;
    ovlHeight    = 50;
    ovlTop       = 180;
    barColor     = MakeRGB15(21,25,0);
    borderColor  = MakeRGB15(1,10,31);
    textColor    = MakeRGB15(12,14,25);
    bgColor      = MakeRGB15(0,0,6);
    dsp          = TRUE;
    ram          = TRUE;
    rate.tv_sec  = 1;
    rate.tv_usec = 0;
    quit         = FALSE;
    newOpts      = TRUE;

    /* this block of assignments is to keep the silly compiler from complaining
     * about possible use of uninited variables
     */
    oldDRAMPages = 0xffffffff;
    oldVRAMPages = 0xffffffff;
    oldDSPCode   = 0xffffffff;
    oldDSPTicks  = 0xffffffff;
    plotX        = HORIZONTAL_MARGIN + 5;
    histoBorders = TRUE;
    first        = TRUE;
    nominalCount = 0;

    ApplyOP(op);

    while (!quit)
    {
        while (!quit)
        {
            msgItem = GetMsg(optPort);
            if (msgItem <= 0)
                break;

            msg = (Msg *)LookupItem(msgItem);

            ApplyOP((OptionPacket *)msg->msg_DataPtr);
            newOpts = TRUE;
            ReplyMsg(msgItem,0,NULL,0);
        }

        if (!quit)
        {
            if (newOpts)
            {
                newOpts      = FALSE;
                oldDRAMPages = 0xffffffff;
                oldVRAMPages = 0xffffffff;
                oldDSPCode   = 0xffffffff;
                oldDSPTicks  = 0xffffffff;
                plotX        = HORIZONTAL_MARGIN + 5;
                histoBorders = TRUE;
                first        = TRUE;

                err = CreateOverlay();
                if (err <= 0)
                {
                    Error("Unable to create overlay: ",err);
                    return err;
                }

                /* how much can the thread count to at the given sample rate,
                 * in a mostly unimpeeded way
                 */
                SetItemPri(busyThread,198);
                busyCounter = 0;
                WaitFor(&rate);
                nominalCount = busyCounter / 1000;
                SetItemPri(busyThread,10);
            }

            sub = HISTOGRAM_HEIGHT + 1;
            if (dsp && ram)
                sub += HISTOGRAM_HEIGHT - 1;

            if (ram)
            {
                AvailMem(&memInfo,MEMTYPE_DRAM);
                amount = memInfo.minfo_SysFree / GetPageSize(MEMTYPE_DRAM);

                if (amount != oldDRAMPages)
                {
                    oldDRAMPages = amount;
                    Histo(bm,NUM_DRAM_PAGES - amount,NUM_DRAM_PAGES,
                          "DRAM %d/64",
                          HORIZONTAL_MARGIN,ovlHeight - sub,
                          (ovlWidth / 2) - HORIZONTAL_MARGIN, HISTOGRAM_HEIGHT,histoBorders);
                }

                AvailMem(&memInfo,MEMTYPE_VRAM);
                amount = memInfo.minfo_SysFree / GetPageSize(MEMTYPE_SYSTEMPAGESIZE|MEMTYPE_VRAM);

                if (amount != oldVRAMPages)
                {
                    oldVRAMPages = amount;

                    Histo(bm,NUM_VRAM_PAGES - amount,NUM_VRAM_PAGES,
                          "VRAM %d/64",
                          (ovlWidth / 2) - 1,ovlHeight - sub,
                          (ovlWidth / 2) - HORIZONTAL_MARGIN + 1, HISTOGRAM_HEIGHT,histoBorders);
                }

                sub -= HISTOGRAM_HEIGHT - 1;
            }

            if (dsp)
            {
                amount = DSPGetTotalRsrcUsed(DRSC_N_MEM);

                if (amount != oldDSPCode)
                {
                     oldDSPCode = amount;

                     Histo(bm,amount,512,
                           "DSP-CODE %d/512",
                           HORIZONTAL_MARGIN,ovlHeight - sub,
                           (ovlWidth / 2) - HORIZONTAL_MARGIN,HISTOGRAM_HEIGHT,histoBorders);
                }

                amount = DSPGetTotalRsrcUsed(DRSC_TICKS);

                if (amount != oldDSPTicks)
                {
                    oldDSPTicks = amount;
                    Histo(bm,amount,565,
                          "DSP-TCKS %d/565",
                          (ovlWidth / 2) - 1,ovlHeight - sub,
                          (ovlWidth / 2) - HORIZONTAL_MARGIN + 1,HISTOGRAM_HEIGHT,histoBorders);
                }
            }

            histoBorders = FALSE;

            busyCounter = 0;
            WaitFor(&rate);
            busyCounter /= 1000;
            if (busyCounter > nominalCount)
                busyCounter = nominalCount;
            index = (busyCounter * (ovlGraphHeight - 4)) / nominalCount;

            SetFGPen(&gc,barColor);
            if (first)
            {
                first = FALSE;
                MoveTo(&gc,plotX++,index + 2);
            }
            else
            {
                DrawTo(bm,&gc,plotX++,index + 2);

                if (plotX == ovlWidth - HORIZONTAL_MARGIN - 1)
                {
                    plotX -= 5;
                    MoveTo(&gc,plotX,index + 2);
                    ScrollBitmap(bm,HORIZONTAL_MARGIN+1,2,
                                 ovlWidth - HORIZONTAL_MARGIN - 2, ovlGraphHeight - 3,
                                 -5,0);

                }
            }
        }
    }

    DeleteOverlay();

    return (0);
}


/*****************************************************************************/


int main(int argc, char **argv)
{
int          parm;
OptionPacket op;
Item         repPort;
Item         msg;
Item         err;

    memset(&op,0,sizeof(OptionPacket));

    for (parm = 1; parm < argc; parm++)
    {
        if ((strcmp("-help",argv[parm]) == 0)
         || (strcmp("-?",argv[parm]) == 0)
         || (strcmp("help",argv[parm]) == 0)
         || (strcmp("?",argv[parm]) == 0))
        {
            printf("sysload: monitor system load\n");
            printf("  -top <top edge>     - location of performance meter\n");
            printf("  -height <height>    - height of performance meter\n");
            printf("  -rate <update rate> - milliseconds between meter updates\n");
            printf("  -barcolor <color>   - 16-bit RGB value for bar and graph\n");
            printf("  -bordercolor <color>- 16-bit RGB value for borders around histograms\n");
            printf("  -textcolor <color>  - 16-bit RGB value for text\n");
            printf("  -bgcolor <color>    - 16-bit RGB value for display background\n");
            printf("  -dsp                - display the DSP histograms\n");
            printf("  -nodsp              - remove the DSP histograms\n");
            printf("  -ram                - display the RAM histograms\n");
            printf("  -noram              - remove the RAM histograms\n");
            printf("  -quit               - quit the program\n");
            return (0);
        }

        if (strcmp("-top",argv[parm]) == 0)
        {
            parm++;
            op.op_Top           = ConvertNum(argv[parm]);
            op.op_ValidOptions |= SYSLOAD_OPTF_TOP;
        }
        else if (strcmp("-height",argv[parm]) == 0)
        {
            parm++;
            op.op_Height        = ConvertNum(argv[parm]);
            op.op_ValidOptions |= SYSLOAD_OPTF_HEIGHT;
        }
        else if (strcmp("-rate",argv[parm]) == 0)
        {
            parm++;
            op.op_Rate          = ConvertNum(argv[parm]);
            op.op_ValidOptions |= SYSLOAD_OPTF_RATE;
        }
        else if (strcmp("-textcolor",argv[parm]) == 0)
        {
            parm++;
            op.op_TextColor     = ConvertNum(argv[parm]);
            op.op_ValidOptions |= SYSLOAD_OPTF_TEXTCOLOR;
        }
        else if (strcmp("-barcolor",argv[parm]) == 0)
        {
            parm++;
            op.op_BarColor      = ConvertNum(argv[parm]);
            op.op_ValidOptions |= SYSLOAD_OPTF_BARCOLOR;
        }
        else if (strcmp("-bordercolor",argv[parm]) == 0)
        {
            parm++;
            op.op_BorderColor   = ConvertNum(argv[parm]);
            op.op_ValidOptions |= SYSLOAD_OPTF_BORDERCOLOR;
        }
        else if (strcmp("-bgcolor",argv[parm]) == 0)
        {
            parm++;
            op.op_BgColor       = ConvertNum(argv[parm]);
            op.op_ValidOptions |= SYSLOAD_OPTF_BGCOLOR;
        }
        else if (strcmp("-dsp",argv[parm]) == 0)
        {
            op.op_DSP           = TRUE;
            op.op_ValidOptions |= SYSLOAD_OPTF_DSP;
        }
        else if (strcmp("-nodsp",argv[parm]) == 0)
        {
            op.op_DSP           = FALSE;
            op.op_ValidOptions |= SYSLOAD_OPTF_DSP;
        }
        else if (strcmp("-ram",argv[parm]) == 0)
        {
            op.op_RAM           = TRUE;
            op.op_ValidOptions |= SYSLOAD_OPTF_RAM;
        }
        else if (strcmp("-noram",argv[parm]) == 0)
        {
            op.op_RAM           = FALSE;
            op.op_ValidOptions |= SYSLOAD_OPTF_RAM;
        }
        else if (strcmp("-quit",argv[parm]) == 0)
        {
            op.op_Quit = TRUE;
        }
    }

    /* Now that the options are parsed, we need to figure out what to do
     * with them.
     *
     * This code checks to see if another version of SysLoad is already running.
     * If it is already running, then a message is sent to it with the
     * new options. If SysLoad is not already running, we initialize
     * the universe, and jump into the metering loop.
     */

    optPort = FindMsgPort(SYSLOAD_PORTNAME);
    if (optPort >= 0)
    {
        repPort = CreateMsgPort(NULL,0,0);
        if (repPort >= 0)
        {
            msg = CreateMsg(NULL,0,repPort);
            if (msg >= 0)
            {
                err = SendMsg(optPort,msg,&op,sizeof(OptionPacket));
                if (err >= 0)
                {
                    WaitPort(repPort,msg);
                }
                else
                {
                    Error("Unable to send a message: ",err);
                }
            }
            else
            {
                Error("Unable to create a message: ",msg);
            }
            DeleteMsg(msg);
        }
        else
        {
            Error("Unable to create a message port: ",repPort);
        }
        DeleteMsgPort(repPort);
    }
    else
    {
        err = OpenAudioFolio();
        if (err >= 0)
        {
            err = OpenGraphicsFolio();
            if (err >= 0)
            {
                err = ResetCurrentFont();
                if (err >= 0)
                {
                    ccb = (CCB *)AllocMem(sizeof(CCB),MEMTYPE_CEL);
                    if (ccb)
                    {
                        timer = OpenNamedDevice("timer",NULL);
                        if (timer >= 0)
                        {
                            timerIO = CreateIOReq(NULL,0,timer,0);
                            if (timerIO >= 0)
                            {
                                optPort = CreateMsgPort(SYSLOAD_PORTNAME,0,0);
                                if (optPort >= 0)
                                {
                                    busyThread = CreateThread("SysLoadBusyThread",10,BusyThread,128);
                                    if (busyThread >= 0)
                                    {
                                        MeterLoop(&op);
                                        DeleteThread(busyThread);
                                    }
                                    else
                                    {
                                        Error("Unable to create busy thread: ",busyThread);
                                    }
                                    DeleteMsgPort(optPort);
                                }
                                else
                                {
                                    Error("Unable to create message port: ",optPort);
                                }
                                DeleteIOReq(timerIO);
                            }
                            else
                            {
                                Error("Unable to create timer IO request: ",timerIO);
                            }
                            CloseNamedDevice(timer);
                        }
                        else
                        {
                            Error("Unable to open timer device: ",timer);
                        }
                        FreeMem(ccb,sizeof(CCB));
                    }
                }
                CloseGraphicsFolio();
            }
            CloseAudioFolio();
        }
    }

    return (0);
}
