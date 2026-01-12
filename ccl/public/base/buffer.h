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
// Filename    : ccl/public/base/buffer.h
// Description : Buffer class
//
//************************************************************************************************

#ifndef _ccl_buffer_h
#define _ccl_buffer_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/base/ibuffer.h"

#include "core/public/corebuffer.h"

namespace CCL {

using Core::IO::Array;
using Core::IO::BitAccessor;
using Core::IO::ConstBitAccessor;

//************************************************************************************************
// Buffer
/** \ingroup base_io */
//************************************************************************************************

class Buffer: public Unknown,
			  public IBuffer,
			  public Core::IO::Buffer
{
public:
	Buffer (void* buffer = nullptr, uint32 size = 0, bool copy = true)
	: Core::IO::Buffer (buffer, size, copy)
	{}

	Buffer (uint32 size, bool initWithZero = true)
	: Core::IO::Buffer (size, initWithZero)
	{}

	// IBuffer
	void* CCL_API getBufferAddress () const override
	{
		return const_cast<Buffer*> (this)->getAddressAligned ();
	}

	uint32 CCL_API getBufferSize () const override
	{
		return getSize ();
	}

	CLASS_INTERFACE (IBuffer, Unknown)
};

} // namespace CCL

#endif // _ccl_buffer_h
