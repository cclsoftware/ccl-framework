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
// Filename    : ccl/base/storage/filefilter.h
// Description : File Filter
//
//************************************************************************************************

#ifndef _ccl_filefilter_h
#define _ccl_filefilter_h

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/storableobject.h"
#include "ccl/base/collections/stringlist.h"

namespace CCL {

class StringDictionary;

//////////////////////////////////////////////////////////////////////////////////////////////////

/*
	Example filefilter.xml:

	<FileFilter>
		<FileFilter.Folder name="folderA" key="Application.identity" include="appIdA"/>
		<FileFilter.Folder name="folderB" key="Application.identity" exclude="appIdB"/>
		<FileFilter.File name="fileA" key="Application.identity" exclude="appIdB"/>
	</FileFilter>
*/

//************************************************************************************************
// FileFilter
//************************************************************************************************

class FileFilter: public CCL::StorableObject,
				  public IUrlFilter
{
public:
	DECLARE_CLASS (FileFilter, StorableObject)

	static const String kFileName;
	static const String kAppIdentityKey;
	static StringDictionary& getGlobalConditions ();

	FileFilter ();
	FileFilter (UrlRef folder);

	enum Mode { kExcludeMode, kIncludeMode };
	PROPERTY_VARIABLE (Mode, filterMode, FilterMode)
	void addItem (Mode mode, StringRef name, int type = IUrl::kFile);

	bool loadFromFolder (UrlRef folder);

	bool matchesFolderName (StringRef name) const;
	bool matchesFileName (StringRef name) const;

	// IUrlFilter
	tbool CCL_API matches (UrlRef url) const override;

	// Object
	bool load (const Storage& storage) override;

	CLASS_INTERFACE (IUrlFilter, StorableObject)

private:
	AutoPtr<StringList> excludedSubFolders;
	AutoPtr<StringList> excludedFileNames;
	AutoPtr<StringList> includedSubFolders;
	AutoPtr<StringList> includedFileNames;

	class Item;
	class FolderItem;
	class FileItem;
	class GlobalConditions;

	bool matchesName (const StringList& list, StringRef name) const;
};

} // namespace CCL

#endif // _ccl_filefilter_h
