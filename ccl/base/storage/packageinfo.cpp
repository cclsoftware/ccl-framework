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
// Filename    : ccl/base/storage/packageinfo.cpp
// Description : Package Meta Information
//
//************************************************************************************************

#include "ccl/base/storage/packageinfo.h"

#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/archivehandler.h"
#include "ccl/base/storage/storableobject.h"

#include "ccl/public/storage/filetype.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// PackageResource
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PackageResource, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageResource::PackageResource (StringID id, StringRef fileName, IStorable* data)
: id (id),
  fileName (fileName)
{
	setData (data);
}

//************************************************************************************************
// PackageInfo
//************************************************************************************************

const String PackageInfo::kFileName = CCLSTR ("metainfo.xml");
const CString PackageInfo::kRootName = CSTR ("MetaInformation");

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (PackageInfo, PersistentAttributes)
DEFINE_CLASS_NAMESPACE (PackageInfo, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageInfo::PackageInfo ()
{
	resources.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageInfo::PackageInfo (const IAttributeList& attributes)
{
	copyFrom (attributes);
	resources.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageInfo::addResource (PackageResource* resource)
{
	set (resource->getID (), resource->getFileName ());
	resources.add (resource);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageResource* PackageInfo::addResource (StringID id, StringRef fileName, IStorable* data)
{
	PackageResource* resource = NEW PackageResource (id, fileName, data);
	addResource (resource);
	return resource;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageResource* PackageInfo::getResource (StringID id) const
{
	ForEach (resources, PackageResource, r)
		if(r->getID () == id)
			return r;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStorable* CCL_API PackageInfo::getResourceData (StringID id) const
{
	PackageResource* resource = getResource (id);
	return resource ? resource->getData () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageInfo::loadFromPackage (UrlRef path, int options)
{
	AutoPtr<IPackageFile> package = System::GetPackageHandler ().openPackage (path, options);	
	if(!package)
		return false;

	bool result = loadFromPackage (*package);
	package->close ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageInfo::saveWithPackage (UrlRef path, UIDRef cid) const
{
	AutoPtr<IPackageFile> package = System::GetPackageHandler ().createPackage (path, cid);
	if(!package)
		return false;
	if(!package->create ())
		return false;

	bool result = saveWithPackage (*package);
	package->close ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageInfo::loadFromPackage (IPackageFile& package)
{
	IFileSystem* fileSystem = package.getFileSystem ();
	ASSERT (fileSystem != nullptr)
	if(!fileSystem)
		return false;

	ArchiveHandler handler (*fileSystem);
	return loadFromHandler (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageInfo::saveWithPackage (IPackageFile& package) const
{
	IFileSystem* fileSystem = package.getFileSystem ();
	ASSERT (fileSystem != nullptr)
	if(!fileSystem)
		return false;

	AutoPtr<ArchiveHandler> handler = NEW ArchiveHandler (*fileSystem);
	return saveWithHandler (*handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageInfo::loadFromHandler (ArchiveHandler& handler)
{
	bool result = handler.loadItem (kFileName, kRootName, *this);
	
	if(!resources.isEmpty ())
		ForEach (resources, PackageResource, r)
			ASSERT (r->getData () != nullptr)
			if(r->getData ())
				handler.loadStream (r->getFileName (), *r->getData ());
		EndFor

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageInfo::saveWithHandler (ArchiveHandler& handler) const
{
	bool result = handler.addSaveTask (kFileName, kRootName, *const_cast<PackageInfo*> (this));

	if(!resources.isEmpty ())
		ForEach (resources, PackageResource, r)
			ASSERT (r->getData () != nullptr)
			if(r->getData ())
			{
				FileType format;
				if(r->getData ()->getFormat (format)) // otherwise resource is treated empty
					handler.addSaveTask (r->getFileName (), *r->getData ());
			}
		EndFor

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageInfo::toXml (IStream& xmlStream) const
{
	return StorableObject::saveToStream (*this, xmlStream);
}
