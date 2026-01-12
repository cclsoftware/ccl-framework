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
// Filename    : ccl/app/safety/appsafety.h
// Description : Application Safety Options and Filters
//
//************************************************************************************************

#ifndef _ccl_appsafety_h
#define _ccl_appsafety_h

#include "ccl/public/base/irecognizer.h"
#include "ccl/public/app/isafetyoption.h"
#include "ccl/public/system/cclsafety.h"

namespace CCL {
	
interface IClassDescription;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Safety IDs
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace SafetyID
{
	// Actions
	const CStringPtr kApplicationStartupAction = "ApplicationStartup";
	const CStringPtr kApplicationShutdownAction = "ApplicationShutdown";

	const CStringPtr kOpenDocumentAction = "OpenDocument";
	const CStringPtr kSaveAction = "SaveDocument";
	const CStringPtr kAutoSaveAction = "AutoSave";

	// Options
	static CStringPtr kProfileDocumentSaving = "profileDocumentSaving";
	static CStringPtr kProfileDocumentLoading = "profileDocumentLoading";
}

//************************************************************************************************
// SafetyFilter
/** Base class for safety filters. Filters IClassDescription instances based on a safety option */
//************************************************************************************************

class SafetyFilter: public Unknown,
					public IObjectFilter
{
public:
	SafetyFilter (CStringRef safetyOptionId);

	// IObjectFilter
	tbool CCL_API matches (IUnknown* object) const override;

	CLASS_INTERFACE (IObjectFilter, Unknown)

protected:
	CString optionId;

	virtual bool matches (IClassDescription& description) const;
};

//************************************************************************************************
// PluginOptionsProvider
//************************************************************************************************

class PluginOptionsProvider: public Unknown,
							 public ISafetyOptionProvider
{
public:
	static IObjectFilter* createNativePluginsFilter (const Vector<String>& vendors, const Vector<String>& categories);
	static IObjectFilter* createThirdPartyPluginsFilter (const Vector<String>& vendors, const Vector<String>& categories);

	// ISafetyOptionProvider
	tbool CCL_API checkContext (IUnknown* context) override;
	int CCL_API getOptionCount () const override;
	tbool CCL_API getOptionDescription (SafetyOptionDescription& description, int index) const override;

	CLASS_INTERFACE (ISafetyOptionProvider, Unknown)
};

//************************************************************************************************
// DocumentOptionsProvider
//************************************************************************************************

class DocumentOptionsProvider: public Unknown,
							   public ISafetyOptionProvider
{
public:
	// ISafetyOptionProvider
	tbool CCL_API checkContext (IUnknown* context) override;
	int CCL_API getOptionCount () const override;
	tbool CCL_API getOptionDescription (SafetyOptionDescription& description, int index) const override;

	CLASS_INTERFACE (ISafetyOptionProvider, Unknown)
};

} // namespace CCL

#endif // _ccl_appsafety_h
