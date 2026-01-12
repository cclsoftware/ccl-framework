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
// Filename    : ccl/platform/win/filemanager.win.cpp
// Description : Windows file manager
//
//************************************************************************************************

#include "filemanager.win.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/base/buffer.h"
#include "ccl/public/system/userthread.h"
#include "ccl/public/systemservices.h"

#include "ccl/platform/win/cclwindows.h"
#include "ccl/platform/win/system/cclcom.h"

namespace CCL {

//************************************************************************************************
// WindowsFileSystemMonitorThread
//************************************************************************************************

class WindowsFileManager::MonitorThread: public Threading::UserThread
{
public:
	MonitorThread ();
	~MonitorThread ();

	bool startWatching (UrlRef url, int flags);
	void stopWatching (UrlRef url);

	void cancel ();

protected:
	class MonitoredDirectory: public UrlItem
	{
		DECLARE_CLASS_ABSTRACT (MonitoredDirectory, UrlItem)

		MonitoredDirectory (UrlRef url);

		Buffer changeBuffer;
		HANDLE handle = nullptr;
		OVERLAPPED overlapped = {};

		static const int kChangeBufferSize = 1024;
	};

	ObjectArray items;
	bool scanning;
	HANDLE exitHandle;
	HANDLE itemsChangedHandle;

	void waitForItemsChanged ();
	void scanFileChanges ();

