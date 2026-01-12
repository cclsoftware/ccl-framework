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
// Filename    : ccl/platform/win/system/cclcom.impl.h
// Description : COM/WinRT shared
//
//************************************************************************************************

//************************************************************************************************
// COM macros
//************************************************************************************************

/** Cast GUID to UIDBytes - can't be a function because it is used in different namespaces. */
#define com_uid_cast(guid) \
	reinterpret_cast<CCL::UIDRef> (guid)

/** Cast UIDBytes to GUID. */
#define com_uid_invcast(uid) \
	reinterpret_cast<const ::GUID&> (uid)

/** Delegate COM's IUnknown to CCL's IUnknown. */
#define DELEGATE_COM_IUNKNOWN \
STDMETHODIMP QueryInterface (REFIID riid, void** ppvObject) override \
{ return (HRESULT)queryInterface (com_uid_cast (riid), ppvObject); } \
STDMETHODIMP_ (ULONG) AddRef () override \
{ return retain (); } \
STDMETHODIMP_ (ULONG) Release () override \
{ return release (); }

/** Query for COM interface inside CCL's queryInterface. */
#define QUERY_COM_INTERFACE(Interface) \
if(CCL::UID (com_uid_cast (__uuidof (Interface))).equals (iid)) \
{ *ptr = (Interface*)this; retain (); return CCL::kResultOk; }

//************************************************************************************************
// ComUnknownPtr
/** Query COM IUnknown for CCL interface. */
//************************************************************************************************

template<class T> class ComUnknownPtr: public UnknownPtr<T>
{
public:
	ComUnknownPtr<T> (::IUnknown* unk)
	: UnknownPtr<T> (reinterpret_cast<CCL::IUnknown*> (unk))
	{}
};

//************************************************************************************************
// ComPtr
//************************************************************************************************

template <class T>
class ComPtr
{
public:
	ComPtr (T* ptr = nullptr);
	ComPtr (const ComPtr& other);
	~ComPtr ();

	bool isValid () const;
	ComPtr& assign (T* ptr);
	ComPtr& fromUnknown (::IUnknown* unk);
	ComPtr& fromUnknown (CCL::IUnknown* unk);
	ComPtr& share (T* ptr);
	ComPtr& release ();
	T* detach ();

	template <class T2> HRESULT as (ComPtr<T2>& other)
	{
		other.release ();
		return _ptr ? _ptr->QueryInterface (__uuidof (T2), other) : E_NOINTERFACE;
	}

	ComPtr& operator = (T* ptr);
	ComPtr& operator = (const ComPtr&);

	T* operator -> () const { return _ptr; }
	T** operator & ()		{ return &_ptr; }

	operator T* () const	{ return _ptr; }
	operator T*& ()			{ return _ptr; }
	operator T** ()			{ return &_ptr; }

	operator void** ()		{ return (void**)&_ptr; }	///< usage: QueryInterface ( ... comPtr)
	operator bool ()		{ return _ptr != nullptr; }		///< usage: if(comPtr)...

protected:
	T* _ptr;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ComPtr inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ComPtr<T>::ComPtr (T* ptr)
: _ptr (ptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ComPtr<T>::ComPtr (const ComPtr& other)
: _ptr (other._ptr)
{ if(_ptr) _ptr->AddRef (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ComPtr<T>::~ComPtr ()
{ release (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
INLINE bool ComPtr<T>::isValid () const
{ return _ptr != nullptr; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
INLINE ComPtr<T>& ComPtr<T>::assign (T* ptr)
{
	release ();
	_ptr = ptr;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
INLINE ComPtr<T>& ComPtr<T>::fromUnknown (::IUnknown* unk)
{
	release ();
	if(unk)
		unk->QueryInterface (__uuidof(T), *this);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
INLINE ComPtr<T>& ComPtr<T>::fromUnknown (CCL::IUnknown* unk)
{
	return fromUnknown (reinterpret_cast<::IUnknown*> (unk));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ComPtr<T>& ComPtr<T>::share (T* ptr)
{
	if(ptr)
		ptr->AddRef ();
	if(_ptr)
		_ptr->Release ();
	_ptr = ptr;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ComPtr<T>& ComPtr<T>::release ()
{
	if(_ptr)
		_ptr->Release ();
	_ptr = nullptr;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T* ComPtr<T>::detach ()
{
	T* temp = _ptr;
	_ptr = nullptr;
	return temp;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ComPtr<T>& ComPtr<T>::operator = (T* ptr)
{ return assign (ptr); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ComPtr<T>& ComPtr<T>::operator = (const ComPtr& other)
{ return share (other._ptr); }

//////////////////////////////////////////////////////////////////////////////////////////////////
