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
// Filename    : ccl/public/base/idatatransformer.h
// Description : Data Transformation Interfaces
//
//************************************************************************************************

#ifndef _ccl_idatatransformer_h
#define _ccl_idatatransformer_h

#include "ccl/public/base/istream.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in data transformation classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** ZLib Compression. */
	DEFINE_CID (ZlibCompression, 0xf677662, 0xcda7, 0x40b0, 0x95, 0xa, 0x88, 0xb1, 0xbf, 0xfc, 0xc6, 0xfb);

	DEFINE_CID (Base16Encoding, 0xb460cac2, 0xc56c, 0x47e7, 0xb4, 0xbd, 0x9c, 0x11, 0x4b, 0xe4, 0xf8, 0x72);
	DEFINE_CID (Base32Encoding, 0x6ed03060, 0xbc9e, 0x4e9d, 0x97, 0x7d, 0xfe, 0xe8, 0x80, 0xb5, 0x5, 0x41);
	DEFINE_CID (Base64Encoding, 0x16a5be85, 0x5d6b, 0x47d0, 0xb9, 0xb6, 0x18, 0x4c, 0xa2, 0xb5, 0xf4, 0x10);
}

//************************************************************************************************
// TransformData
/** Transformation data description.
	\ingroup base_io */
//************************************************************************************************

struct TransformData
{
	const void* sourceBuffer;	///< data to be transformed
	void* destBuffer;			///< transformed data
	int sourceSize;				///< size of source buffer
	int destSize;				///< size of destination buffer
	tbool flush;				///< tells that no more input data will follow (transform will be called repeatedly until it delivers no output)

	TransformData ()
	: sourceBuffer (nullptr),
	  destBuffer (nullptr),
	  sourceSize (0),
	  destSize (0),
	  flush (false)
	{}
};

//************************************************************************************************
// IDataTransformer
/** Byte-oriented data transformation interface.
	\ingroup base_io */
//************************************************************************************************

interface IDataTransformer: IUnknown
{
	/** Transformation mode. */
	enum Mode
	{
		kDecode,	///< transformer used for decoding
		kEncode		///< transformer used for encoding
	};

	/** Transformation constants. */
	enum Constants
	{
		kDefaultBufferSize = 16384,	///< default size for transformation buffers
		kLargerBufferSize = 32768
	};

	/** The caller proposes sizes which can be adjusted by transformer. */
	virtual tresult CCL_API suggestBufferSizes (int& sourceSize, int& destSize) = 0;

	/** Begin transformation with specified input/output buffer sizes. */
	virtual tresult CCL_API open (int sourceSize, int destSize) = 0;

	/** Transformer consumes as much of the sourceBuffer and fills as much of the destBuffer as reasonable. */
	virtual tresult CCL_API transform (const TransformData& data, int& sourceUsed, int& destUsed) = 0;

	/** End transformation. */
	virtual void CCL_API close () = 0;

	/** Reset transformation, more efficient than calling close/open. */
	virtual void CCL_API reset () = 0;

	DECLARE_IID (IDataTransformer)
};

DEFINE_IID (IDataTransformer, 0xc4ae3cd7, 0x4343, 0x4a67, 0xae, 0xeb, 0xdf, 0xc9, 0x75, 0x57, 0xf5, 0xc3)

//************************************************************************************************
// IDataCompressor
/** Additional interface for transformation object supporting compression.
	\ingroup base_io  */
//************************************************************************************************

interface IDataCompressor: IUnknown
{
	/** Level is between 0 (no compression) and 1 (best compression). */
	virtual tresult CCL_API setCompressionLevel (float level) = 0;

	DECLARE_IID (IDataCompressor)

	//////////////////////////////////////////////////////////////////////////////////////////////

	static const float NoCompression () { return 0.f; }
	static const float BestSpeed () { return 0.1f; }
	static const float BestCompression () { return 1.f; }
};

DEFINE_IID (IDataCompressor, 0x68c45f1a, 0xe8c8, 0x4383, 0xa5, 0xb, 0x72, 0x50, 0xff, 0xd, 0x68, 0x2b)

//************************************************************************************************
// IZLibTransformer
/** Additional interface for ZLib transformation objects.
	\ingroup base_io  */
//************************************************************************************************

interface IZLibTransformer: IUnknown
{
	/** Get maximum value of window bits. */
	virtual int CCL_API getMaxWindowBits () const = 0;

	/** Set window bits value. */
	virtual tresult CCL_API setWindowBits (int windowBits) = 0;

	DECLARE_IID (IZLibTransformer)
};

DEFINE_IID (IZLibTransformer, 0xb64e6b9c, 0xb91b, 0x461f, 0xbe, 0x56, 0xff, 0xb, 0x2c, 0x9b, 0x9, 0x57)

//************************************************************************************************
// ITransformStream
/** Interface extension for transformation stream.
	\ingroup base_io */
//************************************************************************************************

interface ITransformStream: IStream
{
	/** Assign new target stream. */
	virtual void CCL_API setTargetStream (IStream* targetStream) = 0;

	/** Flush pending transformation data. */
	virtual void CCL_API flush () = 0;

	DECLARE_IID (ITransformStream)
};

DEFINE_IID (ITransformStream, 0xd3272bcd, 0x7637, 0x41bd, 0x8c, 0x54, 0xb6, 0xbd, 0x34, 0x57, 0xf8, 0xf5)

//************************************************************************************************
// AbstractDataTransformer
/** Base class for implementing data transformers.
	\ingroup base_io */
//************************************************************************************************

class AbstractDataTransformer: public IDataTransformer
{
public:
	tresult CCL_API suggestBufferSizes (int& sourceSize, int& destSize) override
	{
		sourceSize = destSize = kDefaultBufferSize;
		return kResultOk;
	}

	tresult CCL_API open (int sourceSize, int destSize) override
	{
		return kResultOk;
	}

	void CCL_API close () override
	{}

	void CCL_API reset () override
	{}
};

} // namespace CCL

#endif // _ccl_idatatransformer_h
