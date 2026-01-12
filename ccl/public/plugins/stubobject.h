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
// Filename    : ccl/public/plugins/stubobject.h
// Description : Basic Stub Classes
//
//************************************************************************************************

#ifndef _ccl_stubobject_h
#define _ccl_stubobject_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/base/iobject.h"

namespace CCL {

//************************************************************************************************
// IStubObject
/**
	\ingroup base_plug */
//************************************************************************************************

interface IStubObject: IUnknown
{
	virtual tresult CCL_API stubQueryInterface (UIDRef iid, void** ptr) = 0;

	virtual unsigned int CCL_API stubRetain () = 0;

	virtual unsigned int CCL_API stubRelease () = 0;
};

//************************************************************************************************
// IInnerUnknown
/**
	\ingroup base_plug */
//************************************************************************************************

interface IInnerUnknown: IUnknown
{
	virtual void CCL_API setOuterUnknown (IUnknown* outerUnknown) = 0;

	DECLARE_IID (IInnerUnknown)
};

//************************************************************************************************
// IOuterUnknown
/**
	\ingroup base_plug */
//************************************************************************************************

interface IOuterUnknown: IUnknown
{
	virtual IUnknown* CCL_API getInnerUnknown () = 0;

	DECLARE_IID (IOuterUnknown)
};

//************************************************************************************************
// Stub macros
//************************************************************************************************

#define DECLARE_STUB_METHODS(Interface, Class) \
Class (CCL::IObject* object, CCL::IUnknown* outerUnknown) \
: StubObject (object, outerUnknown) {} \
static CCL::IStubObject* CCL_API createInstance (CCL::UIDRef iid, CCL::IObject* object, CCL::IUnknown* outerUnknown) \
{ return NEW Class (object, outerUnknown); } \
DELEGATE_UNKNOWN (StubObject) \
CCL::tresult CCL_API stubQueryInterface (CCL::UIDRef iid, void** ptr) override \
{ if(CCL::ccl_iid<Interface> ().equals (iid)) \
  { *ptr = (Interface*)this; stubRetain (); return CCL::kResultOk; } \
  return StubObject::stubQueryInterface (iid, ptr); }

#define REGISTER_STUB_CLASS(Interface, Stub) \
System::GetPlugInManager ().registerStubClass (CCL::ccl_iid<Interface> (), #Interface, Stub::createInstance);

//************************************************************************************************
// StubObject
//************************************************************************************************

class StubObject: public Unknown,
				  public IStubObject
{
public:
	StubObject (IObject* object, IUnknown* outerUnknown);
	~StubObject ();

	// IUnknown
	DECLARE_UNKNOWN

	// IStubObject
	tresult CCL_API stubQueryInterface (UIDRef iid, void** ptr) override;
	unsigned int CCL_API stubRetain () override;
	unsigned int CCL_API stubRelease () override;

protected:
	IObject* object;
	IUnknown* outerUnknown;

	StubObject (const StubObject&);
	StubObject& operator = (const StubObject&);

	bool invokeMethod (Variant& returnValue, MessageRef msg) const
	{
		ASSERT (object != nullptr)
		return object ? object->invokeMethod (returnValue, msg) != 0 : false;
	}

	bool getProperty (Variant& var, IObject::MemberID propertyId) const
	{
		ASSERT (object != nullptr)
		return object ? object->getProperty (var, propertyId) != 0 : false;
	}
};

} // namespace CCL

#endif // _ccl_stubobject_h
