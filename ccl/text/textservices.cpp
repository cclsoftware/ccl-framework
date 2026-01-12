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
// Filename    : ccl/text/textservices.cpp
// Description : Text Service APIs
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "core/public/coreplatform.h"

#include "core/system/corethread.h"

#if !CCL_STATIC_LINKAGE
#include "ccl/main/cclmodmain.h"

#define INIT_IID  // make IID symbols for ccltext module
#include "ccl/public/system/iallocator.h" 
#include "ccl/public/system/threadsync.h"
#endif

#include "ccl/text/strings/unicodestring.h"
#include "ccl/text/strings/stringtable.h"
#include "ccl/text/strings/translationtable.h"
#include "ccl/text/strings/formatparser.h"
#include "ccl/text/strings/cstringbuffer.h"
#include "ccl/text/strings/regularexpression.h"
#include "ccl/text/strings/jsonhandler.h"

#include "ccl/text/xml/xmlparser.h"
#include "ccl/text/xml/xmlwriter.h"
#include "ccl/text/xml/xmlstringdict.h"

#include "ccl/text/transform/zlibcompression.h"
#include "ccl/text/transform/encodings/baseencoding.h"
#include "ccl/text/transform/transformstreams.h"
#include "ccl/text/transform/textstreamer.h"

#include "ccl/text/writer/htmlwriter.h"
#include "ccl/text/writer/plaintextwriter.h"

