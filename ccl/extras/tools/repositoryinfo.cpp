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
// Filename    : ccl/extras/tools/repositoryinfo.cpp
// Description : Repository Info
//
//************************************************************************************************

#include "ccl/extras/tools/repositoryinfo.h"

#include "ccl/base/storage/jsonarchive.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/file.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// RepositoryInfo
//************************************************************************************************

const String RepositoryInfo::kFileName = "repo.json";

DEFINE_STRINGID_MEMBER_ (RepositoryInfo, kSubmoduleDirectories, "submodules")
DEFINE_STRINGID_MEMBER_ (RepositoryInfo, kTemplateDirectories, "templates")
DEFINE_STRINGID_MEMBER_ (RepositoryInfo, kIdentityDirectories, "identities")
DEFINE_STRINGID_MEMBER_ (RepositoryInfo, kClassModelDirectories, "classmodels")
DEFINE_STRINGID_MEMBER_ (RepositoryInfo, kDocumentationDirectories, "documentation")
DEFINE_STRINGID_MEMBER_ (RepositoryInfo, kSkinDirectories, "skins")
DEFINE_STRINGID_MEMBER_ (RepositoryInfo, kSigningDirectories, "signing")
DEFINE_STRINGID_MEMBER_ (RepositoryInfo, kTranslationsDirectories, "translations")

//////////////////////////////////////////////////////////////////////////////////////////////////

RepositoryInfo::RepositoryInfo ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

RepositoryInfo::~RepositoryInfo ()
{
	removeEntries ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RepositoryInfo::load (UrlRef startFolder, bool reload)
{
	if(reload)
		removeEntries ();
	
	Url folder (startFolder);
	Url infoFilePath;
	while(!folder.isRootPath ())
	{
		infoFilePath = folder;
		infoFilePath.descend (kFileName, IUrl::kFile);
		if(File (infoFilePath).exists ())
		{
			AutoPtr<IStream> fileStream = System::GetFileSystem ().openStream (infoFilePath, IStream::kOpenMode);
			if(!fileStream.isValid ())
				continue;

			Attributes a;
			if(!JsonArchive (*fileStream).loadAttributes (nullptr, a))
				return false;

			rootDirectory = folder;

			for(StringID category : { kSubmoduleDirectories, kTemplateDirectories, kIdentityDirectories,
				kClassModelDirectories, kDocumentationDirectories, kSkinDirectories, kSigningDirectories,
				kTranslationsDirectories })
			{
				Entry entry;
				entry.category = category;

				Variant value;
				while(a.unqueueAttribute (category, value))
				{
					Url* path = NEW Url (folder);
					path->descend (value.asString (), IUrl::kFolder);
					entry.paths.add (path);
				}

				entries.add (entry);
			}

			return true;
		}
		folder.ascend ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef RepositoryInfo::getRootDirectory () const
{
	return rootDirectory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RepositoryInfo::getPaths (Container& paths, StringID category) const
{
	bool success = false;
	for(const Entry& entry : entries)
		if(entry.category == category)
		{
			success = true;
			paths.add (entry.paths, Container::kShare);
		}

	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RepositoryInfo::findPath (Url& path, StringID category, StringRef innerPath) const
{
	ObjectArray paths;
	paths.objectCleanup ();
	if(findAllPaths (paths, category, innerPath))
	{
		if(paths.count () > 0)
		{
			Url* firstPath = ccl_cast<Url> (paths[0]);
			if(firstPath)
			{
				path = *firstPath;
				return true;
			}
		}
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RepositoryInfo::findAllPaths (Container& paths, StringID category, StringRef innerPath) const
{
	bool success = false;
	ObjectArray categoryPaths;
	categoryPaths.objectCleanup ();
	if(getPaths (categoryPaths, category))
	{
		ForEach (categoryPaths, Url, folder)
			Url folderPath (*folder);
			folderPath.descend (innerPath, IUrl::kFolder);
			if(File (folderPath).exists ())
			{
				paths.add (NEW Url (*folder));
				success = true;
			}
			else
			{
				folderPath.descend ("", IUrl::kFile);
				if(File (folderPath).exists ())
				{
					paths.add (NEW Url (*folder));
					success = true;
				}
			}
		EndFor
	}
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RepositoryInfo::removeEntries ()
{
	for(const Entry& entry : entries)
	{
		for(Object* path : entry.paths)
			path->release ();
	}
	entries.removeAll ();
}
