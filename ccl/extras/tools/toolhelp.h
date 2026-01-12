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
// Filename    : ccl/extras/tools/toolhelp.h
// Description : Tool Helper
//
//************************************************************************************************

#ifndef _toolhelp_h
#define _toolhelp_h

#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/system/iconsole.h"
#include "ccl/public/system/ipackagefile.h"

namespace CCL {

//************************************************************************************************
// PackFolderOptions
//************************************************************************************************

struct PackFolderOptions
{
	bool compressed;
	bool encrypted;
	bool useZip;
	bool xteaEncrypted;
	bool aesEncrypted;
	int formatVersion;
	int reservedBlockSize;
	MutableCString keyString;
	bool keepHidden;

	PackFolderOptions ()
	: compressed (true),
	  encrypted (false),
	  useZip (false),
	  xteaEncrypted (false),
	  aesEncrypted (false),
	  formatVersion (0),
	  reservedBlockSize (0),
	  keepHidden (false)
	{}

	PackFolderOptions& fromString (StringID string);
};

//************************************************************************************************
// ToolHelper
//************************************************************************************************

class ToolHelper: public Object
{
public:
	DECLARE_CLASS (ToolHelper, Object)
	DECLARE_METHOD_NAMES (ToolHelper)

	static bool copyFile (UrlRef dstPath, UrlRef srcPath);
	static bool copyFolder (UrlRef dstPath, UrlRef srcPath, const IUrlFilter& filter, bool recursive);

	static MutableCString generateKeyString ();

	static bool packageFolder (UrlRef dstPath, UrlRef srcPath, const IUrlFilter& filter, bool recursive, 
							   const PackFolderOptions& options = PackFolderOptions (),
							   IProgressNotify* progress = nullptr, UrlRef dependencyFilePath = Url ());

	static bool embeddDataInPackageFile (UrlRef dstPath, UrlRef srcPath, StringRef comment = nullptr);

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// CommandLineTool
//************************************************************************************************

class CommandLineTool
{
public:
	CommandLineTool ();
	~CommandLineTool ();

	void configureLogging (int severity = kSeverityInfo, int format = Alert::Event::kWithTime | Alert::Event::kWithSeverity);

	void makeAbsolute (Url& path);
	void makeAbsolute (Url& path, StringRef fileName, int type = Url::kFile, UrlRef baseDir = Url ());

protected:
	Url workDir;
	System::IConsole& console;
};

//************************************************************************************************
// BatchProcessor
//************************************************************************************************

class BatchProcessor: public CommandLineTool
{
public:
	bool run (StringRef batchFileName, bool changeWorkDir = true);
	bool run (UrlRef batchFilePath, bool changeWorkDir = true);

	virtual bool runLine (StringRef line) = 0;

protected:
	bool isCommentLine (StringRef line) const;
};

//************************************************************************************************
// PackageFilter
//************************************************************************************************

class PackageFilter: public UrlFilter,
					 public IPackageItemFilter
{
public:
	PackageFilter ();

	PROPERTY_BOOL (compressed, Compressed)
	PROPERTY_BOOL (encrypted, Encrypted)
	PROPERTY_BOOL (keepHidden, KeepHidden)
	PROPERTY_BOOL (externalKeyEnabled, ExternalKeyEnabled)

	void applyOptions (const PackFolderOptions& options);
	void resetOptions ();

	// IPackageItemFilter
	int CCL_API getPackageItemAttributes (UrlRef path) const override;

	CLASS_INTERFACE (IPackageItemFilter, UrlFilter)

private:
	bool isMetaInfoFile (UrlRef path) const;
};

//************************************************************************************************
// PatternFilter
//************************************************************************************************

class PatternFilter: public PackageFilter
{
public:
	PatternFilter ();

	PROPERTY_BOOL (caseSensitive, CaseSensitive)
	PROPERTY_BOOL (positive, Positive)

	bool loadFromFile (UrlRef path);

	void add (StringRef string);
	void remove (StringRef string);
	void removeAll ();

	// PackageFilter
	tbool CCL_API matches (UrlRef url) const override;

protected:
	ObjectList strings;
};

//************************************************************************************************
// ExtensionFilter
//************************************************************************************************

class ExtensionFilter: public PackageFilter
{
public:
	ExtensionFilter (StringRef extensions);

	// PackageFilter
	tbool CCL_API matches (UrlRef url) const override;

protected:
	String extensions;
};

//************************************************************************************************
// DependencyFile
//************************************************************************************************

class DependencyFile: public Unknown
{
public:
	DependencyFile (UrlRef path);
	~DependencyFile ();

	void setOutputFile (UrlRef outputFile);
	void addDependency (UrlRef inputFile);

private:
	struct Target
	{
		Url path;
		Vector<Url> dependencies;

		Target (UrlRef path = Url ())
		: path (path)
		{}

		bool operator == (const Target& other) const
		{
			return path == other.path;
		}
	};
	
	Url path;
	Vector<Target> targets;

	static String getPathString (UrlRef path);
};

} // namespace CCL

#endif // _toolhelp_h
