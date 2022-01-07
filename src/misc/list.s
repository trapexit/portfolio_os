	;; $Id: list.s,v 1.18 1994/09/14 16:20:01 vertex Exp $
; file: list.s
; List management kernel routines
;
	GET	structs.i
	GET	nodes.i
	GET	list.i

;;;	AUTODOC PUBLIC spg/kernel/scanlist
;;;	ScanList - Walk through all the nodes in a list.
;;;
;;;	  Synopsis
;;;
;;;	    ScanList(List *l, void *n, <node type>)
;;;
;;;	  Description
;;;
;;;	    This macro lets you easily walk through all the elements
;;;	    in a list from the first to the last.
;;;
;;;	    For example:
;;;
;;;	    List       *l;
;;;	    DataStruct *d;
;;;	    uint32      i;
;;;
;;;	    i = 0;
;;;	    ScanList(l,d,DataStruct)
;;;	    {
;;;	        printf("Node %d is called %s\n",i,d->d.n_Name);
;;;	        i++;
;;;	    }
;;;
;;;	  Arguments
;;;
;;;	    l                           A pointer to the list to scan.
;;;
;;;	    n                           A variable which will be altered
;;;	                                to hold a pointer to every node in
;;;	                                the list in succession.
;;;
;;;	    <node type>                 The data type of the nodes on the
;;;	                                list. This is used for type casting
;;;	                                within the macro.
;;;
;;;	  Implementation
;;;
;;;	    Macro implemented in list.h V22.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Macro definition
;;;
;;;	  See Also
;;;
;;;	    ScanListB()
;;;

;;;	AUTODOC PUBLIC spg/kernel/scanlistb
;;;	ScanListB - Walk through all the nodes in a list backwards.
;;;
;;;	  Synopsis
;;;
;;;	    ScanListB(List *l, void *n, <node type>)
;;;
;;;	  Description
;;;
;;;	    This macro lets you easily walk through all the elements
;;;	    in a list from the last to the first.
;;;
;;;	    For example:
;;;
;;;	    List       *l;
;;;	    DataStruct *d;
;;;	    uint32      i;
;;;
;;;	    i = 0;
;;;	    ScanListB(l,d,DataStruct)
;;;	    {
;;;	        printf("Node %d (counting from the end) is called %s\n",i,
;;;	               d->d.n_Name);
;;;	        i++;
;;;	    }
;;;
;;;	  Arguments
;;;
;;;	    l                           A pointer to the list to scan.
;;;
;;;	    n                           A variable which will be altered
;;;	                                to hold a pointer to every node in
;;;	                                the list in succession.
;;;
;;;	    <node type>                 The data type of the nodes on the
;;;	                                list. This is used for type casting
;;;	                                within the macro.
;;;
;;;	  Implementation
;;;
;;;	    Macro implemented in list.h V24.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Macro definition
;;;
;;;	  See Also
;;;
;;;	    ScanList()
;;;

;;;	AUTODOC PUBLIC spg/kernel/firstnode
;;;	FirstNode - Get the first node in a list.
;;;
;;;	  Synopsis
;;;
;;;	    Node *FirstNode( List *l )
;;;
;;;	  Description
;;;
;;;	    This macro returns a pointer to the first node in the specified
;;;	    list.  If the list is empty, the macro returns a pointer to the
;;;	    tail (end-of-list) anchor.  To determine if the return value is an
;;;	    actual node rather than the tail anchor, call the IsNode()
;;;	    procedure.
;;;
;;;	    Example 15-2                Use of FirstNode in forward list
;;;	                                traversal
;;;
;;;	    for (n = FirstNode(list); IsNode(list, n); n = NextNode( n ))
;;;
;;;	                                        {    . . .    };
;;;
;;;
;;;	  Arguments
;;;
;;;	    l                           A pointer to the list from which to
;;;	                                get the first node.
;;;
;;;	  Return Value
;;;
;;;	    The macro returns a pointer to first node in the list or, if the
;;;	    list is empty, a pointer to the tail (end-of-list) anchor.
;;;
;;;	  Implementation
;;;
;;;	    Macro implemented in list.h V20.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Macro definition
;;;
;;;	  See Also
;;;
;;;	    IsListEmpty(), IsNode(), IsNodeB(), LastNode(), NextNode(),
;;;	    PrevNode(), ScanList()
;;;

