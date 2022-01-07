/* $Id: dspp_loader.c,v 1.34.1.2 1995/01/27 02:56:08 phil Exp $ */
/****************************************************************
**
** DSPP Instrument Loader
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
** 930617 PLB Use afi_DeleteLinkedItems in DSPPFreeInstrument
** 930704 PLB Free resources if allocation fails in DSPPLoadPatch
** 930817 PLB Free memory for unrecognised chunk.
** 930824 PLB Added support for ID_DHDR
** 930824 PLB Only count external references to allow ADPCM to be freed.
** 930908 PLB Init I Mem to zero when allocated to reduce pops.
** 931215 PLB Make Knob and Port name matching case insensitive.
** 940224 PLB Move Attachment list to aitp to prepare for shared dtmp.
** 940502 PLB Add support for audio input.
** 940609 PLB Added shared library template support. 
** 940811 PLB Used %.4s to print ChunkTypes instead of scratch array kludge.
** 940812 PLB Added half rate calculation:
**            Allocate Ticks from even or odd frame.
**            DSPPJumpTo() handles jump from execsplit.dsp to odd frame list.
** 940912 PLB Use Read16 to read uint16 values for NuPup
** 941102 PLB Fix bad error cleanup in DSPPLoadPatch
** 950124 WJB Fixed memory list corruption under low memory problem in DSPPCloneTemplate(). Fixes CR 4196.
** 950126 PLB Moved overwrite of SLEEP to DSPPStartCodeExecution() in dspp_instr.c
****************************************************************/

#include "audio_internal.h"
#include "touch_hardware.h"


#define DBUG(x)      /* PRT(x) */

#define DBUGRSRC(x)  DBUG(x)

/*****************************************************************/
/*** Static prototypes *******************************************/
/*****************************************************************/
static int32 DSPPAllocateTicks( DSPPInstrument *dins, DSPPResource *drsc, uint32 *AllocatedPtr );
static void DSPPFreeTicks( DSPPInstrument *dins, DSPPResource *drsc );

/*****************************************************************/
/*** Called from IFF Parser **************************************/
/*****************************************************************/
int32 DSPPHandleInsChunk ( iff_control *iffc, int32 ChunkType , int32 ChunkSize )
{
	void *tmp;
	DSPPKnob *dknb;
	DSPPTemplate *dtmp;
	int32 num;
	int32 Result=0;
	
	dtmp = (DSPPTemplate *) iffc->iffc_UserContext;
	
DBUG(("DSPPHandleInsChunks: %.4s, %d\n",  &ChunkType, ChunkSize));

	tmp = (void *) EZMemAlloc(ChunkSize+8, MEMTYPE_FILL);
	if (tmp == 0)
	{
		Result = AF_ERR_NOMEM;
		goto error;
	}
	
	Result = iffReadChunkData(iffc, tmp, ChunkSize);
	CHECKRSLT(("Error reading CHUNK data = %d\n", Result));

	switch(ChunkType)
	{
		case ID_DRSC:
			dtmp->dtmp_Resources = (DSPPResource *) tmp;
			dtmp->dtmp_NumResources = ChunkSize / sizeof(DSPPResource);
			break;
				
		case ID_DRLC:
			dtmp->dtmp_Relocations = (DSPPRelocation *) tmp;
			dtmp->dtmp_NumRelocations = ChunkSize / sizeof(DSPPRelocation);
			break;
				
		case ID_DKNB:
			dtmp->dtmp_Knobs = (DSPPKnob *) tmp;
/* Count knobs */
			dknb = dtmp->dtmp_Knobs;
			num = 0;
			do
			{
				if(dknb->dknb_Next > ChunkSize)
				{
					Result = AF_ERR_BADOFX;
					goto error;
				}
				num++;
			} while ((dknb = DSPPNextKnob(dtmp->dtmp_Knobs, dknb)) != NULL);
			dtmp->dtmp_NumKnobs = num;
TRACEB(TRACE_INT,TRACE_OFX,("DSPPHandleInsChunks: %d knobs loaded.\n", num));
			break;
				
		case ID_DNMS:
			dtmp->dtmp_ResourceNames = (char *) tmp;
			break;
				
		case ID_DCOD:
			dtmp->dtmp_Codes = (DSPPCodeHeader *) tmp;
			dtmp->dtmp_CodeSize = ChunkSize;
			break;
			
		case ID_DINI:
			dtmp->dtmp_DataInitializer = (DSPPDataInitializer *) tmp;
			dtmp->dtmp_DataInitializerSize = ChunkSize;
			break;
				
		case ID_DHDR:
			DBUG(("DHDR chunk, function ID = %d\n", ((DSPPHeader *)tmp)->dhdr_FunctionID ));
			dtmp->dtmp_FunctionID =  ((DSPPHeader *)tmp)->dhdr_FunctionID; /* 930825 */
			EZMemFree(tmp);  /* 930824 */
			break;
			
		default:
			ERR(("Unrecognised Chunk type! 0x%x\n", ChunkType));
			EZMemFree(tmp);  /* 930817 */
	}
		
TRACEB(TRACE_INT,TRACE_OFX,("DSPPHandleInsChunks: %d bytes remaining.\n", iffc->iffc_length));
	return(0);
	
error:
	StripInsTemplate(dtmp);
	return(Result);
}

/*****************************************************************/
int32 DSPPValidateTemplate( DSPPTemplate *dtmp )
{
	int32 Result, i, j;
	DSPPRelocation *drlc;
	DSPPKnob *dknb;
	DSPPKnobResource *dknr;
	
/* Validate Template structure address. */
	Result = afi_IsRamAddr( (char *) dtmp, sizeof(DSPPTemplate) );
	if(Result < 0) return Result;
	
	Result = afi_IsRamAddr( (char *)dtmp->dtmp_Resources, 1 );
	if(Result < 0) return AF_ERR_BADOFX;
	Result = afi_IsRamAddr( (char *)dtmp->dtmp_ResourceNames, 1 );
	if(Result < 0) return AF_ERR_BADOFX;
	Result = afi_IsRamAddr( (char *)dtmp->dtmp_Relocations, 1 );
	if(Result < 0) return AF_ERR_BADOFX;
	Result = afi_IsRamAddr( (char *)dtmp->dtmp_Codes, 1 );
	if(Result < 0) return AF_ERR_BADOFX;
	
	Result = DSPPVerifyDataInitializer( dtmp );
	if(Result < 0) return Result;
		
/* Verify that relocations index valid resources. */
	for( i=0; i<dtmp->dtmp_NumRelocations; i++)
	{
		drlc = &(dtmp->dtmp_Relocations[i]);
		if( drlc->drlc_RsrcIndex > dtmp->dtmp_NumResources )
		{
			ERRDBUG(("DSPPValidateTemplate: Invalid drlc_RsrcIndex = %d\n", drlc->drlc_RsrcIndex));
			return AF_ERR_BADOFX;
		}
	}
	
/* Knobs count verified in DSPPHandleInsChunk */

/* Verify that Knobs index valid resources. */
	dknb = dtmp->dtmp_Knobs;
	for( i=0; i<dtmp->dtmp_NumKnobs; i++)
	{
		dknr = dknb->dknb_Resources;
		for( j=0; j<dknb->dknb_NumResources; j++)
		{
			if( dknr->dknr_RsrcIndex > dtmp->dtmp_NumResources )
			{
				ERRDBUG(("DSPPValidateTemplate: Invalid dknr_RsrcIndex = %d\n", dknr->dknr_RsrcIndex));
				return AF_ERR_BADOFX;
			}
/* advance to next knob resource */
			dknr++;
		}
		dknb = DSPPNextKnob(dtmp->dtmp_Knobs, dknb);
	}

	return Result;
}

