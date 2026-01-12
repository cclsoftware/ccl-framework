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
// Filename    : ccl/system/virtualfilesystem.cpp
// Description : Virtual File System
//
//************************************************************************************************

#include "ccl/system/virtualfilesystem.h"
#include "ccl/system/nativefilesystem.h"
#include "ccl/system/memoryfilesystem.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/protocolhandler.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/isearcher.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// FileProtocolHandler
//************************************************************************************************

class FileProtocolHandler: public ProtocolHandler
{
public:
	FileProtocolHandler ()
	: fileSystem (NativeFileSystem::instance ())
	{}

	StringRef CCL_API getProtocol () const override
	{
		static const String fileProtocol = CCLSTR ("file");
		return fileProtocol;
	}

	IFileSystem* CCL_API getMountPoint (StringRef name) override
	{
		return &fileSystem;
	}

protected:
	NativeFileSystem& fileSystem;
};

//************************************************************************************************
// ResourceProtocolHandler
//************************************************************************************************

class ResourceProtocolHandler: public ProtocolHandler
{
public:
	ResourceProtocolHandler ()
	: fileSystem (ResourceFileSystem::instance ())
	{}

	StringRef CCL_API getProtocol () const override
	{
		static const String resourceProtocol = CCLSTR ("resource");
		return resourceProtocol;
	}

	IFileSystem* CCL_API getMountPoint (StringRef name) override
	{
		return &fileSystem;
	}

protected:
	ResourceFileSystem& fileSystem;
};

//************************************************************************************************
// SymbolProtocolHandler
//************************************************************************************************

class SymbolProtocolHandler: public ProtocolHandler
{
public:
	StringRef CCL_API getProtocol () const override
	{
		static const String symbolProtocol = CCLSTR ("local");
		return symbolProtocol;
	}

	IFileSystem* CCL_API getMountPoint (StringRef name) override
	{
		return &fileSystem;
	}

protected:
	SymbolicFileSystem fileSystem;
};

//************************************************************************************************
// MemoryProtocolHandler
//************************************************************************************************

class MemoryProtocolHandler: public ProtocolHandler
{
public:
	StringRef CCL_API getProtocol () const override
	{
		static const String memoryProtocol = CCLSTR ("memory");
		return memoryProtocol;
	}

	IFileSystem* CCL_API getMountPoint (StringRef name) override
	{
		return &fileSystem;
	}

protected:
	MemoryFileSystem fileSystem;
};

//************************************************************************************************
// FileSearcher
//************************************************************************************************