;;;	AUTODOC PUBLIC spg/kernel/insertnodefromhead
;;;	InsertNodeFromHead - Insert a node into a list.
;;;
;;;	  Synopsis
;;;
;;;	    void InsertNodeFromHead( List *l, Node *n )
;;;
;;;	  Description
;;;
;;;	    This procedure inserts a new node into a list.  The order of nodes
;;;	    in a list is normally determined by their priority.  The procedure
;;;	    compares the priority of the new node to the priorities of nodes
;;;	    currently in the list, beginning at the head of the list, and
;;;	    inserts the new node immediately after all nodes whose priority is
;;;	    higher.  If the priorities of all the nodes in the list are
;;;	    higher, the node is added at the end of the list.
;;;
;;;	  Arguments
;;;
;;;	    l                           A pointer to the list into which to
;;;	                                insert the node.
;;;
;;;	    n                           A pointer to the node to insert.
;;;
;;;	  Implementation
;;;
;;;	    Folio call implemented in kernel folio V20.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Prototype
;;;
;;;	    clib.lib                    ARM Link Library
;;;
;;;	  Notes
;;;
;;;	    To arrange the nodes in a list by a value or values other than
;;;	    priority, use UniversalInsertNode().
;;;
;;;	    A node can only be included in one list.
;;;
;;;	  See Also
;;;
;;;	    AddHead(), AddTail(), RemHead(), RemNode(), RemTail(),
;;;	    InsertNodeFromTail(), UniversalInsertNode()
;;;

;;;	AUTODOC PUBLIC spg/kernel/insertnodefromtail
;;;	InsertNodeFromTail - Insert a node into a list.
;;;
;;;	  Synopsis
;;;
;;;	    void InsertNodeFromTail( List *l, Node *n )
;;;
;;;	  Description
;;;
;;;	    This procedure inserts a new node into a list.  The order of nodes
;;;	    in a list is determined by their priority.  The procedure compares
;;;	    the priority of the new node to the priorities of nodes currently
;;;	    in the list, beginning at the tail of the list, and inserts the
;;;	    new node immediately before the nodes whose priority is lower.  If
;;;	    there are no nodes in the list whose priority is lower, the node
;;;	    is added at the head of the list.
;;;
;;;	  Arguments
;;;
;;;	    l                           A pointer to the list into which to
;;;	                                insert the node.
;;;
;;;	    n                           A pointer to the node to insert.
;;;
;;;	  Implementation
;;;
;;;	    Folio call implemented in kernel folio V20.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Prototype
;;;
;;;	    clib.lib                    ARM Link Library
;;;
;;;	  Notes
;;;
;;;	    To arrange the nodes in a list by a value or values other than
;;;	    priority, use UniversalInsertNode().
;;;
;;;	    A node can only be included in one list.
;;;
;;;	  See Also
;;;
;;;	    AddHead(), AddTail(), RemHead(), RemNode(), RemTail(),
;;;	    InsertNodeFromHead(), UniversalInsertNode()
;;;

;;;	AUTODOC PUBLIC spg/kernel/isemptylist
;;;	IsEmptyList - Check whether a list is empty.
;;;
;;;	  Synopsis
;;;
;;;	    bool IsEmptyList( List *l )
;;;
;;;	  Description
;;;
;;;	    This macro checks whether a list is empty.
;;;
;;;	  Arguments
;;;
;;;	    l                           A pointer to the list to check.
;;;
;;;	  Return Value
;;;
;;;	    The macro returns TRUE if the list is empty or FALSE if it isn't.
;;;
;;;	  Implementation
;;;
;;;	    Macro implemented in list.h V20.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Macro definition
;;;
;;;	  See Also
;;;
;;;	    FirstNode(), IsNode(), IsNodeB(), LastNode(), NextNode(),
;;;	    PrevNode(), ScanList(), IsListEmpty()
;;;

