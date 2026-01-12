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
// Filename    : ccl/text/transform/encodings/textencoding.h
// Description : Base classes for text encoders/decoders
//
//************************************************************************************************

#ifndef _ccl_textencoding_obsolete_h
#define _ccl_textencoding_obsolete_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/base/idatatransformer.h"

namespace CCL {

//************************************************************************************************
// TextDecoder
/** Transforms a specific encoding to a uchar32 sequence. */
//************************************************************************************************

class TextDecoder: public Unknown, 
				   public AbstractDataTransformer
{
public:
	TextDecoder ();

	// IDataTransformer
	tresult CCL_API open (int sourceSize, int destSize) override;
	tresult CCL_API transform (const TransformData& data, int& sourceUsed, int& destUsed) override;
	void CCL_API close () override;

	CLASS_INTERFACE (IDataTransformer, Unknown)

protected:
	bool isOpen;

	enum ReturnCodes ///< special return codes for decodeChar
	{
		kNotEnoughSourceData = 0,
		kIllegalSequence = -1
	};

	/** Return number of used source bytes or one of the codes above. */
	virtual int decodeChar (uchar32& c, const unsigned char* sourceBuffer, int sourceSize) = 0;
};

//************************************************************************************************
// TextEncoder
/** Transforms a uchar32 sequence to a specific encoding. */
//************************************************************************************************

class TextEncoder: public Unknown, 
				   public AbstractDataTransformer
{
public:
	TextEncoder ();

	// IDataTransformer
	tresult CCL_API open (int sourceSize, int destSize) override;
	tresult CCL_API transform (const TransformData& data, int& sourceUsed, int& destUsed) override;
	void CCL_API close () override;

	CLASS_INTERFACE (IDataTransformer, Unknown)

protected:
	bool isOpen;

	enum ReturnCodes ///< special return codes for encodeChar
	{
		kDestBufferTooSmall = 0,
		kNoUnicodeChar = -1
	};

	/** Return number of written dest bytes or one of the codes above. */
	virtual int encodeChar (uchar32 c, unsigned char* destBuffer, int destSize) = 0;
};

} // namespace CCL

#endif // _ccl_textencoding_obsolete_h
