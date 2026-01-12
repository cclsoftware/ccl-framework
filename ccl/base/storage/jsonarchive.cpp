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
// Filename    : ccl/base/storage/jsonarchive.cpp
// Description : JSON/ UBJSON Archive
//
//************************************************************************************************

#include "ccl/base/storage/jsonarchive.h"

#include "ccl/base/kernel.h"
#include "ccl/base/storage/storage.h"

#include "ccl/public/storage/filetype.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/base/idatatransformer.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// JsonObject / JsonArray
//************************************************************************************************

typedef Attributes JsonObject;
typedef AttributeQueue JsonArray;

//************************************************************************************************
// JsonArchive::Reader
//************************************************************************************************

class JsonArchive::Reader: public AttributesBuilder
{
public:
	Reader (JsonArchive& archive, Attributes& root, bool appendMode);
	~Reader ();

protected:
	JsonArchive& archive;

	void convertObjects (Attributes& a);
	bool convertToObject (Object*& resultObject, const Attributes& a);
};

//************************************************************************************************
// JsonArchive::Writer
//************************************************************************************************

class JsonArchive::Writer
{
public:
	Writer (JsonArchive& archive, IAttributeHandler& handler);

	bool write (const Attributes& root);
	bool write (const AttributeQueue& queue);

protected:
	JsonArchive& archive;
	IAttributeHandler& handler;

