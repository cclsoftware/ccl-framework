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
// Filename    : ccl/base/storage/xmlarchive.cpp
// Description : XML Archive
//
//************************************************************************************************

#define SAFE_STRING_ATTRIBUTE 1	// escape ambiguous string attributes

#define DEBUG_LOG 0

#include "ccl/base/storage/xmlarchive.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/xmlpihandler.h"
#include "ccl/base/kernel.h"

#include "ccl/public/collections/stack.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/storage/filetype.h"

#include "ccl/public/text/xmlcontentparser.h"
#include "ccl/public/text/ixmlwriter.h"
#include "ccl/public/text/istringdict.h"

#include "ccl/public/cclversion.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Reserved XML Identifiers
//////////////////////////////////////////////////////////////////////////////////////////////////

static const String kNamespaceID = CCLSTR ("xmlns:x");
static const String kNamespaceURI = CCLSTR (CCL_PRODUCT_WEBSITE "/xml");

static const String kListID = CCLSTR ("List");
static const String kObjectID = CCLSTR ("x:id");
static const String kCharDataID = CCLSTR ("CDATA");

static StringRef kDataID = CCLSTR ("Data");
static StringID kDataValueID = CSTR ("value");
static const String kDataValueIDStr (kDataValueID);

#if SAFE_STRING_ATTRIBUTE
static const uchar kStringEscapeChar = 0xFFF9; // Interlinear Annotation Anchor
static const uchar kStringEscapeSequence[2] = {kStringEscapeChar, 0};
static const String kStringEscapeLiteral (kStringEscapeSequence);
#endif

//************************************************************************************************
// XmlArchiveParser
//************************************************************************************************

class XmlArchiveParser: public XmlContentParser,
						public XmlProcessingInstructionHandler

{
public:
	XmlArchiveParser (XmlArchive& archive, XmlArchive::ObjectID rootTag);
	~XmlArchiveParser ();

	void pushFirst (Attributes* a);
	bool parse ();

	// XmlContentParser
	tresult CCL_API startElement (StringRef name, const IStringDictionary& attributes) override; 
	tresult CCL_API endElement (StringRef name) override;
	tresult CCL_API characterData (const uchar* data, int length, tbool isCDATA) override;
	tresult CCL_API processingInstruction (StringRef target, StringRef data) override;

protected:
	XmlArchive& archive;
	String rootTag;

	enum ParserState { kNowhere, kRoot, kObject, kList };

	struct State
	{
		ParserState type;
		Attributes* attributes;
		String className;
		String id;

		State (ParserState type, Attributes* a = nullptr)
		: type (type),
		  attributes (a)
		{}

		~State ()
		{
			if(attributes)
				attributes->release ();
		}

		Attributes& getAttributes ()
		{
			if(!attributes)
				attributes = NEW Attributes;
			return *attributes;
		}
	};

	Stack<State*> stack;

	void convertAttributes (Attributes& dst, const IStringDictionary& xmlAttributes);
};

//************************************************************************************************
// XmlArchive
//************************************************************************************************

