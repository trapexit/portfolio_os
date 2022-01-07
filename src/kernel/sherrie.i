; $Id: sherrie.i,v 1.2 1994/02/09 02:04:35 limes Exp $
;--------------------------------------------------
;
; File:		Sherrie.i
; By:		Stephen H. Landrum
; Last update:	24-Apr-92
;
; Copyright (c) 1992, New Technologies Group, Inc.
;
; This document is proprietary and confidential
;
;--------------------------------------------------

 IF :DEF:|__sherrie_i|
 ELSE
	GBLL	|__sherrie_i|


; /* 26-bit ARM processor modes */
ARM_MODE_USR26	EQU &00000000
ARM_MODE_FIQ26	EQU &00000001
ARM_MODE_IRQ26	EQU &00000002
ARM_MODE_SVC26	EQU &00000003

; /* 32-bit ARM processor modes */
ARM_MODE_USR	EQU &00000010
ARM_MODE_FIQ	EQU &00000011
ARM_MODE_IRQ	EQU &00000012
ARM_MODE_SVC	EQU &00000013
ARM_MODE_ABT	EQU &00000017
ARM_MODE_UND	EQU &0000001b


; /* 26-bit PSR bit locations */
ARM_MODE_MASK26	EQU &00000003
ARM_F_BIT26	EQU &04000000
ARM_I_BIT26	EQU &08000000
ARM_V_BIT26	EQU &10000000
ARM_C_BIT26	EQU &20000000
ARM_Z_BIT26	EQU &40000000
ARM_N_BIT26	EQU &80000000

; /* 32-bit PSR bit locations */
ARM_MODE_MASK	EQU &0000001f
ARM_F_BIT	EQU &00000040
ARM_I_BIT	EQU &00000080
ARM_V_BIT	EQU &10000000
ARM_C_BIT	EQU &20000000
ARM_Z_BIT	EQU &40000000
ARM_N_BIT	EQU &80000000


; /* ARM Exception vectors */
ARM_RSTV	EQU &00000000
ARM_UNDV	EQU &00000004
ARM_SWIV	EQU &00000008
ARM_PREABTV	EQU &0000000C
ARM_DATABTV	EQU &00000010
ARM_ADRV	EQU &00000014
ARM_IRQV	EQU &00000018
ARM_FIQV	EQU &0000001C



; /* addresses of memory ranges */
VRAM		EQU &00000000
ROM		EQU &03000000
SLOWBUS		EQU &03100000
SPORT		EQU &03200000
MADAM		EQU &03300000
CLIO		EQU &03400000
Other1		EQU &03404000
Other2		EQU &03408000
UNCLE		EQU &0340c000
TRACEW		EQU &03600000
TRACE53		EQU &03700000
TRACEBIGRAM	EQU &03800000

SRAM		EQU TRACE53


; /* MADAM addresses */
MADAMREV	EQU MADAM+&0000
MSYSBits	EQU MADAM+&0004

MultiCHIP	EQU MADAM+&0010

AbortBits	EQU MADAM+&0020
PrivBits	EQU MADAM+&0024
STATBits	EQU MADAM+&0028

MADAMDIAG0	EQU MADAM+&0040
MADAMDIAG1	EQU MADAM+&0044

SPRSTRT		EQU MADAM+&0100
SPRSTOP		EQU MADAM+&0104
SPRCNTU		EQU MADAM+&0108
SPRPAUS		EQU MADAM+&010c
SCOBCTL0	EQU MADAM+&0110

PPMPC		EQU MADAM+&0120

REGCTL0		EQU MADAM+&0130
REGCTL1		EQU MADAM+&0134
REGCTL2		EQU MADAM+&0138
REGCTL3		EQU MADAM+&013c
XYPOSL		EQU MADAM+&0140
XYPOSH		EQU MADAM+&0144
DXYL		EQU MADAM+&0148
DXYH		EQU MADAM+&014c
LINEDXYL	EQU MADAM+&0150
LINEDXYH	EQU MADAM+&0154
DDXYL		EQU MADAM+&0158
DDXYH		EQU MADAM+&015c


PIPSTACK	EQU MADAM+&0180

