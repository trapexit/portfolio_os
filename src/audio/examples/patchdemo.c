
/******************************************************************************
**
**  $Id: patchdemo.c,v 1.23 1994/11/22 23:16:59 peabody Exp $
**
**  patchdemo.arm main module.
**
**  Related files:
**      . patchdemo.h   central .h file for patchdemo
**      . pd_patch.c    patch structure management and file parser
**      . pd_faders.c   gui stuff
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC tpg/shell/patchdemo
|||	patchdemo - tool to experiment with DSP patch construction
|||
|||	  Format
|||
|||	    patchdemo \<patch file>
|||
|||	  Description
|||
|||	    This program interprets a text file containing a description of a patch
|||	    made out of DSP instruments, samples, delay lines, etc. It then provides you
|||	    with a set of faders for adjusting knobs in the patch's instruments. The patch
|||	    language is a very simple command oriented language that closely resembles
|||	    audio folio function names. Therefore, patchdemo can be used to quickly construct
|||	    DSP patches that you later plan to implement in C. Also, with only a few
|||	    restrictions, patchdemo patches can be imported directly into ARIA.
|||
|||	    patchdemo also has a built-in oscilloscope for capturing and viewing output
|||	    audio signal.
|||
|||	    Controls (faders mode):
|||
|||	        A                       Toggle between Start and Release of instrument.
|||
|||	        LShift + A              Stop instrument.
|||
|||	        B                       Start instrument when pressed, Release instrument
|||	                                when released (like a key on a MIDI keyboard).
|||
|||	        C                       Display faders for next instrument in patch.
|||
|||	        LShift + C              Display faders for previous instrument in patch.
|||
|||	        X                       Quit
|||
|||	        Up or Down              Select a fader by moving up or down.
|||
|||	        Left or Right           Adjust current fader (coarse).
|||
|||	        LShift + Left or Right  Adjust current fader (fine).
|||
|||	        RShift                  Switch to scope mode.
|||
|||	    Controls (scope mode):
|||
|||	        A                       Acquire signal as long as A button is held down.
|||	                                Hold last acquired signal when released.
|||
|||	        C                       Switch to faders mode.
|||
|||	        X                       Quit
|||
|||	        Up or Down              Increase or decrease vertical scaling.
|||
|||	        Left or Right           Pan left or right.
|||
|||	        LShift or RShift        Decrease or increase horizontal scaling.
|||
|||	  Arguments
|||
|||	    \<patch file>                Name of patch file to load.
|||
|||	  Implementation
|||
|||	    Released as an example in V21.
|||
|||	    Implemented as a command in V24 (source code no longer distributed).
|||
|||	  Location
|||
|||	    $c/patchdemo
|||
|||	  See Also
|||
|||	    3DO cookbook for Sound Designers - more complete documentation of patchdemo
|||	    features, operation, and language.
|||
|||	    Examples/Audio/PatchDemo - examples patches.
|||
|||	    ARIA - tool to build custom DSP instruments.
|||
|||	    dspfaders - demo a single DSP instrument.
|||
**/

    /* local */
#include "patchdemo.h"

    /* audiodemo */
#include "faders.h"         /* DriveFaders() */
#include "graphic_tools.h"  /* InitGraphics(), SwitchScreens() */

    /* portfolio */
#include <audio.h>          /* Open/CloseAudioFolio() */
#include <event.h>          /* Init/KillEventUtility(), GetControlPadEvent() */
#include <operror.h>        /* PrintError() */
#include <stdio.h>          /* printf() */


/* -------------------- Debugging */

#define DEBUG_DumpPatch     1   /* display final Patch structure returned by LoadPatch() */
#define DEBUG_Mem           0   /* availmem test */


/* -------------------- local functions */

static Err DemoPatch (Patch *);

#if DEBUG_DumpPatch
  static void DumpPatch (const Patch *);
#endif


/* -------------------- main() */

