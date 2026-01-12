//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/public/collections/unknownlist.h
// Description : IUnknown List
//
//************************************************************************************************

#ifndef _ccl_unknownlist_h
#define _ccl_unknownlist_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/collections/iunknownlist.h"
#include "ccl/public/collections/linkedlist.h"

namespace CCL {

class UnknownIterator;

//************************************************************************************************
// UnknownList
/** \ingroup base_collect */
//************************************************************************************************

class UnknownList: public Unknown,
				   public IUnknownList
{
public:
	~UnknownList ();

	static IUnknownList* convert (IUnknown* unknown);
	bool isMultiple () const {return list.isMultiple ();}

	// IContainer
	IUnknownIterator* CCL_API createIterator () const override;

	// IUnknownList
	tbool CCL_API isEmpty () const override;
	IUnknown* CCL_API getFirst () const override;
	IUnknown* CCL_API getLast () const override;
	tbool CCL_API contains (IUnknown* object) const override;
	tbool CCL_API add (IUnknown* object, tbool share = false) override;
	tbool CCL_API remove (IUnknown* object) override;
	void CCL_API removeAll () override;

	template<class Element = Unknown> RangeIterator<UnknownList, UnknownIterator, Element*> begin () const;
	template<class Element = Unknown> RangeIterator<UnknownList, UnknownIterator, Element*> end () const;
	using IteratorClass = UnknownIterator;

	CLASS_INTERFACE2 (IUnknownList, IContainer, Unknown)

protected:
	friend class UnknownIterator;
	LinkedList<IUnknown*> list;
};

template<class Element>
inline RangeIterator<UnknownList, UnknownIterator, Element*> UnknownList::begin () const
{ return RangeIterator<UnknownList, UnknownIterator, Element*> (*this); }

template<class Element>
inline RangeIterator<UnknownList, UnknownIterator, Element*> UnknownList::end () const
{ static UnknownList dummy; return RangeIterator<UnknownList, UnknownIterator, Element*> (dummy); }

//************************************************************************************************
// UnknownIterator
/** \ingroup base_collect */
//************************************************************************************************

class UnknownIterator: public Unknown,
					   public IUnknownIterator,
					   public ListIterator<IUnknown*>
{
public:
	UnknownIterator (const UnknownList& list);

	// IUnknownIterator
	tbool CCL_API done () const override;
	IUnknown* CCL_API nextUnknown () override;

	CLASS_INTERFACE (IUnknownIterator, Unknown)
};

//************************************************************************************************
// InterfaceList
/** \ingroup base_collect */
//************************************************************************************************

template<class IFace>
class InterfaceList: public LinkedList<IFace*>
{
public:
	~InterfaceList ();

	void removeAll ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// InterfaceList implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class IFace>
InterfaceList<IFace>::~InterfaceList ()
{
	ListForEach (*this, IFace*, object)
		object->release ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class IFace>
void InterfaceList<IFace>::removeAll ()
{
	ListForEach (*this, IFace*, object)
		object->release ();
	EndFor
	LinkedList<IFace*>::removeAll ();
 }

} // namespace CCL

#endif // _ccl_unknownlist_h