const CString XmlArchive::defaultRootTag = "CCL.XmlArchive";

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& XmlArchive::getFileType ()
{
	return FileTypes::Xml ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlArchive::XmlArchive (IStream& stream, Attributes* context, StringID saveType)
: Archive (stream, context, saveType)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlArchive::saveAttributes (ObjectID root, const Attributes& attributes)
{
	AutoPtr<IXmlWriter> writer = System::CreateXmlWriter ();
	if(writer->beginDocument (getStream (), Text::kUTF8) != kResultOk)
		return false;

	bool result = writeAttributes (String (root), String::kEmpty, attributes, *writer);
	if(result)
		result = writer->endDocument () == kResultOk;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlArchive::loadAttributes (ObjectID root, Attributes& attributes)
{
	XmlArchiveParser parser (*this, root);
	parser.setSilentOnErrors (silentOnErrors ());
	parser.pushFirst (&attributes);
	bool result = parser.parse ();
	if(result == false && dontFailOnXmlError ())
		result = true;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlArchive::writeAttributes (StringRef tagName, StringRef objectName, const Attributes& attributes, IXmlWriter& writer)
{
	// collect simple attributes
	AutoPtr<IStringDictionary> tagAttributes = System::CreateStringDictionary ();
	if(defineNamespace () && writer.getCurrentDepth () == 0)
		tagAttributes->appendEntry (kNamespaceID, kNamespaceURI);
	if(!objectName.isEmpty ())
		tagAttributes->appendEntry (kObjectID, objectName);

	int numObjects = 0;
	ForEachAttribute (attributes, name, value)
		if(value.getType () == Variant::kObject)
			numObjects++;
		else
		{
			String valueString;
			value.toString (valueString);

			#if SAFE_STRING_ATTRIBUTE
			if(value.getType () == Variant::kString)
			{
				// check if backwards conversion would be ambiguous...
				Variant test;
				test.fromString (valueString);
				if(test.getType () != Variant::kString)
				{
					String testString;
					test.toString (testString);
					if(testString != valueString)
						valueString.prepend (kStringEscapeLiteral);
				}
			}
			#endif

			tagAttributes->appendEntry (String (name), valueString);
		}
	EndFor

	// save sub-objects...
	bool result = true;
	if(numObjects > 0)
	{
		if(writer.startElement (tagName, tagAttributes) != kResultOk)
			return false;

		ForEachAttribute (attributes, cname, value)
			if(value.getType () == Variant::kObject)
			{
				String name (cname);
				AttributeQueue* list = unknown_cast<AttributeQueue> (value);
				if(list)
					result = saveList (name, *list, writer);
				else
				if(name == kCharDataID)
				{
					UnknownPtr<IStream> stream (value);
					ASSERT (stream != nullptr)
					result = stream && writer.characterData (*stream, charDataUTF8 () ? Text::kUTF8 : Text::kUnknownEncoding) == kResultOk;
				}
				else if(value.asUnknown ())
					result = saveObject (name, value, writer);

				if(!result)
					break;
			}
		EndFor

		if(result)
			result = writer.endElement (tagName) == kResultOk;
	}
	else
		result = writer.writeElement (tagName, tagAttributes) == kResultOk;

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlArchive::saveList (StringRef name, const Container& list, IXmlWriter& writer)
{
	if(list.isEmpty ())
		return true;

	if(!name.isEmpty ())
	{
		AutoPtr<IStringDictionary> tagAttributes = System::CreateStringDictionary ();
		tagAttributes->appendEntry (kObjectID, name);
		if(writer.startElement (kListID, tagAttributes) != kResultOk)
			return false;
	}

	bool result = true;
	ForEach (list, Object, obj)
		Attribute* attr = ccl_cast<Attribute> (obj);
		if(attr == nullptr)
			return false;

		result = saveData (String::kEmpty, attr->getValue (), writer);

		if(!result)
			break;
	EndFor

	if(result && !name.isEmpty ())
		result = writer.endElement (kListID) == kResultOk;

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlArchive::saveObject (StringRef objectName, IUnknown* unknown, IXmlWriter& writer)
{
	Object* obj = unknown_cast<Object> (unknown);
	ASSERT (obj != nullptr)
	if(!obj)
		return false;

	// try to write directly...
	if(obj->isClass (ccl_typeid<Attributes> ()))
	{
		String tagName = obj->myClass ().getPersistentName ();
		return writeAttributes (tagName, objectName, *(Attributes*)obj, writer);
	}

	// ...or get object attributes
	Attributes attributes;
	if(!obj->save (Storage (attributes, this)))
		return false;

	String tagName = obj->myClass ().getPersistentName ();
	return writeAttributes (tagName, objectName, attributes, writer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlArchive::saveData (StringRef name, VariantRef toSave, IXmlWriter& writer)
{
	if(toSave.isObject ())
		return saveObject (String::kEmpty, toSave.asUnknown (), writer);

	String valueString;
	toSave.toString (valueString);

	AutoPtr<IStringDictionary> tagAttributes = System::CreateStringDictionary ();
	tagAttributes->appendEntry (kDataValueIDStr, valueString);

	return writer.writeElement (kDataID, tagAttributes) == kResultOk;
}

//************************************************************************************************
// XmlArchiveParser
//************************************************************************************************

XmlArchiveParser::XmlArchiveParser (XmlArchive& archive, XmlArchive::ObjectID rootTag)
: archive (archive),
  rootTag (rootTag)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlArchiveParser::~XmlArchiveParser ()
{
	State* state;
	while((state = stack.pop ()) != nullptr)
		delete state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlArchiveParser::pushFirst (Attributes* a)
{
	stack.push (NEW State (kNowhere, a));
	a->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlArchiveParser::parse ()
{
	return xmlParser->parse (archive.getStream ()) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlArchiveParser::convertAttributes (Attributes& dst, const IStringDictionary& xmlAttributes)
{
	int count = xmlAttributes.countEntries ();
	for(int i = 0; i < count; i++)
	{
		StringRef xmlName = xmlAttributes.getKeyAt (i);
		StringRef xmlValue = xmlAttributes.getValueAt (i);
		if(xmlName == kObjectID)
			continue;

		Variant value;
		#if SAFE_STRING_ATTRIBUTE
		if(xmlValue.firstChar () == kStringEscapeChar)
		{
			String unescaped = xmlValue.subString (1);
			value = unescaped;
			value.share ();
		}
		else
		#endif
			value.fromString (xmlValue);

		dst.setAttribute (MutableCString (xmlName), value, Attributes::kTemp);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlArchiveParser::startElement (StringRef name, const IStringDictionary& xmlAttributes)
{
	if(skipping)
		return kResultOk;

	State* current = stack.peek ();
	State* state = nullptr;

	if(current->type == kNowhere)
	{
		if(name != rootTag)
			return kResultFalse;

		state = NEW State (kRoot);

		state->attributes = &current->getAttributes ();
		state->attributes->retain ();

		convertAttributes (current->getAttributes (), xmlAttributes);
	}
	else
	{
		if(name == kListID)
		{
			state = NEW State (kList);
			state->attributes = &current->getAttributes ();
			state->attributes->retain ();
		}
		else
		{
			state = NEW State (kObject);
			state->className = name;
			convertAttributes (state->getAttributes (), xmlAttributes);
		}

		state->id = xmlAttributes.lookupValue (kObjectID);
	}

	stack.push (state);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlArchiveParser::endElement (StringRef name)
{
	if(skipping)
		return kResultOk;

	State* state = stack.pop ();
	State* parent = stack.peek ();

	if(state && parent)
	{
		if(state->type == kObject)
		{
			Object* object = nullptr;

			static const String strArributes = ccl_typeid<Attributes> ().getPersistentName ();
			if(state->className == strArributes)
			{
				object = &state->getAttributes ();
				object->retain ();
			}
			else if(state->className == kDataID)
			{
				Variant data = state->getAttributes ().getVariant (kDataValueID);
				if(parent->type == kList)
					parent->getAttributes ().queueAttribute (MutableCString (parent->id), data);
				else
					parent->getAttributes ().queueAttribute (nullptr, data);
			}
			else
			{
				MutableCString className (state->className);
				object = Kernel::instance ().getClassRegistry ().createObject (className);
				if(object)
				{
					if(!object->load (Storage (state->getAttributes (), &archive)))
					{
						object->release ();
						object = nullptr;
					}
				}
				else
				{
					SOFT_ASSERT (className.isEmpty (), "XmlArchive class not found!\n")
				}
			}

			if(object)
			{
				int flags = Attributes::kOwns;

				if(parent->type == kList || state->id.isEmpty ())
				{
					if(parent->type == kList)
						parent->getAttributes ().queue (MutableCString (parent->id), object, flags);
					else
						parent->getAttributes ().queue (nullptr, object, flags);
				}
				else
					parent->getAttributes ().set (MutableCString (state->id), object, flags);
			}
		}

		delete state;
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlArchiveParser::characterData (const uchar* data, int length, tbool isCDATA)
{
	if(skipping)
		return kResultOk;

	State* current = stack.peek ();
	if(current && isCDATA)
	{
		Attributes& a = current->getAttributes ();

		UnknownPtr<IStream> stream (a.getUnknown ("CDATA"));
		if(!stream)
		{
			IMemoryStream* newStream = NEW MemoryStream;
			a.set ("CDATA", newStream, Attributes::kOwns);
			stream = newStream;
		}

		if(!stream)
			return kResultOk;		

		if(archive.charDataUTF8 ())
		{
			// hmm... use string class for UTF-16 to UTF-8 conversion
			MutableCString utf8 (String ().append (data, length), Text::kUTF8);
			stream->write (utf8.str (), utf8.length ());
		}
		else
			stream->write (data, length * sizeof(uchar));
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlArchiveParser::processingInstruction (StringRef target, StringRef data)
{
	XmlProcessingInstructionHandler::handleInstruction (target, data);
	return kResultOk;
}

//************************************************************************************************
// XmlArchiveUtils
//************************************************************************************************

void XmlArchiveUtils::setCharDataFromString (Attributes& a, StringRef string)
{
	StringChars stringChars (string);
	const uchar* charBuffer = static_cast<const uchar*> (stringChars);
	int charBufferSize = string.length () * sizeof(uchar);

	AutoPtr<MemoryStream> charData = NEW MemoryStream;
	charData->write (charBuffer, charBufferSize);
	a.set ("CDATA", static_cast<IMemoryStream*> (charData), Attributes::kShare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlArchiveUtils::getStringFromCharData (String& string, const Attributes& a)
{
	string.empty ();
	UnknownPtr<IMemoryStream> charData (a.getUnknown ("CDATA"));
	if(!charData.isValid ())
		return false;

	const uchar* charBuffer = reinterpret_cast<const uchar*> (charData->getMemoryAddress ());
	int charCount = static_cast<int> (charData->getBytesWritten () / sizeof(uchar));
	string.append (charBuffer, charCount);
	return true;
}

} // namespace CCL
