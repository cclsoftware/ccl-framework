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
// Filename    : ccl/base/storage/storableobject.h
// Description : Storable Object
//
//************************************************************************************************

#ifndef _ccl_storableobject_h
#define _ccl_storableobject_h

#include "ccl/base/object.h"
#include "ccl/public/storage/istorage.h"

namespace CCL {

//************************************************************************************************
// StorableObject
//************************************************************************************************

class StorableObject: public Object,
					  public IStorable
{
public:
	DECLARE_CLASS (StorableObject, Object)
	DECLARE_METHOD_NAMES (StorableObject)
	
	bool saveToFile (UrlRef path) const;
	bool loadFromFile (UrlRef path);
	bool saveToStream (IStream& stream) const;
	bool loadFromStream (IStream& stream);

	// IStorable
	tbool CCL_API getFormat (FileType& format) const override;
	tbool CCL_API save (IStream& stream) const override;
	tbool CCL_API load (IStream& stream) override;

	CLASS_INTERFACE (IStorable, Object)

	static bool saveToFile (const Object& object, UrlRef path, int flags = 0);
	static bool loadFromFile (Object& object, UrlRef path, int flags = 0);
	
	static bool saveToStream (const Object& object, IStream& stream, int flags = 0);
	static bool loadFromStream (Object& object, IStream& stream, int flags = 0);

protected:
	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// JsonStorableObject
//************************************************************************************************

class JsonStorableObject: public StorableObject
{
public:
	DECLARE_CLASS (JsonStorableObject, StorableObject)

	// StorableObject
	tbool CCL_API getFormat (FileType& format) const override;
	tbool CCL_API save (IStream& stream) const override;
	tbool CCL_API load (IStream& stream) override;
};

} // namespace CCL

#endif // _ccl_storableobject_h
