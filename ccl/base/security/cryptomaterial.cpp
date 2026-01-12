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
// Filename    : ccl/base/security/cryptomaterial.cpp
// Description : Cryptographical Material
//
//************************************************************************************************

#include "ccl/base/security/cryptomaterial.h"

#include "ccl/base/storage/storage.h"

#include "ccl/public/base/idatatransformer.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

namespace CCL {
namespace Security {
namespace Crypto {

//************************************************************************************************
// MaterialUtils
//************************************************************************************************

namespace MaterialUtils
{	
	bool transform (IStream& dstStream, BlockRef srcBlock, UIDRef cid, int mode)
	{
		AutoPtr<IDataTransformer> transformer = System::CreateDataTransformer (cid, mode);
		ASSERT (transformer)
		if(!transformer)
			return false;

		AutoPtr<IStream> transformStream = System::CreateTransformStream (&dstStream, transformer, true);
		ASSERT (transformStream)
		if(transformStream->write (srcBlock.data, srcBlock.length) != srcBlock.length)
			return false;
		
		transformStream.release ();
		return true;
	}

	BlockRef trimBlock (Block& block)
	{
		while(block.length > 0 && CString::isWhitespace (block.data[0]))
		{
			block.data++;
			block.length--;
		}
		while(block.length > 0 && CString::isWhitespace (block.data[block.length-1]))
			block.length--;
		
		return block;
	}

	bool encode (IStream& dstStream, BlockRef srcBlock, UIDRef cid)
	{
		return transform (dstStream, srcBlock, cid, IDataTransformer::kEncode);
	}

	bool decode (IStream& dstStream, BlockRef _srcBlock, UIDRef cid)
	{
		Block srcBlock (_srcBlock);		
		return transform (dstStream, trimBlock (srcBlock), cid, IDataTransformer::kDecode);
	}

	MutableCString encodeToCString (BlockRef srcBlock, UIDRef cid)
	{
		MutableCString string;

		MemoryStream encodedStream;
		if(encode (encodedStream, srcBlock, cid))
			string.append ((char*)encodedStream.getMemoryAddress (), encodedStream.getBytesWritten ());

		return string;
	}

	bool decodeCString (IStream& dstStream, CStringRef string, UIDRef cid)
	{
		return decode (dstStream, Block (string.str (), string.length ()), cid);
	}
}

} // namespace Crypto
} // namespace Security
} // namespace CCL

using namespace CCL;
using namespace Security;
using namespace Crypto;

//************************************************************************************************
// MaterialUtils
//************************************************************************************************

