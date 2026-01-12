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
// Filename    : ccl/public/base/memorystream.h
// Description : Memory Stream
//
//************************************************************************************************

#ifndef _ccl_memorystream_h
#define _ccl_memorystream_h

#include "ccl/public/base/istream.h"
#include "ccl/public/base/unknown.h"

#include "core/public/corememstream.h"

namespace CCL {

//************************************************************************************************
// MemoryStream
/** \ingroup base_io */
//************************************************************************************************

class MemoryStream: public Unknown,
					public IMemoryStream,
					public Core::IO::MemoryStream
{
public:
	MemoryStream (uint32 memoryGrow = kDefaultGrow)
	: Core::IO::MemoryStream (memoryGrow)
	{}

	MemoryStream (void* buffer, unsigned int size)
	: Core::IO::MemoryStream (buffer, size)
	{}

	MemoryStream (const MemoryStream& ms)
	: Core::IO::MemoryStream (ms)
	{}

	// IMemoryStream
	int CCL_API read (void* buffer, int size) override
	{
		return Core::IO::MemoryStream::readBytes (buffer, size);
	}

	int CCL_API write (const void* buffer, int size) override
	{
		return Core::IO::MemoryStream::writeBytes (buffer, size);
	}

	int64 CCL_API tell () override
	{
		return Core::IO::MemoryStream::getPosition ();
	}

	tbool CCL_API isSeekable () const override
	{
		return true;
	}

	int64 CCL_API seek (int64 pos, int mode) override
	{
		return Core::IO::MemoryStream::setPosition (pos, mode);
	}

	void* CCL_API getMemoryAddress () const override
	{
		return const_cast<MemoryStream*> (this)->memory.getAddress ();
	}

	uint32 CCL_API getBytesWritten () const override
	{
		return bytesWritten;
	}

	tbool CCL_API setBytesWritten (uint32 bytesWritten) override
	{
		return Core::IO::MemoryStream::setBytesWritten (bytesWritten);
	}

	tbool CCL_API allocateMemoryForStream (uint32 size) override
	{
		return Core::IO::MemoryStream::allocateMemory (size);
	}

	CLASS_INTERFACE2 (IStream, IMemoryStream, Unknown)
};

} // namespace CCL

#endif // _ccl_memorystream_h
