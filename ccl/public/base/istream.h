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
// Filename    : ccl/public/base/istream.h
// Description : Stream interface
//
//************************************************************************************************

#ifndef _ccl_istream_h
#define _ccl_istream_h

#include "ccl/public/base/iunknown.h"

#include "core/public/corestream.h"

namespace CCL {

//************************************************************************************************
// IStream
/** Basic stream interface for byte-oriented reading/writing.
	\ingroup base_io  */
//************************************************************************************************

interface IStream: IUnknown
{
	/** Seek mode. */
	enum SeekMode
	{
		kSeekSet = Core::IO::kSeekSet,	///< seek from beginning of stream
		kSeekCur = Core::IO::kSeekCur,	///< seek relative from current position
		kSeekEnd = Core::IO::kSeekEnd	///< seek from end of stream
	};

	/** Open mode. */
	enum OpenMode
	{
		kWriteMode  = Core::IO::kWriteMode,	///< open for writing
		kReadMode   = Core::IO::kReadMode,	///< open for reading
		kShareRead	= 1<<2,					///< allow shared reading
		kShareWrite	= 1<<3,					///< allow shared writing
		kCreate		= 1<<4,					///< create if not existing, truncate to size 0 if existing

        kOptionBits = 0xFF<<8,	///< reserved for stream options

		kOpenMode   = kReadMode|kShareRead, ///< open for shared reading
		kCreateMode = kWriteMode|kReadMode|kCreate ///< open for reading and writing, create if not existing, truncate to size 0 if existing
	};

	/** Read data from stream. */
	virtual int CCL_API read (void* buffer, int size) = 0;

	/** Write data to stream. */
	virtual int CCL_API write (const void* buffer, int size) = 0;

	/** Get current stream position in bytes. */
	virtual int64 CCL_API tell () = 0;

	/** Returns true if stream is seekable. */
	virtual tbool CCL_API isSeekable () const = 0;

	/** Move current stream position. */
	virtual int64 CCL_API seek (int64 pos, int mode) = 0;

	DECLARE_IID (IStream)

	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Move stream position to zero. */
	inline bool rewind () { return seek (0, kSeekSet) == 0; }
};

DEFINE_IID (IStream, 0x7fcab9b0, 0xe595, 0x4a01, 0x9e, 0xf1, 0xa7, 0x3f, 0x22, 0xba, 0x89, 0xbd)

//************************************************************************************************
// IMemoryStream
/** IStream interface extension for memory-based streams.
	\ingroup base_io  */
//************************************************************************************************

interface IMemoryStream: IStream
{
	/** Returns current memory base address, might change when resizing! */
	virtual void* CCL_API getMemoryAddress () const = 0;

	/** Returns number of bytes written to stream. */
	virtual uint32 CCL_API getBytesWritten () const = 0;

	/** Set number of bytes written to stream. */
	virtual tbool CCL_API setBytesWritten (uint32 bytesWritten) = 0;

	/** Allocate memory of given size. */
	virtual tbool CCL_API allocateMemoryForStream (uint32 size) = 0;

	DECLARE_IID (IMemoryStream)

	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Write data to destination stream. */
	bool writeTo (IStream& dstStream) const
	{
		int toWrite = (int)getBytesWritten ();
		return dstStream.write (getMemoryAddress (), toWrite) == toWrite;
	}
};

DEFINE_IID (IMemoryStream, 0x4bfcd923, 0xcd79, 0x47ff, 0x8f, 0xb4, 0xfe, 0x2, 0x45, 0xb, 0x38, 0x18)

//************************************************************************************************
// CoreStream
/** Core stream adapter class.
	\ingroup base_io  */
//************************************************************************************************

class CoreStream: public Core::IO::Stream
{
public:
	CoreStream (IStream& stream)
	: stream (stream)
	{
		stream.retain ();
	}

	~CoreStream ()
	{
		stream.release ();
	}

	int64 getPosition () override
	{
		return stream.tell ();
	}

	int64 setPosition (int64 pos, int mode) override
	{
		if(stream.isSeekable ())
			return stream.seek (pos, mode);
		else
			return stream.tell ();
	}

	int readBytes (void* buffer, int size) override
	{
		return stream.read (buffer, size);
	}

	int writeBytes (const void* buffer, int size) override
	{
		return stream.write (buffer, size);
	}

protected:
	IStream& stream;
};

} // namespace CCL

#endif // _ccl_istream_h
