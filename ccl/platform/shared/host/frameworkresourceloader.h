//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/shared/host/frameworkresourceloader.h
// Description : Cross-platform resource loader implementation
//
//************************************************************************************************

#ifndef _ccl_frameworkresourceloader_h
#define _ccl_frameworkresourceloader_h

#include "ccl/system/virtualfilesystem.h"
#include "ccl/system/packaging/zipfile.h"
#include "ccl/public/system/threadsync.h"

namespace CCL {

//************************************************************************************************
// FrameworkResourceFileSystem
//************************************************************************************************

class FrameworkResourceFileSystem: public ResourceFileSystem
{
public:
	static const String kResourceFolder;
	DECLARE_STRINGID_MEMBER (kResourceDataFunction)
	DECLARE_STRINGID_MEMBER (kResourceSizeFunction)

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

#endif // _ccl_frameworkresourceloader_h
