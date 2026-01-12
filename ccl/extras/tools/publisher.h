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
// Filename    : ccl/extras/tools/publisher.h
// Description : Publisher Tool
//
//************************************************************************************************

#ifndef _publisher_h
#define _publisher_h

#include "ccl/extras/tools/toolhelp.h"
#include "ccl/extras/tools/repositoryinfo.h"

#include "ccl/base/collections/stringlist.h"

namespace CCL {

//************************************************************************************************
// Publisher
//************************************************************************************************

class Publisher: public BatchProcessor
{
public:
	Publisher ();

	PROPERTY_OBJECT (Url, outDir, OutputDir)
	PROPERTY_OBJECT (Url, outBaseDir, OutputBaseDir)

	enum ErrorMode { kFail, kDontFail };
	PROPERTY_VARIABLE (ErrorMode, errorMode, ErrorMode)
	PROPERTY_STRING (returnValue, ReturnValue)
	
	void addInputDirectory (UrlRef path);
	bool getInputDirectory (Url& path, int index = 0) const;

	void makeInputPath (Url& path, StringRef fileName, int type = Url::kFile, int index = 0);
	void makeOutputPath (Url& path, StringRef fileName, int type = Url::kFile);

	bool addDefinition (StringRef key, StringRef value, bool list = false);
	
	void setDependencyFilePath (UrlRef path);

	virtual bool perform (StringID command, StringRef name); 

	// BatchProcessor
	bool runLine (StringRef line) override;

protected:
	static const String kListDelimiter;
	static const String kHash;
	
	struct Definition
	{
		String key, value;

		Definition (StringRef key = nullptr, StringRef value = nullptr)
		: key (key), value (value)
		{}

		bool operator == (const Definition& d) const
		{ return key == d.key; }
	};

	struct Macro: public Object
	{
		String name;
		StringList lines;
	};

	ObjectArray inputDirectories;
	PatternFilter filter;
	Vector<Definition> definitions;
	Vector<Definition> listDefinitions;
	ObjectList macros;
	Macro* currentMacro;
	AutoPtr<DependencyFile> dependencyFile;
	RepositoryInfo repositoryInfo;
	Url repositoryWorkDir;

	void updateRepositoryInfo (UrlRef workDir);

	bool dontFail () const { return errorMode == kDontFail; }
	
	String preprocess (StringRef string) const;
	const Macro* findMacro (StringRef name) const;
	bool runMacro (const Macro& macro);
};

} // namespace CCL

#endif // _publisher_h
