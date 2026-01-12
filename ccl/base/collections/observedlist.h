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
// Filename    : ccl/base/collections/observedlist.h
// Description : Observed List
//
//************************************************************************************************

#ifndef _observedlist_h
#define _observedlist_h

#include "ccl/base/message.h"
#include "ccl/public/collections/linkedlist.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// ObservedListForEach : iterate trough objects in ObservedList
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ObservedListForEach(list, T, var) \
{ CCL::ObservedListIterator<T> __iter (list); \
  while(!__iter.done ()) { \
   T* var = __iter.next ().ptr;

template <class T> class ObservedListIterator;

//************************************************************************************************
// ObservedList
/** List of pointers that are removed automatically when their objects are destroyed. */
//************************************************************************************************

template <class T>
class ObservedList: public Object
{
public:
	ObservedList ();

	bool isEmpty () const;
	int count () const;
	T* at (int idx) const;	
	T* getFirst () const;

	bool add (T* subject);

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	
	struct SubjectEntry
	{
		T* ptr;
		ISubject* subject;

		SubjectEntry ()
		: ptr (nullptr),
		  subject (nullptr)
		{}
	};

	LinkedList<SubjectEntry> list;

	friend class ObservedListIterator<T>;
};

//************************************************************************************************
// ObservedListIterator
//************************************************************************************************

template <class T>
class ObservedListIterator : public ListIterator<typename ObservedList<T>::SubjectEntry>
{
public:
	ObservedListIterator (const ObservedList<T>& list)
	: ListIterator<typename ObservedList<T>::SubjectEntry> (list.list)
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ObservedList inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ObservedList<T>::ObservedList ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool ObservedList<T>::isEmpty () const 
{ 
	return list.isEmpty (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
int ObservedList<T>::count () const
{
	return list.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T* ObservedList<T>::at (int idx) const
{
	return list.at (idx).ptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T* ObservedList<T>::getFirst () const 
{ 
	if(list.isEmpty ())
		return 0;
	else
		return list.getFirst ().ptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool ObservedList<T>::add (T* subject)
{
	if(subject)
	{
		SubjectEntry entry;
		if(subject->queryInterface (ccl_iid<ISubject> (), (void**)&entry.subject) == kResultOk)
		{
			entry.subject->release ();
			entry.subject->addObserver (this);
			entry.ptr = subject;

			list.append (entry);
			signal (Message (kChanged));
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void CCL_API ObservedList<T>::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kDestroyed)
	{
		bool found = false;

		ListForEach (list, SubjectEntry, currentEntry)
			if(currentEntry.subject && currentEntry.subject == subject)
			{
				currentEntry.subject->removeObserver (this);
				list.remove (__iter);
				found = true;
				//break; //TODO? care about double entries?
			}
		EndFor

		if(found)
			signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _observedlist_h
