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
// Filename    : ccl/extras/portable/resourcepackage.h
// Description : Resource package
//
//************************************************************************************************

#ifndef _ccl_resourcepackage_h
#define _ccl_resourcepackage_h

#include "ccl/base/storage/file.h"

#include "core/portable/corefile.h"

namespace CCL {

//************************************************************************************************
// ResourcePackage
//************************************************************************************************

class ResourcePackage: public Core::Portable::FilePackage
{
public:
	ResourcePackage (CStringPtr resourceFolder)
	: resourceFolder (resourceFolder)
	{}

	// FilePackage
	bool fileExists (CStringPtr fileName) override
	{
		ResourceUrl url (resourceFolder, Url::kFolder);
		url.descend (fileName);
		return File (url).exists ();
	}

	Core::IO::Stream* openStream (CStringPtr fileName) override
	{
		ResourceUrl url (resourceFolder, Url::kFolder);
		url.descend (fileName);
		if(AutoPtr<IStream> stream = File (url).open ())
			return NEW CoreStream (*stream);
		return 0;
	}

protected:
	String resourceFolder;
};

} // namespace CCL

#endif // _ccl_resourcepackage_h
