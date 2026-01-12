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
// Filename    : ccl/base/storage/compressionhandler.h
// Description : Compression Handler
//
//************************************************************************************************

#ifndef _ccl_compressionhandler_h
#define _ccl_compressionhandler_h

#include "ccl/public/base/idatatransformer.h"

namespace CCL {

//************************************************************************************************
// CompressionHandler
//************************************************************************************************

class CompressionHandler
{
public:
	CompressionHandler (float compressionLevel = .5f);

	void unzip (IStream& dstStream, const void* buffer, int size);
	void zip (IStream& dstStream, const void* buffer, int size);

protected:
	float compressionLevel;
	AutoPtr<IDataTransformer> decompressor;
	AutoPtr<IDataTransformer> compressor;
	UnknownPtr<ITransformStream> decompressionStream;
	UnknownPtr<ITransformStream> compressionStream;

	void initDecompression ();
	void initCompression ();
};

} // namespace CCL

#endif // _ccl_compressionhandler_h
