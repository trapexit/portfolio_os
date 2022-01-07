/* $Id: tags.c,v 1.23 1995/02/24 03:45:05 vertex Exp $ */
/* file: tags.c */

#include "types.h"
#include "item.h"
#include "kernel.h"
#include "operror.h"
#include "internalf.h"
#include "tags.h"

#define DBUG(x)	/*printf x*/

int32
internalTagProcessor(void *np, TagArg *tagpt, int32 (*callback)(), void *dataP,
                     bool cloneName)
{
    ItemNode *n = (ItemNode *)np;
    uint32 *tagp = (uint32 *)tagpt;
    uint32 tagc,taga;
    jmp_buf jb;
    jmp_buf *old_co;
    int jump_tag_count = 0;
    int32 ret = 0;

    DBUG(("TagProcessor n=%lx tagpt=%lx callback=%lx\n",n,tagpt,callback));
    if (tagpt == 0)	return 0;	/* none supplied */
    old_co = KernelBase->kb_CatchDataAborts;
    KernelBase->kb_CatchDataAborts = &jb;

    if (setjmp(jb))
    {
	ret = BADPTR;
	goto done;
    }

    while ((tagc = *tagp++) != (uint32)0) {
	taga = *tagp++;
	DBUG(("tagc=%lx taga=%lx\n",tagc,taga));
	switch (tagc) {
	    case TAG_ITEM_PRI:	n->n_Priority = (uint8)taga;
				DBUG(("TAG_ITEM_PRI\n"));
				break;
	    case TAG_ITEM_NAME:
			if (taga == 0)
			{
			    ret = BADNAME;
			    goto done;
			}
			if (cloneName)
			{
                            n->n_Name = AllocateName((char *)taga);
                            DBUG(("TAG_ITEM_NAME\n"));
                            if (!n->n_Name)
                            {
                                DBUG(("TagProcessor returning NOMEM\n"));
                                DBUG(("name=%s\n",taga));
                                ret = NOMEM;
                                goto done;
                            }
                        }
                        else
                        {
                            if (!IsLegalName((char *)taga))
                            {
                                ret = BADNAME;
                                goto done;
                            }
                            n->n_Name = (char *)taga;
                        }
			break;

	    case TAG_ITEM_UNIQUE_NAME:	n->n_ItemFlags |= (uint8)ITEMNODE_UNIQUE_NAME;
				DBUG(("TAG_ITEM_UNIQUE_NAME\n"));
				break;
	    case TAG_ITEM_VERSION:	n->n_Version = (uint8)taga;
				DBUG(("TAG_ITEM_VERSION\n"));
				break;
	    case TAG_ITEM_REVISION:	n->n_Revision = (uint8)taga;
				DBUG(("TAG_ITEM_REVISION\n"));
				break;
	    case TAG_JUMP:
				DBUG(("TAG_JUMP\n"));
				tagp = (uint32 *)taga;

				if (tagp == NULL)
				{
				    /* end of the road */
				    goto done;
				}

				/* simple tag loop check */
				if((++jump_tag_count > 20) ||
				    ((uint32 *)tagpt == tagp)) {
				    	ret = BADPTR;
				    	goto done;
				}
				break;
	    case TAG_NOP:	DBUG(("TAG_NOP\n"));
				break;
	    default :   if (callback)
                            ret = (*callback)(n,dataP,tagc,taga);
                        else
                            ret = BADTAG;

			if (ret < 0)	goto done;
	}
    }
    DBUG(("End of tag args\n"));

done:
    if (ret < 0)
    {
        if (cloneName)
            FreeString(n->n_Name);

	n->n_Name = NULL;
    }
    KernelBase->kb_CatchDataAborts = old_co;
    return ret;
}

int32
TagProcessor(np,tagpt,callback,dataP)
void *np;
TagArg *tagpt;
int32 (*callback)();
void *dataP;
{
    return internalTagProcessor(np,tagpt,callback,dataP,TRUE);
}

int32
TagProcessorNoAlloc(np,tagpt,callback,dataP)
void *np;
TagArg *tagpt;
int32 (*callback)();
void *dataP;
{
    return internalTagProcessor(np,tagpt,callback,dataP,FALSE);
}

