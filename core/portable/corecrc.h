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
// Filename    : core/portable/corecrc.h
// Description : Cyclic Redundancy Check (CRC) algorithms with compile-time lookup tables
//
//************************************************************************************************

#ifndef _corecrc_h
#define _corecrc_h

#include "core/public/coretypes.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// Crc
/** Generic class for computing cyclic redundancy checks.

	Usage:

	Crc32 crc;
	while(receiveData (buffer, size))
		crc.update (buffer, size);
	compare (crc.get (), expectedCrc);

	See the explicit template initializations below for common CRC algorithms. */
//************************************************************************************************

template <typename CrcType, CrcType polynomial, CrcType initialValue, bool reflectInput, bool reflectOutput, CrcType finalXorValue>
class Crc
{
public:
	/** Calculate the CRC value from a stream of bytes by calling update() for every new data packet. */
	void update (const void* data, int numBytes);

	/** Get the final CRC value. */
	CrcType get () const;

private:
	CrcType crc = initialValue;

	template <typename T>
	static T reflect (T value);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Common 16-bit CRC algorithm */
using Crc16 = Crc<uint16, 0x8005, 0x0000, true, true, 0x0000>;

/** Common 32-bit CRC algorithm */
using Crc32 = Crc<uint32, 0x04C11DB7, 0xFFFFFFFF, true, true, 0xFFFFFFFF>;

/** Variant of the 32-bit CRC algorithm used predominantly for audio and video data. */
using Crc32Mpeg2 = Crc<uint32, 0x04C11DB7, 0xFFFFFFFF, false, false, 0x00000000>;

//************************************************************************************************
// CrcLookupTable
/** Internal class for precomputing CRC lookup-tables in compile-time. */
//************************************************************************************************

template <typename CrcType, CrcType polynomial>
struct CrcLookupTable
{
	struct Table
	{
		static constexpr int kSize = 256;
		CrcType data[kSize] = {};
	};

	static constexpr Table generateTable ();
	static constexpr Table table = generateTable ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Crc implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CrcType, CrcType polynomial, CrcType initialValue, bool reflectInput, bool reflectOutput, CrcType finalXorValue>
template <typename T>
T Crc<CrcType, polynomial, initialValue, reflectInput, reflectOutput, finalXorValue>::reflect (T value)
{
	T result = 0;
	for(int i = 0; i < sizeof (T) * 8; i++)
		if(value & (T (1) << i))
			result |= (T (1) << ((sizeof (T) * 8 - 1) - i));
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CrcType, CrcType polynomial, CrcType initialValue, bool reflectInput, bool reflectOutput, CrcType finalXorValue>
void Crc<CrcType, polynomial, initialValue, reflectInput, reflectOutput, finalXorValue>::update (const void* data, int numBytes)
{
	for(int i = 0; i < numBytes; i++)
	{
		unsigned char input = static_cast<const unsigned char*> (data)[i];
		if constexpr(reflectInput)
			input = reflect<unsigned char> (input);
		unsigned char byte = input ^ (crc >> (sizeof (CrcType) * 8 - 8));
		crc = CrcLookupTable<CrcType, polynomial>::table.data[byte] ^ (crc << 8);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CrcType, CrcType polynomial, CrcType initialValue, bool reflectInput, bool reflectOutput, CrcType finalXorValue>
CrcType Crc<CrcType, polynomial, initialValue, reflectInput, reflectOutput, finalXorValue>::get () const
{
	CrcType output = crc;
	if constexpr(reflectOutput)
		output = reflect<CrcType> (output);
	return output ^ finalXorValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// CrcLookupTable implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CrcType, CrcType polynomial>
constexpr typename CrcLookupTable<CrcType, polynomial>::Table CrcLookupTable<CrcType, polynomial>::generateTable ()
{
	Table table {};
	for(int i = 0; i < Table::kSize; i++)
	{
		CrcType remainder = CrcType(i) << (sizeof (CrcType) * 8 - 8);
		for(unsigned char bit = 8; bit > 0; --bit)
			if(remainder & (CrcType (1) << (sizeof (CrcType) * 8 - 1)))
				remainder = (remainder << 1) ^ polynomial;
			else
				remainder = (remainder << 1);
		table.data[i] = remainder;
	}
	return table;
}

} // namespace Portable
} // namespace Core

#endif // _corecrc_h
