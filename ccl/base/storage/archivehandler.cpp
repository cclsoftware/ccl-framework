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
// Filename    : ccl/base/storage/archivehandler.cpp
// Description : Archive Handler
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/base/storage/archivehandler.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/xmlarchive.h"

#include "ccl/base/collections/objectlist.h"

#include "ccl/public/storage/istorage.h"
#include "ccl/public/base/streamer.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// StorableSaveTask
//************************************************************************************************

class StorableSaveTask: public ArchiveSaveTask
{
public:
	StorableSaveTask (IStorable& storable, StringID _debugName)
	: storable (storable)
	{
		#if DEBUG
		debugName = _debugName;
		#endif
		storable.retain ();
	}

	~StorableSaveTask ()
	{
		storable.release ();
	}

	// ArchiveSaveTask
	tresult CCL_API writeData (IStream& dstStream, IProgressNotify* progress) override
	{
		CCL_PRINT ("StorableSaveTask writing data for ")
		CCL_PRINTLN (debugName)

		tbool result = storable.save (dstStream);
		return result ? kResultOk : kResultFailed;
	}

protected:
	IStorable& storable;
	#if DEBUG
	MutableCString debugName;
	#endif
};

//************************************************************************************************
// StreamCopyTask
//************************************************************************************************

class StreamCopyTask: public ArchiveSaveTask
{
public:
	StreamCopyTask (IStream& data)
	: data (data)
	{
		data.retain ();
	}

	~StreamCopyTask ()
	{
		data.release ();
	}

	// ArchiveSaveTask
	tresult CCL_API writeData (IStream& dstStream, IProgressNotify* progress) override
	{
		if(UnknownPtr<IMemoryStream> memStream = &data)
			return memStream->writeTo (dstStream) ? kResultOk : kResultFailed;
		else
		{
			data.rewind ();
			return System::GetFileUtilities ().copyStream (dstStream, data, progress) ? kResultOk : kResultFailed;
		}
	}

protected:
	IStream& data;
};

//************************************************************************************************
// ObjectSaveTask
//************************************************************************************************

class ObjectSaveTask: public ArchiveSaveTask
{
public:
	ObjectSaveTask (ArchiveHandler& handler, StringID name, Object& object, int xmlFlags)
	: handler (handler),
	  name (name),
	  object (object),
	  xmlFlags (xmlFlags)
	{
		handler.retain ();
		object.retain ();
	}

	~ObjectSaveTask ()
	{
		object.release ();
		handler.release ();
	}

	// ArchiveSaveTask
	tresult CCL_API writeData (IStream& dstStream, IProgressNotify* progress) override
	{
		CCL_PRINT ("ObjectSaveTask writing data for ")
		CCL_PRINT (object.myClass ().getPersistentName ())
		CCL_PRINTLN ("")

		XmlArchive archive (dstStream, &handler.getContext (), handler.getSaveType ());
		archive.setFlags (xmlFlags);
		bool result = archive.saveObject (name, object);
		return result ? kResultOk : kResultFailed;
	}

protected:
	ArchiveHandler& handler;
	MutableCString name;
	Object& object;
	int xmlFlags;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// ArchiveHandler::ArrayTocItem
//************************************************************************************************

class ArchiveHandler::ArrayTocItem
{
public:
	ArrayTocItem (int64 offset = 0, int64 size = 0)
	: offset (offset), size (size)
	{}

	int64 offset;
	int64 size;
};

//************************************************************************************************
// ArchiveHandler::ArrayTask
//************************************************************************************************

class ArchiveHandler::ArrayTask: public ArchiveSaveTask
{
public:
	DECLARE_CLASS_ABSTRACT (ArrayTask, ArchiveSaveTask)

	ArrayTask ();

	int addItem (ArchiveSaveTask* task);

	// IPackageItemWriter
	tresult CCL_API writeData (IStream& dstStream, IProgressNotify* progress = nullptr) override;

private:
	ObjectList items;
	int numItems;
};

//************************************************************************************************
// ArchiveHandler::ArrayStream
//************************************************************************************************

class ArchiveHandler::ArrayStream: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (ArrayStream, Object)

	ArrayStream (IStream* stream);

	IStream* openItem (int index);

private:
	AutoPtr<IStream> stream;
	Vector<ArrayTocItem> toc;
};