;;;	AUTODOC PUBLIC spg/kernel/islistempty
;;;	IsListEmpty - Check whether a list is empty.
;;;
;;;	  Synopsis
;;;
;;;	    bool IsListEmpty( List *l )
;;;
;;;	  Description
;;;
;;;	    This macro checks whether a list is empty.
;;;
;;;	  Arguments
;;;
;;;	    l                           A pointer to the list to check.
;;;
;;;	  Return Value
;;;
;;;	    The macro returns TRUE if the list is empty or FALSE if it isn't.
;;;
;;;	  Implementation
;;;
;;;	    Macro implemented in list.h V24.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Macro definition
;;;
;;;	  See Also
;;;
;;;	    FirstNode(), IsNode(), IsNodeB(), LastNode(), NextNode(),
;;;	    PrevNode(), ScanList()
;;;

;;;	AUTODOC PUBLIC spg/kernel/isnode
;;;	IsNode - Checks that a node pointer is not the forward list anchor.
;;;
;;;	  Synopsis
;;;
;;;	    bool IsNode( List *l, Node *n )
;;;
;;;	  Description
;;;
;;;	    This macro is used to test whether the specified node is an actual
;;;	    node or is the tail (end-of-list) anchor.  Use this macro when
;;;	    traversing a list from head to tail.  When traversing a list from
;;;	    tail to head, use the IsNodeB() macro.
;;;
;;;	  Arguments
;;;
;;;	    l                           A pointer to the list containing the
;;;	                                node to check.
;;;
;;;	    n                           A pointer to the node to check.
;;;
;;;	  Return Value
;;;
;;;	    The macro returns TRUE if the node is an actual node or FALSE if
;;;	    it is the tail (end-of-list) anchor.
;;;
;;;	    Note:  This macro currently returns TRUE for any node that is not
;;;	    the tail anchor, whether or not the node is in the specified list.
;;;
;;;	  Implementation
;;;
;;;	    Macro implemented in list.h V20.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Macro Definition
;;;
;;;	  See Also
;;;
;;;	    FirstNode(), IsListEmpty(), IsNodeB(), LastNode(), NextNode(),
;;;	    PrevNode(), ScanList()
;;;

;;;	AUTODOC PUBLIC spg/kernel/isnodeb
;;;	IsNodeB - Checks that a node pointer is not the reverse list anchor.
;;;
;;;	  Synopsis
;;;
;;;	    bool IsNodeB( List *l, Node *n )
;;;
;;;	  Description
;;;
;;;	    This macro is used to test whether the specified node is an actual
;;;	    node or is the head (beginning-of-list) anchor.  Use this macro
;;;	    when traversing a list from tail to head.  When traversing a list
;;;	    from head to tail, use the IsNode() macro.
;;;
;;;	  Arguments
;;;
;;;	    l                           A pointer to the list containing the
;;;	                                node to check.
;;;
;;;	    n                           A pointer to the node to check.
;;;
;;;	  Return Value
;;;
;;;	    The macro returns TRUE if the node is an actual node or FALSE if
;;;	    it is the head (beginning-of-list) anchor.
;;;
;;;	    Note:  This macro currently returns TRUE for any node that is not
;;;	    the head anchor, whether or not the node is in the specified list.
;;;
;;;	  Implementation
;;;
;;;	    Macro implemented in list.h V20.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Macro Definition
;;;
;;;	  See Also
;;;
;;;	    FirstNode(), IsListEmpty(), IsNode(), LastNode(), NextNode(),
;;;	    PrevNode(), ScanListB()
;;;

