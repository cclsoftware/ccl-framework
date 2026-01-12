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
// Filename    : ccl/app/fileinfo/fileinforegistry.h
// Description : File Info Registry
//
//************************************************************************************************

#ifndef _ccl_fileinforegistry_h
#define _ccl_fileinforegistry_h

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/app/ifileinforegistry.h"

namespace CCL {

class FileInfoFactory;

//************************************************************************************************
// FileInfoRegistry
//** Registry of FileInfoFactories. Can create a file info component for a file.*/
//************************************************************************************************

class FileInfoRegistry: public Object,
						public IFileInfoFactory,
						public Singleton<FileInfoRegistry>

{
public:
	FileInfoRegistry ();

	void registerFileInfoFactory (FileInfoFactory* factory);
	bool unregisterFileInfoFactory (FileInfoFactory* factory);

	// IFileInfoFactory
	IFileInfoComponent* CCL_API createComponent (UrlRef path) const override;

	CLASS_INTERFACE (IFileInfoFactory, Object)

protected:
	ObjectList factories;
};

//************************************************************************************************
// FileInfoFactory
/** Factory that can create a file info component for a file. */
//************************************************************************************************

class FileInfoFactory: public Object,
					   public IFileInfoFactory
{
public:
	FileInfoFactory ();

	PROPERTY_BOOL (localFilesOnly, LocalFilesOnly)

	CLASS_INTERFACE (IFileInfoFactory, Object)
};

} // namespace CCL

#endif // _ccl_fileinforegistry_h
