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
// Filename    : ccl/app/fileinfo/fileinforegistry.cpp
// Description : File Info Registry
//
//************************************************************************************************

#include "ccl/app/fileinfo/fileinforegistry.h"
#include "ccl/app/fileinfo/volumeinfocomponent.h"
#include "ccl/app/fileinfo/presetfileinfo.h"
#include "ccl/app/fileinfo/pluginfileinfo.h"

#include "ccl/app/utilities/pathclassifier.h"

#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// DefaultFileInfo
//************************************************************************************************

class DefaultFileInfo: public StandardFileInfo
{
public:
	tbool CCL_API isDefault () override { return true; }
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// FileInfoFactory
//************************************************************************************************

FileInfoFactory::FileInfoFactory ()
: localFilesOnly (true)
{}

//************************************************************************************************
// FileInfoRegistry
//************************************************************************************************

DEFINE_SINGLETON (FileInfoRegistry)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileInfoRegistry::FileInfoRegistry ()
{
	factories.objectCleanup (true);

	PresetFileInfo::registerInfo ();
	PlugInFileInfo::registerInfo ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileInfoRegistry::registerFileInfoFactory (FileInfoFactory* factory)
{
	factories.append (factory);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileInfoRegistry::unregisterFileInfoFactory (FileInfoFactory* factory)
{
	if(factories.remove (factory))
	{
		factory->release ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileInfoComponent* CCL_API FileInfoRegistry::createComponent (UrlRef path) const
{
	bool isLocal = FileInfoComponent::isLocal (path);

	IFileInfoComponent* c = nullptr;
	ForEach (factories, FileInfoFactory, f)
		if(!isLocal && f->isLocalFilesOnly ())
			continue;
		c = f->createComponent (path);
		if(c)
			return c;
	EndFor

	// special cases for volumes, packages, etc.
	switch(PathClassifier::classify (path))
	{
	case PathClassifier::kNativeVolume : 
		c = NEW VolumeInfoComponent;
		break;

	case PathClassifier::kPackageVolume : 
		c = NEW PackageVolumeInfo;
		break;
	}

	if(c == nullptr)
		c = NEW DefaultFileInfo ();

	c->setFile (path);
	return c;
}
