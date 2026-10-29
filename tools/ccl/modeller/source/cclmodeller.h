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
// Filename    : cclmodeller.h
// Description : Modeller Tool
//
//************************************************************************************************

#ifndef _cclmodeller_h
#define _cclmodeller_h

#include "ccl/extras/tools/toolhelp.h"

namespace CCL {

//************************************************************************************************
// ModellerTool
//************************************************************************************************

class ModellerTool: public CommandLineTool
{
public:
	DECLARE_STRINGID_MEMBER (kActionList)
	DECLARE_STRINGID_MEMBER (kActionExport)
	DECLARE_STRINGID_MEMBER (kActionScan)
	DECLARE_STRINGID_MEMBER (kActionStaticExport)
	DECLARE_STRINGID_MEMBER (kActionUpdate)

	bool run (StringID action, StringRef arg1, StringRef arg2, StringRef typelib = nullptr);

protected:
	static void collectTypeLibraryNames (Vector<String>& names);
	static void addFileExtension (Url& url, StringRef extension);
	static String toDisplayString (const Vector<Url>& urls);

	void makeAbsolutePaths (Vector<Url>& paths, StringRef pathList);

	bool logTypeLibraries () const;
	bool exportClassModel (StringID typeLibraryName, UrlRef ouputFile) const;
	bool updateClassModel (UrlRef documentedPath, UrlRef prototypePath) const;
	bool checkPathsExist (const Vector<Url>& paths) const;
	bool scanSourceCode (const Vector<Url>& sourcePaths, UrlRef modelPath) const;
	bool staticExportClassModel (const Vector<Url>& sourcePaths, UrlRef outputFile) const;
	bool staticExportTypeLibClassModel (const Vector<Url>& sourcePaths, UrlRef outputFile, StringRef typelibToken) const;
};

} // namespace CCL

#endif // _cclmodeller_h