/*****************************************************************
** Take validated template created in user mode and make blessed clone
** for Folio use. Supervisor mode.
*****************************************************************/
DSPPTemplate *DSPPCloneTemplate (AudioInsTemplate *aitp, DSPPTemplate *UserTmp)
{
	DSPPTemplate *SuperTmp;
	uint32 size;
	
/* Allocate Supervisor mode structure */
	SuperTmp = (DSPPTemplate *)EZMemAlloc(sizeof(DSPPTemplate), MEMTYPE_FILL);
	aitp->aitp_DeviceTemplate = SuperTmp;
	if (SuperTmp == NULL)
	{
		return NULL;
	}
/* 950124: Copy non-pointers from User template to Super (@@@ must remain in sync w/ DSPPTemplate definition) */
	SuperTmp->dtmp_NumResources        = UserTmp->dtmp_NumResources;
	SuperTmp->dtmp_NumRelocations      = UserTmp->dtmp_NumRelocations;
	SuperTmp->dtmp_NumKnobs            = UserTmp->dtmp_NumKnobs;
	SuperTmp->dtmp_CodeSize            = UserTmp->dtmp_CodeSize;
	SuperTmp->dtmp_DataInitializerSize = UserTmp->dtmp_DataInitializerSize;
	SuperTmp->dtmp_FunctionID          = UserTmp->dtmp_FunctionID;

TRACEB(TRACE_INT,TRACE_OFX,("DSPPCloneTemplate: SuperTmp = $%x\n", SuperTmp ));


#define CLONEMEMBER(type,src,dst)  if (src) \
	{ \
		size = EZMemSize ( (void *) src ); \
TRACEB(TRACE_INT,TRACE_OFX,("DSPPCloneTemplate: member size = %d\n", size)); \
		dst = (type *)EZMemAlloc(size, MEMTYPE_FILL); \
		if (dst == NULL) goto nomem; \
		bcopy (src, dst, (int) size); \
	}
	
	CLONEMEMBER(DSPPResource, UserTmp->dtmp_Resources, SuperTmp->dtmp_Resources);
	CLONEMEMBER(char, UserTmp->dtmp_ResourceNames, SuperTmp->dtmp_ResourceNames);
	CLONEMEMBER(DSPPRelocation, UserTmp->dtmp_Relocations, SuperTmp->dtmp_Relocations);
	CLONEMEMBER(DSPPKnob, UserTmp->dtmp_Knobs, SuperTmp->dtmp_Knobs);
	CLONEMEMBER(DSPPCodeHeader, UserTmp->dtmp_Codes, SuperTmp->dtmp_Codes);
	CLONEMEMBER(DSPPDataInitializer, UserTmp->dtmp_DataInitializer, SuperTmp->dtmp_DataInitializer);

/* 940609 Init list to hold library templates. */
DBUG(("InitList for dtmp->dtmp_LibraryTemplateRefs, dtmp = 0x%x\n", SuperTmp ));
	InitList (&SuperTmp->dtmp_LibraryTemplateRefs, "LibraryTemplates");
	
	return SuperTmp;

nomem:
	if(SuperTmp->dtmp_Resources) EZMemFree(SuperTmp->dtmp_Resources);
	if(SuperTmp->dtmp_ResourceNames) EZMemFree(SuperTmp->dtmp_ResourceNames);
	if(SuperTmp->dtmp_Relocations) EZMemFree(SuperTmp->dtmp_Relocations);
	if(SuperTmp->dtmp_Knobs) EZMemFree(SuperTmp->dtmp_Knobs);
	if(SuperTmp->dtmp_Codes) EZMemFree(SuperTmp->dtmp_Codes);
	if(SuperTmp->dtmp_DataInitializer) EZMemFree(SuperTmp->dtmp_DataInitializer);
	EZMemFree( SuperTmp );
	aitp->aitp_DeviceTemplate = NULL;
	return NULL;
}


/******************************************************************
** After execution of FromIns, JUMP to ToIns.
** This routine has knowledge of how .dsp instruments are terminated.
** The end in _SLEEP  _NOP so we have to subtract 2 from size
** to index to where _SLEEP is.
** The _SLEEP and _NOP are setup by }ins in dspp_asm.fth
******************************************************************/
void DSPPJumpTo ( DSPPInstrument *FromIns, DSPPInstrument *ToIns )
{
	int32 FromIndex;
	
TRACEB(TRACE_INT, TRACE_OFX, ("DSPPJumpTo( $%x -> $%x)\n", FromIns, ToIns ));
DBUG(("DSPPJumpTo( $%x -> $%x)\n", FromIns, ToIns ));

	FromIndex = FromIns->dins_EntryPoint + FromIns->dins_DSPPCodeSize - 2;

	dsphWriteCodeMem( FromIndex, (DSPP_JUMP_OPCODE + ToIns->dins_EntryPoint) );
}

/******************************************************************
** After execution of Split, JUMP to ToIns.
** Patch ODD FRAME branch.
** This routine has knowledge of how splitexec.dsp instruments are terminated.
******************************************************************/
#define DSPP_BEQ_OPCODE (0xB400)
void DSPPJumpFromSplitTo ( DSPPInstrument *SplitIns, DSPPInstrument *ToIns )
{
	int32 FromIndex;
	
TRACEB(TRACE_INT, TRACE_OFX, ("DSPPJumpTo( $%x -> $%x)\n", SplitIns, ToIns ));
DBUG(("DSPPJumpFromSplitTo( $%x -> $%x)\n", SplitIns, ToIns ));

/* Subtract 3 instead of 2 to catch other branch in splitexec.dsp */
	FromIndex = SplitIns->dins_EntryPoint + SplitIns->dins_DSPPCodeSize - 3;

	dsphWriteCodeMem( FromIndex, (DSPP_BEQ_OPCODE + ToIns->dins_EntryPoint) ); /* %Q .dsp knowledgeable */
}


