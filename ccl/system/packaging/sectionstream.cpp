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
// Filename    : ccl/system/packaging/sectionstream.cpp
// Description : Section Stream
//
//************************************************************************************************

#include "ccl/system/packaging/sectionstream.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/textservices.h" // for CRC-32
#include "ccl/public/system/ilockable.h"

using namespace CCL;
using namespace Security::Crypto;

//************************************************************************************************
// StreamAlias
//************************************************************************************************

StreamAlias::StreamAlias (IStream* innerStream)
: innerStream (innerStream)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API StreamAlias::read (void* buffer, int size)
{
	return innerStream->read (buffer, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API StreamAlias::write (const void* buffer, int size)
{
	return innerStream->write (buffer, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API StreamAlias::tell ()
{
	return innerStream->tell ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StreamAlias::isSeekable () const
{
	return innerStream->isSeekable ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API StreamAlias::seek (int64 pos, int mode)
{
	return innerStream->seek (pos, mode);
}

//************************************************************************************************
// SectionStream
//************************************************************************************************

SectionStream::SectionStream (IStream* sourceStream, int64 sourceOffset, int64 sectionSize, int mode, Threading::ILockable* lock)
: StreamAlias (sourceStream),
  sourceOffset (sourceOffset),
  sectionSize  (sectionSize),
  seekPosition (0),
  mode (mode),
  lock (return_shared (lock))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SectionStream::~SectionStream ()
{
	safe_release (lock);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SectionStream::read (void* buffer, int size)
{
	// limit read to section size...
	int64 maxBytesToRead = getMaxPosition () - seekPosition;
	size = (int)ccl_min<int64> (size, maxBytesToRead);
	if(size <= 0)
		return 0;

	Threading::AutoLock autoLock (lock);

	if(innerStream->tell () != sourceOffset + seekPosition)
	{
		ASSERT (innerStream->isSeekable ())
		innerStream->seek (sourceOffset + seekPosition, kSeekSet);
	}

	int numBytesRead = innerStream->read (buffer, size);
	if(numBytesRead < 0) // an error occurred!
		return numBytesRead;

	seekPosition += numBytesRead;
	return numBytesRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SectionStream::write (const void* buffer, int size)
{
	ASSERT ((mode & kWriteMode) != 0)
	if((mode & kWriteMode) == 0) // write not allowed!
		return -1;

	// limit write to section size...
	int64 maxBytesToWrite = getMaxPosition () - seekPosition;
	size = (int)ccl_min<int64> (size, maxBytesToWrite);
	if(size <= 0)
		return 0;

	Threading::AutoLock autoLock (lock);

	if(innerStream->tell () != sourceOffset + seekPosition)
	{
		ASSERT (innerStream->isSeekable ())
		innerStream->seek (sourceOffset + seekPosition, kSeekSet);
	}

	int numBytesWritten = innerStream->write (buffer, size);
	if(numBytesWritten < 0) // an error occurred!
		return numBytesWritten;

	seekPosition += numBytesWritten;
	return numBytesWritten;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API SectionStream::tell ()
{
	return seekPosition;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API SectionStream::seek (int64 pos, int mode)
{
	ASSERT (innerStream->isSeekable ())
	
	switch(mode)
	{
	case kSeekSet :
		seekPosition = pos;
		break;

	case kSeekCur :
		seekPosition += pos;
		break;

	case kSeekEnd :
		seekPosition = getMaxPosition () + pos; // pos should be negative!
		break;
	}
		
	seekPosition = ccl_bound<int64> (seekPosition, 0, getMaxPosition ());
	
	return seekPosition;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 SectionStream::getMaxPosition ()
{
	if(sectionSize >= 0)
		return sectionSize;
	else
	{
		int64 prevPos = innerStream->tell ();
		int64 maxPos = innerStream->seek (0, kSeekEnd) - sourceOffset;
		innerStream->seek (prevPos, kSeekSet);
		return maxPos;
	}
}

//************************************************************************************************
// Crc32Stream
//************************************************************************************************

Crc32Stream::Crc32Stream (IStream* innerStream, int mode)
: StreamAlias (innerStream),
  mode (mode),
  crc32 (0)
{
	// initial value
	crc32 = System::Crc32 (nullptr, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Crc32Stream::read (void* buffer, int size)
{
	int numRead = innerStream->read (buffer, size);
	
	if(numRead > 0)
		if(mode == kReadMode)
			crc32 = System::Crc32 (buffer, numRead, crc32);

	return numRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Crc32Stream::write (const void* buffer, int size)
{
	if(size > 0)
		if(mode == kWriteMode)
			crc32 = System::Crc32 (buffer, size, crc32);

	return innerStream->write (buffer, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Crc32Stream::isSeekable () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API Crc32Stream::seek (int64 pos, int mode)
{
	CCL_DEBUGGER ("CRC-32 not seekable!\n")
	return innerStream->tell ();
}

//************************************************************************************************
// EncryptionStream
//************************************************************************************************

Security::Crypto::ICryptoFactory* EncryptionStream::factoryInstance = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

EncryptionStream::EncryptionStream (IStream* innerStream)
: StreamAlias (innerStream),
  writeBuffer (nullptr),
  byteCounter (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

EncryptionStream::~EncryptionStream ()
{
	if(writeBuffer)
		writeBuffer->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* EncryptionStream::getWriteBuffer (int size)
{
	if(writeBuffer == nullptr)
		writeBuffer = NEW Buffer;

	if((int)writeBuffer->getSize () < size)
		writeBuffer->resize (size);

	return writeBuffer->getAddress ();
}


//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API EncryptionStream::read (void* buffer, int size)
{
	int numRead = innerStream->read (buffer, size);
	if(numRead > 0)
		encrypt ((uint8*)buffer, (const uint8*)buffer, numRead);
	return numRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API EncryptionStream::write (const void* buffer, int size)
{
	void* temp = getWriteBuffer (size);
	encrypt ((uint8*)temp, (const uint8*)buffer, size);
	return innerStream->write (temp, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API EncryptionStream::tell ()
{
	// Inner stream can have data preceding the section we are currently working on,
	// so we report the local byte counter instead.
	return byteCounter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API EncryptionStream::seek (int64 pos, int mode)
{
	// See tell(): This works for our purpose if an additional SectionStream is involved,
	// or if seek() is not called while working on a stream with preceding data.
	return byteCounter = innerStream->seek (pos, mode);
}

//************************************************************************************************
// BasicEncryptionStream
//************************************************************************************************

BasicEncryptionStream::BasicEncryptionStream (IStream* innerStream, const uint8 _key[16])
: EncryptionStream (innerStream)
{
	::memcpy (key, _key, 16);

	if(factoryInstance)
		xorProcesor = factoryInstance->createXORProcessor ();
	
	if(xorProcesor)
		for(int i = 0 ; i < kBufferSize ; i++)
		{
			counterBuffer[i] =(uint8)((i + 0x1234) % 0xFF);
			keyBuffer[i] = key[i % 16];
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BasicEncryptionStream::encrypt (uint8* dst, const uint8* src, int size)
{
	if(xorProcesor && size >= 8)
	{
		bool inplace = src == dst;
		
		int remaining = size;
		while(remaining > 0)
		{
			int counterPosition = (int)(byteCounter % 0xFF);
			int keyPosition = (int)(byteCounter % 16);
			int chunkSize = ccl_min (remaining, kBufferSize - ccl_max (counterPosition, keyPosition));
			
			if(!inplace)
			{
				::memcpy (dst, src, chunkSize);
				src += chunkSize;
			}
			
			Block source (counterBuffer + counterPosition, chunkSize);
			Block destination (dst, chunkSize);
			xorProcesor->process (destination, source);
			
			source.data = keyBuffer + keyPosition;
			xorProcesor->process (destination, source);
			
			dst += chunkSize;
			byteCounter += chunkSize;
			remaining -= chunkSize;
		}
	}
	else
		for(int i = 0; i < size; i++, byteCounter++)
		{
			uint8 ctr = (uint8)((byteCounter + 0x1234) % 0xFF);
			uint8 k = key[byteCounter % 16];
			uint8 v = *src++;
			
			v ^= ctr;
			v ^= k;
			
			*dst++ = v;
		}
}

//************************************************************************************************
// AdvancedEncryptionStream
//************************************************************************************************

AdvancedEncryptionStream::AdvancedEncryptionStream (IStream* _innerStream, int _blockSize)
: EncryptionStream (_innerStream),
  blockSize (_blockSize)
{
	if(factoryInstance)
		xorProcesor = factoryInstance->createXORProcessor ();
	ASSERT (xorProcesor != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AdvancedEncryptionStream::encrypt (uint8* dst, const uint8* src, int size)
{
	bool inplace = src == dst;
	
	int remaining = size;
	while(remaining > 0)
	{
		if(!blockBuffer.isValid (byteCounter))
			generateKeystreamBlocks (byteCounter);

		int positionInBuffer = (int)(byteCounter - blockBuffer.startOffset);
		int chunkSize = ccl_min (remaining, blockBuffer.size - positionInBuffer);
		
		if(!inplace)
		{
			::memcpy (dst, src, chunkSize);
			src += chunkSize;
		}
		
		Block source (blockBuffer.data + positionInBuffer, chunkSize);
		Block destination (dst, chunkSize);
		if(xorProcesor)
			xorProcesor->process (destination, source);
		
		dst += chunkSize;
		byteCounter += chunkSize;
		remaining -= chunkSize;
	}
}

//************************************************************************************************
// XTEAEncryptionStream
//************************************************************************************************

XTEAEncryptionStream::XTEAEncryptionStream (IStream* innerStream, const uint8 _key[16], int64 nonce)
: AdvancedEncryptionStream (innerStream, kBlockSize),
  nonce (nonce)
{
	::memcpy (key, _key, 16);
	generateKeystreamBlocks (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XTEAEncryptionStream::generateKeystreamBlocks (int64 streamPos)
{
	int64 blockIndex = streamPos / blockSize;
	blockBuffer.startOffset = blockIndex * blockSize;

	int blockCount = blockBuffer.size / blockSize;
	uint8* keystreamBlock = blockBuffer.data;
	int64 streamPosition = blockBuffer.startOffset;

	for(int i = 0; i < blockCount; i++)
	{
		int64* block64 = (int64*)keystreamBlock;
		*block64 = nonce ^ streamPosition;
		
		// Hmm, looks like we have a security issue here!
		// From Wikipedia: "...Simply adding or XORing the nonce and counter into a single value
		// would completely break the security under a chosen-plaintext attack..."
		// http://en.wikipedia.org/wiki/Block_cipher_mode_of_operation (CTR mode)

		uint32* block32 = (uint32*)keystreamBlock;

		// this is the actual XTEA encryption for one block (64 bits)
		uint32 v0 = block32[0];
		uint32 v1 = block32[1];
		uint32 sum = 0;
		uint32 delta = 0x9E3779B9;

		for(int r = 0; r < kNumRounds; r++)
		{
			v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
			sum += delta;
			v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
		}

		block32[0] = v0;
		block32[1] = v1;
		
		keystreamBlock += blockSize;
		streamPosition += blockSize;
	}
}

//************************************************************************************************
// AESEncryptionStream
//************************************************************************************************

AESEncryptionStream::AESEncryptionStream (IStream* innerStream, const uint8 key[16], int64 _nonce)
: AdvancedEncryptionStream (innerStream, Security::Crypto::kAES_BlockSize), 
  nonce (_nonce)
{
	if(factoryInstance)
		cryptor = factoryInstance->createCryptor (Security::Crypto::kEncryptMode, 
												  Security::Crypto::kAlgorithmAES, 
												  Security::Crypto::Block (key, 16));
	ASSERT (cryptor != nullptr)
	
	generateKeystreamBlocks (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AESEncryptionStream::generateKeystreamBlocks (int64 streamPos)
{
	int64 blockIndex = streamPos / blockSize;
	blockBuffer.startOffset = blockIndex * blockSize;
	
	int blockCount = blockBuffer.size / blockSize;
	int64* block64 = (int64*)blockBuffer.data;

	for(int i = 0; i < blockCount; i++)
	{
		*block64++ = blockIndex++;
		*block64++ = nonce;
	}

	Block keystream (blockBuffer.data, blockBuffer.size);
	tresult status = cryptor->process (keystream, keystream);
	ASSERT (status == kResultOk)
}

