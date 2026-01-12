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
// Filename    : ccl/public/textservices.h
// Description : Text Service APIs
//
//************************************************************************************************

#ifndef _ccl_textservices_h
#define _ccl_textservices_h

#include "ccl/public/cclexports.h"
#include "ccl/public/base/iunknown.h"
#include "ccl/public/text/textencoding.h"

namespace CCL {

interface IString;
interface ICString;
interface IUnicodeUtilities;
interface ITranslationTable;
interface IStringDictionary;
interface ICStringDictionary;
interface IXmlParser;
interface IXmlWriter;
interface IStream;
interface IDataTransformer;
interface ITextStreamer;
interface ITextWriter;
interface IAttributeHandler;
interface IRegularExpression;

namespace System {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Unicode String APIs
//////////////////////////////////////////////////////////////////////////////////////////////////
/** \addtogroup ccl_text
@{ */

/** Returns the empty string singleton */
CCL_EXPORT IString& CCL_API CCL_ISOLATED (GetEmptyString) ();
inline IString& GetEmptyString () { return CCL_ISOLATED (GetEmptyString) (); }

/** Returns string object for constant C-string (ASCII-encoded) */
CCL_EXPORT StringRef CCL_API CCL_ISOLATED (GetConstantString) (CStringPtr asciiString);
inline StringRef GetConstantString (CStringPtr asciiString) { return CCL_ISOLATED (GetConstantString) (asciiString); }

/** Returns the Unicode utilities singleton */
CCL_EXPORT IUnicodeUtilities& CCL_API CCL_ISOLATED (GetUnicodeUtilities) ();
inline IUnicodeUtilities& GetUnicodeUtilities () { return CCL_ISOLATED (GetUnicodeUtilities) (); }

/** Create translation table */
CCL_EXPORT ITranslationTable* CCL_API CCL_ISOLATED (CreateTranslationTable) ();
inline ITranslationTable* CreateTranslationTable () { return CCL_ISOLATED (CreateTranslationTable) (); }

/** Creates an empty string dictionary object */
CCL_EXPORT IStringDictionary* CCL_API CCL_ISOLATED (CreateStringDictionary) ();
inline IStringDictionary* CreateStringDictionary () { return CCL_ISOLATED (CreateStringDictionary) (); }

/** Parse string representation of Variant (integer, floating-point, or text) */
CCL_EXPORT tresult CCL_API CCL_ISOLATED (ParseVariantString) (Variant& result, StringRef string);
inline tresult ParseVariantString (Variant& result, StringRef string) { return CCL_ISOLATED (ParseVariantString) (result, string); }

/** Create regular expression instance */
CCL_EXPORT IRegularExpression* CCL_API CCL_ISOLATED (CreateRegularExpression) ();
inline IRegularExpression* CreateRegularExpression () { return CCL_ISOLATED (CreateRegularExpression) (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// C-String APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Create mutable C-String */
CCL_EXPORT ICString* CCL_API CCL_ISOLATED (CreateMutableCString) (CStringPtr text);
inline ICString* CreateMutableCString (CStringPtr text) { return CCL_ISOLATED (CreateMutableCString) (text); }

/** Get constant C-String */
CCL_EXPORT CStringRef CCL_API CCL_ISOLATED (GetConstantCString) (CStringPtr asciiString);
inline CStringRef GetConstantCString (CStringPtr asciiString) { return CCL_ISOLATED (GetConstantCString) (asciiString); }

/** Creates an empty C-String dictionary object */
CCL_EXPORT ICStringDictionary* CCL_API CCL_ISOLATED (CreateCStringDictionary) ();
inline ICStringDictionary* CreateCStringDictionary () { return CCL_ISOLATED (CreateCStringDictionary) (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// XML APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Creates a new XML parser instance */
CCL_EXPORT IXmlParser* CCL_API CCL_ISOLATED (CreateXmlParser) (tbool parseNamespaces);
inline IXmlParser* CreateXmlParser (tbool parseNamespaces) { return CCL_ISOLATED (CreateXmlParser) (parseNamespaces); }

/** Creates a new XML writer instance */
CCL_EXPORT IXmlWriter* CCL_API CCL_ISOLATED (CreateXmlWriter) ();
inline IXmlWriter* CreateXmlWriter () { return CCL_ISOLATED (CreateXmlWriter) (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// JSON APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Parse JSON stream, issues callbacks to given handler */
CCL_EXPORT tresult CCL_API CCL_ISOLATED (JsonParse) (IStream& srcStream, IAttributeHandler& handler);
inline tresult JsonParse (IStream& srcStream, IAttributeHandler& handler) { return CCL_ISOLATED (JsonParse) (srcStream, handler); }

/** Create handler for JSON stringification to given destination stream */
CCL_EXPORT IAttributeHandler* CCL_API CCL_ISOLATED (JsonStringify) (IStream& dstStream, int options = 0);
inline IAttributeHandler* JsonStringify (IStream& dstStream, int options = 0) { return CCL_ISOLATED (JsonStringify) (dstStream, options); }

/** Parse JSON5 stream, issues callbacks to given handler */
CCL_EXPORT tresult CCL_API CCL_ISOLATED (Json5Parse) (IStream& srcStream, IAttributeHandler& handler);
inline tresult Json5Parse (IStream& srcStream, IAttributeHandler& handler) { return CCL_ISOLATED (Json5Parse) (srcStream, handler); }

/** Create handler for JSON5 stringification to given destination stream */
CCL_EXPORT IAttributeHandler* CCL_API CCL_ISOLATED (Json5Stringify) (IStream& dstStream, int options = 0);
inline IAttributeHandler* Json5Stringify (IStream& dstStream, int options = 0) { return CCL_ISOLATED (Json5Stringify) (dstStream, options); }

/** Parse UBJSON stream, issues callbacks to given handler */
CCL_EXPORT tresult CCL_API CCL_ISOLATED (UBJsonParse) (IStream& srcStream, IAttributeHandler& handler);
inline tresult UBJsonParse (IStream& srcStream, IAttributeHandler& handler) { return CCL_ISOLATED (UBJsonParse) (srcStream, handler); }

/** Create handler for UBJSON serialization to given destination stream */
CCL_EXPORT IAttributeHandler* CCL_API CCL_ISOLATED (UBJsonWrite) (IStream& dstStream, int options = 0);
inline IAttributeHandler* UBJsonWrite (IStream& dstStream, int options = 0) { return CCL_ISOLATED (UBJsonWrite) (dstStream, options); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Transformation APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Creates new data transformer for encoding or decoding */
CCL_EXPORT IDataTransformer* CCL_API CCL_ISOLATED (CreateDataTransformer) (UIDRef cid, int mode);
inline IDataTransformer* CreateDataTransformer (UIDRef cid, int mode) { return CCL_ISOLATED (CreateDataTransformer) (cid, mode); }

/** Creates stream for reading or writing from/to a data stream using the specified transformer */
CCL_EXPORT IStream* CCL_API CCL_ISOLATED (CreateTransformStream) (IStream* dataStream, IDataTransformer* transformer, tbool writeMode);
inline IStream* CreateTransformStream (IStream* dataStream, IDataTransformer* transformer, tbool writeMode) { return CCL_ISOLATED (CreateTransformStream) (dataStream, transformer, writeMode); }

/** Calculate CRC-32 checksum */
CCL_EXPORT uint32 CCL_API CCL_ISOLATED (Crc32) (const void* key, uint32 length, uint32 initialValue);
inline uint32 Crc32 (const void* key, uint32 length, uint32 initialValue) { return CCL_ISOLATED (Crc32) (key, length, initialValue); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Text I/O APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Text streamer description for CreateTextStreamer(). */
struct TextStreamerDescription
{
	TextEncoding encoding = Text::kUnknownEncoding;
	TextLineFormat format = Text::kUnknownLineFormat;
	int options = 0;
};

/** Creates new text streamer for reading or writing from/to a data stream, with specified text encoding and line format */
CCL_EXPORT ITextStreamer* CCL_API CCL_ISOLATED (CreateTextStreamer) (IStream& dataStream, const TextStreamerDescription& description);
inline ITextStreamer* CreateTextStreamer (IStream& dataStream, const TextStreamerDescription& description = {}) { return CCL_ISOLATED (CreateTextStreamer) (dataStream, description); }

/** Creates new text writer for format specified by class identifier (XML, HTML, etc.) */
CCL_EXPORT ITextWriter* CCL_API CCL_ISOLATED (CreateTextWriter) (UIDRef cid);
inline ITextWriter* CreateTextWriter (UIDRef cid) { return CCL_ISOLATED (CreateTextWriter) (cid); }

/** Shortcut to create text writer via specialization for requested interface */
template <class Interface> Interface* CreateTextWriter (UIDRef cid = kNullUID)
{
	AutoPtr<IUnknown> writer = reinterpret_cast<IUnknown*> (CreateTextWriter (cid.isValid () ? cid : ccl_iid<Interface> ()));
	Interface* iface = nullptr;
	if(writer) writer->queryInterface (ccl_iid<Interface> (), (void**)&iface);
	return iface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/** @}*/

} // namespace System
} // namespace CCL

#endif // _ccl_textservices_h
