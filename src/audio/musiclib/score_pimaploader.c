/* $Id: score_pimaploader.c,v 1.11 1994/10/25 00:14:54 phil Exp $ */
/****************************************************************
**
** PIMap loader for score player.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
** 940413 WJB Split off from score.c
** 940727 WJB Added autodocs.
** 940812 WJB Cleaned up triple bangs in includes.
**            Added usage of SCORE_MIN|MAX_PRIORITY.
** 940921 PLB Added 'r' command for rate shift.
** 940921 PLB pimp_RateShift is now pimp_RateDivide.
****************************************************************/

#include <audio.h>
#include <ctype.h>              /* isspace() */
#include <filestreamfunctions.h>
#include <musicerror.h>         /* ML_ERR_ */
#include <score.h>
#include <stdio.h>              /* printf() */


/* -------------------- Macros */

#define	PRT(x)	       printf x
#define	ERR(x)         PRT(x)               /* !!! only define in DEVELOPMENT mode? */
#define	DBUG(x)        /* PRT(x) */
#define	DBUGALLOC(x)   /* PRT(x) */
#define	DBUGLOAD(x)	   PRT(x)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s, result = 0x%x\n",name,Result)); \
		goto cleanup; \
	}


/* -------------------- Local Functions */

    /* stream support */
static int32 ReadStreamLine( Stream *str, char *pad, int32 PadSize );
static int32 StripTrailingBlanks( char *s );
static char *ParseWord( char *s, char **newp );


/* -------------------- PIMap Loader */

static Item CreateAttachment( Item Instrument, Item Sample, char *FIFOName, uint32 Flags);

/******************************************************************
** Load a PIMap by parsing a text file.
******************************************************************/
static Item CreateAttachment( Item Instrument, Item Sample, char *FIFOName, uint32 Flags)
{
	Item Result = -1;
	TagArg Tags[5];

	Tags[0].ta_Tag = AF_TAG_HOOKNAME;
	Tags[0].ta_Arg = (void *) FIFOName;
	Tags[1].ta_Tag = AF_TAG_SAMPLE;
	Tags[1].ta_Arg = (void *) Sample;
	Tags[2].ta_Tag = AF_TAG_INSTRUMENT;
	Tags[2].ta_Arg = (void *) Instrument;
	Tags[3].ta_Tag = AF_TAG_SET_FLAGS;
	Tags[3].ta_Arg = (void *) Flags;
	Tags[4].ta_Tag =  TAG_END;
	Result = CreateItem( MKNODEID(AUDIONODE,AUDIO_ATTACHMENT_NODE), Tags );
	return Result;
}

