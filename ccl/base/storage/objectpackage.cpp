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
// Filename    : ccl/base/storage/objectpackage.cpp
// Description : Object Package
//
//************************************************************************************************

#include "ccl/base/storage/objectpackage.h"
#include "ccl/base/storage/archivehandler.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// ObjectPackage
//************************************************************************************************

UrlRef ObjectPackage::getBaseFolder ()
{
	static Url* path = nullptr;
	if(path == nullptr)
	{
		path = MemoryUrl::newBin ();
		Object::addGarbageCollected (path);
	}
	return *path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ObjectPackage, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectPackage::ObjectPackage ()
: path (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectPackage::~ObjectPackage ()
{
	empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectPackage::isEmpty () const
{
	return path == nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectPackage::empty ()
{
	removeFile ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url& ObjectPackage::getPath (int64 objectId)
{
	if(path == nullptr)
	{
		path = NEW Url (getBaseFolder ());
		String fileName ("object");
		fileName.appendHexValue (objectId, 8);
		path->descend (fileName, Url::kFile);
		path->makeUnique ();
	}
	return *path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectPackage::removeFile ()
{
	if(path)
	{
		if(System::GetFileSystem ().fileExists (*path))
			System::GetFileSystem ().removeFile (*path);

		path->release ();
		path = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectPackage::storeObject (const Object& object, StringID saveType, IUnknown* context, IProgressNotify* progress)
{
	empty ();

	bool result = storeInternal (object, saveType, context, progress);
	if(!result)
		empty ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectPackage::storeInternal (const Object& object, StringID saveType, IUnknown* context, IProgressNotify* progress)
{
	Url& path = getPath ((int64)&object);

	AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().createPackage (path, ClassID::PackageFile);
	ASSERT (packageFile != nullptr)
	packageFile->setOption (PackageOption::kCompressed, true);
	if(!packageFile->create ())
		return false;

	ASSERT (packageFile->getFileSystem () != nullptr)
	ArchiveHandler archiveHandler (*packageFile->getFileSystem (), saveType);
	IFileResource::Closer packageFileCloser (*packageFile);
	if(context)
		archiveHandler.getContext ().set ("context", context);

	if(!archiveHandler.addSaveTask (CCLSTR ("content.xml"), "Content", *const_cast<Object*> (&object)))
		return false;

	if(!packageFile->flush (progress))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectPackage::restoreObject (Object& object, StringID saveType, IUnknown* context, IProgressNotify* progress)
{
	if(isEmpty ())
		return false;

	bool result = restoreInternal (object, saveType, context, progress);
	if(result)
		removeFile ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectPackage::restoreInternal (Object& object, StringID saveType, IUnknown* context, IProgressNotify* progress)
{
	ASSERT (path != nullptr)

	AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().createPackage (*path, ClassID::PackageFile);
	ASSERT (packageFile != nullptr)
	if(!packageFile->open ())
		return false;

	IFileResource::Closer packageFileCloser (*packageFile);
	ASSERT (packageFile->getFileSystem () != nullptr)

	ArchiveHandler archiveHandler (*packageFile->getFileSystem (), saveType);
	if(context)
		archiveHandler.getContext ().set ("context", context);

	return archiveHandler.loadItem (CCLSTR ("content.xml"), "Content", object);
}
