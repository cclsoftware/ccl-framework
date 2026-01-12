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
// Filename    : core/portable/corefile.h
// Description : File System
//
//************************************************************************************************

#ifndef _corefilesystem_h
#define _corefilesystem_h

#include "core/platform/corefeatures.h"

#if CORE_FILESYSTEM_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
	#include CORE_PLATFORM_IMPLEMENTATION_HEADER (corefilesystem)
#elif CORE_FILESYSTEM_IMPLEMENTATION == CORE_EXTERNAL_PLATFORM_IMPLEMENTATION
	#include CORE_EXTERNAL_PLATFORM_IMPLEMENTATION_HEADER (corefilesystem)
#elif CORE_FILESYSTEM_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
	#include "core/platform/shared/posix/corefilesystem.posix.h"
#elif CORE_FILESYSTEM_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED
	#include "core/platform/shared/coreplatformfilesystem.h"
#endif

#include "core/public/corememstream.h"
#include "core/public/coreintrusivelist.h"
#include "core/public/corevector.h"
#include "core/public/corethreading.h"
#include "core/portable/coresingleton.h"

namespace Core {
namespace Portable {

struct BackgroundTask;

//************************************************************************************************
// FileIterator
/** File iterator class usually created on stack with a given directory name. 
	\ingroup core_portable */
//************************************************************************************************

class FileIterator
{
public:
	FileIterator (CStringPtr dirname);

	/** File entry. */
	typedef FindFileData Entry;
	
	/** Advance iterator to next file.
		\return current file or null when finished
	*/
	const Entry* next ();

protected:
	Platform::FileIterator platformIterator;
};

//************************************************************************************************
// FileUtils
/** File utility functions. 
	\ingroup core_portable */
//************************************************************************************************

class FileUtils
{
public:
	/** Get path to temporary directory. */
	static void getTempDir (FileName& dirname);

	/** Get path to application data directory (per user or shared). */
	static void getDataDir (FileName& dirname, bool shared = false);

	/** Get path to applications directory (per user or shared). */
	static void getAppDir (FileName& dirname, bool shared = false);

	/** Get path to application support directory (per user or shared). */
	static void getAppSupportDir (FileName& dirname, bool shared = false);
	
	/** Get path to user home directory. */
	static void getHomeDir (FileName& dirname);

	/** Get current working directory. */
	static void getWorkingDir (FileName& dirname);

	/** Create new directory. */
	static bool	makeDirectory (CStringPtr dirname);
	
	/** Check if given file exists. */
	static bool	fileExists (CStringPtr filename);

	/** Check if given directory exists. */
	static bool dirExists (CStringPtr dirname);
	
	/** Get timestamp of last file modification. */
	static int64 fileLastModified (CStringPtr filename);
	
	/** Delete given file. */
	static bool	deleteFile (CStringPtr filename);
	
	/** Remove directory. */
	static bool removeDirectory (CStringPtr dirname);
	
	/** Remove directory tree and all contained files/directories. Potentially very dangerous! Be careful... */
	static bool removeDirectoryTree (CStringPtr dirname);
	
	/** Rename file. */
	static bool	renameFile (CStringPtr oldname, CStringPtr newname);
	
	/** Truncate file to given length. */
	static bool	truncate (CStringPtr filename, Core::int64 length);

	/** Load file contents into new memory stream instance owned by the caller. */
	static IO::MemoryStream* loadFile (CStringPtr filename);

	/** Save file contents from memory to disk in a single write operation. */
	static bool saveFile (CStringPtr filename, const IO::MemoryStream& data);

	/** Change access and modification time to current. */
	static bool touchFile (CStringPtr filename);
	
	/** Copy file. */
	static bool copyFile (CStringPtr source, CStringPtr destination);

	/** Copy directory recursively. Files that exist in both source tree and destination get overridden, files only existant in destination remain untouched. */
	static bool copyDirectoryTree (CStringPtr source, CStringPtr destination);
};

//************************************************************************************************
// FilePackage
/**	Represents a logical collection of files.
	This could be a folder on disk or some other implementation. */
//************************************************************************************************

class FilePackage
{
public:
	virtual ~FilePackage () {}
	
