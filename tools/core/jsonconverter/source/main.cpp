//************************************************************************************************
//
// JSON Converter
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
// Filename    : main.cpp
// Description : JSON Converter
//
//************************************************************************************************

#include "core/portable/corepersistence.h"

#include "core/public/coreversion.h"

using namespace Core;
using namespace Portable;

enum Arguments
{
	kInputFile = 1,
	kOutputFile,
	kNumArguments
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void printhelp (void)
{
	printf ("JSON Converter " CORE_AUTHOR_COPYRIGHT "\n");
	printf ("Usage: jsonconverter <inputfile> <outputfile>\n");
	printf ("\n");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int convert (CStringPtr inputFileName, CStringPtr outputFileName, Archiver::Format inputFormat, Archiver::Format outputFormat)
{
	Attributes attributes (AttributeAllocator::getDefault ());
	if(!ArchiveUtils::loadFromFile (attributes, inputFileName, inputFormat))
	{
		printf ("ERROR: Could not parse input file\n");
		return 1;
	}
	if(!ArchiveUtils::saveToFile (outputFileName, attributes, outputFormat))
	{
		printf ("ERROR: Could not write output file\n");
		return 1;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char* argv[])
{
	if(argc != kNumArguments)
	{
		printhelp ();
		return 0;
	}

	bool isInputUbjson = false;
	ConstString inputFileName (argv[kInputFile]);
	if(inputFileName.endsWith (".ubj") || inputFileName.endsWith (".ubjson"))
		isInputUbjson = true;
	else if(!inputFileName.endsWith (".json"))
	{
		printf ("ERROR: Unexpected input file extension (should be one of .ubj, .ubjson or .json)\n");
		return 1;
	}

	FileUtils::deleteFile (argv[kOutputFile]);
	
	if(isInputUbjson)
		return convert (argv[kInputFile], argv[kOutputFile], Archiver::kUBJSON, Archiver::kJSON);
	else
		return convert (argv[kInputFile], argv[kOutputFile], Archiver::kJSON, Archiver::kUBJSON);
}