; /* read only addresses for the PIP */
PIP0L		EQU MADAM+&0180
PIP0R		EQU MADAM+&0184
PIP1L		EQU MADAM+&0188
PIP1R		EQU MADAM+&018c
PIP2L		EQU MADAM+&0190
PIP2R		EQU MADAM+&0194
PIP3L		EQU MADAM+&0198
PIP3R		EQU MADAM+&019c
PIP4L		EQU MADAM+&01a0
PIP4R		EQU MADAM+&01a4
PIP5L		EQU MADAM+&01a8
PIP5R		EQU MADAM+&01ac
PIP6L		EQU MADAM+&01b0
PIP6R		EQU MADAM+&01b4
PIP7L		EQU MADAM+&01b8
PIP7R		EQU MADAM+&01bc
PIP8L		EQU MADAM+&01c0
PIP8R		EQU MADAM+&01c4
PIP9L		EQU MADAM+&01c8
PIP9R		EQU MADAM+&01cc
PIPAL		EQU MADAM+&01d0
PIPAR		EQU MADAM+&01d4
PIPBL		EQU MADAM+&01d8
PIPBR		EQU MADAM+&01dc
PIPCL		EQU MADAM+&01e0
PIPCR		EQU MADAM+&01e4
PIPDL		EQU MADAM+&01e8
PIPDR		EQU MADAM+&01ec
PIPEL		EQU MADAM+&01f0
PIPER		EQU MADAM+&01f4
PIPFL		EQU MADAM+&01f8
PIPFR		EQU MADAM+&01fc

; /* write only addresses for the PIP */

PIP0		EQU MADAM+&0180
PIP1		EQU MADAM+&0184
PIP2		EQU MADAM+&0188
PIP3		EQU MADAM+&018c
PIP4		EQU MADAM+&0190
PIP5		EQU MADAM+&0194
PIP6		EQU MADAM+&0198
PIP7		EQU MADAM+&019c
PIP8		EQU MADAM+&01a0
PIP9		EQU MADAM+&01a4
PIPA		EQU MADAM+&01a8
PIPB		EQU MADAM+&01ac
PIPC		EQU MADAM+&01b0
PIPD		EQU MADAM+&01b4
PIPE		EQU MADAM+&01b8
PIPF		EQU MADAM+&01bc


FENCESTACK	EQU MADAM+&0200

; /* FIFO read only addresses */
FENCE0L		EQU MADAM+&0230
FENCE0R		EQU MADAM+&0234
FENCE1L		EQU MADAM+&0238
FENCE1R		EQU MADAM+&023c

FENCE2L		EQU MADAM+&0270
FENCE2R		EQU MADAM+&0274
FENCE3L		EQU MADAM+&0278
FENCE3R		EQU MADAM+&027c

; /* FIFO write only addresses */
FENCE0		EQU MADAM+&0218
FENCE1		EQU MADAM+&021c

FENCE2		EQU MADAM+&0238
FENCE3		EQU MADAM+&023c


MMU		EQU MADAM+&0300


DMASTACK	EQU MADAM+&0400

; /* DMA Stack register offsets */
DMAAddress	EQU 0
DMALength	EQU 1
DMANextAddress	EQU 2
DMANextLength	EQU 3

; /* DMA Stack registers */
RAMtoDSPP0	EQU MADAM+&0400
RAMtoDSPP1	EQU MADAM+&0410
RAMtoDSPP2	EQU MADAM+&0420
RAMtoDSPP3	EQU MADAM+&0430
RAMtoDSPP4	EQU MADAM+&0440
RAMtoDSPP5	EQU MADAM+&0450
RAMtoDSPP6	EQU MADAM+&0460
RAMtoDSPP7	EQU MADAM+&0470
RAMtoDSPP8	EQU MADAM+&0480
RAMtoDSPP9	EQU MADAM+&0490
RAMtoDSPP10	EQU MADAM+&04a0
RAMtoDSPP11	EQU MADAM+&04b0
RAMtoDSPP12	EQU MADAM+&04c0
DSPPtoRAM0	EQU MADAM+&04d0
DSPPtoRAM1	EQU MADAM+&04e0
DSPPtoRAM2	EQU MADAM+&04f0
DSPPtoRAM3	EQU MADAM+&0500
RAMtoDSPPN	EQU MADAM+&0510
DMAtoEXP0	EQU MADAM+&0520
DMAfromEXP0	EQU MADAM+&0520		; *note that this is the same as DMAtoEXP0
RAMtoUncle	EQU MADAM+&0530
UncletoRAM	EQU MADAM+&0540
RAMtoEXTERNAL	EQU MADAM+&0550
EXTERNALtoRAM	EQU MADAM+&0560
RAMtoPLAYER	EQU MADAM+&0570
RAMfromPLAYER	EQU MADAM+&0570		; *note that this is the same as RAMtoPLAYER

