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
// Filename    : ccl/platform/win/system/cclcom.cpp
// Description : COM Compatibility Layer
//
//************************************************************************************************

#include "ccl/platform/win/system/cclcom.h"

#include "ccl/public/base/debug.h"
#include "ccl/public/base/uid.h"

#include "ccl/public/text/cstring.h"

#include <propvarutil.h>

#pragma comment (lib, "shlwapi.lib") // needed for variant helper functions
#pragma comment(lib, "Propsys.lib") // needed for variant array helper functions

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// ComString
//************************************************************************************************

ComString::ComString ()
: data (nullptr)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ComString::ComString (const ComString& other)
: data (::SysAllocString (other.data))
{
	ASSERT(data != nullptr)
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ComString::ComString (ComString&& other)
: data (other.data)
{
	other.data = nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ComString::ComString (BSTR bstr)
: data (::SysAllocString (bstr))
{
	ASSERT(data != nullptr)
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ComString::~ComString ()
{
	::SysFreeString (data);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ComString::ComString (CCL::StringRef str)
: data (nullptr)
{
	int length = str.length ();
	data = ::SysAllocStringByteLen (nullptr, length * sizeof(OLECHAR));

	if(data != nullptr)
		str.copyTo (reinterpret_cast<uchar*>(data), length + 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ComString::ComString (CCL::CStringRef str)
: data (nullptr)
{
	int length = str.length ();
	data = ::SysAllocStringByteLen (nullptr, length + sizeof(OLECHAR));
	for(int i = 0; i < length; i++)
		data[i] = str[i];
	data[length] = L'\0';
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void ComString::assign (BSTR bstr)
{
	::SysFreeString (data);
	data = bstr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

BSTR ComString::detach ()
{
	BSTR temp = data;
	data = nullptr;
	return temp;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ComString& ComString::operator= (const OLECHAR* rhs)
{
	::SysFreeString (data);
	data = ::SysAllocString (rhs);
	ASSERT (data != nullptr)
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ComString& ComString::operator= (const ComString& rhs)
{
	if(this != &rhs)
	{
		::SysFreeString (data);
		data = ::SysAllocString (rhs.data);
		ASSERT (data != nullptr)
	}

	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ComString& ComString::operator= (ComString&& rhs)
{
	if(this == &rhs)
	{
		::SysFreeString (data);
		data = rhs.data;
		rhs.data = nullptr;
	}

	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ComString& ComString::operator= (CCL::StringRef rhs)
{
	::SysFreeString (data);

	int length = rhs.length ();
	data = ::SysAllocStringByteLen (nullptr, length * sizeof(OLECHAR));

	if(data != nullptr)
		rhs.copyTo (reinterpret_cast<uchar*>(data), length + 1);

	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ComString::operator CCL::String () const
{
	CCL::String result;
	result.appendNativeString (data);
	return result;
}

//************************************************************************************************
// ComVariant
//************************************************************************************************

bool ComVariant::convert (CCL::Variant& cclVariant, const VARIANT& comVariant)
{
	cclVariant.clear ();
	bool result = true;

	VARTYPE vt = (comVariant.vt & VT_TYPEMASK);
	bool byRef = (comVariant.vt & VT_BYREF) != 0;

	switch(vt)
	{
	case VT_NULL : // hmm???
		break;

	case VT_I4 :
	case VT_BOOL :
		if(byRef)
			cclVariant = comVariant.pintVal ? *comVariant.pintVal : 0;
		else
			cclVariant = comVariant.intVal;
		cclVariant.setBoolFormat (vt == VT_BOOL);
		break;

	case VT_BSTR :
		{
			String string;
			if(byRef)
				string.appendNativeString (comVariant.pbstrVal ? *comVariant.pbstrVal : nullptr);
			else
				string.appendNativeString (comVariant.bstrVal);
			cclVariant = string;
			cclVariant.share ();
		}
		break;

	case VT_DISPATCH :
	case VT_UNKNOWN :
		if(byRef)
			cclVariant = reinterpret_cast<CCL::IUnknown*> (comVariant.ppunkVal ? *comVariant.ppunkVal : nullptr);
		else
			cclVariant = reinterpret_cast<CCL::IUnknown*> (comVariant.punkVal); // same as pdispVal
		break;

	case VT_VARIANT :
		ASSERT (byRef == true)
		result = comVariant.pvarVal ? convert (cclVariant, *comVariant.pvarVal) : false;
		break;

	default :
		//CCL_NOT_IMPL ("VARIANT not converted!")
		result = false;
		break;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComVariant::convert (VARIANT& comVariant, const CCL::Variant& cclVariant)
{
	::VariantClear (&comVariant);

	switch(cclVariant.getType ())
	{
	case CCL::Variant::kInt :
		if(cclVariant.isBoolFormat ())
		{
			comVariant.vt = VT_BOOL;
			comVariant.intVal = cclVariant.asInt ();
		}
		else
		{
			comVariant.vt = VT_INT;
			comVariant.intVal = cclVariant.asInt ();
		}
		return true;

	case CCL::Variant::kFloat :
		comVariant.vt = VT_R4;
		comVariant.fltVal = cclVariant.asFloat ();
		return true;

	case CCL::Variant::kString :
		comVariant.vt = VT_BSTR;
		comVariant.bstrVal = ComString (cclVariant.asString ()).detach ();
		return comVariant.bstrVal != nullptr;

	default:
		ASSERT (false)
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComVariant::fromInt32Vector (VARIANT& comVariant, const CCL::Vector<LONG>& values)
{
	HRESULT hr = ::VariantClear (&comVariant);
	ASSERT (SUCCEEDED (hr))
	if(SUCCEEDED (hr))
	{
		hr = ::InitVariantFromInt32Array (values, values.count (), &comVariant);
		ASSERT (SUCCEEDED (hr))
	}
	return SUCCEEDED (hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComVariant::fromDoubleVector (VARIANT& comVariant, const CCL::Vector<double>& values)
{
	HRESULT hr = ::VariantClear (&comVariant);
	ASSERT (SUCCEEDED (hr))
	if(SUCCEEDED (hr))
	{
		hr = ::InitVariantFromDoubleArray (values, values.count (), &comVariant);
		ASSERT (SUCCEEDED (hr))
	}
	return SUCCEEDED (hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComVariant::takeSafeArray (VARIANT& comVariant, SAFEARRAY* safeArray, VARTYPE type)
{
	comVariant.vt = VT_ARRAY | type;
	comVariant.parray = safeArray;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComVariant::toVariant (Variant& value) const
{
	return convert (value, *this);
}

//************************************************************************************************
// PropVariant
//************************************************************************************************

bool PropVariant::fromString (StringRef string)
{
	HRESULT hr = ::InitPropVariantFromString (StringChars (string), this);
	ASSERT (SUCCEEDED (hr))
	return SUCCEEDED (hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PropVariant::toVariant (Variant& value) const
{
	value.clear ();
	bool result = true;
	String string;

	switch(vt)
	{
	case VT_EMPTY :
		break;

	case VT_LPWSTR :
		string.append (pwszVal);
		value = string;
		value.share ();
		break;

	case VT_UI4 :
		value = lVal;
		break;

	case VT_UI8 :
		value =	hVal.QuadPart;
		break;

	case VT_R8 :
		value =	dblVal;
		break;

	case VT_CLSID :
	{	CCL::UID uid = com_uid_cast (*puuid);
		uid.toString (string);
		value = string;
		value.share ();
	}	break;
	
	default :
		CCL_NOT_IMPL ("PROPVARIANT not converted!")
		result = false;
		break;
	}

	return result;
}