int main (int argc, char *argv[])
{
    Patch *patch = NULL;
    Err errcode = 0;

  #if DEBUG_Mem
    printf ("main() entry: "); printavail(); printf ("\n");
  #endif

        /* check usage */
    if (argc < 2) {
        printf ("usage: %s <patch file>\n", argv[0]);
        goto fail;
    }

        /* open libraries */
    if ((errcode = OpenAudioFolio()) < 0) {
        PrintError (NULL, "open", "AudioFolio", errcode);
        goto fail;
    }
#if 1
    if ((errcode = EnableAudioInput (TRUE, NULL)) < 0) {
        PrintError (NULL, "enable", "audio input", errcode);
        goto fail;
    }
#else
	printf("EnableAudioInput() disabled.\n");
#endif
    if ((errcode = InitGraphics(2)) < 0) {
        PrintError (NULL, NULL, "InitGraphics()", errcode);
        goto fail;
    }
    if ((errcode = InitEventUtility (1, 0, TRUE)) < 0) {
        PrintError (NULL, NULL, "InitEventUtility()", errcode);
        goto fail;
    }

        /* load patch */
    if ((patch = LoadPatch (argv[1], &errcode)) == NULL) {
        PrintError (NULL, "load patch", argv[1], errcode); /* 940906 */
        goto clean;
    }

  #if DEBUG_DumpPatch
    DumpPatch (patch);
  #endif

        /* demo patch */
    if ((errcode = DemoPatch (patch)) < 0) {
        PrintError (NULL, "demo patch", argv[0], errcode);
        goto clean;
    }

clean:
    UnloadPatch (patch);

fail:
    KillEventUtility();
    TermGraphics();         /* !!! this function doesn't seem to clean up everything done by InitGraphics() */
    CloseAudioFolio();

  #if DEBUG_Mem
    printf ("main() exit: "); printavail(); printf ("\n");
  #endif

	printf( "%s finished.\n", argv[0] );
    return 0;
}


/* -------------------- DemoPatch() */

static Err procevents (PatchPageList *);
#define triggerpatch(patch,noteon) ( ( (noteon) != 0 ? StartPatch : ReleasePatch ) (patch, NULL) )

    /* !!! publish? */
#define NextLoopNode(l,n) (IsNode((l),NextNode(n)) ? NextNode(n) : FirstNode(l))
#define PrevLoopNode(l,n) (IsNodeB((l),PrevNode(n)) ? PrevNode(n) : LastNode(l))


/*
    Create and display PatchPageList for Patch.
*/
static Err DemoPatch (Patch *patch)
{
    PatchPageList *pagelist = NULL;
    Err errcode = 0;

        /* create PatchPageList for Patch */
    if ((pagelist = NewPatchPageList (patch, &errcode)) == NULL) goto clean;

        /* trap no pages */
    if (IsEmptyList (&pagelist->ppage_PageList)) {
        PrintError (NULL, "\\no instruments to edit", NULL, 0);        /* !!! more graceful error message here */
        errcode = -1;       /* !!! real code */
        goto clean;
    }

        /* process events */
    errcode = procevents (pagelist);

clean:
    DeletePatchPageList (pagelist);
    return errcode;
}

/*
    process events for PatchPageList

    controls:
        X                   quit
        A                   toggle instrument start/release
        B                   start on down, release on up
        C                   select next PatchPage
        LShift C            select previous PatchPage
        Up/Down             select fader
        Left/Right          adjust fader value quickly
        LShift Left/Right   adjust fader value slowly
        RShift              patches/scope/audiomon
*/
static Err procevents (PatchPageList *pagelist)
{
    ControlPadEventData cur_cped, last_cped;
    Boolean noteon = FALSE;
    Err errcode = 0;
    PatchPage *page = (PatchPage *)FirstNode (&pagelist->ppage_PageList);       /* empty patch is trapped in DemoPatch() */

	ScopeProbe *scpr;

	scpr = CreateScopeProbe( SCOPE_MAX_SAMPLES*2 );
	if( scpr == NULL )
	{
		printf("Couldn't make scpr\n");
		goto clean;
	}

        /* show first page */
    if ((errcode = ShowPatchPage (page)) < 0) goto clean;

        /* get initial state of controller */
    if ((errcode = GetControlPad (1, FALSE, &cur_cped)) < 0) goto clean;
    last_cped = cur_cped;

        /* start instrument */
    if ((errcode = triggerpatch (pagelist->ppage_Patch, noteon = TRUE)) < 0) goto clean;

        /* process controller events */
    for (;;) {
            /* get a button event, build changed buttons mask */
        if ((errcode = GetControlPad (1, FALSE, &cur_cped)) < 0) goto clean;

            /* demultiplex button state changes */
        {
            const uint32 changedbuttons = last_cped.cped_ButtonBits ^ cur_cped.cped_ButtonBits;
            uint32 buttonmask;

            /* process fader events */
        	DriveFaders (&page->ppage_FaderBlock, cur_cped.cped_ButtonBits);

               if (changedbuttons) for (buttonmask = 0x80000000; buttonmask; buttonmask >>= 1) if (changedbuttons & buttonmask) {
                const Boolean buttondown = (cur_cped.cped_ButtonBits & buttonmask) != 0;

                switch (buttonmask) {
                    case ControlA:      /* A: toggle start/release instrument; LShift+A: stop instrument */
                        if (buttondown) {
                            if (cur_cped.cped_ButtonBits & ControlLeftShift) {
                                noteon = FALSE;
                                if ((errcode = StopPatch (pagelist->ppage_Patch, NULL)) < 0) goto clean;
                            }
                            else {
                                if ((errcode = triggerpatch (pagelist->ppage_Patch, noteon = !noteon)) < 0) goto clean;
                            }
                        }
                        break;

                    case ControlB:      /* B: start/release instrument based on B button state. */
                        if ((errcode = triggerpatch (pagelist->ppage_Patch, noteon = buttondown)) < 0) goto clean;
                        break;

                    case ControlC:      /* C: next instrument page; LShift+C: prev instrument page */
                        if (buttondown) {
                            page = (PatchPage *)( (cur_cped.cped_ButtonBits & ControlLeftShift)
                                                  ? PrevLoopNode (&pagelist->ppage_PageList, page)
                                                  : NextLoopNode (&pagelist->ppage_PageList, page) );

                            if ((errcode = ShowPatchPage (page)) < 0) goto clean;
                        }
                        break;

                    case ControlX:      /* Stop (X): quit */
                        if (buttondown) goto clean;

                    case ControlRightShift:   /* Switch to scope display. 940715 */
			if (buttondown) {
                            if ((errcode = DoScope( scpr, page )) < 0) goto clean;
                            if ((errcode = ShowPatchPage (page)) < 0) goto clean;
			}
                        break;
                }
            }
        }


            /* store last event */
        last_cped = cur_cped;

            /* switch screen buffers (also waits a vblank) */
        SwitchScreens();
    }

clean:
    StopPatch (pagelist->ppage_Patch, NULL);
	if( scpr ) DeleteScopeProbe( scpr );
    return errcode;
}


