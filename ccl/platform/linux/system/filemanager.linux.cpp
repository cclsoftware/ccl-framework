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
// Filename    : ccl/platform/linux/filemanager.linux.cpp
// Description : Linux file manager
//
//************************************************************************************************

#include "filemanager.linux.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/system/userthread.h"
#include "ccl/public/systemservices.h"

#include <sys/inotify.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

namespace CCL {

//************************************************************************************************
// LinuxFileSystemMonitorThread
//************************************************************************************************

class LinuxFileSystemMonitorThread: public Threading::UserThread
{
public:
	LinuxFileSystemMonitorThread ();
	~LinuxFileSystemMonitorThread ();

	bool startWatching (UrlRef url, int flags);
	void stopWatching (UrlRef url);

	void cancel ();

protected:
	class MonitoredDirectory: public LinuxFileManager::UrlItem
	{
		DECLARE_CLASS (MonitoredDirectory, UrlItem)

		MonitoredDirectory (UrlRef url = Url (), int wd = -1);

		bool operator == (const MonitoredDirectory& other);

		int wd;
	};

	ObjectArray items;
	bool scanning;
	bool changing;
	int handle;
	int itemsChangedHandle[2];
	
	bool waitForItemsChanged ();
	void scanFileChanges ();

	// UserThread
	int threadEntry ();
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// LinuxFileManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (FileManager, LinuxFileManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxFileManager::LinuxFileManager ()
: thread (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxFileManager::~LinuxFileManager ()
{
	ASSERT (thread == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxFileManager::terminate ()
{
	if(thread)
	{
		thread->cancel ();
		thread->stopThread (500);
		delete thread;
		thread = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult LinuxFileManager::startWatching (UrlRef url, int flags)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	if(thread == nullptr)
	{
		thread = NEW LinuxFileSystemMonitorThread;
		thread->startThread (Threading::kPriorityBelowNormal);
	}

	if(thread && thread->startWatching (url, flags))
		return kResultOk;

	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult LinuxFileManager::stopWatching (UrlRef url)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	if(thread)
		thread->stopWatching (url);

	return kResultOk;
}

//************************************************************************************************
// LinuxFileSystemMonitorThread
//************************************************************************************************

LinuxFileSystemMonitorThread::LinuxFileSystemMonitorThread ()
: UserThread ("FileSystemMonitor"),
  scanning (false),
  changing (false),
  handle (-1),
  itemsChangedHandle {-1, -1}
{
	handle = inotify_init1 (IN_NONBLOCK);
	::pipe2 (itemsChangedHandle, O_NONBLOCK);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxFileSystemMonitorThread::~LinuxFileSystemMonitorThread ()
{
	ASSERT (items.isEmpty ())

	if(itemsChangedHandle[0] >= 0)
		::close (itemsChangedHandle[0]);
	if(itemsChangedHandle[1] >= 0)
		::close (itemsChangedHandle[1]);
	if(handle >= 0)
		::close (handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxFileSystemMonitorThread::startWatching (UrlRef url, int flags)
{
	ScopedVar<bool> scope (changing, true);
	
	ASSERT (((flags & IFileManager::kDeep) == 0) || url.isFolder ())
	
	char buffer = 1;
	::write (itemsChangedHandle[1], &buffer, 1);
	while(scanning)
		System::ThreadSleep (10);
		
	Url directory (url);
	if(directory.isFile ())
		directory.ascend ();
	MutableCString pathString (UrlDisplayString (directory), Text::kSystemEncoding);
	int wd = inotify_add_watch (handle, pathString.str (), IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE);
	
	if(wd < 0)
		return false;
	
	MonitoredDirectory* item = NEW MonitoredDirectory (url, wd);
	item->flags = flags;
	items.add (item);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxFileSystemMonitorThread::stopWatching (UrlRef url)
{
	ScopedVar<bool> scope (changing, true);
	
	char buffer = 1;
	::write (itemsChangedHandle[1], &buffer, 1);
	while(scanning)
		System::ThreadSleep (10);
	
	for(MonitoredDirectory* item : iterate_as<MonitoredDirectory> (items))
	{
		if(item->url == url)
		{
			inotify_rm_watch (handle, item->wd);
			item->release ();
			items.remove (item);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxFileSystemMonitorThread::cancel ()
{
	requestTerminate ();
	
	char buffer = 1;
	::write (itemsChangedHandle[1], &buffer, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int LinuxFileSystemMonitorThread::threadEntry ()
{
	while(true)
	{
		while(changing)
			System::ThreadSleep (10);
		
		ScopedVar<bool> scope (scanning, true);
		
		bool itemsChanged = waitForItemsChanged ();
		
		if(shouldTerminate ())
			break;
		
		if(itemsChanged)
			scanFileChanges ();

		if(shouldTerminate ())
			break;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxFileSystemMonitorThread::waitForItemsChanged ()
{
	pollfd fds[] =
	{
		{ handle, POLLIN, 0 },
		{ itemsChangedHandle[0], POLLIN, 0 }
	};
	
	::poll (fds, ARRAY_COUNT (fds), -1);
	
	char buffer;
	while(::read (itemsChangedHandle[0], &buffer, 1) > 0);
	
	return fds[0].revents & POLLIN > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxFileSystemMonitorThread::scanFileChanges ()
{
	char buffer[STRING_STACK_SPACE_MAX];
	Url oldUrl;
	
	while(true)
	{
		ssize_t length = ::read (handle, buffer, STRING_STACK_SPACE_MAX);
		if(length <= 0)
			break;
		
		inotify_event* event = nullptr;
		for(char* current = buffer; current < buffer + length; current += sizeof(inotify_event) + event->len)
		{
			event = reinterpret_cast<inotify_event*> (current);
			
			for(MonitoredDirectory* item : iterate_as<MonitoredDirectory> (items))
			{
				if(item->wd == event->wd)
				{
					Url fileUrl (item->url);
					
					if(fileUrl.isFolder ())
						fileUrl.descend (String (Text::kSystemEncoding, event->name));
					
					if(event->mask & IN_CREATE)
						LinuxFileManager::instance ().signalFileCreated (fileUrl, true);
					if(event->mask & IN_DELETE)
						LinuxFileManager::instance ().signalFileRemoved (fileUrl, true);
					if(event->mask & IN_MODIFY)
						LinuxFileManager::instance ().signalFileChanged (fileUrl, true);
					if(event->mask & IN_MOVED_FROM)
						oldUrl = fileUrl;
					if(event->mask & IN_MOVED_TO)
						LinuxFileManager::instance ().signalFileMoved (oldUrl, fileUrl, true);
				}
			}
		}
	}
}

//************************************************************************************************
// MonitoredDirectory
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LinuxFileSystemMonitorThread::MonitoredDirectory, UrlItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxFileSystemMonitorThread::MonitoredDirectory::MonitoredDirectory (UrlRef url, int wd)
: UrlItem (url),
  wd (wd)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxFileSystemMonitorThread::MonitoredDirectory::operator == (const MonitoredDirectory& other)
{ 
	return url == other.url; 
}
