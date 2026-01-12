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
// Filename    : ccl/text/strings/jsonhandler.cpp
// Description : JSON Handler
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/text/strings/jsonhandler.h"

#include "ccl/text/strings/stringtable.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/base/unknown.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"

#include "core/text/corejsonhandler.h"

using namespace CCL;

//************************************************************************************************
// JsonHandler::HandlerDelegate
//************************************************************************************************

class JsonHandler::HandlerDelegate: public Core::AttributeHandler,
									public Core::Text::Json::ErrorHandler
{
public:
	HandlerDelegate (IAttributeHandler& handler)
	: handler (handler),
	  stringTable (128)
	{}

	// AttributeHandler
	void startObject (CStringPtr id, int flags) override	{ handler.startObject (getId (id)); }
	void endObject (CStringPtr id, int flags) override		{ handler.endObject (getId (id)); }
	void startArray (CStringPtr id, int flags) override		{ handler.startArray (getId (id)); }
	void endArray (CStringPtr id, int flags) override		{ handler.endArray (getId (id)); }

	void setValue (CStringPtr id, int64 value, int flags) override		{ handler.setValue (getId (id), value); }
	void setValue (CStringPtr id, double value, int flags) override		{ handler.setValue (getId (id), value); }
	void setValue (CStringPtr id, bool value, int flags) override		{ handler.setValue (getId (id), Variant (value, Variant::kBoolFormat)); }
	void setValue (CStringPtr id, CStringPtr value, int flags) override
	{
		String s;
		s.appendCString (Text::kUTF8, value);
		handler.setValue (getId (id), s);
	}

	void setNullValue (CStringPtr id, int flags) override { handler.setValue (getId (id), Variant ()); }

	// ErrorHandler
	void onError (int64 position, CStringPtr errorMessage) override
	{
		// TODO: delegate error upwards...
		CCL_PRINTF ("A JSON reading error occurred at position %" FORMAT_INT64 "d: %s\n", position, errorMessage)
	}

protected:
	IAttributeHandler& handler;
	StringTable stringTable;

	StringRef getId (CStringPtr id)
	{
		UnicodeStringEntry* result = static_cast<UnicodeStringEntry*> (stringTable.lookup (id));
		if(result == nullptr)
		{
			String s;
			s.appendCString (Text::kUTF8, id);
			result = NEW UnicodeStringEntry (id, s, StringEntry::kCopy);
			stringTable.add (result);
		}

		return result->theString;
	}
};

//************************************************************************************************
// JsonHandler::BaseWriter
//************************************************************************************************

class JsonHandler::BaseWriter: public Unknown,
							   public IAttributeHandler
{
public:
	BaseWriter (Core::AttributeHandler& writer)
	: writer (writer)
	{}

	class Identifier: public MutableCString
	{
	public:
		Identifier (StringRef id)
		: MutableCString (id, Text::kUTF8)
		{}
	};

	// TODO: better error handling needed here...

	// IAttributeHandler
	tbool CCL_API startObject (StringRef id) override	{ writer.startObject (Identifier (id)); return true; }
	tbool CCL_API endObject (StringRef id) override		{ writer.endObject (Identifier (id)); return true; }
	tbool CCL_API startArray (StringRef id) override	{ writer.startArray (Identifier (id)); return true; }
	tbool CCL_API endArray (StringRef id) override		{ writer.endArray (Identifier (id)); return true; }

	tbool CCL_API setValue (StringRef _id, VariantRef value) override
	{
		Identifier id (_id);
		return setValue (id, value);
	}

	tbool CCL_API setValue (CStringPtr id, VariantRef value) override
	{
		switch(value.getType ())
		{
		case Variant::kInt : 
			if(value.isBoolFormat ())
				writer.setValue (id, value.asBool ());
			else
				writer.setValue (id, value.asLargeInt ()); 
			break;
		case Variant::kFloat : writer.setValue (id, value.asDouble ()); break;
		case Variant::kString : writer.setValue (id, Identifier (value.asString ())); break;
		default : writer.setNullValue (id); break;
		}
		return true;
	}

	CLASS_INTERFACE (IAttributeHandler, Unknown)

protected:
	Core::AttributeHandler& writer;
};

//************************************************************************************************
// JsonHandler::TextWriter
//************************************************************************************************

class JsonHandler::TextWriter: public BaseWriter
{
public:
	TextWriter (IStream& _stream, int options)
	: stream (_stream),
	  textWriter (&stream),
	  BaseWriter (textWriter)
	{
		textWriter.setSuppressWhitespace ((options & kSuppressWhitespace) != 0);
	}

	~TextWriter ()
	{
		textWriter.flush ();
	}

protected:
	CoreStream stream;
	Core::Text::Json::Writer textWriter;
};

//************************************************************************************************
// JsonHandler::BinaryWriter
//************************************************************************************************

class JsonHandler::BinaryWriter: public BaseWriter
{
public:
	BinaryWriter (IStream& _stream, int options)
	: stream (_stream),
	  binaryWriter (&stream),
	  BaseWriter (binaryWriter)
	{
		binaryWriter.setDoublePrecisionEnabled ((options & kDoublePrecisionEnabled) != 0);
	}

protected:
	CoreStream stream;
	Core::Text::Json::BinaryWriter binaryWriter;
};

//************************************************************************************************
// JsonHandler
//************************************************************************************************

tresult JsonHandler::parse (IStream& srcStream, IAttributeHandler& handler)
{
	CoreStream streamReader (srcStream);
	HandlerDelegate handlerDelegate (handler);
	Core::Text::Json::Parser parser (&streamReader, &handlerDelegate, &handlerDelegate);
	if(!parser.parse ())
		return kResultFailed;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeHandler* JsonHandler::stringify (IStream& dstStream, int options)
{
	return NEW TextWriter (dstStream, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult JsonHandler::parseBinary (IStream& srcStream, IAttributeHandler& handler)
{
	CoreStream streamReader (srcStream);
	HandlerDelegate handlerDelegate (handler);
	Core::Text::Json::BinaryParser parser (&streamReader, &handlerDelegate, &handlerDelegate);
	if(!parser.parse ())
		return kResultFailed;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeHandler* JsonHandler::writeBinary (IStream& dstStream, int options)
{
	return NEW BinaryWriter (dstStream, options);
}

//************************************************************************************************
// Json5Handler
//************************************************************************************************

tresult Json5Handler::parse (IStream& srcStream, IAttributeHandler& handler)
{
	CoreStream streamReader (srcStream);
	HandlerDelegate handlerDelegate (handler);
	Core::Text::Json::Parser parser (&streamReader, &handlerDelegate, &handlerDelegate, true);
	if(!parser.parse ())
		return kResultFailed;
	return kResultOk;
}