CLUTMIDctl	EQU MADAM+&0580
CLUTMIDvideo	EQU MADAM+&0584
CLUTMIDmidline	EQU MADAM+&0588

VIDMIDprev	EQU MADAM+&0590
VIDMIDcur	EQU MADAM+&0594
VIDMIDprevmid	EQU MADAM+&0598
VIDMIDcurmid	EQU MADAM+&059c
SpriteCtl0	EQU MADAM+&05a0
SpriteCtl1	EQU MADAM+&05a4
SpriteCtlPIP	EQU MADAM+&05a8
SpriteCtlData	EQU MADAM+&05ac
SpriteAddrA	EQU MADAM+&05b0
SpriteLenA	EQU MADAM+&05b4
SpriteAddrB	EQU MADAM+&05b8
SpriteLenB	EQU MADAM+&05bc

CommandGrabber	EQU MADAM+&05c0
FrameGrabber	EQU MADAM+&05d0




; /* CLIO Registers */
CLIOREV		EQU CLIO+&0000
CSYSBits	EQU CLIO+&0004
Vint0		EQU CLIO+&0008
Vint1		EQU CLIO+&000c

AUDIN		EQU CLIO+&0020
AUDOUT		EQU CLIO+&0024
CSTATBits	EQU CLIO+&0028
WDOG		EQU CLIO+&002c
HCNT		EQU CLIO+&0030
VCNT		EQU CLIO+&0034
SEED		EQU CLIO+&0038

SetInt0Bits	EQU CLIO+&0040
ClrInt0Bits	EQU CLIO+&0044
SetInt0EnBits	EQU CLIO+&0048
ClrInt0EnBits	EQU CLIO+&004c
SetMode		EQU CLIO+&0050
ClrMode		EQU CLIO+&0054
BadBits		EQU CLIO+&0058

SetInt1Bits	EQU CLIO+&0060
ClrInt1Bits	EQU CLIO+&0064
SetInt1EnBits	EQU CLIO+&0068
ClrInt1EnBits	EQU CLIO+&006c

EXPCtl		EQU CLIO+&0080

Timer0		EQU CLIO+&0100
Timer0Back	EQU CLIO+&0104
Timer1		EQU CLIO+&0108
Timer1Back	EQU CLIO+&010c
Timer2		EQU CLIO+&0110
Timer2Back	EQU CLIO+&0114
Timer3		EQU CLIO+&0118
Timer3Back	EQU CLIO+&011c
Timer4		EQU CLIO+&0120
Timer4Back	EQU CLIO+&0124
Timer5		EQU CLIO+&0128
Timer5Back	EQU CLIO+&012c
Timer6		EQU CLIO+&0130
Timer6Back	EQU CLIO+&0134
Timer7		EQU CLIO+&0138
Timer7Back	EQU CLIO+&013c
Timer8		EQU CLIO+&0140
Timer8Back	EQU CLIO+&0144
Timer9		EQU CLIO+&0148
Timer9Back	EQU CLIO+&014c
Timer10		EQU CLIO+&0150
Timer10Back	EQU CLIO+&0154
Timer11		EQU CLIO+&0158
Timer11Back	EQU CLIO+&015c
Timer12		EQU CLIO+&0160
Timer12Back	EQU CLIO+&0164
Timer13		EQU CLIO+&0168
Timer13Back	EQU CLIO+&016c
Timer14		EQU CLIO+&0170
Timer14Back	EQU CLIO+&0174
Timer15		EQU CLIO+&0178
Timer15Back	EQU CLIO+&017c

SetTm0		EQU CLIO+&0200
ClrTm0		EQU CLIO+&0204
SetTm1		EQU CLIO+&0208
ClrTm1		EQU CLIO+&020c

FIFOINIT	EQU CLIO+&0300
DMAREQEN	EQU CLIO+&0304
DMAREQDIS	EQU CLIO+&0308

FIFOstatus	EQU CLIO+&0380

SEMAPHORE	EQU CLIO+&17d0
SEMAACK		EQU CLIO+&17d4

DSPPDMA		EQU CLIO+&17e0
DSPPRST0	EQU CLIO+&17e4
DSPPRST1	EQU CLIO+&17e8

DSPPPC		EQU CLIO+&17f4
DSPPNR		EQU CLIO+&17f8
DSPPGW		EQU CLIO+&17fc

