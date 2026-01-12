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
// Filename    : ccl/platform/shared/host/platformintegrationloader.cpp
// Description : Platform Integration Loader
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/shared/host/platformintegrationloader.h"
#include "ccl/platform/shared/interfaces/platformintegration.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/plugins/icoderesource.h"
#include "ccl/public/plugins/icoreplugin.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/text/stringbuilder.h"

using namespace CCL;
using namespace PlatformIntegration;

//************************************************************************************************
// PlatformImplementation
//************************************************************************************************

PlatformImplementation::PlatformImplementation (ICoreClass* coreClass, Core::IPropertyHandler* platformImplementation)
: coreClass (coreClass),
  platformImplementation (platformImplementation)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlatformImplementation::~PlatformImplementation ()
{
	cleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformImplementation::cleanup ()
{
	if(platformImplementation)
		platformImplementation->release ();
	platformImplementation = nullptr;
	if(coreClass)
		ccl_release (coreClass);
	coreClass = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API PlatformImplementation::getPlatformImplementation ()
{ 
	return platformImplementation;
}

//************************************************************************************************
// PlatformIntegrationLoader
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (PlatformIntegrationLoader, kSettingsName, "PlatformIntegration")

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlatformIntegrationLoader::terminate ()
{
	for(const SharedPtr<PlatformImplementation>& instance : platformImplementationInstances)
		instance->cleanup ();
	platformImplementationInstances.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformIntegrationLoader::getPlatformIntegrationFolder (Url& folder, StringRef packageName) const
{
	folder = getPlatformIntegrationFolder ();
	folder.descend (packageName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPlatformImplementation* CCL_API PlatformIntegrationLoader::createPlatformImplementation (StringRef packageName, Core::InterfaceID iid)
{
	IPlatformImplementation* result = nullptr;
	
	if(!platformImplementationPackages.contains (packageName))
	{
		Url platformIntegrationFolder;
		getPlatformIntegrationFolder (platformIntegrationFolder, packageName);
		System::GetPlugInManager ().scanFolder (platformIntegrationFolder, CodeResourceType::kCore);
		platformImplementationPackages.add (packageName);
	}
	
	String implementationName;
	restoreSettings (implementationName, packageName ,iid);
	result = createPlatformImplementation (packageName, iid, implementationName);
	
	if(result == nullptr && !implementationName.isEmpty ())
		result = createPlatformImplementation (packageName, iid, String::kEmpty);
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPlatformImplementation* PlatformIntegrationLoader::createPlatformImplementation (StringRef packageName, Core::InterfaceID iid, StringRef implementationName)
{
	struct Candidate
	{
		ICoreClass* coreClass;
		int priority;
		
		bool operator > (const Candidate& other) const
		{
			return priority > other.priority;
		}
	};
	Vector<Candidate> candidates;
	
	ForEachPlugInClass (CLASS_TYPE_PLATFORMINTEGRATION, description)
		if(implementationName.isEmpty () || description.getModuleVersion ().getName () == implementationName)
		{
			int priority = getPriority (description);
			if(priority < 0)
				continue;
				
			ICoreClass* coreClass = ccl_new<ICoreClass> (description.getClassID ());
			if(coreClass == nullptr)
				continue;
			
			CCL_PRINTF ("Platform implementation candidate \"%s\" (priority %d) for \"%s\"\n", coreClass->getClassInfo ().displayName, priority, MutableCString (packageName).str ())
			candidates.addSorted ({ coreClass, priority });
		}
	EndFor
	candidates.reverse ();
	
	PlatformImplementation* result = nullptr;
	for(Candidate& candidate : candidates)
	{
		if(result == nullptr)
		{
			void* instance = candidate.coreClass->getClassInfo ().createFunction (iid);
			if(instance)
			{
				result = NEW PlatformImplementation (candidate.coreClass, static_cast<Core::IPropertyHandler*> (instance));
				platformImplementationInstances.add (result);
				CCL_PRINTF ("Loaded platform implementation \"%s\" (priority %d) for \"%s\"\n", candidate.coreClass->getClassInfo ().displayName, candidate.priority, MutableCString (packageName).str ())
				continue;
			}
		}
		ccl_release (candidate.coreClass);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlatformIntegrationLoader::releasePlatformImplementation (IPlatformImplementation* implementation)
{
	for(const SharedPtr<PlatformImplementation>& instance : platformImplementationInstances)
	{
		if(instance == implementation)
		{
			platformImplementationInstances.remove (instance);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings& PlatformIntegrationLoader::getSettings ()
{
	if(settings == nullptr)
	{
		settings = NEW XmlSettings (String (kSettingsName));
		settings->isPlatformSpecific (true);
		settings->isBackupEnabled (true);
		settings->restore ();
	}
	return *settings;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlatformIntegrationLoader::restoreSettings (String& implementationName, StringRef packageName, Core::InterfaceID iid)
{
	Settings& settings = getSettings ();
	Attributes& a = settings.getSection (packageName)->getAttributes ();
	
	return a.get (implementationName, kSettingsName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformIntegrationLoader::commitSettings (StringRef packageName, Core::InterfaceID iid, StringRef implementationName)
{
	Settings& settings = getSettings ();
	Attributes& a = settings.getSection (packageName)->getAttributes ();
	
	if(!implementationName.isEmpty ())
		a.set (kSettingsName, implementationName);
	settings.flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PlatformIntegrationLoader::getPriority (const IClassDescription& description) const
{
	int priority = 0;
	
	Variant var;
	description.getClassAttribute (var, PLATFORMINTEGRATION_DEPENDENCIES);
	ForEachStringToken (var.asString (), CCLSTR (";"), dependency)
		if(dependency.isEmpty ())
			continue;
		bool loaded = false;
		for(const SharedPtr<PlatformImplementation>& instance : platformImplementationInstances)
		{
			CStringPtr className = instance->getCoreClass ()->getClassInfo ().displayName;
			if(MutableCString (dependency) == className)
			{
				loaded = true;
				break;
			}
		}
		if(loaded == false)
			priority -= 100;
	EndFor
	
	return priority;
}