namespace CCL {
namespace System {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Globals
//////////////////////////////////////////////////////////////////////////////////////////////////

static UnicodeString* theEmptyString = nullptr;
static StringTable* theStringTable = nullptr;
static StringTable* theCStringTable = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Unicode String APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IString& CCL_API CCL_ISOLATED (GetEmptyString) ()
{
	if(!theEmptyString)
		theEmptyString = UnicodeString::newString ();
	return *theEmptyString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT StringRef CCL_API CCL_ISOLATED (GetConstantString) (CStringPtr asciiString)
{
	static Core::Threads::Lock theLock;
	Core::Threads::ScopedLock scopedLock (theLock);

	if(!theStringTable)
		theStringTable = NEW StringTable (2500);	

	UnicodeStringEntry* result = (UnicodeStringEntry*)theStringTable->lookup (asciiString);
	if(result == nullptr)
	{
		String theString;
		theString.writeEnable ();
		UnicodeString* unicodeString = castToString<UnicodeString> (reinterpret_cast<PlainString&> (theString).theString);
		ASSERT (unicodeString != nullptr)
		unicodeString->makeConstant (asciiString);

		result = NEW UnicodeStringEntry (asciiString, theString, StringEntry::kCopy);
		theStringTable->add (result);
	}
	return result->theString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IUnicodeUtilities& CCL_API CCL_ISOLATED (GetUnicodeUtilities) ()
{
	return UnicodeUtilities::getInstance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ITranslationTable* CCL_API CCL_ISOLATED (CreateTranslationTable) ()
{
	return NEW TranslationTable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IStringDictionary* CCL_API CCL_ISOLATED (CreateStringDictionary) ()
{
	return NEW XmlStringDictionary;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tresult CCL_API CCL_ISOLATED (ParseVariantString) (Variant& result, StringRef string)
{
	return FormatParser::parseVariant (result, string) ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IRegularExpression* CCL_API CCL_ISOLATED (CreateRegularExpression) ()
{
	return NEW RegularExpression;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// C-String APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ICString* CCL_API CCL_ISOLATED (CreateMutableCString) (CStringPtr text)
{
	return NEW CStringBuffer (text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT CStringRef CCL_API CCL_ISOLATED (GetConstantCString) (CStringPtr asciiString)
{
	static Core::Threads::Lock theLock;
	Core::Threads::ScopedLock scopedLock (theLock);

	if(!theCStringTable)
		theCStringTable = NEW StringTable (2500);

	CStringEntry* result = (CStringEntry*)theCStringTable->lookup (asciiString);
	if(result == nullptr)
	{
		MutableCString theCString (asciiString);
		result = NEW CStringEntry (theCString);
		theCStringTable->add (result);
	}
	return result->theCString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ICStringDictionary* CCL_API CCL_ISOLATED (CreateCStringDictionary) ()
{
	return NEW XmlCStringDictionary;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// XML APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IXmlParser* CCL_API CCL_ISOLATED (CreateXmlParser) (tbool parseNamespaces)
{
	return NEW XmlParser (parseNamespaces != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IXmlWriter* CCL_API CCL_ISOLATED (CreateXmlWriter) ()
{
	return NEW XmlWriter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// JSON APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tresult CCL_API CCL_ISOLATED (JsonParse) (IStream& srcStream, IAttributeHandler& handler)
{
	return JsonHandler::parse (srcStream, handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IAttributeHandler* CCL_API CCL_ISOLATED (JsonStringify) (IStream& dstStream, int options)
{
	return JsonHandler::stringify (dstStream, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tresult CCL_API CCL_ISOLATED (Json5Parse) (IStream& srcStream, IAttributeHandler& handler)
{
	return Json5Handler::parse (srcStream, handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IAttributeHandler* CCL_API CCL_ISOLATED (Json5Stringify) (IStream& dstStream, int options)
{
	return Json5Handler::stringify (dstStream, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tresult CCL_API CCL_ISOLATED (UBJsonParse) (IStream& srcStream, IAttributeHandler& handler)
{
	return JsonHandler::parseBinary (srcStream, handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IAttributeHandler* CCL_API CCL_ISOLATED (UBJsonWrite) (IStream& dstStream, int options)
{
	return JsonHandler::writeBinary (dstStream, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Transformation APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IDataTransformer* CCL_API CCL_ISOLATED (CreateDataTransformer) (UIDRef cid, int mode)
{
	IDataTransformer* transformer = nullptr;

	if(cid.equals (ClassID::ZlibCompression))
	{
		if(mode == IDataTransformer::kEncode)
			transformer = NEW ZlibEncoder;
		else if(mode == IDataTransformer::kDecode)
			transformer = NEW ZlibDecoder;
	}

	if(transformer == nullptr)
	{
		if(mode == IDataTransformer::kEncode)
			transformer = BaseEncoder::createInstance (cid);
		else if(mode == IDataTransformer::kDecode)
			transformer = BaseDecoder::createInstance (cid);
	}

	return transformer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IStream* CCL_API CCL_ISOLATED (CreateTransformStream) (IStream* dataStream, IDataTransformer* transformer, tbool writeMode)
{
	if(writeMode)
	{
		TransformWriter* writer = NEW TransformWriter;
		if(writer->open (transformer, dataStream) == kResultOk)
			return writer;
		else
			writer->release ();
	}
	else
	{
		TransformReader* reader = NEW TransformReader;
		if(reader->open (transformer, dataStream) == kResultOk)
			return reader;
		else
			reader->release ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT uint32 CCL_API CCL_ISOLATED (Crc32) (const void* key, uint32 length, uint32 initialValue)
{
	return (int)::crc32 (initialValue, (Bytef*)key, length);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Text I/O APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ITextStreamer* CCL_API CCL_ISOLATED (CreateTextStreamer) (IStream& dataStream, const TextStreamerDescription& description)
{
	return NEW TextStreamer (dataStream, description.encoding, description.format, description.options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ITextWriter* CCL_API CCL_ISOLATED (CreateTextWriter) (UIDRef cid)
{
	TextWriter* writer = nullptr;

	if(cid == ccl_iid<ITextWriter> ())
		writer = NEW TextWriter;
	else if(cid == ccl_iid<IXmlWriter> ())
		writer = NEW XmlWriter;
	else if(cid == ccl_iid<IHtmlWriter> ())
		writer = NEW HtmlWriter;
	else if(cid == ccl_iid<IPlainTextWriter> ())
		writer = NEW PlainTextWriter;

	return writer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Cleanup
//////////////////////////////////////////////////////////////////////////////////////////////////

struct TextFrameworkCleanup
{
	~TextFrameworkCleanup ()
	{
		safe_release (theEmptyString);

		if(theStringTable)
		{
			CCL_PRINTF ("\nConstant Unicode String Count: %d\n", theStringTable->count ());
			theStringTable->release ();
			theStringTable = nullptr;
		}

		if(theCStringTable)
		{
			CCL_PRINTF ("Constant C-String Count: %d\n", theCStringTable->count ());
			theCStringTable->release ();
			theCStringTable = nullptr;
		}
	}
};

static TextFrameworkCleanup theTextFrameworkCleanup;

//////////////////////////////////////////////////////////////////////////////////////////////////

#if !CCL_STATIC_LINKAGE
// stuff needed by Debugger::reportWarning() in debug.output.cpp:
ModuleRef GetCurrentModuleRef () { return nullptr; }
void CCL_API CCL_ISOLATED (DebugReportWarning) (ModuleRef module, StringRef message) {}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace System
} // namespace CCL

//////////////////////////////////////////////////////////////////////////////////////////////////
// Main Entry
//////////////////////////////////////////////////////////////////////////////////////////////////

#if !CCL_STATIC_LINKAGE
CCL_EXPORT CCL::tbool CCL_API CCLModuleMain (CCL::ModuleRef module, CCL::ModuleEntryReason reason)
{
	return true;
}
#endif
