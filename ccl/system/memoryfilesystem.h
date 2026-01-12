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
// Filename    : ccl/system/memoryfilesystem.h
// Description : Memory-based File System
//
//************************************************************************************************

#ifndef _ccl_memoryfilesystem_h
#define _ccl_memoryfilesystem_h

#include "ccl/public/text/cclstring.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/base/memorystream.h"

#include "ccl/public/system/ilockable.h"
#include "ccl/public/system/ifilesystem.h"

#include "ccl/base/collections/objectarray.h"

namespace CCL {

//************************************************************************************************
// MemoryFileSystem
//************************************************************************************************

class MemoryFileSystem: public Unknown,
						public AbstractFileSystem
{
public:
	MemoryFileSystem ();

	class Bin;
	class Entry;
	class ReadStream;
	class WriteStream;
	class EntryIterator;

	PROPERTY_SHARED_AUTO (Threading::ILockable, lock, Lock)

	// IFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode = IStream::kOpenMode, IUnknown* context = nullptr) override;
	tbool CCL_API fileExists (UrlRef url) override;
	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) override;
	tbool CCL_API removeFile (UrlRef url, int mode = 0) override;
	tbool CCL_API removeFolder (UrlRef url, int mode = 0) override;
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;

	CLASS_INTERFACE (IFileSystem, Unknown)

protected:
	ObjectArray bins;

	Entry* findEntry (UrlRef url, Bin** outBin = nullptr);
	Entry* makeEntry (UrlRef url);
};

//************************************************************************************************
// MemoryFileSystem::Bin
//************************************************************************************************

class MemoryFileSystem::Bin: public Object
{
public:
	Bin (StringRef name = nullptr);

	PROPERTY_STRING (name, Name)

	// Object
	int compare (const Object& obj) const override;

protected:
	friend class MemoryFileSystem;
	ObjectArray entries;
};

//************************************************************************************************
// MemoryFileSystem::Entry
//************************************************************************************************

class MemoryFileSystem::Entry: public Object
{
public:
	Entry (StringRef fileName = nullptr);

	PROPERTY_STRING (fileName, FileName)
	PROPERTY_SHARED_AUTO (Buffer, buffer, Buffer)
	PROPERTY_VARIABLE (uint32, bytesWritten, BytesWritten)

	// Object
	int compare (const Object& obj) const override;
};

//************************************************************************************************
// MemoryFileSystem::ReadStream
//************************************************************************************************

class MemoryFileSystem::ReadStream: public MemoryStream
{
public:
	ReadStream (Entry* entry);

	PROPERTY_SHARED_AUTO (Entry, entry, Entry)
	PROPERTY_SHARED_AUTO (Buffer, sharedBuffer, SharedBuffer)
};

//************************************************************************************************
// MemoryFileSystem::WriteStream
//************************************************************************************************

class MemoryFileSystem::WriteStream: public MemoryStream
{
public:
	WriteStream (Entry* entry);
	~WriteStream ();

	PROPERTY_SHARED_AUTO (Entry, entry, Entry)
};

//************************************************************************************************
// MemoryFileSystem::EntryIterator
//************************************************************************************************

class MemoryFileSystem::EntryIterator: public Unknown,
									   public IFileIterator
{
public:
	EntryIterator (StringRef protocol);

	void add (StringRef binName, StringRef entryName);

	// IFileIterator
	const IUrl* CCL_API next () override;

	CLASS_INTERFACE (IFileIterator, Unknown)

protected:
	String protocol;
	ObjectArray paths;
	AutoPtr<Iterator> iter;
};

} // namespace CCL

#endif // _ccl_memoryfilesystem_h
