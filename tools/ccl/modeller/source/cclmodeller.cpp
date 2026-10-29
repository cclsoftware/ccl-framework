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
// Filename    : cclmodeller.cpp
// Description : Modeller Tool
//
//************************************************************************************************

#include "cclmodeller.h"
#include "classscanner.h"
#include "docscanner.h"
#include "typelibscanner.h"

#include "ccl/extras/modeling/classrepository.h"

#include "ccl/public/plugservices.h"
#include "ccl/public/plugins/itypelibregistry.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/logging.h"

using namespace CCL;

//************************************************************************************************
// ModellerTool
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (ModellerTool, kActionList, "-list")
DEFINE_STRINGID_MEMBER_ (ModellerTool, kActionExport, "-export")
DEFINE_STRINGID_MEMBER_ (ModellerTool, kActionScan, "-scan")
DEFINE_STRINGID_MEMBER_ (ModellerTool, kActionStaticExport, "-staticexport")
DEFINE_STRINGID_MEMBER_ (ModellerTool, kActionUpdate, "-update")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModellerTool::run (StringID action, StringRef arg1, StringRef arg2, StringRef typelib)
{
	// Check input arguments, convert them and run requested action.

	if(action == ModellerTool::kActionList)
	{
		logTypeLibraries ();
	}
	else if(action == ModellerTool::kActionExport)
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
	else if(action == ModellerTool::kActionScan)
	{
		// Mandatory args: source path(s), output file.
		Vector<Url> sourcePaths;
		makeAbsolutePaths (sourcePaths, arg1);
		if(sourcePaths.isEmpty () || arg2.isEmpty ())
		{
			Logging::error ("Failed to run scan, missing arguments");
			return false;
		}

		Url modelPath;
		makeAbsolute (modelPath, arg2);

		if(!scanSourceCode (sourcePaths, modelPath))
			return false;
	}
	else if(action == ModellerTool::kActionStaticExport)
	{
		// Mandatory arg: source path(s).
		Vector<Url> sourcePaths;
		makeAbsolutePaths (sourcePaths, arg1);
		if(sourcePaths.isEmpty ())
		{
			Logging::error ("Failed to run static export, missing source path argument", String (action));
			return false;
		}

		// Optional arg: output file, fallback to auto-named file (based on the first path's own name).
		Url outputFile;
		if(!arg2.isEmpty ())
		{
			makeAbsolute (outputFile, arg2);
			addFileExtension (outputFile, Model::ClassRepository::getFileType ().getExtension ());
		}
		else
		{
			System::GetFileSystem ().getWorkingDirectory (outputFile);
			String baseName;
			sourcePaths.at (0).getName (baseName, !sourcePaths.at (0).isFile ());
			outputFile.descend (baseName);
			outputFile.setExtension (Model::ClassRepository::getFileType ().getExtension ());
		}

		if(!typelib.isEmpty ())
		{
			if(!staticExportTypeLibClassModel (sourcePaths, outputFile, typelib))
				return false;
		}
		else if(!staticExportClassModel (sourcePaths, outputFile))
			return false;
	}
	else if(action == ModellerTool::kActionUpdate)
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

void ModellerTool::collectTypeLibraryNames (Vector<String>& names)
{
	// Assemble string list of type library names.
	ASSERT (names.isEmpty ())

	IterForEachUnknown (System::GetTypeLibRegistry ().newIterator (), unk)
		if(UnknownPtr<ITypeLibrary> typeLibrary = unk)
			names.add (typeLibrary->getLibraryName ());
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ModellerTool::addFileExtension (Url& url, StringRef extension)
{
	// Check that 'url' has file extension 'extension' set. User configured
	// (output) paths may be incomplete or specified without an extension.

	String existing;
	url.getExtension (existing);
	if(existing.isEmpty ())
		url.setExtension (extension);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ModellerTool::toDisplayString (const Vector<Url>& urls)
{
	String result;
	for(UrlRef url : urls)
	{
		String display;
		url.toDisplayString (display);
		if(!result.isEmpty ())
			result << ";";
		result << display;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ModellerTool::makeAbsolutePaths (Vector<Url>& paths, StringRef pathList)
{
	// Split a semicolon separated list of (possibly relative) folder or file paths, skipping
	// duplicates. A path is taken as a folder unless only a file exists under that name.

	ForEachStringToken (pathList, CCLSTR (";"), token)
		token.trimWhitespace ();
		if(token.isEmpty ())
			continue;

		Url path;
		makeAbsolute (path, token, Url::kFolder);
		if(!System::GetFileSystem ().fileExists (path))
		{
			Url filePath;
			makeAbsolute (filePath, token, Url::kFile);
			if(System::GetFileSystem ().fileExists (filePath))
				path = filePath;
		}

		bool isDuplicate = false;
		for(UrlRef existing : paths)
			if(path == existing)
			{
				isDuplicate = true;
				break;
			}

		if(isDuplicate)
		{
			String pathStr;
			path.toDisplayString (pathStr);
			Logging::warning ("Ignoring duplicate source path '%(1)'", pathStr);
			continue;
		}

		paths.add (path);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModellerTool::checkPathsExist (const Vector<Url>& paths) const
{
	for(UrlRef path : paths)
	{
		if(System::GetFileSystem ().fileExists (path) == false)
		{
			String pathStr;
			path.toDisplayString (pathStr);
			Logging::error ("Source path '%(1)' does not exist", pathStr);
			return false;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModellerTool::logTypeLibraries () const
{
	// Print registered type libraries to console.

	Vector<String> libraryNames;
	collectTypeLibraryNames (libraryNames);

	for(StringRef name : libraryNames)
		Logging::info ("Found type library '%(1)'", name);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModellerTool::exportClassModel (StringID typeLibraryName, UrlRef outputFile) const
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

bool ModellerTool::scanSourceCode (const Vector<Url>& sourcePaths, UrlRef modelPath) const
{
	// Scan documentation from sources and add it to an existing classmodel file.

	String sourcePathStr = toDisplayString (sourcePaths);
	String modelPathStr;
	modelPath.toDisplayString (modelPathStr);

	if(!checkPathsExist (sourcePaths))
		return false;

	Model::ClassRepository repository;
	if(!repository.loadFromFile (modelPath))
	{
		Logging::error ("Failed to load class model '%(1)'", modelPathStr);
		return false;
	}

	AutoPtr<DocumentationScanner> scanner = DocumentationScanner::createScannerForModel (repository);
	if(scanner)
	{
		if(!scanner->scanCode (sourcePaths))
		{
			Logging::error ("Failed to scan source path '%(1)'", sourcePathStr);
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

	Logging::info ("Scanned sources '%(1)', updated model '%(2)'", sourcePathStr, modelPathStr);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModellerTool::staticExportClassModel (const Vector<Url>& sourcePaths, UrlRef outputFile) const
{
	// Build a classmodel by statically scanning source code, without compiling or running it.

	String sourcePathStr = toDisplayString (sourcePaths);
	String outputFileStr;
	outputFile.toDisplayString (outputFileStr);

	if(!checkPathsExist (sourcePaths))
		return false;

	Model::ClassRepository repository;
	if(!ClassScanner::scan (repository, sourcePaths))
	{
		Logging::error ("Failed to scan source path '%(1)'", sourcePathStr);
		return false;
	}

	String baseName;
	sourcePaths.at (0).getName (baseName, !sourcePaths.at (0).isFile ());
	repository.setName (MutableCString (baseName, Text::kASCII));

	if(!repository.saveToFile (outputFile))
	{
		Logging::error ("Failed to save class model '%(1)'", outputFileStr);
		return false;
	}

	Logging::info ("Statically exported sources '%(1)' to class model '%(2)'", sourcePathStr, outputFileStr);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModellerTool::staticExportTypeLibClassModel (const Vector<Url>& sourcePaths, UrlRef outputFile, StringRef typelibToken) const
{
	// Build a classmodel for one type library (typelibmacros.h) by statically scanning source code,
	// without compiling or running it.

	String sourcePathStr = toDisplayString (sourcePaths);
	String outputFileStr;
	outputFile.toDisplayString (outputFileStr);

	if(!checkPathsExist (sourcePaths))
		return false;

	MutableCString libraryToken (typelibToken);

	Model::ClassRepository repository;
	if(!TypeLibScanner::scan (repository, sourcePaths, libraryToken))
	{
		Logging::error ("Failed to scan source path '%(1)' for type library '%(2)'", sourcePathStr, typelibToken);
		return false;
	}

	repository.setName (libraryToken);

	if(!repository.saveToFile (outputFile))
	{
		Logging::error ("Failed to save class model '%(1)'", outputFileStr);
		return false;
	}

	Logging::info ("Statically exported sources '%(1)' to class model '%(2)' for type library '%(3)'",
		sourcePathStr, outputFileStr, typelibToken);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModellerTool::updateClassModel (UrlRef documentedPath, UrlRef prototypePath) const
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
