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
// Filename    : ccl/base/storage/compressionhandler.cpp
// Description : Compression Handler
//
//************************************************************************************************

#include "ccl/base/storage/compressionhandler.h"

#include "ccl/public/textservices.h"

using namespace CCL;

//************************************************************************************************
// CompressionHandler
//************************************************************************************************

CompressionHandler::CompressionHandler (float compressionLevel)
: compressionLevel (compressionLevel)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompressionHandler::initDecompression ()
{
	if(!decompressor)
	{
		decompressor = System::CreateDataTransformer (ClassID::ZlibCompression, IDataTransformer::kDecode);
		decompressionStream = System::CreateTransformStream (nullptr, decompressor, true);
		decompressionStream->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompressionHandler::initCompression ()
{
	if(!compressor)
	{
		compressor = System::CreateDataTransformer (ClassID::ZlibCompression, IDataTransformer::kEncode);
		UnknownPtr<IDataCompressor> (compressor)->setCompressionLevel (compressionLevel);
		compressionStream = System::CreateTransformStream (nullptr, compressor, true);
		compressionStream->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompressionHandler::unzip (IStream& dstStream, const void* buffer, int size)
{
	initDecompression ();
	decompressionStream->setTargetStream (&dstStream);
	decompressionStream->write (buffer, size);
	decompressionStream->flush ();
	decompressor->reset ();
	decompressionStream->setTargetStream (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompressionHandler::zip (IStream& dstStream, const void* buffer, int size)
{
	initCompression ();
	compressionStream->setTargetStream (&dstStream);
	compressionStream->write (buffer, size);
	compressionStream->flush ();
	compressor->reset ();
	compressionStream->setTargetStream (nullptr);
}
