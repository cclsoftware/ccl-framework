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
// Filename    : core/public/corestreamaccessor.h
// Description : Stream Accessor
//
//************************************************************************************************

#ifndef _corestreamaccessor_h
#define _corestreamaccessor_h

#include "core/public/corestream.h"

namespace Core {
namespace IO {

//************************************************************************************************
// BinaryAccessor
/** Base class for accessing typed binary data. */
//************************************************************************************************

class BinaryAccessor
{
public:
	/** Construct accessor with given byte order. */
	BinaryAccessor (int byteOrder = CORE_NATIVE_BYTEORDER);

	/** Check if bytes need to be swapped. */
	bool isByteSwap () const;

	/** Get configured byte order. */
	int getByteOrder () const;

	/** Configure byte order. */
	void setByteOrder (int byteOrder);

	/** Byte order marker. */
	static const uint16 kByteOrderMark;

	/** Write byte order marker. */
	bool writeByteOrder ();

	/** Read byte order marker. */
	bool readByteOrder ();

	/** Read data from underlying storage, to be implemented by derived class. */
	virtual int read (void* buffer, int size) = 0;

	/** Write data to underlying storage, to be implemented by derived class. */
	virtual int write (const void* buffer, int size) = 0;

	/** Read array of typed elements into buffer. */
	template <typename T> int readElements (T buffer[], int count);

	/** Write array of typed elements from buffer. */
	template <typename T> int writeElements (const T buffer[], int count);

	bool write (int8 c);
	bool read (int8& c);
	bool write (uint8 uc);
	bool read (uint8& uc);

	bool write (int16 s);
	bool read (int16& s);
	bool write (uint16 us);
	bool read (uint16& us);

	bool write (int32 l);
	bool read (int32& l);
	bool write (uint32 ul);
	bool read (uint32& ul);

	bool write (int64 ll);
	bool read (int64& ll);
	bool write (uint64 ull);
	bool read (uint64& ull);

	/** Write variable-length integer. */
	bool writeVarLen (uint32 value);

	/** Read variable-length integer. */
	bool readVarLen (uint32& value);

	bool write (float f);
	bool read (float& f);

	bool write (double f);
	bool read (double& f);

	bool write (char c);
	bool read (char& c);

	/** Write C-String with optional null-termination. */
	bool writeCString (CStringPtr string, bool terminate = true);

	/** Read null-terminated C-String into buffer. */
	bool readCStringBuffer (char* charBuffer, int bufferSize);

	/** Write four-character code. */
	bool writeFCC (int32 fcc);

	/** Read four-character code. */
	bool readFCC (int32& fcc);

protected:
	int byteOrder;
};

//************************************************************************************************
// BinaryStreamAccessor
/** Access typed data from stream. */
//************************************************************************************************

class BinaryStreamAccessor: public BinaryAccessor
{
public:
	BinaryStreamAccessor (IByteStream& stream, int byteOrder = CORE_NATIVE_BYTEORDER)
	: BinaryAccessor (byteOrder),
	  stream (stream)
	{}

	IByteStream& getStream ()
	{
		return stream;
	}

	using BinaryAccessor::read;
	using BinaryAccessor::write;

	// BinaryAccessor
	int read (void* buffer, int size) override
	{
		return stream.readBytes (buffer, size);
	}

	int write (const void* buffer, int size) override
	{
		return stream.writeBytes (buffer, size);
	}

protected:
	IByteStream& stream;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// BinaryAccessor inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool BinaryAccessor::isByteSwap () const	 { return byteOrder != CORE_NATIVE_BYTEORDER; }
inline int BinaryAccessor::getByteOrder () const { return byteOrder; }
inline void BinaryAccessor::setByteOrder (int _byteOrder) { byteOrder = _byteOrder; }

template <typename T> int BinaryAccessor::readElements (T buffer[], int count)
{ return read ((void*)buffer, count * sizeof(T)) / sizeof(T); }

template <typename T>  int BinaryAccessor::writeElements (const T buffer[], int count)
{ return write ((const void*)buffer, count * sizeof(T)) / sizeof(T); }

inline bool BinaryAccessor::write (int8 c) { return write (&c, 1) == 1; }
inline bool BinaryAccessor::read (int8& c) { return read  (&c, 1) == 1; }
inline bool BinaryAccessor::write (uint8 uc) { return write ((int8)uc); }
inline bool BinaryAccessor::read (uint8& uc) { return read  ((int8&)uc); }
inline bool BinaryAccessor::write (uint32 ul) { return write ((int32)ul); }
inline bool BinaryAccessor::read (uint32& ul) { return read  ((int32&)ul); }
inline bool BinaryAccessor::write (uint16 us) { return write ((int16)us); }
inline bool BinaryAccessor::read (uint16& us) { return read  ((int16&)us); }
inline bool BinaryAccessor::write (uint64 ull) { return write ((int64)ull); }
inline bool BinaryAccessor::read (uint64& ull) { return read  ((int64&)ull); }

inline bool BinaryAccessor::write (char c) { return write ((int8)c); }
inline bool BinaryAccessor::read  (char& c) { return read ((int8&)c); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace IO
} // namespace Core

#endif // _corestreamaccessor_h
