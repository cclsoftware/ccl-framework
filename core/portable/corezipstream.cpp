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
// Filename    : core/portable/corezipstream.cpp
// Description : Zlib Compression
//
//************************************************************************************************

#include "core/portable/corezipstream.h"
#include "core/public/coreprimitives.h"
#include "core/system/corethread.h"

#include "zlib.h"

#include <math.h>

namespace Core {
namespace Portable {

//************************************************************************************************
// ZlibTransformStream::Helper
//************************************************************************************************

struct ZlibTransformStream::Helper
{
	static z_stream& toZStream (OpaqueZStream opaqueZStream)
	{
		ASSERT (sizeof(OpaqueZStream) >= sizeof(z_stream))
		return *reinterpret_cast<z_stream*> (opaqueZStream);
	}

	typedef int (*TransformFunction) (z_streamp strm, int flush);

	static OpaqueTransformFunction toOpaqueFunction (TransformFunction function)
	{
		return reinterpret_cast<OpaqueTransformFunction> (function);
	}

	static TransformFunction toTransformFunction (OpaqueTransformFunction function)
	{
		return reinterpret_cast<TransformFunction> (function);
	}
};

//************************************************************************************************
// ThreadAwareCompressionHandler
//************************************************************************************************

class ThreadAwareCompressionHandler
{
public:
	ThreadAwareCompressionHandler ()
	: mainThreadId (Threads::CurrentThread::getID ()),
	  handler1 (ZlibCompressorStream::getPreferredLevel (), MAX_WBITS),
	  handler2 (ZlibCompressorStream::getPreferredLevel (), -MAX_WBITS)
	{}

	CompressionHandler* getHandlerForMainThread (int windowBits)
	{
		if(Threads::CurrentThread::getID () == mainThreadId)
			switch(windowBits)
			{
			case MAX_WBITS : return &handler1;
			case -MAX_WBITS : return &handler2;
			}
		return nullptr;
	}

	bool zip (IO::Stream& dstStream, const void* buffer, int size, int windowBits)
	{
		if(CompressionHandler* handler = getHandlerForMainThread (windowBits))
			return handler->zip (dstStream, buffer, size);
		else
		{
			ZlibCompressorStream cs (&dstStream, handler1.getCompressionLevel (), windowBits);
			return	cs.writeBytes (buffer, size) == size &&
					cs.flush () &&
					dstStream.setPosition (0, IO::kSeekSet) == 0;
		}
	}

	bool unzip (IO::Stream& dstStream, const void* buffer, int size, int windowBits)
	{
		if(CompressionHandler* handler = getHandlerForMainThread (windowBits))
			return handler->unzip (dstStream, buffer, size);
		else
		{
			ZlibDecompressorStream ds (&dstStream, windowBits);
			return	ds.writeBytes (buffer, size) == size &&
					ds.flush () &&
					dstStream.setPosition (0, IO::kSeekSet) == 0;
		}
	}

protected:
	Threads::ThreadID mainThreadId;
	CompressionHandler handler1;
	CompressionHandler handler2;
};

static ThreadAwareCompressionHandler theThreadAwareCompressionHandler;

} // namespace Portable
} // namespace Core

using namespace Core;
using namespace Portable;

//************************************************************************************************
// ThreadAwareCompression
//************************************************************************************************

