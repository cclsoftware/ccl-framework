//************************************************************************************************
//
// CCL Modeller
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : cclmodellermain.cpp
// Description : Modeller Tool Main
//
//************************************************************************************************

#include "cclmodeller.h"
#include "appversion.h"

#include "ccl/extras/tools/argumentparser.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_main
//////////////////////////////////////////////////////////////////////////////////////////////////

int ccl_main (ArgsRef args)
{
	System::IConsole& console = System::GetConsole ();
	console.writeLine (APP_FULL_NAME ", " APP_COPYRIGHT);

	ArgumentParser argParser;
	argParser.add (
		"action",
		{String (ModellerTool::kActionList), String (ModellerTool::kActionExport), String (ModellerTool::kActionScan),
		 String (ModellerTool::kActionStaticExport), String (ModellerTool::kActionUpdate)}, // choices
		"model action to perform"
	);

	argParser.add (
		"arg1",
		"[-export]: type library name, [-scan]/[-staticexport]: source folder(s) or file(s), semicolon separated, [-update]: documented file",
		Argument::kOptional
	);

	argParser.add (
		"arg2",
		"[-export]/[-staticexport]: classmodel output file, optional, [-scan]: model file, [-update]: prototype file",
		Argument::kOptional
	);

	argParser.add (
		"typelibOption",
		{"typelib"},
		"[-staticexport]: type library macro token to export (e.g. typelib=SKIN_TYPELIB), switches into type library mode",
		Argument::kOptional | Argument::kShiftable | Argument::kExpectsValue
	);

	argParser.add (
		"optverbose",
		{"-v"},
		"print debug logs",
		Argument::kOptional | Argument::kShiftable
	);

	if(argParser.parse (args) != kResultOk)
	{
		console.writeLine ("Usage:");
		argParser.printUsage (console, APP_ID);
		return -1;
	}

	String action = argParser.get ("action");
	String arg1 = argParser.get ("arg1");
	String arg2 = argParser.get ("arg2");
	String typelib = argParser.get ("typelibOption");
	bool debugLog = argParser.get ("optverbose") == "-v";

	ModellerTool modeller;
	modeller.configureLogging (debugLog ? kSeverityDebug : kSeverityInfo);

	bool result = modeller.run (MutableCString (action), arg1, arg2, typelib);
	return result ? 0 : -1;
}
