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
// Filename    : ccl/app/utilities/sortfolderlist.cpp
// Description : Sort folder list
//
//************************************************************************************************

#include "ccl/app/utilities/sortfolderlist.h"

#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/url.h"

using namespace CCL;

//************************************************************************************************
// SortFolderListCollection
//************************************************************************************************

SortFolderListCollection::SortFolderListCollection ()
: listClass (nullptr)
{
	folderLists.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SortFolderListCollection::setListClass (MetaClassRef listClass)
{
	this->listClass = &listClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SortFolderListCollection::removeAll ()
{
	folderLists.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SortFolderList* SortFolderListCollection::getSortFolderList (StringRef id, bool create)
{
	SortFolderList* list = folderLists.findIf<SortFolderList> ([&] (const SortFolderList& list) { return list.getID () == id; });
	if(!list && create)
	{
		list = listClass ? ccl_cast<SortFolderList> (listClass->createObject ()) : NEW SortFolderList;
		list->setID (id);
		folderLists.add (list);
	}
	return list;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SortFolderListCollection::store (Attributes& attributes) const
{
	return attributes.queue (nullptr, folderLists, Attributes::kShare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SortFolderListCollection::restore (Attributes& attributes)
{
	attributes.unqueue (folderLists, nullptr, ccl_typeid<SortFolderList> ());
	return true;
}

//************************************************************************************************
// SortFolderList
//************************************************************************************************

DEFINE_CLASS (SortFolderList, StringList)

//////////////////////////////////////////////////////////////////////////////////////////////////

String SortFolderList::makeLegalFolderPath (StringRef pathString)
{
	String path (UrlUtils::stripSlashes (pathString));

	// replace double slashes by single slashes
	static const String doublePathChar (String () << Url::strPathChar << Url::strPathChar);

	int index = -1;
	while((index = path.index (doublePathChar)) >= 0)
		path.remove (index, 1);

	#if DEBUG_LOG
	if(path != pathString)
		CCL_PRINTF ("makeLegalFolderName: %s  ->  %s\n", MutableCString (pathString).str (), MutableCString (path).str ())
	#endif
	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String SortFolderList::getParentFolder (StringRef path)
{
	String parentPath;
	int index = path.lastIndex (Url::strPathChar);
	if(index > 0)
		parentPath = path.subString (0, index);
	return makeLegalFolderPath (parentPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SortFolderList::removeFolder (StringRef path)
{
	// remove the sort folder and it's subFolders
	remove (path);

	String subFolderPrefix (path);
	subFolderPrefix << Url::strPathChar;

	ForEach (*this, Boxed::String, folder)
		if(folder->startsWith (subFolderPrefix))
			remove (*folder);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SortFolderList::moveSortFolder (StringRef oldPath, StringRef newPath)
{
	remove (oldPath);
	addOnce (newPath);

	String subFolderPrefix (oldPath);
	subFolderPrefix << Url::strPathChar;

	ForEach (*this, Boxed::String, folder)
		if(folder->startsWith (subFolderPrefix))
		{
			String newFolder (newPath);
			newFolder << folder->subString (oldPath.length ());

			remove (*folder);
			addOnce (newFolder);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SortFolderList::loadFolders (const Storage& storage, StringID id)
{
	setID (storage.getAttributes ().getString (id));

	StringList::load (storage);

	// repair illegal paths
	for(auto string : *this)
		*string = makeLegalFolderPath (*string);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SortFolderList::saveFolders (const Storage& storage, StringID id) const
{
	storage.getAttributes ().set (id, getID ());
	return StringList::save (storage);
}