	// UserThread
	int threadEntry () override;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// WindowsFileManager
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (WindowsFileManager, FileManager)
DEFINE_EXTERNAL_SINGLETON (FileManager, WindowsFileManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsFileManager::WindowsFileManager ()
: monitorThread (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsFileManager::~WindowsFileManager ()
{
	ASSERT (monitorThread == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsFileManager::terminate ()
{	
	if(monitorThread)
	{
		monitorThread->cancel ();
		monitorThread->stopThread (500);
		delete monitorThread;
		monitorThread = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsFileManager::getFileDisplayString (String& string, UrlRef url, int type) const
{
	if(type == Url::kStringDisplayPath && url.isNativePath ())
	{
		UrlDisplayString nativePathString (url, Url::kStringNativePath);

		if(nativePathString.contains (CCLSTR ("~")) && url.isRelative () == false) // beautify only if string contains "~" (icloud)
		{	
			// from: C:\Users\Public\iCloudDrive\iCloud~com~vendor~AppName
			//   to: C:\Users\Public\iCloudDrive\AppName
	
			Vector<String> parts;
			Url url2 (url);
			while(true)
			{
				bool success = false;
				Win32::ComPtr<IShellItem> item;

				::SHCreateItemFromParsingName (StringChars (UrlDisplayString (url2, Url::kStringNativePath)), nullptr, IID_IShellItem, item);
				if(item)
				{
					LPWSTR name = nullptr;
					item->GetDisplayName (SIGDN_NORMALDISPLAY, &name);
					if(name)
					{
						parts.add (name);
						::CoTaskMemFree (name);
						success = true;
					}
				}
		
				if(success == false || url2.ascend () == false)
					break;
			
				if(url2.isRootPath ())
				{
					for(int i = parts.count () - 1; i >= 0; i--)
						url2.descend (parts[i], Url::kFolder);			
			
					SuperClass::getFileDisplayString (string, url2, Url::kStringNativePath);
					return true;
				}
			}	
		}
	}
	else if(type == Url::kStringDisplayName && url.isNativePath ())
	{		
		if(url.isFolder ())
		{
			Win32::ComPtr<IShellItem> item;
			::SHCreateItemFromParsingName (StringChars (UrlDisplayString (url, Url::kStringNativePath)), nullptr, IID_IShellItem, item);
			if(item)
			{
				LPWSTR name = nullptr;
				item->GetDisplayName (SIGDN_NORMALDISPLAY, &name);
				if(name)
				{
					string = name;
					::CoTaskMemFree (name);
					return true;
				}
			}	
		}		
	}

	return SuperClass::getFileDisplayString (string, url, type);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API WindowsFileManager::getFileLocationType (UrlRef url) const
{
	UrlDisplayString nativePathString (url, Url::kStringNativePath);
	if(nativePathString.contains ("iCloud~com~"))
		return FileLocationType::kICloud;
		
	auto isInUserSubfolder = [&] (StringRef folderName)
	{
		if(nativePathString.contains (folderName, false))
		{
			uchar p[Url::kMaxLength] = {0};
			if(::SHGetSpecialFolderPath (nullptr, p, CSIDL_PROFILE, FALSE))
			{
				Url driveFolder;
				driveFolder.fromNativePath (p, Url::kFolder);
				driveFolder.descend (folderName, Url::kFolder);				
				UrlDisplayString nativeFolderString (driveFolder, Url::kStringNativePath);

				if(nativePathString.startsWith (nativeFolderString, false))
					return true;
			}
		}
		return false;
	};
	
	if(isInUserSubfolder (CCLSTR ("iCloudDrive")))
		return FileLocationType::kICloud;

	else if(isInUserSubfolder (CCLSTR ("OneDrive")))
		return FileLocationType::kOneDrive;

	else if(isInUserSubfolder (CCLSTR ("Dropbox")))
		return FileLocationType::kDropBox;

	else
	{
		// Google Drive is mounted as volume 
		// this here should work unless the user renames the drive
		Url root (url);
		while(root.isRootPath () == false)
			if(root.ascend () == false)
				break;
		if(root.isRootPath ())
		{
			String driveName;
			getFileDisplayString (driveName, root, Url::kStringDisplayName);
			if(StringUtils::strip (driveName, Unicode::isAlpha).startsWith (CCLSTR ("GoogleDrive"), false))
				return FileLocationType::kGoogleDrive;
		}
	}

	return SuperClass::getFileLocationType (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WindowsFileManager::startWatching (UrlRef url, int flags)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	if(monitorThread == nullptr)
	{
		monitorThread = NEW MonitorThread;
		monitorThread->startThread (Threading::kPriorityBelowNormal);
	}

	if(monitorThread && monitorThread->startWatching (url, flags))
		return kResultOk;

	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WindowsFileManager::stopWatching (UrlRef url)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	if(monitorThread)
		monitorThread->stopWatching (url);

	return kResultOk;
}

//************************************************************************************************
// WindowsFileManager::MonitorThread
//************************************************************************************************

WindowsFileManager::MonitorThread::MonitorThread ()
: scanning (false)
{
	exitHandle = ::CreateEvent (nullptr, FALSE, FALSE, nullptr);
	itemsChangedHandle = ::CreateEvent (nullptr, TRUE, FALSE, nullptr);
	ASSERT (exitHandle != INVALID_HANDLE_VALUE && itemsChangedHandle != INVALID_HANDLE_VALUE)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsFileManager::MonitorThread::~MonitorThread ()
{
	ASSERT (items.isEmpty ())
	items.objectCleanup (true);

	if(exitHandle != INVALID_HANDLE_VALUE)
		::CloseHandle (exitHandle);
	if(itemsChangedHandle != INVALID_HANDLE_VALUE)
		::CloseHandle (itemsChangedHandle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsFileManager::MonitorThread::startWatching (UrlRef url, int flags)
{
	if(itemsChangedHandle == INVALID_HANDLE_VALUE)
		return false;

	ASSERT (items.count () < MAXIMUM_WAIT_OBJECTS - 2)
	if(items.count () >= MAXIMUM_WAIT_OBJECTS - 2)
		return false;

	ASSERT (((flags & IFileManager::kDeep) == 0) || url.isFolder ())

	while(scanning)
	{
		::SetEvent (itemsChangedHandle);
		System::ThreadSleep (10);
	}

	// Create a handle of the monitored directory
	Url directory (url);
	if(directory.isFile ())
		directory.ascend ();
	NativePath directoryPath (directory);
	HANDLE handle = ::CreateFile (directoryPath, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

	if(handle != INVALID_HANDLE_VALUE)
	{
		MonitoredDirectory* item = NEW MonitoredDirectory (url);
		item->flags = flags;
		item->handle = handle;
		items.add (item);
	}

	::SetEvent (itemsChangedHandle);

	return handle != INVALID_HANDLE_VALUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsFileManager::MonitorThread::stopWatching (UrlRef url)
{
	if(itemsChangedHandle == INVALID_HANDLE_VALUE)
		return;

	while(scanning)
	{
		::SetEvent (itemsChangedHandle);
		System::ThreadSleep (10);
	}

	for(MonitoredDirectory* item : iterate_as<MonitoredDirectory> (items))
	{
		if(item->url == url)
		{
			::CloseHandle (item->handle);
			items.remove (item);
			item->release ();
			break;
		}
	}

	::SetEvent (itemsChangedHandle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsFileManager::MonitorThread::cancel ()
{
	requestTerminate ();
	if(exitHandle != INVALID_HANDLE_VALUE)
		::SetEvent (exitHandle);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WindowsFileManager::MonitorThread::threadEntry ()
{
	while(true)
	{
		waitForItemsChanged ();
		
		if(shouldTerminate ())
			break;
		
		scanFileChanges ();

		if(shouldTerminate ())
			break;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsFileManager::MonitorThread::waitForItemsChanged ()
{
	HANDLE objects[2] = {exitHandle, itemsChangedHandle};
	DWORD waitStatus = ::WaitForMultipleObjects (2, objects, false, INFINITE);
	
	if(waitStatus == WAIT_OBJECT_0 + 1)
		::ResetEvent (itemsChangedHandle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsFileManager::MonitorThread::scanFileChanges ()
{
	ScopedVar<bool> scope (scanning, true);

	const DWORD flags = FILE_NOTIFY_CHANGE_FILE_NAME |
		FILE_NOTIFY_CHANGE_DIR_NAME |
		FILE_NOTIFY_CHANGE_ATTRIBUTES |
		FILE_NOTIFY_CHANGE_SIZE |
		FILE_NOTIFY_CHANGE_LAST_WRITE |
		FILE_NOTIFY_CHANGE_LAST_ACCESS |
		FILE_NOTIFY_CHANGE_CREATION |
		FILE_NOTIFY_CHANGE_SECURITY;


	int numObjects = items.count () + 2;
	FixedSizeVector<HANDLE, MAXIMUM_WAIT_OBJECTS> objects;

	objects[0] = exitHandle;
	objects[1] = itemsChangedHandle;
	int i = 0;
	for(MonitoredDirectory* item : iterate_as<MonitoredDirectory> (items))
	{
		item->overlapped.hEvent = ::CreateEvent (nullptr, TRUE, FALSE, nullptr);

		ASSERT (item->overlapped.hEvent != INVALID_HANDLE_VALUE)
		if(item->overlapped.hEvent == INVALID_HANDLE_VALUE)
		{
			cancel ();
			break;
		}

		// Read directory changes asynchronously
		if(!::ReadDirectoryChangesW (item->handle, item->changeBuffer, item->changeBuffer.getSize (),
			item->flags & IFileManager::kDeep, flags, nullptr, &item->overlapped, nullptr))
		{
			cancel ();
			break;
		}

		objects[i + 2] = item->overlapped.hEvent;
		++i;
	}

	while(true)
	{
		DWORD waitStatus = ::WaitForMultipleObjects (numObjects, objects, FALSE, INFINITE);

		if(shouldTerminate ())
			break;

		if(waitStatus == WAIT_OBJECT_0)
		{
			// exitSignal
			break;
		}
		else if(waitStatus == WAIT_OBJECT_0 + 1)
		{
			// monitored directories changed
			::ResetEvent (itemsChangedHandle);
			break;
		}
		else if(waitStatus >= WAIT_OBJECT_0 + 2 && waitStatus < WAIT_OBJECT_0 + numObjects)
		{
			int itemIndex = waitStatus - WAIT_OBJECT_0 - 2;
			MonitoredDirectory* item = ccl_cast<MonitoredDirectory> (items.at (itemIndex));

			DWORD bytesReturned;
			if(!::GetOverlappedResult (item->handle, &item->overlapped, &bytesReturned, FALSE))
				continue;

			String expectedFileName;
			if(item->url.isFile ())
				item->url.getName (expectedFileName);
			
			Url oldUrl;
			FILE_NOTIFY_INFORMATION* info = nullptr;
			for(int offset = 0; ; offset += info->NextEntryOffset)
			{
				info = reinterpret_cast<FILE_NOTIFY_INFORMATION*> (&item->changeBuffer[offset]);
				
				String fileName;
				fileName.assign (info->FileName, info->FileNameLength / sizeof(info->FileName[0]));
				fileName.replace (CCLSTR ("\\"), Url::strPathChar); // make sure to follow Url rules
					
				Url fileUrl (item->url);
				if(fileUrl.isFolder ())
					fileUrl.descend (fileName);
			
				if(expectedFileName.isEmpty () || expectedFileName == fileName)
					switch(info->Action)
					{
					case FILE_ACTION_ADDED:
						FileManager::instance ().signalFileCreated (fileUrl, true);
						break;
					case FILE_ACTION_REMOVED:
						FileManager::instance ().signalFileRemoved (fileUrl, true);
						break;
					case FILE_ACTION_MODIFIED:
						FileManager::instance ().signalFileChanged (fileUrl, true);
						break;
					case FILE_ACTION_RENAMED_OLD_NAME:
						oldUrl = fileUrl;
						break;
					case FILE_ACTION_RENAMED_NEW_NAME:
						if(oldUrl.isEmpty () == false)
							FileManager::instance ().signalFileMoved (oldUrl, fileUrl, true);
						oldUrl = Url ();
						break;
					default:
						break;
					}

				if(info->NextEntryOffset == 0)
					break;
			}

			// Reset the event to not signaled
			::ResetEvent (item->overlapped.hEvent);

			// Read directory changes asynchronously
			if(!::ReadDirectoryChangesW (item->handle, item->changeBuffer, item->changeBuffer.getSize (),
				item->flags & IFileManager::kDeep, flags, nullptr, &item->overlapped, nullptr))
			{
				cancel ();
			}
		}
	}
	
	for(MonitoredDirectory* item : iterate_as<MonitoredDirectory> (items))
	{
		if(item->overlapped.hEvent != INVALID_HANDLE_VALUE)
		{
			::CloseHandle (item->overlapped.hEvent);
			item->overlapped.hEvent = INVALID_HANDLE_VALUE;
		}
	}
}

//************************************************************************************************
// WindowsFileManager::MonitorThread::MonitoredDirectory
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (WindowsFileManager::MonitorThread::MonitoredDirectory, WindowsFileManager::UrlItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsFileManager::MonitorThread::MonitoredDirectory::MonitoredDirectory (UrlRef url)
: UrlItem (url),
  handle (INVALID_HANDLE_VALUE)
{
	changeBuffer.resize (kChangeBufferSize);
	overlapped.hEvent = INVALID_HANDLE_VALUE;
}
