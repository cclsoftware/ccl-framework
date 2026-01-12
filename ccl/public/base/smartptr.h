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
// Filename    : ccl/public/base/smartptr.h
// Description : Smart Pointers
//
//************************************************************************************************

#ifndef _ccl_smartptr_h
#define _ccl_smartptr_h

namespace CCL {

template<class T> class AutoPtr;
template<class T> class SharedPtr;

//************************************************************************************************
// Smart Pointer Macros
//************************************************************************************************

/** Define smart pointer methods. */
#define DEFINE_SMARTPTR_METHODS(SmartPtr)											\
	bool isValid () const	{ return _ptr != nullptr; }									\
	SmartPtr& release ()	{ if(_ptr) _ptr->release (); _ptr = nullptr; return *this; }	\
	T* detach ()			{ T* temp = _ptr; _ptr = nullptr; return temp; }				\
	void** as_ppv ()		{ return (void**)&_ptr; } \
	T* as_plain () const	{ return _ptr; }

/** Define smart pointer operators. */
#define DEFINE_SMARTPTR_OPERATORS(SmartPtr)		\
	T* operator -> () const	{ return _ptr; }	\
	operator T* () const	{ return _ptr; }	\
	operator T*& ()			{ return _ptr; }	\

//************************************************************************************************
// UnknownPtr
/** Smart pointer class for interfaces, managing queryInteface() and release(). 
	\ingroup ccl_base */
//************************************************************************************************

template <class T>
class UnknownPtr
{
public:
	UnknownPtr (IUnknown* unk = nullptr);
	UnknownPtr (const UnknownPtr&);
	template<class P> UnknownPtr (const UnknownPtr<P>& other);
	template<class P> UnknownPtr (const AutoPtr<P>& other);
	template<class P> UnknownPtr (const SharedPtr<P>& other);
	~UnknownPtr ();

	DEFINE_SMARTPTR_METHODS (UnknownPtr)

	UnknownPtr& assign (IUnknown* unk);

	UnknownPtr& operator = (IUnknown* unk);
	UnknownPtr& operator = (const UnknownPtr& other);
	template<class P> UnknownPtr& operator = (const UnknownPtr<P>& other);
	template<class P> UnknownPtr& operator = (const AutoPtr<P>& other);
	template<class P> UnknownPtr& operator = (const SharedPtr<P>& other);

	DEFINE_SMARTPTR_OPERATORS (UnknownPtr)

protected:
	T* _ptr;
};

/** Compare canonical IUnknown. ATTENTION: Reference count is modified, must not be called in dtor! */
inline bool isEqualUnknown (IUnknown* u1, IUnknown* u2)
{
	return UnknownPtr<IUnknown> (u1) == UnknownPtr<IUnknown> (u2);
}

//************************************************************************************************
// AutoPtr
/** Smart pointer class taking ownership on assignement.
	\ingroup ccl_base */
//************************************************************************************************

template <class T> 
class AutoPtr
{
public:
	AutoPtr (T* ptr = nullptr);
	AutoPtr (const AutoPtr&);
	template<class P> AutoPtr (const AutoPtr<P>& other);
	template<class P> AutoPtr (const SharedPtr<P>& other);
	template<class P> AutoPtr (const UnknownPtr<P>& other);
	~AutoPtr ();

	DEFINE_SMARTPTR_METHODS (AutoPtr)

	AutoPtr& assign (T* ptr);
	AutoPtr& share (T* ptr);

	AutoPtr& operator = (T* ptr);
	AutoPtr& operator = (const AutoPtr&);
	template<class P> AutoPtr& operator = (const AutoPtr<P>& other);
	template<class P> AutoPtr& operator = (const SharedPtr<P>& other);
	template<class P> AutoPtr& operator = (const UnknownPtr<P>& other);

