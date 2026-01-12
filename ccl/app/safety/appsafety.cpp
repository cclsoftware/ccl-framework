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
// Filename    : ccl/app/safety/appsafety.cpp
// Description : Application Safety Options and Filters
//
//************************************************************************************************

#include "ccl/app/safety/appsafety.h"
#include "ccl/app/component.h"

#include "ccl/public/app/idocument.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/isafetymanager.h"
#include "ccl/public/plugins/ipluginmanager.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// PluginsFilter
//************************************************************************************************

class PluginsFilter: public SafetyFilter
{
public:
	PluginsFilter (CStringRef safetyOptionId);

	void filterVendors (const Vector<String>& vendors, bool exclusive);
	void filterCategory (StringRef category);

	// SafetyFilter
	bool matches (IClassDescription& description) const override;

protected:
	bool vendorFilterExclusive;
	Vector<String> vendors;
	Vector<String> categories;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Option IDs
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace SafetyID 
{ 
	static CStringPtr kDisableNativePlugins = "disableNativePlugins";
	static CStringPtr kDisableThirdPartyPlugins = "disableThirdPartyPlugins";
}

} // namespace CCLs

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("AppSafetyProviders")
	XSTRING (DisableNativePluginsOption, "Do not load $APPCOMPANY plug-ins")
	XSTRING (DisableNativePluginsExplanation, "Do not load plug-ins made by $APPCOMPANY.")
	XSTRING (DisableThirdPartyPluginsOption, "Do not load third-party plug-ins")
	XSTRING (DisableThirdPartyPluginsExplanation, "Do not load plug-ins made by third-party vendors.")
	XSTRING (ProfileSavingOption, "Profile document saving")
	XSTRING (ProfileSavingExplanation, "Find items with long save times or large size.")
	XSTRING (ProfileLoadingOption, "Profile document loading")
	XSTRING (ProfileLoadingExplanation, "Find items with long load times.")
END_XSTRINGS

BEGIN_XSTRINGS ("CCLAppSafety")
	XSTRING (ApplicationStartupAction, "Start $APPNAME")
	XSTRING (ApplicationShutdownAction, "Quit $APPNAME")
	XSTRING (OpenDocumentAction, "Open document \"%(1)\"")
	XSTRING (SaveDocumentAction, "Save document \"%(1)\"")
	XSTRING (AutoSaveAction, "Auto-save")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (CCLAppSafety, kSetupLevel)
{
	if(System::IsInMainAppModule ())
	{
		System::GetSafetyManager ().registerAction (SafetyID::kApplicationStartupAction, XSTR (ApplicationStartupAction));
		System::GetSafetyManager ().registerAction (SafetyID::kApplicationShutdownAction, XSTR (ApplicationShutdownAction));
		System::GetSafetyManager ().registerAction (SafetyID::kOpenDocumentAction, XSTR (OpenDocumentAction));
		System::GetSafetyManager ().registerAction (SafetyID::kSaveAction, XSTR (SaveDocumentAction));
		System::GetSafetyManager ().registerAction (SafetyID::kAutoSaveAction, XSTR (AutoSaveAction));
	}
	return true;
}

//************************************************************************************************
// PluginOptionsProvider
//************************************************************************************************

IObjectFilter* PluginOptionsProvider::createNativePluginsFilter (const Vector<String>& vendors, const Vector<String>& categories)
{
	PluginsFilter* filter = NEW PluginsFilter (SafetyID::kDisableNativePlugins);
	filter->filterVendors (vendors, false);
	for(StringRef category : categories)
		filter->filterCategory (category);
	return filter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectFilter* PluginOptionsProvider::createThirdPartyPluginsFilter (const Vector<String>& vendors, const Vector<String>& categories)
{
	PluginsFilter* filter = NEW PluginsFilter (SafetyID::kDisableThirdPartyPlugins);
	filter->filterVendors (vendors, true);
	for(StringRef category : categories)
		filter->filterCategory (category);
	return filter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool PluginOptionsProvider::checkContext (IUnknown* context)
{
	// Enable for any document type for now.
	UnknownPtr<IDocument> document = context;
	return document != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PluginOptionsProvider::getOptionCount () const
{
	return 2;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool PluginOptionsProvider::getOptionDescription (SafetyOptionDescription& description, int index) const
{
	switch(index)
	{
	case 0:
		description.id = SafetyID::kDisableNativePlugins;
		description.title = XSTR (DisableNativePluginsOption);
		description.explanationText = XSTR (DisableNativePluginsExplanation);
		return true;
	case 1:
		description.id = SafetyID::kDisableThirdPartyPlugins;
		description.title = XSTR (DisableThirdPartyPluginsOption);
		description.explanationText = XSTR (DisableThirdPartyPluginsExplanation);
		return true;
	default:
		return false;
	}
}

//************************************************************************************************
// DocumentOptionsProvider
//************************************************************************************************

tbool DocumentOptionsProvider::checkContext (IUnknown* context)
{
	// Enable for any document type for now.
	UnknownPtr<IDocument> document = context;
	return document != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DocumentOptionsProvider::getOptionCount () const
{
	return 2;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool DocumentOptionsProvider::getOptionDescription (SafetyOptionDescription& description, int index) const
{
	switch(index)
	{
	case 0:
		description.id = SafetyID::kProfileDocumentLoading;
		description.title = XSTR (ProfileLoadingOption);
		description.explanationText = XSTR (ProfileLoadingExplanation);
		description.displayPriority = 200;
		return true;
	case 1:
		description.id = SafetyID::kProfileDocumentSaving;
		description.title = XSTR (ProfileSavingOption);
		description.explanationText = XSTR (ProfileSavingExplanation);
		description.displayPriority = 200;
		return true;
	default:
		return false;
	}
}

//************************************************************************************************
// SafetyFilter
//************************************************************************************************

SafetyFilter::SafetyFilter (CStringRef safetyOptionId)
: optionId (safetyOptionId)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SafetyFilter::matches (IUnknown* object) const
{
	UnknownPtr<IClassDescription> description = object;
	if(description)
	{
		if(System::GetSafetyManager ().getValue (optionId) == false)
			return false;

		return matches (*description);
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SafetyFilter::matches (IClassDescription& description) const
{
	return true;
}

//************************************************************************************************
// PluginsFilter
//************************************************************************************************

PluginsFilter::PluginsFilter (CStringRef optionId)
: SafetyFilter (optionId),
  vendorFilterExclusive (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PluginsFilter::filterVendors (const Vector<String>& _vendors, bool exclusive)
{
	vendors = _vendors;
	vendorFilterExclusive = exclusive;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PluginsFilter::filterCategory (StringRef category)
{
	categories.add (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PluginsFilter::matches (IClassDescription& description) const
{
	bool matchesVendor = false;
	for(StringRef vendor : vendors)
	{
		if(description.getModuleVersion ().getVendor () == vendor)
		{
			matchesVendor = true;
			break;
		}
	}
	if(vendorFilterExclusive == matchesVendor)
		return false;

	bool matchesType = false;
	for(StringRef category : categories)
	{
		if(category == description.getCategory ())
		{
			matchesType = true;
			break;
		}
	}
	return matchesType;
}
