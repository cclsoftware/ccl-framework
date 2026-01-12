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
// Filename    : ccl/platform/shared/skia/skiastream.cpp
// Description : Skia Stream
//
//************************************************************************************************

#include "ccl/platform/shared/skia/skiastream.h"

using namespace CCL;

//************************************************************************************************
// SkiaStream
//************************************************************************************************

SkiaStream::SkiaStream (IStream* stream)
: cclStream (stream),
  size (0)
{
	ASSERT (stream && stream->isSeekable ())
	stream->seek (0, IStream::kSeekEnd);
	size = stream->tell ();
	stream->seek (0, IStream::kSeekSet);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

size_t SkiaStream::read (void* buffer, size_t size)
{
	return cclStream->read (buffer, static_cast<int> (size));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkiaStream::isAtEnd () const
{
	return cclStream->tell () >= size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkiaStream::hasLength () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

size_t SkiaStream::getLength () const
{
	return size;
}
