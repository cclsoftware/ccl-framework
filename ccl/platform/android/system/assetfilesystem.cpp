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
// Filename    : ccl/platform/android/system/assetfilesystem.cpp
// Description : Android Asset File System
//
//************************************************************************************************

#include "ccl/platform/android/system/assetfilesystem.h"

#include "ccl/platform/android/system/system.android.h"
#include "ccl/platform/android/system/nativefilesystem.android.h"

#include "ccl/platform/android/interfaces/iframeworkactivity.h"

#include "ccl/base/storage/protocolhandler.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/text/cstring.h"

#include <android/asset_manager.h>

//************************************************************************************************
// JNI classes
//************************************************************************************************

namespace CCL {
namespace Android {

//************************************************************************************************
// AssetManager
//************************************************************************************************

DECLARE_JNI_CLASS (AssetManager, "android/content/res/AssetManager")
	DECLARE_JNI_METHOD (jobjectArray, list, jstring)
END_DECLARE_JNI_CLASS (AssetManager)

DEFINE_JNI_CLASS (AssetManager)
	DEFINE_JNI_METHOD (list, "(Ljava/lang/String;)[Ljava/lang/String;")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace Core;
using namespace Core::Java;

namespace CCL {
namespace Android {

//************************************************************************************************
// Asset
//************************************************************************************************

class Asset: public Unknown
{
public:
	Asset (CStringRef fileName);
	~Asset ();

	CStringRef getFileName () const { return fileName; }
	bool getFileInfo (FileInfo& info) const;

	int64 getFileSize () const;
	int readAt (int64 pos, void* buffer, int size);

	bool isValid () const { return asset != nullptr; }

	// IUnknown
	unsigned int CCL_API release () override;

private:
	Threading::ExclusiveLock lock;

