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
// Filename    : ccl/public/collections/ringbuffer.h
// Description : Ringbuffer class
//
//************************************************************************************************

#ifndef _ccl_ringbuffer_h
#define _ccl_ringbuffer_h

#include "ccl/public/base/platform.h"
#include "ccl/public/base/primitives.h"

namespace CCL {

//************************************************************************************************
// RingBuffer
/** \ingroup base_collect */
//************************************************************************************************

template <class T>
class RingBuffer
{
public:
	RingBuffer (int capacity);
	virtual ~RingBuffer ();

	bool resize (int capacity);
	int getCapacity () const;

	bool push (const T& data);
	bool wasPushed (const T& data);
	bool pop (T& data);
	bool peek (T& data);
	bool isEmpty ();
	int count ();
	int getFree ();
	void removeAll ();

protected:
	T* items;
	int capacity;
	int read;
	int write;

	void increment (int& pos)
	{
		int newPos = pos + 1;
		if(newPos >= capacity)
			newPos = 0;
		pos = newPos;
	}
	void decrement (int& pos)
	{
		int newPos = pos - 1;
		if(newPos < 0)
			newPos = capacity - 1;
		pos = newPos;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// RingBuffer implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
RingBuffer<T>::RingBuffer (int _capacity)
: items (nullptr),
  capacity (0),
  read (0),
  write (0)
{
	resize (_capacity);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
RingBuffer<T>::~RingBuffer ()
{
	resize (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool RingBuffer<T>::resize (int _capacity)
{
	if(capacity == _capacity)
		return true;

	if(_capacity <= 0)
	{
		if(items)
			delete [] items; 
		items = nullptr;
		capacity = 0;
		read = 0;
		write = 0; // no more items
		return true;
	}

	int newCapacity = _capacity;

	T* newItems = NEW T[newCapacity];
	if(newItems)
	{
		// copy old items
		if(capacity > newCapacity)
			capacity = newCapacity;
		for(int i = 0; i < capacity; i++)
			newItems[i] = items[i];

		read = ccl_bound<int> (read, 0, newCapacity);
		write = ccl_bound<int> (write, 0, newCapacity);

		if(items)
			delete [] items;

		items = newItems;
		capacity = newCapacity;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
int RingBuffer<T>::getCapacity () const
{
	return capacity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool RingBuffer<T>::push (const T& data)
{
	items[write] = data;
	this->increment (write);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool RingBuffer<T>::wasPushed (const T& data)
{
	int pos = write;
	int numItems = this->count ();
	for(int i = 0; i < numItems; i++)
	{
		decrement (pos);
		if(items[pos] == data)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool RingBuffer<T>::pop (T& data)
{
	if(read == write)
		return false;

	data = items[read];
	this->increment (read);
	return true;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool RingBuffer<T>::peek (T& data)
{
	if(read == write)
		return false;
	
	data = items[read];
	return true;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
int RingBuffer<T>::count ()
{
	int _write = write;
	int _read = read;	
	if(_read <= _write)
		return _write - _read;
	else
		return capacity - _read + _write;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
int RingBuffer<T>::getFree ()
{
	return capacity - this->count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool RingBuffer<T>::isEmpty ()
{
	return write == read;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
void RingBuffer<T>::removeAll ()
{
	read = 0; 
	write = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_ringbuffer_h
