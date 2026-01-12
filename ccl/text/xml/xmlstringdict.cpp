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
// Filename    : ccl/text/strings/xmlstringdict.cpp
// Description : XML String Dictionary
//
//************************************************************************************************

#include "ccl/text/xml/xmlstringdict.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Dictionary>
void copyDictionary (Dictionary& This, const Dictionary& dictionary)
{
	This.removeAll ();

	int count = dictionary.countEntries ();
	for(int i = 0; i < count; i++)
		This.appendEntry (dictionary.getKeyAt (i), dictionary.getValueAt (i));
}

//************************************************************************************************
// XmlStringDictionary
//************************************************************************************************

XmlStringDictionary::XmlStringDictionary (const XmlStringDictionary& d)
{
	copyFrom (d);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlStringDictionary::XmlStringDictionary (const IStringDictionary& d)
{
	copyFrom (d);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API XmlStringDictionary::copyFrom (const IStringDictionary& dictionary)
{
	copyDictionary<IStringDictionary> (*this, dictionary);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API XmlStringDictionary::convertTo (ICStringDictionary& dst, TextEncoding encoding) const
{
	dst.removeAll ();
	for(int i = 0, count = countEntries (); i < count; i++)
	{
		MutableCString key (getKeyAt (i), encoding);
		MutableCString value (getValueAt (i), encoding);
		dst.appendEntry (key, value);
	}
}

//************************************************************************************************
// XmlCStringDictionary
//************************************************************************************************

XmlCStringDictionary::XmlCStringDictionary (const XmlCStringDictionary& d)
{
	copyFrom (d);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlCStringDictionary::XmlCStringDictionary (const ICStringDictionary& d)
{
	copyFrom (d);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API XmlCStringDictionary::copyFrom (const ICStringDictionary& dictionary)
{
	copyDictionary<ICStringDictionary> (*this, dictionary);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API XmlCStringDictionary::convertTo (IStringDictionary& dst, TextEncoding encoding) const
{
	dst.removeAll ();
	for(int i = 0, count = countEntries (); i < count; i++)
	{
		String key, value;
		key.appendCString (encoding, getKeyAt (i));
		value.appendCString (encoding, getValueAt (i));
		dst.appendEntry (key, value);
	}
}