/******************************************************************
** Load a PIMap by parsing a text file.
******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/loadpimap
 |||	LoadPIMap - Loads a program-instrument map from a text file.
 |||
 |||	  Synopsis
 |||
 |||	    Err LoadPIMap( ScoreContext *scon, char *FileName )
 |||
 |||	  Description
 |||
 |||	    This procedure reads the designated program-instrument map file (PIMap
 |||	    file) and writes appropriate values to the specified score context's
 |||	    PIMap.  It also assigns appropriate sampled-sound instruments to samples
 |||	    listed in the PIMap file and imports all instrument templates listed in
 |||	    the PIMap file.
 |||
 |||	    For information about the format of a PIMap file, read the "Playing
 |||	    MIDI Scores" chapter of the Portfolio Programmer's Guide.
 |||
 |||	    Note that if you want to set a score context's PIMap entries directly,
 |||	    you can use SetPIMapEntry().
 |||
 |||	  Arguments
 |||
 |||	    scon                         Pointer to a ScoreContext data structure.
 |||
 |||	    FileName                     Pointer to the character string containing
 |||	                                 the name of the PIMap file.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    score.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    SetPIMapEntry(), UnloadPIMap()
 |||
**/
int32 LoadPIMap ( ScoreContext *scon, char *FileName )
{
	Stream *str = NULL;
	int32 Result = 0;
	char *p,*Com, *NumText;
	int32 ProgramNum, LineNum = 0;
	Item TemplateItem;
	Item SampleItem;
	Item Attachment;
	char *InstrumentName, *SampleName;
	int32 IfVariable, Temp;
	int32 Len;
	TagArg SampleTags[5];
	int32 SampleTagIndex;

#define PADSIZE 128
	char pad[PADSIZE];

	str =  OpenDiskStream( FileName, 0);
	if (str == NULL)
	{
		ERR(("Couldn't open %s\n", FileName));
		Result = ML_ERR_BAD_FILE_NAME;
		goto cleanup;
	}

	LineNum = -1;
	while(1)
	{
		do
		{
			Len = ReadStreamLine( str, pad, PADSIZE );
DBUG(("Line = %s\n", pad));
			if (Len == -1) goto cleanup;
			CHECKRESULT(Len,"LoadPIMap: ReadStreamLine");
			LineNum++;
			Len = StripTrailingBlanks( pad );
		} while( (pad[0] == ';') || (Len == 0));   /* Comment char = ';'  OR blank line */
		SampleTagIndex = 0;

		p = &pad[0];
		IfVariable = TRUE;
		ProgramNum = strtol(p, &p, 0);
/* Convert to internal zero based format. */
		ProgramNum -= 1;
		if ((ProgramNum < 0) || (ProgramNum > scon->scon_PIMapSize))
		{
			ERR(("PIMap line %d: Program Number out of range = %d\n",
				LineNum, ProgramNum+1));
			Result = ML_ERR_OUT_OF_RANGE;
			goto cleanup;
		}

		SampleName = ParseWord(p, &p);   /* Name follows number */
		PRT(("%d = %s\n", ProgramNum+1, SampleName));

/* Parse flags from command line. */
		while(p != NULL)
		{
			Com = ParseWord(p, &p);
			if (strlen(Com) == 0) break;
DBUG(("Com = %s\n", Com));
			if (Com[0] != '-')
			{
				ERR(("PIMap line %d: LoadPIMap expects '-' before flags, got %s\n",
					LineNum, Com));
				Result = ML_ERR_BAD_FORMAT;
				goto cleanup;
			}
			switch(Com[1])
			{
				case 'f':
DBUG(("Fixed rate.\n"));
					IfVariable = FALSE;
					break;

				case 'm':
					NumText = ParseWord(p, &p);
					Temp = atoi( NumText );
					if ((Temp < 0) || (Temp > 127))
					{
						ERR(("Max Voices out of range.\n", Temp));
						Result = ML_ERR_OUT_OF_RANGE;
						goto cleanup;
					}
					scon->scon_PIMap[ProgramNum].pimp_MaxVoices = (uint8)Temp;
DBUGALLOC(("Max Voices = %d.\n", Temp));
					break;


				case 'r':
					NumText = ParseWord(p, &p);
					Temp = atoi( NumText );
					if ((Temp < 1) || (Temp > 2))
					{
						ERR(("Execution Rate Divisor out of range. N = %d\n", Temp));
						Result = ML_ERR_OUT_OF_RANGE;
						goto cleanup;
					}
					scon->scon_PIMap[ProgramNum].pimp_RateDivide = (uint8)Temp;
DBUGALLOC(("Rate Shift = %d.\n", Temp));
					break;

				case 'p':
					NumText = ParseWord(p, &p);
					Temp = atoi( NumText );
					if ((Temp < SCORE_MIN_PRIORITY) || (Temp > SCORE_MAX_PRIORITY))
					{
						ERR(("Priority out of range.\n", Temp));
						Result = ML_ERR_OUT_OF_RANGE;
						goto cleanup;
					}
					scon->scon_PIMap[ProgramNum].pimp_Priority = (uint8)Temp;
DBUGALLOC(("Priority = %d.\n", Temp));
					break;

				case 'l':
					NumText = ParseWord(p, &p);
					Temp = atoi( NumText );
					if ((Temp < 0) || (Temp > 127))
					{
						ERR(("Low note out of range.\n", Temp));
						Result = ML_ERR_OUT_OF_RANGE;
						goto cleanup;
					}
					SampleTags[SampleTagIndex].ta_Tag = AF_TAG_LOWNOTE;
					SampleTags[SampleTagIndex++].ta_Arg = (void *) Temp;
DBUGLOAD(("Low note = %d.\n", Temp));
					break;

				case 'b':
					NumText = ParseWord(p, &p);
					Temp = atoi( NumText );
					if ((Temp < 0) || (Temp > 127))
					{
						ERR(("Base note out of range.\n", Temp));
						Result = ML_ERR_OUT_OF_RANGE;
						goto cleanup;
					}
					SampleTags[SampleTagIndex].ta_Tag = AF_TAG_BASENOTE;
					SampleTags[SampleTagIndex++].ta_Arg = (void *) Temp;
DBUGLOAD(("Base note = %d.\n", Temp));
					break;

				case 'h':
					NumText = ParseWord(p, &p);
					Temp = atoi( NumText );
					if ((Temp < 0) || (Temp > 127))
					{
						ERR(("High note out of range.\n", Temp));
						Result = ML_ERR_OUT_OF_RANGE;
						goto cleanup;
					}
					SampleTags[SampleTagIndex].ta_Tag = AF_TAG_HIGHNOTE;
					SampleTags[SampleTagIndex++].ta_Arg = (void *) Temp;
DBUGLOAD(("High note = %d.\n", Temp));
					break;

					case 'd':
					NumText = ParseWord(p, &p);
					Temp = atoi( NumText );
					if ((Temp < -100) || (Temp > 100))
					{
						ERR(("Detune out of range.\n", Temp));
						Result = ML_ERR_OUT_OF_RANGE;
						goto cleanup;
					}
					SampleTags[SampleTagIndex].ta_Tag = AF_TAG_DETUNE;
					SampleTags[SampleTagIndex++].ta_Arg = (void *) Temp;
DBUGLOAD(("Detune = %d.\n", Temp));
					break;

				default:
					ERR(("Bad option in PIMap = %c\n", Com[1] ));
			}
		}

/* Determine whether it is a sample or not. */
		Len = strlen(SampleName);
		if( (strcasecmp(&SampleName[Len-5], ".aiff") == 0) ||
			(strcasecmp(&SampleName[Len-5], ".aifc") == 0) ||
			(strcasecmp(&SampleName[Len-4], ".aif") == 0) )
		{
/* Load sample and attach it to Sampler Template */
			SampleItem = LoadSample(SampleName);
			if (SampleItem < 0)
			{
				ERR(("LoadPIMap failed to load sample = %s\n", SampleName));
				Result = SampleItem;
				goto cleanup;
			}

/* Set sample info. */
			if(SampleTagIndex > 0)
			{
				SampleTags[SampleTagIndex].ta_Tag = TAG_END;
				Result = SetAudioItemInfo( SampleItem, SampleTags );
				CHECKRESULT(Result,"LoadPIMap: SetAudioItemInfo");
			}

			if( scon->scon_PIMap[ProgramNum].pimp_InsTemplate > 0)
			{
				TemplateItem = scon->scon_PIMap[ProgramNum].pimp_InsTemplate;
				DBUG(("Multisample on %d\n", ProgramNum));
			}
			else
			{
				InstrumentName = SelectSamplePlayer( SampleItem , IfVariable );
				if (InstrumentName == NULL)
				{
					ERR(("No instrument to play that sample.\n"));
					Result = ML_ERR_UNSUPPORTED_SAMPLE;
					goto cleanup;
				}
	PRT(("Use instrument: %s for %s\n", InstrumentName, SampleName));
				TemplateItem = LoadInsTemplate (InstrumentName, 0);
				if (TemplateItem < 0)
				{
					ERR(("LoadPIMap failed for %s\n", InstrumentName));
					Result = TemplateItem;
					goto cleanup;
				}
			}

			Attachment = CreateAttachment( TemplateItem, SampleItem, 0, AF_ATTF_FATLADYSINGS);
		}
		else
		{
			TemplateItem = LoadInsTemplate (SampleName, 0);
			if (TemplateItem < 0)
			{
				ERR(("LoadPIMap failed for instrument %d = %s\n", ProgramNum+1, SampleName));
				Result = TemplateItem;
				goto cleanup;
			}
		}

		scon->scon_PIMap[ProgramNum].pimp_InsTemplate = TemplateItem;
	};

cleanup:
	if (str) CloseDiskStream(str);
	if(Result < 0)
	{
		ERR(("LoadPIMap line %d = %s\n", LineNum, pad));
	}

	return(Result);
}