DSPPN32		EQU CLIO+&1800
DSPPN16		EQU CLIO+&2000
DSPPEI32	EQU CLIO+&3000
DSPPEI16	EQU CLIO+&3400
DSPPEO32	EQU CLIO+&3800
DSPPEO16	EQU CLIO+&3c00



; /* Other-1 registers */
Other1REV	EQU Other1+&0000
Other1Bits	EQU Other1+&0004



; /* Other-2 registers */
Other2REV	EQU Other2+&0000
Other2Bits	EQU Other2+&0004



; /* UNCLE registers */
UNCLEREV	EQU UNCLE+&0000
USYSBits	EQU UNCLE+&0004





; /* Bit definitions for various registers */

; /* for all hardware rev. registers */
HardID_MASK	EQU &ff000000
MADAM_ID	EQU &01000000
CLIO_ID		EQU &02000000
UNCLE_ID	EQU &03000000

HardID_SHFT	EQU 24

; /* for all software rev. registers */
SoftRev_MASK	EQU &ff000000

SoftRev_SHFT	EQU 24

; /* MSYSBits */
VRAMSIZE_MASK	EQU &00000007
SYSRAMSIZE_MASK	EQU &00000018
DISPMOD_MASK	EQU &00000030
CLUTXEN		EQU &00000040
PLAYXEN		EQU &00000080
BIST_MASK	EQU &00000700
CPUVEN		EQU &00000800
;SoftRev_MASK	EQU &ff000000

VRAMSIZE_SHFT	EQU 0
SYSRAMSIZE_SHFT	EQU 3
DISPMOD_SHFT	EQU 5
BIST_SHFT	EQU 9
;SoftRev_SHFT	EQU 24


; /* AbortBits */
ABT_ROMF	EQU &00000001
ABT_ROMW	EQU &00000002
ABT_CLIOT	EQU &00000004
ABT_HARDU	EQU &00000008
ABT_SYSRAMU	EQU &00000010
ABT_FENCEV	EQU &00000020
ABT_VPR		EQU &00000040
ABT_R26E	EQU &00000080
ABT_SPSC	EQU &00000100
ABT_BITE	EQU &00000200
ABT_BADDEC	EQU &00000400

; /* PrivBits */
PRIV_DMAtoSYSRAM	EQU &00000001
PRIV_SPORTtoSYSRAM	EQU &00000002
PRIV_REGIStoSYSRAM	EQU &00000004
PRIV_DMA_VRAMSIZE	EQU &00000008
PRIV_SPORT_VRAMSIZE	EQU &00000010
PRIV_REGIS_VRAMSIZE	EQU &00000020
PRIV_REGIS_MATH		EQU &00000040


; /* STATBits */
DIAGRESTART	EQU &00000001
DIPIRRESTART	EQU &00000002
BIST_STAT_MASK	EQU &0000000c
SPRON		EQU &00000010
SPRPAU		EQU &00000020
SPREND		EQU &00000040
SPRPRQ		EQU &00000080

BIST_STAT_SHFT	EQU 2

; /* REGCTL0 */
G1_RMOD32	EQU &00000001
G1_RMOD256	EQU &00000004
G2_RMOD64	EQU &00000010
G2_RMOD128	EQU &00000020
G2_RMOD256	EQU &00000040
G1_WMOD32	EQU &00000100
G1_WMOD256	EQU &00000400
G2_WMOD64	EQU &00001000
G2_WMOD128	EQU &00002000
G2_WMOD256	EQU &00004000

; /* REGCTL1 */
REG_XCLIP_MASK	EQU &000007ff
REG_YCLIP_MASK	EQU &07ff0000

REG_XCLIP_SHFT	EQU 0
REG_YCLIP_SHFT	EQU 16


; /* AUDIN */
AUDIN_FORMAT	EQU &00000001
AUDIN_ENABLE	EQU &00000002

; /* AUDOUT */
AUDOUT_ENABLE	EQU &80000000
AUDOUT_LEFTHIGH	EQU &40000000
AUDOUT_POSHIGH	EQU &20000000
AUDOUT_BITCLKR_MASK	EQU &000f0000
AUDOUT_WRDSTRT_MASK	EQU &0000ff00
AUDOUT_WRDREL_MASK	EQU &000000ff

AUDOUT_BITCLKR_SHFT	EQU 16
AUDOUT_WRDSTRT_SHFT	EQU 8
AUDOUT_WRDREL_SHFT	EQU 0

