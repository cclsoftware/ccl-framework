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
// Filename    : core/portable/corefilesystem.cpp
// Description : File System
//
//************************************************************************************************

#include "corefile.h"
#include "coreworker.h"
#include "corezipstream.h"

#include "core/system/coredebug.h"
#include "core/system/corezipfileformat.impl.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// FileUtils
//************************************************************************************************

bool FileUtils::removeDirectoryTree (CStringPtr dirname)
{
	#if DEBUG
		static int sanityDepth = 0;
		sanityDepth++;
		ASSERT (sanityDepth < 4); // This method could nuke your harddrive... how deep are you planning on going!?
		if(sanityDepth > 3)
			return false;
	#endif
		
	bool allDeleted = true;
	FileIterator iter (dirname);
	while(const FileIterator::Entry* entry = iter.next ())
	{
		if(entry->directory)
			allDeleted = removeDirectoryTree (entry->name);
		else
			allDeleted = deleteFile (entry->name);
			
		if(!allDeleted)
			break;
	}
		
	#if DEBUG
		sanityDepth--;
	#endif
		
	if(allDeleted)
		return removeDirectory (dirname);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IO::MemoryStream* FileUtils::loadFile (CStringPtr filename)
{
	FileStream file;
	if(file.open (filename, IO::kReadMode))
	{
		int size = (int)file.getFileSize ();
		IO::MemoryStream* stream = NEW IO::MemoryStream;
		if(stream->allocateMemory (size))
		{
			void* buffer = const_cast<IO::Buffer&> (stream->getBuffer ()).getAddress ();
			int numRead = file.readBytes (buffer, size);
			if(numRead >= 0)
			{
				stream->setBytesWritten (numRead);
				return stream;
			}
		}
		delete stream;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileUtils::saveFile (CStringPtr filename, const IO::MemoryStream& data)
{
	bool result = false;
	FileStream file;
	if(file.create (filename))
	{
		int size = (int)data.getBytesWritten ();
		result = file.writeBytes (data.getBuffer ().getAddress (), size) == size;
		file.close ();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileUtils::copyFile (CStringPtr source, CStringPtr destination)
{
	IO::MemoryStream* sourceStream = loadFile (source);
	if(!sourceStream)
		return false;

	bool success = saveFile (destination, *sourceStream);
	delete sourceStream;
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileUtils::copyDirectoryTree (CStringPtr source, CStringPtr destination)
{
	FileIterator iter (source);
	while(const FileIterator::Entry* entry = iter.next ())
	{
		FileName name;
		entry->name.getName (name);

		FileName destinationPath (destination);
		destinationPath.descend (name);

		if(entry->directory)
		{
			if(!makeDirectory (destinationPath))
				return false;
			
			if(!copyDirectoryTree (entry->name, destinationPath))
				return false;
		}
		else if(!copyFile (entry->name, destinationPath))
				return false;
	}
	return true;
}

//************************************************************************************************
// FolderPackage
//************************************************************************************************

FolderPackage::FolderPackage (CStringPtr baseFolder, bool bufferedMode)
: baseFolder (baseFolder),
  bufferedMode (bufferedMode)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderPackage::fileExists (CStringPtr fileName)
{
	FileName fullName (baseFolder);
	fullName.descend (fileName);

	return FileUtils::fileExists (fullName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IO::Stream* FolderPackage::openStream (CStringPtr fileName)
{
	FileName fullName (baseFolder);
	fullName.descend (fileName);

	if(bufferedMode == true)
		return FileUtils::loadFile (fullName);
	else
	{
		FileStream* stream = NEW FileStream;
		if(stream->open (fullName, IO::kReadMode))
			return stream;

		delete stream;
		return nullptr;
	}
}

//************************************************************************************************
// SubPackage
//************************************************************************************************

SubPackage::SubPackage (FilePackage& parent, CStringPtr baseFolder)
: parent (parent),
  baseFolder (baseFolder)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SubPackage::fileExists (CStringPtr _fileName)
{
	FileName fullName (baseFolder);
	fullName.descend (_fileName);
	
	return parent.fileExists (fullName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IO::Stream* SubPackage::openStream (CStringPtr _fileName)
{
	FileName fullName (baseFolder);
	fullName.descend (_fileName);

	return parent.openStream (fullName);
}

//************************************************************************************************
// ZipPackage
//************************************************************************************************

ZipPackage::ZipPackage ()
: file (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ZipPackage::~ZipPackage ()
{
	close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZipPackage::openFromFile (CStringPtr fileName, bool bufferedMode)
{
	if(bufferedMode == true)
		file = FileUtils::loadFile (fileName);
	else
	{
		FileStream* fileStream = NEW FileStream;
		fileStream->open (fileName, IO::kReadMode);
		file = fileStream;
	}
	return readFormat ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZipPackage::openFromMemory (const void* data, uint32 size)
{
	file = NEW IO::MemoryStream (const_cast<void*> (data), size);
	IO::Buffer temp (const_cast<void*> (data), size, false);
	fileBuffer.take (temp);
	return readFormat ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZipPackage::close ()
{
	delete file;
	file = nullptr;
	fileBuffer.resize (0);
	entries.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZipPackage::readFormat ()
{
	if(file == nullptr)
		return false;

	// search for central directory
	Zip::CentralDirEndRecord endRecord = {0};
	if(!Zip::findCentralDirectoryEnd (endRecord, *file))
		return false;
		
	// load central directory into memory stream
	IO::MemoryStream dirStream;
	if(!fileBuffer.isNull ())
	{
		// take members, no copy
		IO::MemoryStream temp (fileBuffer.as<uint8> () + endRecord.dirOffset, endRecord.dirSize);
		dirStream.take (temp);
	}
	else
	{
		if(!dirStream.allocateMemory (endRecord.dirSize))
			return false;
		if(file->setPosition (endRecord.dirOffset, IO::kSeekSet) != endRecord.dirOffset)
			return false;
		if(static_cast<uint32> (file->readBytes ((void*)dirStream.getBuffer ().getAddress (), endRecord.dirSize)) != endRecord.dirSize)
			return false;
		dirStream.setBytesWritten (endRecord.dirSize);
	}

	// parse central directory entries
	entries.resize (endRecord.numEntriesThisDisk);
	IO::BinaryStreamAccessor dirAccessor (dirStream, Zip::kZipByteOrder); 
	for(int i = 0; i < endRecord.numEntriesThisDisk; i++)
	{
		// read header
		Zip::CentralDirFileHeader header = {0};
		if(!header.read (dirAccessor))
			break;
		if(header.signature != Zip::kCentralDirFileHeaderSignature)
			break;

		int offset = header.getAdditionalSize ();

		Entry entry;
		entry.localHeaderOffset = header.localHeaderOffset;
		entry.compressedSize = header.compressedSize;
		entry.uncompressedSize = header.uncompressedSize;

		// read file name
		int nameLength = get_min<int> (header.fileNameLength, entry.name.getSize ()-1);
		if(dirStream.readBytes (entry.name.getBuffer (), nameLength) != nameLength)
			break;

		entry.name.truncate (nameLength);
		entry.name.adjustPathDelimiters (FileName::kForwardSlash); // make sure we are using forward slashes
		offset -= nameLength;

		#if (0 && DEBUG)
		DebugPrintf ("%02d: %s\n", i, entry.name.str ());
		#endif

		if(header.compressedSize > 0) // ignore directories
			entries.addSorted (entry);

		if(offset != 0)
			dirStream.setPosition (offset, IO::kSeekCur);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ZipPackage::Entry* ZipPackage::findEntry (CStringPtr fileName) const
{
	const Entry* entry = entries.search (Entry (fileName));
	if(entry == nullptr && FileName::kPathDelimiter[0] != '/')
	{
		// second try with adjusted path delimiters
		FileName fileName2 (fileName);
		fileName2.adjustPathDelimiters (FileName::kForwardSlash);
		if(fileName2 != fileName)
			entry = entries.search (Entry (fileName2));
	}
	return entry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 ZipPackage::getStreamSize (CStringPtr fileName) const
{
	const Entry* entry = findEntry (fileName);
	if(entry == nullptr)
		return 0;
	return entry->uncompressedSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IO::Stream* ZipPackage::openFirstStream (uint32& uncompressedSize)
{
	if(entries.isEmpty ())
		return nullptr;

	uncompressedSize = entries[0].uncompressedSize;
	return openEntry (entries[0]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZipPackage::fileExists (CStringPtr fileName)
{
	return findEntry (fileName) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IO::Stream* ZipPackage::openStream (CStringPtr fileName)
{
	const Entry* entry = findEntry (fileName);
	if(entry == nullptr)
		return nullptr;

	return openEntry (*entry);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IO::Stream* ZipPackage::openEntry (const Entry& entry)
{
	ASSERT (file != nullptr)
	if(file == nullptr)
		return nullptr;

	if(file->setPosition (entry.localHeaderOffset, IO::kSeekSet) != entry.localHeaderOffset)
		return nullptr;

	Zip::LocalFileHeader localHeader = {0};
	IO::BinaryStreamAccessor accessor (*file);
	localHeader.read (accessor);
	if(localHeader.signature != Zip::kLocalFileHeaderSignature)
		return nullptr;

	// ATTENTION: Seems like some ZIP tools don't set the (un)compressed size in local header correctly!
	uint32 uncompressedSize = entry.uncompressedSize; //localHeader.uncompressedSize
	uint32 compressedSize = entry.compressedSize; //localHeader.compressedSize

	file->setPosition (localHeader.getAdditionalSize (), IO::kSeekCur);

	// check if memory can be accessed directly, without additional copy
	const uint8* startAddress = !fileBuffer.isNull () ? fileBuffer.as<uint8> () + file->getPosition () : nullptr;

	//*** Method = Deflated ***
	if(localHeader.compressionMethod == Zip::kCompressionMethodDeflated)
	{
		ZlibReadStream* readStream = NEW ZlibReadStream (-MAX_WBITS);
		readStream->setUncompressedSize (uncompressedSize);
		if(startAddress != nullptr)
		{
			readStream->initFromMemory (startAddress, compressedSize);
			return readStream;
		}
		else
		{
			if(readStream->copyFromSource (*file, compressedSize))
				return readStream;
		}

		delete readStream;
	}
	//*** Method = No Compression ***
	else if(localHeader.compressionMethod == Zip::kCompressionMethodNone)
	{
		if(startAddress != nullptr)
		{
			return NEW IO::MemoryStream ((void*)startAddress, compressedSize);
		}
		else
		{
			IO::MemoryStream* memStream = NEW IO::MemoryStream;
			if(memStream->allocateMemory (compressedSize))
				if(file->readBytes ((void*)memStream->getBuffer ().getAddress (), compressedSize) == static_cast<int> (compressedSize))
				{
					memStream->setBytesWritten (compressedSize);
					return memStream;
				}
		
			delete memStream;
		}
	}
	return nullptr;
}

//************************************************************************************************
// FileStorageContext
//************************************************************************************************

class FileStorageContext::Implementation: public FileStorageContext
{
public:
	// FileStorageContext
	IO::MemoryStream* loadFile (CStringPtr filename, Mode mode, int streamSizeEstimate = 0) override
	{
		switch(mode)
		{
		case kCopy : return FileUtils::loadFile (filename);
		case kCompress : return loadPlainToZip (filename);
		case kDecompress : return loadZipToPlain (filename, streamSizeEstimate);
		}
		return nullptr;
	}

	bool saveFile (CStringPtr filename, const IO::MemoryStream& data, Mode mode) override
	{
		switch(mode)
		{
		case kCopy : return FileUtils::saveFile (filename, data);
		case kCompress : return savePlainToZip (filename, data);
		case kDecompress : return saveZipToPlain (filename, data);
		}
		return 0;
	}

	bool compress (IO::Stream& compressedData, const IO::MemoryStream& data) override
	{
		return compressionHandler.zip (compressedData, data.getBuffer (), (int)data.getBytesWritten ());
	}

	bool decompress (IO::Stream& plainData, const IO::MemoryStream& data) override
	{
		return compressionHandler.unzip (plainData, data.getBuffer (), (int)data.getBytesWritten ());
	}

protected:
	CompressionHandler compressionHandler;

	IO::MemoryStream* loadPlainToZip (CStringPtr filename)
	{
		if(IO::MemoryStream* plainData = FileUtils::loadFile (filename))
		{
			Deleter<IO::MemoryStream> streamDeleter (plainData);
			IO::MemoryStream* zipData = NEW IO::MemoryStream;
			if(compress (*zipData, *plainData) == true)
				return zipData;

			delete zipData;
		}
		return nullptr;
	}

	IO::MemoryStream* loadZipToPlain (CStringPtr filename, int streamSizeEstimate = 0)
	{
		if(IO::MemoryStream* zipData = FileUtils::loadFile (filename))
		{
			Deleter<IO::MemoryStream> streamDeleter (zipData);
			IO::MemoryStream* plainData = NEW IO::MemoryStream;
			if(streamSizeEstimate > 0)
				plainData->allocateMemory (streamSizeEstimate);
			if(decompress (*plainData, *zipData) == true)
			{
				// sanity check - increase estimate if needed!
				ASSERT (streamSizeEstimate == 0 || plainData->getBuffer ().getSize () == streamSizeEstimate)
				return plainData;
			}

			delete plainData;
		}
		return nullptr;
	}

	bool saveZipToPlain (CStringPtr filename, const IO::MemoryStream& data)
	{
		FileStream file;
		if(file.create (filename))
			return decompress (file, data);
		else
			return false;
	}

	bool savePlainToZip (CStringPtr filename, const IO::MemoryStream& data)
	{
		FileStream file;
		if(file.create (filename))
			return compress (file, data);
		else
			return false;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

FileStorageContext* FileStorageContext::create ()
{
	return NEW Implementation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileStorageContext& FileStorageContext::getMainThreadInstance ()
{
	static Implementation theInstance;
	return theInstance;
}

//************************************************************************************************
// FileIOManager::Worker
//************************************************************************************************

class FileIOManager::Worker: public BackgroundWorker,
							 public FileStorageContext::Implementation
{
};

//************************************************************************************************
// FileIOManager::SaveTask
//************************************************************************************************

struct FileIOManager::SaveTask: public BackgroundTask
{
	Worker& worker;
	FileName filename;
	FileStorageContext::Mode mode;
	NotifyEntry* entry;
	
	SaveTask (Worker& worker, CStringPtr filename, FileStorageContext::Mode mode, NotifyEntry* entry = nullptr)
	: BackgroundTask (entry), // entry used as task id
	  worker (worker),
	  filename (filename),
	  mode (mode),
	  entry (entry)
	{}
};

//************************************************************************************************
// FileIOManager::DataSaveTask
//************************************************************************************************

struct FileIOManager::DataSaveTask: public FileIOManager::SaveTask
{
	IO::MemoryStream* data;
	
	DataSaveTask (Worker& worker, CStringPtr filename, IO::MemoryStream* data, FileStorageContext::Mode mode, NotifyEntry* entry = nullptr)
	: SaveTask (worker, filename, mode, entry),
	  data (data)
	{}
		
	~DataSaveTask ()
	{
		delete data;
	}
	
	// SaveTask
	void work () override
	{
		worker.saveFile (filename, *data, mode);
		if(entry)
			entry->completed = true;
	}
};

//************************************************************************************************
// FileIOManager::PromiseSaveTask
//************************************************************************************************

struct FileIOManager::PromiseSaveTask: public FileIOManager::SaveTask
{
	FileDataPromise* promise;
	
	PromiseSaveTask (Worker& worker, CStringPtr filename, FileDataPromise* promise, FileStorageContext::Mode mode, NotifyEntry* entry = nullptr)
	: SaveTask (worker, filename, mode, entry),
	  promise (promise)
	{}
		
	~PromiseSaveTask ()
	{
		delete promise;
	}
	
	// SaveTask
	void work () override
	{
		IO::MemoryStream* data = promise->createFileData ();
		Deleter<IO::MemoryStream> dataDeleter (data);
		if(data)
			worker.saveFile (filename, *data, mode);
		if(entry)
			entry->completed = true;
	}
};

//************************************************************************************************
// FileIOManager::LoadTask
//************************************************************************************************

struct FileIOManager::LoadTask: public BackgroundTask
{
	Worker& worker;
	FileStorageContext::Mode mode;
	NotifyEntry& entry;
	
	LoadTask (Worker& worker, FileStorageContext::Mode mode, NotifyEntry& entry)
	: BackgroundTask (&entry), // entry used as task id
	  worker (worker),
	  mode (mode),
	  entry (entry)
	{}
	
	// BackgroundTask
	void work () override
	{
		entry.data = worker.loadFile (entry.filename, mode);
		if(entry.handler)
			entry.handler->onLoadFileCompletedAsync (entry.data, entry.filename);
		entry.completed = true;
	}
};

//************************************************************************************************
// FileIOManager::ExternalTask
//************************************************************************************************

struct FileIOManager::ExternalTask: BackgroundTask
{
	NotifyEntry* entry;
	BackgroundTask* externalTask;

	ExternalTask (NotifyEntry* entry, BackgroundTask* externalTask)
	: BackgroundTask (entry), // entry used as task id
	  entry (entry),
	  externalTask (externalTask)
	{}

	~ExternalTask ()
	{
		delete externalTask;
	}

	// BackgroundTask
	void cancel () override
	{
		externalTask->cancel ();
	}
	
	void work () override
	{
		externalTask->work ();
		if(entry)
			entry->completed = true;
	}
};

//************************************************************************************************
// FileIOManager
//************************************************************************************************

DEFINE_STATIC_SINGLETON (FileIOManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileIOManager::FileIOManager ()
: worker (NEW Worker)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileIOManager::~FileIOManager ()
{
	delete worker;

	ASSERT (notificationQueue.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileIOManager::setPriority (Threads::ThreadPriority priority)
{
	worker->setPriority (priority);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileIOManager::NotifyEntry* FileIOManager::addNotifyEntry (NotifyEntry::Type type, CStringPtr filename, FileIOCompletionHandler* handler)
{
	NotifyEntry* entry = NEW NotifyEntry (type);
	entry->filename = filename;
	entry->handler = handler;
	notificationQueue.append (entry);
	return entry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileIOTaskID FileIOManager::addSaveTask (CStringPtr filename, IO::MemoryStream* data, FileIOCompletionHandler* completionHandler, StorageMode mode)
{
	NotifyEntry* entry = nullptr;
	if(completionHandler)
		entry = addNotifyEntry (NotifyEntry::kSave, filename, completionHandler);

	worker->addTask (NEW DataSaveTask (*worker, filename, data, mode, entry));
	return entry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileIOTaskID FileIOManager::addSaveTask (CStringPtr filename, FileDataPromise* promise, FileIOCompletionHandler* completionHandler, StorageMode mode)
{
	NotifyEntry* entry = nullptr;
	if(completionHandler)
		entry = addNotifyEntry (NotifyEntry::kSave, filename, completionHandler);

	worker->addTask (NEW PromiseSaveTask (*worker, filename, promise, mode, entry));
	return entry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileIOTaskID FileIOManager::addLoadTask (CStringPtr filename, FileIOCompletionHandler* completionHandler, StorageMode mode)
{
	NotifyEntry* entry = addNotifyEntry (NotifyEntry::kLoad, filename, completionHandler);

	worker->addTask (NEW LoadTask (*worker, mode, *entry));
	return entry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileIOTaskID FileIOManager::addExternalTask (BackgroundTask* task, CStringPtr filename, FileIOCompletionHandler* completionHandler, bool isSaveTask)
{
	NotifyEntry* entry = nullptr;
	if(completionHandler)
		entry = addNotifyEntry (isSaveTask ? NotifyEntry::kSave : NotifyEntry::kLoad, filename, completionHandler);

	worker->addTask (NEW ExternalTask (entry, task));
	return entry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileIOManager::cancelTask (FileIOTaskID id)
{
	NotifyEntry* entry = reinterpret_cast<NotifyEntry*> (id);
	ASSERT (entry != nullptr)
	if(entry == nullptr)
		return;

	int result = worker->cancelTask (entry);
	#if (0 && DEBUG)
	DebugPrintf ("FileIOManager::cancelTask result = %d\n", result);
	#endif
	if(result == BackgroundWorker::kCancelNotFound)
		return; // not expected, is the id stale?

	// Make sure not to delete a notification entry still being referenced by the worker!
	entry->canceled = true; // will be deleted on next idle (or when completed)
	if(result == BackgroundWorker::kCancelDone)
		entry->completed = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileIOManager::idle ()
{
	if(!notificationQueue.isEmpty ())
		IntrusiveListForEach (notificationQueue, NotifyEntry, e)
			if(e->canceled)
				e->handler->onCancel ();
	
			if(e->completed)
			{
				if(!e->canceled)
				{
					switch(e->type)
					{
					case NotifyEntry::kLoad :
						e->handler->onLoadFileCompleted (e->data, e->filename);
						break;
					case NotifyEntry::kSave :
						e->handler->onSaveFileCompleted (e->filename);
						break;
					}
				}

				notificationQueue.remove (e);
				delete e;
			}
		EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileIOManager::terminate ()
{
	worker->terminate ();

	while(NotifyEntry* e = notificationQueue.removeFirst ())
		delete e;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileIOManager::hasTasks () const
{
	return notificationQueue.isEmpty () == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int FileIOManager::countTasks () const
{
	return notificationQueue.count ();
}
