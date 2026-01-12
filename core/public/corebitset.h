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
// Filename    : core/public/corebitset.h
// Description : BitSet class
//
//************************************************************************************************

#ifndef _corebitset_h
#define _corebitset_h

#include "core/public/corevector.h"
#include "core/public/coretypes.h"

namespace Core {

//************************************************************************************************
// BitSet
/** Set of bits.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

class BitSet
{
public:
	BitSet (int size = 0);
	BitSet (const BitSet&);
	virtual ~BitSet ();

	BitSet& operator = (const BitSet&);
	
	bool operator == (const BitSet&) const;
	bool operator != (const BitSet&) const;

	/** Resize set to new bit count. */
	void resize (int bitCount);
	
	/** Get current size (in bits). */
	int getSize () const;
	
	/** Set (or clear) bit at given index. */
	void setBit (int which, bool state);	
	
	/** Get bit state at given index. */
	bool getBit (int which) const;	
	
	/** Toggle bit at given index. */
	void toggleBit (int which);	
	
	/** Set (or clear) all bits. */
	void setAllBits (bool state);			
	
	/** Get index of first set (or cleared) bit. */
	int findFirst (bool state) const;
	
	/** Returns number of set (or cleared) bits. */
	int countBits (bool state) const;

private:
	typedef uint32 BitField;
	
	static const int kBitsInField = sizeof(BitField) * 8;
	static const BitField kFullField = 0xfffffffful;

	int bitCount;
	Vector<BitField> bits;

	inline int getFieldCount () const {return bits.count ();}
};

//************************************************************************************************
// IDSet
/** Maintains a set of integer identifiers.
    \ingroup core_collect
    \ingroup base_collect */
//************************************************************************************************

class IDSet: private BitSet
{
public:
	IDSet (int startOffset = 0, int delta = 128);	

	/** Allocate new identifier. */
	int newID ();
	
	/** Release given identifier. */
	void releaseID (int id);	
 	
private:
	int startOffset;
	int delta;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// BitSet implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

inline BitSet::BitSet (int size)
: bitCount (0),
  bits (0, 1)
{
	resize (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline BitSet::BitSet (const BitSet& bs)
: bitCount (bs.bitCount),
  bits (bs.bits)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline BitSet::~BitSet ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void BitSet::resize (int newBitCount)
{
	if(newBitCount != bitCount)
	{
		if(newBitCount < 0)
			return;
		bitCount = newBitCount;
		int oldFieldCount = getFieldCount ();
		int fieldCount = (bitCount + kBitsInField - 1) / kBitsInField;
		if(fieldCount != oldFieldCount)
		{
			bits.setCount (fieldCount);
			for(int i = oldFieldCount; i < fieldCount; i++)
				bits[i] = 0;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline int BitSet::getSize () const
{
	return bitCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline BitSet& BitSet::operator = (const BitSet& bs) 
{
	if(&bs != this)
	{
		resize (bs.getSize ());
		bits = bs.bits;
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool BitSet::operator == (const BitSet& bs) const
{
	if(bs.getSize () == getSize ())
	{
		for(int fieldIndex = 0, fieldCount = getFieldCount (); fieldIndex < fieldCount; fieldIndex++)
		{
			int bitIndex = fieldIndex * kBitsInField;
			bool fullField = bitIndex < bitCount;
			if(fullField)
			{
				if(bits[fieldIndex] != bs.bits[fieldIndex])
					return false;
			}
			else
			{
				for(int i = bitIndex; i < bitCount; i++)
				{
					if(getBit (i) != bs.getBit (i))
						return false;
				}			
			}			
		}
		return true;
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool BitSet::operator != (const BitSet& bs) const
{
	return ! (operator == (bs));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void BitSet::setBit (int which, bool state)
{
	if(which < 0)
		return;
	if(which >= bitCount)
	{
		if(state) 
			resize (which + 1);
		else
			return;
	}

	int fieldIndex = which / kBitsInField;
	int remainder = which % kBitsInField;
	BitField bitInField = (BitField(1) << remainder);

	if(state)
		bits[fieldIndex] |= bitInField;
	else
		bits[fieldIndex] &= ~ bitInField;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool BitSet::getBit (int which) const
{
	if(which < 0 || which >= bitCount)
		return false;
	
	int fieldIndex = which / kBitsInField;
	int remainder = which % kBitsInField;
	BitField bitInField = (BitField(1) << remainder);

	return (bits[fieldIndex] & bitInField) != 0;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void BitSet::toggleBit (int which)
{
	setBit (which, !getBit (which));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void BitSet::setAllBits (bool state)
{
	if(state)
	{
		BitField full = kFullField;
		bits.fill (full);
	}
	else
		bits.zeroFill ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline int BitSet::findFirst (bool state) const
{
	if(bitCount == 0)
		return -1;

	int startIndex = -1;
	BitField opposite = state ? 0 : kFullField;
	for(int fieldIndex = 0, fieldCount = getFieldCount (); fieldIndex < fieldCount; fieldIndex++)
	{
		if(bits[fieldIndex] != opposite)
		{
			startIndex = fieldIndex * kBitsInField;
			break;
		}	
	}

	if(startIndex < 0)
		return -1;

	for(int i = startIndex; i < bitCount; i++)
	{
		if(getBit (i) == state)
			return i;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline int BitSet::countBits (bool state) const
{
	int count = 0;
	for(int i = 0; i < bitCount; i++)
	{
		if(getBit (i) == state)
			count++;
	}
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IDSet implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

inline IDSet::IDSet (int _startOffset, int _delta) 
: BitSet (_delta), 
  startOffset (_startOffset), 
  delta (_delta)
{
	if(delta < 1)
		delta = 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline int IDSet::newID ()
{
	int firstFree = findFirst (false);
	if(firstFree < 0)
	{
		int size = getSize ();
		resize (size + delta);
		firstFree = size;
	}
	setBit (firstFree, true);

	return firstFree + startOffset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void IDSet::releaseID (int id)
{
	setBit (id - startOffset, false);
}

} // namespace Core 

#endif // _corebitset_h
