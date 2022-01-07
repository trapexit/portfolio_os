/* list routines in macro format */

#define l_Tail ListAnchor.tail.links.blink
#define l_Head ListAnchor.head.links.flink
#define l_Middle ListAnchor.head.links.blink

#ifdef ROMBUILD
#define ADDHEAD(l,n) \
	{ \
	    Node *currenthead = FIRSTNODE(l); \
	    ((Node *)n)->n_Next = currenthead; \
	    ((Node *)n)->n_Prev = (Node *)&((List *)l)->l_Head; \
	    ((List *)l)->l_Head = (Link *)n; \
	    currenthead->n_Prev = (Node *)n; \
	}
#define ADDTAIL(l,n) \
	{ \
	    Node *currentlast = LASTNODE(l); \
	    ((List *)l)->l_Tail = (Link *)n; \
	    ((Node *)n)->n_Prev = currentlast; \
	    ((Node *)n)->n_Next = (Node *)&((List *)l)->l_Middle; \
	    currentlast->n_Next = (Node *)n; \
	}

/* RemoveNode with no return value */
#define REMOVENODE(n)	{ \
			    Node *r1 = (n)->n_Next; \
			    Node *r2 = (n)->n_Prev; \
			    r2->n_Next = r1; \
			    r1->n_Prev = r2; \
			}

#else
/* Use the misc code */
#define ADDHEAD(l,n) AddHead(l,n)
#define ADDTAIL(l,n) AddTail(l,n)
#define REMOVENODE(n) RemNode(n)
#define REMHEAD(var,l) var = RemHead(l)
#endif

#ifdef undef
/* This stuff here is unfinished! */

	n->n_Prev->n_Next = n->n_Next;
	n->Next->n_Prev = n->n_Prev;

/* REMHEAD(f = (Firq *),list) */

#define REMHEAD(var,l) \
		{ \
		    Node *n = FIRSTNODE(l); \
		    if (!ISNODE(l,n)) var 0; \
		    else \
		    { \
			REMOVENODE(n); \
			var n; \
		    } \
		}


 ((l)->l_Head == &(l)->l_Middle ? 0 : RemNode((l)->l_Head) )
#endif