/*****************************************************************/
DSPPResource *DSPPFindResource( DSPPInstrument *dins, int32 RsrcType, char *Name)
{
	int32 i;
	DSPPResource *drsc, *Result = NULL, *drscarray;

	drscarray = dins->dins_Resources;
TRACEE(TRACE_INT,TRACE_OFX, ("DSPPFindResource ( dins=0x%x, Type=0x%x, %s)\n",
	dins, RsrcType,Name));
TRACEB(TRACE_INT,TRACE_OFX, ("DSPPFindResource: NumRsrc=$%x\n", dins->dins_NumResources));

	for (i=0; i<dins->dins_NumResources; i++)
	{
		drsc = &drscarray[i];
TRACEB(TRACE_INT,TRACE_OFX, ("DSPPFindResource: i=%d, Type=$%x, Alloc=$%x\n",
			i, drsc->drsc_Type, drsc->drsc_Allocated));
		
		if((RsrcType < 0) || (drsc->drsc_Type == RsrcType)) 
		{
TRACEB(TRACE_INT,TRACE_OFX, ("DSPPFindResource: == %s ?\n",
	DSPPGetRsrcName(dins,i) ));
	
			if ((Name == NULL) ||
				(!strcasecmp(Name, DSPPGetRsrcName(dins,i)))) /* 931215 */
			{
				Result = drsc;
				break;  /* found it */
			}
		}
	}
	
TRACER(TRACE_INT,TRACE_OFX, ("DSPPFindResource returns 0x%x\n", Result));
	return Result;
}

/*****************************************************************/
/* Find FIFO Control in instrument with matching name. */
FIFOControl *DSPPFindFIFO( DSPPInstrument *dins, char *Name)
{
	int32 i, ri;
	FIFOControl *fico, *Result = NULL, *ficoarray;
	
TRACEE(TRACE_INT,TRACE_OFX, ("DSPPFindFIFO ( dins=0x%x, name=0x%x)\n", dins));

	ficoarray = &dins->dins_FIFOControls[0];

	for (i=0; i<dins->dins_NumFIFOs; i++)
	{
		fico = &ficoarray[i];
		ri = fico->fico_RsrcIndex;
		
		if ((Name == NULL) ||
				(!strcasecmp(Name, DSPPGetRsrcName(dins,ri)))) /* 931215 */
		{
			Result = fico;
			break;  /* found it */
		}
	}
TRACER(TRACE_INT,TRACE_OFX, ("DSPPFindFIFO returns fico=0x%x)\n", Result));
	return Result;
}


/*****************************************************************/
/* Initialize memory for instrument for matching AT bits. */
int32 DSPPInitInsMemory( DSPPInstrument *dins, int32 AT_Mask )
{
	DSPPTemplate *dtmp;
	DSPPDataInitializer *dini;
	DSPPResource *drsc;
	int32 *DataPtr;
	int32 Result, i;
	int32 Many;
	int32 StartAddr;
	uint32 PtrLimit, PtrValue;   /* For doing arithmetic comparisons on pointers. */
	Result = 0;
	
	dtmp = dins->dins_Template;
	dini = dtmp->dtmp_DataInitializer;
	if( dini == NULL ) return 0;
	
	PtrValue = (uint32) dini;
	PtrLimit = PtrValue + dtmp->dtmp_DataInitializerSize;
	
	while(PtrValue < PtrLimit)
	{
		Many = dini->dini_Many;
		DataPtr = (int32 *) (((char *)dini) + sizeof(DSPPDataInitializer));
		if( dini->dini_Flags & AT_Mask )
		{
			drsc = &(dins->dins_Resources[dini->dini_RsrcIndex]);
			StartAddr = drsc->drsc_Allocated;
			for(i=0; i<Many; i++)
			{
TRACEB(TRACE_INT,TRACE_OFX, ("DSPPInitInsMemory: 0x%x <= 0x%x\n", StartAddr+i, *DataPtr));
				Result = dsphWriteIMem( StartAddr+i, *DataPtr++ );
				if(Result < 0)
				{
					ERRDBUG(("DSPPInitInsMemory: error = 0x%x\n", Result));
					return Result;
				}
			}
			dini = (DSPPDataInitializer *) DataPtr;  /* Next struct just past data. */
		}
		else
		{
			dini = (DSPPDataInitializer *) (DataPtr + Many);
		}
		PtrValue = (uint32) dini;
	}
	return Result;
}

/*****************************************************************/
/* Initialize memory for instrument for matching AT bits. */
int32 DSPPVerifyDataInitializer( DSPPTemplate *dtmp )
{
	DSPPDataInitializer *dini;
	DSPPResource *drsc;
	int32 *DataPtr;
	int32 Result;
	int32 Many;
	uint32 PtrLimit, PtrValue;   /* For doing arithmetic comparisons on pointers. */
	Result = 0;
	
	dini = dtmp->dtmp_DataInitializer;
	if( dini == NULL ) return 0;
	
	PtrValue = (uint32) dini;
	PtrLimit = PtrValue + dtmp->dtmp_DataInitializerSize;
	
	while(PtrValue < PtrLimit)
	{
		drsc = &(dtmp->dtmp_Resources[dini->dini_RsrcIndex]);
		if( drsc->drsc_Type != DRSC_I_MEM )
		{
			ERRDBUG(("DSPPVerifyDataInitializer: type not IMEM\n"));
			return AF_ERR_BADRSRCTYPE;
		}
				
		Many = dini->dini_Many;
		DataPtr = (int32 *) (((char *)dini) + sizeof(DSPPDataInitializer));
		DataPtr += Many;
		
		dini = (DSPPDataInitializer *) DataPtr;  /* Next struct just past data. */
		PtrValue = (uint32) dini;
	}
	if(PtrValue > PtrLimit)
	{
		ERRDBUG(("DSPPVerifyDataInitializer: dini_Many was too big.\n"));
		return AF_ERR_BADOFX;
	}
		
	return Result;
}

/*****************************************************************/

#define CODESTART(codes, indx) (codes[indx].dcod_Offset + (uchar *) codes)

