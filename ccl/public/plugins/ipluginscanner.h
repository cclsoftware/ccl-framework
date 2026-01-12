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
// Filename    : ccl/public/plugins/ipluginscanner.h
// Description : Plug-in Scanner Interface
//
//************************************************************************************************

#ifndef _ccl_ipluginscanner_h
#define _ccl_ipluginscanner_h

#include "ccl/public/plugins/ipluginmanager.h"

namespace CCL {

interface IUnknownList;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (PlugInScanner, 0x581e285a, 0xa8d4, 0x4801, 0x85, 0xe4, 0x74, 0xb8, 0xb2, 0xf0, 0x9a, 0xf)
}

//************************************************************************************************
// IPlugInScanner
//************************************************************************************************

interface IPlugInScanner: IUnknown
{
	virtual tresult CCL_API addFolder (UrlRef url, ICodeResourceLoader& loader, int options = PlugScanOption::kRecursive,
									   IUrlFilter* urlFilter = nullptr, IObjectFilter* classFilter = nullptr) = 0;

	enum Mode { kStartupMode, kRescanMode };

	virtual tresult CCL_API run (int mode, StringRef context = nullptr, IUnknownList* restartList = nullptr) = 0;

	virtual int CCL_API getResultCount () const = 0;
	
	DECLARE_IID (IPlugInScanner)
};

DEFINE_IID (IPlugInScanner, 0x7bc81e27, 0xca69, 0x411d, 0xac, 0x7e, 0x68, 0x69, 0x5f, 0x7e, 0x98, 0xf0)

} // namespace CCL

#endif // _ccl_ipluginscanner_h
