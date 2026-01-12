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
// Filename    : ccl/extras/modeling/modeltool.h
// Description : Class Model Tool
//
//************************************************************************************************

#ifndef _modeltool_h
#define _modeltool_h

#include "ccl/extras/tools/toolhelp.h"

namespace CCL {

class ArgumentParser;

//************************************************************************************************
// ModelTool
//************************************************************************************************

class ModelTool: public CommandLineTool
{
public:
	DECLARE_STRINGID_MEMBER (kActionList)
	DECLARE_STRINGID_MEMBER (kActionExport)
	DECLARE_STRINGID_MEMBER (kActionScan)
	DECLARE_STRINGID_MEMBER (kActionUpdate)
	DECLARE_STRINGID_MEMBER (kActionDoc)

	static void setupArgs (ArgumentParser& parser);
	static bool runFromArgs (const ArgumentParser& argParser);

	bool run (StringID action, StringRef file1, StringRef file2);

private:
	static void collectTypeLibraryNames (Vector<String>& names);
	static void addFileExtension (Url& url, StringRef extension);

	bool logTypeLibraries () const;
	bool exportClassModel (StringID typeLibraryName, UrlRef ouputFile) const;
	bool updateClassModel (UrlRef documentedPath, UrlRef prototypePath) const;
	bool scanSourceCode (UrlRef sourceFolder, UrlRef modelPath) const;
};

} // namespace CCL

#endif // _modeltool_h
