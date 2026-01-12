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
// Filename    : ccl/public/base/variant.cpp
// Description : Variant type
//
//************************************************************************************************

#include "ccl/public/base/variant.h"
#include "ccl/public/base/primitives.h"
#include "ccl/public/text/cstring.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IVariant, 0xbc561730, 0x96d, 0x4fb9, 0xb6, 0xc1, 0xf1, 0x52, 0xa9, 0x8e, 0xee, 0x7c)

//************************************************************************************************
// Variant
//************************************************************************************************

void Variant::clear ()
{
	if(isShared ())
	{
		if(getType () == kString && string)
			string->release ();
		else
		if(getType () == kObject && object)
			object->release ();
	}

	type = 0;
	lValue = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Variant::share ()
{
	if(!isShared ())
	{
		if(getType () == kString && string)
			string->retain ();
		else
		if(getType () == kObject && object)
			object->retain ();

		type |= kShared;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Variant::operator == (const Variant& v) const
{
	if(getUserFlags () != v.getUserFlags ())
		return false;

	if(getType () != v.getType ())
	{
		if((getType () == kFloat || getType () == kInt) && (v.getType () == kFloat || v.getType () == kInt))
			return asFloat () == v.asFloat ();

		return false;
	}

	switch(getType ())
	{
	default :
	case kInt :
		return lValue == v.lValue;

	case kFloat :
		return fValue == v.fValue;

	case kString :
		return String (string) == String (v.string);

	case kObject :
		// TODO: compare objects!? isEqualUnknown ??
		return object == v.object;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Variant::operator > (const Variant& v) const
{
	if(getType () == v.getType ())
	{
		switch(getType ())
		{
		default :
		case kInt :
			return lValue > v.lValue;

		case kFloat :
			return fValue > v.fValue;

		case kString :
			return String (string) > String (v.string);

		case kObject :
			// TODO: compare objects!?
			return object > v.object;
		}
	}

	if(getType () == kFloat || v.getType () == kFloat)
		return static_cast<double> (*this) > static_cast<double> (v);

	if(getType () == kString || v.getType () == kString)
		return String (string) > v;

	// TODO: compare objects!?
	return lValue > v.lValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Variant::operator < (const Variant& v) const
{
	if(getType () == v.getType ())
	{
		switch(getType ())
		{
		default :
		case kInt :
			return lValue < v.lValue;

		case kFloat :
			return fValue < v.fValue;

		case kString :
			return String (string) < String (v.string);

		case kObject :
			// TODO: compare objects!?
			return object < v.object;
		}
	}

	if(getType () == kFloat || v.getType () == kFloat)
		return static_cast<double> (*this) < static_cast<double> (v);

	if(getType () == kString || v.getType () == kString)
		return String (string) < v;

	// TODO: compare objects!?
	return lValue < v.lValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Variant::compare (const Variant& v) const
{
	if(getType () == v.getType ())
	{
		switch(getType ())
		{
		case kInt :
			return ccl_compare (lValue, v.lValue);
		case kFloat :
			return ccl_compare (fValue, v.fValue);
		case kString :
			return String (string).compare (String (v.string));
		}
	}
	
	if(getType () == kFloat || v.getType () == kFloat)
		return ccl_compare<double> (static_cast<double> (*this), static_cast<double> (v));

	if(getType () == kString || v.getType () == kString)
		return String (string).compare (v);

	// TODO: object comparison
	return ccl_compare (lValue, v.lValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Variant::toString (String& s) const
{
	static const String strObject = CCLSTR ("[OBJECT]");

	s.empty ();
	switch(getType ())
	{
	case kInt :
		s.appendIntValue (lValue);
		break;

	case kFloat :
		s.appendFloatValue (fValue);
		break;

	case kString :
		s = String (string);
		break;

	case kObject :
		s = strObject;
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant& Variant::fromString (StringRef s)
{
	clear ();
	System::ParseVariantString (*this, s);
	share ();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Variant::toCString (MutableCString& cs, TextEncoding encoding) const
{
	String s;
	toString (s);
	cs = MutableCString (s, encoding);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 Variant::parseLargeInt (int64 error) const
{
	if(isNumeric ())
		return asLargeInt ();
	else
	{
		int64 value = 0;
		if(VariantString (*this).getIntValue (value))
			return value;
		return error;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Variant::parseInt (int error) const
{
	return static_cast<int> (parseLargeInt (error));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double Variant::parseDouble (double error) const
{
	if(isNumeric ())
		return asDouble ();
	else
	{
		double value = 0;
		if(VariantString (*this).getFloatValue (value))
			return value;
		return error;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float Variant::parseFloat (float error) const
{
	return static_cast<float> (parseDouble (error));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Variant::parseBool () const
{
	if(isString ())
	{
		MutableCString string (asString ());
		return string == "1" || string.compare ("true", false) == 0;
	}
	else
		return asBool ();
}
