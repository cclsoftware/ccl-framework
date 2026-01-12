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
// Filename    : core/public/corememstream.h
// Description : Memory Stream
//
//************************************************************************************************

#ifndef _corememstream_h
#define _corememstream_h

#include "core/public/corebuffer.h"
#include "core/public/corestream.h"

namespace Core {
namespace IO {

//************************************************************************************************
// MemoryStream
/**	Stream backed by a block of heap memory that can grow dynamically.
	\ingroup core */
//************************************************************************************************

class MemoryStream: public Stream,
					public BufferProvider
{
public:
	static const uint32 kDefaultGrow = 8192;

	/** [HEAVY] Stream memory grows with given amount. */
	MemoryStream (uint32 memoryGrow = kDefaultGrow);

	/** [LIGHT] Wraps buffer into stream, DOES NOT copy memory. */
	MemoryStream (void* buffer, uint32 size);
	
	/** Copy constructor. */
	MemoryStream (const MemoryStream& ms);

	/** Take over memory from other memory stream. */
	MemoryStream& take (MemoryStream& ms);

	/** Take over memory from other buffer. */
	MemoryStream& take (Buffer& buffer);

	/** Copy data from other memory stream. */
	bool copyFrom (const MemoryStream& ms);

	/** Allocate (and optionally initialize) internal stream memory. */
	bool allocateMemory (uint32 size, bool initWithZero = false);

	/** Access underlying buffer (read-only). */
	const Buffer& getBuffer () const;

	/** Get current memory grow amount. */
	uint32 getMemoryGrow () const;

	/** Change memory grow amount. */
	void setMemoryGrow (uint32 memoryGrow);

	/** Get number of bytes written to stream. */
	uint32 getBytesWritten () const;

	/** Manually set number of bytes written to stream. */
	bool setBytesWritten (uint32 bytesWritten);

	// Stream
	int64 getPosition () override;
	int64 setPosition (int64 pos, int mode) override;
	int readBytes (void* buffer, int size) override;
	int writeBytes (const void* buffer, int size) override;
	BufferProvider* getBufferProvider () override { return this; }

	// BufferProvider
	void moveBufferTo (Buffer& buffer) override;

protected:
	Buffer memory;
	uint32 memoryGrow;
	int position;
	int bytesWritten;
};

} // namespace IO
} // namespace Core

#endif // _corememstream_h

