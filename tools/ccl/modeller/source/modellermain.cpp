//************************************************************************************************
//
// CCL Modeller Tool
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
// Filename    : modellermain.cpp
// Description : CCL Modeller Tool Main
//
//************************************************************************************************

#include "appversion.h"

#include "ccl/main/cclargs.h"

#include "ccl/extras/modeling/modeltool.h"
#include "ccl/extras/tools/argumentparser.h"

#include "ccl/public/text/cstring.h"
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
	ModelTool::setupArgs (argParser);
	if(argParser.parse (args) != kResultOk)
	{
		console.writeLine ("Usage:");
		argParser.printUsage (console, APP_ID);
		return -1;
	}

	bool result = ModelTool::runFromArgs (argParser);
	return result ? 0 : -1;
}
