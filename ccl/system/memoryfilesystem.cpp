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
// Filename    : ccl/system/memoryfilesystem.cpp
// Description : Memory-based File System
//
//************************************************************************************************

#include "ccl/system/memoryfilesystem.h"
#include "ccl/system/threading/threadlocks.h"

#include "ccl/base/storage/url.h"

using namespace CCL;

//************************************************************************************************
// MemoryFileSystem
//************************************************************************************************

MemoryFileSystem::MemoryFileSystem ()
{
	lock = NEW Threading::ReadWriteLock;
	bins.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MemoryFileSystem::Entry* MemoryFileSystem::findEntry (UrlRef url, Bin** outBin)
{
	StringRef binName = url.getHostName ();
	StringRef fileName = url.getPath ();
	ASSERT (!fileName.contains (Url::strPathChar)) // subfolders are not fully supported

	Bin* bin = (Bin*)bins.search (Bin (binName));
	if(outBin)
		*outBin = bin;
	return bin ? (Entry*)bin->entries.search (Entry (fileName)) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MemoryFileSystem::Entry* MemoryFileSystem::makeEntry (UrlRef url)
{
	StringRef binName = url.getHostName ();
	StringRef fileName = url.getPath ();
	ASSERT (!fileName.contains (Url::strPathChar)) // subfolders are not fully supported

	Entry* entry = nullptr;
	Bin* bin = (Bin*)bins.search (Bin (binName));
	if(bin == nullptr)
	{
		bin = NEW Bin (binName);
		bins.addSorted (bin);
	}
	else
		entry = (Entry*)bin->entries.search (Entry (fileName));

	if(entry == nullptr)
	{
		entry = NEW Entry (fileName);
		bin->entries.addSorted (entry);
	}
	return entry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IStream* CCL_API MemoryFileSystem::openStream (UrlRef url, int mode, IUnknown* context)
{
	bool create = (mode & CCL::IStream::kCreate) != 0;
	bool write = (mode & CCL::IStream::kWriteMode) != 0;
	if(create)
	{
		Threading::AutoLock autoLock (lock, Threading::ILockable::kWrite);

		if(Entry* entry = makeEntry (url))
			return NEW WriteStream (entry);
	}
	else
	{
		Threading::AutoLock autoLock (lock, Threading::ILockable::kRead);

		if(Entry* entry = findEntry (url))
		{
			if(write)
				return NEW WriteStream (entry);
			else
				return NEW ReadStream (entry);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MemoryFileSystem::fileExists (UrlRef url)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kRead);

	return findEntry (url) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MemoryFileSystem::getFileInfo (FileInfo& info, UrlRef url)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kRead);

	// get total memory utilization
	if(url.getHostName ().isEmpty ())
	{
		int64 totalSize = 0;
		ArrayForEachFast (bins, Bin, bin)
			ArrayForEachFast (bin->entries, Entry, entry)
				totalSize += entry->getBytesWritten ();
			EndFor
		EndFor

		info.fileSize = totalSize;
		return true;
	}
	// get utilization per bin
	else if(url.getPath ().isEmpty ())
	{
		StringRef binName = url.getHostName ();
		Bin* bin = (Bin*)bins.search (Bin (binName));
		if(bin == nullptr)
			return false;

		int64 totalSize = 0;
		ArrayForEachFast (bin->entries, Entry, entry)
			totalSize += entry->getBytesWritten ();
		EndFor

		info.fileSize = totalSize;
		return true;
	}
	// get memory utilized by entry
	else
	{
		Entry* entry = findEntry (url);
		if(entry == nullptr)
			return false;

		info.fileSize = entry->getBytesWritten ();
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MemoryFileSystem::removeFile (UrlRef url, int mode)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kWrite);

	Bin* bin = nullptr;
	Entry* entry = findEntry (url, &bin);
	if(entry == nullptr)
		return false;

	bin->entries.remove (entry);
	entry->release ();

	if(bin->entries.isEmpty ())
	{
		bins.remove (bin);
		bin->release ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MemoryFileSystem::removeFolder (UrlRef url, int mode)
{
	// can only remove whole bins
	ASSERT (url.getPath ().isEmpty ())
	if(!url.getPath ().isEmpty ())
		return false;

	Threading::AutoLock autoLock (lock, Threading::ILockable::kWrite);

	StringRef binName = url.getHostName ();
	Bin* bin = (Bin*)bins.search (Bin (binName));
	if(!bin)
		return false;

	bins.remove (bin);
	bin->release ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API MemoryFileSystem::newIterator (UrlRef url, int mode)
{
	if(!(mode & IFileIterator::kFiles))
		return nullptr;

	Threading::AutoLock autoLock (lock, Threading::ILockable::kRead);

	AutoPtr<EntryIterator> iter = NEW EntryIterator (url.getProtocol ());

	// all entries
	if(url.getHostName ().isEmpty ())
	{
		ArrayForEachFast (bins, Bin, bin)
			ArrayForEachFast (bin->entries, Entry, entry)
				iter->add (bin->getName (), entry->getFileName ());
			EndFor
		EndFor
	}
	// entries of given bin
	else
	{
		StringRef binName = url.getHostName ();
		Bin* bin = (Bin*)bins.search (Bin (binName));
		if(bin == nullptr)
			return nullptr;

		ArrayForEachFast (bin->entries, Entry, entry)
			iter->add (binName, entry->getFileName ());
		EndFor
	}

	return iter.detach ();
}

//************************************************************************************************
// MemoryFileSystem::Bin
//************************************************************************************************

MemoryFileSystem::Bin::Bin (StringRef name)
: name (name)
{
	entries.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MemoryFileSystem::Bin::compare (const Object& obj) const
{
	return name.compare (((Bin&)obj).name);
}

//************************************************************************************************
// MemoryFileSystem::Entry
//************************************************************************************************

MemoryFileSystem::Entry::Entry (StringRef fileName)
: fileName (fileName),
  bytesWritten (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MemoryFileSystem::Entry::compare (const Object& obj) const
{
	return fileName.compare (((Entry&)obj).fileName);
}

//************************************************************************************************
// MemoryFileSystem::ReadStream
//************************************************************************************************

MemoryFileSystem::ReadStream::ReadStream (Entry* entry)
: MemoryStream (entry->getBuffer () ? entry->getBuffer ()->getAddress () : nullptr, entry->getBuffer () ? entry->getBuffer ()->getSize () : 0)
{
	setBytesWritten (entry->getBytesWritten ());
	setEntry (entry);
	setSharedBuffer (entry->getBuffer ());
}

//************************************************************************************************
// MemoryFileSystem::WriteStream
//************************************************************************************************

MemoryFileSystem::WriteStream::WriteStream (Entry* entry)
{
	setEntry (entry);

	if(Buffer* oldBuffer = entry->getBuffer ())
	{
		MemoryStream temp (oldBuffer->getAddress (), oldBuffer->getSize ());
		copyFrom (temp);
		setBytesWritten (entry->getBytesWritten ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MemoryFileSystem::WriteStream::~WriteStream ()
{
	AutoPtr<Buffer> newBuffer = NEW Buffer;
	newBuffer->take (this->memory);
	entry->setBytesWritten (this->bytesWritten);
	entry->setBuffer (newBuffer);
}

//************************************************************************************************
// MemoryFileSystem::EntryIterator
//************************************************************************************************

MemoryFileSystem::EntryIterator::EntryIterator (StringRef protocol)
: protocol (protocol)
{
	paths.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MemoryFileSystem::EntryIterator::add (StringRef binName, StringRef entryName)
{
	Url* path = NEW Url;
	path->setProtocol (protocol);
	path->setHostName (binName);
	path->setPath (entryName);
	paths.add (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* CCL_API MemoryFileSystem::EntryIterator::next ()
{
	if(!iter)
		iter = paths.newIterator ();
	if(!iter)
		return nullptr;
	return (Url*)iter->next ();
}