	DEFINE_SMARTPTR_OPERATORS (AutoPtr)

protected:
	T* _ptr;
};

//************************************************************************************************
// SharedPtr
/** Smart pointer class managing a reference count. 
	\ingroup ccl_base */
//************************************************************************************************

template <class T> 
class SharedPtr
{
public:
	SharedPtr (T* ptr = nullptr);
	SharedPtr (const SharedPtr&);
	template<class P> SharedPtr (const SharedPtr<P>& other);
	template<class P> SharedPtr (const AutoPtr<P>& other);
	template<class P> SharedPtr (const UnknownPtr<P>& other);
	~SharedPtr ();

	DEFINE_SMARTPTR_METHODS (SharedPtr)

	SharedPtr& assign (T* ptr);

	SharedPtr& operator = (T* ptr);
	SharedPtr& operator = (const SharedPtr& other);
	template<class P> SharedPtr& operator = (const SharedPtr<P>& other);
	template<class P> SharedPtr& operator = (const AutoPtr<P>& other);
	template<class P> SharedPtr& operator = (const UnknownPtr<P>& other);

	DEFINE_SMARTPTR_OPERATORS (SharedPtr)

protected:
	T* _ptr;
};

//************************************************************************************************
// ComparablePtr
/** Smart pointer class for comparison, T::compare() must be defined.
	\ingroup ccl_base */
//************************************************************************************************

template <class T> 
class ComparablePtr
{
public:
	ComparablePtr (T* ptr = nullptr);
	ComparablePtr (const ComparablePtr<T>&);
	~ComparablePtr ();

	ComparablePtr<T>& assign (T* ptr);
	ComparablePtr<T>& operator = (T* ptr);
	ComparablePtr<T>& operator = (const ComparablePtr<T>&);

	bool operator > (const ComparablePtr<T>& other) const;
	bool operator < (const ComparablePtr<T>& other) const;
	bool operator == (const ComparablePtr<T>& other) const;
		
