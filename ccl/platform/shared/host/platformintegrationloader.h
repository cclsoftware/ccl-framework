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
// Filename    : ccl/platform/shared/host/platformintegrationloader.h
// Description : Platform Integration Loader
//
//************************************************************************************************

#ifndef _ccl_platformintegrationloader_h
#define _ccl_platformintegrationloader_h

#include "ccl/platform/shared/host/iplatformintegrationloader.h"

#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/url.h"

namespace CCL {
interface ICoreClass;
interface IClassDescription;

namespace PlatformIntegration {
	
//************************************************************************************************
// PlatformImplementation
//************************************************************************************************

class PlatformImplementation: public Unknown,
							  public IPlatformImplementation
{
public:
	PlatformImplementation (ICoreClass* coreClass = nullptr, Core::IPropertyHandler* platformImplementation = nullptr);
	~PlatformImplementation ();
	
	PROPERTY_POINTER (ICoreClass, coreClass, CoreClass);
	PROPERTY_POINTER (Core::IPropertyHandler, platformImplementation, Instance);
	
	void cleanup ();
	
	// IPlatformImplementation
	void* CCL_API getPlatformImplementation () override;
	
	CLASS_INTERFACE (IPlatformImplementation, Unknown)
};

//************************************************************************************************
// PlatformIntegrationLoader
//************************************************************************************************

class PlatformIntegrationLoader: public Unknown,
								 public IPlatformIntegrationLoader
{
public:
	DECLARE_STRINGID_MEMBER (kSettingsName)
	
	PROPERTY_VARIABLE (Url, platformIntegrationFolder, PlatformIntegrationFolder)
	
	void terminate ();
	
	// IPlatformIntegrationLoader
	IPlatformImplementation* CCL_API createPlatformImplementation (StringRef packageName, Core::InterfaceID iid) override;
	void CCL_API releasePlatformImplementation (IPlatformImplementation* implementation) override;	
	
	CLASS_INTERFACE (IPlatformIntegrationLoader, Unknown)
	
protected:
	Vector<String> platformImplementationPackages;
	Vector<SharedPtr<PlatformImplementation>> platformImplementationInstances;
	AutoPtr<Settings> settings;
	
	IPlatformImplementation* createPlatformImplementation (StringRef packageName, Core::InterfaceID iid, StringRef implementationName);
	void getPlatformIntegrationFolder (Url& folder, StringRef packageName) const;
	
	Settings& getSettings ();
	bool restoreSettings (String& implementationName, StringRef packageName, Core::InterfaceID iid);
	void commitSettings (StringRef packageName, Core::InterfaceID iid, StringRef implementationName);
	virtual int getPriority (const IClassDescription& description) const;
};

} // namespace PlatformIntegration
} // namespace CCL
    
#endif // _ccl_platformintegrationloader_h