/*****************************************************************/
int32 DSPPLoadPatch( DSPPTemplate *dtmp, DSPPInstrument **DInsPtr, int32 RateShift)
{
	DSPPInstrument *dins = NULL;
	DSPPCodeHeader *Codes = NULL;
	DSPPResource *RsrcPtr = NULL, *drsc;
	FIFOControl *fico;
	int32 CodeSize, RsrcSize, NumFIFOs;
	int32 i, Result;
	uint16 *Image;
	int32 NumAllocated; /* 930704 */
	
TRACEB(TRACE_INT,TRACE_OFX,("DSPPLoadPatch: >>>>>>>>>>>>>>>>>>>>>>>>>\n"));
TRACEB(TRACE_INT,TRACE_OFX,("DSPPLoadPatch: dtmp = $%x\n", dtmp));

	*DInsPtr = NULL;
	NumAllocated = 0;
	
/* Allocate Instrument structure. */
	dins = (DSPPInstrument *) EZMemAlloc(sizeof(DSPPInstrument)+8,MEMTYPE_FILL);
	if (dins == NULL)
	{
		ERR(("DSPPLoadPatch: could not allocate memory.\n"));
		Result = AF_ERR_NOMEM;
		goto error;
	}
	dins->dins_Template = dtmp;
	dins->dins_RateShift = RateShift; /* 940812 */
	
TRACEB(TRACE_INT,TRACE_OFX,("DSPPLoadPatch: dins = $%x\n", dins));

/* Allocate private copy of resource requests. */
	RsrcSize = dtmp->dtmp_NumResources * sizeof(DSPPResource);
	RsrcPtr = (DSPPResource *) EZMemAlloc(RsrcSize+8,0);
	if (RsrcPtr == NULL)
	{
		ERR(("DSPPLoadPatch: could not allocate RsrcPtr, size = $%x\n", RsrcSize));
		Result = AF_ERR_NOMEM;
		goto error;
	}
	bcopy(dtmp->dtmp_Resources, RsrcPtr, (int) RsrcSize);
	dins->dins_NumResources = dtmp->dtmp_NumResources;
	dins->dins_Resources = RsrcPtr;
	
/* Allocate desired resources. */
	for (i=0; i<dtmp->dtmp_NumResources; i++)
	{
		Result = DSPPAllocateResource(dins,i);
		if (Result != 0) goto error;
		NumAllocated++;
	}
	
/* Scan resources. */
	NumFIFOs = 0;
	for (i=0; i<dtmp->dtmp_NumResources; i++)
	{
		drsc = &RsrcPtr[i];
		
/* Relocate any N_MEM with EXTERNAL and MANY=0 */
		if ((drsc->drsc_Many == 0) && ((drsc->drsc_Type & 0xFF) == DRSC_N_MEM))
		{
TRACEB(TRACE_INT,TRACE_OFX,("DSPPLoadPatch: Relocate external, $%x + $%x\n",
	drsc->drsc_Allocated, dins->dins_EntryPoint));
			drsc->drsc_Allocated += dins->dins_EntryPoint;
		}
		
/* Count number of FIFOs so we know how many controllers to allocate. */
		if (((drsc->drsc_Type & 0xFF) == DRSC_IN_FIFO) ||
		    ((drsc->drsc_Type & 0xFF) == DRSC_OUT_FIFO))       NumFIFOs++;
	}
	
/* Allocate FIFO Controllers */
	if (NumFIFOs)
	{
		dins->dins_FIFOControls = (FIFOControl *) EZMemAlloc( NumFIFOs * sizeof(FIFOControl), MEMTYPE_FILL);
		if (dins->dins_FIFOControls == NULL)
		{
			Result = AF_ERR_NOMEM;
			goto error;
		}
		dins->dins_NumFIFOs = NumFIFOs;
		for (i=0; i<NumFIFOs; i++)
		{
			InitList(&dins->dins_FIFOControls[i].fico_Attachments,"Attachments");
		}
					
/* Scan resources again to fill in FIFO control indices */
		fico = &dins->dins_FIFOControls[0];
		for (i=0; i<dtmp->dtmp_NumResources; i++)
		{
			drsc = &RsrcPtr[i];
			if (((drsc->drsc_Type & 0xFF) == DRSC_IN_FIFO) ||
		    	((drsc->drsc_Type & 0xFF) == DRSC_OUT_FIFO))
			{
TRACEB(TRACE_INT,TRACE_OFX,("DSPPLoadPatch: FIFO Index = %d\n", i));
				(fico++)->fico_RsrcIndex = i;
			}
			
		}
	}

/* Allocate Code image to perform relocations on. */
	CodeSize = dtmp->dtmp_CodeSize;
	Codes = (DSPPCodeHeader *) EZMemAlloc(CodeSize+8,0);
	if (Codes == NULL)
	{
		Result = AF_ERR_NOMEM;
		goto error;
	}
	bcopy(dtmp->dtmp_Codes, Codes, (int) CodeSize);

/* Perform relocations based on RLOC chunk. */
	Result = DSPPRelocateAll(dins, Codes);
	if (Result != 0) goto error;
	
	Image = (uint16 *) CODESTART(Codes,0); /* %Q just index=0 */
	
	
/* Patch in JUMP at beginning to hang if at PC=0 while loading.  Use allocated temp image. */
	if( dins->dins_EntryPoint == 0 )
	{
/* Used to skip over head but where did we go? */
/*		Image[0] = (uint16) (DSPP_JUMP_OPCODE +
			(dins->dins_EntryPoint + dins->dins_DSPPCodeSize)); */
		Write16(&Image[0], (uint16) (DSPP_SLEEP_OPCODE )); /* 940812 */
	}
	
/* Download Code to DSPP  */
TRACEB(TRACE_INT,TRACE_OFX,("DSPPLoadPatch: Code Entry at $%x\n", dins->dins_EntryPoint));
	DSPPDownloadCode(Image, dins->dins_EntryPoint,
		dins->dins_DSPPCodeSize);
	
	EZMemFree(Codes);
	Codes = NULL;

/* Initialize Instrument I Memory */
	Result = DSPPInitInsMemory( dins, DINI_AT_ALLOC );
	if (Result != 0) goto error;
	
/* 950126 Moved overwrite of SLEEP to DSPPStartCodeExecution() in dspp_instr.c */

TRACEB(TRACE_INT,TRACE_OFX,("DSPPLoadPatch: dins = $%x\n", dins));
TRACEB(TRACE_INT,TRACE_OFX,("DSPPLoadPatch: <<<<<<<<<<<<<<<<<<<\n"));

/* We made it, set return parameter. */
	*DInsPtr = dins;
	return(Result);
	
error:
DBUG(("DSPPLoadPatch: error = 0x%x\n", Result));
	if(NumAllocated > 0)
	{
/* Deallocate resources. 930704 */
		for (i=0; i<NumAllocated; i++)
		{
			DSPPFreeResource(dins,i);
		}
		dins->dins_NumResources = 0;
	}

	if (RsrcPtr) EZMemFree(RsrcPtr);
	if (Codes) EZMemFree(Codes);
	if (dins)
	{
/* Don't reference dins unless non-NULL 941102 */
		if (dins->dins_FIFOControls) EZMemFree(dins->dins_FIFOControls);
		EZMemFree(dins);
	}
	return(Result);
}

