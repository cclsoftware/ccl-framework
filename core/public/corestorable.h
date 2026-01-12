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
// Filename    : core/public/corestorable.h
// Description : Storable interface and base classes
//
//************************************************************************************************

#ifndef _corestorable_h
#define _corestorable_h

#include "core/public/corestreamaccessor.h"

namespace Core {
namespace IO {

//************************************************************************************************
// IStreamStorable
/**	A storable can store/restore its state to/from a stream. 
	This class is abstract. \ingroup core */
//************************************************************************************************

struct IStreamStorable
{
	/** Store current state to stream */
	virtual tbool save (IByteStream& stream) const = 0;
	
	/** Restore state from stream */
	virtual tbool load (IByteStream& stream) = 0;

	static const InterfaceID kIID = FOUR_CHAR_ID ('S','t','r', 'S');
};

//************************************************************************************************
// SizeWriter
/**	Helper struct writing the size of written data to a stream. */
//************************************************************************************************

struct SizeWriter
{
	SizeWriter (IByteStream& stream)
	: streamAccessor (stream),
	  sizePosition (stream.getPosition ())
	{
		int64 size = 0;
		streamAccessor.write (size);
	}
	
	~SizeWriter ()
	{
		int64 position = streamAccessor.getStream ().getPosition ();
		int64 size = position - sizePosition - sizeof(int64);
		streamAccessor.getStream ().setPosition (sizePosition, kSeekSet);
		streamAccessor.write (size);
		streamAccessor.getStream ().setPosition (position, kSeekSet);
	}
	
protected:
	BinaryStreamAccessor streamAccessor;
	int64 sizePosition;
};

//************************************************************************************************
// ContainerStorer
/**	Helper struct writing/reading multiple IStreamStorable's to/from a stream. */
//************************************************************************************************

struct ContainerStorer
{
	struct Item
	{
		IStreamStorable& storable;
		int32 fourCharId;
	};

	ContainerStorer (IByteStream& stream, const Item items[], int count);
	~ContainerStorer ();
	
	/** Store all storables to stream */
	bool storeAll ();

	/** Restore a single storable from stream */
	bool restore (int32 id);

	/** Restore all storables from stream */
	bool restoreAll ();

protected:
	IByteStream& stream;
	const Item* items;
	int count;
	int64 initialPosition;
	int64 nextFreePosition;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ContainerStorer implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

inline ContainerStorer::ContainerStorer (IByteStream& stream, const Item items[], int count)
: stream (stream),
  items (items),
  count (count),
  initialPosition (stream.getPosition ()),
  nextFreePosition (initialPosition)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline ContainerStorer::~ContainerStorer ()
{
	stream.setPosition (nextFreePosition, kSeekSet);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool ContainerStorer::storeAll ()
{
	bool succeeded = true;

	BinaryStreamAccessor streamAccessor (stream);

	stream.setPosition (initialPosition, kSeekSet);

	SizeWriter sizeWriter (stream);

	int64 headerPosition = stream.getPosition ();
	nextFreePosition = headerPosition + count * (sizeof(int64) + sizeof(int32));
	for(int i = 0; i < nextFreePosition - headerPosition; ++i)
		streamAccessor.write ('0');

	for(int i = 0; i < count; ++i)
	{
		IStreamStorable& storable = items[i].storable;
		int32 id = items[i].fourCharId;
		
		int64 size = -stream.getPosition ();
		if(storable.save (stream))
			size += stream.getPosition ();
		else
			succeeded = false;

		nextFreePosition = stream.getPosition ();

		stream.setPosition (headerPosition + i * (sizeof(int64) + sizeof(int32)), kSeekSet);

		succeeded &= streamAccessor.writeFCC (id);
		succeeded &= streamAccessor.write (size);
		
		stream.setPosition (nextFreePosition, kSeekSet);
	}

	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool ContainerStorer::restore (int32 id)
{
	BinaryStreamAccessor streamAccessor (stream);

	stream.setPosition (initialPosition, kSeekSet);

	int64 totalSize = 0;
	if(!streamAccessor.read (totalSize) || totalSize == 0)
		return false;

	nextFreePosition = initialPosition + totalSize + sizeof(int64);

	int64 headerPosition = stream.getPosition ();
	int64 dataPosition = headerPosition + count * (sizeof(int64) + sizeof(int32));

	int64 offset = 0;
	int index = -1;
	for(int i = 0; i < count; ++i)
	{
		int32 fourCharId = 0;
		int64 size = 0;
		if(!streamAccessor.readFCC (fourCharId) || !streamAccessor.read (size))
			return false;

		if(fourCharId == id)
			index = i;

		if(index < 0)
			offset += size;
	}

	if(index < 0)
		return false;

	if(dataPosition + offset >= nextFreePosition)
		return false;
	
	stream.setPosition (dataPosition + offset, kSeekSet);
	
	return items[index].storable.load (stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool ContainerStorer::restoreAll ()
{
	bool succeeded = true;
	for(int i = 0; i < count; ++i)
	{
		succeeded &= restore (items[i].fourCharId);
	}
	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace IO
} // namespace Core

#endif // _corestorable_h
