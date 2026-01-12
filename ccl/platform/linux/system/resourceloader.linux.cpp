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
// Filename    : ccl/platform/linux/system/resourceloader.linux.cpp
// Description : Linux Resource Loader
//
//************************************************************************************************

#include "ccl/system/virtualfilesystem.h"
#include "ccl/system/packaging/zipfile.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/threadsync.h"
#include "ccl/public/systemservices.h"
#include "ccl/base/storage/url.h"

namespace CCL {

//************************************************************************************************
// LinuxResourceFileSystem
//************************************************************************************************

class LinuxResourceFileSystem: public ResourceFileSystem
{
public:
	static const String kResourceFolder;
	DECLARE_STRINGID_MEMBER (kResourceDataFunction);
	DECLARE_STRINGID_MEMBER (kResourceSizeFunction);
	
	// ResourceFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode, IUnknown* context) override;
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;
	tbool CCL_API fileExists (UrlRef url) override;
	
private:
	typedef unsigned char* (*ResourceDataFunction) ();
	typedef unsigned int (*ResourceSizeFunction) ();
	
	struct ResourceArchive
	{
		ModuleRef moduleRef;
		AutoPtr<ZipFile> file;
		AutoPtr<IStream> stream;
		ResourceDataFunction dataFunction;
		
		ResourceArchive (ModuleRef moduleRef = nullptr)
		: moduleRef (moduleRef),
		  file (nullptr),
		  dataFunction (nullptr)
		{
			if(moduleRef != nullptr)
				file = NEW ZipFile;
		}
		
		bool operator == (const ResourceArchive& other) const
		{
			return moduleRef == other.moduleRef;
		}
		
		bool operator > (const ResourceArchive& other) const
		{
			return moduleRef > other.moduleRef;
		}
	};
	Vector<ResourceArchive> resourceArchives;
	Threading::CriticalSection lock;
	
	ResourceArchive& getResourceArchive (UrlRef url);
};
	
} // namespace CCL

using namespace CCL;

//************************************************************************************************
// ResourceFileSystem
//************************************************************************************************

ResourceFileSystem& ResourceFileSystem::instance ()
{
	static LinuxResourceFileSystem theInstance;
	return theInstance;
}

//************************************************************************************************
// LinuxResourceFileSystem
//************************************************************************************************

const String LinuxResourceFileSystem::kResourceFolder = "Resource";
DEFINE_STRINGID_MEMBER_ (LinuxResourceFileSystem, kResourceDataFunction, "CCLGetResourceData");
DEFINE_STRINGID_MEMBER_ (LinuxResourceFileSystem, kResourceSizeFunction, "CCLGetResourceSize");

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxResourceFileSystem::ResourceArchive& LinuxResourceFileSystem::getResourceArchive (UrlRef url)
{
	ModuleRef moduleRef = System::GetModuleWithIdentifier (url.getHostName ());
	AutoPtr<IExecutableImage> image = System::GetExecutableLoader ().createImage (moduleRef);
	if(image == nullptr)
		return resourceArchives.getError ();
	
	ResourceDataFunction GetResourceData = reinterpret_cast<ResourceDataFunction> (image->getFunctionPointer (kResourceDataFunction));
	ResourceSizeFunction GetResourceSize = reinterpret_cast<ResourceSizeFunction> (image->getFunctionPointer (kResourceSizeFunction));
	if(GetResourceData == nullptr || GetResourceSize == nullptr)
		return resourceArchives.getError ();
	
	int index = resourceArchives.index (moduleRef);
	if(index >= 0)
	{
		if(resourceArchives[index].file->isOpen () && resourceArchives[index].dataFunction == GetResourceData)
			return resourceArchives[index];
		else
			resourceArchives.removeAt (index);
	}
	
	unsigned char* data = GetResourceData ();
	unsigned int size = GetResourceSize ();
	
	resourceArchives.add (moduleRef);
	ResourceArchive& result = resourceArchives.last ();
	result.dataFunction = GetResourceData;
	MemoryStream stream (data, size);
	result.stream = System::GetFileUtilities ().createStreamCopyInMemory (stream);
	
	if(!result.file->openWithStream (*result.stream))
	{
		resourceArchives.removeLast ();
		return resourceArchives.getError ();
	}
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API LinuxResourceFileSystem::openStream (UrlRef url, int mode, IUnknown* context)
{
	Threading::ScopedLock guard (lock);
	ResourceArchive& archive = getResourceArchive (url);
	if(!archive.file.isValid ())
		return nullptr;
	return archive.file->openStream (url, mode, context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API LinuxResourceFileSystem::newIterator (UrlRef url, int mode)
{
	Threading::ScopedLock guard (lock);
	ResourceArchive& archive = getResourceArchive (url);
	if(!archive.file.isValid ())
		return nullptr;
	return archive.file->newIterator (url, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxResourceFileSystem::fileExists (UrlRef url)
{
	Threading::ScopedLock guard (lock);
	ResourceArchive& archive = getResourceArchive (url);
	if(!archive.file.isValid ())
		return false;
	return archive.file->fileExists (url);
}