/*****************************************************************/
int32 DSPPAllocInstrument( DSPPTemplate *dtmp, DSPPInstrument **DInsPtr, int32 RateShift )
{
	DSPPInstrument *dins;
	DSPPKnob *dknb;
	int32 Result = 0;
	int32 scratch;

	*DInsPtr = NULL;
	Result = DSPPLoadPatch( dtmp, &dins, RateShift );
	if (Result) return Result;
	   
/* Attach Standard Knobs, OK if this fails. */
	DSPPAttachKnob(&dins->dins_FreqKnob ,dins, "Frequency");
	DSPPAttachKnob(&dins->dins_AmplitudeKnob, dins, "Amplitude");

/* Set Default Values for Knobs even if not grabbed. */
	dknb = dtmp->dtmp_Knobs;
	while (dknb != NULL)
	{
		DSPPPutKnob(dins, dknb, dknb->dknb_Default, &scratch, FALSE);
		dknb = DSPPNextKnob(dtmp->dtmp_Knobs, dknb);
	}

	InitList( &dins->dins_EnvelopeAttachments, "EnvAtt" );
	
	*DInsPtr = dins;
	return(Result);
	
}


/*****************************************************************/
int32 DSPPFreeInstrument( DSPPInstrument *dins, int32 IfOwned )
{
	int32 i, da, Result;
	List *AttList;
	
TRACEE(TRACE_INT,TRACE_OFX,("DSPPFreeInstrument: dins = $%x, dtmp = $%x\n", dins, dins->dins_Template));
	
/* Stop it in case it is still playing. */
	DSPPStopInstrument ( dins, NULL);

/* Just to be paranoid, paint with SLEEP code. %Q */
	da = dins->dins_EntryPoint;
#ifdef PARANOID
	for (i=0; i<dins->dins_DSPPCodeSize; i++ )
	{
		dsphWriteCodeMem( da++, DSPP_SLEEP_OPCODE );
	}
#endif

/* Scan list of Sample Attachments and delete them. */
/*	if(IfOwned) */
/*	{ */
		for (i=0; i<dins->dins_NumFIFOs; i++)
		{
			AttList = &dins->dins_FIFOControls[i].fico_Attachments;
			Result = afi_DeleteLinkedItems(AttList);  /* 930617 */
		}

		Result = afi_DeleteLinkedItems( &dins->dins_EnvelopeAttachments );
/*	} */
	
/* Deallocate resources. */
	for (i=0; i<dins->dins_NumResources; i++)
	{
		Result = DSPPFreeResource(dins,i);
TRACEB(TRACE_INT,TRACE_OFX,("DSPPFreeInstrument: rsrc[%d], result = 0x%x\n", i, Result));

		if (Result != 0) return Result;
	}
	dins->dins_NumResources = 0;
	
/* Free structures. */
	if (dins->dins_Resources) EZMemFree(dins->dins_Resources);
	if (dins->dins_FIFOControls) EZMemFree(dins->dins_FIFOControls);
	EZMemFree(dins);
TRACER(TRACE_INT,TRACE_OFX,("DSPPFreeInstrument: returns 0\n"));
	return 0;
}

/*****************************************************************/

void DSPPDownloadCode(uint16 *Image, int32 Entry, int32 CodeSize)
{
	int32 i;
	uint32 CodeVal;
	
	for (i=0; i<CodeSize; i++)
	{
		CodeVal = (uint32) Read16(&Image[i]);
		dsphWriteCodeMem( Entry + i, CodeVal );
	}
}


/*****************************************************************/
char *DSPPGetRsrcName(DSPPInstrument *dins, int32 indx)
{
	char *RsrcNames, *p;
	DSPPTemplate *dtmp;
	int32 i;
/* Scan packed names to find Nth name. */
	dtmp = dins->dins_Template;
	if( dtmp == NULL )
	{
		ERRDBUG(("DSPPGetRsrcName: NULL template\n"));
		return NULL;
	}
	
	RsrcNames = dins->dins_Template->dtmp_ResourceNames;
	p = RsrcNames;
	for (i=0; i<indx; i++)
	{
		p += strlen(p) + 1;
	}
	return p;
}

/*****************************************************************/
int32 DSPPImportResource( char *name, uint32 *Allocated)
{
	DSPPExternal *dext;
	
TRACEE(TRACE_INT,TRACE_OFX,("DSPPImportResource (%s, alloc=0x%s)\n", name, Allocated));
	dext = (DSPPExternal *) FindNamedNode(&DSPPData.dspp_ExternalList, name);
	if (dext == NULL)
	{
		ERR(("DSPPImportResource: Could not find external = %s\n", name));
		return AF_ERR_EXTERNALREF;
	}
	else
	{
TRACEB(TRACE_INT,TRACE_OFX,("DSPPImportResource: Found external node = %s\n", name));
		*Allocated = dext->dext_Resource->drsc_Allocated;
		dext->dext_Resource->drsc_References += 1;
		return 0;
	}
}
/*****************************************************************/
int32 DSPPExportResource( DSPPResource *drsc, char *name )
{
/* Export for other instruments to link to by adding to global list. */
	DSPPExternal *dext;
TRACEB(TRACE_INT,TRACE_OFX,("DSPPExportResource( drsc=0x%x, %s)\n", drsc, name));

/* Freed in DSPPUnExportResource. */
	dext = (DSPPExternal *) EZMemAlloc(sizeof(DSPPResource)+8,0);
	if (dext == NULL) 
	{
		return (AF_ERR_NOMEM);
	}
	else
	{
		dext->dext_Node.n_Name = name;
		dext->dext_Resource = drsc;
		AddTail(&DSPPData.dspp_ExternalList, (Node *) dext);
		return 0;
	}
}

/******************************************************************
** Scan all resources for an instrument.
** Return sum of all external reference counts.
******************************************************************/
int32 DSPPSumResourceReferences( DSPPInstrument *dins )
{
	int32 Sum = 0;
	int32 i;
	DSPPResource *drsc, *drscarray;

	drscarray = dins->dins_Resources;
TRACEE(TRACE_INT,TRACE_OFX, ("DSPPSumResourceReferences ( dins=0x%x, Type=0x%x, %s)\n",
	dins, RsrcType,Name));
TRACEB(TRACE_INT,TRACE_OFX, ("DSPPSumResourceReferences: NumRsrc=$%x\n", dins->dins_NumResources));

	for (i=0; i<dins->dins_NumResources; i++)
	{
		drsc = &drscarray[i];
TRACEB(TRACE_INT,TRACE_OFX, ("DSPPSumResourceReferences: i=%d, Type=$%x, Alloc=$%x\n",
			i, drsc->drsc_Type, drsc->drsc_Allocated));
		Sum += drsc->drsc_References;
	}
	
TRACER(TRACE_INT,TRACE_OFX, ("DSPPSumResourceReferences returns 0x%x\n", Sum));
	return Sum;
}

