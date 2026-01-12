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
// Filename    : ccl/text/transform/zlibcompression.h
// Description : zlib compression: encoder & decoder classes
//
//************************************************************************************************

#ifndef _ccl_zlibcompression_h
#define _ccl_zlibcompression_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/base/idatatransformer.h"

#include "zlib.h"

namespace CCL {

//************************************************************************************************
// ZlibTransformer
/** Common base class for encoder and decoder */
//************************************************************************************************

class ZlibTransformer: public Unknown,
					   public IZLibTransformer,
					   public IDataTransformer
{
public:
	// IZLibTransformer
	int CCL_API getMaxWindowBits () const override;
	tresult CCL_API setWindowBits (int windowBits) override;

	// IDataTransformer
	tresult CCL_API suggestBufferSizes (int& sourceSize, int& destSize) override;
	tresult CCL_API open (int sourceSize, int destSize) override;
	void CCL_API close () override;
	void CCL_API reset () override;

	CLASS_INTERFACE2 (IZLibTransformer, IDataTransformer, Unknown)

protected:
	ZlibTransformer ();

	virtual bool initStream () = 0;
	virtual void exitStream () = 0;
	virtual void resetStream () = 0;

	z_stream zstream;
	bool isOpen;
	int windowBits;
};

//************************************************************************************************
// ZlibEncoder
//************************************************************************************************

class ZlibEncoder: public ZlibTransformer, 
				   public IDataCompressor
{
public:
	ZlibEncoder ();
	
	// IDataTransformer
	tresult CCL_API transform (const TransformData& data, int& sourceUsed, int& destUsed) override;

	// IDataCompressor
	virtual tresult CCL_API setCompressionLevel (float level) override;

	CLASS_INTERFACE (IDataCompressor, ZlibTransformer)

private:
	bool initStream () override;
	void exitStream () override;
	void resetStream () override;
	float level;
};

//************************************************************************************************
// ZlibDecoder
//************************************************************************************************

class ZlibDecoder: public ZlibTransformer
{
public:
	ZlibDecoder ();
	
	// IDataTransformer
	tresult CCL_API transform (const TransformData& data, int& sourceUsed, int& destUsed) override;

private:
	bool initStream () override;
	void exitStream () override;
	void resetStream () override;
};

} // namespace CCL

#endif // _ccl_zlibcompression_h
