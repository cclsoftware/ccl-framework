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
// Filename    : ccl/system/packaging/bufferedstream.h
// Description : Buffered Stream
//
//************************************************************************************************

#ifndef _ccl_bufferedstream_h
#define _ccl_bufferedstream_h

#include "ccl/public/base/istream.h"
#include "ccl/public/base/buffer.h"

namespace CCL {

//************************************************************************************************
// BufferedStream
/** Wraps an IStream, reads & writes through an intermediate buffer.
    Using a BufferedStream can improve performance when client code is reading / writing many small
	portions of data. The target stream will be called with larger blocks. */
//************************************************************************************************

class BufferedStream: public Unknown,
					  public IStream
{
public:
	BufferedStream (IStream* stream, unsigned int bufferSize);
	~BufferedStream ();

	void setStream (IStream* newStream);
	void flush (); ///< write all 'hot' buffered data to stream

	void setStreamOptions (int options);

	// IStream
	int CCL_API read (void* buffer, int size) override;
	int CCL_API write (const void* buffer, int size) override;
	int64 CCL_API tell () override;
	tbool CCL_API isSeekable () const override;
	int64 CCL_API seek (int64 pos, int mode) override;

	CLASS_INTERFACE (IStream, Unknown)

protected:
	void seekStream (int64 pos);

	AutoPtr<IStream> stream;
	Buffer buffer;
	int64 bufferStart;		///< buffer start position in stream
	int64 streamPos;		///< current read/write position of stream
	unsigned int filled;	///< number of valid bytes in buffer (always from start)
	unsigned int bufferPos;	///< current position in buffer, relative to bufferStart
	unsigned int hotStart;	///< start of 'hot area' (that must be written to stream), relative to bufferStart
	unsigned int hotEnd;	///< end of 'hot area', relative to bufferStart
};

} // namespace CCL

#endif // _ccl_bufferedstream_h
