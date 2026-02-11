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
// Filename    : testrunnermain.cpp
// Description : Test Runner Main
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "appversion.h"
#include "testtool.h"
#include "testreporter.h"
#include "junitreporter.h"

#include "ccl/extras/tools/argumentparser.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_main
//////////////////////////////////////////////////////////////////////////////////////////////////

int ccl_main (ArgsRef args)
{
	System::GetSystem ().setApplicationName (nullptr, APP_NAME);
	System::GetAlertService ().setTitle (APP_NAME);

	System::GetConsole ().writeLine (APP_FULL_NAME ", " APP_COPYRIGHT);

	// Parse arguments
	ArgumentParser argumentParser;
	argumentParser.add 
	(
		"plug-ins",
		"The test plug-ins",
		Argument::kOptional,
		"none"
	);

	argumentParser.add 
	(
		"filter",
		{"-filter"},
		"Filter the tests to be run by suite and/or test name",
		Argument::kOptional | Argument::kShiftable | Argument::kExpectsValue,
		"*"
	);
	
	argumentParser.add
	(
		"verbose",
		{"-v"},
		"Print debug logs",
		Argument::kOptional | Argument::kShiftable
	);

	argumentParser.add
	(
		"junit-format",
		{"-junit"},
		"Generate JUnit formatted output",
		Argument::kOptional | Argument::kShiftable
	);

	argumentParser.add
	(
		"internal-tests",
		{"-internal"},
		"Run internal tests",
		Argument::kOptional | Argument::kShiftable
	);

	if(argumentParser.parse (args, ArgumentParser::kAllowUnknownArguments) != kResultOk)
	{
		argumentParser.printUsage (System::GetConsole (), APP_NAME);
		return -1;
	}

	TestTool testTool;
	
	bool enableDebugLogs = argumentParser.get ("verbose") == "-v";
	testTool.configureLogging (enableDebugLogs ? kSeverityDebug : kSeverityInfo);

	// Load plug-ins
	if(argumentParser.get ("plug-ins") != "none")
		testTool.addPluginUrlFromPath (argumentParser.get ("plug-ins"));

	for(auto* argument : argumentParser.getUnparsedArguments ())
		testTool.addPluginUrlFromPath (*argument);

	if(argumentParser.get ("internal-tests") == "-internal")
		testTool.loadInternalTests ();
	testTool.loadPlugins ();

	// Add test reporters
	AutoPtr<ITestReporter> testReporter;
	if(argumentParser.get ("junit-format") == "-junit")
		testReporter = NEW JUnit::TestReporter ();
	else
		testReporter = NEW TestReporter ();

	testTool.addTestReporter (testReporter);

	// Add filter
	String filter = argumentParser.get ("filter");
	testTool.run (filter);

	return testReporter->allTestsPassed ()? 0 : 1;
}
