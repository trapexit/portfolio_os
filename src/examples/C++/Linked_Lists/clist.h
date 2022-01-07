#ifndef _CLIST_H_
#define _CLIST_H_

/******************************************************************************
**
**  $Id: clist.h,v 1.3 1994/11/23 23:32:04 vertex Exp $
**
**  List management classes.
**
******************************************************************************/

typedef void *ListEntry;

class CListLink
{
	friend class CListLink;
	friend class CLLIterator;

	public:
		CListLink *next;
		ListEntry entry;

		CListLink(ListEntry e, CListLink *r);
		virtual ~CListLink(void);
};

class CList
{
	friend class CLLIterator;

	CListLink *last;

	public:
		CList(void);
		CList(ListEntry e);
		virtual ~CList(void);

		long Append(ListEntry e);
		long Count(void);
		ListEntry GetNth(long n);
		long Insert(ListEntry e);
		void Remove(long n);
};

class CLLIterator
{
	public:
		CLLIterator(CList *s);
		virtual ~CLLIterator(void);

		ListEntry Operator(void);
		void Reset(void);

	private:
		CListLink *fLink;
		CList *fList;
		CList *fSaveList;
};
#endif
