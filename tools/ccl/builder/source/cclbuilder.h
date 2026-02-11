//************************************************************************************************
//
// CCL Builder
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
// Filename    : cclbuilder.h
// Description : Command line tool for setting up a new CCL-based project
//
//************************************************************************************************

#ifndef _cclbuilder_h
#define _cclbuilder_h

#include "ccl/extras/tools/toolhelp.h"

#include "ccl/base/collections/stringdictionary.h"

//************************************************************************************************
// Builder
//************************************************************************************************

class Builder: public CCL::CommandLineTool
{
public:
	Builder ();

	DECLARE_STRINGID_MEMBER (kAttrTemplates)

	// predefined variables
	static const CCL::String kVendor;
	static const CCL::String kVendorName;
	static const CCL::String kVendorWebsite;
	static const CCL::String kDestination;
	static const CCL::String kRelativeDestination;
	static const CCL::String kDefaultCopyright;
	static const CCL::String kDefaultNamespace;
	static const CCL::String kRepositoryRoot;
	static const CCL::String kFrameworkRoot;
	static const CCL::String kRelativePathToRoot;
	static const CCL::String kNativeRelativePathToRoot;
	static const CCL::String kRelativePathToFramework;
	static const CCL::String kFolderLevelsToRoot;
	static const CCL::String kVendorPackageDomain;
	static const CCL::String kProjectGUID;
	static const CCL::String kPreferredFrameworkVersion;

	PROPERTY_BOOL (interactive, Interactive)

	void addTemplateFolder (CCL::UrlRef templateFolder);
	PROPERTY_STRING (vendorId, VendorID)
	PROPERTY_STRING (templateName, TemplateName)
	PROPERTY_STRING (selectedPlatforms, Platforms)

	PROPERTY_STRING (destPath, DestinationPath)

	void setVariable (CCL::StringRef key, CCL::StringRef value);

	void initialize ();
	void listTemplates ();
	void listVendors ();
	bool run ();

private:
	static const CCL::String kIdentityFileName;

	class Vendor;
	class Variable;
	class Replacement;
	class FileEntry;
	class Template;

	CCL::ObjectList vendors;
	CCL::ObjectList templateDirectories;
	CCL::ObjectList identityDirectories;
	CCL::ObjectList templates;
	CCL::StringDictionary variables;

	CCL::Url userIdentitiesPath;

	CCL::Url repositoryRoot;
	CCL::Url frameworkRoot;
	CCL::Url destFolder;
	Template* selectedTemplate;
	Vendor* selectedVendor;

	void resolvePath (CCL::Url& url, CCL::UrlRef basePath, CCL::StringRef path);
	bool prepare ();
	bool copyFiles ();
	bool replaceStrings ();
	bool replaceStrings (CCL::UrlRef path);
	bool renameFiles ();
	bool renameFiles (CCL::UrlRef path);
	bool checkIdentityFile ();
	void evaluate (CCL::String& string);
	Template* lookupTemplate (CCL::StringRef name) const;
	void scanVendors ();
	void scanTemplates ();
	void prepareVendorID (CCL::String& vendorId);
	Vendor* lookupVendor (CCL::StringRef vendorId) const;
};

#endif // _cclbuilder_h
