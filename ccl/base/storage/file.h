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
// Filename    : ccl/base/storage/file.h
// Description : File class
//
//************************************************************************************************

#ifndef _ccl_file_h
#define _ccl_file_h

#include "ccl/base/storage/url.h"

#include "ccl/public/system/ifileitem.h"
#include "ccl/public/system/isearcher.h"
#include "ccl/public/system/inativefilesystem.h"

namespace CCL {

class StringList;
interface IStorable;

//************************************************************************************************
// File
//************************************************************************************************

class File: public Object
{
public:
	DECLARE_CLASS (File, Object)
	DECLARE_PROPERTY_NAMES (File)
	DECLARE_METHOD_NAMES (File)

	File ();
	File (UrlRef path);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Helpers
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get the file system (same as System::GetFileSystem()). */
	static INativeFileSystem& getFS ();

	/** Load file into memory. */
	static IMemoryStream* loadBinaryFile (UrlRef path, IFileSystem* fileSystem = nullptr);

	/** Save memory stream to file. */
	static bool save (UrlRef path, const IMemoryStream& stream);

	/** Save storable object as file. */
	static bool save (UrlRef path, const IStorable& storable);

	/** Load file into storable object. */
	static bool load (UrlRef path, IStorable& storable);

	/** Notify the application about a filesystem change (e.g. kFileCreated, kFileRemoved). */
	static void signalFile (StringID signalID, UrlRef path);

	/** Returns iterator of files matching a search pattern in the specified path. */
	static IFileIterator* findFiles (UrlRef path, StringRef searchPattern, int mode = IFileIterator::kAll);

	/** Create filtered file iterator. */
	static IFileIterator* filterIterator (IFileIterator& iter, const IUrlFilter& filter);

	/** Check if folder is empty. */
	static bool isFolderEmpty (UrlRef path);

	/** Copy folder. */
	static bool copyFolder (UrlRef dstPath, UrlRef srcPath, const IUrlFilter* filter = nullptr, bool recursive = false);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Path
	//////////////////////////////////////////////////////////////////////////////////////////////

	PROPERTY_OBJECT (Url, path, Path)

	bool isFile () const	{ return path.isFile (); }
	bool isFolder () const	{ return path.isFolder (); }

	void signalRelease ()	{ signalFile (Signals::kReleaseFile, path); }
	void signalCreated ()	{ signalFile (Signals::kFileCreated, path); }
	void signalRemoved ()	{ signalFile (Signals::kFileRemoved, path); }

	//////////////////////////////////////////////////////////////////////////////////////////////
	// IFileSystem methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** @see IFileSystem::fileExists() */
	bool exists () const;

	/** @see IFileSystem::createFolder() or IFileSystem::openStream() */
	bool create () const;

	/** @see IFileSystem::removeFile() or IFileSystem::removeFolder() */
	bool remove (int mode = 0) const;

	/** @see IFileSystem::renameFile() */
	bool rename (StringRef newName, int mode = 0);

	/** @see IFileSystem::getFileInfo() */
	bool getInfo (FileInfo& info) const;

	/** @see IFileSystem::openStream() */
	IStream* open (int mode = IStream::kOpenMode, IUnknown* context = nullptr) const;

	/** @see IFileSystem::newIterator() */
	IFileIterator* newIterator (int mode = IFileIterator::kAll) const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// IVolumeFileSystem methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** @see IVolumeFileSystem::getVolumeInfo() */
	bool getVolumeInfo (VolumeInfo& info) const;

	/** @see IVolumeFileSystem::isLocalFile() */
	bool isLocal () const;

	/** @see IVolumeFileSystem::isHiddenFile() */
	bool isHidden () const;

	/** @see IVolumeFileSystem::isWriteProtected() */
	bool isWriteProtected () const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// INativeFileSystem methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** @see INativeFileSystem::moveFile() */
	bool moveTo (UrlRef dstPath, int mode = 0, IProgressNotify* progress = nullptr);

