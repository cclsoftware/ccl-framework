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
// Filename    : xstringextractor.cpp
// Description : String Extractor Tool
//
//************************************************************************************************

#include "xstringextractor.h"
#include "xstringmodehandler.h"
#include "xstringmodel.h"
#include "xlffilter.h"
#include "pofilter.h"

using namespace CCL;
using namespace XString;

//************************************************************************************************
// Extractor
//************************************************************************************************

Extractor::Extractor (CStringRef scanMode, CStringRef outputFormat, IUrl* userModelPath)
: scanMode (scanMode),
  outputFormat (outputFormat),
  userModelPath (userModelPath)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Extractor::run (UrlRef inPath, UrlRef outPath)
{
	if(!System::GetFileSystem ().fileExists (inPath))
	{
		Logging::error ("Input path '%(1)' does not exist", UrlDisplayString (inPath));
		return false;
	}

	Bundle bundle;
	if(!parseFolder (bundle, inPath))
	{
		Logging::error ("Parser aborted");
		return false;
	}
	Logging::info ("Found %(1) strings total", bundle.countEntries ());

	String outputFile = UrlDisplayString (outPath);
	if(!generateOutput (bundle, outPath))
	{
		Logging::error ("Failed to create output file '%(1)'", outputFile);
		return false;
	}
	Logging::info ("Wrote output file '%(1)'", outputFile);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Extractor::parseFolder (Bundle& bundle, UrlRef inPath) const
{
	AutoPtr<ModeHandler> handler = createHandler (bundle, inPath);
	if(handler == nullptr)
	{
		Logging::error ("Unsupported parser mode '%(1)'", String (scanMode));
		return false;
	}

	return handler->run ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Extractor::generateOutput (Bundle& bundle, UrlRef outPath) const
{
	if(outputFormat == "-proto")
		return PrototypeFilter (bundle, outPath).create ();
	else if(outputFormat == "-ref")
		return ReferenceFilter (bundle, outPath).create ();
	else if(outputFormat == "-po")
		return PortableObjectFilter (bundle, outPath).create ();
	else if(outputFormat == "-xliff")
		return XliffFilter (bundle, outPath).create ();
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ModeHandler* Extractor::createHandler (Bundle& bundle, UrlRef inPath) const
{
	if(scanMode == "-skin")
		return NEW BuiltInXmlHandler (bundle, inPath, userModelPath, "Skin");
	else if(scanMode == "-menu")
		return NEW BuiltInXmlHandler (bundle, inPath, userModelPath, "MenuBar");
	else if(scanMode == "-tutorial")
		return NEW BuiltInXmlHandler (bundle, inPath, userModelPath, "HelpTutorialCollection");
	else if(scanMode == "-metainfo")
		return NEW BuiltInXmlHandler (bundle, inPath, userModelPath, "MetaInformation");
	else if(scanMode == "-template")
		return NEW BuiltInXmlHandler (bundle, inPath, userModelPath, "DocumentTemplate");
	else if(scanMode == "-custom")
		return NEW CustomXmlHandler (bundle, inPath, userModelPath);
	else if(scanMode == "-auto")
		return NEW AutoModeHandler (bundle, inPath, userModelPath);
	else if(scanMode == "-code")
		return NEW SourceCodeHandler (bundle, inPath);

	// Mode not supported.
	return nullptr;
}