; /* CSTATbits */
CSTAT_PON	EQU &00000001
CSTAT_ROLLOVER	EQU &00000002
CSTAT_WDOGpin	EQU &00000004
CSTAT_WDOGcntr	EQU &00000008
CSTAT_BIST_MASK	EQU &00000e00

CSTAT_BIST_SHFT	EQU 9

; /* VCNT */
VCNT_MASK	EQU &000001ff
FIELD		EQU &00000200

VCNT_SHFT	EQU 0

; /* SEED */
SEED_MSB_MASK	EQU &0fff0000
SEED_LSB_MASK	EQU &00000fff

SEED_MSB_SHFT	EQU 16
SEED_LSB_SHFT	EQU 0

; /* Interrupt word 0 */
INT_VINT0	EQU &00000001
INT_VINT1	EQU &00000002
INT_EXINT	EQU &00000004
INT_TIMINT15	EQU &00000008
INT_TIMINT13	EQU &00000010
INT_TIMINT11	EQU &00000020
INT_TIMINT9	EQU &00000040
INT_TIMINT7	EQU &00000080
INT_TIMINT5	EQU &00000100
INT_TIMINT3	EQU &00000200
INT_TIMINT1	EQU &00000400
INT_DSPPINT	EQU &00000800
INT_DDRINT0	EQU &00001000
INT_DDRINT1	EQU &00002000
INT_DDRINT2	EQU &00004000
INT_DDRINT3	EQU &00008000
INT_DRDINT0	EQU &00010000
INT_DRDINT1	EQU &00020000
INT_DRDINT2	EQU &00040000
INT_DRDINT3	EQU &00080000
INT_DRDINT4	EQU &00100000
INT_DRDINT5	EQU &00200000
INT_DRDINT6	EQU &00400000
INT_DRDINT7	EQU &00800000
INT_DRDINT8	EQU &01000000
INT_DRDINT9	EQU &02000000
INT_DRDINT10	EQU &04000000
INT_DRDINT11	EQU &08000000
INT_DRDINT12	EQU &10000000
INT_DEXINT	EQU &20000000
INT_SCNDPINT	EQU &40000000

; /* Interrupt word 1 */
INT_PLYINT	EQU &00000001
INT_DIPIR	EQU &00000002
INT_PDINT	EQU &00000004
INT_RAMtoDSPPN	EQU &00000008
INT_DMAtoUNCLE	EQU &00000010
INT_DMAfrUNCLE	EQU &00000020
INT_DMAtoEXTERN	EQU &00000040
INT_DMAfrEXTERN	EQU &00000080
INT_BadBits	EQU &00000100

; /* BadBits */
BB_Underfl_MASK	EQU &00007fff
BB_Overfl_MASK	EQU &000f0000
BB_Priv		EQU &00100000
BB_DMANFW	EQU &00200000
BB_DMANFW_MASK	EQU &0fc00000

BB_Underfl_SHFT	EQU 0
BB_Overfl_SHFT	EQU 16
BB_DMANFW_SHFT	EQU 22

; /* FIFO status */
FIFO_Count0	EQU &00000001
FIFO_Count1	EQU &00000002
FIFO_Count2	EQU &00000004
FIFO_OverUnder	EQU &00000008
FIFO_FullEmpty	EQU &00000010
FIFO_Flush	EQU &00000020


; /* CLUT DMA control */
CLUT_LINE_MASK	EQU &000001ff
CLUT_LEN_MASK	EQU &00007e00
CLUT_LDPREV	EQU &00008000
CLUT_LDCUR	EQU &00010000
CLUT_PREVSEL	EQU &00020000
CLUT_RELSEL	EQU &00040000
CLUT_480RES	EQU &00080000
CLUT_DMAchannel	EQU &00100000
CLUT_ENVIDDMA	EQU &00200000
CLUT_SLIPEN	EQU &00400000

CLUT_LINE_SHFT	EQU 0
CLUT_LEN_SHFT	EQU 9

; /* CLUT Palette data */
CLUT_B_MASK	EQU &000000ff
CLUT_G_MASK	EQU &0000ff00
CLUT_R_MASK	EQU &00ff0000
CLUT_PEN_MASK	EQU &1f000000
CLUT_BLUEONLY	EQU &20000000
CLUT_GREENONLY	EQU &40000000
CLUT_REDONLY	EQU &60000000
CLUT_CONTROL	EQU &80000000

CLUT_B_SHFT	EQU 0
CLUT_G_SHFT	EQU 8
CLUT_R_SHFT	EQU 16