	DEFINE_SMARTPTR_OPERATORS (ComparablePtr)

protected:
	T* _ptr;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// UnknownPtr inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
UnknownPtr<T>::UnknownPtr (IUnknown* unk)
: _ptr (nullptr)
{ assign (unk); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
UnknownPtr<T>::UnknownPtr (const UnknownPtr& other)
: _ptr (other.as_plain ())
{ if(_ptr) _ptr->retain (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
UnknownPtr<T>::UnknownPtr (const UnknownPtr<P>& other)
: _ptr (nullptr)
{ assign (other); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
UnknownPtr<T>::UnknownPtr (const AutoPtr<P>& other)
: _ptr (nullptr)
{ assign (other); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
template<class P>
UnknownPtr<T>::UnknownPtr (const SharedPtr<P>& other)
: _ptr (nullptr)
{ assign (other); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
UnknownPtr<T>::~UnknownPtr ()
{ release (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
UnknownPtr<T>& UnknownPtr<T>::assign (IUnknown* unk)
{
	release ();
	if(unk) unk->queryInterface (ccl_iid<T> (), (void**)&_ptr);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T> 
UnknownPtr<T>& UnknownPtr<T>::operator = (IUnknown* unk)	
{ return assign (unk); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T> 
UnknownPtr<T>& UnknownPtr<T>::operator = (const UnknownPtr<T>& other)
{
	if(other._ptr)
		other._ptr->retain ();
	if(_ptr)
		_ptr->release ();
	_ptr = other._ptr;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T> 
template <class P>
UnknownPtr<T>& UnknownPtr<T>::operator = (const UnknownPtr<P>& other)
{ return assign (other.as_plain ()); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
UnknownPtr<T>& UnknownPtr<T>::operator = (const AutoPtr<P>& other)
{ return assign (other); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
UnknownPtr<T>& UnknownPtr<T>::operator = (const SharedPtr<P>& other)
{ return assign (other); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// AutoPtr inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
AutoPtr<T>::AutoPtr (T* ptr)
: _ptr (ptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
AutoPtr<T>::AutoPtr (const AutoPtr& other)
: _ptr (other.as_plain ())
{ if(_ptr) _ptr->retain (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
AutoPtr<T>::AutoPtr (const AutoPtr<P>& other)
: _ptr (other.as_plain ())
{ if(_ptr) _ptr->retain (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
AutoPtr<T>::AutoPtr (const SharedPtr<P>& other)
: _ptr (other.as_plain ())
{ if(_ptr) _ptr->retain (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
AutoPtr<T>::AutoPtr (const UnknownPtr<P>& other)
: _ptr (other.as_plain ())
{ if(_ptr) _ptr->retain (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
AutoPtr<T>::~AutoPtr ()
{ release (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
INLINE AutoPtr<T>& AutoPtr<T>::assign (T* ptr)
{
	release ();
	_ptr = ptr;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
AutoPtr<T>& AutoPtr<T>::share (T* ptr)
{
	if(ptr)
		ptr->retain ();
	if(_ptr)
		_ptr->release ();
	_ptr = ptr;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
AutoPtr<T>& AutoPtr<T>::operator = (T* ptr)
{ return assign (ptr); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
AutoPtr<T>& AutoPtr<T>::operator = (const AutoPtr& other)
{ return share (other); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
AutoPtr<T>& AutoPtr<T>::operator = (const AutoPtr<P>& other)
{ return share (other); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
AutoPtr<T>& AutoPtr<T>::operator = (const SharedPtr<P>& other)
{ return share (other); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
AutoPtr<T>& AutoPtr<T>::operator = (const UnknownPtr<P>& other)
{ return share (other); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// ComparablePtr inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ComparablePtr<T>::ComparablePtr (T* ptr)
: _ptr (ptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ComparablePtr<T>::ComparablePtr (const ComparablePtr& other)
: _ptr (other._ptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ComparablePtr<T>::~ComparablePtr ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
INLINE ComparablePtr<T>& ComparablePtr<T>::assign (T* ptr)
{
	_ptr = ptr;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ComparablePtr<T>& ComparablePtr<T>::operator = (T* ptr)
{ return assign (ptr); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
ComparablePtr<T>& ComparablePtr<T>::operator = (const ComparablePtr& other)
{ 
	return assign (other._ptr); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool ComparablePtr<T>::operator > (const ComparablePtr<T>& other) const
{
	return _ptr->compare (other._ptr) > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool ComparablePtr<T>::operator < (const ComparablePtr<T>& other) const
{
	return _ptr->compare (other._ptr) < 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool ComparablePtr<T>::operator == (const ComparablePtr<T>& other) const
{
	return _ptr->compare (other._ptr) == 0;
}
		
//////////////////////////////////////////////////////////////////////////////////////////////////
// SharedPtr inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
SharedPtr<T>::SharedPtr (T* ptr)
: _ptr (ptr)
{ if(_ptr) _ptr->retain (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
SharedPtr<T>::SharedPtr (const SharedPtr& other)
: _ptr (other.as_plain ())
{ if(_ptr) _ptr->retain (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
SharedPtr<T>::SharedPtr (const SharedPtr<P>& other)
: _ptr (other.as_plain ())
{ if(_ptr) _ptr->retain (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
SharedPtr<T>::SharedPtr (const AutoPtr<P>& other)
: _ptr (other.as_plain ())
{ if(_ptr) _ptr->retain (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
SharedPtr<T>::SharedPtr (const UnknownPtr<P>& other)
: _ptr (other.as_plain ())
{ if(_ptr) _ptr->retain (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
SharedPtr<T>::~SharedPtr ()
{ release (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
INLINE SharedPtr<T>& SharedPtr<T>::assign (T* ptr)
{
	if(ptr)
		ptr->retain ();

	release ();
	_ptr = ptr;

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
SharedPtr<T>& SharedPtr<T>::operator = (T* ptr)
{ return assign (ptr); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
SharedPtr<T>& SharedPtr<T>::operator = (const SharedPtr& other)
{ return assign (other); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
SharedPtr<T>& SharedPtr<T>::operator = (const SharedPtr<P>& other)
{ return assign (other); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
SharedPtr<T>& SharedPtr<T>::operator = (const AutoPtr<P>& other)
{ return assign (other); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
template <class P>
SharedPtr<T>& SharedPtr<T>::operator = (const UnknownPtr<P>& other)
{ return assign (other); }


} // namespace CCL

#endif // _ccl_smartptr_h
