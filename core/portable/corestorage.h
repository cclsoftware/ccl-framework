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
// Filename    : core/portable/corestorage.h
// Description : Storage classes
//
//************************************************************************************************

#ifndef _corestorage_h
#define _corestorage_h

#include "core/portable/coreattributes.h"
#include "core/portable/coretypeinfo.h"

namespace Core {
namespace Threads {
class Lock; }

namespace Portable {

//************************************************************************************************
// IStorageFilter
/** Filter for storage operations \ingroup core_portable */
//************************************************************************************************

class IStorageFilter: public ITypedObject
{
public:
	DECLARE_CORE_CLASS_ ('IStF', IStorageFilter)

	virtual bool shouldLoad (int typeId, CStringPtr name, void* object) const = 0;
	virtual bool shouldSave (int typeId, CStringPtr name, void* object) const = 0;
};

//************************************************************************************************
// IStorageCancelHook
/** Cancel storage operations \ingroup core_portable */
//************************************************************************************************

class IStorageCancelHook
{
public:
	virtual bool shouldCancelStorage () const = 0;
};

//************************************************************************************************
// StorageBase
/** Filter and mode options for storage operations \ingroup core_portable */
//************************************************************************************************

class StorageBase
{
public:
	StorageBase (const IStorageFilter* filter = nullptr)
    : filter (filter),
	  cancelHook (nullptr),
	  lock (nullptr)
	{}

	StorageBase (const StorageBase& storage)
	: mode (storage.mode),
	  filter (storage.filter),
	  cancelHook (storage.cancelHook),
	  lock (nullptr)
	{}

	PROPERTY_CSTRING_BUFFER (32, mode, Mode)
	PROPERTY_POINTER (const IStorageFilter, filter, Filter)
	PROPERTY_POINTER (const IStorageCancelHook, cancelHook, CancelHook)
	PROPERTY_POINTER (Threads::Lock, lock, Lock)

	bool isCanceled () const
	{
		return cancelHook ? cancelHook->shouldCancelStorage () : false;
	}
};

//************************************************************************************************
// InputStorage
/** Storage class for load operations. \ingroup core_portable */
//************************************************************************************************

class InputStorage: public StorageBase
{
public:
	InputStorage (const Attributes& attributes, const IStorageFilter* filter = nullptr)
	: StorageBase (filter),
	  attributes (attributes),
	  parentAttributes (nullptr)
	{}

	InputStorage (const Attributes& attributes, const InputStorage& storage)
	: StorageBase (storage),
	  attributes (attributes),
	  parentAttributes (nullptr)
	{}

	PROPERTY_POINTER (const Attributes, parentAttributes, ParentAttributes)

	const Attributes& getAttributes () const { return attributes; }

protected:
	const Attributes& attributes;
};

//************************************************************************************************
// OutputStorage
/** Storage class for save operations. \ingroup core_portable */
//************************************************************************************************

class OutputStorage: public StorageBase
{
public:
	OutputStorage (AttributeHandler& writer, const IStorageFilter* filter = nullptr)
	: StorageBase (filter),
	  writer (writer)
	{}

	OutputStorage (AttributeHandler& writer, const OutputStorage& storage)
	: StorageBase (storage),
	  writer (writer)
	{}

	AttributeHandler& getWriter () { return writer; }

protected:
	AttributeHandler& writer;
};

} // namespace Portable
} // namespace Core

#endif // _corestorage_h
