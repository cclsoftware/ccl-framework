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
// Filename    : ccl/public/plugins/icoreplugin.h
// Description : Core Plug-in Wrapper Interfaces
//
//************************************************************************************************

#ifndef _ccl_icoreplugin_h
#define _ccl_icoreplugin_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/base/cclmacros.h"

#include "core/public/coreplugin.h"

namespace CCL {

struct ClassDesc;
interface IClassFactory;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Cast from Core's IPropertyHandler to CCL's IUnknown (does not change reference count).
//////////////////////////////////////////////////////////////////////////////////////////////////

static const Core::InterfaceID kCoreIUnknownIID = FOUR_CHAR_ID ('I', 'U', 'n', 'k');

inline IUnknown* GetIUnknownFromCoreInterface (Core::IPropertyHandler* handler)
{
	return Core::GetInterface<IUnknown> (handler, kCoreIUnknownIID);
}

template <class Class, class Interface>
bool ImplementCoreGetIUnknownInterface (Class* This, Core::Property& value)
{
	return Core::ImplementGetInterface<Class, Interface> (This, value, kCoreIUnknownIID);
}

//************************************************************************************************
// CorePropertyHandlerHelper
/** \ingroup base_plug */
//************************************************************************************************

template <class CoreInterface>
class CorePropertyHandlerHelper: public CoreInterface
{
public:
	void setProperty (const Core::Property& value) override
	{}

	void getProperty (Core::Property& value) override
	{}

private:
	void release () override
	{
		releaseInstance ();
	}

	virtual void releaseInstance () = 0;
};

//************************************************************************************************
// CorePropertyHandler
/**	Template class to implement mix of interface based on Core's IPropertyHandler with CCL class
	based on IUnknown.
	\ingroup base_plug */
//************************************************************************************************

template <class CoreInterface, class UnknownClass, class UnknownInterface>
class CorePropertyHandler: public UnknownClass,
						   public CorePropertyHandlerHelper<CoreInterface>
{
public:
	void getProperty (Core::Property& value) override
	{
		ImplementCoreGetIUnknownInterface<UnknownClass, UnknownInterface> (this, value);
	}

	// UnknownClass
	using UnknownClass::release;
	using UnknownClass::retain;

	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override
	{
		QUERY_INTERFACE (UnknownInterface)
		return UnknownClass::queryInterface (iid, ptr);
	}

private:
	// CorePropertyHandlerHelper
	void releaseInstance () override
	{
		UnknownClass::release ();
	}
};

//************************************************************************************************
// ICoreClass
/** \ingroup base_plug */
//************************************************************************************************

interface ICoreClass: IUnknown
{
	/** Get Core plug-in class information. */
	virtual const Core::Plugins::ClassInfo& CCL_API getClassInfo () const = 0;

	/** Get associated component class identifier. */
	virtual tbool CCL_API getComponentClassID (UIDBytes& cid) const = 0;

	DECLARE_IID (ICoreClass)
};

DEFINE_IID (ICoreClass, 0x43436371, 0x235b, 0x43b4, 0x89, 0x61, 0x6f, 0xbf, 0xeb, 0x3a, 0x24, 0xd4)

//************************************************************************************************
// ICoreClassHandler
/** \ingroup base_plug */
//************************************************************************************************

interface ICoreClassHandler: IUnknown
{
	/** Get CCL class description for given Core plug-in class. */
	virtual tbool CCL_API getDescription (ClassDesc& description, const Core::Plugins::ClassInfo& classInfo) = 0;

	/** Create IUnknown instance for given Core plug-in class. */
	virtual IUnknown* CCL_API createInstance (const Core::Plugins::ClassInfo& classInfo, UIDRef iid) = 0;

	DECLARE_IID (ICoreClassHandler)
};

DEFINE_IID (ICoreClassHandler, 0xd8e60c73, 0x32f7, 0x4f0c, 0x8e, 0xdf, 0x55, 0x53, 0x9e, 0xa0, 0x5, 0x2c)

//************************************************************************************************
// ICoreCodeLoader
/** \ingroup base_plug */
//************************************************************************************************

interface ICoreCodeLoader: ICoreClassHandler
{
	/** Register class handler. */
	virtual tresult CCL_API registerHandler (ICoreClassHandler* handler) = 0;

	/** Unregister class handler. */
	virtual tresult CCL_API unregisterHandler (ICoreClassHandler* handler) = 0;
	
	/** Create CCL class factory for Core class bundle. */
	virtual IClassFactory* CCL_API createClassFactory (const Core::Plugins::ClassInfoBundle& classBundle) = 0;

	DECLARE_IID (ICoreCodeLoader)
	DECLARE_STRINGID_MEMBER (kExtensionID);
};

DEFINE_IID (ICoreCodeLoader, 0xdb51ed8f, 0x66fb, 0x46f9, 0xb3, 0xee, 0x3e, 0xc2, 0xe1, 0xc3, 0xc0, 0x10)
DEFINE_STRINGID_MEMBER (ICoreCodeLoader, kExtensionID, "CoreCodeLoader");

} // namespace CCL

#endif // _ccl_icoreplugin_h
