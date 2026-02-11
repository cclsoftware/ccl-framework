//************************************************************************************************
//
// CCL String Extractor
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
// Filename    : xstring.cpp
// Description : String Extractor Main
//
//************************************************************************************************

#include "appversion.h"
#include "xstringextractor.h"
#include "xstringparser.h"

#include "ccl/base/development.h"
#include "ccl/extras/tools/argumentparser.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/system/inativefilesystem.h"

using namespace CCL;
using namespace XString;

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_main
//////////////////////////////////////////////////////////////////////////////////////////////////

int ccl_main (ArgsRef _args)
{
	System::IConsole& console = System::GetConsole ();
	console.writeLine (APP_FULL_NAME ", " APP_COPYRIGHT);

	ArgumentParser argParser;
	argParser.add ("mode", {"-skin", "-menu", "-tutorial", "-metainfo", "-template", "-custom", "-auto", "-code"}, "parser mode", Argument::kOptional|Argument::kShiftable, "-auto");
	argParser.add ("inputFolder", "input path, must be folder", 0);
	argParser.add ("format", {"-po", "-xliff"}, "output file format", Argument::kOptional | Argument::kShiftable, "-po");
	argParser.add ("outputFile", "output file to write");
	argParser.add ("model", "path to custom xml model json, can be file or folder", Argument::kOptional);
	argParser.add ("option", {"-v"}, "print debug logs", Argument::kOptional | Argument::kShiftable);

	if(argParser.parse (_args) != kResultOk)
	{
		console.writeLine ("Usage:");
		argParser.printUsage (console, APP_ID);
		console.writeLine ("");
		console.writeLine ("Examples:");
		console.writeLine ("\t" APP_ID " -skin /path/to/skin -po /path/skin.po");
		console.writeLine ("\t" APP_ID " -custom /path/to/custom -po /path/custom.po custom.json -v");

		return -1;
	}

	MutableCString mode (argParser.get ("mode"));

	Url inPath;
	inPath.fromDisplayString (argParser.get ("inputFolder"), Url::kFolder);

	MutableCString outType (argParser.get ("format"));
	Url outPath;
	outPath.fromDisplayString (argParser.get ("outputFile"), Url::kFile);

	Url workDir;
	System::GetFileSystem ().getWorkingDirectory (workDir);

	if(inPath.isRelative ())
		inPath.makeAbsolute (workDir);
	if(outPath.isRelative ())
		outPath.makeAbsolute (workDir);

	Url baseFolder;
	GET_DEVELOPMENT_FOLDER_LOCATION (baseFolder, "tools", "");
	if(!baseFolder.isEmpty ())
		baseFolder.ascend ();
	else // empty in release build, take input folder
		baseFolder = inPath;
	Parser::setBaseFolder (baseFolder);

	AutoPtr<Url> modelPath = nullptr;
	String model = argParser.get ("model");
	if(!model.isEmpty ())
	{
		modelPath = NEW Url;
		modelPath->fromDisplayString (model, Url::kDetect);
		if(modelPath->isRelative ())
			modelPath->makeAbsolute (workDir);
	}

	XString::Extractor extractor (mode, outType, modelPath);

	bool debugLog = argParser.get ("option") == "-v";
	int format = Alert::Event::kWithTime | Alert::Event::kWithSeverity;
	extractor.configureLogging (debugLog ? kSeverityDebug : kSeverityInfo, format);

	bool result = extractor.run (inPath, outPath);
	return result ? 0 : -1;
}
