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
// Filename    : ccl/public/base/multiplexstream.h
// Description : Multiplex Stream
//
//************************************************************************************************

#ifndef _ccl_multiplexstream_h
#define _ccl_multiplexstream_h

#include "ccl/public/base/istream.h"
#include "ccl/public/base/unknown.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

//************************************************************************************************
// MultiplexStream
/** A stream made up of multiple stream parts (read-only).
	\ingroup base_io  */
//************************************************************************************************

class MultiplexStream: public Unknown,
					   public IStream
{
public:
	MultiplexStream ();

	int64 getTotalSize () const;
	void addStream (IStream* stream, int64 size);

	// IStream
	int CCL_API read (void* buffer, int size) override;
	int CCL_API write (const void* buffer, int size) override;
	int64 CCL_API tell () override;
	tbool CCL_API isSeekable () const override;
	int64 CCL_API seek (int64 pos, int mode) override;

	CLASS_INTERFACE (IStream, Unknown)

protected:
	struct StreamPart
	{
		int64 start;
		int64 size;
		SharedPtr<IStream> stream;

		StreamPart (int64 start = 0, int64 size = 0, IStream* stream = nullptr)
		: start (start),
		  size (size),
		  stream (stream)
		{}
	};

	int64 totalSize;
	int64 readPosition;
	Vector<StreamPart> streams;

	bool findStream (StreamPart& result, int64 position);
};

} // namespace CCL

#endif // _ccl_multiplexstream_h
