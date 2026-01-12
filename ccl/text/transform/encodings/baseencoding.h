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
// Filename    : ccl/text/transform/encodings/baseencoding.h
// Description : Base 16/32/64 Encoding
//
//************************************************************************************************

#ifndef _ccl_baseencoding_h
#define _ccl_baseencoding_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/base/idatatransformer.h"

namespace CCL {

//************************************************************************************************
// BaseTransformer
//************************************************************************************************

class BaseTransformer: public Unknown, 
					   public AbstractDataTransformer
{
public:
	enum Type
	{
		kBase16,
		kBase32,
		kBase64
	};

	BaseTransformer (Type type);

	CLASS_INTERFACE (IDataTransformer, Unknown)

protected:
	int bitsPerChar;
	int blockSize;
	int charsPerBlock;
	const char* alphabet;
	int alphabetLength;

	static const char* kAlphabet16;
	static const char* kAlphabet32;
	static const char* kAlphabet64;

	template <class Transformer> static Transformer* createInstance (UIDRef cid);
};

//************************************************************************************************
// BaseEncoder
//************************************************************************************************

class BaseEncoder: public BaseTransformer 
{
public:
	BaseEncoder (Type type);
	~BaseEncoder ();

	static BaseEncoder* createInstance (UIDRef cid);

	// BaseTransformer
	tresult CCL_API transform (const TransformData& data, int& sourceUsed, int& destUsed) override;

protected:
	char* inputBuffer;
	int inputCount;
};

//************************************************************************************************
// BaseDecoder
//************************************************************************************************

class BaseDecoder: public BaseTransformer 
{
public:
	BaseDecoder (Type type);
	~BaseDecoder ();

	static BaseDecoder* createInstance (UIDRef cid);

	// BaseTransformer
	tresult CCL_API transform (const TransformData& data, int& sourceUsed, int& destUsed) override;

protected:
	char* charBuffer;
	int charCount;
	char* outputBuffer;
	int outputCount;
	int outputValid;
	
	int getCharValue (char c) const;
};

} // namespace CCL

#endif // _ccl_baseencoding_h
