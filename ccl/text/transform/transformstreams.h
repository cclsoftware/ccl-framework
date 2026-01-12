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
// Filename    : ccl/text/transform/transformstreams.h
// Description : Stream classes reading/writing through a data transformers
//
//************************************************************************************************

#ifndef _ccl_transformstreams_h
#define _ccl_transformstreams_h

#include "ccl/public/base/buffer.h"
#include "ccl/public/base/idatatransformer.h"

namespace CCL {

//************************************************************************************************
// TransformWriter
//************************************************************************************************

class TransformWriter: public Unknown,
					   public ITransformStream
{
public:
	TransformWriter ();
	~TransformWriter ();
	
	tresult open (IDataTransformer* transformer, IStream* targetStream);
	void close ();
	
	// IStream
	int CCL_API write (const void* buffer, int size) override;
	int CCL_API read (void* buffer, int size) override;
	int64 CCL_API tell () override;
	tbool CCL_API isSeekable () const override;
	int64 CCL_API seek (int64 pos, int mode) override;

	// ITransformStream
	void CCL_API setTargetStream (IStream* targetStream) override;
	void CCL_API flush () override;

	CLASS_INTERFACE2 (IStream, ITransformStream, Unknown)

private:
	IDataTransformer* transformer;
	IStream* targetStream;
	int64 seekPos;
	TransformData transformData;
	Buffer destBuffer;
};

//************************************************************************************************
// TransformReader
//************************************************************************************************

class TransformReader: public Unknown,
					   public IStream
{
public:
	TransformReader ();
	~TransformReader ();
	
	tresult open (IDataTransformer* transformer, IStream* sourceStream);
	int preloadSourceData (const void* buffer, int size); // fill in source data, to be transformed in next read call
	void close ();
	
	// IStream
	int CCL_API read (void* buffer, int size) override;
	int CCL_API write (const void* buffer, int size) override;
	int64 CCL_API tell () override;
	tbool CCL_API isSeekable () const override;
	int64 CCL_API seek (int64 pos, int mode) override;

	CLASS_INTERFACE (IStream, Unknown)

private:
	IDataTransformer* transformer;
	IStream* sourceStream;
	int64 seekPos;
	TransformData transformData;
	Buffer sourceBuffer;
	int sourceFilled;
};

} // namespace CCL

#endif // _ccl_transformstreams_h