bool ThreadAwareCompression::zip (IO::Stream& dstStream, const void* buffer, int size, int windowBits)
{
	return theThreadAwareCompressionHandler.zip (dstStream, buffer, size, windowBits);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ThreadAwareCompression::unzip (IO::Stream& dstStream, const void* buffer, int size, int windowBits)
{
	return theThreadAwareCompressionHandler.unzip (dstStream, buffer, size, windowBits);
}

//************************************************************************************************
// ZlibTransformStream
//************************************************************************************************

ZlibTransformStream::ZlibTransformStream (OpaqueTransformFunction opaqueFunction, IO::Stream* targetStream)
: targetStream (targetStream),
  opaqueFunction (opaqueFunction),
  inputPosition (0)
{
	::memset (&opaqueZStream, 0, sizeof(opaqueZStream));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZlibTransformStream::transform (const void* srcBuffer, int size, bool finish)
{
	z_stream& zstream = Helper::toZStream (opaqueZStream);
	zstream.next_in = (Bytef*)srcBuffer;
	zstream.avail_in = size;

	bool done = false;
	while(!done)
	{
		zstream.next_out = (Bytef*)dstBuffer;
		zstream.avail_out = kBufferSize;

		int result = (*Helper::toTransformFunction (opaqueFunction)) (&zstream, finish ? Z_FINISH : Z_NO_FLUSH);
		ASSERT (result >= Z_OK)
		if(result < Z_OK)
			return false;

		int bytesDone = kBufferSize - zstream.avail_out;
		if(bytesDone > 0 && targetStream)
			if(targetStream->writeBytes (dstBuffer, bytesDone) < 0)
				return false;

		done = finish ? result == Z_STREAM_END : zstream.avail_in == 0;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ZlibTransformStream::writeBytes (const void* buffer, int size)
{
	if(!transform (buffer, size, false))
		return -1;

	inputPosition += size;
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZlibTransformStream::flush ()
{
	if(targetStream)
		return transform (nullptr, 0, true);

	return false;
}

//************************************************************************************************
// ZlibCompressorStream
//************************************************************************************************

const float ZlibCompressorStream::kBestSpeed = 0.1f;
const float ZlibCompressorStream::kBestCompression = 1.f;

//////////////////////////////////////////////////////////////////////////////////////////////////

ZlibCompressorStream::ZlibCompressorStream (IO::Stream* targetStream, float level, int windowBits)
: ZlibTransformStream (Helper::toOpaqueFunction (deflate), targetStream)
{
	z_stream& zstream = Helper::toZStream (opaqueZStream);
	int zlibLevel = int(round (level * float(Z_BEST_COMPRESSION)));
	deflateInit2 (&zstream, zlibLevel, Z_DEFLATED, windowBits, 8, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ZlibCompressorStream::~ZlibCompressorStream ()
{
	flush ();
	z_stream& zstream = Helper::toZStream (opaqueZStream);
	deflateEnd (&zstream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZlibCompressorStream::reset ()
{
	z_stream& zstream = Helper::toZStream (opaqueZStream);
	deflateReset (&zstream);
	inputPosition = 0;
}

//************************************************************************************************
// ZlibDecompressorStream
//************************************************************************************************

ZlibDecompressorStream::ZlibDecompressorStream (IO::Stream* targetStream, int windowBits)
: ZlibTransformStream (Helper::toOpaqueFunction (inflate), targetStream)
{
	z_stream& zstream = Helper::toZStream (opaqueZStream);
	inflateInit2 (&zstream, windowBits);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ZlibDecompressorStream::~ZlibDecompressorStream ()
{
	flush ();
	z_stream& zstream = Helper::toZStream (opaqueZStream);
	inflateEnd (&zstream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZlibDecompressorStream::reset ()
{
	z_stream& zstream = Helper::toZStream (opaqueZStream);
	inflateReset (&zstream);
	inputPosition = 0;
}

//************************************************************************************************
// ZlibReadStream
//************************************************************************************************

ZlibReadStream::ZlibReadStream (int windowBits)
: uncompressedSize (0),
  windowBits (windowBits),
  prepared (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZlibReadStream::copyFromSource (IO::Stream& source, int compressedSize)
{
	ASSERT (prepared == false)

	if(!compressedData.resize (compressedSize))
		return false;

	return source.readBytes (compressedData.getAddress (), compressedSize) == compressedSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZlibReadStream::initFromMemory (const void* sourceAddress, int compressedSize)
{
	ASSERT (prepared == false)

	// take members, no copy
	IO::Buffer temp (const_cast<void*> (sourceAddress), compressedSize, false);
	compressedData.take (temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZlibReadStream::prepareForRead ()
{
	if(prepared == false)
	{
		if(uncompressedSize != 0)
			uncompressedData.allocateMemory (uncompressedSize);
		
		// OPTIMIZATION: use thread-aware compression handler to avoid multiple zlib init calls on main thread
		ThreadAwareCompression::unzip (uncompressedData, compressedData.getAddress (), compressedData.getSize (), windowBits);
		compressedData.resize (0);
		prepared = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 ZlibReadStream::getPosition ()
{
	prepareForRead ();
	return uncompressedData.getPosition ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 ZlibReadStream::setPosition (int64 pos, int mode)
{
	prepareForRead ();
	return uncompressedData.setPosition (pos, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ZlibReadStream::readBytes (void* buffer, int size)
{
	prepareForRead ();
	return uncompressedData.readBytes (buffer, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ZlibReadStream::writeBytes (const void* buffer, int size)
{
	ASSERT (0)
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZlibReadStream::moveBufferTo (IO::Buffer& buffer)
{
	prepareForRead ();
	uncompressedData.moveBufferTo (buffer);
}

//************************************************************************************************
// CompressionHandler
//************************************************************************************************

CompressionHandler::CompressionHandler (float compressionLevel, int windowBits)
: compressionLevel (compressionLevel),
  windowBits (windowBits),
  compressor (nullptr),
  decompresssor (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CompressionHandler::~CompressionHandler ()
{
	delete compressor;
	delete decompresssor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ZlibCompressorStream& CompressionHandler::getCompressor ()
{
	if(compressor == nullptr)
		compressor = NEW ZlibCompressorStream (nullptr, compressionLevel, windowBits);
	return *compressor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ZlibDecompressorStream& CompressionHandler::getDecompressor ()
{
	if(decompresssor == nullptr)
		decompresssor = NEW ZlibDecompressorStream (nullptr, windowBits);
	return *decompresssor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CompressionHandler::zip (IO::Stream& compressedData, const void* buffer, int size)
{
	ZlibCompressorStream& compressor = getCompressor ();
	compressor.setTargetStream (&compressedData);

	int numRead = compressor.writeBytes (buffer, size);
	if(compressor.flush () == false)
		numRead = -1;

	compressor.reset ();
	compressor.setTargetStream (nullptr);
	compressedData.setPosition (0, IO::kSeekSet);

	return numRead >= 0; // -1 means stream error
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CompressionHandler::unzip (IO::Stream& plainData, const void* buffer, int size)
{
	ZlibDecompressorStream& decompressor = getDecompressor ();
	decompressor.setTargetStream (&plainData);

	int numWritten = decompressor.writeBytes (buffer, size);
	if(decompressor.flush () == false)
		numWritten = -1;

	decompressor.reset ();
	decompressor.setTargetStream (nullptr);
	plainData.setPosition (0, IO::kSeekSet);

	return numWritten >= 0; // -1 means stream error
}

