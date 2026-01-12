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
// Filename    : ccl/public/base/iobserver.h
// Description : Observer interface
//
//************************************************************************************************

#ifndef _ccl_iobserver_h
#define _ccl_iobserver_h

#include "ccl/public/base/imessage.h"

namespace CCL {

interface IObserver;

//************************************************************************************************
// ISubject
/** A subject notifies multiple observer objects on state changes. 
	\ingroup ccl_base */
//************************************************************************************************

interface CCL_NOVTABLE ISubject: IUnknown
{
	/** Connect observer to subject. */
	virtual void CCL_API addObserver (IObserver* observer) = 0;

	/** Disconnect observer from subject. */
	virtual void CCL_API removeObserver (IObserver* observer) = 0;
	
	/** Send message to connected observers. */
	virtual void CCL_API signal (MessageRef msg) = 0;
	
	/** Send message to connected observers asynchronously. */
	virtual void CCL_API deferSignal (IMessage* msg) = 0;

	DECLARE_IID (ISubject)

//////////////////////////////////////////////////////////////////////////////////////////////////

	/** Helper to connect observer to given IUnknown. */
	static void addObserver (IUnknown* unknown, IObserver* observer);

	/** Helper to disconnect observer from given IUnknown. */
	static void removeObserver (IUnknown* unknown, IObserver* observer);
};

DEFINE_IID (ISubject, 0xdefb56d5, 0x495e, 0x4f17, 0x9d, 0x52, 0xfa, 0xd4, 0x8b, 0x2f, 0xc8, 0x71)

//************************************************************************************************
// IObserver
/** An observer will be notified if one of its subjects sends a message.
	\ingroup ccl_base */
//************************************************************************************************

interface CCL_NOVTABLE IObserver: IUnknown
{
	/** Receive notification from subject. */
	virtual void CCL_API notify (ISubject* subject, MessageRef msg) = 0;

	DECLARE_IID (IObserver)
	
//////////////////////////////////////////////////////////////////////////////////////////////////

	/** Helper to send a message to given IUnknown. */
	static void notify (IUnknown* unknown, ISubject* subject, MessageRef msg);
};

DEFINE_IID (IObserver, 0xfbb66ab6, 0xcdf0, 0x4e39, 0x8e, 0x8e, 0x0c, 0xb9, 0x9f, 0x06, 0x2c, 0x46)

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Assign IUnknown variable with reference counting and observer registration. */
template <typename T>
inline void share_and_observe_unknown (IObserver* This, T*& member, T* value)			
{
	if(member)
	{
		ISubject::removeObserver (member, This);
		member->release ();
	}
	member = value;
	if(member)
	{
		member->retain ();
		ISubject::addObserver (member, This);
	}
}

/** Assign IUnknown variable with observer registration but without reference counting. */
template <typename T>
inline bool assign_and_observe_unknown (IObserver* This, T*& member, T* value)
{
	if(member != value)
	{
		if(member)
		{
			ISubject::removeObserver (member, This);
		}
		member = value;
		if(member)
		{
			ISubject::addObserver (member, This);
		}
		return true;
	}
	return false;
}

//************************************************************************************************
// ObservedPtr
/** Observed pointer is nulled automatically when its subject is destroyed.
	\ingroup ccl_base */
//************************************************************************************************

template <class T>
class ObservedPtr: public IObserver
{
public:
	ObservedPtr (T* subject = nullptr);
	ObservedPtr (const ObservedPtr&);
	~ObservedPtr ();

	bool isValid () const;
	ObservedPtr& assign (T* subject);

   	ObservedPtr& operator = (T* subject);
	ObservedPtr& operator = (const ObservedPtr& other);
	operator T* () const;
	T* operator -> () const;

protected:
	T* _ptr;
	ISubject* _subject;

	// IObserver
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;
	unsigned int CCL_API retain () override;
	unsigned int CCL_API release () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ObservedPtr inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ObservedPtr<T>::ObservedPtr (T* subject)
: _ptr (nullptr),
  _subject (nullptr)
{ assign (subject); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ObservedPtr<T>::ObservedPtr (const ObservedPtr& other)
: _ptr (nullptr),
  _subject (nullptr)
{ assign (other._ptr); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ObservedPtr<T>::~ObservedPtr () 
{ assign (nullptr); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool ObservedPtr<T>::isValid () const 
{ return _ptr != nullptr; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ObservedPtr<T>& ObservedPtr<T>::assign (T* subject)
{
	if(_subject)
	{
		_subject->removeObserver (this);
		_ptr = nullptr;
	}
	
	if(subject)
	{
		if(subject->queryInterface (ccl_iid<ISubject> (), (void**)&_subject) == kResultOk)
			_subject->release ();
	}
	else
		_subject = nullptr;

	if(_subject)
	{
		_subject->addObserver (this);
		_ptr = subject;
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ObservedPtr<T>& ObservedPtr<T>::operator = (T* subject)
{ return assign (subject); }

//////////////////////////////////////////////////////////////////////////////////////////////////
    
template <class T>
ObservedPtr<T>& ObservedPtr<T>::operator = (const ObservedPtr& other)
{ return assign (other._ptr); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ObservedPtr<T>::operator T* () const
{ return _ptr; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T* ObservedPtr<T>::operator -> () const
{ return _ptr; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void CCL_API ObservedPtr<T>::notify (ISubject* subject, MessageRef msg)
{
	if(_subject && _subject == subject && msg == kDestroyed)
		assign (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
tresult CCL_API ObservedPtr<T>::queryInterface (UIDRef iid, void** ptr)
{ *ptr = nullptr; return kResultNoInterface; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
unsigned int CCL_API ObservedPtr<T>::retain () 
{ return 1; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
unsigned int CCL_API ObservedPtr<T>::release () 
{ return 1; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_iobserver_h
