//************************************************************************************************
//
// CCL Generator
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
// Filename    : cclgeneratormain.cpp
// Description : CCL Generator Main
//
//************************************************************************************************

#include "cclgenerator.h"
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
	argParser.add ("mode", {"-g", "-p"}, "select tool mode (generate or parse)");
	argParser.add ("input", "path to input file", Argument::kExpectsValue);
	argParser.add ("output", "path to output file", Argument::kExpectsValue);
	argParser.add ("template", "path to template file", Argument::kExpectsValue|Argument::kOptional);
	argParser.add ("option", {"-v"}, "print debug logs", Argument::kOptional | Argument::kShiftable);

	bool succeeded = argParser.parse (args) == kResultOk;
	if(!succeeded)
	{
		console.writeLine ("Usage:");
		argParser.printUsage (console, APP_ID);
		console.writeLine ("");
		return -1;
	}

	GeneratorTool generator (APP_NAME " v" APP_SHORT_VERSION);
	
	bool debugLog = argParser.get ("option") == "-v";
	int format = Alert::Event::kWithTime | Alert::Event::kWithSeverity;
	generator.configureLogging (debugLog ? kSeverityDebug : kSeverityInfo, format);
	
	MutableCString modeString (argParser.get ("mode"));
	generator.setMode (modeString == "-p" ? 
					   GeneratorTool::Mode::kParse : 
					   GeneratorTool::Mode::kGenerate);
	generator.setInputFile (argParser.get ("input"));
	generator.setOutputFile (argParser.get ("output"));
	generator.setTemplateFile (argParser.get ("template"));

	succeeded = generator.run ();
	return succeeded ? 0 : -1;
}
