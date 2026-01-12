//************************************************************************************************
//
// CCL Package Tool
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
// Filename    : package.cpp
// Description : Command line package tool
//
//************************************************************************************************

#include "appversion.h"

#include "ccl/main/cclargs.h"
#include "ccl/extras/tools/publisher.h"
#include "ccl/extras/tools/argumentparser.h"

#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/securityservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_main
//////////////////////////////////////////////////////////////////////////////////////////////////

int ccl_main (ArgsRef args)
{
	System::IConsole& console = System::GetConsole ();

	ArgumentParser argParser;
	argParser.add ("action", "action to perform");
	argParser.add ("first", "first positonal argument");
	argParser.add ("second", "second positonal argument", Argument::kOptional);
	argParser.add ("third", "third positonal argument", Argument::kOptional);
	argParser.add ("destination", {"-dest"}, "set an output directory prefix", Argument::kOptional | Argument::kShiftable | Argument::kExpectsValue);
	argParser.add ("depfile", {"-depfile"}, "generate a dependency file", Argument::kOptional | Argument::kShiftable | Argument::kExpectsValue);
	
	if(argParser.parse (args) != kResultOk)
	{
		console.writeLine (APP_FULL_NAME ", " APP_COPYRIGHT);
	
		static CStringPtr usage =	"Usage:\n"
									"\t" APP_ID " -[action] [args...]\n"
									"\n"
									"\t* Compress package file : -c inFolder outPackage [filter]\n"
									"\t* Encrypt package file  : -e inFolder outPackage [filter]\n"
									"\t* Create ZIP file       : -z inFolder outPackage [filter]\n"
									"\t* Create plain ZIP file : -p inFolder outPackage [filter]\n"
									"\t* Batch processing      : -batch batchFile\n"
									"\t* Data embedding        : -d packageFile dataFile comment\n"
									"\n"
									"\tAdditional action options (append to the action in a single argument, e.g. -e-aes):\n"
									"\n"
									"\t-v2 : package format V2\n"
									"\t-v3 : package format V3\n"
									"\t-r8k : 8KB reserved block\n"
									"\t-key=XX... : external encryption key\n"
									"\t-xtea : use XTEA algorithm for encryption\n"
									"\t-aes : use AES algorithm for encryption\n"
									"\t-hidden : preserve hidden attribute\n"
									"\n"
									"\tGeneric options (specify as separate arguments):\n"
									"\n"
									"\t-dest=XX... : set an output directory prefix\n"
									"\t-depfile=XX... : generate a dependency file\n";

		console.writeLine (usage);
		return -1;
	}

	// assign factory for strong content encryption
	System::GetPackageHandler ().setCryptoFactory (&System::GetCryptoFactory ());

	MutableCString actionString = argParser.get ("action").asString ();
	String firstArgument = argParser.get ("first").asString ();
	String secondArgument = argParser.get ("second").asString ();
	String thirdArgument = argParser.get ("third").asString ();

	Url destDir;
	String destDirString = argParser.get ("destination").asString ();
	if(!destDirString.isEmpty ())
		CommandLineTool ().makeAbsolute (destDir, destDirString, IUrl::kFile);

	Url dependencyFilePath;
	String dependencyFileString = argParser.get ("depfile").asString ();
	if(!dependencyFileString.isEmpty ())
		CommandLineTool ().makeAbsolute (dependencyFilePath, dependencyFileString, IUrl::kFile, destDir);

	// trim to primary action string
	MutableCString action (actionString);
	int nextSeparator = CString (action.str () + 1).index ("-");
	if(nextSeparator != -1)
		action.truncate (nextSeparator + 1);

	// *** Create Package File ***
	if(action == "-c" || action == "-e" || action == "-z" || action == "-p")
	{
		Url inPath;
		CommandLineTool ().makeAbsolute (inPath, firstArgument, Url::kFolder);

		Url outPath;
		CommandLineTool ().makeAbsolute (outPath, secondArgument, Url::kFile, destDir);

		PackFolderOptions options;
		options.fromString (MutableCString (actionString));
		ExtensionFilter filter (thirdArgument);
		filter.applyOptions (options);

		if(!ToolHelper::packageFolder (outPath, inPath, filter, true, options, nullptr, dependencyFilePath))
		{
			console.writeLine (String () << "Failed to create package file: " << UrlDisplayString (outPath));
			return -1;
		}
	}
	// *** Batch Processing ***
	else if(action == "-batch")
	{
		Publisher publisher;
		publisher.setDependencyFilePath (dependencyFilePath);
		publisher.setOutputBaseDir (destDir);
		if(!publisher.run (firstArgument))
		{
			console.writeLine (String () << "Batch processing failed on: " << publisher.run (firstArgument));
			return -1;
		}
	}
	// *** Embedd Data **
	else if(action == "-d")
	{
		Url dstPath;
		CommandLineTool ().makeAbsolute (dstPath, firstArgument, Url::kFile, destDir);

		Url srcPath;
		CommandLineTool ().makeAbsolute (srcPath, secondArgument, Url::kFile);

		String comment (thirdArgument);

		if(!ToolHelper::embeddDataInPackageFile (dstPath, srcPath, comment))
		{
			console.writeLine (String () << "Failed to embedd data into package file: " << UrlDisplayString (dstPath));
			return -1;
		}
	}
	else
	{
		console.writeLine ("Unknown action!");
		return -1;
	}

	return 0;
}
