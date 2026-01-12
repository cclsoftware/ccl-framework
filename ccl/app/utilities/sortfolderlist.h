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
// Filename    : ccl/app/utilities/sortfolderlist.h
// Description : Sort folder list
//
//************************************************************************************************

#ifndef _ccl_sortfolderlist_h
#define _ccl_sortfolderlist_h

#include "ccl/base/collections/stringlist.h"
#include "ccl/base/collections/objectarray.h"

namespace CCL {

class Attributes;

//************************************************************************************************
// SortFolderList
/** List of sort folder paths. */
//************************************************************************************************

class SortFolderList: public StringList
{
public:
	DECLARE_CLASS (SortFolderList, StringList)

	PROPERTY_STRING (id, ID)

	static String makeLegalFolderPath (StringRef pathString);
	static String getParentFolder (StringRef path);

	void removeFolder (StringRef path);
	void moveSortFolder (StringRef oldPath, StringRef newPath);

protected:
	bool loadFolders (const Storage& storage, StringID id);
	bool saveFolders (const Storage& storage, StringID id) const;
};

//************************************************************************************************
// SortFolderListCollection
/** Collection of multiple SortFolderLists, identified by their IDs. */
//************************************************************************************************

class SortFolderListCollection: public ObjectArray
{
public:
	SortFolderListCollection ();
	void setListClass (MetaClassRef listClass);

	SortFolderList* getSortFolderList (StringRef id, bool create = true);
	void removeAll () override;

	bool store (Attributes& attributes) const;
	bool restore (Attributes& attributes);

private:
	ObjectArray folderLists;
	const MetaClass* listClass;
};

} // namespace CCL

#endif // _ccl_sortfolderlist_h

