//************************************************************************************************
//
// CCL Test Runner
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
// Filename    : testtool.h
// Description : Headless Test Tool
//
//************************************************************************************************

#ifndef _ccl_testtool_h
#define _ccl_testtool_h

#include "ccl/extras/tools/testrunner.h"
#include "ccl/extras/tools/toolhelp.h"

namespace CCL {

//************************************************************************************************
// TestTool
//************************************************************************************************

class TestTool: public CommandLineTool,
				public TestRunner
{
public:
	TestTool ();
	~TestTool ();

	void addPluginUrlFromPath (StringRef path);
	void loadPlugins ();
	void loadInternalTests ();

private:
	Vector<AutoPtr<Url>> pluginUrls;
	bool internalTestsLoaded;

	void loadPlugin (UrlRef url);
	void logSuccess (UrlRef pluginUrl);
	void logFailure (UrlRef pluginUrl);
};

} // namespace CCL

#endif // _ccl_testtool_h