/*****************************************************************/
int32 DSPPUnExportResource( DSPPResource *drsc, char *name )
{
/* Remove Export node from Global export list. */
	DSPPExternal *dext;
	
TRACEE(TRACE_INT,TRACE_OFX,("DSPPUnExportResource ( 0x%x, %s )\n", drsc, name));
	if (drsc->drsc_References > 0)
	{
		ERRDBUG(("DSPPUnExportResource: %s in use.\n", name));
		return AF_ERR_INUSE;
	}

	dext = (DSPPExternal *) FindNamedNode(&DSPPData.dspp_ExternalList, name);
	if (dext == NULL)
	{
		ERRDBUG(("DSPPUnExportResource: Could not find external = %s\n", name));
		return AF_ERR_EXTERNALREF;
	}
	else
	{
TRACEB(TRACE_INT,TRACE_OFX,("DSPPUnExportResource: Found external node = %s\n", name));
DBUG(("DSPPUnExportResource: Found external node = %s\n", name));
		ParanoidRemNode( (Node *) dext );
		EZMemFree( (void *) dext );
		return 0;
	}
}

/*****************************************************************
** Returns number of ticks allocated or negative error.
*****************************************************************/
static int32 DSPPAllocateTicks( DSPPInstrument *dins, DSPPResource *drsc, uint32 *AllocatedPtr )
{
	int32 Result = 0;
	int32 *AvailTicks;
	
/* Cache pointer to save CPU time and code space. */
	AvailTicks = &DSPPData.dspp_AvailableTicks[0];
	
/* Check to see whether execution rate is full or half. */
	if( dins->dins_RateShift == 0 )
	{
		
		if( (AvailTicks[0] >= drsc->drsc_Many) &&
			(AvailTicks[1] >= drsc->drsc_Many) )
		{
			AvailTicks[0] -= drsc->drsc_Many;
			AvailTicks[1] -= drsc->drsc_Many;
			*AllocatedPtr = drsc->drsc_Many;
		}
		else
		{
			Result = AF_ERR_NORSRC;
		}
	}
	else if( dins->dins_RateShift == 1 )
	{
/* Allocate ticks from even or odd frame. */
		dins->dins_ExecFrame = ( AvailTicks[0] >=
		                 AvailTicks[1] ) ? 0 : 1 ;
		if( AvailTicks[dins->dins_ExecFrame] >= drsc->drsc_Many )
		{
			
			AvailTicks[dins->dins_ExecFrame] -= drsc->drsc_Many;
			*AllocatedPtr = drsc->drsc_Many;
		}
		else
		{
			Result = AF_ERR_NORSRC;
		}
	}
#ifdef PARANOID
	else
	{
		ERR(("Bad dins_RateShift\n"));
		Result = AF_ERR_BADOFX;
	}
#endif	
TRACEB(TRACE_INT,TRACE_OFX,("DSPPAllocateResource: AvailTicks[0] = %d, AvailTicks[1] = %d\n",
						AvailTicks[0], AvailTicks[0]));

	return Result;
}

/*****************************************************************/
static void DSPPFreeTicks( DSPPInstrument *dins, DSPPResource *drsc )
{
	int32 *AvailTicks;
	
/* Cache pointer to save CPU time and code space. */
	AvailTicks = &DSPPData.dspp_AvailableTicks[0];
	
/* Check to see whether execution rate is full or half. */
	if( dins->dins_RateShift == 0 )
	{
/* Free ticks from both frames. */
		AvailTicks[0] += drsc->drsc_Many;
		AvailTicks[1] += drsc->drsc_Many;
	}
	else if( dins->dins_RateShift == 1 )
	{
/* Free ticks from even or odd frame. */

		AvailTicks[dins->dins_ExecFrame] += drsc->drsc_Many;
	}
}

/*****************************************************************/
int32 DSPPAllocateResource(DSPPInstrument *dins, int32 indx)
{
	uint32 Allocated;
	int32 Type;
	DSPPResource *drsc, *RsrcArray;
	int32 Result;
	AudioFolioTaskData *aftd;
	int32 folioIndex;

	RsrcArray = dins->dins_Resources;
	drsc = &RsrcArray[indx];
	
TRACEB(TRACE_INT,TRACE_OFX|TRACE_HACK,("DSPPAllocateResource: RsrcName = %s\n",
	DSPPGetRsrcName(dins, indx) ));

	Type = drsc->drsc_Type & 0xFF; /* Mask off Import/Export */
	if (Type >= DSPP_NUM_RSRC_TYPES)
	{
		ERRDBUG(("DSPPAllocateResource: Type out of range = $%x\n",
			drsc->drsc_Type));
		return AF_ERR_BADRSRCTYPE;
	}
	
	if (drsc->drsc_Type & DRSC_IMPORT)
	{
		TRACEB(TRACE_INT,TRACE_OFX,("DSPPAllocateResource: Imported resource! \n"));
		Result = DSPPImportResource( DSPPGetRsrcName(dins, indx), &Allocated);
		if (Result) return Result;
	}
	else
	{
TRACEB(TRACE_INT,TRACE_OFX,("DSPPAllocateResource: Type = %d, Many = %d\n", Type, drsc->drsc_Many ));
		if (drsc->drsc_Many  > 0)
		{
		
/* Handle special cases */

			switch(Type)
			{
				case DRSC_TICKS:
					Result = DSPPAllocateTicks( dins, drsc, &Allocated );
					if( Result < 0 ) return Result;
					break;
					
				case DRSC_RBASE4:
					Result = AllocAlignedThings(&DSPPAllocators[DRSC_I_MEM],
							drsc->drsc_Many << 2, &Allocated, 4);
					if (Result) return AF_ERR_NORSRC;
/* This peculiar equation is for RMAP=0 register addressing. */
					Allocated = ((Allocated - OFFSET_I_MEM) >> 2) ^ 1;
					DSPPAllocators[Type].tall_Many += drsc->drsc_Many;
					break;
					
				case DRSC_RBASE8:
					Result = AllocAlignedThings(&DSPPAllocators[DRSC_I_MEM],
							drsc->drsc_Many << 3, &Allocated, 8);
					if (Result) return AF_ERR_NORSRC;
/* This peculiar equation is for RMAP=0 register addressing. */
					Allocated = ((Allocated - OFFSET_I_MEM) >> 2) + 1;
					DSPPAllocators[Type].tall_Many += drsc->drsc_Many;
DBUG(("RBASE8 = %d\n", Allocated));
					break;

				case DRSC_N_MEM:
					Result = AllocThings(&DSPPAllocators[Type],
							drsc->drsc_Many, &Allocated);
					if (Result) return AF_ERR_NORSRC;
					dins->dins_EntryPoint = Allocated;
					dins->dins_DSPPCodeSize = drsc->drsc_Many;

DBUG(("DRSC_N_MEM, Many = %d, Allocated = %d\n", drsc->drsc_Many, Allocated ));
DBUG(("DRSC_N_MEM, tall_Many %d\n", DSPPAllocators[Type].tall_Many ));
/* DisplayThingTable(&DSPPAllocators[Type]); */
					break;

				case DRSC_I_MEM:
					Result = AllocThings(&DSPPAllocators[Type],
							drsc->drsc_Many, &Allocated);
					if (Result) return AF_ERR_NORSRC;
					
/* Initialise the allocated data to zero to reduce pops. 930908 */
					if(DSPPData.dspp_IMEMAccessOn) /* %Q change for Bulldog */
					{
						
						Result = dsphWriteIMem( Allocated, 0 );
						if (Result < 0)
						{
							ERRDBUG(("Could not init IMEM\n"));
							return AF_ERR_NORSRC;
						}
					}
					break;
					
/* Allocate fixed addresses for Audio Input. 940502 */
				case DRSC_LEFT_ADC:
					folioIndex = AudioBase->af_Folio.f_TaskDataIndex;
					aftd = (AudioFolioTaskData *) CURRENTTASK->t_FolioData[folioIndex];
					if( aftd && (aftd->aftd_InputEnables > 0))
					{
						Allocated = DSPI_INPUT0;
					}
					else
					{
						return AF_ERR_NORSRC; /* %Q "not enabled" error better */
					}
DBUG(("Left ADC at 0x%x\n", Allocated ));
					break;
					
				case DRSC_RIGHT_ADC:
					folioIndex = AudioBase->af_Folio.f_TaskDataIndex;
					aftd = (AudioFolioTaskData *) CURRENTTASK->t_FolioData[folioIndex];
					if( aftd && (aftd->aftd_InputEnables > 0))
					{
						Allocated = DSPI_INPUT1;
					}
					else
					{
						return AF_ERR_NORSRC;
					}
DBUG(("Right ADC at 0x%x\n", Allocated ));
					break;
					
				case DRSC_NOISE:
					Allocated = DSPI_NOISE;
DBUG(("Noise at 0x%x\n", Allocated ));
					break;
					
				default:
					Result = AllocThings(&DSPPAllocators[Type],
							drsc->drsc_Many, &Allocated);
					if (Result) return AF_ERR_NORSRC;
					break;
			}
		}
		else
		{
			Allocated = drsc->drsc_Allocated; /* Use preallocation. */
		}
	}
	
TRACEB(TRACE_INT,TRACE_HACK,
	("DSPPAllocateResource: Type = $%x, Allocated = $%x\n", Type, Allocated));
	
	drsc->drsc_Allocated = Allocated;
/*	drsc->drsc_References += 1;  */
	drsc->drsc_References = 0;  /* Only count external references. 930827  */
		
	if (drsc->drsc_Type & DRSC_EXPORT)
	{
		Result = DSPPExportResource( drsc, DSPPGetRsrcName(dins, indx) );
		return Result;
	}
DBUG(("DSPPAllocateResource: RsrcName = %s, At 0x%x\n", DSPPGetRsrcName(dins, indx), Allocated ));

	return(0);
}

