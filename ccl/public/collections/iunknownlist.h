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
// Filename    : ccl/public/collections/iunknownlist.h
// Description : IUnknown List Interface
//
//************************************************************************************************

#ifndef _ccl_iunknownlist_h
#define _ccl_iunknownlist_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IUnknownIterator;

//************************************************************************************************
// IUnknownList macros
//************************************************************************************************

#define ForEachUnknown(cont, var) \
{ CCL::AutoPtr<CCL::IUnknownIterator> __iter = (cont).createIterator (); \
  if(__iter.isValid ()) while(!__iter->done ()) { \
  CCL::IUnknown* var = __iter->nextUnknown ();

#define IterForEachUnknown(createIter, var) \
{ CCL::AutoPtr<CCL::IUnknownIterator> __iter = createIter; \
  if(__iter.isValid ()) while(!__iter->done ()) { \
  CCL::IUnknown* var = __iter->nextUnknown ();

//************************************************************************************************
// IContainer
/** Basic container interface. \ingroup base_collect */
//************************************************************************************************

interface CCL_NOVTABLE IContainer: IUnknown
{
	/** Create iterator. */
	virtual IUnknownIterator* CCL_API createIterator () const = 0;

	DECLARE_IID (IContainer)
};

//************************************************************************************************
// IUnknownList
/** List of IUnknown objects. \ingroup base_collect  */
//************************************************************************************************

interface CCL_NOVTABLE IUnknownList: IContainer
{
	/** Check if list is empty. */
	virtual tbool CCL_API isEmpty () const = 0;
	
	/** Get first object in list. */
	virtual IUnknown* CCL_API getFirst () const = 0;

	/** Get last object in list. */
	virtual IUnknown* CCL_API getLast () const = 0;

	/** Check if object is in list. */
	virtual tbool CCL_API contains (IUnknown* object) const = 0;

	/** Add object. Ownership is transferred to container or shared. */
	virtual tbool CCL_API add (IUnknown* object, tbool share = false) = 0;

	/** Remove object. Ownership is transferred to caller. */
	virtual tbool CCL_API remove (IUnknown* object) = 0;

	/** Remove (and release) all objects. */
	virtual void CCL_API removeAll () = 0;

	DECLARE_IID (IUnknownList)
};

//************************************************************************************************
// IUnknownIterator
/** IUnknown list iterator. \ingroup base_collect */
//************************************************************************************************

interface CCL_NOVTABLE IUnknownIterator: IUnknown
{
	/** Check if iteration finished. */
	virtual tbool CCL_API done () const = 0;

	/** Get next object. */
	virtual IUnknown* CCL_API nextUnknown () = 0;

	DECLARE_IID (IUnknownIterator)
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// IterHasData
/** Return if iterator has at least 'count' elements. \ingroup base_collect  */
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool IterHasData (IUnknownIterator* newIterator, int count = 1) 
{
	int counter = 0;
	AutoPtr<IUnknownIterator> iter (newIterator);
	if(iter)
	{
		while(iter->done () == false)
		{
			if(++counter == count)
				return true;
			iter->nextUnknown ();
		}
	}
	return count == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IterCountData
/** Return the amount of iterator data. \ingroup base_collect  */
//////////////////////////////////////////////////////////////////////////////////////////////////

inline int IterCountData (IUnknownIterator* newIterator) 
{
	int counter = 0;
	AutoPtr<IUnknownIterator> iter (newIterator);
	if(iter)
	{
		while(iter->done () == false)
		{
			++counter;
			iter->nextUnknown ();
		}
	}
	return counter;
}

} // namespace CCL

#endif // _ccl_iunknownlist_h