; /* CLUT display control word */
CLUT_VINTDIS	EQU &00000001

CLUT_HINTEN	EQU &00000004
CLUT_VINTEN	EQU &00000008
CLUT_G2BEN	EQU &00000010
CLUT_B2BEN	EQU &00000020
CLUT_HSUBPOS	EQU &00000040
CLUT_HSUBSEL	EQU &00000080
CLUT_VSUBPOS	EQU &00000100
CLUT_VSUBSEL	EQU &00000200
CLUT_SWAPHV	EQU &00000400
CLUT_WINEN	EQU &00000800
CLUT_RANDOMEN	EQU &00001000
CLUT_WINHINTEN	EQU &00002000
CLUT_WINVINTEN	EQU &00004000
CLUT_WING2BEN	EQU &00008000
CLUT_WINB2BEN	EQU &00010000
CLUT_WINHSUBPOS	EQU &00020000
CLUT_WINHSUBSEL	EQU &00040000
CLUT_WINVSUBPOS	EQU &00080000
CLUT_WINVSUBSEL	EQU &00100000
CLUT_WINSWAPHV	EQU &00200000
CLUT_BACKTRANS	EQU &00400000
CLUT_FORCETRANS	EQU &00800000
CLUT_OVERLAYSEL	EQU &01000000
CLUT_CLUTBYPASS	EQU &02000000

CLUT_PALSEL	EQU &08000000
CLUT_NULLAMY	EQU &10000000
CLUT_BACKGROUND	EQU &20000000
CLUT_DISPCTRL	EQU &c0000000

; /* AMY control word */
CLUT_AMYCTRL	EQU &80000000

; /* Special CLUT 'NOP' */
CLUT_NULLCLUT	EQU &e1000000
CLUT_AMYNULL	EQU CLUT_AMYCTRL+0


; /* SCB control word bits */
SCB_SKIP	EQU &80000000
SCB_LAST	EQU &40000000
SCB_NPABS	EQU &20000000
SCB_SPABS	EQU &10000000
SCB_PPABS	EQU &08000000
SCB_LDSIZE	EQU &04000000
SCB_LDPRS	EQU &02000000
SCB_LDPPMP	EQU &01000000
SCB_LDPIP	EQU &00800000
SCB_SCBPRE	EQU &00400000
SCB_YOXY	EQU &00200000

SCB_ACW		EQU &00040000
SCB_ACCW	EQU &00020000
SCB_TWD		EQU &00010000
SCB_LCE		EQU &00008000
SCB_ACE		EQU &00004000

SCB_PACKED	EQU &00000200
SCB_DOVER_MASK	EQU &00000180
SCB_PIPPOS	EQU &00000040
SCB_BGND	EQU &00000020
SCB_NOBLK	EQU &00000010
SCB_PIPA_MASK	EQU &0000000f

SCB_DOVER_SHFT	EQU 7
SCB_PIPA_SHFT	EQU 0

; /* Sprite first preamble word bits */
PRE0_VCNT_MASK	EQU &0000ffc0
PRE0_LINEAR	EQU &00000010
PRE0_REP8	EQU &00000008
PRE0_BPP_MASK	EQU &00000007

PRE0_VCNT_SHFT	EQU 6
PRE0_BPP_SHFT	EQU 0

PRE0_BPP_1	EQU &00000001
PRE0_BPP_2	EQU &00000002
PRE0_BPP_4	EQU &00000003
PRE0_BPP_6	EQU &00000004
PRE0_BPP_8	EQU &00000005
PRE0_BPP_16	EQU &00000006

; /* Sprite second preamble word bits */
PRE1_WOFFSET8_MASK	EQU &ff000000
PRE1_WOFFSET10_MASK	EQU &03ff0000
PRE1_NOSWAP	EQU &00004000
PRE1_TLLSB_MASK	EQU &00003000
PRE1_LRFORM	EQU &00000800
PRE1_TLHPCNT_MASK	EQU &000007ff

PRE1_WOFFSET8_SHFT	EQU 24
PRE1_WOFFSET10_SHFT	EQU 16
PRE1_TLLSB_SHFT		EQU 12
PRE1_TLHPCNT_SHFT	EQU 0

PRE1_TLLSB_0	EQU &00000000
PRE1_TLLSB_IPN0	EQU &00001000
PRE1_TLLSB_IPN4	EQU &00002000
PRE1_TLLSB_IPN5	EQU &00003000


 ENDIF


		END