#if DEBUG_DumpPatch

/* -------------------- DumpPatch() */

static void DumpPatch (const Patch *patch)
{
    printf ("Patch: $%p\n", patch);

    if (patch) {
            /* display instruments */
        {
            PatchInstrument *inst;

            for (inst = (PatchInstrument *)FirstNode (&patch->patch_InstrumentList); IsNode(&patch->patch_InstrumentList,inst); inst = (PatchInstrument *)NextNode (inst)) {
                printf ("  Instrument \"%s\": template=\"%s\" item=%ld attr=$%02x knobtbl=$%p nknobs=%ld\n", inst->pinst_Symbol.psym_Node.n_Name, inst->pinst_TemplateName, inst->pinst_Instrument, inst->pinst_AttrFlags, inst->pinst_KnobTable, inst->pinst_NKnobs);

                {
                    int32 nknobs = inst->pinst_NKnobs;
                    PatchKnob *knob = inst->pinst_KnobTable;

                    for (; nknobs--; knob++) {
                        printf ("    Knob \"%s\": ", knob->pknob_Name);

                        if (IsPatchKnobGrabbed (knob)) {
                            TagArg getknobtags[] = {    /* @@@ order is assumed below */
                                { AF_TAG_CURRENT },
                                { TAG_END }
                            };

                            GetAudioItemInfo (knob->pknob_Knob, getknobtags);

                            printf ("grabbed val=%ld\n", (int32)getknobtags[0].ta_Arg);
                        }
                        else {
                            printf ("connected\n");
                        }
                    }
                }
            }
        }

            /* display samples */
        {
            PatchSample *samp;

            for (samp = (PatchSample *)FirstNode (&patch->patch_SampleList); IsNode(&patch->patch_SampleList,samp); samp = (PatchSample *)NextNode (samp)) {
                TagArg tags[] = {           /* @@@ order assumed below */
                    { AF_TAG_FRAMES },
                    { AF_TAG_CHANNELS },
                    { AF_TAG_WIDTH },
                    TAG_END
                };

                printf ("  Sample \"%s\":", samp->psamp_Symbol.psym_Node.n_Name);
                if (samp->psamp_FileName)
                    printf (" file=\"%s\"", samp->psamp_FileName);
                printf (" item=%ld", samp->psamp_Sample);
                if (!GetAudioItemInfo (samp->psamp_Sample, tags))
                    printf (" nframes=%lu nchannels=%lu width=%lu", (uint32)tags[0].ta_Arg, (uint32)tags[1].ta_Arg, (uint32)tags[2].ta_Arg);
                printf ("\n");
            }
        }

            /* display attachments */
        {
            PatchSampleAttachment *att;

            for (att = (PatchSampleAttachment *)FirstNode (&patch->patch_SampleAttachmentList); IsNode(&patch->patch_SampleAttachmentList,att); att = (PatchSampleAttachment *)NextNode (att)) {
                printf ("  Attachment \"%s\": item=%ld\n", att->psatt_Symbol.psym_Node.n_Name, att->psatt_Attachment);
            }
        }
    }
}

#endif  /* DEBUG_DumpPatch */
