//************************************************************************************************
//
// CCL Builder
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
// Filename    : cclbuildermain.cpp
// Description : CCL Builder Main
//
//************************************************************************************************

#include "cclbuilder.h"
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
	argParser.add ("verbose", {"-v"}, "print debug logs", Argument::kOptional | Argument::kShiftable);
	argParser.add ("interactive", {"-i"}, "start interactive mode", Argument::kOptional | Argument::kShiftable);
	argParser.add ("templatefolder", {"-templatefolder"}, "a path to a folder containing json files with template descriptions", Argument::kOptional | Argument::kShiftable | Argument::kExpectsValue);
	argParser.add ("template", {"-template"}, "the name of a template", Argument::kOptional | Argument::kShiftable | Argument::kExpectsValue);
	argParser.add ("vendor", {"-vendor"}, "vendor identifier", Argument::kOptional | Argument::kShiftable | Argument::kExpectsValue);
	argParser.add ("platforms", {"-platforms"}, "comma-separated list of platforms to be supported by the generated project", Argument::kOptional | Argument::kShiftable | Argument::kExpectsValue);
	argParser.add ("destination", {"-destination"}, "destination path", Argument::kOptional | Argument::kShiftable | Argument::kExpectsValue);

	Builder builder;

	bool succeeded = argParser.parse (args, ArgumentParser::kAllowUnknownArguments) == kResultOk && (!argParser.get ("template").asString ().isEmpty () || !argParser.get ("interactive").asString ().isEmpty ());
	if(!succeeded)
	{
		console.writeLine ("Usage:");
		argParser.printUsage (console, APP_ID, "[key value] [key2 value2] [...]");
		console.writeLine ("");
		console.writeLine ("Examples:");
		console.writeLine ("\t" APP_ID " - template apptemplate ProjectName myapp ProjectTitle \"My Application\" AuthorName \"Some Author\"");
		console.writeLine ("");
	}

	bool debugLog = argParser.get ("verbose") == "-v";
	builder.configureLogging (debugLog ? kSeverityDebug : kSeverityInfo);

	Url templateFolder;
	templateFolder.fromDisplayString (argParser.get ("templatefolder"), IUrl::kFolder);
	builder.addTemplateFolder (templateFolder);

	builder.initialize ();

	if(!succeeded)
	{
		builder.listTemplates ();
		return -1;
	}

	builder.setTemplateName (argParser.get ("template"));

	builder.setVendorID (argParser.get ("vendor"));

	builder.setPlatforms (argParser.get ("platforms"));

	builder.setDestinationPath (argParser.get ("destination"));

	builder.setInteractive (argParser.get ("interactive").asString ().isEmpty () || argParser.get ("template").asString ().isEmpty ());

	for(int i = 0; i + 1 < argParser.getUnparsedArguments ().count (); i += 2)
		builder.setVariable (argParser.getUnparsedArguments ().at (i), argParser.getUnparsedArguments ().at (i + 1));

	bool result = builder.run ();
	return result ? 0 : -1;
}
