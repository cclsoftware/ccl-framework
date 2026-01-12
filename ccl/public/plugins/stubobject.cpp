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
// Filename    : ccl/public/plugins/stubobject.cpp
// Description : Basic Stub Classes
//
//************************************************************************************************

#include "ccl/public/plugins/stubobject.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IInnerUnknown, 0x81c02aa7, 0xd5aa, 0x4d56, 0x90, 0x9, 0x33, 0x28, 0x13, 0x67, 0x7, 0x64)
DEFINE_IID_ (IOuterUnknown, 0xcc7be0b1, 0x9400, 0x4bf3, 0x99, 0x30, 0xd7, 0x55, 0x31, 0x5e, 0xf8, 0x77)

//************************************************************************************************
// StubObject
//************************************************************************************************

StubObject::StubObject (IObject* object, IUnknown* outerUnknown)
: object (object),
  outerUnknown (outerUnknown)
{
	ASSERT (object != nullptr && outerUnknown != nullptr)
	if(object)
		object->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StubObject::StubObject (const StubObject&)
: object (nullptr),
  outerUnknown (nullptr)
{
	CCL_DEBUGGER ("Stub copy constructor not allowed!")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StubObject::~StubObject ()
{
	if(object)
		object->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StubObject& StubObject::operator = (const StubObject&)
{
	CCL_DEBUGGER ("Stub instance assignement not allowed!")
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API StubObject::queryInterface (UIDRef iid, void** ptr)
{
	ASSERT (outerUnknown != nullptr)
	if(outerUnknown)
		return outerUnknown->queryInterface (iid, ptr);
	*ptr = nullptr;
	return kResultNoInterface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API StubObject::retain ()
{ 
	ASSERT (outerUnknown != nullptr)
	if(outerUnknown)
		return outerUnknown->retain ();
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API StubObject::release ()
{ 
	ASSERT (outerUnknown != nullptr)
	if(outerUnknown)
		return outerUnknown->release ();
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API StubObject::stubQueryInterface (UIDRef iid, void** ptr)
{
	return Unknown::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API StubObject::stubRetain ()
{
	return Unknown::retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API StubObject::stubRelease ()
{
	return Unknown::release ();
}
