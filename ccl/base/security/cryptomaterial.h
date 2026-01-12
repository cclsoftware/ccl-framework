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
// Filename    : ccl/base/security/cryptomaterial.h
// Description : Cryptographical Material
//
//************************************************************************************************

#ifndef _ccl_cryptomaterial_h
#define _ccl_cryptomaterial_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/cryptotypes.h"

#include "ccl/base/storage/storableobject.h"

namespace CCL {
namespace Security {
namespace Crypto {

//************************************************************************************************
// Crypto::Material
/** Load/save methods use XML Archive Format. */
//************************************************************************************************

class Material: public StorableObject
{
public:
	DECLARE_CLASS (Material, StorableObject)

	Material (uint32 size = 0);
	Material (BlockRef block);
	Material (IStream& stream);
	Material (const Material& material);

	Material& operator = (const Material& material);

	bool isEmpty () const;
	int getSize () const; ///< size in bytes
	int getBitCount () const { return getSize () * 8; }

	Material& empty ();
	Material& resize (uint32 size);
	Material& copyFrom (BlockRef block);
	Material& copyFrom (IStream& stream);
	Material& copyFrom (CStringRef string);
	Material& copyFrom (const Material& material);
	Material& copyPart (const Material& other, int offset, int length);
	Material& append (CStringRef string);
	Material& append (StringRef string, TextEncoding encoding);
	Material& append (const Material& other);
	Material& appendBytes (const void* data, int length);

	bool copyTo (IStream& stream) const;
	bool copyTo (MutableCString& string) const;
	bool copyTo (String& string, TextEncoding encoding) const;
	void moveTo (MemoryStream& memStream); ///< move data without copying

	Block asBlock () const;
	operator Block () const;
	IStream& asStream ();
	operator IStream& ();

	// Encoding
	String toHex () const;
	MutableCString toCHex () const;
	String toBase32 () const;
	MutableCString toCBase32 () const;
	String toBase64 () const;
	MutableCString toCBase64 () const;
	String toBase64URL () const; ///< Base 64 Encoding with URL and Filename Safe Alphabet
	MutableCString toCBase64URL () const;

	// Decoding
	Material& fromHex (StringRef string);
	Material& fromHex (CStringRef string);
	Material& fromBase32 (StringRef string);
	Material& fromBase32 (CStringRef string);
	Material& fromBase64 (StringRef string);
	Material& fromBase64 (CStringRef string);
	Material& fromBase64URL (StringRef string);
	Material& fromBase64URL (CStringRef string);

	// Object
	int getHashCode (int size) const override;
	bool equals (const Object& object) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
	bool toString (String& string, int flags = 0) const override;

protected:
	MemoryStream material;

	MutableCString encode (UIDRef cid) const;
	Material& decode (CStringRef string, UIDRef cid);
};

//************************************************************************************************
// Crypto::RawMaterial
/** Load/save methods use binary ASN.1. */
//************************************************************************************************

class RawMaterial: public Material
{
public:
	DECLARE_CLASS (RawMaterial, Material)

	// Material
	tbool CCL_API getFormat (FileType& format) const override;
	tbool CCL_API save (IStream& stream) const override;
	tbool CCL_API load (IStream& stream) override;
};

//************************************************************************************************
// Crypto::MaterialUtils
//************************************************************************************************

namespace MaterialUtils
{
	/** Convert binary data block to base64-encoded stream with given output text encoding. */
	bool toBase64Stream (IStream& base64Stream, BlockRef binaryData, TextEncoding outputEncoding = Text::kASCII);

	/** Convert base64-encoded data block in given input encoding back to binary stream. */
	bool fromBase64Stream (IStream& binaryStream, BlockRef base64Data, TextEncoding inputEncoding = Text::kASCII);

	/** Convert binary data block to base64-encoded ASCII string. */
	MutableCString toBase64CString (BlockRef binaryData);

	/** Convert base64-encoded ASCII string back to binary stream. */
	bool fromBase64CString (IStream& binaryStream, CStringRef base64String);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Material::operator IStream& ()
{ return asStream (); }

inline Material::operator Block () const
{ return asBlock (); }

inline String Material::toHex () const
{ return String (toCHex ()); }

inline String Material::toBase32 () const
{ return String (toCBase32 ()); }

inline String Material::toBase64 () const
{ return String (toCBase64 ()); }

inline String Material::toBase64URL () const
{ return String (toCBase64URL ()); }

inline Material& Material::fromHex (StringRef string)
{ return fromHex (MutableCString (string)); }

inline Material& Material::fromBase32 (StringRef string)
{ return fromBase32 (MutableCString (string)); }

inline Material& Material::fromBase64 (StringRef string)
{ return fromBase64 (MutableCString (string)); }

inline Material& Material::fromBase64URL (StringRef string)
{ return fromBase64URL (MutableCString (string)); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Crypto
} // namespace Security
} // namespace CCL

#endif // _ccl_cryptomaterial_h
