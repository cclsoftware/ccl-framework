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
// Filename    : ccl/platform/cocoa/quartz/cgdataprovider.cpp
// Description : Quartz data provider
//
//************************************************************************************************

#include "ccl/platform/cocoa/quartz/cgdataprovider.h"

#include "ccl/public/base/istream.h"

using namespace CCL;

//************************************************************************************************
// CGStreamDataProvider
//************************************************************************************************

size_t CGStreamDataProvider::getBytes (void* info, void* buffer, size_t count)
{
	CGStreamDataProvider* This = (CGStreamDataProvider*)info;
	size_t result = 0;
	if(This->stream)
		result = This->stream->read (buffer, (int)count);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

off_t CGStreamDataProvider::skipBytes (void* info, off_t count)
{
	CGStreamDataProvider* This = (CGStreamDataProvider*)info;
	off_t result = 0;
	if(This->stream)
		result = This->stream->seek (count, IStream::kSeekCur);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CGStreamDataProvider::rewind (void* info)
{
	CGStreamDataProvider* This = (CGStreamDataProvider*)info;
	if(This->stream)
		This->stream->rewind ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CGStreamDataProvider::releaseInfo (void* info)
{
	CGStreamDataProvider* This = (CGStreamDataProvider*)info;
	delete This;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CGDataProviderRef CGStreamDataProvider::create (IStream* stream)
{
	CGStreamDataProvider* info = NEW CGStreamDataProvider (stream);

	CGDataProviderSequentialCallbacks callbacks =
	{
		0,   // version
		getBytes,
		skipBytes,
		rewind,
		releaseInfo
	};
	
	return ::CGDataProviderCreateSequential (info, &callbacks);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CGStreamDataProvider::CGStreamDataProvider (IStream* stream)
: stream (stream)
{
	if(stream)
		stream->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CGStreamDataProvider::~CGStreamDataProvider ()
{
	if(stream)
		stream->release ();
}

