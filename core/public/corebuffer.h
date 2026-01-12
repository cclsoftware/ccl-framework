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
// Filename    : core/public/corebuffer.h
// Description : Buffer class
//
//************************************************************************************************

#ifndef _corebuffer_h
#define _corebuffer_h

#include "core/public/coretypes.h"

namespace Core {
namespace IO {

class Buffer;

//************************************************************************************************
// BufferProvider
/*	Interface to transfer memory ownership of a buffer.
	\ingroup core */
//************************************************************************************************

class BufferProvider
{
public:
	virtual ~BufferProvider () {}

	/** Transfer memory ownership to given buffer. */
	virtual void moveBufferTo (Buffer& buffer) = 0;
};

//************************************************************************************************
// Buffer
/** Buffer class managing a heap memory block. */
//************************************************************************************************

class Buffer
{
public:
	/** [HEAVY/LIGHT] Construct buffer with existing data. Copies or just wraps the data. */
	Buffer (void* buffer = nullptr, uint32 size = 0, bool copy = true);
	
	/** [HEAVY] Construct buffer with given size. Always allocates a new heap block. */
	Buffer (uint32 size, bool initWithZero = true);
	
	~Buffer ();

	/** Take memory ownership from other buffer. */
	Buffer& take (Buffer& buffer);

	/** Get memory address (writable). */
	void* getAddress ();
	
	/** Get memory address (read-only). */
	const void* getAddress () const;

	/** Get aligned memory address (writable). */
	void* getAddressAligned ();
	
	/** Get aligned memory address (read-only). */
	const void* getAddressAligned () const;

	/** Check if memory address is null, i.e. nothing allocated. */
	bool isNull () const;
	
	/** Get size of allocated memory. */
	uint32 getSize () const;
	
	/** Get memory alignment. */
	uint32 getAlignment () const;

	/** Resize buffer to new size. Returns false when (re-)allocation fails. */
	bool resize (uint32 newSize);
	
	/** Set memory alignment, must be power of two. */
	void setAlignment (uint32 alignment);	
	
	/** Manipulate valid size without reallocation (must be <= current size). */
	bool setValidSize (uint32 newSize);

	/** Fill memory with zeros. */
	void zeroFill ();
	
	/** Fill memory with byte value. */
	void byteFill (uint8 value);
	
	/** Copy data from source to internal memory. */
	uint32 copyFrom (const void* src, uint32 srcSize);
	
	/** Copy data from source to internal memory at given offset. */
	uint32 copyFrom (uint32 dstOffset, const void* src, uint32 srcSize);	
	
	/** Copy internal data to destination buffer. */
	uint32 copyTo (void* dst, uint32 dstSize) const;

	/** Return typed memory address (writable). */
	template <class T> T* as ();
	
	/** Return typed memory address (read-only). */
	template <class T> const T* as () const;

	operator void* ();
	operator char* ();

	operator const void* () const;
	operator const char* () const;

	bool operator == (const Buffer& buffer) const;

protected:
	void* buffer;
	uint32 size;
	uint32 alignment;
	bool ownMemory;
};

//************************************************************************************************
// Array
/** Template class for element-based buffers. */
//************************************************************************************************

template <class T>
class Array: protected Buffer
{
public:
	/** [HEAVY/LIGHT] Construct array with existing data. Copies or just wraps the data. */
	Array (T* buffer = nullptr, uint32 size = 0, bool copy = true)
	: Buffer (buffer, size * sizeof(T), copy)
	{}

	/** [HEAVY] Construct array with given size. Always allocates a new heap block. */
	Array (uint32 size, bool initWithZero = true)
	: Buffer (size * sizeof(T), initWithZero)
	{}

	/** Resize array to given number of elements. */
	bool resize (uint32 newSize) { return Buffer::resize (newSize * sizeof(T)); }
	
	/** Fill array with zeros. */
	void zeroFill () { Buffer::zeroFill (); }

	/** Get array start address (writable). */
	INLINE T* getAddress () { return (T*)buffer; }
	
	/** Get array start address (read-only). */
	INLINE const T* getAddress () const { return (T*)buffer; }
	
	/** Get array size, i.e. max. number of elements. */
	INLINE uint32 getSize () const { return size / sizeof(T); }

	INLINE operator T* () { return (T*)buffer; }
	INLINE operator const T* () const { return (T*)buffer; }
	
	INLINE T& operator [] (int index) { return ((T*)buffer)[index]; }
	INLINE const T& operator [] (int index) const { return ((T*)buffer)[index]; }
};

//************************************************************************************************
// ConstBitAccessor
/** Helper class to get bits in a byte array with safety checks. No buffer management is done. */
//************************************************************************************************

class ConstBitAccessor
{
public:
	ConstBitAccessor (const char bytes[], uint32 byteSize, bool reversed = false)
	: bytes (bytes),
	  bitCount (byteSize << 3),
	  reversed (reversed)
	{}

	/** Get value of bit at given index. */
	INLINE bool getBit (uint32 bitIndex) const
	{
		ASSERT (bytes && bitIndex < bitCount)
		if(!bytes || bitIndex >= bitCount)
			return false;

		char mask = reversed ? (char)(1 << (7 - (bitIndex & 7))) : (char)(1 << (bitIndex & 7));
		uint32 byteIndex = bitIndex >> 3;
		return (bytes[byteIndex] & mask) != 0;
	}

	/** Get total bit count. */
	INLINE uint32 countBits () const
	{
		return bitCount;
	}

protected:
	const char* bytes;
	uint32 bitCount;
	bool reversed;
};

//************************************************************************************************
// BitAccessor
/** Helper class to set/get bits in a byte array with safety checks. No buffer management is done! */
//************************************************************************************************

class BitAccessor: public ConstBitAccessor
{
public:
	BitAccessor (char* bytes, uint32 byteSize, bool reversed = false)
	: ConstBitAccessor (bytes, byteSize, reversed)
	{}

	/** Set value of bit at given index. */
	INLINE void setBit (uint32 bitIndex, bool state = true)
	{
		ASSERT (bytes != nullptr && bitIndex < bitCount)
		if(!bytes || bitIndex >= bitCount)
			return;

		char mask = reversed ? (char)(1 << (7 - (bitIndex & 7))) : (char)(1 << (bitIndex & 7));
		uint32 byteIndex = bitIndex >> 3;
		if(state)
			const_cast<char*>(bytes)[byteIndex] |= mask;
		else
			const_cast<char*>(bytes)[byteIndex] &= ~mask;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Buffer inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void* Buffer::getAddress () { return buffer; }
inline const void* Buffer::getAddress () const { return buffer; }

inline bool Buffer::isNull () const { return buffer == nullptr; }
inline uint32 Buffer::getSize () const { return size; }
inline uint32 Buffer::getAlignment () const { return alignment; }

template <class T> T* Buffer::as () { return (T*)buffer; }
template <class T> const T* Buffer::as () const { return (const T*)buffer; }

inline Buffer::operator void* () { return buffer; }
inline Buffer::operator char* () { return (char*)buffer; }

inline Buffer::operator const void* () const { return buffer; }
inline Buffer::operator const char* () const { return (const char*)buffer; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace IO
} // namespace Core

#endif // _corebuffer_h
