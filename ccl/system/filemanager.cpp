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
// Filename    : ccl/system/filemanager.cpp
// Description : File Manager
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/filemanager.h"

#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/public/text/istringdict.h"
#include "ccl/public/system/ifileitem.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IFileManager& CCL_API System::CCL_ISOLATED (GetFileManager) ()
{
	return FileManager::instance ();
}

//************************************************************************************************
// FileManager::UrlItem
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (FileManager::UrlItem, Object)

//************************************************************************************************
// FileManager
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (FileManager, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileManager::FileManager ()
: signalSource (Signals::kFileSystem)
{
	watchedUrls.objectCleanup ();
	usedUrls.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileManager::signalFileCreated (UrlRef url, bool defer)
{
	if(defer)
	{
		AutoPtr<IUrl> urlCopy = NEW Url (url);
		signalSource.deferSignal (NEW Message (Signals::kFileCreated, Variant (urlCopy, true)));
	}
	else
		signalSource.signal (Message (Signals::kFileCreated, ccl_const_cast (&url)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileManager::signalFileRemoved (UrlRef url, bool defer)
{
	if(defer)
	{
		AutoPtr<IUrl> urlCopy = NEW Url (url);
		signalSource.deferSignal (NEW Message (Signals::kFileRemoved, Variant (urlCopy, true)));
	}
	else
		signalSource.signal (Message (Signals::kFileRemoved, ccl_const_cast (&url)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileManager::signalFileChanged (UrlRef url, bool defer)
{
	if(defer)
	{
		AutoPtr<IUrl> urlCopy = NEW Url (url);
		signalSource.deferSignal (NEW Message (Signals::kFileChanged, Variant (urlCopy, true)));
	}
	else
		signalSource.signal (Message (Signals::kFileChanged, ccl_const_cast (&url)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileManager::signalFileMoved (UrlRef oldUrl, UrlRef newUrl, bool defer)
{
	if(defer)
	{
		AutoPtr<IUrl> oldUrlCopy = NEW Url (oldUrl);
		AutoPtr<IUrl> newUrlCopy = NEW Url (newUrl);
		signalSource.deferSignal (NEW Message (Signals::kFileMoved, Variant (oldUrlCopy, true), Variant (newUrlCopy, true)));
	}
	else
		signalSource.signal (Message (Signals::kFileMoved, ccl_const_cast (&oldUrl), ccl_const_cast (&newUrl)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileManager::UrlItem* FileManager::getItemFromList (ObjectList& urlItems, UrlRef url, bool create)
{
	UrlItem* item = urlItems.findIf<UrlItem> ([&] (const UrlItem& i) { return i.url == url; });

	if(item == nullptr && create)
	{
		item = NEW UrlItem (url);
		urlItems.add (item);
	}

	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FileManager::addWatchedLocation (UrlRef url, int flags)
{
	return setWatchedLocation (url, true, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FileManager::removeWatchedLocation (UrlRef url)
{
	return setWatchedLocation (url, false, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult FileManager::setWatchedLocation (UrlRef url, bool state, int flags)
{
	if(System::IsInMainThread () == false)
	{
		ASSERT (0)
		return kResultWrongThread;
	}

	bool watch = (state != 0);
	ObjectList& itemList = watchedUrls;

	UrlItem* item = getItemFromList (itemList, url, watch);
	if(watch)
	{
		ASSERT (item)

		item->useCount++;

		int oldFlags = item->flags;
		item->flags = item->flags | flags;

		if(item->useCount == 1 || item->flags > oldFlags)
			return startWatching (url, item->flags);

		return kResultOk;
	}
	else if(item)
	{	
		item->useCount--;
		if(item->useCount <= 0)
		{
			tresult result = stopWatching (url);
			if(result != kResultOk)
				return result;

			itemList.remove (item);
			item->release ();
		}
		return kResultOk;	
	}

	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FileManager::setFileUsed (UrlRef url, tbool state)
{
	if(url.isEmpty ())
		return kResultInvalidArgument;
	
	if(System::IsInMainThread () == false)
	{
		ASSERT (0)
		return kResultWrongThread;
	}

	bool use = state != 0;
	ObjectList& itemList = usedUrls;

	UrlItem* item = getItemFromList (itemList, url, use);
	if(use)
	{
		ASSERT (item)
		item->useCount++;
		if(item->useCount == 1)
			startUsing (url);
	}
	else if(item)
	{		
		item->useCount--;
		if(item->useCount <= 0)
		{
			stopUsing (url);
			itemList.remove (item);
			item->release ();
		}
		return kResultOk;
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FileManager::setFileWriting (UrlRef url, tbool state)
{
	if(System::IsInMainThread () == false)
	{
		ASSERT (0)
		return kResultWrongThread;
	}

	UrlItem* item = getItemFromList (usedUrls, url, false);
	if(item)
		item->writing (state != 0);
		
	setWriting (url, state != 0);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API FileManager::triggerFileUpdate (UrlRef url)
{
	return AsyncOperation::createCompleted ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult FileManager::startWatching (UrlRef url, int flags)
{
	CCL_NOT_IMPL ("FileManager::startWatching")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult FileManager::stopWatching (UrlRef url)
{
	CCL_NOT_IMPL ("FileManager::stopWatching")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult FileManager::startUsing (UrlRef url)
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult FileManager::stopUsing (UrlRef url)
{	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileManager::setWriting (UrlRef url, bool state)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileManager::buildDisplayPath (Url& displayPath, UrlRef url, IFileItemProvider& provider) const
{
	Url parent (url);
	if(parent.ascend ())
		if(buildDisplayPath (displayPath, parent, provider) == false)
			return false;

	AutoPtr<IFileDescriptor> descriptor = provider.openFileItem (url);
	if(descriptor)
	{
		String fileName;
		if(descriptor->getFileName (fileName))
		{
			displayPath.descend (fileName, url.isFolder () ? IUrl::kFolder : IUrl::kFile);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileManager::getFileDisplayString (String& string, UrlRef url, int type) const
{
	if(type == Url::kStringNativePath)
	{
		NativePath path (url);
		string = path;
		return true;
	}
	else if(type == Url::kStringDisplayPath)
	{
		if(url.isNativePath () == false)
		{
			UnknownPtr<IFileItemProvider> provider (&System::GetFileSystem ());
			ASSERT (provider)
			Url displayPath;
			if(provider && buildDisplayPath (displayPath, url, *provider))
			{
				string = displayPath.getPath ();
				return true;
			}
		}
		return getFileDisplayString (string, url, Url::kStringNativePath);
	}
	else if(type == Url::kStringDisplayName)
	{
		string = UrlUtils::getNameFromParameters (url, false);
		if(string.isEmpty ())
			url.getName (string, false);
		
		return true;
	}
	return false;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API FileManager::getFileLocationType (UrlRef _url) const
{
	Url url (_url);
	if(url.isFile ())
		url.ascend ();

	Url documentLocation;
	System::GetSystem ().getLocation (documentLocation, System::kUserDocumentFolder);
	if(documentLocation.isEqualUrl (url, false) || documentLocation.contains (url))
		return FileLocationType::kDocuments;
	
	return FileLocationType::kOther;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileManager::terminate ()
{}
