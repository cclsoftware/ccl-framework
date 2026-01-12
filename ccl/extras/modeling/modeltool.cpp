//************************************************************************************************
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
// Filename    : ccl/extras/modeling/modeltool.cpp
// Description : Class Model Tool
//
//************************************************************************************************

#include "modeltool.h"
#include "docscanner.h"

#include "ccl/extras/tools/argumentparser.h"
#include "ccl/extras/modeling/classrepository.h"

#include "ccl/public/plugservices.h"
#include "ccl/public/plugins/itypelibregistry.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/logging.h"

using namespace CCL;

//************************************************************************************************
// ModelTool
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (ModelTool, kActionList, "-list")
DEFINE_STRINGID_MEMBER_ (ModelTool, kActionExport, "-export")
DEFINE_STRINGID_MEMBER_ (ModelTool, kActionScan, "-scan")
DEFINE_STRINGID_MEMBER_ (ModelTool, kActionUpdate, "-update")

//////////////////////////////////////////////////////////////////////////////////////////////////

void ModelTool::setupArgs (ArgumentParser& parser)
{
	parser.add (
		"action",
		{String (ModelTool::kActionList), String (ModelTool::kActionExport), String (ModelTool::kActionScan), String (ModelTool::kActionUpdate)}, // choices
		"model action to perform"
	);

	parser.add (
		"arg1",
		"[-export]: type library name, [-scan]: source folder, [-update]: documented file",
		Argument::kOptional
	);

	parser.add (
		"arg2",
		"[-export]: classmodel output file, optional, [-scan]: model file, [-update]: prototype file",
		Argument::kOptional
	);

	parser.add (
		"optverbose",
		{"-v"},
		"print debug logs",
		Argument::kOptional | Argument::kShiftable
	);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModelTool::runFromArgs (const ArgumentParser& argParser)
{
	String action = argParser.get ("action");
	String arg1 = argParser.get ("arg1");
	String arg2 = argParser.get ("arg2");
	bool debugLog = argParser.get ("optverbose") == "-v";

	ModelTool tool;
	tool.configureLogging (debugLog ? kSeverityDebug : kSeverityInfo);

	bool result = tool.run (MutableCString (action), arg1, arg2);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModelTool::run (StringID action, StringRef arg1, StringRef arg2)
{
	// Check input arguments, convert them and run requested action.

	if(action == ModelTool::kActionList)
	{
		logTypeLibraries ();
	}
	else if(action == ModelTool::kActionExport)
	{
		// Mandatory arg: type library name.
		MutableCString typeLibraryName = arg1;
		if(typeLibraryName.isEmpty ())
		{
			Logging::error ("Failed to export classmodel, missing type library name argument", String (action));
			return false;
		}

		// Optional arg: output file, fallback to auto-named file.
		Url outputFile;
		if(!arg2.isEmpty ())
		{
			makeAbsolute (outputFile, arg2);
			addFileExtension (outputFile, Model::ClassRepository::getFileType ().getExtension ());
		}
		else
		{
			System::GetFileSystem ().getWorkingDirectory (outputFile);
			outputFile.descend (String (typeLibraryName));
			outputFile.setExtension (Model::ClassRepository::getFileType ().getExtension ());
		}

		if(!exportClassModel (typeLibraryName, outputFile))
			return false;
	}
	else if(action == ModelTool::kActionScan)
	{
		// Mandatory args: sources path, output file.
		if(arg1.isEmpty () || arg2.isEmpty ())
		{
			Logging::error ("Failed to run scan, missing arguments");
			return false;
		}

		Url sourceFolder, modelPath;
		makeAbsolute (sourceFolder, arg1, Url::kFolder);
		makeAbsolute (modelPath, arg2);

		if(!scanSourceCode (sourceFolder, modelPath))
			return false;
	}
	else if(action == ModelTool::kActionUpdate)
	{
		// Mandatory args: file to merge into, file to merge.
		if(arg1.isEmpty () || arg2.isEmpty ())
		{
			Logging::error ("Failed to run update, missing arguments");
			return false;
		}

		Url documentedPath, prototypePath;
		makeAbsolute (documentedPath, arg1);
		makeAbsolute (prototypePath, arg2);

		if(!updateClassModel (documentedPath, prototypePath))
			return false;
	}
	else
	{
		Logging::error ("Unsupported action '%(1)'", String (action));
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ModelTool::collectTypeLibraryNames (Vector<String>& names)
{
	// Assemble string list of type library names.
	ASSERT (names.isEmpty ())

	IterForEachUnknown (System::GetTypeLibRegistry ().newIterator (), unk)
		if(UnknownPtr<ITypeLibrary> typeLibrary = unk)
			names.add (typeLibrary->getLibraryName ());
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ModelTool::addFileExtension (Url& url, StringRef extension)
{
	// Check that 'url' has file extension 'extension' set. User configured
	// (output) paths may be incomplete or specified without an extension.

	String existing;
	url.getExtension (existing);
	if(existing.isEmpty ())
		url.setExtension (extension);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModelTool::logTypeLibraries () const
{
	// Print registered type libraries to console.

	Vector<String> libraryNames;
	collectTypeLibraryNames (libraryNames);

	for(StringRef name : libraryNames)
		Logging::info ("Found type library '%(1)'", name);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModelTool::exportClassModel (StringID typeLibraryName, UrlRef outputFile) const
{
	// Export registered type library to classmodel file.

	ITypeLibrary* typeLibrary = System::GetTypeLibRegistry ().findTypeLib (typeLibraryName);
	if(typeLibrary == nullptr)
	{
		// Requested type library not found, log registered libraries.
		Vector<String> libraryNames;
		collectTypeLibraryNames (libraryNames);

		String knownTypeLibraries = "[";
		for(StringRef name : libraryNames)
			knownTypeLibraries << "'" << name << "', ";

		knownTypeLibraries << "]";
		knownTypeLibraries.replace (", ]", "]");

		Logging::error ("Could not find type library '%(1)', known libraries are %(2)",
			String (typeLibraryName), knownTypeLibraries);

		return false;
	}

	Model::ClassRepository repository;
	Model::ClassRepositoryBuilder builder (repository);
	if(!builder.build (*typeLibrary))
	{
		Logging::error ("Failed to build class repository for type library '%(1)'", String (typeLibraryName));
		return false;
	}

	repository.setName (typeLibraryName);

	String outputFileStr;
	outputFile.toDisplayString (outputFileStr);

	if(!repository.saveToFile (outputFile))
	{
		Logging::error ("Failed to export class model '%(1)'", outputFileStr);
		return false;
	}

	Logging::info ("Wrote '%(1)' class model '%(2)'", String (typeLibraryName), outputFileStr);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModelTool::scanSourceCode (UrlRef sourceFolder, UrlRef modelPath) const
{
	// Scan documentation from sources and add it to an existing classmodel file.

	String sourceFolderStr;
	sourceFolder.toDisplayString (sourceFolderStr);
	String modelPathStr;
	modelPath.toDisplayString (modelPathStr);

	ASSERT (sourceFolder.isFolder ())
	if(System::GetFileSystem ().fileExists (sourceFolder) == false)
	{
		Logging::error ("Source folder '%(1)' does not exist", sourceFolderStr);
		return false;
	}

	Model::ClassRepository repository;
	if(!repository.loadFromFile (modelPath))
	{
		Logging::error ("Failed to load class model '%(1)'", modelPathStr);
		return false;
	}

	AutoPtr<DocumentationScanner> scanner = DocumentationScanner::createScannerForModel (repository);
	if(scanner)
	{
		if(!scanner->scanCode (sourceFolder))
		{
			Logging::error ("Failed to scan source folder '%(1)'", sourceFolderStr);
			return false;
		}

		scanner->applyToModel (repository);

		if(!repository.saveToFile (modelPath))
		{
			Logging::error ("Failed to save updated class model '%(1)'", modelPathStr);
			return false;
		}
	}
	else
	{
		Logging::error ("Source code scan not supported for class model '%(1)'", modelPathStr);
		return false;
	}

	Logging::info ("Scanned sources '%(1)', updated model '%(2)'", sourceFolderStr, modelPathStr);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModelTool::updateClassModel (UrlRef documentedPath, UrlRef prototypePath) const
{
	// Merge a classmodel into an existing one, updating the existing one.

	String documentedPathStr;
	documentedPath.toDisplayString (documentedPathStr);

	String prototypePathStr;
	prototypePath.toDisplayString (prototypePathStr);

	Model::ClassRepository documented;
	if(!documented.loadFromFile (documentedPath))
	{
		Logging::error ("Failed to load documented class model '%(1)'", documentedPathStr);
		return false;
	}

	Model::ClassRepository prototype;
	if(!prototype.loadFromFile (prototypePath))
	{
		Logging::error ("Failed to load prototype class model '%(1)'", prototypePathStr);
		return false;
	}

	Model::ClassRepository temp;
	Model::ClassRepositoryBuilder (temp).update (documented, prototype);
	documented.removeAll ();
	documented.takeAll (temp);

	if(!documented.saveToFile (documentedPath))
	{
		String documentedPathStr;
		documentedPath.toDisplayString (documentedPathStr);
		Logging::error ("Failed to save updated class model to '%(1)'", documentedPathStr);
		return false;
	}

	Logging::info ("Updated model '%(1)' with prototype model '%(2)'", documentedPathStr, prototypePathStr);
	return true;
}