/******************************************************************
** UNload stuff from Load PIMap
******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/unloadpimap
 |||	UnloadPIMap - Unloads instrument templates loaded previously with PIMap
 |||	              file.
 |||
 |||	  Synopsis
 |||
 |||	    Err UnloadPIMap( ScoreContext *scon )
 |||
 |||	  Description
 |||
 |||	    This procedure is the inverse of LoadPIMap; it unloads any instrument
 |||	    templates specified in the score context's PIMap.  Any instruments
 |||	    created using those templates are freed when the templates are unloaded.
 |||
 |||	  Arguments
 |||
 |||	    scon                         Pointer to a ScoreContext data structure.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    score.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    LoadPIMap(), SetPIMapEntry()
 |||
**/
int32 UnloadPIMap ( ScoreContext *scon )
{
	int32 i;
#ifdef REAL_TIME_TRACE
	DumpTraceRecord();
#endif
	for (i=0; i<scon->scon_PIMapSize; i++)
	{
		if (scon->scon_PIMap[i].pimp_InsTemplate)
		{
			UnloadInsTemplate( scon->scon_PIMap[i].pimp_InsTemplate );
			scon->scon_PIMap[i].pimp_InsTemplate = 0;
		}
	}
	return 0;
}


/* -------------------- Stream support functions */