;;;	AUTODOC PUBLIC spg/kernel/lastnode
;;;	LastNode - Get the last node in a list.
;;;
;;;	  Synopsis
;;;
;;;	    Node *LastNode( List *l )
;;;
;;;	  Description
;;;
;;;	    This macro returns a pointer to the last node in a list.  If the
;;;	    list is empty, the macro returns a pointer to the head
;;;	    (beginning-of-list) anchor.  To determine if the return value is
;;;	    an actual node rather than the head anchor, call the IsNodeB()
;;;	    procedure.
;;;
;;;	    Example 15-3                Use of LastNode in reverse list
;;;	                                traversal
;;;
;;;	    for (n = LastNode(list); IsNodeB(list, n); n = PrevNode( n ))
;;;
;;;	                                        {    . . .    };
;;;
;;;
;;;	  Arguments
;;;
;;;	    l                           A pointer to the list structure to be
;;;	                                examined.
;;;
;;;	  Return Value
;;;
;;;	    The macro returns a pointer to last node in the list or, if the
;;;	    list is empty, a pointer to the head (beginning-of-list) anchor.
;;;
;;;	  Implementation
;;;
;;;	    Macro implemented in list.h V20.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Macro definition
;;;
;;;	  See Also
;;;
;;;	    FirstNode(), IsListEmpty(), IsNode(), IsNodeB(), NextNode(),
;;;	    PrevNode(), ScanList()
;;;

;;;	AUTODOC PUBLIC spg/kernel/nextnode
;;;	NextNode - Get the next node in a list.
;;;
;;;	  Synopsis
;;;
;;;	    Node *NextNode( Node *n )
;;;
;;;	  Description
;;;
;;;	    This macro gets a pointer to the next node in a list.  If the
;;;	    current node is the last node in the list, the result is a pointer
;;;	    to the tail (end-of-list) anchor.  To determine if the return
;;;	    value is an actual node rather than the tail anchor, call the
;;;	    IsNode() procedure.
;;;
;;;	    Example 15-4                Use of NextNode in forward list
;;;	                                traversal
;;;
;;;	    for (n = FirstNode( l ); IsNode( l, n ); n = NextNode( n ))
;;;
;;;	                                        {   . . .   };
;;;
;;;
;;;	  Arguments
;;;
;;;	    n                           Pointer to the current node.
;;;
;;;	  Return Value
;;;
;;;	    The macro returns a pointer to the next node in the list or, if
;;;	    the current node is the last node in the list, to the tail
;;;	    (end-of-list) anchor.
;;;
;;;	  Implementation
;;;
;;;	    Macro implemented in list.h V20.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Macro definition
;;;
;;;	  Caveats
;;;
;;;	    Assumes that n is a node in a list.  If not, watch out.
;;;
;;;	  See Also
;;;
;;;	    FirstNode(), IsListEmpty(), IsNode(), IsNodeB(), LastNode(),
;;;	    PrevNode(), ScanList()
;;;

;;;	AUTODOC PUBLIC spg/kernel/prevnode
;;;	PrevNode - Get the previous node in a list.
;;;
;;;	  Synopsis
;;;
;;;	    Node *PrevNode( Node *node )
;;;
;;;	  Description
;;;
;;;	    This macro returns a pointer to the previous node in a list.  If
;;;	    the current node is the first node in the list, the result is a
;;;	    pointer to the head (beginning-of-list) anchor.  To determine
;;;	    whether the return value is an actual node rather than the head
;;;	    anchor, use the IsNodeB() procedure.
;;;
;;;	    Example 15-5                Use of PrevNode in reverse list
;;;	                                traversal
;;;
;;;	    for (n = LastNode( l ); IsNodeB( l, n ); n = PrevNode( n ))
;;;
;;;	                                        {   . . .   };
;;;
;;;
;;;	  Arguments
;;;
;;;	    node                        A pointer to the current node.
;;;
;;;	  Return Value
;;;
;;;	    The macro returns a pointer to the previous node in the list or,
;;;	    if the current node is the first node in the list, to the head
;;;	    (beginning-of-list) anchor.
;;;
;;;	  Implementation
;;;
;;;	    Macro implemented in list.h V20.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Macro definition
;;;
;;;	  See Also
;;;
;;;	    FirstNode(), IsListEmpty(), IsNode(), IsNodeB(), LastNode(),
;;;	    ScanList()
;;;

