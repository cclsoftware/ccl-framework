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
// Filename    : ccl/base/storage/storableobject.cpp
// Description : Storable Object
//
//************************************************************************************************

#include "ccl/base/storage/storableobject.h"
#include "ccl/base/storage/xmlarchive.h"
#include "ccl/base/storage/jsonarchive.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// StorableObject (static helpers)
//************************************************************************************************

bool StorableObject::saveToFile (const Object& object, UrlRef path, int flags)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kCreateMode);
	return stream ? saveToStream (object, *stream, flags) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StorableObject::loadFromFile (Object& object, UrlRef path, int flags)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kOpenMode);
	return stream ? loadFromStream (object, *stream, flags) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StorableObject::saveToStream (const Object& object, IStream& stream, int flags)
{
	XmlArchive archive (stream);
	archive.setFlags (flags);
	return archive.saveObject (object.myClass ().getPersistentName (), object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StorableObject::loadFromStream (Object& object, IStream& stream, int flags)
{
	XmlArchive archive (stream);
	archive.setFlags (flags);
	return archive.loadObject (object.myClass ().getPersistentName (), object);
}

//************************************************************************************************
// StorableObject
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StorableObject, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StorableObject::saveToFile (UrlRef path) const
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kCreateMode);
	return stream && save (*stream) ? true : false; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StorableObject::loadFromFile (UrlRef path)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kOpenMode);
	return stream && load (*stream) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StorableObject::saveToStream (IStream& stream) const
{
	return save (stream) != 0; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StorableObject::loadFromStream (IStream& stream)
{
	return load (stream) != 0; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StorableObject::getFormat (FileType& format) const
{
	format = XmlArchive::getFileType ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StorableObject::save (IStream& stream) const
{
	return saveToStream (*this, stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StorableObject::load (IStream& stream)
{
	return loadFromStream (*this, stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (StorableObject)
	DEFINE_METHOD_ARGR ("saveToFile", "path", "bool")
	DEFINE_METHOD_ARGR ("loadFromFile", "path", "bool")
END_METHOD_NAMES (StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StorableObject::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "saveToFile")
	{
		UnknownPtr<IUrl> path (msg[0].asUnknown ());
		ASSERT (path)
		returnValue = path && saveToFile (*path);
		return true;
	}
	else if(msg == "loadFromFile")
	{
		UnknownPtr<IUrl> path (msg[0].asUnknown ());
		ASSERT (path)
		returnValue = path && loadFromFile (*path);
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// JsonStorableObject
//************************************************************************************************

DEFINE_CLASS_HIDDEN (JsonStorableObject, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API JsonStorableObject::getFormat (FileType& format) const
{
	format = JsonArchive::getFileType ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API JsonStorableObject::save (IStream& stream) const
{
	JsonArchive archive (stream);
	return archive.saveObject (nullptr, *this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API JsonStorableObject::load (IStream& stream)
{
	JsonArchive archive (stream);
	return archive.loadObject (nullptr, *this);
}