	/** @see INativeFileSystem::copyFile() */
	bool copyTo (UrlRef dstPath, int mode = 0, IProgressNotify* progress = nullptr) const;

	/** @see INativeFileSystem::setFileTime() */
	bool setTime (const FileTime& modifiedTime) const;

protected:
	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// TempFile
//************************************************************************************************

class TempFile: public File
{
public:
	/** Create empty temp file. */
	TempFile (StringRef fileName);

	/** Create temp file with data. */
	TempFile (IStream& data, StringRef fileName);

	/** Delete temp file. */
	~TempFile ();
};

//************************************************************************************************
// LockFile
//************************************************************************************************

namespace LockFile
{
	bool lockDirectory (UrlRef path, StringRef applicationName = nullptr);
	bool unlockDirectory (UrlRef path);
	bool isDirectoryLocked (UrlRef path);
	void getLockingApplicationNames (StringList& applicationNames, UrlRef path);
	const FileType& getLockFileType ();
}

//************************************************************************************************
// ScopedLockFile
//************************************************************************************************

class ScopedLockFile
{
public:
	ScopedLockFile (UrlRef path, StringRef applicationName = nullptr, bool enable = true);
	~ScopedLockFile ();

	bool isLocked () const;

private:
	Url path;
	bool enabled;
};

//************************************************************************************************
// FileDescriptor
//************************************************************************************************

class FileDescriptor: public Object,
					  public IFileDescriptor
{
public:
	DECLARE_CLASS (FileDescriptor, Object)

	FileDescriptor (StringRef fileName = nullptr, int64 fileSize = -1);
	~FileDescriptor ();

	PROPERTY_STRING (fileName, FileName)
	PROPERTY_STRING (explicitTitle, ExplicitTitle)
	PROPERTY_VARIABLE (int64, fileSize, FileSize)
	PROPERTY_OBJECT (DateTime, fileTime, FileTime)
	Attributes& getMetaInfo ();

	// IFileDescriptor
	tbool CCL_API getTitle (String& title) const override;
	tbool CCL_API getFileName (String& fileName) const override;
	tbool CCL_API getFileType (FileType& fileType) const override;
	tbool CCL_API getFileSize (int64& fileSize) const override;
	tbool CCL_API getFileTime (DateTime& fileTime) const override;
	tbool CCL_API getMetaInfo (IAttributeList& a) const override;

	CLASS_INTERFACE (IFileDescriptor, Object)

protected:
	Attributes* metaInfo;
};

//************************************************************************************************
// SearchDescription
//************************************************************************************************

class SearchDescription: public Object,
						 public ISearchDescription
{
public:
	DECLARE_CLASS_ABSTRACT (SearchDescription, Object)
	
	static SearchDescription* create (UrlRef startPoint, StringRef searchTerms, int options = 0, StringRef delimiter = String::kEmpty);

	void setStartPoint (UrlRef url)	{ startPoint = url; }
	void setPaginationOffset (int offset) { paginationOffset = offset; }

	// ISearchDescription
	UrlRef CCL_API getStartPoint () const override { return startPoint; }
	StringRef CCL_API getSearchTerms () const override { return searchTerms; }
	tbool CCL_API matchesName (StringRef name) const override;
	int CCL_API getPaginationOffset () const override { return paginationOffset; }
	int CCL_API getOptions () const override { return options; }
	int CCL_API getSearchTokenCount () const override { return 0; }
	StringRef CCL_API getSearchToken (int index) const override { return String::kEmpty; }
	StringRef CCL_API getTokenDelimiter () const override { return String::kEmpty; }

	CLASS_INTERFACE (ISearchDescription, Object)

protected:
	Url startPoint;
	String searchTerms;
	int options;
	int paginationOffset;

	SearchDescription (UrlRef startPoint, StringRef searchTerms, int options)
	: startPoint (startPoint),
	  searchTerms (searchTerms),
	  options (options),
	  paginationOffset (0)
	{}

	static void removeDelimiters (String& string);
};

} // namespace CCL

#endif // _ccl_file_h