	MutableCString fileName;
	AAsset* asset;
};

//************************************************************************************************
// AssetCache
/** Caches opened assets for reuse upon subsequent open requests.
    AAssetManager_open maps the whole asset to memory, so opening a large asset multiple times
    can easily fill up the virtual address space if caching is not being used. */
//************************************************************************************************

class AssetCache: public Unknown,
				  public Threading::ILockProvider
{
public:
	static AssetCache& instance ();

	void addAsset (Asset* asset);
	void removeAsset (Asset* asset);

	Asset* requestAsset (CStringRef fileName);

	// ILockProvider
	Threading::ILockable* CCL_API getLock () const override { return &lock; }

	CLASS_INTERFACE (Threading::ILockProvider, Unknown)

private:
	AssetCache () {}

	mutable Threading::ExclusiveLock lock;

	Vector<Asset*> assets;
};

//************************************************************************************************
// AssetStream
//************************************************************************************************

class AssetStream: public Unknown,
				   public IStream
{
public:
	AssetStream (Asset* asset);

	// IStream
	int CCL_API read (void* buffer, int size) override;
	int CCL_API write (const void* buffer, int size) override;
	int64 CCL_API tell () override;
	tbool CCL_API isSeekable () const override;
	int64 CCL_API seek (int64 pos, int mode) override;

	CLASS_INTERFACE (IStream, Unknown)

protected:
	SharedPtr<Asset> asset;
	int64 position;
};

//************************************************************************************************
// AssetIterator
//************************************************************************************************

class AssetIterator: public Unknown,
					 public IFileIterator
{
public:
	AssetIterator (jobjectArray list, int mode, CStringRef dirName);

	// IFileIterator
	const IUrl* CCL_API next () override;

	CLASS_INTERFACE (IFileIterator, Unknown)

protected:
	JniStringArray list;
	int mode;
	int index;
	MutableCString dirName;
	Url current;
};

//************************************************************************************************
// AssetProtocol
/** Protocol handler for "asset://" urls.
	Such URLs are used internally to access files in the assets folder of Android apps. */
//************************************************************************************************

class AssetProtocol: public Object,
					 public Singleton<AssetProtocol>
{
public:
	class Handler: public ProtocolHandler
	{
	public:
		Handler ()
		: fileSystem (NEW AssetFileSystem)
		{}

		// ProtocolHandler
		StringRef CCL_API getProtocol () const override { return AssetUrl::Protocol; }
		IFileSystem* CCL_API getMountPoint (StringRef name) override { return fileSystem; }

	private:
		AutoPtr<AssetFileSystem> fileSystem;
	};

	AssetProtocol ()
	: handler (NEW Handler)
	{
		UnknownPtr<IProtocolHandlerRegistry> registry (&System::GetFileSystem ());
		ASSERT (registry != nullptr)
		if(registry)
			registry->registerProtocol (handler);
	}

	~AssetProtocol ()
	{
		UnknownPtr<IProtocolHandlerRegistry> registry (&System::GetFileSystem ());
		if(registry)
			registry->unregisterProtocol (handler);
	}

private:
	AutoPtr<Handler> handler;
};

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace Android;

//************************************************************************************************
// AssetProtocol
//************************************************************************************************

DEFINE_SINGLETON (AssetProtocol)

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (AssetProtocol, kFrameworkLevelFirst - 1) // before locale manager
{
	AssetProtocol::instance ();
	return true;
}

//************************************************************************************************
// AssetFileSystem
//************************************************************************************************

IStream* CCL_API AssetFileSystem::openStream (UrlRef url, int mode, IUnknown* context)
{
	ASSERT (url.getProtocol () == AssetUrl::Protocol)

	MutableCString fileName (url.getPath (), Text::kUTF8);
	if(AutoPtr<Asset> asset = AssetCache::instance ().requestAsset (fileName))
		return NEW AssetStream (asset);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AssetFileSystem::getFileInfo (FileInfo& info, UrlRef url)
{
	ASSERT (url.getProtocol () == AssetUrl::Protocol)

	MutableCString fileName (url.getPath (), Text::kUTF8);
	if(AutoPtr<Asset> asset = AssetCache::instance ().requestAsset (fileName))
		return asset->getFileInfo (info);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API AssetFileSystem::newIterator (UrlRef url, int mode)
{
	ASSERT (url.getProtocol () == AssetUrl::Protocol)

	jobject assetManager = AndroidSystemInformation::getInstance ().getJavaAssetManager ();
	ASSERT (assetManager != nullptr)
	if(assetManager == nullptr)
		return nullptr;

	JniAccessor jni;
	MutableCString dirName (url.getPath (), Text::kUTF8);
	LocalRef list (jni, AssetManager.list (assetManager, JniString (jni, dirName)));
	JniStringArray listArray (jni, (jobjectArray) list.getJObject ());
	if(listArray.getLength () > 0)
	{
		if(!dirName.isEmpty ())
			dirName += "/";
		return NEW AssetIterator (listArray, mode, dirName);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AssetFileSystem::fileExists (UrlRef url)
{
	ASSERT (url.getProtocol () == AssetUrl::Protocol)

	if(url.isFolder ())
	{
		AutoPtr<IFileIterator> iter = newIterator (url);
		return iter.isValid ();
	}
	else
	{
		AutoPtr<IStream> stream = openStream (url, IStream::kReadMode);
		return stream.isValid ();
	}
}

//************************************************************************************************
// AssetUrl
//************************************************************************************************

const String AssetUrl::Protocol ("asset");

//////////////////////////////////////////////////////////////////////////////////////////////////

AssetUrl::AssetUrl (StringRef path, int type)
{
	setProtocol (Protocol);
	setPath (path, type);
}

//************************************************************************************************
// AssetIterator
//************************************************************************************************

AssetIterator::AssetIterator (jobjectArray list, int mode, CStringRef dirName)
: list (Jni::getEnvironment (), list),
  mode (mode),
  index (0),
  dirName (dirName)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* CCL_API AssetIterator::next ()
{
	JniAccessor jni;

	bool wantFiles = (mode & kFiles) != 0;
	bool wantFolders = (mode & kFolders) != 0;
	while(list.getLength () > index)
	{
		LocalStringRef nextName (jni, list[index++]);

		String path;
		if(!dirName.isEmpty ()) // make absolute
			path.appendCString (Text::kUTF8, dirName);
		path.appendCString (Text::kUTF8, JniCStringChars (jni, nextName));

		AutoPtr<Asset> asset = AssetCache::instance ().requestAsset (MutableCString (path, Text::kUTF8));
		bool isFile = asset.isValid ();
		if((wantFiles && isFile) || (wantFolders && !isFile))
		{
			current = Url (AssetUrl::Protocol, String::kEmpty, path, isFile ? Url::kFile : Url::kFolder);
			return &current;
		}
	}
	return nullptr;
}

//************************************************************************************************
// Asset
//************************************************************************************************

Asset::Asset (CStringRef _fileName)
: fileName (_fileName),
  asset (nullptr)
{
	if(fileName.startsWith ("/"))
		fileName = fileName.subString (1);

	AAssetManager* assetManager = AndroidSystemInformation::getInstance ().getAssetManager ();
	ASSERT (assetManager != nullptr)
	if(assetManager == nullptr)
		return;

	if(asset = AAssetManager_open (assetManager, fileName, AASSET_MODE_RANDOM))
		AssetCache::instance ().addAsset (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Asset::~Asset ()
{
	if(!isValid ())
		return;

	AssetCache::instance ().removeAsset (this);
	AAsset_close (asset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Asset::getFileInfo (FileInfo& info) const
{
	if(!isValid ())
		return false;

	IFrameworkActivity* activity = AndroidSystemInformation::getInstance ().getNativeActivity ();
	if(!activity)
		return false;

	info.fileSize = getFileSize ();

	info.createTime = UnixTime::toLocal (activity->getPackageInstallTime ());
	info.modifiedTime = UnixTime::toLocal (activity->getPackageUpdateTime ());
	info.accessTime = info.modifiedTime;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 Asset::getFileSize () const
{
	if(!isValid ())
		return 0;

	return AAsset_getLength64 (asset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Asset::readAt (int64 position, void* buffer, int size)
{
	if(!isValid ())
		return 0;

	Threading::AutoLock autoLock (&lock);

	AAsset_seek64 (asset, position, SEEK_SET);
	return AAsset_read (asset, buffer, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API Asset::release ()
{
	// The asset cache needs to be locked when releasing an asset to avoid a possible race
	// condition when an asset is about to be deleted, but still included in the cache.
	Threading::AutoLock autoLock (AssetCache::instance ());

	return Unknown::release ();
}

//************************************************************************************************
// AssetCache
//************************************************************************************************

AssetCache& AssetCache::instance ()
{
	static AssetCache theInstance;
	return theInstance;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AssetCache::addAsset (Asset* asset)
{
	Threading::AutoLock autoLock (&lock);

	if(asset)
		assets.add (asset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AssetCache::removeAsset (Asset* asset)
{
	Threading::AutoLock autoLock (&lock);

	if(asset)
		assets.remove (asset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Asset* AssetCache::requestAsset (CStringRef fileName)
{
	Threading::AutoLock autoLock (&lock);

	for(Asset* asset : assets)
	{
		if(asset->getFileName () != fileName)
			continue;

		asset->retain ();
		return asset;
	}

	AutoPtr<Asset> asset = NEW Asset (fileName);
	if(asset->isValid ())
		return asset.detach ();

	return nullptr;
}

//************************************************************************************************
// AssetStream
//************************************************************************************************

AssetStream::AssetStream (Asset* asset)
: asset (asset),
  position (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AssetStream::read (void* buffer, int size)
{
	int numRead = asset->readAt (position, buffer, size);
	if(numRead > 0)
		position += numRead;
	return numRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AssetStream::write (const void* buffer, int size)
{
	return -1; // assets are read-only!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API AssetStream::tell ()
{
	return position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AssetStream::isSeekable () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API AssetStream::seek (int64 pos, int mode)
{
	int64 newPos = 0;
	switch(mode)
	{
	case kSeekSet:
		newPos = pos;
		break;
	case kSeekCur:
		newPos = position + pos;
		break;
	case kSeekEnd:
		newPos = asset->getFileSize () + pos;
		break;
	}

	if(newPos < 0 || newPos > asset->getFileSize ())
		return -1;

	return position = newPos;
}
