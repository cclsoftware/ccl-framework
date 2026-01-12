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
// Filename    : ccl/extras/tools/publisher.cpp
// Description : Publisher Tool
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/extras/tools/publisher.h"

#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/textfile.h"
#include "ccl/base/storage/xmlpihandler.h"

#include "ccl/main/cclargs.h"

#include "ccl/public/system/iexecutable.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// Publisher
//************************************************************************************************

const String Publisher::kListDelimiter = "&";
const String Publisher::kHash = "#";

//////////////////////////////////////////////////////////////////////////////////////////////////

Publisher::Publisher ()
: currentMacro (nullptr),
  errorMode (kFail)
{
	inputDirectories.objectCleanup ();
	addInputDirectory (workDir);
	setOutputDir (workDir);
	
	macros.objectCleanup (true);
	
	updateRepositoryInfo (workDir);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Publisher::addInputDirectory (UrlRef path)
{
	inputDirectories.add (NEW Url (path));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Publisher::getInputDirectory (Url& path, int index) const
{
	if(inputDirectories.isEmpty () || inputDirectories.count () < (index + 1))
		return false;
	if(Url* inputPath = ccl_cast<Url> (inputDirectories.at (index)))
	{
		path = *inputPath;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Publisher::makeInputPath (Url& path, StringRef fileName, int type, int index)
{
	path.fromDisplayString (fileName, type);
	Url inDir;
	if(getInputDirectory (inDir, index))
		path.makeAbsolute (inDir);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Publisher::makeOutputPath (Url& path, StringRef fileName, int type)
{
	path.fromDisplayString (fileName, type);
	path.makeAbsolute (outDir);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Publisher::addDefinition (StringRef key, StringRef value, bool list)
{
	if(key.isEmpty ())
		return false;
	
	Vector<Definition>& usedList = list ? listDefinitions : definitions;
	
	int index = usedList.index (Definition (key));
	if(index != -1)
		usedList.removeAt (index);

	usedList.add (Definition (key, value));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Publisher::updateRepositoryInfo (UrlRef _workDir)
{
	if(!_workDir.isEqualUrl (repositoryWorkDir))
	{
		repositoryInfo.load (_workDir, true);
		repositoryWorkDir = _workDir;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Publisher::preprocess (StringRef string) const
{
	String result;
	ForEachStringToken (string, " ", token)
		String processedToken;
		if(token.startsWith (kHash))
			processedToken = token.subString (1);
		else
		{
			StringList listElements;
			VectorForEach (listDefinitions, Definition, listDefinition)
				if(token.contains (listDefinition.key))
				{
					ForEachStringToken (listDefinition.value, kListDelimiter, listToken)
						String expanded (token);
						expanded.replace (listDefinition.key, listToken);
						listElements.add (expanded);
					EndFor
				}
			EndFor
			if(listElements.isEmpty ())
				listElements.add (token);
			for(auto listElement : listElements)
			{
				String element (*listElement);
				VectorForEach (definitions, Definition, definition)
					element.replace (definition.key, definition.value);
				EndFor
				if(!processedToken.isEmpty ())
					processedToken << kListDelimiter;
				processedToken << element;
			}
		}
		if(!result.isEmpty ())
			result << " ";
		result << processedToken;
	EndFor
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Publisher::Macro* Publisher::findMacro (StringRef name) const
{
	ListForEachObject (macros, Macro, m)
		if(m->name == name)
			return m;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Publisher::runMacro (const Macro& macro)
{
	ForEach (macro.lines, Boxed::String, str)
		if(!runLine (*str))
		{
			CCL_PRINTLN ("Failed in macro.")
			return false;
		}
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Publisher::runLine (StringRef line)
{
	String command;
	String name;

	if(currentMacro && !line.contains ("#endMacro"))
	{
		CCL_PRINT ("Adding to macro ")
		CCL_PRINT (currentMacro->name)
		CCL_PRINT (" : ")
		CCL_PRINTLN (line)

		currentMacro->lines.add (line);
		return true;
	}

	static const String colon = CCLSTR (":");
	int colonIndex = line.index (colon);
	if(colonIndex != -1)
	{
		command = line.subString (0, colonIndex);
		name = line.subString (colonIndex + 1);
	}
	else
		command = line;

	command.trimWhitespace ();
	name.trimWhitespace ();

	if(command.isEmpty ())
		return false;

	if(!command.startsWith (kHash) || command == "#include")
		name = preprocess (name);

	CCL_PRINT (command)
	CCL_PRINT (" : ")
	CCL_PRINTLN (name)

	return perform (MutableCString (command), name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Publisher::setDependencyFilePath (UrlRef path)
{
	dependencyFile = NEW DependencyFile (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Publisher::perform (StringID command, StringRef name)
{
	bool succeeded = true;

	updateRepositoryInfo (workDir);

	static const CString macroSymbol ("@");

	// *** Preprocessor ***
	if(command.startsWith ("#define"))
	{
		// check for per-platform definition
		bool enabled = true;
		if(command.contains ("."))
			enabled = command.endsWith (XmlProcessingInstructionHandler::getPlatform ());

		if(enabled == true)
		{
			int index = name.index ("=");
			String key = name.subString (0, index);
			key.trimWhitespace ();
			String value = name.subString (index+1);
			value.trimWhitespace ();
			succeeded = addDefinition (key, value);
		}
	}
	else if(command == "#include")
	{
		succeeded = run (name, false); // keep working directory!
	}
	else if(command == "#beginMacro")
	{
		succeeded = currentMacro == nullptr;
		currentMacro = NEW Macro;
		currentMacro->name = name;
		macros.add (currentMacro);
	}
	else if(command == "#endMacro")
	{
		succeeded = currentMacro != nullptr;
		currentMacro = nullptr;
	}

	// *** Macros ***
	else if(command.startsWith (macroSymbol))
	{
		String macroName = String (command).subString (1);
		const Macro* macro = findMacro (macroName);
		succeeded = macro && runMacro (*macro);
	}

	// *** Search Paths ***
	else if(command == "findPath" || command == "findPathList")
	{
		int tokenIndex = 0;
		String key, category, innerPath;
		ForEachStringToken (name, " ", token)
			token.trimWhitespace ();
			if(token.isEmpty ())
				continue;
			switch(tokenIndex++)
			{
			case 0: key = token; break;
			case 1: category = token; break;
			case 2: innerPath = token; break;
			}
		EndFor

		if(command == "findPathList")
		{
			ObjectArray paths;
			paths.objectCleanup ();
			if(repositoryInfo.findAllPaths (paths, MutableCString (category, Text::kASCII), innerPath))
			{
				String pathsString;
				ForEach (paths, Url, path)
					if(!pathsString.isEmpty ())
						pathsString << kListDelimiter;
					pathsString << UrlDisplayString (*path);
				EndFor
				addDefinition (key, pathsString, true);
				succeeded = true;
			}
		}
		else
		{
			Url path;
			if(!repositoryInfo.findPath (path, MutableCString (category, Text::kASCII), innerPath))
				succeeded = false;
			else
				succeeded = addDefinition (key, UrlDisplayString (path));
		}
	}

	// *** Configuration ***
	else if(command == "setInputDir")
	{
		inputDirectories.removeAll ();
		Url inputPath;
		makeAbsolute (inputPath, name, Url::kFolder);
		addInputDirectory (inputPath);

		//console.writeLine ("Input directory set to:");
		//console.writeLine (UrlDisplayString (inputPath));
	}
	else if(command == "setInputDirList")
	{
		inputDirectories.removeAll ();
		ForEachStringToken (name, kListDelimiter, token)
			token.trimWhitespace ();
			if(token.isEmpty ())
				continue;
			Url inputPath;
			makeAbsolute (inputPath, token, Url::kFolder);
			addInputDirectory (inputPath);
		EndFor
	}
	else if(command == "setOutputDir")
	{
		makeAbsolute (outDir, name, Url::kFolder, outBaseDir);

		//console.writeLine ("Output directory set to:");
		//console.writeLine (UrlDisplayString (outDir));
	}
	else if(command == "setErrorMode")
	{
		setErrorMode (name.contains ("dontfail", false) ? kDontFail : kFail);
	}

	// *** File Filter ***
	else if(command == "setFilter")
	{
		filter.setPositive (name == "true" || name == "1");
	}
	else if(command == "addFilter")
	{
		filter.add (name);
	}
	else if(command == "removeFilter")
	{
		filter.remove (name);
	}
	else if(command == "resetFilter")
	{
		filter.removeAll ();
		filter.setPositive (false);
	}
	else if(command == "loadFilter")
	{
		Url filterPath;
		filterPath.fromDisplayString (name);
		makeAbsolute (filterPath);
		succeeded = filter.loadFromFile (filterPath);
	}

	// *** Copy Operations ***
	else if(command.startsWith ("copyFile"))
	{
		String srcFileName, dstFileName;
		int renameIndex = name.index (">");
		if(renameIndex != -1)
		{
			srcFileName = name.subString (0, renameIndex);
			srcFileName.trimWhitespace ();
			dstFileName = name.subString (renameIndex + 1);
			dstFileName.trimWhitespace ();
		}
		else
			srcFileName = dstFileName = name;

		Url srcPath;
		makeInputPath (srcPath, srcFileName);

		Url dstPath;
		makeOutputPath (dstPath, dstFileName);

		if(dependencyFile)
		{
			dependencyFile->setOutputFile (dstPath);
			dependencyFile->addDependency (srcPath);
		}

		succeeded = ToolHelper::copyFile (dstPath, srcPath) || dontFail ();
	}
	else if(command == "copyFolder" || command == "copyFolder-f" || command == "copyFolder-r")
	{
		for(int i = 0; i < inputDirectories.count (); i++)
		{
			Url srcPath;
			makeInputPath (srcPath, name, Url::kFolder, i);

			Url dstPath;
			makeOutputPath (dstPath, name, Url::kFolder);

			if(dependencyFile)
			{
				dependencyFile->setOutputFile (dstPath);
				dependencyFile->addDependency (srcPath);
			}
			
			bool recursive = command == "copyFolder-r";
			bool success = ToolHelper::copyFolder (dstPath, srcPath, filter, recursive) || dontFail ();
			if(!success)
				succeeded = false;
		}
	}

	// *** Package Operations ***
	else if(command.startsWith ("packageFolder"))
	{
		bool recursive = command.contains ("-r");
		PackFolderOptions options;
		options.fromString (command);

		Url srcPath;
		if(!getInputDirectory (srcPath))
			succeeded = false;
		else
		{
			Url dstPath;
			makeOutputPath (dstPath, name, Url::kFile);

			if(dependencyFile)
			{
				dependencyFile->setOutputFile (dstPath);
				dependencyFile->addDependency (srcPath);
			}

			filter.applyOptions (options);
			succeeded = ToolHelper::packageFolder (dstPath, srcPath, filter, recursive, options) || dontFail ();
			filter.resetOptions ();
		}
	}

	// *** Others ***
	else if(command == "replace")
	{
		// Syntax: srcfile "searchString"="replacementString" > dstfile
		int tokenIndex = 0;
		String srcFileName, searchString, replacementString, dstFileName;
		ForEachStringToken (name, "\"=>", token)
			token.trimWhitespace ();
			if(token.isEmpty ())
				continue;
			switch(tokenIndex++)
			{
			case 0 : srcFileName = token; break;
			case 1 : searchString = token; break;
			case 2 : replacementString = token; break;
			case 3 : dstFileName = token; break;
			}
		EndFor

		if(dstFileName.isEmpty ()) // dstfile is optional
			dstFileName = srcFileName;

		Url srcPath;
		makeInputPath (srcPath, srcFileName);

		Url dstPath;
		makeOutputPath (dstPath, dstFileName, Url::kFile);

		if(dependencyFile)
		{
			dependencyFile->setOutputFile (dstPath);
			dependencyFile->addDependency (srcPath);
		}

		succeeded = false;
		TextResource textFile; // hardcoded to UTF-8 with special JSON handling for now
		textFile.setSuppressByteOrderMark (srcPath.getFileType () == FileTypes::Json ());
		if(textFile.loadFromFile (srcPath))
		{
			String content (textFile.getContent ());
			content.replace (searchString, replacementString);
			textFile.setContent (content);

			succeeded = textFile.saveToFile (dstPath);
		}

		if(dontFail ())
			succeeded = true;
	}
	else if(command == "call")
	{
		// Syntax: exename[]arguments
		const String kArgumentSeparator ("[]");
		int argSeparatorIndex = name.index (kArgumentSeparator);
		String exeFileName = name.subString (0, argSeparatorIndex);
		String argumentString;
		if(argSeparatorIndex != -1)
			argumentString = name.subString (argSeparatorIndex + kArgumentSeparator.length ());
		MutableArgumentList arguments (argumentString);

		Url exePath;
		makeInputPath (exePath, exeFileName);

		Url oldWorkDir, newWorkDir;
		System::GetFileSystem ().getWorkingDirectory (oldWorkDir);
		if(getInputDirectory (newWorkDir))
		{
			System::GetFileSystem ().setWorkingDirectory (newWorkDir);

			int flags = System::kSuppressProcessGUI|System::kWaitForProcessExit;

			TempFile tempFile ("call_output");
			AutoPtr<IStream> outputStream = tempFile.open (IStream::kOpenMode|IStream::kWriteMode);
			flags |= System::kRedirectProcessOutput;

			Threading::ProcessID processId = 0;
			tresult exitCode = System::GetExecutableLoader ().execute (processId, exePath, arguments, flags, outputStream);

			outputStream.release ();
			StringList output;
			TextUtils::loadStringList (output, tempFile.getPath (), true, Text::kUTF8);
			ForEach (output, Boxed::String, string)
				console.writeLine (*string);
			EndFor

			System::GetFileSystem ().setWorkingDirectory (oldWorkDir);

			succeeded = exitCode == kResultOk || dontFail ();
		}
		else
			succeeded = false;
	}
	else if(command == "print")
	{
		console.writeLine (name);
	}
	else if(command == "return")
	{
		setReturnValue (name);
	}
	else
	{
		succeeded = false;
	}

	return succeeded;
}
