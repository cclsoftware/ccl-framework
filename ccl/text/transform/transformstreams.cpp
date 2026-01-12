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
// Filename    : ccl/text/transform/transformstreams.cpp
// Description : Stream classes reading/writing through data transformers
//
//************************************************************************************************

#include "ccl/text/transform/transformstreams.h"

using namespace CCL;

//************************************************************************************************
// TransformWriter
//************************************************************************************************

TransformWriter::TransformWriter ()
: targetStream (nullptr),
  transformer (nullptr),
  seekPos (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TransformWriter::~TransformWriter ()
{
	close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult TransformWriter::open (IDataTransformer* transformer, IStream* targetStream)
{
	take_shared (this->transformer, transformer);
	take_shared (this->targetStream, targetStream);
	seekPos = 0;

	if(transformer)
	{
		// negotiate buffer sizes
		transformData.sourceSize = IDataTransformer::kDefaultBufferSize;
		transformData.destSize   = IDataTransformer::kDefaultBufferSize;
		transformer->suggestBufferSizes (transformData.sourceSize, transformData.destSize);

		destBuffer.resize (transformData.destSize);
		transformData.destBuffer = destBuffer;
		transformData.flush      = false;

		if(transformer->open (transformData.sourceSize, transformData.destSize) == kResultTrue)
			return kResultTrue;
	}

	safe_release (this->transformer);
	safe_release (this->targetStream);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransformWriter::close ()
{
	if(transformer)
	{
		if(targetStream)
			flush ();
		transformer->close ();
	}

	if(UnknownPtr<ITransformStream> transformStream = targetStream)
		transformStream->flush ();

	safe_release (this->transformer);
	safe_release (this->targetStream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TransformWriter::write (const void* buffer, int size)
{
	// while there's data left in source buffer
	transformData.sourceBuffer = buffer;
	transformData.sourceSize   = size;
	while(transformData.sourceSize > 0)
	{
		// offer the whole source buffer
		int sourceUsed = 0;
		int destUsed   = 0;
		transformer->transform (transformData, sourceUsed, destUsed);

		// advance in source buffer
		seekPos += sourceUsed;
		transformData.sourceSize  -= sourceUsed;
		transformData.sourceBuffer = (char*)transformData.sourceBuffer + sourceUsed;

		// write out the transformed data
		if(destUsed > 0)
			targetStream->write (destBuffer, destUsed);
		else if(sourceUsed == 0)
			return size - transformData.sourceSize; // abort if nothing was consumed or produced
	}
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TransformWriter::setTargetStream (IStream* targetStream)
{
	take_shared (this->targetStream, targetStream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TransformWriter::flush ()
{
	transformData.sourceBuffer = nullptr;
	transformData.sourceSize   = 0;
	transformData.flush = true;
	int sourceUsed;
	int destUsed;
	do
	{
		sourceUsed = 0;
		destUsed   = 0;
		transformer->transform (transformData, sourceUsed, destUsed);

		// write out the transformed data
		if(destUsed > 0)
			targetStream->write (destBuffer, destUsed);

	} while (destUsed != 0);
	transformData.flush = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TransformWriter::read (void* buffer, int size)
{
	CCL_NOT_IMPL ("TransformWriter::read not possible!")
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API TransformWriter::tell ()
{
	return seekPos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TransformWriter::isSeekable () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API TransformWriter::seek (int64 pos, int mode)
{
	CCL_NOT_IMPL ("TransformWriter::seek not possible!")
	return -1;
}

//************************************************************************************************
// TransformReader
//************************************************************************************************

TransformReader::TransformReader ()
: sourceStream (nullptr),
  transformer (nullptr),
  seekPos (0),
  sourceFilled (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TransformReader::~TransformReader ()
{
	close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult TransformReader::open (IDataTransformer* transformer, IStream* sourceStream)
{
	take_shared (this->transformer, transformer);
	take_shared (this->sourceStream, sourceStream);
	seekPos = 0;

	if(transformer)
	{
		// negotiate buffer sizes
		transformData.sourceSize = IDataTransformer::kDefaultBufferSize;
		transformData.destSize   = IDataTransformer::kDefaultBufferSize;
		transformer->suggestBufferSizes (transformData.sourceSize, transformData.destSize);

		sourceBuffer.resize (transformData.sourceSize);
		transformData.sourceBuffer = sourceBuffer;
		transformData.flush      = false;

		if(transformer->open (transformData.sourceSize, transformData.destSize) == kResultTrue)
			return kResultTrue;
	}

	safe_release (this->transformer);
	safe_release (this->sourceStream);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransformReader::close ()
{
	if(transformer)
		transformer->close ();

	safe_release (this->transformer);
	safe_release (this->sourceStream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TransformReader::read (void* buffer, int size)
{
	transformData.destBuffer = buffer;
	transformData.destSize   = size;
	do
	{
		// refill source buffer (may still contain sourceFilled bytes)
		int toRead = sourceBuffer.getSize () - sourceFilled;
		void* readPtr = (char*)sourceBuffer.getAddress () + sourceFilled;

		int read = sourceStream->read (readPtr, toRead);
		if(read <= 0)
			transformData.flush = true;
		else
			sourceFilled += read;

		transformData.sourceSize = sourceFilled;
		int sourceUsed = 0;
		int destUsed   = 0;
		tresult result = transformer->transform (transformData, sourceUsed, destUsed);

		// if there was an error in the transform
		if(result != kResultTrue)
			return -1;

		sourceFilled             -= sourceUsed;
		ASSERT (sourceFilled >= 0)
		transformData.destSize   -= destUsed;
		transformData.destBuffer = (char*)transformData.destBuffer + destUsed;

		// move remaining source to buffer start
		if(sourceFilled > 0)
			memmove (sourceBuffer.getAddress (), (const char*)sourceBuffer.getAddress () + sourceUsed, sourceFilled);

		if(transformData.flush && destUsed == 0)
			break; // no more output

	} while (transformData.destSize > 0); // written out enough

	int numBytesRead = size - transformData.destSize;
	seekPos += numBytesRead;
	return numBytesRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TransformReader::preloadSourceData (const void* buffer, int size)
{
	// fill source buffer, data will be used in next read call
	int free = sourceBuffer.getSize () - sourceFilled;
	int bytesToFill = ccl_min (free, size);
	if(bytesToFill > 0)
	{
		void* readPtr = (char*)sourceBuffer.getAddress () + sourceFilled;
		memcpy (readPtr, buffer, bytesToFill);
		sourceFilled += bytesToFill;
	}
	return bytesToFill;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TransformReader::write (const void* buffer, int size)
{
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API TransformReader::tell ()
{
	return seekPos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TransformReader::isSeekable () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API TransformReader::seek (int64 pos, int mode)
{
	return -1;
}
