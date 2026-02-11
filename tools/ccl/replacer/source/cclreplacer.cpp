//************************************************************************************************
//
// CCL Replacer
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
// Filename    : cclreplacer.cpp
// Description : Replacer Tool
//
//************************************************************************************************

#include "cclreplacer.h"

#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/textfile.h"

#include "ccl/public/system/logging.h"

using namespace CCL;

//************************************************************************************************
// HeaderRecipe
//************************************************************************************************

HeaderRecipe::HeaderRecipe ()
: maxLineCount (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderRecipe::startsWithCommentStyle (String& outStyle, StringRef candidate) const
{
	for(StringRef style : commentStyles)
		if(candidate.startsWith (style))
		{
			outStyle = style;
			return true;
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const HeaderRecipe::Replacement* HeaderRecipe::matchesReplacements (StringRef candidate) const
{
	for(const Replacement& replacement : replacements)
		if(replacement.searchDescription->matchesName (candidate))
			return &replacement;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderRecipe::load (const Storage& storage)
{
	const Attributes& a = storage.getAttributes ();

	rootFolder = a.getString ("root");
	
	String fileTypesString = a.getString ("fileTypes");
	ForEachStringToken (fileTypesString, ";", tokenString)
		Url url;
		url.setName (tokenString);
		if(url.getFileType ().isValid ())
			fileTypes.addFileType (url.getFileType ());
	EndFor

	IterForEach (a.newQueueIterator ("commentStyles", ccl_typeid<Attribute> ()), Attribute, attr)
		String style = attr->getValue ().asString ();
		if(!style.isEmpty ())
			commentStyles.add (style);
	EndFor

	if(const Attributes* optionsAttr = a.getAttributes ("options"))
	{
		maxLineCount = optionsAttr->getInt ("maxLineCount");
	}

	IterForEach (a.newQueueIterator ("replacements", ccl_typeid<Attributes> ()), Attributes, attr)
		Replacement replacement;
		StringID replaceMode = attr->getCString ("replaceMode");
		replacement.replaceMode = replaceMode == "remove" ? Replacement::kRemove : 
								  replaceMode == "remove+1" ? Replacement::RemovePlusOne :
								  Replacement::kReplace;
		
		String searchString = attr->getString ("searchString");
		if(searchString.isEmpty ())
			continue;

		replacement.searchDescription = SearchDescription::create (Url::kEmpty, searchString);

		if(replacement.replaceMode == Replacement::kReplace)
		{
			if(attr->contains ("replaceString"))
				replacement.replaceStrings.add (attr->getString ("replaceString"));
			else
			{
				IterForEach (attr->newQueueIterator ("replaceStrings", ccl_typeid<Attribute> ()), Attribute, itemAttr)
					replacement.replaceStrings.add (itemAttr->getValue ().asString ());
				EndFor
			}
		}

		replacements.add (replacement);
	EndFor

	return true;
}

//************************************************************************************************
// ReplacerTool
//************************************************************************************************

ReplacerTool::ReplacerTool ()
: mode (kHeaderMode)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ReplacerTool::run ()
{
	ASSERT (mode == kHeaderMode)
	return runHeadeMode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ReplacerTool::runHeadeMode ()
{
	// load recipe
	Url recipePath;
	recipePath.fromDisplayString (recipeFile);
	makeAbsolute (recipePath);

	HeaderRecipe recipe;
	if(!recipe.loadFromFile (recipePath))
	{
		Logging::error (String () << "Failed to load recipe from file: " << UrlDisplayString (recipePath));
		return false;
	}
	
	Url rootPath;
	rootPath.fromDisplayString (recipe.getRootFolder (), Url::kFolder);
	makeAbsolute (rootPath);
	if(!File (rootPath).exists ())
	{
		Logging::error (String () << "Root folder not found: " << UrlDisplayString (rootPath));
		return false;
	}

	HeaderModeStats stats;
	bool result = runHeaderReplacementRecursive (stats, rootPath, recipe);	

	Logging::info (String () << stats.totalFileCount << " total files checked");
	Logging::info (String () << stats.modifiedFileCount << " files modified");

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ReplacerTool::runHeaderReplacementRecursive (HeaderModeStats& stats, UrlRef folder, const HeaderRecipe& recipe)
{
	ForEachFile (File (folder).newIterator (), path)
		if(path->isFolder ())
		{
			if(!runHeaderReplacementRecursive (stats, *path, recipe))
				return false;
		}
		else if(recipe.getFileTypes ().matches (*path))
		{
			if(!replaceFileHeader (stats, *path, recipe))
				return false;
		}
	EndFor	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ReplacerTool::replaceFileHeader (HeaderModeStats& stats, UrlRef path, const HeaderRecipe& recipe)
{
	stats.totalFileCount++;

	TextFile srcFile (path, TextFile::kOpen);
	if(!srcFile.isValid ())
	{
		Logging::error (String () << "Failed to open source file: " << UrlDisplayString (path));
		return false;
	}

	Logging::info (String () << "Replacing header in file " << UrlDisplayString (path) << "...");

	Vector<String> lines;
	bool replaced = false;
	bool removeNext = false;
	while(1)
	{
		// give up if not found in max. header line count
		bool inHeader = lines.count () <= recipe.getMaxLineCount ();
		if(inHeader == false && replaced == false)
			break;

		String line;
		if(!srcFile->readLine (line))
			break;
			
		if(inHeader)
		{
			if(removeNext)
			{
				removeNext = false;
				replaced = true;
				continue;
			}

			String trimmedLine (line);
			trimmedLine.trimWhitespace ();

			String commentStyle;
			if(recipe.startsWithCommentStyle (commentStyle, trimmedLine))
				if(const HeaderRecipe::Replacement* replacement = recipe.matchesReplacements (trimmedLine))
				{
					if(replacement->replaceMode == HeaderRecipe::Replacement::kReplace)
					{
						for(int i = 0; i < replacement->replaceStrings.count (); i++)
						{
							String replacedLine;
							replacedLine << commentStyle << " " << replacement->replaceStrings[i];
							lines.add (replacedLine);
						}
					}

					removeNext = replacement->replaceMode == HeaderRecipe::Replacement::RemovePlusOne;
					replaced = true;
					continue;
				}
		}

		lines.add (line);
	}

	if(!replaced)
	{
		Logging::info ("...no matching header found.");
		return true;
	}

	TextEncoding encoding = srcFile->getTextEncoding ();
	TextLineFormat lineFormat = srcFile->getLineFormat ();
	srcFile.close ();

	// rewrite file with original attributes
	bool rewriteFailed = false;
	TextFile dstFile (path, encoding, lineFormat, ITextStreamer::kSuppressByteOrderMark);
	if(dstFile.isValid ())
	{
		for(StringRef line : lines)
			if(!dstFile->writeLine (line))
			{
				rewriteFailed = true;
				break;
			}
		
		dstFile.close ();
	}
	else
		rewriteFailed = true;

	if(rewriteFailed)
	{
		Logging::error (String () << "Failed to rewrite file: " << UrlDisplayString (path));
		return false;
	}

	stats.modifiedFileCount++;
	Logging::info ("...replacement successful.");
	return true;
}
