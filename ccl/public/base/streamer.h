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
// Filename    : ccl/public/base/streamer.h
// Description : Streamer
//
//************************************************************************************************

#ifndef _ccl_streamer_h
#define _ccl_streamer_h

#include "ccl/public/base/istream.h"

#include "core/public/corestreamaccessor.h"

namespace CCL {

class MutableCString;

//************************************************************************************************
// Streamer
/** Typed stream reader/writer
	\ingroup base_io  */
//************************************************************************************************

class Streamer: public Core::IO::BinaryAccessor
{
public:
	Streamer (IStream& stream, int byteOrder = kNativeByteOrder);

	IStream& getStream () const;
	IStream* operator -> () const;

	// BinaryAccessor
	int read (void* buffer, int size) override;
	int write (const void* buffer, int size) override;

	using BinaryAccessor::read;
	using BinaryAccessor::write;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// C-Strings
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	/** Read null-terminated C-String. */
	bool readCString (MutableCString& string);

	/** Write C-String with preceding length field. */
	bool writeWithLength (CStringRef string);

	/** Read C-String with preceding length field. */
	bool readWithLength (MutableCString& string);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Unicode Strings
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Write a Unicode character. */
	bool writeChar (uchar c);

	/** Read a Unicode character. */
	bool readChar (uchar& c);

	/** Write Unicode string with platform line ending. */
	bool writeLine (StringRef line);
	
	/** Write Unicode string with optional null-termination. */
	bool writeString (StringRef string, bool terminate = true);
	bool writeString (const uchar* chars, bool terminate = true);

	/** Write Unicode string with preceding length field. */
	bool writeWithLength (StringRef string);

	/** Read null-terminated Unicode string. */
	bool readString (String& string);

	/** Read Unicode string with preceding length field. */
	bool readWithLength (String& string);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Other types
	//////////////////////////////////////////////////////////////////////////////////////////////

	bool write (FOURCC fcc);
	bool read (FOURCC& fcc);

	bool write (UIDRef uid);
	bool read (UIDBytes& uid);

protected:
	IStream& stream;

	template <class Str, typename CharType> 
	bool readWithLength (Str& string);

	template <typename CharType>
	bool writeWithLength (const CharType* string, uint32 length);
};

//************************************************************************************************
// StreamPacketizer
/** \ingroup base_io */
//************************************************************************************************

class StreamPacketizer
{
public:
	StreamPacketizer (IStream& stream, int packetSize);

	IStream& getStream () const;
	IStream* operator -> () const;

	int readPackets (void* buffer, int packetCount, int bufferSize);
	int writePackets (const void* buffer, int packetCount);
	int64 seekPacket (int64 packetOffset, int mode);
	int64 getPacketPosition () const;

protected:
	IStream& stream;
	int packetSize;
};

//************************************************************************************************
// StreamSizeWriter
/** \ingroup base_io */
//************************************************************************************************

template<class T>
struct StreamSizeWriter
{
	StreamSizeWriter (CCL::Streamer& stream)
	: stream (stream)
	{
		ASSERT (stream->isSeekable () != 0)
		oldPos = stream->tell ();
		stream.write ((T)0);
	}

	~StreamSizeWriter ()
	{
		int64 newPos = stream->tell ();
		T size = (T)(newPos - oldPos) - sizeof(T);
		ASSERT (size >= 0)
		stream->seek (oldPos, CCL::IStream::kSeekSet);
		stream.write (size);
		stream->seek (newPos, CCL::IStream::kSeekSet);
	}

	CCL::Streamer& stream;
	int64 oldPos;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline IStream& Streamer::getStream () const				{ return stream; }
inline IStream* Streamer::operator -> () const				{ return &stream; }				

inline IStream& StreamPacketizer::getStream () const		{ return stream; }
inline IStream* StreamPacketizer::operator -> () const		{ return &stream; }				

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_streamer_h
