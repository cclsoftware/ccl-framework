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
// Filename    : xstringmodehandler.cpp
// Description : Parser modes
//
//************************************************************************************************

#include "xstringmodehandler.h"

#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace XString;

//************************************************************************************************
// FileStats
//************************************************************************************************

FileStats::FileStats ()
: count (0),
  result (Parser::Result::kFileNotParsed)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileStats::log () const
{
	// Convert parser result code to human readable string - safely.
	// Keep switch in sync with Parser::Result enum.
	auto toDisplayResult = [] (Parser::Result result) -> String
	{
		switch(result)
		{
		case Parser::Result::kFileOk :
			return CCLSTR ("ok");
		case Parser::Result::kFileInvalid :
			return CCLSTR ("invalid");
		case Parser::Result::kFileUnsupported :
			return CCLSTR ("unsupported format");
		case Parser::Result::kFileNotParsed :
			return CCLSTR ("unsupported extension");
		case Parser::Result::kFileInvalidRoot :
			return CCLSTR ("invalid root element element");
		default:
			break;
		}

		return CCLSTR ("unknown reason");
	};

	// Print positives as info level to minimize output, print issues as debug.
	switch(result)
	{
	case Parser::Result::kFileOk :
		if(count > 0)
			Logging::info ("Found %(1) strings in '%(2)'", count, name);
		else
			Logging::debug ("No strings found in '%(1)'", name);
		break;
	case Parser::Result::kFileNotParsed :
	case Parser::Result::kFileUnsupported :
	case Parser::Result::kFileInvalidRoot :
		Logging::debug ("Skipped file '%(1)', %(2)", name, toDisplayResult (result));
		break;
	case Parser::Result::kFileInvalid :
		Logging::warning ("Skipped file '%(1)', %(2)", name, toDisplayResult (result));
		break;
	default:
		break;
	}
}

//************************************************************************************************
// ModeHandler
//************************************************************************************************

ModeHandler::ModeHandler (Bundle& bundle, UrlRef inPath)
 : bundle (bundle),
   inPath (inPath)
{}

//************************************************************************************************
// ExclusiveModeHandler
//************************************************************************************************

ExclusiveModeHandler::ExclusiveModeHandler (Bundle& bundle, UrlRef inPath)
: ModeHandler (bundle, inPath)
{}

//************************************************************************************************
// SourceCodeHandler
//************************************************************************************************

SourceCodeHandler::SourceCodeHandler (Bundle& bundle, UrlRef inPath)
: ExclusiveModeHandler (bundle, inPath)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SourceCodeHandler::run ()
{
	return parseFolder<SourceParser> (bundle, inPath);
}

//************************************************************************************************
// BuiltInXmlHandler
//************************************************************************************************

BuiltInXmlHandler::BuiltInXmlHandler (Bundle& bundle, UrlRef inPath, IUrl* modelPath, String rootFilter)
: ExclusiveModeHandler (bundle, inPath),
  modelPath (modelPath),
  rootFilter (rootFilter)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BuiltInXmlHandler::run ()
{
	if(!initModels ())
		return false;

	return parseFolder<XmlParser> (bundle, inPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BuiltInXmlHandler::initModels ()
{
	// Exclusive mode must register a single model only.
	auto& registry = XmlModelRegistry::instance ();
	registry.setRootFilter (rootFilter);
	registry.loadBuiltIns ();
	if(modelPath)
		registry.load (*modelPath);

	if(registry.countModels () != 1)
	{
		Logging::error ("Invalid XML model configuration");
		return false;
	}

	return true;
}

//************************************************************************************************
// CustomXmlHandler
//************************************************************************************************

CustomXmlHandler::CustomXmlHandler (Bundle& bundle, UrlRef inPath, IUrl* modelPath)
: ExclusiveModeHandler (bundle, inPath),
  modelPath (modelPath)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomXmlHandler::run ()
{
	if(!initModels ())
		return false;

	return parseFolder<XmlParser> (bundle, inPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomXmlHandler::initModels ()
{
	auto& registry = XmlModelRegistry::instance ();
	if(modelPath)
		registry.load (*modelPath);

	// Exclusive mode, expect single model only.
	// Implicit check for modelPath != nullptr.
	if(registry.countModels () != 1)
	{
		Logging::error ("Invalid XML model configuration");
		return false;
	}

	return true;
}

//************************************************************************************************
// AutoModeHandler
//************************************************************************************************

AutoModeHandler::AutoModeHandler (Bundle& bundle, UrlRef inPath, IUrl* modelPath)
: ModeHandler (bundle, inPath),
  modelPath (modelPath)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AutoModeHandler::run ()
{
	if(!initModels ())
		return false;

	parseFolder (inPath);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AutoModeHandler::initModels ()
{
	auto& registry = XmlModelRegistry::instance ();
	registry.loadBuiltIns ();
	if(modelPath)
		registry.load (*modelPath);

	// modelPath may introduce any number of new formats or
	// overwrite any number of built-in formats.
	if(registry.countModels () == 0)
	{
		Logging::error ("Invalid XML model configuration");
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AutoModeHandler::parseFolder (UrlRef inPath)
{
	Logging::info ("Parsing folder '%(1)'", UrlDisplayString (inPath));

	ForEachFile (System::GetFileSystem ().newIterator (inPath), _path)
		UrlRef path = *_path;
		if(path.isFolder ())
			parseFolder (path);
		else if(path.isFile ())
		{
			FileStats stats;
			String fileName;
			path.getName (fileName);
			stats.setName (fileName);
			if(AutoPtr<Parser> parser = createParser (path))
			{
				int count = bundle.countEntries ();
				stats.setResult (parser->parse ());
				stats.setCount (bundle.countEntries () - count);
			}
			stats.log ();
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parser* AutoModeHandler::createParser (UrlRef inPath)
{
	if(SourceParser::getFilter ().matches (inPath))
		return NEW SourceParser (bundle, inPath);
	else if(XmlParser::getFilter ().matches (inPath))
		return NEW XmlParser (bundle, inPath);
	else
		return nullptr;
};
