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
// Filename    : ccl/extras/extensions/pluginversionprovider.h
// Description : Plug-in Version Provider
//
//************************************************************************************************

#ifndef _ccl_pluginversionprovider_h
#define _ccl_pluginversionprovider_h

#include "ccl/app/browser/pluginmanagement.h"

namespace CCL {
namespace Install {

//************************************************************************************************
// PlugInVersionProvider
//************************************************************************************************

class PlugInVersionProvider: public Unknown,
							 public IPlugInVersionProvider
{
public:
	PlugInVersionProvider ();

	// IPlugInVersionProvider
	tresult CCL_API getVersionString (String& version, const IClassDescription& description) const override;
	tresult CCL_API getLastModifiedTime (FileTime& lastModified, const IClassDescription& description) const override;

	CLASS_INTERFACE (IPlugInVersionProvider, Unknown)

private:
	Url plugInsPath;
	DateTime appModifiedTime;
};

}; // namespace Install
}; // namespace CCL

#endif // _ccl_pluginversionprovider_h
