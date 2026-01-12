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
// Filename    : ccl/platform/shared/host/iplatformintegrationloader.h
// Description : Platform Integration Loader Interface
//
//************************************************************************************************

#ifndef _ccl_iplatformintegrationloader_h
#define _ccl_iplatformintegrationloader_h

#include "ccl/public/text/cclstring.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

#include "core/public/coreproperty.h"

namespace CCL {	
namespace PlatformIntegration {

//************************************************************************************************
// IPlatformImplementation
//************************************************************************************************

interface IPlatformImplementation: IUnknown
{
	virtual void* CCL_API getPlatformImplementation () = 0;
	
	DECLARE_IID (IPlatformImplementation)
};

//************************************************************************************************
// IPlatformIntegrationLoader
//************************************************************************************************

interface IPlatformIntegrationLoader: IUnknown
{
	virtual IPlatformImplementation* CCL_API createPlatformImplementation (StringRef packageName, Core::InterfaceID iid) = 0;
	
	virtual void CCL_API releasePlatformImplementation (IPlatformImplementation* implementation) = 0;
	
	DECLARE_IID (IPlatformIntegrationLoader)
};

//************************************************************************************************
// PlatformImplementationPtr
//************************************************************************************************

template<class T>
class PlatformImplementationPtr
{
public:
	PlatformImplementationPtr (StringRef name)
	: name (name)
	{}
	
	~PlatformImplementationPtr ()
	{
		reset ();
	}
	
	operator T* () const
	{
		return instance.isValid () ? static_cast<T*> (instance->getPlatformImplementation ()) : nullptr;
	}
	
	T* operator -> () const
	{
		return instance.isValid () ? static_cast<T*> (instance->getPlatformImplementation ()) : nullptr;
	}
	
	bool load ()
	{
		reset ();
		UnknownPtr<IPlatformIntegrationLoader> loader (&System::GetSystem ());
		if(loader)
			instance = loader->createPlatformImplementation (name, T::kIID);
		return instance.isValid ();
	}
	
	void reset ()
	{
		UnknownPtr<IPlatformIntegrationLoader> loader (&System::GetSystem ());
		if(loader && instance.isValid ())
			loader->releasePlatformImplementation (instance);
		instance.release ();
	}
	
protected:
	AutoPtr<IPlatformImplementation> instance;
	String name;
};

} // namespace PlatformIntegration
} // namespace CCL

#endif // _ccl_iplatformintegrationloader_h