bool MaterialUtils::toBase64Stream (IStream& base64Stream, BlockRef binaryData, TextEncoding outputEncoding)
{
	if(Text::isUTF16Encoding (outputEncoding))
	{
		MutableCString base64Ascii (toBase64CString (binaryData));
		String base64Unicode (base64Ascii);
		int byteSize = base64Unicode.length () * sizeof(uchar);
		return base64Stream.write (StringChars (base64Unicode), byteSize) == byteSize;
	}
	else
		return encode (base64Stream, binaryData, ClassID::Base64Encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MaterialUtils::fromBase64Stream (IStream& binaryStream, BlockRef base64Data, TextEncoding inputEncoding)
{
	if(Text::isUTF16Encoding (inputEncoding))
	{
		String base64Unicode;
		base64Unicode.append (reinterpret_cast<const uchar*> (base64Data.data), base64Data.length / sizeof(uchar));
		MutableCString base64Ascii (base64Unicode);		
		return fromBase64CString (binaryStream, base64Ascii);
	}
	else
		return decode (binaryStream, base64Data, ClassID::Base64Encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString MaterialUtils::toBase64CString (BlockRef binaryData)
{
	return encodeToCString (binaryData, ClassID::Base64Encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MaterialUtils::fromBase64CString (IStream& binaryStream, CStringRef base64String)
{
	return decodeCString (binaryStream, base64String, ClassID::Base64Encoding);
}

//************************************************************************************************
// Crypto::Material
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Material, StorableObject, "CryptoMaterial")
DEFINE_CLASS_NAMESPACE (Material, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

Material::Material (uint32 size)
{
	if(size > 0)
		resize (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material::Material (BlockRef block)
{
	copyFrom (block);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material::Material (IStream& stream)
{
	copyFrom (stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material::Material (const Material& material)
{
	copyFrom (material.asBlock ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::operator = (const Material& material)
{
	copyFrom (material.asBlock ());
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Material::isEmpty () const
{
	return getSize () == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Material::getSize () const
{
	return material.getBytesWritten ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::empty ()
{
	material.rewind ();
	material.setBytesWritten (0);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::resize (uint32 size)
{
	material.allocateMemory (size, true); // allocate & zero
	material.setBytesWritten (size);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::copyFrom (BlockRef block)
{
	empty ();
	material.write (block.data, block.length);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::copyFrom (IStream& stream)
{
	empty ();
	AutoPtr<IMemoryStream> result = System::GetFileUtilities ().createStreamCopyInMemory (stream, &material);
	ASSERT (result == &material)
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::copyFrom (CStringRef string)
{
	empty ();
	material.write (string.str (), string.length ());
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::copyFrom (const Material& material)
{
	return copyFrom (material.asBlock ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::copyPart (const Material& other, int offset, int length)
{
	Block block = other.asBlock ();
	if(offset + length <= block.length)
	{
		block.data += offset;
		block.length = length;
		copyFrom (block);
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::append (CStringRef string)
{
	material.write (string.str (), string.length ());
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::append (StringRef string, TextEncoding encoding)
{
	ASSERT (Text::isValidCStringEncoding (encoding))
	MutableCString cString (string, encoding);
	return append (cString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::append (const Material& other)
{
	Block block = other.asBlock ();
	material.write (block.data, block.length);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::appendBytes (const void* data, int length)
{
	material.write (data, length);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Material::copyTo (IStream& dst) const
{
	return material.writeTo (dst);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Material::copyTo (MutableCString& string) const
{
	string.empty ();
	string.append ((char*)material.getMemoryAddress (), material.getBytesWritten ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Material::copyTo (String& string, TextEncoding encoding) const
{
	ASSERT (Text::isValidCStringEncoding (encoding))
	string.empty ();
	string.appendCString (encoding, (char*)material.getMemoryAddress (), material.getBytesWritten ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Material::moveTo (MemoryStream& memStream)
{
	memStream.allocateMemory (0);
	memStream.take (material);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Block Material::asBlock () const
{
	return Block (material.getMemoryAddress (), material.getBytesWritten ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream& Material::asStream ()
{
	material.rewind ();
	return material;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Material::toCHex () const
{
	return encode (CCL::ClassID::Base16Encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Material::toCBase32 () const
{
	return encode (CCL::ClassID::Base32Encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Material::toCBase64 () const
{
	return encode (CCL::ClassID::Base64Encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Material::toCBase64URL () const
{
	// https://tools.ietf.org/html/rfc4648#section-5
	// https://tools.ietf.org/html/rfc7515#appendix-C

	MutableCString s = toCBase64 (); // Regular base64 encoder
	int paddingStart = s.index ('='); // Remove any trailing '='s
	if(paddingStart != -1)
		s.truncate (paddingStart);
	s.replace ('+', '-'); // 62nd char of encoding
	s.replace ('/', '_'); // 63rd char of encoding
	return s;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::fromHex (CStringRef string)
{
	return decode (string, CCL::ClassID::Base16Encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::fromBase32 (CStringRef string)
{
	return decode (string, CCL::ClassID::Base32Encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::fromBase64 (CStringRef string)
{
	return decode (string, CCL::ClassID::Base64Encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::fromBase64URL (CStringRef string)
{
	// https://tools.ietf.org/html/rfc4648#section-5
	// https://tools.ietf.org/html/rfc7515#appendix-C

	MutableCString s (string);
	s.replace ('-', '+'); // 62nd char of encoding
	s.replace ('_', '/'); // 63rd char of encoding
	switch(s.length () % 4) // Pad with trailing '='s
	{
	case 0: break; // No pad chars in this case
	case 2: s += "=="; break; // Two pad chars
	case 3: s += "="; break; // One pad char
	default : CCL_DEBUGGER ("Illegal base64url string!\n") break;
	}
	return fromBase64 (s); // Standard base64 decoder
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Material::encode (UIDRef cid) const
{
	return MaterialUtils::encodeToCString (asBlock (), cid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material& Material::decode (CStringRef string, UIDRef cid)
{
	empty ();
	MaterialUtils::decodeCString (material, string, cid);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Material::getHashCode (int size) const
{
	Block block (asBlock ());
	int value = System::Hash (block.data, block.length, 0);
	return (value & 0x7FFFFFFF) % size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Material::equals (const Object& object) const
{
	const Material* other = ccl_cast<Material> (&object);
	if(other)
	{
		Block thisBlock = asBlock ();
		Block otherBlock = other->asBlock ();

		if(thisBlock.length == otherBlock.length)
		{
			if(thisBlock.length == 0) // both blocks empty
				return true;

			if(thisBlock.data && otherBlock.data)
				return ::memcmp (thisBlock.data, otherBlock.data, thisBlock.length) == 0;
		}
		return false;
	}
	else
		return SuperClass::equals (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Material::load (const Storage& storage)
{
	String string = storage.getAttributes ().getString ("material");
	fromBase64 (string);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Material::save (const Storage& storage) const
{
	storage.getAttributes ().set ("material", toBase64 ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Material::toString (String& string, int flags) const
{
	string = toHex ();
	return true;
}

//************************************************************************************************
// Crypto::RawMaterial
//************************************************************************************************

DEFINE_CLASS_HIDDEN (RawMaterial, Material)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RawMaterial::getFormat (FileType& format) const
{
	CCL_NOT_IMPL ("Crypt::RawMaterial::getFormat() not implemented!")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RawMaterial::save (IStream& stream) const
{
	return copyTo (stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RawMaterial::load (IStream& stream)
{
	copyFrom (stream);
	return true;
}
