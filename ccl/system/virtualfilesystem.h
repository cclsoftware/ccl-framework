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
// Filename    : ccl/system/virtualfilesystem.h
// Description : Virtual File System
//
//************************************************************************************************

#ifndef _ccl_virtualfilesystem_h
#define _ccl_virtualfilesystem_h

#include "ccl/base/object.h"

#include "ccl/public/collections/linkedlist.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/iprotocolhandler.h"
#include "ccl/public/system/ifileitem.h"

namespace CCL {

class Url;
class FileProtocolHandler;

//************************************************************************************************
// VirtualFileSystem
/** Virtual file system */
//************************************************************************************************

class VirtualFileSystem: public Object,
						 public INativeFileSystem,
						 public IFileItemProvider,
						 public IProtocolHandlerRegistry
{
public:
	DECLARE_CLASS (VirtualFileSystem, Object)

	VirtualFileSystem ();
	~VirtualFileSystem ();

	static VirtualFileSystem& instance ();

	IFileSystem* getMountPoint (UrlRef url);
	IFileSystem* getProtocolMountPoint (UrlRef url);

	// IProtocolHandlerRegistry
	tresult CCL_API registerProtocol (IProtocolHandler* handler) override;
	tresult CCL_API unregisterProtocol (IProtocolHandler* handler) override;
	IProtocolHandler* CCL_API getHandler (StringRef protocol) const override;

	// IFileItemProvider
	IFileDescriptor* CCL_API openFileItem (UrlRef url) override;

	// IFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode = IStream::kOpenMode, IUnknown* context = nullptr) override;
	tbool CCL_API fileExists (UrlRef url) override;
	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) override;
	tbool CCL_API removeFile (UrlRef url, int mode = 0) override;
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;
	tbool CCL_API createFolder (UrlRef url) override;
	tbool CCL_API removeFolder (UrlRef url, int mode = 0) override;
	tbool CCL_API isCaseSensitive () override;

	// IVolumeFileSystem
	tbool CCL_API getVolumeInfo (VolumeInfo& info, UrlRef rootUrl) override;
	tbool CCL_API isLocalFile (UrlRef url) override;
	tbool CCL_API isHiddenFile (UrlRef url) override;
	tbool CCL_API isWriteProtected (UrlRef url) override;
	tbool CCL_API moveFile (UrlRef dstPath, UrlRef srcPath, int mode = 0, IProgressNotify* progress = nullptr) override;
	tbool CCL_API copyFile (UrlRef dstPath, UrlRef srcPath, int mode = 0, IProgressNotify* progress = nullptr) override;

	// INativeFileSystem
	tbool CCL_API getPathType (int& type, UrlRef baseFolder, StringRef fileName) override;
	tbool CCL_API renameFile (UrlRef url, StringRef newName, int mode = 0) override;
	tbool CCL_API setFileTime (UrlRef url, const FileTime& modifiedTime) override;
	ISearcher* CCL_API createSearcher (ISearchDescription& description) override;
	tbool CCL_API getWorkingDirectory (IUrl& url) override;
	tbool CCL_API setWorkingDirectory (UrlRef url) override;
	tbool CCL_API getFirstError (int& errorCode) override;
	String CCL_API getErrorString (int errorCode) override;
	tbool CCL_API beginTransaction () override;
	tbool CCL_API endTransaction (int mode, IProgressNotify* progress = nullptr) override;

	CLASS_INTERFACES (Object)

protected:
	LinkedList<IProtocolHandler*> protocols;
	FileProtocolHandler* fileProtocolHandler;

	INativeFileSystem& getNativeFileSystem ();
};

//************************************************************************************************
// RelativeFileSystem
/** File system translating relative URLs */
//************************************************************************************************

class RelativeFileSystem: public Object,
						  public IFileSystem
{
public:
	DECLARE_CLASS (RelativeFileSystem, Object)

	RelativeFileSystem (IFileSystem* fileSys = nullptr, IUrl* baseUrl = nullptr);
	~RelativeFileSystem ();

	virtual IUrl* translateUrl (UrlRef url) const;

	// IFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode = IStream::kOpenMode, IUnknown* context = nullptr) override;
	tbool CCL_API fileExists (UrlRef url) override;
	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) override;
	tbool CCL_API removeFile (UrlRef url, int mode = 0) override;
	tbool CCL_API renameFile (UrlRef url, StringRef newName, int mode = 0) override;
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;
	tbool CCL_API createFolder (UrlRef url) override;
	tbool CCL_API removeFolder (UrlRef url, int mode = 0) override;
	tbool CCL_API isCaseSensitive () override;

	CLASS_INTERFACE (IFileSystem, Object)

protected:
	IFileSystem* fileSys;
	IUrl* baseUrl;
};

//************************************************************************************************
// RelativeFileIterator
/** Relative file iterator */
//************************************************************************************************

class RelativeFileIterator: public Object,
							public IFileIterator
{
public:
	RelativeFileIterator (IFileIterator& iter, UrlRef outDir);
	~RelativeFileIterator ();

	// IFileIterator
	const IUrl* CCL_API next () override;

	CLASS_INTERFACE (IFileIterator, Object)

protected:
	IFileIterator& iter;
	Url& outDir;
	Url& current;
};

//************************************************************************************************
// ResourceFileSystem
/** File system class for module resources. */
//************************************************************************************************

class ResourceFileSystem: public Unknown, 
						  public AbstractFileSystem
{
public:
	static ResourceFileSystem& instance ();

	CLASS_INTERFACE (IFileSystem, Unknown)
};

//************************************************************************************************
// SymbolicFileSystem
/** File system with symbolic path identifiers. */
//************************************************************************************************

class SymbolicFileSystem: public Unknown,
						  public AbstractFileSystem
{
public:
	// IFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode, IUnknown* context) override;
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;
	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) override;
	tbool CCL_API fileExists (UrlRef url) override;
	tbool CCL_API removeFile (UrlRef url, int mode = 0) override;
	tbool CCL_API removeFolder (UrlRef url, int mode = 0) override;

	CLASS_INTERFACE (IFileSystem, Unknown)

protected:
	bool resolve (Url& resolved, UrlRef url) const;
};

} // namespace CCL

#endif // _ccl_virtualfilesystem_h
