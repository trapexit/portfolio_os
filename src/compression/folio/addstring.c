/* $Id: addstring.c,v 1.1 1994/08/17 16:34:58 vertex Exp $ */


/*****************************************************************************/


/* NOTE: This function should be part of the compress.c source file, but
 *       is kept here separately because of a compiler bug in the ARM 1.61
 *       compiler. AddString() must be compiled without optimization, or
 *       it will not work. If the compiler gets fixed, performance would
 *       improve if this function were put back into compress.c, and
 *	 was compiled with the optimizer turned on.
 *
 *	 If this function gets merged back into compress.c, then the
 *	 definitions from the bottom section of lzss.h can be moved into
 *	 compress.c
 */


/*****************************************************************************/


#include "types.h"
#include "string.h"
#include "lzss.h"


/*****************************************************************************/


/* This is where most of the encoder's work is done. This routine is
 * responsible for adding the new node to the binary tree. It also has to
 * find the best match among all the existing nodes in the tree, and return
 * that to the calling routine. To make matters even more complicated, if
 * the newNode has a duplicate in the tree, the oldNode is deleted, for
 * reasons of efficiency.
 */

uint32 AddString(CompNode *tree, unsigned char *window,
                 uint32 newNode, uint32 *matchPos)
{
uint32    i;
uint32    testNode;
uint32    parentNode;
int32     delta;
uint32    matchLen;
CompNode *node;
CompNode *parent;
CompNode *test;

    if (newNode == END_OF_STREAM)
        return (0);

    testNode = tree[TREE_ROOT].cn_RightChild;
    node     = &tree[newNode];
    matchLen = 0;

    while (TRUE)
    {
        for (i = 0; i < LOOK_AHEAD_SIZE; i++)
        {
            delta = window[MOD_WINDOW(newNode + i)] - window[MOD_WINDOW(testNode + i)];
            if (delta)
                break;
        }

        test = &tree[testNode];

        if (i >= matchLen)
        {
            matchLen  = i;
            *matchPos = testNode;
            if (matchLen >= LOOK_AHEAD_SIZE)
            {
                parentNode = test->cn_Parent;
                parent = &tree[parentNode];

                if (parent->cn_LeftChild == testNode)
                    parent->cn_LeftChild = newNode;
                else
                    parent->cn_RightChild = newNode;

                *node                               = *test;
                tree[node->cn_LeftChild].cn_Parent  = newNode;
                tree[node->cn_RightChild].cn_Parent = newNode;
                test->cn_Parent                     = UNUSED;

                return (matchLen);
            }
        }

        if (delta >= 0)
        {
            if (test->cn_RightChild == UNUSED)
            {
                test->cn_RightChild = newNode;
                node->cn_Parent     = testNode;
                node->cn_LeftChild  = UNUSED;
                node->cn_RightChild = UNUSED;
                return (matchLen);
            }
            testNode = test->cn_RightChild;
        }
        else
        {
            if (test->cn_LeftChild == UNUSED)
            {
                test->cn_LeftChild  = newNode;
                node->cn_Parent     = testNode;
                node->cn_LeftChild  = UNUSED;
                node->cn_RightChild = UNUSED;
                return (matchLen);
            }
            testNode = test->cn_LeftChild;
        }
    }
}
