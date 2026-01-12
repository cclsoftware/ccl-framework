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
// Filename    : ccl/base/storage/binaryarchive.cpp
// Description : Binary Archive
//
//************************************************************************************************

#define USE_UTF8_STRINGS 1

#include "ccl/base/storage/binaryarchive.h"
#include "ccl/base/storage/storage.h"

#include "ccl/base/kernel.h"

#include "ccl/public/storage/filetype.h"
#include "ccl/public/base/streamer.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Binary Archive File Format
//////////////////////////////////////////////////////////////////////////////////////////////////

static const DEFINE_FOURCC (kArchiveID, '.', 'c', 'c', 'l');
static const DEFINE_FOURCC (kAttributesID, 'a', 't', 't', 'r');

static const int32 kArchiveVersion = 1;
static const ByteOrder kArchiveByteOrder = kLittleEndian;

enum BinaryArchiveTypes // see Variant for other types
{
	kAttrQueueID = 0x10,
	kUtf8StringID
};

typedef int16 ArchiveTypeID;
	
} // namespace CCL

using namespace CCL;

//************************************************************************************************
// BinaryArchive
//************************************************************************************************

const FileType& BinaryArchive::getFileType ()
{
	return FileTypes::Binary ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BinaryArchive::BinaryArchive (IStream& stream, Attributes* context, StringID saveType)
: Archive (stream, context, saveType)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryArchive::saveAttributes (ObjectID root, const Attributes& attributes)
{
	Streamer s (stream, kArchiveByteOrder);
	if(!(s.write (kArchiveID) && s.write (kArchiveVersion) && s.writeWithLength (root)))
		return false;
	return writeAttributes (attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryArchive::writeAttributes (const Attributes& attributes)
{
	Streamer s (stream, kArchiveByteOrder);
	int32 count = attributes.countAttributes ();
	if(!(s.write (kAttributesID) && s.write (count)))
		return false;

	ForEachAttribute (attributes, name, value)
		if(!s.writeWithLength (name))
			return false;

		ArchiveTypeID type = value.getType ();
		if(type == Variant::kObject)
		{
			Object* object = unknown_cast<Object> (value);
			ASSERT (object)
			if(!object)
				return false;

			if(AttributeQueue* queue = ccl_cast<AttributeQueue> (object))
			{
				int32 queueCount = queue->count ();
				if(!(s.write ((ArchiveTypeID)kAttrQueueID) && s.write (queueCount)))
					return false;

				ArrayForEachFast (*queue, Attribute, attr)
					Object* item = unknown_cast<Object> (attr->getValue ());
					ASSERT (item != nullptr)
					if(!(item && writeObject (*item)))
						return false;
				EndFor
			}
			else
			{
				if(!writeObject (*object))
					return false;
			}
		}
		else // primitive types
		{
			#if USE_UTF8_STRINGS
			if(type == Variant::kString)
				type = kUtf8StringID;
			#endif

			if(!s.write (type))
				return false;

			bool result = false;
			switch(type)
			{
			case Variant::kInt : result = s.write (value.lValue); break;
			case Variant::kFloat : result = s.write (value.fValue); break;
			case Variant::kString : result = s.writeWithLength (value.asString ()); break;
			case kUtf8StringID : 
				{
					MutableCString utf8 (value.asString (), Text::kUTF8);
					result = s.writeWithLength (utf8); 
				}
				break;
			default :
				CCL_DEBUGGER ("Invalid type!\n")
				break;
			}

			if(!result)
				return false;
		}
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryArchive::writeObject (const Object& object)
{
	Streamer s (stream, kArchiveByteOrder);
	CString className = object.myClass ().getPersistentName ();
	if(!(s.write ((ArchiveTypeID)Variant::kObject) && s.writeWithLength (className)))
		return false;

	if(object.isClass (ccl_typeid<Attributes> ()))
		return writeAttributes ((Attributes&)object);
	else
	{
		Attributes attributes;
		if(!object.save (Storage (attributes, this)))
			return false;

		return writeAttributes (attributes);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryArchive::loadAttributes (ObjectID root, Attributes& attributes)
{
	Streamer s (stream, kArchiveByteOrder);
	FOURCC id = {0};
	int32 version = 0;
	if(!(s.read (id) && id == kArchiveID && s.read (version) && version == kArchiveVersion))
		return false;

	MutableCString name;
	if(!s.readWithLength (name) && name == root)
		return false;

	return readAttributes (attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BinaryArchive::readAttributes (Attributes& attributes)
{
	Streamer s (stream, kArchiveByteOrder);
	FOURCC id = {0};
	int32 count = 0;
	if(!(s.read (id) && id == kAttributesID && s.read (count)))
		return false;

	for(int i = 0; i < count; i++)
	{
		MutableCString name;
		if(!s.readWithLength (name))
			return false;

		ArchiveTypeID type = 0;
		if(!s.read (type))
			return false;

		if(type == kAttrQueueID)
		{
			int32 queueCount = 0;
			if(!s.read (queueCount))
				return false;

			AttributeQueue* queue = NEW AttributeQueue;
			attributes.set (name, queue, Attributes::kOwns);
			for(int queueIndex = 0; queueIndex < queueCount; queueIndex++)
			{
				Object* item = readObject (true); // type not yet read
				if(!item)
					return false;

				queue->addValue (static_cast<IObject*> (item), Attributes::kOwns);
			}
		}
		else if(type == Variant::kObject)
		{
			Object* object = readObject ();
			if(!object)
				return false;

			attributes.set (name, object, Attributes::kOwns);
		}
		else // primitive types
		{
			Variant value;
			bool result = false;
			switch(type)
			{
			case Variant::kInt : { int64 lValue = 0; result = s.read (lValue); value = lValue; } break;
			case Variant::kFloat : { double fValue = 0.; result = s.read (fValue); value = fValue; } break;
			case Variant::kString : { String string; result = s.readWithLength (string); value = string; value.share (); } break;
			case kUtf8StringID : 
				{
					MutableCString utf8;
					if(s.readWithLength (utf8))
					{
						result = true;
						String string; 
						string.appendCString (Text::kUTF8, utf8);
						value = string;
						value.share (); 
					}
				}
				break;
			default :
				CCL_DEBUGGER ("Invalid type!\n")
				break;
			}

			if(!result)
				return false;

			attributes.setAttribute (name, value);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* BinaryArchive::readObject (bool typeNeeded)
{
	Streamer s (stream, kArchiveByteOrder);

	if(typeNeeded)
	{
		ArchiveTypeID type = 0;
		if(!(s.read (type) && type == Variant::kObject))
			return nullptr;
	}

	MutableCString className;
	if(!(s.readWithLength (className) && !className.isEmpty ()))
		return nullptr;

	static const CString strArributes = ccl_typeid<Attributes> ().getPersistentName ();
	if(className == strArributes)
	{
		AutoPtr<Attributes> a = NEW Attributes;
		if(!readAttributes (*a))
			return nullptr;
		return a.detach ();
	}
	else
	{
		AutoPtr<Object> object = Kernel::instance ().getClassRegistry ().createObject (className);
		ASSERT (object != nullptr)
		if(!object)
			return nullptr;

		Attributes attributes;
		if(!readAttributes (attributes))
			return nullptr;

		if(!object->load (Storage (attributes, this)))
			return nullptr;

		return object.detach ();
	}
}