//************************************************************************************************
// ArchiveHandler
//************************************************************************************************

ArchiveHandler* ArchiveHandler::getHandler (const Storage& storage)
{
	ArchiveHandler* handler = unknown_cast<ArchiveHandler> (storage.getContextUnknown ("ArchiveHandler"));
	return handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (ArchiveHandler, Object)

/*static*/ArchiveHandler* ArchiveHandler::toplevelHandler = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

ArchiveHandler::ArchiveHandler (IFileSystem& fileSystem, StringID saveType)
: fileSystem (fileSystem),
  saveType (saveType)
{
	fileSystem.retain ();
	context.set ("ArchiveHandler", this);

	if(toplevelHandler == nullptr)
		toplevelHandler = this;
	else if(&toplevelHandler->getFileSystem () == &fileSystem)
		setSourcePackage (toplevelHandler->getSourcePackage ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ArchiveHandler::~ArchiveHandler ()
{
	fileSystem.release ();

	if(toplevelHandler == this)
		toplevelHandler = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileSystem& ArchiveHandler::getFileSystem ()
{
	return fileSystem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& ArchiveHandler::getContext ()
{
	return context;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* ArchiveHandler::openStream (StringRef path, int mode)
{
	Url url;
	url.setPath (path);
	return fileSystem.openStream (url, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArchiveHandler::loadItem (StringRef path, Archive::ObjectID name, Object& item, int xmlFlags)
{
	bool result = false;
	AutoPtr<IStream> stream = openStream (path, IStream::kOpenMode);
	if(stream)
	{
		XmlArchive archive (*stream, &context, saveType);
		archive.setFlags (xmlFlags);
		result = archive.loadObject (name, item);
		ASSERT (result == true)
		if(result == false)
		{
			CCL_WARN ("Failed to load \"%s\" from package!\n", MutableCString (path).str ())
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArchiveHandler::loadStream (StringRef path, IStorable& item)
{
	bool result = false;
	AutoPtr<IStream> stream = openStream (path, IStream::kOpenMode);
	if(stream)
	{
		result = item.load (*stream) ? true : false;
		ASSERT (result == true)
		if(result == false)
		{
			CCL_WARN ("Failed to load \"%s\" from package!\n", MutableCString (path).str ())
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* ArchiveHandler::copyData (StringRef path)
{
	AutoPtr<IStream> stream = openStream (path, IStream::kOpenMode);
	if(stream == nullptr)
		return nullptr;

	return System::GetFileUtilities ().createStreamCopyInMemory (*stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArchiveHandler::addSaveTask (StringRef path, ArchiveSaveTask* task, int* attributes)
{
	ASSERT (task != nullptr)

	UnknownPtr<IPackageFile> package (&fileSystem);
	if(package && !package->getPath ().isFolder ()) // createItem() is not supported by FolderPackage!
	{
		Url url;
		url.setPath (path);
		IPackageItem* item = package->createItem (url, task, attributes);
		return item != nullptr;
	}
	else
	{
		AutoPtr<ArchiveSaveTask> taskReleaser (task);
		AutoPtr<IStream> stream = openStream (path, IStream::kCreateMode);
		return stream ? task->writeData (*stream, progress) == kResultOk : false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArchiveHandler::addSaveTask (StringRef path, IStorable& item, StringID debugName, int* attributes)
{
	return addSaveTask (path, NEW StorableSaveTask (item, debugName), attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArchiveHandler::addSaveTask (StringRef path, IStream& data, int* attributes)
{
	return addSaveTask (path, NEW StreamCopyTask (data), attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArchiveHandler::addSaveTask (StringRef path, Archive::ObjectID name, Object& item, int xmlFlags)
{
	return addSaveTask (path, NEW ObjectSaveTask (*this, name, item, xmlFlags));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArchiveHandler::addCopyTask (IPackageFile* sourcePackage, StringRef sourcePath, StringRef destPath)
{
	Url sourceUrl;
	sourceUrl.setPath (sourcePath);

	UnknownPtr<IPackageFile> package (&fileSystem);
	if(package && !package->getPath ().isFolder ()) // createItem() is not supported by FolderPackage!
	{
		IUrl* destUrl = nullptr;
		Url tmpUrl;
		if(!destPath.isEmpty ())
		{
			tmpUrl.setPath (destPath);
			destUrl = &tmpUrl;
		}
		IPackageItem* item = package->copyItem (sourcePackage, sourceUrl, destUrl);
		return item != nullptr;
	}
	else
	{
		AutoPtr<IStream> destStream = openStream (destPath, IStream::kCreateMode);
		AutoPtr<IStream> sourceStream = sourcePackage ? sourcePackage->getFileSystem ()->openStream (sourceUrl) : nullptr;
		if(destStream && sourceStream)
			return System::GetFileUtilities ().copyStream (*destStream, *sourceStream) != 0;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ArchiveHandler::addArrayItemTask (StringRef path, ArchiveSaveTask* task)
{
	MutableCString key ("array:");
	key += path;

	ArrayTask* arrayItem = getContext ().getObject<ArrayTask> (key);
	if(!arrayItem)
	{
		arrayItem = NEW ArrayTask;
		getContext ().set (key, arrayItem);

		addSaveTask (path, arrayItem);
	}
	return arrayItem->addItem (task);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* ArchiveHandler::openArrayItem (StringRef path, int index)
{
	MutableCString key ("array:");
	key += path;

	ArrayStream* arrayItem = getContext ().getObject<ArrayStream> (key);
	if(!arrayItem)
	{
		IStream* stream = openStream (path, IStream::kOpenMode);

		// even if opening stream failed, to avoid trying again
		arrayItem = NEW ArrayStream (stream);
		getContext ().set (key, arrayItem, Attributes::kOwns);
	}
	return arrayItem->openItem (index);
}

//************************************************************************************************
// ArchiveSaveTask
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ArchiveSaveTask, Object)

//************************************************************************************************
// ArchiveHandler::ArrayTask
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ArchiveHandler::ArrayTask, ArchiveSaveTask)

//////////////////////////////////////////////////////////////////////////////////////////////////

inline ArchiveHandler::ArrayTask::ArrayTask ()
: numItems (0)
{
	items.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline int ArchiveHandler::ArrayTask::addItem (ArchiveSaveTask* task)
{
	items.add (task);
	return numItems++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ArchiveHandler::ArrayTask::writeData (IStream& stream, IProgressNotify* progress)
{
	LinkedList<ArrayTocItem> toc;
	int64 itemStart = 0;

	ListForEachObject (items, ArchiveSaveTask, task)
		task->writeData (stream, progress);

		int64 currentPos = stream.tell ();
		int64 size = currentPos - itemStart;

		toc.append (ArrayTocItem (itemStart, size));
		itemStart = currentPos;
	EndFor

	// write toc
	int64 tocStart = stream.tell ();
	Streamer streamer (stream, kLittleEndian);
	ListForEach (toc, ArrayTocItem, item)
		streamer.write (item.offset);
		streamer.write (item.size);
	EndFor
	streamer.write (tocStart);
	return kResultOk;
}

//************************************************************************************************
// ArchiveHandler::ArrayStream
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ArchiveHandler::ArrayStream, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

inline ArchiveHandler::ArrayStream::ArrayStream (IStream* _stream)
: stream (_stream)
{
	if(stream)
	{
		stream = System::GetFileUtilities ().createSeekableStream (*stream);

		// read toc
		Streamer streamer (*stream, kLittleEndian);

		int64 end = stream->seek (0, IStream::kSeekEnd);
		stream->seek (end - sizeof(int64), IStream::kSeekSet);
		int64 tocStart = 0;
		streamer.read (tocStart);
		stream->seek (tocStart, IStream::kSeekSet);

		int numItems = int(end - tocStart) / (2 * sizeof(int64));
		toc.resize (numItems);
		for(int i = 0; i < numItems; i++)
		{
			ArrayTocItem item;
			streamer.read (item.offset);
			streamer.read (item.size);
			toc.add (item);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* ArchiveHandler::ArrayStream::openItem (int index)
{
	if(stream && index >= 0 && index < toc.count ())
	{
		ArrayTocItem& item (toc[index]);
		return System::GetFileUtilities ().createSectionStream (*stream, item.offset, item.size);
	}
	return nullptr;
}