	/** Check if file exists. */
	virtual bool fileExists (CStringPtr fileName) = 0;

	/** Open file for reading. */
	virtual IO::Stream* openStream (CStringPtr fileName) = 0;
};

//************************************************************************************************
// FolderPackage
/**	Files in a local folder. */
//************************************************************************************************

class FolderPackage: public FilePackage
{
public:
	FolderPackage (CStringPtr baseFolder, bool bufferedMode = false);

	// FilePackage
	bool fileExists (CStringPtr fileName) override;
	IO::Stream* openStream (CStringPtr fileName) override;

protected:
	FileName baseFolder;
	bool bufferedMode;
};

//************************************************************************************************
// SubPackage
/** Sub-part of another package. */
//************************************************************************************************

class SubPackage: public FilePackage
{
public:
	SubPackage (FilePackage& parent, CStringPtr baseFolder);

	// FilePackage
	bool fileExists (CStringPtr fileName) override;
	IO::Stream* openStream (CStringPtr fileName) override;

protected:
	FilePackage& parent;
	FileName baseFolder;
};

//************************************************************************************************
// ZipPackage
/**	ZIP package. */
//************************************************************************************************

class ZipPackage: public FilePackage
{
public:
	ZipPackage ();
	~ZipPackage ();

	bool openFromFile (CStringPtr fileName, bool bufferedMode = false);
	bool openFromMemory (const void* data, uint32 size); ///< does not copy memory!
	void close ();

	uint32 getStreamSize (CStringPtr fileName) const; // uncompressed
	IO::Stream* openFirstStream (uint32& uncompressedSize);

	// FilePackage
	bool fileExists (CStringPtr fileName) override;
	IO::Stream* openStream (CStringPtr fileName) override;

protected:
	IO::Stream* file;
	IO::Buffer fileBuffer;

	struct Entry
	{
		FileName name;
		uint32 localHeaderOffset;
		uint32 compressedSize;
		uint32 uncompressedSize;

		Entry (CStringPtr name = nullptr)
		: name (name),
		  localHeaderOffset (0),
		  compressedSize (0),
		  uncompressedSize (0)
		{}

		bool operator > (const Entry& other) const
		{
			return name.compare (other.name, false) > 0;
		}

		bool operator == (const Entry& other) const
		{
			return name.compare (other.name, false) == 0;
		}
	};

	Vector<Entry> entries;

	bool readFormat ();
	const Entry* findEntry (CStringPtr fileName) const;
	IO::Stream* openEntry (const Entry& entry);
};

//************************************************************************************************
// FileStream
/** Stream representing a file on disk. 
	\ingroup core_portable */
//************************************************************************************************

class FileStream: public IO::Stream
{
public:
	FileStream (FILE* file = nullptr);

	/** Open file for binary reading and/or writing. */
	bool open (CStringPtr filename, int mode = IO::kReadMode|IO::kWriteMode);
	
	/** Create new file. */
	bool create (CStringPtr filename);
	
	/** Close file. */
	void close ();

	/** Check if file is currently open. */
	bool isOpen () const;

	/** Get current size of file in bytes. */
	int64 getFileSize ();

	// Stream
	int64 getPosition () override;
	int64 setPosition (int64 pos, int mode) override;
	int readBytes (void* buffer, int size) override;
	int writeBytes (const void* buffer, int size) override;

protected:
	Platform::FileStream platformStream;
};

//************************************************************************************************
// FileStorageContext
/** Load/save files with optional compression.
	\ingroup core_portable */
//************************************************************************************************

class FileStorageContext
{
public:
	enum Mode
	{
		kCopy,			///< load/save file as is (copy data only)
		kCompress,		///< load plain file and compress into RAM / save data compressed to disk
		kDecompress		///< load compressed file and decompress into RAM / decompress data and save plain to disk
	};

	static FileStorageContext* create (); ///< create new instance, use one per thread!
	static FileStorageContext& getMainThreadInstance (); ///< shared instance for main thread

	virtual ~FileStorageContext () {}
	class Implementation;