;;;	AUTODOC PUBLIC spg/kernel/remhead
;;;	RemHead - Remove the first node from a list.
;;;
;;;	  Synopsis
;;;
;;;	    Node *RemHead( List *l )
;;;
;;;	  Description
;;;
;;;	    This procedure removes the head (first) node from a list.
;;;
;;;	  Arguments
;;;
;;;	    l                           A pointer to the list to be beheaded.
;;;
;;;	  Return Value
;;;
;;;	    The procedure returns a pointer to the node that was removed from
;;;	    the list or NULL if the list is empty.
;;;
;;;	  Implementation
;;;
;;;	    Folio call implemented in kernel folio V20.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Prototype
;;;
;;;	    clib.lib                    ARM Link Library
;;;
;;;	  See Also
;;;
;;;	    AddHead(), AddTail(), InsertNodeFromTail(), RemNode(), RemTail()
;;;

;;;	AUTODOC PUBLIC spg/kernel/remnode
;;;	RemNode - Remove a node from a list.
;;;
;;;	  Synopsis
;;;
;;;	    void RemNode( Node *n )
;;;
;;;	  Description
;;;
;;;	    This procedure removes the specified node from a list.
;;;
;;;	    Note:  If the specified node structure is not in a list you may
;;;	    get an abort.
;;;
;;;	  Arguments
;;;
;;;	    n                           A pointer to the node to remove.
;;;
;;;	  Implementation
;;;
;;;	    Folio call implemented in kernel folio V20.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Prototype
;;;
;;;	    clib.lib                    ARM Link Library
;;;
;;;	  See Also
;;;
;;;	    AddHead(), AddTail(), InsertNodeFromTail(), RemHead(), RemTail()
;;;

;;;	AUTODOC PUBLIC spg/kernel/remtail
;;;	RemTail - Remove the last node from a list.
;;;
;;;	  Synopsis
;;;
;;;	    Node *RemTail( List *l )
;;;
;;;	  Description
;;;
;;;	    This procedure removes the tail (last) node from a list.
;;;
;;;	  Arguments
;;;
;;;	    l                           A pointer to the list structure that
;;;	                                will have its tail removed.
;;;
;;;	  Return Value
;;;
;;;	    The procedure returns a pointer to the node that was removed from
;;;	    the list or NULL if the list is empty.  The anchor nodes are never
;;;	    returned.
;;;
;;;	  Implementation
;;;
;;;	    Folio call implemented in kernel folio V20.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Prototype
;;;
;;;	    clib.lib                    ARM Link Library
;;;
;;;	  See Also
;;;
;;;	    AddHead(), AddTail(), InsertNodeFromTail(), RemHead(), RemNode()
;;;

;;;	AUTODOC PUBLIC spg/kernel/addhead
;;;	AddHead - Add a node to the head of a list.
;;;
;;;	  Synopsis
;;;
;;;	    void AddHead( List *l, Node *n )
;;;
;;;	  Description
;;;
;;;	    This procedure adds a node to the head (the beginning) of the
;;;	    specified list.
;;;
;;;	  Arguments
;;;
;;;	    l                           A pointer to the list in which to add
;;;	                                the node.
;;;
;;;	    n                           A pointer to the node to add.
;;;
;;;	  Implementation
;;;
;;;	    Folio call implemented in kernel folio V20.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Prototype
;;;
;;;	    clib.lib                    ARM Link Library
;;;
;;;	  Notes
;;;
;;;	    A node can be included in only one list.
;;;
;;;	  Caveats
;;;
;;;	    Attempting to insert a node into a list while  it is a member of
;;;	    another list is not reported as an error, and may confuse the
;;;	    other list.
;;;
;;;	  See Also
;;;
;;;	    AddTail(), InitList(), InsertNodeFromHead(), InsertNodeFromTail(),
;;;	    RemHead(), RemNode(), RemTail(), UniversalInsertNode()
;;;

