//************************************************************************************************
//
// CCL Replacer
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
// Filename    : cclreplacermain.cpp
// Description : Replacer Tool Main
//
//************************************************************************************************

#include "cclreplacer.h"
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
	argParser.add ("mode", {"-header"}, "select tool mode");
	argParser.add ("recipe", "path to recipe file", Argument::kExpectsValue);
	argParser.add ("verbose", {"-v"}, "print debug logs", Argument::kOptional | Argument::kShiftable);

	bool succeeded = argParser.parse (args) == kResultOk;
	if(!succeeded)
	{
		console.writeLine ("Usage:");
		argParser.printUsage (console, APP_ID);
		console.writeLine ("");
		console.writeLine ("Examples:");
		console.writeLine ("\"" APP_ID " -header recipe myrecipe.json\"");
		console.writeLine ("");
		return -1;
	}

	ReplacerTool replacer;

	bool debugLog = argParser.get ("verbose") == "-v";
	replacer.configureLogging (debugLog ? kSeverityDebug : kSeverityInfo);
	
	bool headerMode = argParser.get ("mode") == "-header";
	ASSERT (headerMode) // no other mode yet
	replacer.setRecipeFile (argParser.get ("recipe"));

	bool result = replacer.run ();
	return result ? 0 : -1;
}