#define NUL ('\0')
#define EOL ('\r')

/*********************************************************************/
static int32 ReadStreamLine( Stream *str, char *pad, int32 PadSize )
{
	int32 Len, i;
	char *s;

	s = pad;
	for(i=0; i<PadSize; i++)
	{
		Len = ReadDiskStream ( str, s, 1);
		if (Len < 0)  return Len;

		if (Len == 0)
		{
			if( i == 0 )
			{
				pad[0] = NUL;
				return -1;
			}
			else
			{
				*s = NUL;
			}
		}

		if (*s == EOL)
		{
			*s = NUL;
		}

		if (*s == NUL) break;

		s++;
	}
	return (s-pad);
}

/**********************************************************************
** Remove trailing blanks from a string.
**********************************************************************/
static int32 StripTrailingBlanks( char *s )
{
	int32 Len;
	char *p;

	Len = strlen( s );
	p = s+Len-1;

	while( Len>0 )
	{
		if( isspace(*p) )
		{
			*p = NUL;
		}
		else
		{
			break;
		}
		Len--;
		p--;
	}
DBUG(("Strip returns %d\n", Len));
	return Len;
}

/*********************************************************************/
/* Return empty string if no words. */
/* *newp = NULL if no characters remaining. */
static char *ParseWord( char *s, char **newp )
{
	char *w;

	if(s==NULL)
	{
		ERR(("ParseWord: NULL STRING POINTER!\n"));
		return NULL;
	}

/* Skip leading blanks. */
	while(isspace(*s)) s++;
	w = s;  /* result */
/* Scan for next white space. */
	while(1)
	{
		if (*s == NUL)
		{
			*newp = NULL;
			break;
		}
		if (!isspace(*s))
		{
			s++;
		}
		else
		{
			*s = NUL;  /* NUL terminate returned name. */
			*newp = s+1;
			break;
		}
	}
	return w;
}