;;;	AUTODOC PUBLIC spg/kernel/addtail
;;;	AddTail - Add a node to the tail of a list.
;;;
;;;	  Synopsis
;;;
;;;	    void AddTail( List *l, Node *n )
;;;
;;;	  Description
;;;
;;;	    This procedure adds the specified node to the tail (the end) of
;;;	    the specified list.
;;;
;;;	  Arguments
;;;
;;;	    l                           A pointer to the list in which to add
;;;	                                the node.
;;;
;;;	    n                           A pointer to the node to add.
;;;
;;;	  Implementation
;;;
;;;	    Folio call implemented in kernel folio V20.
;;;
;;;	  Associated Files
;;;
;;;	    list.h                      ANSI C Prototype
;;;
;;;	    clib.lib                    ARM Link Library
;;;
;;;	  Notes
;;;
;;;	    A node can be included only in one list.
;;;
;;;	  Caveats
;;;
;;;	    Attempting to insert a node into a list while  it is a member of
;;;	    another list is not reported as an error, and may confuse the
;;;	    other list.
;;;
;;;	  See Also
;;;
;;;	    AddHead(), InitList(), InsertNodeFromHead(), InsertNodeFromTail(),
;;;	    RemHead(), RemNode(), RemTail(), UniversalInsertNode()
;;;

	AREA	|ASMCODE|, CODE, READONLY

	ALIGN	4		; alignment paranoia

	EXPORT	AddHead
