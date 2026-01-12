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
// Filename    : ccl/network/webfs/webfilesystem.h
// Description : Web File System
//
//************************************************************************************************

#ifndef _ccl_webfilesystem_h
#define _ccl_webfilesystem_h

#include "ccl/base/objectnode.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/protocolhandler.h"

#include "ccl/public/system/ilockable.h"
#include "ccl/public/system/ifileitem.h"
#include "ccl/public/system/inativefilesystem.h"

#include "ccl/public/network/web/iwebclient.h"
#include "ccl/public/network/web/iwebfileclient.h"
#include "ccl/public/network/web/iwebcredentials.h"

namespace CCL {

interface IFileDescriptor;

namespace Web {

class VolumeHandler;
class FileSystem;

//************************************************************************************************
// DirectoryEntry
//************************************************************************************************

class DirectoryEntry: public ObjectNode
{
public:
	DECLARE_CLASS (DirectoryEntry, ObjectNode)

	DirectoryEntry ();

	PROPERTY_OBJECT (FileTime, creationTime, CreationTime)
	PROPERTY_OBJECT (FileTime, modifiedTime, ModifiedTime)
	PROPERTY_STRING (contentType, ContentType)
	PROPERTY_VARIABLE (int64, contentLength, ContentLength)
	PROPERTY_BOOL (directory, Directory)
	PROPERTY_VARIABLE (int, flags, Flags)

	PROPERTY_SHARED_AUTO (IUnknown, object, Object)

	PROPERTY_BOOL (cached, Cached)

	void assign (const IWebFileClient::DirEntry& entry, IUnknown* object);
	void assignSearchResult (UrlRef webfsUrl, const IWebFileSearchClient::ResultEntry& entry, IUnknown* object);
};

//************************************************************************************************
// Volume
//************************************************************************************************

class Volume: public MountPoint
{
public:
	DECLARE_CLASS (Volume, MountPoint)

	Volume (StringRef name = nullptr);
	~Volume ();

	FileSystem* getFs () const;

	void setName (StringRef name);
	PROPERTY_BOOL (hidden, Hidden)
	PROPERTY_STRING (type, Type)
	PROPERTY_VARIABLE (int, flags, Flags)
	PROPERTY_VARIABLE (int64, bytesTotal, BytesTotal)       ///< total size in bytes, 0 means unknown
	PROPERTY_VARIABLE (int64, bytesFree, BytesFree)         ///< number of bytes free, 0 means unknown
	PROPERTY_STRING (label, Label)
	PROPERTY_OBJECT (Url, serverUrl, ServerUrl)
	PROPERTY_SHARED_AUTO (IWebCredentials, credentials, Credentials)
	PROPERTY_SHARED_AUTO (DirectoryEntry, rootDir, RootDirectory)
	PROPERTY_SHARED_AUTO (IWebFileClient, client, Client)
	PROPERTY_SHARED_AUTO (IUnknown, serverHandler, ServerHandler)

	bool canConnect () const;
	bool isEqual (UrlRef serverUrl, IWebCredentials* credentials, bool exact) const;

	IWebFileClient* connect ();									///< ensure that client is connected
	String getRemotePath (UrlRef webfsUrl) const;				///< translate to remote path on server
	void getWebfsUrl (IUrl& webfsUrl, StringRef remotePath, int type = Url::kDetect);	///< translate remote path to WebFS URL
	void getFullUrl (Url& result, UrlRef webfsUrl) const;		///< translate to full URL on server
	String getDisplayString (UrlRef webfsUrl) const;			///< get beautified display string for WebFS URL

protected:
	FileSystem* webFs;
	
