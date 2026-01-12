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
// Filename    : ccl/base/storage/jsonarchive.h
// Description : JSON / UBJSON Archive
//
//************************************************************************************************

#ifndef _ccl_jsonarchive_h
#define _ccl_jsonarchive_h

#include "ccl/base/storage/archive.h"

namespace CCL {

class FileType;
class AttributeQueue;
interface ITransformStream;

//************************************************************************************************
// JsonArchive
/** JSON Archive - http://json.org */
//************************************************************************************************

class JsonArchive: public Archive
{
public:
	JsonArchive (IStream& stream, Attributes* context = nullptr, StringID saveType = nullptr);
	JsonArchive (IStream& stream, int flags);

	DECLARE_STRINGID_MEMBER (kMimeType)
	static const FileType& getFileType ();
	static bool isJson (const void* data, uint32 length); ///< checks if first character looks like JSON

	enum Flags
	{
		kSuppressWhitespace = 1<<1,	///< suppress whitespace on JSON stringification (default is off)
		kTypeIDEnabled = 1<<2,		///< enable load/save of objects using typeid attribute (default is off)
		kKeepDuplicateKeys = 1<<3,	///< keep duplicate keys when parsing JSON (default is off)
		kLastJsonFlag = 3
	};

	PROPERTY_FLAG (flags, kSuppressWhitespace, isSuppressWhitespace)
	PROPERTY_FLAG (flags, kTypeIDEnabled, isTypeIDEnabled)
	PROPERTY_FLAG (flags, kKeepDuplicateKeys, isKeepDuplicateKeys)

	bool saveArray (const AttributeQueue& queue); ///< save array at top level without enclosing object

	// Archive
	ArchiveType getArchiveType () const override { return kJsonArchive; }
	bool isAnonymous () const override { return !isTypeIDEnabled (); }
	bool saveAttributes (ObjectID root, const Attributes& attributes) override;
	bool loadAttributes (ObjectID root, Attributes& attributes) override;

protected:
	class Reader;
	class Writer;

	DECLARE_STRINGID_MEMBER (kTypeIDAttr)

	virtual IAttributeHandler* prepareWrite () const;
};

//************************************************************************************************
// Json5Archive
/** JSON5 Archive - https://json5.org */
//************************************************************************************************

class Json5Archive: public JsonArchive
{
public:
	Json5Archive (IStream& stream, Attributes* context = nullptr, StringID saveType = nullptr);
	
	bool loadAttributes (ObjectID root, Attributes& attributes) override;
};

//************************************************************************************************
// UBJsonArchive
/** Universal Binary JSON Archive - http://ubjson.org */
//************************************************************************************************

class UBJsonArchive: public JsonArchive
{
public:
	UBJsonArchive (IStream& stream, Attributes* context = nullptr, StringID saveType = nullptr);

	DECLARE_STRINGID_MEMBER (kMimeType)
	static const FileType& getFileType ();

	enum Flags
	{
		kDoublePrecisionEnabled = 1<<(kLastJsonFlag+1) ///< enable double-precision floating point numbers (default is off)
	};

	PROPERTY_FLAG (flags, kDoublePrecisionEnabled, isDoublePrecisionEnabled)

	// JsonArchive
	ArchiveType getArchiveType () const override { return kUBJsonArchive; }
	bool loadAttributes (ObjectID root, Attributes& attributes) override;

protected:
	// JsonArchive
	IAttributeHandler* prepareWrite () const override;
};

//************************************************************************************************
// JsonUtils
/** JSON utilities */
//************************************************************************************************

struct JsonUtils
{
	/** Serialize attributes to JSON memory stream. */
	static IStream* serialize (const Attributes& a, int flags = JsonArchive::kSuppressWhitespace);

	/** Serialize attributes to JSON string. */
	static String toString (const Attributes& a, int flags = JsonArchive::kSuppressWhitespace);
	
	/** Parse attributes from JSON stream. */
	static bool parse (Attributes& a, IStream& s);

	/** Parse attributes from JSON string. */
	static bool parseString (Attributes& a, StringRef string);

	/** Convert UBJSON format to JSON. */
	static bool convertFromBinaryFormat (IStream& dest, IStream& source);

	/** Convert JSON format to UBJSON.  */
	static bool convertToBinaryFormat (IStream& dest, IStream& source);

	static IStream* convertStream (IStream& source, bool toBinary);
	static ITransformStream* createTransformStream (bool toBinary);

private:
	class TransformStream;
};

} // namespace CCL

#endif // _ccl_jsonarchive_h