/*****************************************************************/
int32 DSPPFreeResource(DSPPInstrument *dins, int32 indx)
{
	int32 Type, Allocated;
	DSPPResource *drsc, *RsrcArray;
	int32 Result;
	DSPPTemplate *dtmp;
	
	RsrcArray = dins->dins_Resources;
	drsc = &RsrcArray[indx];
	Allocated = drsc->drsc_Allocated;

#ifdef PARANOID
	if( Allocated < 0 )
	{
		ERR(("DSPPFreeResource: Allocated = %d\n", Allocated));
		return -1;
	}
#endif

/* Don't free this if still referenced. */
	if (drsc->drsc_Type & DRSC_EXPORT)
	{
		Result = DSPPUnExportResource( drsc, DSPPGetRsrcName(dins, indx) );
		if( Result < 0 ) return Result;
	}

DBUGRSRC(("DSPPFreeResource: >>>>>>>>>>>>>>>>>>>>>>>>>\n"));
DBUGRSRC(("DSPPFreeResource: RsrcName = %s\n", DSPPGetRsrcName(dins, indx) ));

	Type = drsc->drsc_Type & 0xFF; /* Mask off Import/Export */
	
	if (drsc->drsc_Type & DRSC_IMPORT)
	{
		dtmp = dins->dins_Template;
		if( dtmp )
		{
			TRACEB(TRACE_INT,TRACE_OFX,("DSPPFreeResource: Imported resource! \n"));
			Result = DSPPUnImportResource( DSPPGetRsrcName(dins, indx) );
			if (Result) return Result;
		}
		else
		{
			ERRDBUG(("DSPPFreeResource: NULL template is OK.\n"));
		}
	}
	else
	{
/* Handle special cases */
		switch(Type)
		{
			case DRSC_TICKS:
				DSPPFreeTicks( dins, drsc );
				break;
				
			case DRSC_RBASE4:
				Allocated = ((Allocated - 1) << 2) + OFFSET_I_MEM;
				FreeThings(&DSPPAllocators[DRSC_I_MEM], Allocated, drsc->drsc_Many << 2);
				DSPPAllocators[Type].tall_Many -= drsc->drsc_Many;
				break;
				
			case DRSC_RBASE8:
				Allocated = ((Allocated ^ 1) << 2) + OFFSET_I_MEM;
				FreeThings(&DSPPAllocators[DRSC_I_MEM], Allocated, drsc->drsc_Many << 3);
				DSPPAllocators[Type].tall_Many -= drsc->drsc_Many;
				break;
								
			default:
				FreeThings(&DSPPAllocators[Type], Allocated, drsc->drsc_Many);
		}
	}
	
DBUGRSRC(("DSPPFreeResource: Type = $%x, Allocated = $%x\n", Type, Allocated));
	drsc->drsc_Allocated = -1;
	/* Only count external references. 930827  */
/*	drsc->drsc_References -= 1; */
DBUGRSRC(("DSPPFreeResource: <<<<<<<<<<<<<<<<<<\n"));

	return(0);
}

/*****************************************************************/
int32 DSPPUnImportResource( char *name )
{
/* Remove Import reference. */
	DSPPExternal *dext;
	
TRACEE(TRACE_INT,TRACE_OFX,("DSPPUnImportResource ( %s )\n", name));
	dext = (DSPPExternal *) FindNamedNode(&DSPPData.dspp_ExternalList, name);
	if (dext == NULL)
	{
		ERRDBUG(("DSPPUnImportResource: Could not find external = %s\n", name));
		return AF_ERR_EXTERNALREF;
	}
	else
	{
TRACEB(TRACE_INT,TRACE_OFX,("DSPPUnImportResource: Found external node = %s\n", name));
		dext->dext_Resource->drsc_References -= 1;
		return 0;
	}
}