	void disconnect ();	///< ensure that client is disconnected
};

//************************************************************************************************
// FileSystem
//************************************************************************************************

class FileSystem: public Unknown,
				  public AbstractFileSystem,
				  public IFileItemProvider
{
public:
	FileSystem (Volume& volume);

	class Operation;

	PROPERTY_SHARED_AUTO (Threading::ILockable, lock, Lock)

	tresult updateDirectory (UrlRef url, IProgressNotify* progress);
	tresult discardDirectory (UrlRef url);

	tresult insertSearchResult (const IWebFileSearchClient::IResultIterator& iter, IUnknownList* outItems = nullptr);
	tresult insertSearchResult (const IWebFileSearchClient::ResultEntry& entry, IUnknown* object, IUnknown** outItem = nullptr);

	IFileDescriptor* openFileItemInternal (UrlRef url, bool allowDefault);

	// IFileItemProvider
	IFileDescriptor* CCL_API openFileItem (UrlRef url) override;

	// IFileSystem
	tbool CCL_API fileExists (UrlRef url) override;
	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) override;
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;
	tbool CCL_API createFolder (UrlRef url) override;
	tbool CCL_API removeFile (UrlRef url, int mode = 0) override;
	tbool CCL_API removeFolder (UrlRef url, int mode = 0) override;

	CLASS_INTERFACE2 (IFileSystem, IFileItemProvider, Unknown)
	
protected:
	class FileIterator;
	class DefaultDescriptor;

	Volume& volume;

	void addToDirectory (UrlRef url, const IWebFileClient::IDirIterator* iter);
	void addToDirectory (UrlRef url, Container& entries);
	DirectoryEntry* findEntry (UrlRef url);
	DirectoryEntry* createEntry (UrlRef url);
};

//************************************************************************************************
// RootFileSystem
//************************************************************************************************

class RootFileSystem: public Unknown,
					  public IVolumeFileSystem
{
public:
	RootFileSystem (VolumeHandler& handler);

	// IFileSystem
	tbool CCL_API isCaseSensitive () override;
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;

	// IVolumeFileSystem
	tbool CCL_API getVolumeInfo (VolumeInfo& info, UrlRef rootUrl) override;
	tbool CCL_API isLocalFile (UrlRef url) override;
	tbool CCL_API isHiddenFile (UrlRef url) override;
	tbool CCL_API isWriteProtected (UrlRef url) override;
	tbool CCL_API moveFile (UrlRef dstPath, UrlRef srcPath, int mode = 0, IProgressNotify* progress = nullptr) override;
	tbool CCL_API copyFile (UrlRef dstPath, UrlRef srcPath, int mode = 0, IProgressNotify* progress = nullptr) override;

	CLASS_INTERFACE2 (IFileSystem, IVolumeFileSystem, Unknown)

protected:
	class VolumeIterator;
	class VolumeUpdateTask;

	VolumeHandler& handler;

	// not implemented:
	IStream* CCL_API openStream (UrlRef url, int mode = IStream::kOpenMode, IUnknown* context = nullptr) override { return nullptr; }
	tbool CCL_API fileExists (UrlRef url) override { return false; }
	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) override { return false; }
	tbool CCL_API removeFile (UrlRef url, int mode = 0) override { return false; }
	tbool CCL_API renameFile (UrlRef url, StringRef newName, int mode = 0) override { return false; }
	tbool CCL_API createFolder (UrlRef url) override { return false; }
	tbool CCL_API removeFolder (UrlRef url, int mode = 0) override { return false; }
};

//************************************************************************************************
// VolumeHandler
//************************************************************************************************

class VolumeHandler: public MountProtocolHandler
{
public:
	DECLARE_CLASS (VolumeHandler, MountProtocolHandler)

	VolumeHandler ();

	PROPERTY_AUTO_POINTER (Threading::ILockable, lock, Lock)

	void addVolume (Volume* volume, int position = -1);
	bool removeVolume (StringRef name);
	void removeAll ();

	void getVolumeLocations (Container& paths, bool wantHidden);
	bool getVolumeInfo (VolumeInfo& info, StringRef name);

	Volume* openVolume (StringRef name); ///< get a reference for asynchronous access
	Volume* openWithServerUrl (UrlRef serverUrl, IWebCredentials* credentials, bool exact);
	int getVolumePosition (Volume* volume);

	// MountProtocolHandler
	StringRef CCL_API getProtocol () const override;
	IFileSystem* CCL_API getMountPoint (StringRef name) override;

protected:
	AutoPtr<RootFileSystem> rootFs;

	Volume* find (StringRef name) const;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webfilesystem_h