	virtual IO::MemoryStream* loadFile (CStringPtr filename, Mode mode, int streamSizeEstimate = 0) = 0;
	virtual bool saveFile (CStringPtr filename, const IO::MemoryStream& data, Mode mode) = 0;

	virtual bool compress (IO::Stream& compressedData, const IO::MemoryStream& data) = 0;
	virtual bool decompress (IO::Stream& plainData, const IO::MemoryStream& data) = 0;
};

//************************************************************************************************
// FileDataPromise
/** Promise for a file that's created on demand. Can be used to defer data serialization
	to a background thread. */
//************************************************************************************************

struct FileDataPromise
{
	virtual ~FileDataPromise () {}
	virtual IO::MemoryStream* createFileData () = 0;
};

//************************************************************************************************
// FileIOCompletionHandler
/** Completion handler for asynchronous file operations. 
	\ingroup core_portable */
//************************************************************************************************

struct FileIOCompletionHandler
{
	virtual ~FileIOCompletionHandler () {}

	virtual void onSaveFileCompleted (CStringPtr filename) {}
	
	virtual void onLoadFileCompleted (IO::MemoryStream* data, CStringPtr filename) {} ///< called from main thread
	
	virtual void onLoadFileCompletedAsync (IO::MemoryStream* data, CStringPtr filename) {} ///< called from background thread

	virtual void onCancel () {}
};

/** File I/O task identifer used for cancelation. */
typedef void* FileIOTaskID;

//************************************************************************************************
// FileIOManager
/**	Manager singleton for asynchronous file operations. 
	Make sure to call idle and terminate in your application!
	\ingroup core_portable */
//************************************************************************************************

class FileIOManager: public StaticSingleton<FileIOManager>
{
public:
	FileIOManager ();
	~FileIOManager ();

	typedef FileStorageContext::Mode StorageMode;

	/** Set file I/O thread priority (default is low). */
	void setPriority (Threads::ThreadPriority priority);

	/** Save data from memory stream to disk asynchronously. */
	FileIOTaskID addSaveTask (CStringPtr filename, IO::MemoryStream* data, FileIOCompletionHandler* completionHandler = nullptr,
							  StorageMode mode = FileStorageContext::kCopy);
	
	/** Save data provided by a promise object to disk asynchronously. */
	FileIOTaskID addSaveTask (CStringPtr filename, FileDataPromise* promise, FileIOCompletionHandler* completionHandler = nullptr,
							  StorageMode mode = FileStorageContext::kCopy);

	/** Load data from disk into memory stream asynchronously. */
	FileIOTaskID addLoadTask (CStringPtr filename, FileIOCompletionHandler* completionHandler, 
							  StorageMode mode = FileStorageContext::kCopy);
	
	/** Add external background task. Use if you need special behavior not implemented by this class. */
	FileIOTaskID addExternalTask (BackgroundTask* task, CStringPtr filename = nullptr, FileIOCompletionHandler* completionHandler = nullptr, bool isSaveTask = true);

	/** Cancel queued background task. */
	void cancelTask (FileIOTaskID id);

	/** Idle needs to be called periodically by the application. */
	void idle ();
	
	/** Terminate needs to be called once on application exit. */
	void terminate ();
	
	/** Check if there are tasks in the queue. */
	bool hasTasks () const;
	
	/** Count the tasks in the queue. */
	int countTasks () const;

protected:
	struct SaveTask;
	struct DataSaveTask;
	struct PromiseSaveTask;
	struct LoadTask;
	struct ExternalTask;
	
	struct NotifyEntry: IntrusiveLink<NotifyEntry>
	{
		enum Type { kLoad, kSave, kCanceled };
		Type type;
		FileName filename;
		FileIOCompletionHandler* handler;
		IO::MemoryStream* data;
		bool completed;
		bool canceled;

		NotifyEntry (Type type)
		: type (type),
		  handler (nullptr),
		  data (nullptr),
		  completed (false),
		  canceled (false)
		{}

		~NotifyEntry ()
		{
			delete data;
			delete handler;
		}
	};

	class Worker;
	Worker* worker;
	IntrusiveLinkedList<NotifyEntry> notificationQueue;

