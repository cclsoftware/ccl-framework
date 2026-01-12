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
// Filename    : ccl/platform/win/system/cclcom.h
// Description : COM Compatibility Layer
//
//************************************************************************************************

#ifndef _ccl_cclcom_h
#define _ccl_cclcom_h

#include "ccl/public/base/variant.h"
#include "ccl/public/collections/vector.h"

#include "ccl/platform/win/cclwindows.h"

namespace CCL {
namespace Win32 {

// include shared implementation
#include "ccl/platform/win/system/cclcom.impl.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Create COM object using the same syntax as ccl_new<>().  */
template <class T> inline T* com_new (REFCLSID rclsid)
{
	T* obj = nullptr;
	::CoCreateInstance (rclsid, nullptr, CLSCTX_INPROC_SERVER, __uuidof(T), (void**)&obj);
	return obj;
}

//************************************************************************************************
// ComString
//************************************************************************************************

class ComString
{
public:
	ComString ();
	ComString (const ComString& other);
	ComString (ComString&& other);
	ComString (BSTR bstr);
	ComString (CCL::StringRef str);
	ComString (CCL::CStringRef str);
	~ComString ();

	void assign (BSTR bstr);
	BSTR detach ();
	uint32 length () const { return ::SysStringLen (data); }
	ComString& operator= (const OLECHAR* rhs);
	ComString& operator= (const ComString& rhs);
	ComString& operator= (ComString&& rhs);
	ComString& operator= (CCL::StringRef rhs);
	OLECHAR& operator[] (uint32 index) { return data[index]; }
	const OLECHAR& operator[] (uint32 index) const { return data[index]; }
	operator BSTR () { return data; }
	operator CCL::String () const;

private:
	BSTR data;
};

//************************************************************************************************
// ComVariant
//************************************************************************************************

struct ComVariant: VARIANT
{
	ComVariant ()
	{ ::VariantInit (this); }

	~ComVariant ()
	{ ::VariantClear (this); }

	static bool convert (CCL::Variant& cclVariant, const VARIANT& comVariant);
	static bool convert (VARIANT& comVariant, const CCL::Variant& cclVariant);
	static bool fromInt32Vector (VARIANT& comVariant, const CCL::Vector<LONG>& values);
	static bool fromDoubleVector (VARIANT& comVariant, const CCL::Vector<double>& values);
	static bool takeSafeArray (VARIANT& comVariant, SAFEARRAY* safeArray, VARTYPE type);

	bool toVariant (CCL::Variant& v) const;

	CCL::Variant asVariant () const { Variant v; toVariant (v); return v; }
	CCL::String asString () const	{ return asVariant ().asString (); }
};

//************************************************************************************************
// ConvertedVariant
//************************************************************************************************

struct ConvertedVariant: Variant
{
	ConvertedVariant (const VARIANT& comVariant)
	{
		ComVariant::convert (*this, comVariant);
	}
};

//************************************************************************************************
// PropVariant
//************************************************************************************************

struct PropVariant: PROPVARIANT
{
	PropVariant ()
	{ ::PropVariantInit (this); }

	~PropVariant ()
	{ clear (); }

	void clear () { ::PropVariantClear (this); }

	bool fromString (StringRef string);

	bool toVariant (CCL::Variant& v) const;

	CCL::Variant asVariant () const	{ Variant v; toVariant (v); return v; }
	CCL::String asString () const	{ return asVariant ().asString (); }
};

//************************************************************************************************
// SafeArray
//************************************************************************************************

template<VARTYPE type, typename T>
class SafeArray
{
public:
	SafeArray (T* values, LONG count)
	{
		data = ::SafeArrayCreateVector (type, 0, count);
		ASSERT (data)
		if(!data)
			return;
		for(LONG i = 0; i < count; i++)
			putValue (i, &(values[i]));
	}

	~SafeArray ()
	{
		if(data)
			::SafeArrayDestroy (data);
	}

	SAFEARRAY* detach ()
	{
		SAFEARRAY* result = data;
		data = nullptr;
		return result;
	}

private:
	SAFEARRAY* data;

	void putValue (LONG index, T* value);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<VARTYPE type, typename T>
inline void SafeArray<type, T>::putValue (LONG index, T* value)
{
	ASSERT (type != VT_DISPATCH && type != VT_UNKNOWN && type != VT_BSTR)
	HRESULT hr = ::SafeArrayPutElement (data, &index, reinterpret_cast<void*> (value));
	ASSERT (SUCCEEDED (hr))
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
inline void SafeArray<VT_DISPATCH, ::IDispatch*>::putValue (LONG index, ::IDispatch** value)
{
	HRESULT hr = ::SafeArrayPutElement (data, &index, reinterpret_cast<void*> (*value));
	ASSERT (SUCCEEDED (hr))
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
inline void SafeArray<VT_UNKNOWN, ::IUnknown*>::putValue (LONG index, ::IUnknown** value)
{
	HRESULT hr = ::SafeArrayPutElement (data, &index, reinterpret_cast<void*> (*value));
	ASSERT (SUCCEEDED (hr))
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
inline void SafeArray<VT_BSTR, BSTR>::putValue (LONG index, BSTR* value)
{
	HRESULT hr = ::SafeArrayPutElement (data, &index, reinterpret_cast<void*> (*value));
	ASSERT (SUCCEEDED (hr))
}

//************************************************************************************************
// ComDeleter
//************************************************************************************************

template <class T>
class ComDeleter
{
public:
	ComDeleter (T* ptr = nullptr)
	: _ptr (ptr)
	{}

	~ComDeleter ()
	{ if(_ptr) ::CoTaskMemFree (_ptr); }

	operator T* () const	{ return _ptr; }
	operator T*& ()			{ return _ptr; }
	operator T** ()			{ return &_ptr; }
	operator bool ()		{ return _ptr != 0; }

protected:
	T* _ptr;
};

} // namespace Win32
} // namespace COM

#endif // _ccl_cclcom_h
