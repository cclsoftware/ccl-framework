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
// Filename    : core/portable/corezipstream.h
// Description : Zlib Compression
//
//************************************************************************************************

#ifndef _corezipstream_h
#define _corezipstream_h

#include "core/public/corememstream.h"
#include "core/public/coremacros.h"

namespace Core {
namespace Portable {

// copied from zconf.h:
#ifndef MAX_WBITS
	#define MAX_WBITS 15 // 32K LZ77 window
#endif

//************************************************************************************************
// ZlibTransformStream
/** Base class for data transformation streams using zlib.
	\ingroup core_portable */
//************************************************************************************************

class ZlibTransformStream: public IO::Stream
{
public:
	static const int kBufferSize = 0x8000; // 32KB

	PROPERTY_POINTER (IO::Stream, targetStream, TargetStream)

	bool flush ();

	virtual void reset () = 0;

	// IO::Stream
	int writeBytes (const void* buffer, int size) override;

protected:
	struct Helper;
	static const int kMaxZStreamStructSize = 128;
	typedef char OpaqueZStream[kMaxZStreamStructSize];
	typedef void* OpaqueTransformFunction;

	OpaqueTransformFunction opaqueFunction;
	OpaqueZStream opaqueZStream;
	char dstBuffer[kBufferSize];
	int64 inputPosition;

	ZlibTransformStream (OpaqueTransformFunction opaqueFunction, IO::Stream* targetStream = nullptr);

	bool transform (const void* buffer, int size, bool finish);

	// IO::Stream
	int64 getPosition () override { return inputPosition; }
	int readBytes (void* buffer, int size) override { ASSERT (0) return -1; }
	int64 setPosition (int64 pos, int mode) override { ASSERT (0) return inputPosition; }
};

//************************************************************************************************
// ZlibCompressorStream
/** Stream with zlib compression.
	\ingroup core_portable */
//************************************************************************************************

class ZlibCompressorStream: public ZlibTransformStream
{
public:
	static const float kBestSpeed;
	static const float kBestCompression;

	// prefer fastest compression for embedded platforms (kBestSpeed)
	static float getPreferredLevel () { return kBestSpeed; }

	ZlibCompressorStream (IO::Stream* targetStream = nullptr, float level = getPreferredLevel (),
						  int windowBits = MAX_WBITS);
	~ZlibCompressorStream ();

	// ZlibTransformStream
	void reset () override;
};

//************************************************************************************************
// ZlibDecompressorStream
/** Stream with zlib decompression.
	\ingroup core_portable */
//************************************************************************************************

class ZlibDecompressorStream: public ZlibTransformStream
{
public:
	ZlibDecompressorStream (IO::Stream* targetStream = nullptr, int windowBits = MAX_WBITS);
	~ZlibDecompressorStream ();

	// ZlibTransformStream
	void reset () override;
};

//************************************************************************************************
// ZlibReadStream
/** Stream with "on demand" decompression.
	\ingroup core_portable */
//************************************************************************************************

class ZlibReadStream: public IO::Stream,
					  public IO::BufferProvider

{
public:
	ZlibReadStream (int windowBits = MAX_WBITS);

	PROPERTY_VARIABLE (int, uncompressedSize, UncompressedSize)
	bool copyFromSource (IO::Stream& source, int compressedSize); ///< copy compressed data from source
	void initFromMemory (const void* sourceAddress, int compressedSize); ///< does not copy memory!

	// IO::Stream
	int64 getPosition () override;
	int64 setPosition (int64 pos, int mode) override;
	int readBytes (void* buffer, int size) override;
	int writeBytes (const void* buffer, int size) override;
	BufferProvider* getBufferProvider () override { return this; }

	// BufferProvider
	void moveBufferTo (IO::Buffer& buffer) override;

protected:
	int windowBits;
	IO::Buffer compressedData;
	IO::MemoryStream uncompressedData;
	bool prepared;

	void prepareForRead ();
};

//************************************************************************************************
// CompressionHandler
//************************************************************************************************

class CompressionHandler
{
public:
	CompressionHandler (float compressionLevel = ZlibCompressorStream::getPreferredLevel (),
						int windowBits = MAX_WBITS);
	~CompressionHandler ();

	float getCompressionLevel () const { return compressionLevel; }
	int getWindowBits () const { return windowBits; }

	bool zip (IO::Stream& dstStream, const void* buffer, int size);
	bool unzip (IO::Stream& dstStream, const void* buffer, int size);

public:
	float compressionLevel;
	int windowBits;
	ZlibCompressorStream* compressor;
	ZlibDecompressorStream* decompresssor;

	ZlibCompressorStream& getCompressor ();
	ZlibDecompressorStream& getDecompressor ();
};

//************************************************************************************************
// ThreadAwareCompression
//************************************************************************************************

namespace ThreadAwareCompression
{
	bool zip (IO::Stream& dstStream, const void* buffer, int size, int windowBits = MAX_WBITS);
	bool unzip (IO::Stream& dstStream, const void* buffer, int size, int windowBits = MAX_WBITS);
}

} // namespace Portable
} // namespace Core

#endif // _corezipstream_h