class FileSearcher: public Unknown,
					public ISearcher
{
public:
	FileSearcher (VirtualFileSystem& fileSystem, ISearchDescription& description);
	~FileSearcher ();

	// ISearcher
	tresult CCL_API find (ISearchResultSink& resultSink, IProgressNotify* progress) override;

	CLASS_INTERFACE (ISearcher, Unknown)

protected:
	VirtualFileSystem& fileSystem;
	ISearchDescription& description;

	tresult findInFolder (UrlRef folder, ISearchResultSink& resultSink, IProgressNotify* progress);
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

static VirtualFileSystem theFileSystem;

VirtualFileSystem& VirtualFileSystem::instance ()
{
	return theFileSystem;
}

CCL_EXPORT INativeFileSystem& CCL_API System::CCL_ISOLATED (GetFileSystem) ()
{
	return theFileSystem;
}

//************************************************************************************************
// VirtualFileSystem
//************************************************************************************************

DEFINE_CLASS (VirtualFileSystem, Object)
DEFINE_CLASS_NAMESPACE (VirtualFileSystem, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

VirtualFileSystem::VirtualFileSystem ()
: fileProtocolHandler (nullptr)
{
	AutoPtr<FileProtocolHandler> fileHandler = NEW FileProtocolHandler;
	registerProtocol (fileHandler);
	fileProtocolHandler = fileHandler;

	AutoPtr<IProtocolHandler> resHandler = NEW ResourceProtocolHandler;
	registerProtocol (resHandler);

	AutoPtr<IProtocolHandler> symbolHandler = NEW SymbolProtocolHandler;
	registerProtocol (symbolHandler);

	AutoPtr<IProtocolHandler> memoryHandler = NEW MemoryProtocolHandler;
	registerProtocol (memoryHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VirtualFileSystem::~VirtualFileSystem ()
{
	ListForEach (protocols, IProtocolHandler*, handler)
		handler->release ();
	EndFor
	protocols.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INativeFileSystem& VirtualFileSystem::getNativeFileSystem ()
{
	return NativeFileSystem::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API VirtualFileSystem::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IFileSystem)
	QUERY_INTERFACE (IVolumeFileSystem)
	QUERY_INTERFACE (INativeFileSystem)
	QUERY_INTERFACE (IFileItemProvider)
	QUERY_INTERFACE (IProtocolHandlerRegistry)
	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API VirtualFileSystem::registerProtocol (IProtocolHandler* handler)
{
	IProtocolHandler* oldHandler = getHandler (handler->getProtocol ());
	ASSERT (oldHandler == nullptr)
	if(oldHandler)
		return kResultFailed;

	protocols.append (handler);
	handler->retain ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API VirtualFileSystem::unregisterProtocol (IProtocolHandler* handler)
{
	if(protocols.remove (handler))
	{
		handler->release ();
		return kResultOk;
	}
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IProtocolHandler* CCL_API VirtualFileSystem::getHandler (StringRef protocol) const
{
	ListForEach (protocols, IProtocolHandler*, handler)
		if(handler->getProtocol () == protocol)
			return handler;
	EndFor

	if(protocol.isEmpty ())
		return fileProtocolHandler;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileSystem* VirtualFileSystem::getMountPoint (UrlRef url)
{
	IProtocolHandler* handler = getHandler (url.getProtocol ());
	return handler ? handler->getMountPoint (url.getHostName ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileSystem* VirtualFileSystem::getProtocolMountPoint (UrlRef url)
{
	IProtocolHandler* handler = getHandler (url.getProtocol ());
	return handler ? handler->getMountPoint (String::kEmpty) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileDescriptor* CCL_API VirtualFileSystem::openFileItem (UrlRef url)
{
	IFileSystem* fileSys = getMountPoint (url);
	UnknownPtr<IFileItemProvider> provider (fileSys);
	return provider ? provider->openFileItem (url) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API VirtualFileSystem::openStream (UrlRef url, int mode, IUnknown* context)
{
	IFileSystem* fileSys = getMountPoint (url);
	return fileSys ? fileSys->openStream (url, mode, context) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::fileExists (UrlRef url)
{
	IFileSystem* fileSys = getMountPoint (url);
	return fileSys ? fileSys->fileExists (url) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::getFileInfo (FileInfo& info, UrlRef url)
{
	IFileSystem* fileSys = getMountPoint (url);
	return fileSys ? fileSys->getFileInfo (info, url) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::removeFile (UrlRef url, int mode)
{
	IFileSystem* fileSys = getMountPoint (url);
	return fileSys ? fileSys->removeFile (url, mode) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::renameFile (UrlRef url, StringRef newName, int mode)
{
	IFileSystem* fileSys = getMountPoint (url);
	return fileSys ? fileSys->renameFile (url, newName, mode) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API VirtualFileSystem::newIterator (UrlRef url, int mode)
{
	IFileSystem* fileSys = getMountPoint (url);
	return fileSys ? fileSys->newIterator (url, mode) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::createFolder (UrlRef url)
{
	IFileSystem* fileSys = getMountPoint (url);
	return fileSys ? fileSys->createFolder (url) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::removeFolder (UrlRef url, int mode)
{
	IFileSystem* fileSys = getMountPoint (url);
	return fileSys ? fileSys->removeFolder (url, mode) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::getVolumeInfo (VolumeInfo& info, UrlRef rootUrl)
{
	UnknownPtr<IVolumeFileSystem> fileSys (getMountPoint (rootUrl));
	if(fileSys == nullptr)
		fileSys = getProtocolMountPoint (rootUrl);
	return fileSys ? fileSys->getVolumeInfo (info, rootUrl) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::isLocalFile (UrlRef url)
{
	UnknownPtr<IVolumeFileSystem> fileSys (getMountPoint (url));
	if(fileSys == nullptr)
		fileSys = getProtocolMountPoint (url);

	if(fileSys)
		return fileSys->isLocalFile (url);
	else // second try: not a volume file system, but maybe a local handler or an implicitly local protocol
	{
		return url.getProtocol () == "class"
			|| url.getProtocol () == "virtual"
			|| getHandler (url.getProtocol ()) != nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::isHiddenFile (UrlRef url)
{
	UnknownPtr<IVolumeFileSystem> fileSys (getMountPoint (url));
	if(fileSys == nullptr)
		fileSys = getProtocolMountPoint (url);
	return fileSys ? fileSys->isHiddenFile (url) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::isWriteProtected (UrlRef url)
{
	UnknownPtr<IVolumeFileSystem> fileSys (getMountPoint (url));
	if(fileSys == nullptr)
		fileSys = getProtocolMountPoint (url);
	return fileSys ? fileSys->isWriteProtected (url) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::moveFile (UrlRef dstPath, UrlRef srcPath, int mode, IProgressNotify* progress)
{
	UnknownPtr<IVolumeFileSystem> dstFileSys (getMountPoint (dstPath));
	if(dstFileSys == nullptr)
		dstFileSys = getProtocolMountPoint (dstPath);

	UnknownPtr<IVolumeFileSystem> srcFileSys (getMountPoint (srcPath));
	if(srcFileSys == nullptr)
		srcFileSys = getProtocolMountPoint (srcPath);

	if(dstFileSys && srcFileSys && dstFileSys == srcFileSys)
		return srcFileSys->moveFile (dstPath, srcPath, mode, progress);
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::copyFile (UrlRef dstPath, UrlRef srcPath, int mode, IProgressNotify* progress)
{
	if(dstPath.isNativePath () && srcPath.isNativePath ())
	{
		UnknownPtr<INativeFileSystem> fileSys (getMountPoint (dstPath));
		return fileSys ? fileSys->copyFile (dstPath, srcPath, mode, progress) : false;
	}
	else // copy between different file systems
	{
		AutoPtr<IStream> srcStream = openStream (srcPath, IStream::kOpenMode);
		if(srcStream)
		{			
			// create dest stream if source stream is valid (so no empty dest files are created)
			AutoPtr<IStream> dstStream = openStream (dstPath, IStream::kCreateMode);
			if(dstStream)
			{
				if(System::GetFileUtilities ().copyStream (*dstStream, *srcStream, progress))
					return true;
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::getPathType (int& type, UrlRef baseFolder, StringRef fileName)
{
	UnknownPtr<INativeFileSystem> fileSys (getMountPoint (baseFolder));
	#if DEBUG
	if(!fileSys)
		CCL_NOT_IMPL ("Not supported for this file system!\n")
	#endif
	return fileSys ? fileSys->getPathType (type, baseFolder, fileName) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::setFileTime (UrlRef url, const FileTime& modifiedTime)
{
	UnknownPtr<INativeFileSystem> fileSys (getMountPoint (url));
	#if DEBUG
	if(!fileSys)
		CCL_NOT_IMPL ("Not supported for this file system!\n")
	#endif
	return fileSys ? fileSys->setFileTime (url, modifiedTime) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* CCL_API VirtualFileSystem::createSearcher (ISearchDescription& description)
{
	// try to create specialized searcher
	if(UnknownPtr<INativeFileSystem> nativeFileSys = getMountPoint (description.getStartPoint ()))
		if(ISearcher* searcher = nativeFileSys->createSearcher (description))
			return searcher;

	// fallback to generic file searcher
	return NEW FileSearcher (*this, description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::getWorkingDirectory (IUrl& url)
{
	return getNativeFileSystem ().getWorkingDirectory (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::setWorkingDirectory (UrlRef url)
{
	return getNativeFileSystem ().setWorkingDirectory (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::isCaseSensitive ()
{
	return getNativeFileSystem ().isCaseSensitive ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::getFirstError (int& errorCode)
{
	return getNativeFileSystem ().getFirstError (errorCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CCL_API VirtualFileSystem::getErrorString (int errorCode)
{
	return getNativeFileSystem ().getErrorString (errorCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::beginTransaction ()
{
	return getNativeFileSystem ().beginTransaction ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VirtualFileSystem::endTransaction (int mode, IProgressNotify* progress)
{
	return getNativeFileSystem ().endTransaction (mode, progress);
}

//************************************************************************************************
// FileSearcher
//************************************************************************************************

FileSearcher::FileSearcher (VirtualFileSystem& fileSystem, ISearchDescription& description)
: fileSystem (fileSystem),
  description (description)
{
	description.retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileSearcher::~FileSearcher ()
{
	description.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FileSearcher::find (ISearchResultSink& resultSink, IProgressNotify* progress)
{
	return findInFolder (description.getStartPoint (), resultSink, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult FileSearcher::findInFolder (UrlRef folder, ISearchResultSink& resultSink, IProgressNotify* progress)
{
	ForEachFile (fileSystem.newIterator (folder, IFileIterator::kAll|IFileIterator::kIgnoreHidden), p)
		if(progress && progress->isCanceled ())
			return kResultAborted;

		//if(fileSystem.isHiddenFile (*p)) // ignore hidden files => done by iterator.
		//	continue;

		String fileName;
		p->getName (fileName, false);

		if(description.matchesName (fileName))
		{
			IUrl* resultItem = NEW Url (*p);
			resultSink.addResult (resultItem);
		}
		
		if(p->isFolder ())
		{
			tresult tr = findInFolder (*p, resultSink, progress);
			if(tr != kResultOk)
				return tr;
		}
	EndFor
	return kResultOk;
}

//************************************************************************************************
// RelativeFileSystem
//************************************************************************************************

DEFINE_CLASS (RelativeFileSystem, Object)
DEFINE_CLASS_NAMESPACE (RelativeFileSystem, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

RelativeFileSystem::RelativeFileSystem (IFileSystem* fileSys, IUrl* baseUrl)
: fileSys (fileSys),
  baseUrl (baseUrl)
{
	if(fileSys)
		fileSys->retain ();
	if(baseUrl)
		baseUrl->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RelativeFileSystem::~RelativeFileSystem ()
{
	if(fileSys)
		fileSys->release ();
	if(baseUrl)
		baseUrl->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrl* RelativeFileSystem::translateUrl (UrlRef relUrl) const
{
	IUrl* absUrl = nullptr;
	relUrl.clone (absUrl);
	absUrl->setProtocol (baseUrl->getProtocol ());
	absUrl->setHostName (baseUrl->getHostName ());
	absUrl->makeAbsolute (*baseUrl);
	return absUrl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API RelativeFileSystem::openStream (UrlRef url, int mode, IUnknown* context)
{
	AutoPtr<IUrl> absUrl = translateUrl (url);
	return fileSys->openStream (*absUrl, mode, context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RelativeFileSystem::fileExists (UrlRef url)
{
	AutoPtr<IUrl> absUrl = translateUrl (url);
	return fileSys->fileExists (*absUrl);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RelativeFileSystem::getFileInfo (FileInfo& info, UrlRef url)
{
	AutoPtr<IUrl> absUrl = translateUrl (url);
	return fileSys->getFileInfo (info, *absUrl);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RelativeFileSystem::removeFile (UrlRef url, int mode)
{
	AutoPtr<IUrl> absUrl = translateUrl (url);
	return fileSys->removeFile (*absUrl, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RelativeFileSystem::renameFile (UrlRef url, StringRef newName, int mode)
{
	AutoPtr<IUrl> absUrl = translateUrl (url);
	return fileSys->renameFile (*absUrl, newName, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API RelativeFileSystem::newIterator (UrlRef url, int mode)
{
	AutoPtr<IUrl> absUrl = translateUrl (url);
	IFileIterator* iter = fileSys->newIterator (*absUrl, mode);
	if(iter == nullptr)
		return nullptr;

	return NEW RelativeFileIterator (*iter, url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RelativeFileSystem::createFolder (UrlRef url)
{
	AutoPtr<IUrl> absUrl = translateUrl (url);
	return fileSys->createFolder (*absUrl);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RelativeFileSystem::removeFolder (UrlRef url, int mode)
{
	AutoPtr<IUrl> absUrl = translateUrl (url);
	return fileSys->removeFolder (*absUrl, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RelativeFileSystem::isCaseSensitive ()
{
	return fileSys->isCaseSensitive ();
}

//************************************************************************************************
// RelativeFileIterator
//************************************************************************************************

RelativeFileIterator::RelativeFileIterator (IFileIterator& iter, UrlRef outDir)
: iter (iter),
  outDir (*NEW Url (outDir)),
  current (*NEW Url)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

RelativeFileIterator::~RelativeFileIterator ()
{
	iter.release ();
	outDir.release ();
	current.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* CCL_API RelativeFileIterator::next ()
{
	const IUrl* next = iter.next ();
	if(next == nullptr)
		return nullptr;

	String fileName;
	next->getName (fileName);

	current.assign (outDir);
	current.descend (fileName, next->getType ());
	return &current;
}

//************************************************************************************************
// SymbolicFileSystem
//************************************************************************************************

bool SymbolicFileSystem::resolve (Url& resolved, UrlRef url) const
{
	return System::GetSystem ().resolveLocation (resolved, url) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API SymbolicFileSystem::openStream (UrlRef _url, int mode, IUnknown* context)
{
	Url resolved;
	return resolve (resolved, _url) ? theFileSystem.openStream (resolved, mode, context) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API SymbolicFileSystem::newIterator (UrlRef _url, int mode)
{
	Url resolved;
	return resolve (resolved, _url) ? theFileSystem.newIterator (resolved, mode) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SymbolicFileSystem::fileExists (UrlRef _url)
{
	Url resolved;
	return resolve (resolved, _url) ? theFileSystem.fileExists (resolved) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SymbolicFileSystem::getFileInfo (FileInfo& info, UrlRef _url)
{
	Url resolved;
	return resolve (resolved, _url) ? theFileSystem.getFileInfo (info, resolved) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SymbolicFileSystem::removeFile (UrlRef _url, int mode)
{
	Url resolved;
	return resolve (resolved, _url) ? theFileSystem.removeFile (resolved, mode) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SymbolicFileSystem::removeFolder (UrlRef _url, int mode)
{
	Url resolved;
	return resolve (resolved, _url) ? theFileSystem.removeFolder (resolved, mode) : false;
}
