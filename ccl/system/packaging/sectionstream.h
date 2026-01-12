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
// Filename    : ccl/system/packaging/sectionstream.h
// Description : Section Stream
//
//************************************************************************************************

#ifndef _ccl_sectionstream_h
#define _ccl_sectionstream_h

#include "ccl/public/base/istream.h"
#include "ccl/public/base/unknown.h"
#include "ccl/public/system/icryptor.h"

namespace CCL {
namespace Threading {
interface ILockable; }

class Buffer;

//************************************************************************************************
// StreamAlias
/** Stream wrapping another stream. */
//************************************************************************************************

class StreamAlias: public Unknown,
				   public IStream
{
public:
	StreamAlias (IStream* innerStream = nullptr);

	// IStream
	int CCL_API read (void* buffer, int size) override;
	int CCL_API write (const void* buffer, int size) override;
	int64 CCL_API tell () override;
	tbool CCL_API isSeekable () const override;
	int64 CCL_API seek (int64 pos, int mode) override;

	CLASS_INTERFACE (IStream, Unknown)

protected:
	SharedPtr<IStream> innerStream;
};

//************************************************************************************************
// SectionStream
/** Stream representing a section of its source stream. */
//************************************************************************************************

class SectionStream: public StreamAlias
{
public:
	SectionStream (IStream* sourceStream = nullptr, 
				   int64 sourceOffset = 0, 
				   int64 sectionSize = 0,
				   int mode = kReadMode,
				   Threading::ILockable* lock = nullptr);
	~SectionStream ();

	PROPERTY_VARIABLE (int64, sourceOffset, SourceOffset)
	PROPERTY_VARIABLE (int64, sectionSize, SectionSize)

	IStream* getSourceStream () { return innerStream; }

	// StreamAlias
	int CCL_API read (void* buffer, int size) override;
	int CCL_API write (const void* buffer, int size) override;
	int64 CCL_API tell () override;
	int64 CCL_API seek (int64 pos, int mode) override;

protected:
	int mode;
	int64 seekPosition;
	Threading::ILockable* lock;

	int64 getMaxPosition ();
};

//************************************************************************************************
// Crc32Stream
/** Stream calculating CRC-32 checksum on the fly. */
//************************************************************************************************

class Crc32Stream: public StreamAlias
{
public:
	Crc32Stream (IStream* innerStream, int mode = kReadMode);

	PROPERTY_VARIABLE (uint32, crc32, Crc32)

	// StreamAlias
	int CCL_API read (void* buffer, int size) override;
	int CCL_API write (const void* buffer, int size) override;
	tbool CCL_API isSeekable () const override;
	int64 CCL_API seek (int64 pos, int mode) override;

protected:
	int mode;
};

//************************************************************************************************
// EncryptionStream
/** Note: We cannot use data transformers here, because they would not be seekable. */
//************************************************************************************************

class EncryptionStream: public StreamAlias
{
public:
	EncryptionStream (IStream* innerStream);
	~EncryptionStream ();

	static Security::Crypto::ICryptoFactory* factoryInstance; // set externally

	// StreamAlias
	int CCL_API read (void* buffer, int size) override;
	int CCL_API write (const void* buffer, int size) override;
	int64 CCL_API tell () override;
	int64 CCL_API seek (int64 pos, int mode) override;

protected:
	int64 byteCounter;
	Buffer* writeBuffer;

	void* getWriteBuffer (int size);
	virtual void encrypt (uint8* dst, const uint8* src, int size) = 0;
};

//************************************************************************************************
// BasicEncryptionStream
/** Uses a very simple (unsafe) cipher algorithm. */
//************************************************************************************************

class BasicEncryptionStream: public EncryptionStream
{
public:
	BasicEncryptionStream (IStream* innerStream, const uint8 key[16]);
	
protected:
	static const int kBufferSize = 8 * 1024;

	uint8 key[16];
	uint8 counterBuffer[kBufferSize];
	uint8 keyBuffer[kBufferSize];
	
	AutoPtr<Security::Crypto::IProcessor> xorProcesor;
	
	void encrypt (uint8* dst, const uint8* src, int size) override;
};

//************************************************************************************************
// AdvancedEncryptionStream
/**	Base class for more advanced cipher algorithms. 
	Bytes from the data stream are xor'd with bytes from the key stream. */
//************************************************************************************************

class AdvancedEncryptionStream: public EncryptionStream
{
public:
	AdvancedEncryptionStream (IStream* innerStream, int blockSize);
	
protected:
	template <int dataSize>
	struct BlockBuffer
	{
		BlockBuffer ()
		: size (dataSize),
		  startOffset (0)
		{}

		bool isValid (int64 position) const
		{
			return (position >= startOffset && position < startOffset + size);
		}
		
		int size;
		int64 startOffset;
		uint8 data[dataSize];
	};
	
	static const int kBufferSize = 8 * 1024;

	int blockSize;
	BlockBuffer<kBufferSize> blockBuffer;
	AutoPtr<Security::Crypto::IProcessor> xorProcesor;

	void encrypt (uint8* dst, const uint8* src, int size) override;

	virtual void generateKeystreamBlocks (int64 streamPos) = 0;	
};

//************************************************************************************************
// XTEAEncryptionStream
/**	Uses XTEA (Extended Tiny Encryption Algorithm) block cipher. 
	http://en.wikipedia.org/wiki/XTEA */
//************************************************************************************************

class XTEAEncryptionStream: public AdvancedEncryptionStream
{
public:
	XTEAEncryptionStream (IStream* innerStream, const uint8 key[16], int64 nonce);

protected:
	enum { kBlockSize = 8, kNumRounds = 32 };
	
	uint32 key[4];
	int64 nonce;

	// AdvancedEncryptionStream
	void generateKeystreamBlocks (int64 streamPos) override;
};

//************************************************************************************************
// AESEncryptionStream, fixed block size of 128 bit
//************************************************************************************************

class AESEncryptionStream: public AdvancedEncryptionStream
{
public:
	AESEncryptionStream (IStream* innerStream, const uint8 key[16], int64 nonce);
	
protected:
	AutoPtr<Security::Crypto::ICryptor> cryptor;
	int64 nonce;
	
	// AdvancedEncryptionStream
	void generateKeystreamBlocks (int64 streamPos) override;
};

} // namespace CCL

#endif // _ccl_sectionstream_h
