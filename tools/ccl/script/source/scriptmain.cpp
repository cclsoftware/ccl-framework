//************************************************************************************************
//
// CCL Script Tool
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
// Filename    : scriptmain.cpp
// Description : Command line Script Tool
//
//************************************************************************************************

#include "appversion.h"

#include "ccl/extras/tools/toolhelp.h"

#include "ccl/base/development.h"

#include "ccl/main/cclargs.h"
#include "ccl/public/cclversion.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/plugins/icoderesource.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//************************************************************************************************
// PlugInFilter
//************************************************************************************************

class PlugInFilter: public UrlFilter
{
public:
	// UrlFilter
	tbool CCL_API matches (UrlRef url) const override
	{
		String fileName;
		url.getName (fileName, false);
		return fileName == CCLSTR ("jsengine");
	}
};

//************************************************************************************************
// ScriptErrorReporter
//************************************************************************************************

class ScriptErrorReporter: public Object,
						   public Alert::IReporter
{
public:
	// Alert::IReporter
	void CCL_API reportEvent (const Alert::Event& e) override
	{
		System::IConsole& console = System::GetConsole ();

		String type;
		type << "Scripting ";
		switch(e.type)
		{
		case Alert::kError : type << "Error"; break;
		case Alert::kWarning : type << "Warning"; break;
		}

		console.writeLine (type);
		console.writeLine (String () << "File: " << e.fileName << " Line: " << e.lineNumber);
		console.writeLine (e.message);
	}

	void CCL_API setReportOptions (Severity severity, int eventFormat) override
	{}

	CLASS_INTERFACE (IReporter, Object)
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_main
//////////////////////////////////////////////////////////////////////////////////////////////////

int ccl_main (ArgsRef args)
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Initialization
	//////////////////////////////////////////////////////////////////////////////////////////////

	ModuleRef module = System::GetCurrentModuleRef ();
	System::IConsole& console = System::GetConsole ();

	System::GetSystem ().setApplicationName (nullptr, APP_NAME);
	System::GetAlertService ().setTitle (APP_NAME);

	if(args.count () < 2)
	{
		console.writeLine (APP_FULL_NAME ", " APP_COPYRIGHT);

		static CStringPtr usage =	"Usage:\n"
									"\t" APP_ID " [filename.js]";

		console.writeLine (usage);
		return -1;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Scan Plug-ins
	//////////////////////////////////////////////////////////////////////////////////////////////

	Url pluginsFolder;
	GET_BUILD_FOLDER_LOCATION (pluginsFolder)
	if(pluginsFolder.isEmpty ())
		System::GetSystem ().getLocation (pluginsFolder, System::kAppPluginsFolder);

	PlugInFilter pluginFilter;
	System::GetPlugInManager ().scanFolder (pluginsFolder, CodeResourceType::kNative, PlugScanOption::kRecursive, nullptr, &pluginFilter);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Start Scripting
	//////////////////////////////////////////////////////////////////////////////////////////////

	System::GetScriptingManager ().startup (APP_PACKAGE_ID, module);
	ScriptErrorReporter errorReporter;
	System::GetScriptingManager ().setReporter (&errorReporter);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Execute Script
	//////////////////////////////////////////////////////////////////////////////////////////////

	Url scriptPath;
	CommandLineTool ().makeAbsolute (scriptPath, args[1], Url::kFile);

	AutoPtr<Scripting::IScript> script = System::GetScriptingManager ().loadScript (scriptPath);
	if(script)
	{
		Variant returnValue;
		System::GetScriptingManager ().executeScript (returnValue, *script);
	}
	else
		console.writeLine (String () << "Failed to load script file: \"" << UrlDisplayString (scriptPath) << "\"");
	script.release ();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Stop Scripting
	//////////////////////////////////////////////////////////////////////////////////////////////

	System::GetScriptingManager ().setReporter (nullptr);
	System::GetScriptingManager ().shutdown (module, true);

	return 0;
}
