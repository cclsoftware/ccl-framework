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
// Filename    : xstringextractor.h
// Description : String Extractor Tool
//
//************************************************************************************************

#ifndef _xstring_extractor_h
#define _xstring_extractor_h

#include "ccl/extras/tools/toolhelp.h"

namespace XString {
class ModeHandler;
class Bundle;

//************************************************************************************************
// Extractor
//************************************************************************************************

class Extractor: public CCL::CommandLineTool
{
public:
	Extractor (CCL::CStringRef mode, CCL::CStringRef outputFormat, CCL::IUrl* userModelPath = nullptr);
	bool run (CCL::UrlRef inPath, CCL::UrlRef outPath);

protected:
	CCL::MutableCString scanMode;
	CCL::MutableCString outputFormat;
	CCL::SharedPtr<CCL::IUrl> userModelPath;

	bool parseFolder (Bundle& bundle, CCL::UrlRef inPath) const;
	bool generateOutput (Bundle& bundle, CCL::UrlRef outPath) const;
	ModeHandler* createHandler (Bundle& bundle, CCL::UrlRef inPath) const;
};

} // namespace XString

#endif // _xstring_extractor_h
