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
// Filename    : ccl/text/transform/zlibcompression.cpp
// Description : zlib compression: encoder & decoder classes
//
//************************************************************************************************

#include "ccl/text/transform/zlibcompression.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////

static voidpf ccl_zlib_alloc (voidpf opaque, uInt items, uInt size)
{
#if CORE_MALLOC_ENABLED
	return core_malloc (items * size);
#else
	return malloc (items * size);
#endif
}

static void ccl_zlib_free (voidpf opaque, voidpf address)
{
#if CORE_MALLOC_ENABLED
	core_free (address);
#else
	free (address);
#endif
}

//************************************************************************************************
// ZlibTransformer
//************************************************************************************************

ZlibTransformer::ZlibTransformer ()
: isOpen (false),
  windowBits (MAX_WBITS)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ZlibTransformer::getMaxWindowBits () const
{
	return MAX_WBITS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ZlibTransformer::setWindowBits (int _windowBits)
{
	windowBits = _windowBits;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ZlibTransformer::suggestBufferSizes (int& sourceSize, int& destSize)
{
	sourceSize = IDataTransformer::kLargerBufferSize;
	destSize   = IDataTransformer::kDefaultBufferSize;
	return kResultTrue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ZlibTransformer::open (int sourceSize, int destSize)
{
	if(isOpen)
		close ();

	memset (&zstream, 0, sizeof(z_stream));
	#if 1
	zstream.zalloc = ccl_zlib_alloc;
	zstream.zfree  = ccl_zlib_free;
	#endif

	TRY
	{
		if(initStream ())
		{
			isOpen = true;
			return kResultTrue;
		}
	}
	EXCEPT
	{}
	isOpen = false;
	return kResultFalse;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ZlibTransformer::close ()
{
	TRY
	{
	    exitStream ();
	}
	EXCEPT
	{}
	isOpen = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ZlibTransformer::reset ()
{
	TRY
	{
	    resetStream ();
	}
	EXCEPT
	{}
}

//************************************************************************************************
//	ZlibEncoder
//************************************************************************************************

ZlibEncoder::ZlibEncoder ()
: level (1.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ZlibEncoder::setCompressionLevel (float level)
{
	this->level = ccl_bound (level, 0.f, 1.f);

	return kResultTrue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZlibEncoder::initStream ()
{
	int zlibLevel = ccl_to_int (level * float(Z_BEST_COMPRESSION));
	return deflateInit2 (&zstream, zlibLevel, Z_DEFLATED, windowBits, 8, 0) == Z_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZlibEncoder::exitStream ()
{
	deflateEnd (&zstream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZlibEncoder::resetStream ()
{
	deflateReset (&zstream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ZlibEncoder::transform (const TransformData& data, int& sourceUsed, int& destUsed)
{
	zstream.next_in   = (Bytef*)data.sourceBuffer;
	zstream.avail_in  = data.sourceSize;
	zstream.next_out  = (Bytef*)data.destBuffer;
	zstream.avail_out = data.destSize;

	int result = deflate (&zstream, data.flush ? Z_FINISH : Z_NO_FLUSH);
	ASSERT (result >= Z_OK)
	if(result < Z_OK)
		return kResultFailed;

	destUsed   = data.destSize   - zstream.avail_out;
	sourceUsed = data.sourceSize - zstream.avail_in;
	return kResultTrue;
}

//************************************************************************************************
//	ZlibDecoder
//************************************************************************************************

ZlibDecoder::ZlibDecoder ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZlibDecoder::initStream ()
{
	//was: return inflateInit (&zstream) == Z_OK;
	return inflateInit2 (&zstream, windowBits) == Z_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZlibDecoder::exitStream ()
{
	inflateEnd (&zstream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZlibDecoder::resetStream ()
{
	inflateReset (&zstream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ZlibDecoder::transform (const TransformData& data, int& sourceUsed, int& destUsed)
{
	zstream.next_in   = (Bytef*)data.sourceBuffer;
	zstream.avail_in  = data.sourceSize;
	zstream.next_out  = (Bytef*)data.destBuffer;
	zstream.avail_out = data.destSize;

	int result = inflate (&zstream, Z_NO_FLUSH);
	ASSERT (result != Z_STREAM_ERROR);

	destUsed   = data.destSize   - zstream.avail_out;
	sourceUsed = data.sourceSize - zstream.avail_in;

	return (result == Z_OK || result == Z_STREAM_END) ? kResultTrue : kResultFalse;
}

} // namespace CCL