int32 TagProcessorSearch(TagReturn, tagpt, target_tag)
TagArg *TagReturn;
TagArg *tagpt;
uint32 target_tag;
{
    uint32 *tagp = (uint32 *)tagpt;
    uint32 tagc,taga;
    jmp_buf jb;
    jmp_buf *old_co;
    int32 ret = 0;
    int jump_tag_count = 0;

    DBUG(("TagProcessorSearch tagpt=%lx, target_tag=%lx\n",tagpt, target_tag));
    if (tagpt == 0)	return 0;	/* none supplied */
    old_co = KernelBase->kb_CatchDataAborts;
    KernelBase->kb_CatchDataAborts = &jb;

    if (setjmp(jb))
    {
	ret = BADPTR;
	goto done;
    }

    while ((tagc = *tagp++) != (uint32)0) {
	taga = *tagp++;
	DBUG(("tagc=%lx taga=%lx\n",tagc,taga));
	if(tagc == target_tag) {
	    ret = 1;
	    TagReturn->ta_Tag = target_tag;
	    TagReturn->ta_Arg = (void *)taga;
	}
	switch (tagc) {
	    case TAG_JUMP:
				DBUG(("TAG_JUMP\n"));
				tagp = (uint32 *)taga;

				if (tagp == NULL)
				{
				    /* end of the road */
				    goto done;
				}

				/* simple tag loop check */
				if ((++jump_tag_count > 20) ||
				    ((uint32 *)tagpt == tagp)) {
					/* ret = BADPTR; */
					goto done;
				}
				break;
	    case TAG_NOP:	DBUG(("TAG_NOP\n"));
				break;
	    default:		break;
	}
    }
    DBUG(("End of tag args\n"));

done:
    KernelBase->kb_CatchDataAborts = old_co;
    return ret;
}


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/kernel/nexttagarg
|||	NextTagArg - Find the next TagArg in a tag list.
|||
|||	  Synopsis
|||
|||	    TagArg *NextTagArg( const TagArg **tagList );
|||
|||	  Desription
|||
|||	    This function iterates through a tag list, skipping and chaining
|||	    as dictated by control tags. There are three control tags:
|||
|||	       TAG_NOP
|||	       Ignores that single entry and moves to the next one.
|||
|||	       TAG_JUMP
|||	       Has a pointer to another array of tags.
|||
|||	       TAG_END
|||	       Marks the end of the tag list.
|||
|||	    This function only returns TagArgs which are not system tags.
|||	    Each call returns either the next TagArg you should examine,
|||	    or NULL when the end of the list has been reached.
|||
|||	  Arguments
|||
|||	    tagList                     This is a pointer to a storage location
|||	                                used by the iterator to keep track of
|||	                                its current location in the tag list.
|||	                                The variable that this parameter
|||	                                points to should be initialized to
|||	                                point to the first TagArg in the tag
|||	                                list, and should not be changed
|||	                                thereafter.
|||
|||	  Return Value
|||
|||	    This function returns a pointer to a TagArg structure, or NULL if
|||	    all the tags have been visited. None of the control tags are ever
|||	    returned to you, they are handled transparently by this function.
|||
|||	  Example
|||
|||	    void WalkTagList(const TagArg *tags)
|||	    {
|||	    TagArg *state;
|||	    TagArg *currentTag;
|||
|||	        state = tags;
|||		while ((tag = NextTagItem(&state)) != NULL)
|||	        {
|||		    switch (tag->ta_Tag)
|||		    {
|||	 		case TAG1: // process this tag
|||				   break;
|||
|||	 		case TAG2: // process this tag
|||				   break;
|||
|||			default  : // unknown tag, return an error
|||	                           break;
|||	            }
|||	   	}
|||	    }
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V24.
|||
|||	  Associated Files
|||
|||	    tags.h
|||
|||	  See Also
|||
|||	    FindTagArg()
|||
**/

TagArg *NextTagArg(const TagArg **tagList)
{
const TagArg *t = *tagList;

    while (t)
    {
        switch (t->ta_Tag)
        {
            case TAG_END : t = NULL;
                           break;

            case TAG_NOP : t++;
                           break;

            case TAG_JUMP: t = (TagArg *)t->ta_Arg;
                           break;

            default      : *tagList = &t[1];
                           return (TagArg *)t;
        }
    }

    return NULL;
}


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/kernel/findtagarg
|||	FindTagArg - Look through a tag list for a specific tag.
|||
|||	  Synopsis
|||
|||	    TagArg *FindTagArg( const TagArg *tagList, uint32 tag );
|||
|||	  Desription
|||
|||	    This function scans a tag list looking for a TagArg structure
|||	    with a ta_Tag field equal to the tag parameter. The function
|||	    always returns the last TagArg structure in the list which matches
|||	    the tag. Finally, this function deals with the various control
|||	    tags such as TAG_JUMP and TAG_NOP.
|||
|||	  Arguments
|||
|||	    tagList                     The list of tags to scan.
|||
|||	    tag                         The value to look for.
|||
|||	  Return Value
|||
|||	    This function returns a pointer to a TagArg structure with
|||	    a value of ta_Tag that matches the tag parameter, ot NULL if
|||	    no match can be found. The function always returns the last tag
|||	    in the list which matches.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V24.
|||
|||	  Associated Files
|||
|||	    tags.h
|||
|||	  See Also
|||
|||	    NextTagArg()
|||
**/

TagArg *FindTagArg(const TagArg *tagList, uint32 tag)
{
TagArg *ta;
TagArg *result;

    result = NULL;
    while ((ta = NextTagArg(&tagList)) != NULL)
    {
        if (ta->ta_Tag == tag)
            result = ta;
    }

    return result;
}