/*****************************************************************/

			
void Put16 ( int32 Value, int32 indx, uint16 *ar )
{
	Write16(&ar[indx], (uint16) Value );
TRACEB(TRACE_INT,TRACE_OFX,("Put16: Image[$%x] = $%x\n", indx, Value));
}

/*****************************************************************/
int32 DSPPRelocateAll( DSPPInstrument *dins, DSPPCodeHeader *Codes)
{
	DSPPTemplate *dtmp;
	DSPPRelocation *drlc;
	DSPPResource *drsc;
	uint32  val, ri;
	int32  i, Result = 0;

TRACEE(TRACE_INT,TRACE_OFX,("DSPPRelocateAll( dins=0x%x, Codes=0x%x )\n", dins, Codes));

	dtmp = dins->dins_Template;
	
	for (i=0; i<dtmp->dtmp_NumRelocations; i++)
	{
		drlc = &dtmp->dtmp_Relocations[i];
		
/* Get resource index and do bounds check. */
		ri = drlc->drlc_RsrcIndex;
		if (ri > dtmp->dtmp_NumResources) return AF_ERR_BADRSRCTYPE;

/* Use cloned resource requests from instrument. */
		drsc = &dins->dins_Resources[ri];
		val = GetRsrcAttribute(drsc, drlc->drlc_Attribute);
TRACEB(TRACE_INT,TRACE_OFX,("-----------------\nRelocate Type = %d, val = $%x\n",
		drsc->drsc_Type, val));
		Result = DSPPRelocate ( drlc, val , Codes, Put16 );
		if (Result) goto error;
	}

error:
TRACER(TRACE_INT,TRACE_OFX,("DSPPRelocateAll returns 0x%x\n", Result));
	return (Result);
}

/*****************************************************************/
#define GETPAD(indx) if (if32) { pad = longs[indx]; } \
			else { pad = Read16(&shorts[indx]); }
			
int32 DSPPRelocate( DSPPRelocation *drlc, int32 Value,
	DSPPCodeHeader *Codes, void (*PutFunc)())
{
	uint32  *longs, pad, old, mask, next;
	int32 if32, CodeSize, shft;
	uchar *CodePtr;
	uint16 *shorts;
	int32 Offset, Result = 0;
	
	
TRACEE(TRACE_INT,TRACE_OFX,("DSPPRelocate( 0x%x, 0x%x ", drlc, Value));
TRACEE(TRACE_INT,TRACE_OFX,(", 0x%x, 0x%x)\n", Codes, PutFunc));

	CodePtr = CODESTART(Codes, drlc->drlc_CodeIndex);
	shorts = (uint16 *) CodePtr;
	longs = (uint32 *) CodePtr;
	
	CodeSize = Codes[drlc->drlc_CodeIndex].dcod_Size;
TRACEB(TRACE_INT,TRACE_OFX,("DSPPRelocate: CodeSize = 0x%x\n", CodeSize));

	if32 = (int32) drlc->drlc_Flags & DRLC_32;

	shft = drlc->drlc_Bit;
	mask = ((uint32)(0xFFFFFFFF >> (32L-(int32)drlc->drlc_Width))) << shft;
	
TRACEB(TRACE_INT,TRACE_OFX,("DSPPRelocate: if32 = %d, shft = %d, mask = $%x\n",
		 if32, shft, mask));
			
	if (drlc->drlc_Flags & DRLC_ADD)
	{
		if (drlc->drlc_Flags & DRLC_LIST)
		{
			 Result = AF_ERR_RELOCATION;
			 goto error;
		}
/* Get word from target to be relocated as either 16 or 32 bit */
		Offset = drlc->drlc_CodeOffset;
		if ((Offset < 0) || (Offset >= CodeSize))
		{
			ERRDBUG(("DSPPRelocate: Offset outside code bounds = 0x%x\n", Offset));
			Result = AF_ERR_RELOCATION;
			goto error;
		}
		else
		{
			GETPAD(Offset);
			old = pad & mask;
			pad = (pad & ~mask) | ((Value << shft) + old);
			PutFunc(pad, Offset, CodePtr);
		}
	}
	else
	{ /* set directly */
		if (drlc->drlc_Flags & DRLC_LIST)
		{
/* Scan list and set each entry until 0 link encountered */
			Offset = drlc->drlc_CodeOffset;
			do
			{
				if ((Offset < 0) || (Offset >= CodeSize))
				{
					ERRDBUG(("DSPPRelocate: Offset outside code bounds = 0x%x\n", Offset));
					Result = AF_ERR_RELOCATION;
					goto error;
				}
				else
				{
					GETPAD(Offset);
TRACEB(TRACE_INT,TRACE_OFX,("DSPPRelocate: LIST Code[$%x] = $%x\n", Offset, pad))
					next = (pad & mask) >> shft;   /* get link */
					pad = (pad & ~mask) | (Value << shft);
					PutFunc(pad, Offset, CodePtr);
				}
				Offset = next;
			} while (next != 0);
		}
		else
		{
			Offset = drlc->drlc_CodeOffset;
			if ((Offset < 0) || (Offset >= CodeSize))
			{
				ERRDBUG(("DSPPRelocate: Offset outside code bounds = 0x%x\n", Offset));
				Result = AF_ERR_RELOCATION;
			}
			else
			{
				GETPAD(Offset);
TRACEB(TRACE_INT,TRACE_OFX,("DSPPRelocate: Code[$%x] = $%x\n", Offset, pad))
				pad = (pad & ~mask) | (Value << shft);
				PutFunc(pad, Offset, CodePtr);
			}
		}
	}
error:
TRACER(TRACE_INT,TRACE_OFX,("DSPPRelocate: returns %x\n", Result))
	return Result;
}

/*****************************************************************/
uint32 GetRsrcAttribute(DSPPResource *drsc, int32 Attribute)
{	
	switch(drsc->drsc_Type)
	{
		case DRSC_IN_FIFO:
			switch(Attribute)
			{
				case DRSC_FIFO_NORMAL:
					return drsc->drsc_Allocated + 0xF0; /* Bump */
				case DRSC_FIFO_STATUS:
					return drsc->drsc_Allocated + 0xD0; /* Status */
				case DRSC_FIFO_READ:
					return drsc->drsc_Allocated + 0x70; /* Read */
				default:
					return AF_ERR_RSRCATTR;
			}
			
		case DRSC_OUT_FIFO:
			switch(Attribute)
			{
				case DRSC_FIFO_NORMAL:
					return drsc->drsc_Allocated + 0x3F0; /* Bump */
				case DRSC_FIFO_STATUS:
					return drsc->drsc_Allocated + 0xE0; /* Status */
				default:
					return AF_ERR_RSRCATTR;
			}
			
		default:
			return drsc->drsc_Allocated;
	}
}

