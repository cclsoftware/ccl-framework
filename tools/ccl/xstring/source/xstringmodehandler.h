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
// Filename    : xstringmodehandler.h
// Description : Parser modes
//
//************************************************************************************************

#ifndef _xstringmodehandler_h
#define _xstringmodehandler_h

#include "xstringparser.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/logging.h"

namespace XString {

//************************************************************************************************
// FileStats
/** Store and report parser stats per file. */
//************************************************************************************************

class FileStats
{
public:
	FileStats ();

	PROPERTY_STRING (name, Name)  ///< Name of processed file.
	PROPERTY_VARIABLE (int, count, Count) ///< Number of strings found in file.
	PROPERTY_VARIABLE (Parser::Result, result, Result); ///< Parser processing result

	void log () const;
};

//************************************************************************************************
// ModeHandler
/** Parse strings from a path, write to bundle. */
//************************************************************************************************

class ModeHandler: public CCL::Object
{
public:
	virtual bool run () = 0;

protected:
	Bundle& bundle; ///< Bundle to write to.
	CCL::UrlRef inPath; ///< Root input path.

	ModeHandler (Bundle& bundle, CCL::UrlRef inPath);
};

//************************************************************************************************
// ExclusiveModeHandler
//************************************************************************************************

class ExclusiveModeHandler: public ModeHandler
{
protected:
	ExclusiveModeHandler (Bundle& bundle, CCL::UrlRef inPath);

	/**
	 * Parse inPath recursively for a specific parser type.
	 * Skip files that do not match the parser supported file
	 * extensions.
	 *
	 * @param bundle  bundle to write to
	 * @param inPath  folder to parse recursively
	 */
	template <class Parser>
	static bool parseFolder (Bundle& bundle, CCL::UrlRef inPath)
	{
		CCL::Logging::info ("Parsing folder '%(1)'", CCL::UrlDisplayString (inPath));

		CCL::IUrlFilter& filter = Parser::getFilter ();
		ForEachFile (CCL::System::GetFileSystem ().newIterator (inPath), _path)
			CCL::UrlRef path = *_path;
			if(path.isFolder ())
			{
				if(!parseFolder<Parser> (bundle, path))
					return false;
			}
			else if(path.isFile ())
			{
				FileStats stats;
				CCL::String fileName;
				path.getName (fileName);
				stats.setName (fileName);
				if(filter.matches (path))
				{
					int count = bundle.countEntries ();
					Parser p (bundle, path);
					stats.setResult (p.parse ());
					stats.setCount (bundle.countEntries () - count);
				}
				stats.log ();

				// Legacy behavior: stop execution on malformed file
				if(stats.getResult () == Parser::Result::kFileInvalid)
				{
					CCL::Logging::error ("Execution canceled due to error in file '%(1)'", stats.getName ());
					return false;
				}
			}
		EndFor
		return true;
	}
};

//************************************************************************************************
// SourceCodeHandler
/** Parse strings from sources. */
//************************************************************************************************

class SourceCodeHandler: public ExclusiveModeHandler
{
public:
	SourceCodeHandler (Bundle& bundle, CCL::UrlRef inPath);

	// ExclusiveModeHandler
	bool run () override;
};

//************************************************************************************************
// BuiltInXmlHandler
/** Parse strings from a single, built-in XML format. */
//************************************************************************************************

class BuiltInXmlHandler: public ExclusiveModeHandler
{
public:
	BuiltInXmlHandler (Bundle& bundle, CCL::UrlRef inPath, CCL::IUrl* modelPath, CCL::String rootFilter);

	// ExclusiveModeHandler
	bool run () override;

protected:
	CCL::IUrl* modelPath; ///< Optional models path.
	CCL::String rootFilter; ///< Name of root element model to load.

	bool initModels ();
};

//************************************************************************************************
// CustomXmlHandler
/** Parse strings from a single, custom XML format. */
//************************************************************************************************

class CustomXmlHandler: public ExclusiveModeHandler
{
public:
	CustomXmlHandler (Bundle& bundle, CCL::UrlRef inPath, CCL::IUrl* modelPath);

	// ExclusiveModeHandler
	bool run () override;

protected:
	CCL::IUrl* modelPath; // Mandatory model format spec.

	bool initModels ();
};

//************************************************************************************************
// AutoModeHandler
/** Parse strings from any supported file type and format. */
//************************************************************************************************

class AutoModeHandler: public ModeHandler
{
public:
	AutoModeHandler (Bundle& bundle, CCL::UrlRef inPath, CCL::IUrl* modelPath);

	// ModeHandler
	bool run () override;

protected:
	CCL::IUrl* modelPath; ///< Additional models, optional.

	bool initModels ();
	void parseFolder (CCL::UrlRef inPath);
	Parser* createParser (CCL::UrlRef inPath);
};

} // namespace XString

#endif // _xstringmodehandler_h
