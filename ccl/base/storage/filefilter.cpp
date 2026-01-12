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
// Filename    : ccl/base/storage/filefilter.cpp
// Description : File Filter
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/base/storage/filefilter.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/file.h"

#include "ccl/base/singleton.h"
#include "ccl/base/collections/stringdictionary.h"

namespace CCL {

//************************************************************************************************
// FileFilter::GlobalConditions
//************************************************************************************************

class FileFilter::GlobalConditions: public StaticSingleton<StringDictionary>
{};

//************************************************************************************************
// FileFilter::Item
//************************************************************************************************

class FileFilter::Item: public Object
{
public:
	DECLARE_CLASS (Item, Object)

	PROPERTY_STRING (name, Name)
	PROPERTY_STRING (key, Key)
	PROPERTY_STRING (includeValue, IncludeValue)
	PROPERTY_STRING (excludeValue, ExcludeValue)

	bool matchesValue (StringRef value);

	// Object
	bool load (const Storage& storage) override;
};

//************************************************************************************************
// FileFilter::FolderItem
//************************************************************************************************

class FileFilter::FolderItem: public FileFilter::Item
{
public:
	DECLARE_CLASS (FolderItem, Item)
};

//************************************************************************************************
// FileFilter::FileItem
//************************************************************************************************

class FileFilter::FileItem: public FileFilter::Item
{
public:
	DECLARE_CLASS (FileItem, Item)
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// FileFilter::FileItem
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (FileFilter::FileItem, FileFilter::Item, "FileFilter.File")

//************************************************************************************************
// FileFilter::FolderItem
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (FileFilter::FolderItem, FileFilter::Item, "FileFilter.Folder")

//************************************************************************************************
// FileFilter::Item
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FileFilter::Item, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileFilter::Item::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	name = a.getString ("name");
	key = a.getString ("key");
	includeValue = a.getString ("include");
	excludeValue = a.getString ("exclude");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool FileFilter::Item::matchesValue (StringRef value)
{
	if(value == excludeValue)
		return false;

	if(includeValue.isEmpty ())
		return true;

	return value == includeValue;
}

//************************************************************************************************
// FileFilter
//************************************************************************************************

const String FileFilter::kFileName ("filefilter.xml");
const String FileFilter::kAppIdentityKey ("Application.identity");

//////////////////////////////////////////////////////////////////////////////////////////////////

StringDictionary& FileFilter::getGlobalConditions ()
{
	return GlobalConditions::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (FileFilter, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileFilter::FileFilter ()
: filterMode (kExcludeMode)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileFilter::FileFilter (UrlRef folder)
: filterMode (kExcludeMode)
{ 
	loadFromFolder (folder); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileFilter::loadFromFolder (UrlRef folder)
{
	Url filterFile (folder);
	filterFile.descend (kFileName);
	return loadFromFile (*this, filterFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileFilter::matchesName (const StringList& list, StringRef name) const
{
	#if 1 // allow wildcards in filter list
	// TODO: change members to lists of SearchDescriptions instead of StringList?
	static const String kWildcard (CCLSTR ("*"));
	for(int i = 0, count = list.count (); i < count; i++)
	{
		String string (list[i]);
		if(string.contains (kWildcard))
		{
			AutoPtr<SearchDescription> description = SearchDescription::create (Url (), string);
			if(description->matchesName (name))
				return true;
		}
		else if(string == name)
			return true;
	}
	return false;
	#else
	return list.contains (name);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileFilter::matchesFolderName (StringRef name) const
{
	if(filterMode == kExcludeMode)
		return !excludedSubFolders || !matchesName (*excludedSubFolders, name);
	else
		return includedSubFolders && matchesName (*includedSubFolders, name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileFilter::matchesFileName (StringRef name) const
{
	if(filterMode == kExcludeMode)
		return !excludedFileNames || !matchesName (*excludedFileNames, name);
	else
		return includedFileNames && matchesName (*includedFileNames, name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileFilter::matches (UrlRef url) const
{
	String name;
	url.getName (name);
	if(url.isFolder ())
		return matchesFolderName (name);
	else
		return matchesFileName (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileFilter::addItem (Mode mode, StringRef name, int type)
{
	if(mode == kExcludeMode)
	{
		if(type == IUrl::kFolder)
		{
			if(!excludedSubFolders)
				excludedSubFolders = NEW StringList;
			excludedSubFolders->add (name);
		}
		else
		{
			if(!excludedFileNames)
				excludedFileNames = NEW StringList;
			excludedFileNames->add (name);
		}
	}
	else
	{
		if(type == IUrl::kFolder)
		{
			if(!includedSubFolders)
				includedSubFolders = NEW StringList;
			includedSubFolders->add (name);
		}
		else
		{
			if(!includedFileNames)
				includedFileNames = NEW StringList;
			includedFileNames->add (name);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileFilter::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	filterMode = a.getCString ("filtermode") == "include" ? kIncludeMode : kExcludeMode;

	if(filterMode == kExcludeMode)
	{
		excludedSubFolders = NEW StringList;
		excludedFileNames = NEW StringList;	
	}
	else
	{
		includedSubFolders = NEW StringList;
		includedFileNames = NEW StringList;
	}

	const StringDictionary& filterConditions = getGlobalConditions ();

	AutoPtr<Item> item;
	while((item = a.unqueueObject<Item> (nullptr)))
	{
		StringRef value = filterConditions.lookupValue (item->getKey ());
		CCL_PRINTF ("FileFilter:  %s=%s\n", MutableCString (item->getKey ()).str (), MutableCString (value).str ())
		if(!item->matchesValue (value))
		{
			if(filterMode == kExcludeMode)
			{
				if(ccl_cast<FolderItem> (item))
					excludedSubFolders->add (item->getName ());
				else
					excludedFileNames->add (item->getName ());
			}
		}
		else
		{
			if(filterMode == kIncludeMode)
			{
				if(ccl_cast<FolderItem> (item))
					includedSubFolders->add (item->getName ());
				else
					includedFileNames->add (item->getName ());
			}
		}
	}
	return true;
}