	NotifyEntry* addNotifyEntry (NotifyEntry::Type type, CStringPtr filename, FileIOCompletionHandler* handler);
};

//************************************************************************************************
// FileIterator implementation
//************************************************************************************************

inline FileIterator::FileIterator (CStringPtr dirname)
: platformIterator (dirname)
{}

inline const FileIterator::Entry* FileIterator::next ()
{ return platformIterator.next (); }

//************************************************************************************************
// FileUtils implementation
//************************************************************************************************

inline void FileUtils::getTempDir (FileName& dirname)
{ return Platform::FileSystem::instance ().getDirectory (dirname, Platform::IFileSystem::kTempDir); }

inline void FileUtils::getDataDir (FileName& dirname, bool shared)
{ return Platform::FileSystem::instance ().getDirectory (dirname, shared ? Platform::IFileSystem::kSharedDataDir : Platform::IFileSystem::kDataDir); }

inline void FileUtils::getAppDir (FileName& dirname, bool shared)
{ return Platform::FileSystem::instance ().getDirectory (dirname, shared ? Platform::IFileSystem::kSharedAppDir : Platform::IFileSystem::kAppDir); }

inline void FileUtils::getAppSupportDir (FileName& dirname, bool shared)
{ return Platform::FileSystem::instance ().getDirectory (dirname, shared ? Platform::IFileSystem::kSharedAppSupportDir : Platform::IFileSystem::kAppSupportDir); }

inline void FileUtils::getHomeDir (FileName& dirname)
{ return Platform::FileSystem::instance ().getDirectory (dirname, Platform::IFileSystem::kHomeDir); }

inline void FileUtils::getWorkingDir (FileName& dirname)
{ return Platform::FileSystem::instance ().getDirectory (dirname, Platform::IFileSystem::kWorkingDir); }

inline bool	FileUtils::makeDirectory (CStringPtr dirname)
{ return Platform::FileSystem::instance ().makeDirectory (dirname); }

inline bool	FileUtils::fileExists (CStringPtr filename)
{ return Platform::FileSystem::instance ().fileExists (filename); }

inline bool FileUtils::dirExists (CStringPtr dirname)
{ return Platform::FileSystem::instance ().dirExists (dirname); }

inline int64 FileUtils::fileLastModified (CStringPtr filename)
{ return Platform::FileSystem::instance ().fileLastModified (filename); }

inline bool	FileUtils::deleteFile (CStringPtr filename)
{ return Platform::FileSystem::instance ().deleteFile (filename); }

inline bool FileUtils::removeDirectory (CStringPtr dirname)
{ return Platform::FileSystem::instance ().removeDirectory (dirname); }

inline bool	FileUtils::renameFile (CStringPtr oldname, CStringPtr newname)
{ return Platform::FileSystem::instance ().renameFile (oldname, newname); }

inline bool	FileUtils::truncate (CStringPtr filename, Core::int64 length)
{ return Platform::FileSystem::instance ().truncate (filename, length); }

inline bool FileUtils::touchFile (CStringPtr filename)
{ return Platform::FileSystem::instance ().touchFile (filename); }

//************************************************************************************************
// FileStream implementation
//************************************************************************************************

inline FileStream::FileStream (FILE* file)
: platformStream (file)
{}

inline int64 FileStream::getPosition ()
{ return platformStream.getPosition (); }

inline int64 FileStream::setPosition (int64 pos, int mode)
{ return platformStream.setPosition (pos, mode); }

inline int FileStream::readBytes (void* buffer, int size)
{ return platformStream.readBytes (buffer, size); }

inline int FileStream::writeBytes (const void* buffer, int size)
{ return platformStream.writeBytes (buffer, size); }

inline bool FileStream::open (CStringPtr filename, int mode)
{ return platformStream.open (filename, mode); }

inline bool FileStream::create (CStringPtr filename)
{ return platformStream.create (filename); }

inline void FileStream::close ()
{ platformStream.close (); }

inline bool FileStream::isOpen () const
{ return platformStream.isOpen (); }

inline int64 FileStream::getFileSize ()
{ return platformStream.getFileSize (); }

} // namespace Portable
} // namespace Core

#endif // _corefilesystem_h