	bool writeObject (StringRef id, const JsonObject& object);
	bool writeArray (StringRef id, const JsonArray& a);
	bool writeValue (StringRef id, VariantRef value);
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// JsonArchive
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (JsonArchive, kMimeType, "application/json")
DEFINE_STRINGID_MEMBER_ (JsonArchive, kTypeIDAttr, "__typeid")

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& JsonArchive::getFileType ()
{
	return FileTypes::Json ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonArchive::isJson (const void* data, uint32 length)
{
	char firstChar = length >= 1 ? *reinterpret_cast<const char*> (data) : 0;
	return firstChar == '{' || firstChar == '[';
}

//////////////////////////////////////////////////////////////////////////////////////////////////

JsonArchive::JsonArchive (IStream& stream, Attributes* context, StringID saveType)
: Archive (stream, context, saveType)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

JsonArchive::JsonArchive (IStream& stream, int flags)
: Archive (stream)
{
	setFlags (flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeHandler* JsonArchive::prepareWrite () const
{
	int handlerFlags = 0;
	if(isSuppressWhitespace ())
		handlerFlags |= IAttributeHandler::kSuppressWhitespace;
	return System::JsonStringify (stream, handlerFlags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonArchive::saveArray (const AttributeQueue& queue)
{
	AutoPtr<IAttributeHandler> handler = prepareWrite ();
	return Writer (*this, *handler).write (queue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonArchive::saveAttributes (ObjectID root, const Attributes& attributes)
{
	ASSERT (root.isEmpty ())
	AutoPtr<IAttributeHandler> handler = prepareWrite ();
	return Writer (*this, *handler).write (attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonArchive::loadAttributes (ObjectID root, Attributes& attributes)
{
	ASSERT (root.isEmpty ())
	Reader reader (*this, attributes, isKeepDuplicateKeys ());
	return System::JsonParse (stream, reader) == kResultOk;
}

//************************************************************************************************
// Json5Archive
//************************************************************************************************

Json5Archive::Json5Archive (IStream& stream, Attributes* context, StringID saveType)
: JsonArchive (stream, context, saveType)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Json5Archive::loadAttributes (ObjectID root, Attributes& attributes)
{
	ASSERT (root.isEmpty ())
	Reader reader (*this, attributes, isKeepDuplicateKeys ());
	return System::Json5Parse (stream, reader) == kResultOk;
}

//************************************************************************************************
// UBJsonArchive
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (UBJsonArchive, kMimeType, "application/ubjson")

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& UBJsonArchive::getFileType ()
{
	return FileTypes::UBJson ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UBJsonArchive::UBJsonArchive (IStream& stream, Attributes* context, StringID saveType)
: JsonArchive (stream, context, saveType)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeHandler* UBJsonArchive::prepareWrite () const
{
	int handlerFlags = 0;
	if(isDoublePrecisionEnabled ())
		handlerFlags |= IAttributeHandler::kDoublePrecisionEnabled;
	return System::UBJsonWrite (stream, handlerFlags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UBJsonArchive::loadAttributes (ObjectID root, Attributes& attributes)
{
	ASSERT (root.isEmpty ())
	Reader reader (*this, attributes, isKeepDuplicateKeys ());
	return System::UBJsonParse (stream, reader) == kResultOk;
}

//************************************************************************************************
// JsonArchive::Reader
//************************************************************************************************

JsonArchive::Reader::Reader (JsonArchive& archive, Attributes& root, bool appendMode)
: AttributesBuilder (root, false, appendMode),
  archive (archive)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

JsonArchive::Reader::~Reader ()
{
	if(archive.isTypeIDEnabled ())
		convertObjects (root);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void JsonArchive::Reader::convertObjects (Attributes& parent)
{
	ForEachAttribute (parent, name, value)
		if(IUnknown* unk = value.asUnknown ())
		{
			if(Attributes* attributes = unknown_cast<Attributes> (unk))
			{
				convertObjects (*attributes); // recursion

				Object* object = nullptr;
				if(convertToObject (object, *attributes))
					parent.setAttribute (name, ccl_as_unknown (object), Attributes::kOwns);
			}
			else if(AttributeQueue* queue = unknown_cast<AttributeQueue> (unk))
			{
				ForEach (*queue, Attribute, queueItem)
					if(Attributes* childAttr = unknown_cast<Attributes> (queueItem->getValue ().asUnknown ()))
					{
						convertObjects (*childAttr); // recursion

						Object* object = nullptr;
						if(convertToObject (object, *childAttr))
							queueItem->set (ccl_as_unknown (object), Attributes::kOwns);
					}
				EndFor
			}
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonArchive::Reader::convertToObject (Object*& resultObject, const Attributes& attributes)
{
	MutableCString typeName = attributes.getCString (kTypeIDAttr);
	if(typeName.isEmpty ()) // no type information found
		return false;

	AutoPtr<Object> object = Kernel::instance ().getClassRegistry ().createObject (typeName);
	SOFT_ASSERT (object != nullptr, "JsonArchive failed to convert object!")
	if(object && object->load (Storage (const_cast<Attributes&> (attributes), &archive)))
		resultObject = object.detach ();
	return true;
}

//************************************************************************************************
// AttributesBuilder
//************************************************************************************************

AttributesBuilder::AttributesBuilder (Attributes& root, bool initState, bool appendMode)
: root (root),
  currentState (nullptr),
  appendMode (appendMode)
{
	if(initState) // make ready to write to root attributes
		pushState (&root);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributesBuilder::pushState (const State& state)
{
	stateStack.add (state);
	currentState = &stateStack.at (stateStack.count () -  1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributesBuilder::popState ()
{
	int lastIndex = stateStack.count () -  1;
	if(lastIndex >= 0)
	{
		stateStack.removeAt (lastIndex);
		currentState = lastIndex == 0 ? nullptr : &stateStack.at (lastIndex - 1);
	}
	else
		currentState = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AttributesBuilder::startObject (StringRef id)
{
	Attributes* object = nullptr;
	if(currentState)
	{
		object = NEW Attributes;
		if(currentState->isObject)
			currentState->setObjectValue (MutableCString (id), ccl_as_unknown (object), Attributes::kOwns, appendMode);
		else
			currentState->queue->addValue (object->asUnknown (), Attributes::kOwns);
	}
	else
		object = &root;

	pushState (State (object));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AttributesBuilder::endObject (StringRef id)
{
	popState ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AttributesBuilder::startArray (StringRef id)
{
	AttributeQueue* queue = NEW AttributeQueue;

	if(currentState)
	{
		if(currentState->isObject)
			currentState->setObjectValue (MutableCString (id), ccl_as_unknown (queue), Attributes::kOwns, appendMode);
		else
			currentState->queue->addValue (queue->asUnknown (), Attributes::kOwns);
	}
	else
	{
		// array on top level: make anonymous queue in root
		ASSERT (id.isEmpty ())
		root.set (nullptr, queue, Attributes::kOwns);
	}

	pushState (State (queue));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AttributesBuilder::endArray (StringRef id)
{
	popState ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AttributesBuilder::setValue (StringRef _id, VariantRef value)
{
	MutableCString id (_id);
	return setValue (id.str (), value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AttributesBuilder::setValue (CStringPtr id, VariantRef value)
{
	if(currentState)
	{
		if(currentState->isObject)
			currentState->setObjectValue (id, value, 0, appendMode);
		else
			currentState->queue->addValue (value);
	}
	return true;
}

//************************************************************************************************
// JsonArchive::Writer
//************************************************************************************************

JsonArchive::Writer::Writer (JsonArchive& archive, IAttributeHandler& handler)
: archive (archive),
  handler (handler)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonArchive::Writer::write (const Attributes& root)
{
	return writeObject (String::kEmpty, root);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonArchive::Writer::write (const AttributeQueue& queue)
{
	return writeArray (String::kEmpty, queue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonArchive::Writer::writeObject (StringRef id, const JsonObject& object)
{
	if(!handler.startObject (id))
		return false;

	ForEachAttribute (object, name, value)
		String nameString (name);
		if(!writeValue (nameString, value))
			return false;
	EndFor

	return handler.endObject (id) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonArchive::Writer::writeArray (StringRef id, const JsonArray& a)
{
	if(!handler.startArray (id))
		return false;

	ArrayForEachFast (a, Attribute, attr)
		if(!writeValue (String::kEmpty, attr->getValue ()))
			return false;
	EndFor

	return handler.endArray (id) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonArchive::Writer::writeValue (StringRef id, VariantRef value)
{
	bool result = true;
	if(value.getType () == Variant::kObject)
	{
		Object* object = unknown_cast<Object> (value);
		if(Attributes* attributes = ccl_cast<Attributes> (object))
		{
			result = writeObject (id, *attributes);
		}
		else if(AttributeQueue* queue = ccl_cast<AttributeQueue> (object))
		{
			result = writeArray (id, *queue);
		}
		else
		{
			if(object && archive.isTypeIDEnabled ())
			{
				Attributes objectAttr;
				if(object->save (Storage (objectAttr, &archive)))
				{
					objectAttr.set (kTypeIDAttr, object->myClass ().getPersistentName ());
					result = writeObject (id, objectAttr);
				}
				else
					result = false;
			}
			else
			{
				CCL_DEBUGGER ("Can't save CCL objects to JSON!\n")
				result = handler.setValue (id, Variant ()) != 0; // save as 'null'
			}
		}
	}
	else
		result = handler.setValue (id, value) != 0;

	return result;
}

//************************************************************************************************
// JsonUtils::TransformStream
//************************************************************************************************

class JsonUtils::TransformStream: public Unknown,
			                      public ITransformStream
{
public:
	TransformStream (bool toBinary);
	~TransformStream ();

	// ITransformStream
	void CCL_API setTargetStream (IStream* targetStream) override;
	void CCL_API flush () override;

	// IStream
	int CCL_API write (const void* buffer, int size) override {return memoryStream.write (buffer, size);}
	int CCL_API read (void* buffer, int size) override {return memoryStream.read (buffer, size);}
	int64 CCL_API tell () override {return memoryStream.tell ();}
	tbool CCL_API isSeekable () const override {return memoryStream.isSeekable ();}
	int64 CCL_API seek (int64 pos, int mode) override {return memoryStream.seek (pos, mode);}

	CLASS_INTERFACE2 (IStream, ITransformStream, Unknown)

private:
	MemoryStream memoryStream;
	SharedPtr<IStream> targetStream;
	bool toBinary;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

JsonUtils::TransformStream::TransformStream (bool toBinary)
: toBinary (toBinary)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

JsonUtils::TransformStream::~TransformStream ()
{
	ASSERT (!targetStream)
	if(targetStream)
		flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API JsonUtils::TransformStream::setTargetStream (IStream* s)
{
	targetStream = s;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API JsonUtils::TransformStream::flush ()
{
	if(targetStream)
	{
		IStream& source = memoryStream;
		IStream& target = *targetStream;

		bool success = false;
		if(toBinary)
			success = convertToBinaryFormat (target, source);
		else
			success = convertFromBinaryFormat (target, source);
		ASSERT (success || memoryStream.getBytesWritten () == 0)

		if(UnknownPtr<ITransformStream> transformTarget = &target)
			transformTarget->flush ();

		targetStream = nullptr;
	}
}

//************************************************************************************************
// JsonUtils
//************************************************************************************************

IStream* JsonUtils::serialize (const Attributes& a, int flags)
{
	AutoPtr<MemoryStream> memoryStream = NEW MemoryStream;
	JsonArchive (*memoryStream, flags).saveAttributes (nullptr, a);
	memoryStream->rewind ();
	return memoryStream.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String JsonUtils::toString (const Attributes& a, int flags)
{
	AutoPtr<IStream> s = serialize (a, flags);
	UnknownPtr<IMemoryStream> ms (s);
	ASSERT (ms.isValid ())
	String string;
	string.appendCString (Text::kUTF8, static_cast<CStringPtr> (ms->getMemoryAddress ()), int(ms->getBytesWritten ()));
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::parse (Attributes& a, IStream& s)
{
	s.rewind ();
	return JsonArchive (s).loadAttributes (nullptr, a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::parseString (Attributes& a, StringRef string)
{
	if(AutoPtr<IStream> s = System::GetFileUtilities ().createStringStream (string, Text::kUTF8, IFileUtilities::kSuppressByteOrderMark))
		return parse (a, *s);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::convertFromBinaryFormat (IStream& dest, IStream& source)
{
	source.rewind ();
	Attributes a;
	if(UBJsonArchive (source).loadAttributes (nullptr, a) == false)
		return false;
	if(JsonArchive (dest).saveAttributes (nullptr, a) == false)
		return false;
	dest.rewind ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool JsonUtils::convertToBinaryFormat (IStream& dest, IStream& source)
{
	source.rewind ();
	Attributes a;
	if(JsonArchive (source).loadAttributes (nullptr, a) == false)
		return false;
	if(UBJsonArchive (dest).saveAttributes (nullptr, a) == false)
		return false;
	dest.rewind ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* JsonUtils::convertStream (IStream& source, bool toBinary)
{
	AutoPtr<MemoryStream> memoryStream = NEW MemoryStream;
	if(toBinary)
	{
		if(convertToBinaryFormat (*memoryStream, source) == false)
			return nullptr;
	}
	else
	{
		if(convertFromBinaryFormat (*memoryStream, source) == false)
			return nullptr;
	}

	return memoryStream.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITransformStream* JsonUtils::createTransformStream (bool toBinary)
{
	return NEW TransformStream (toBinary);
}
