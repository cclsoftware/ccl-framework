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
// Filename    : core/public/corestream.h
// Description : Stream interface and base class
//
//************************************************************************************************

#ifndef _corestream_h
#define _corestream_h

#include "core/public/coretypes.h"
#include "core/public/coreproperty.h"

namespace Core {
namespace IO {

class BufferProvider;

//////////////////////////////////////////////////////////////////////////////////////////////////
/** Seek Mode */
//////////////////////////////////////////////////////////////////////////////////////////////////

enum SeekMode
{
	kSeekSet = 0,	///< seek to absolute position
	kSeekCur = 1,	///< offset from current position (positive or negative)
	kSeekEnd = 2	///< seek from end of stream (negative offset)
};

//////////////////////////////////////////////////////////////////////////////////////////////////
/** Open Mode */
//////////////////////////////////////////////////////////////////////////////////////////////////

enum OpenMode
{
	kWriteMode = 1<<0,	///< open stream for writing
	kReadMode = 1<<1	///< open stream for reading
};

//************************************************************************************************
// IByteStream
/**	The byte stream interface represents a sequence of bytes which can be read from/written to
	some underlying storage. */
//************************************************************************************************

struct IByteStream
{
	/** Get current read/write position. */
	virtual int64 getPosition () = 0;

	/** Set current read/write position. \see SeekMode */
	virtual int64 setPosition (int64 pos, int mode) = 0;

	/** Read data from stream
		\return number of bytes read or -1 for error */
	virtual int readBytes (void* buffer, int size) = 0;

	/** Write data to stream
		\return number of bytes written or -1 for error */
	virtual int writeBytes (const void* buffer, int size) = 0;

	static const InterfaceID kIID = FOUR_CHAR_ID ('B','S','t','r');
};

//************************************************************************************************
// Stream
/** Base class for streams. 
	This class is abstract. \ingroup core */
//************************************************************************************************

class Stream: public IByteStream
{
public:
	virtual ~Stream () {}

	/** Provide access to underlying buffer (optional). */
	virtual BufferProvider* getBufferProvider () { return nullptr; }
};

} // namespace IO
} // namespace Core

#endif // _corestream_h
