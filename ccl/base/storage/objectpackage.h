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
// Filename    : ccl/base/storage/objectpackage.h
// Description : Object Package
//
//************************************************************************************************

#ifndef _ccl_objectpackage_h
#define _ccl_objectpackage_h

#include "ccl/base/object.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

class Url;
interface IProgressNotify;

//************************************************************************************************
// ObjectPackage
/** Save state of complex object including sub-streams into memory file. */
//************************************************************************************************

class ObjectPackage: public Object
{
public:
	DECLARE_CLASS (ObjectPackage, Object)

	ObjectPackage ();
	~ObjectPackage ();

	PROPERTY_MUTABLE_CSTRING (storageType, StorageType)

	void empty ();
	bool isEmpty () const;
	bool storeObject (const Object& object, StringID saveType = nullptr, IUnknown* context = nullptr, IProgressNotify* progress = nullptr);
	bool restoreObject (Object& object, StringID saveType = nullptr, IUnknown* context = nullptr, IProgressNotify* progress = nullptr);

protected:
	static UrlRef getBaseFolder ();

	Url* path;

	Url& getPath (int64 objectId);
	void removeFile ();
	bool storeInternal (const Object& object, StringID saveType, IUnknown* context, IProgressNotify* progress);
	bool restoreInternal (Object& object, StringID saveType, IUnknown* context, IProgressNotify* progress);
};

} // namespace CCL

#endif // _ccl_objectpackage_h