;
;	AddHead(List *, Node *)
;		r0,	r1
AddHead
	ldr	r2,[r0,#l_Head]	; get current head node
	str	r2,[r1,#N_Next]
	add	r3,r0,#l_Head	; ptr to anchor
	str	r3,[r1,#N_Prev] ; anchor the head
	str	r1,[r0,#l_Head] ; new head node
	str	r1,[r2,#N_Prev]	; fix up next node
	mov	r15,r14		; return

	EXPORT	RemHead
;
;	Node *RemHead(List *)
;
RemHead
	mov	r1,r0		; use r1 for list head
	ldr	r0,[r1,#l_Head]	; get head of list
	add	r3,r1,#l_Middle	; compute anchor ptr
	cmp	r0,r3		; empty list?
	moveq	r0,#0
	moveq	r15,r14		; return on empty list
	ldr	r2,[r0,#N_Next]	; get next node
	str	r2,[r1,#l_Head]
	sub	r3,r3,#4	; anchor for prev ptr
	str	r3,[r2,#N_Prev]	; anchor new first node
	mov	r15,r14		; return

	EXPORT	RemTail
;
;	Node *RemTail(List *)
;
;;;	******************************************************************
;;; 	**			WARNING!				**
;;;	**								**
;;;	**	The original version of this function			**
;;;	**	delivered on many, many CD-ROMs has a small		**
;;;	**	bug: it does not increment the anchor pointer		**
;;;	**	by four bytes, and thus leaves the NEXT			**
;;;	**	pointer of the final element pointing to the		**
;;;	**	head node.						**
;;;	**								**
;;;	**	The kernel recognizes this by seeing if the		**
;;;	**	sixth instruction is a "moveq r15,r14", and if		**
;;;	**	it is, it changes it to "addne r3,r3,#4" and		**
;;;	**	compliments the top four bits of the next		**
;;;	**	three instructions (changing unconditionals		**
;;;	**	into "ne" conditionals).				**
;;;	**								**
;;;	**	So, don't change the "addne" back into "moveq"		**
;;;	**	or you will trigger the kernel's binary			**
;;;	**	patcher!						**
;;;	******************************************************************

RemTail
	mov	r1,r0		; use r1 for list head
	ldr	r0,[r1,#l_Tail]	; get tail of list
	add	r3,r1,#l_Head	; compute anchor ptr
	cmp	r0,r3		; empty list?
	moveq	r0,#0
	addne	r3,r3,#4	; get tail anchor ptr
	ldrne	r2,[r0,#N_Prev]	; get prev node
	strne	r2,[r1,#l_Tail]	; new last node
	strne	r3,[r2,#N_Next]	; anchor new last node
	mov	r15,r14		; return

	EXPORT	RemNode
	EXPORT	RemoveNode
;
;	void RemNode(Node *)
;			r0
RemNode
RemoveNode
	ldr	r1,[r0,#N_Next]

	cmp	r1,#0
	beq	rem_exit ;illegal!

	ldr	r2,[r0,#N_Prev]
	str	r1,[r2,#N_Next]
	str	r2,[r1,#N_Prev]

	mov	r1, #0	; clear the pointer
	str	r1,[r0,#N_Next]
rem_exit
	mov	r15,r14

	EXPORT	AddTail
;
;	AddTail(List *, Node *)
;		r0,	r1
AddTail
	ldr	r2,[r0,#l_Tail]	; get current last node
	str	r1,[r0,#l_Tail]	; new last node
	str	r2,[r1,#N_Prev] ; set up ptr to previous node
	add	r3,r0,#l_Middle	; address of anchor
	str	r3,[r1,#N_Next]	; anchor new last node
	str	r1,[r2,#N_Next]	; old last node ptrs to this node.
	mov	r15,r14

	EXPORT	TailInsertNode
	EXPORT	InsertNodeFromTail
;
;	InsertTail(List *, Node *)
;	Insert node in list in priority position
;	If there are other nodes in the list with the
;	same priority, put this node after the last
;	node of the same priority
;
TailInsertNode
InsertNodeFromTail
	ldr	r3,[r0,#l_Tail]
	add	r12,r0,#l_Head	; compute anchor
	cmp	r3,r12		; empty list?
	beq	AddTail
;	r12 contains the anchor
;	r1 ptr to Node being inserted
;	r0 will be the Nodes priority
;	r3 is current node we are looking at
;	r2 is tmp Priority
;	now paw through list until we find first node
;	or a node that is the same or greater than r1
	ldrb	r0,[r1,#N_Priority] ; get our nodes priority

	ldrb	r2,[r3,#N_Priority] ; get nodes priority
	cmp	r2,r0
	bge	AFound

LookAgain
	ldr	r3,[r3,#N_Prev]	; get previous node
	cmp	r3,r12		; end of list?
	subeq	r0,r12,#l_Head	; compute list ptr from anchor
	beq	AddHead			; top of list
	ldrb	r2,[r3,#N_Priority]
	cmp	r2,r0
	blt	LookAgain

AFound
;	put Node r1 after Node r3
	str	r3,[r1,#N_Prev]
	ldr	r0,[r3,#N_Next]
	str	r0,[r1,#N_Next]
	str	r1,[r3,#N_Next]
	str	r1,[r0,#N_Prev]
	mov	pc,r14

	EXPORT	InsertNodeFromHead
;
;	InsertHead(List *, Node *)
;		     r0  ,  r1
;	Insert node in list in priority position
;	If there are other nodes in the list with the
;	same priority, put this node before all
;	nodes of the same priority
;
InsertNodeFromHead
	ldr	r3,[r0,#l_Head]
	add	r12,r0,#l_Middle	; compute anchor
	cmp	r3,r12		; empty list?
	beq	AddHead
;	r12 contains the anchor
;	r1 ptr to Node being inserted
;	r0 will be the Nodes priority
;	r3 is current node we are looking at
;	r2 is tmp Priority
;	now paw through list until we find first node
;	or a node that is the same or greater than r1
	ldrb	r0,[r1,#N_Priority] ; get our nodes priority

	ldrb	r2,[r3,#N_Priority] ; get nodes priority
	cmp	r2,r0
	ble	HAFound

HLookAgain
	ldr	r3,[r3,#N_Next]	; get previous node
	cmp	r3,r12		; end of list?
	subeq	r0,r12,#l_Middle	; compute list ptr from anchor
	beq	AddHead			; top of list
	ldrb	r2,[r3,#N_Priority]
	cmp	r2,r0
	bgt	HLookAgain

HAFound
;	put Node r1 before Node r3
	str	r3,[r1,#N_Next]
	ldr	r0,[r3,#N_Prev]
	str	r0,[r1,#N_Prev]
	str	r1,[r3,#N_Prev]
	str	r1,[r0,#N_Next]
	mov	pc,r14


	nop
	nop
	nop
	nop

	EXPORT hang_remnode

hang_remnode
	b hang_remnode

	END
