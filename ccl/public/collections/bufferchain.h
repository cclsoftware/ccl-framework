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
// Filename    : ccl/public/collections/bufferchain.h
// Description : Bufferchain class
//
//************************************************************************************************

#ifndef _ccl_bufferchain_h
#define _ccl_bufferchain_h

#include "ccl/public/base/platform.h"

namespace CCL {

//************************************************************************************************
// BufferChain
//************************************************************************************************

template <class T>
class BufferChain
{
public:
	class Buffer
	{
	public:
		Buffer (int capacity) 
		: capacity (capacity), next (nullptr), count (0)
		{
			data = NEW T[capacity];
		}
		
		~Buffer ()
		{
			delete[] (data);
			if(next)
				delete next;
		}

		T* data;
		Buffer* next;
		int capacity;    // capacity of this buffer
		int count;       // number of elements in this buffer;
	};

	BufferChain (int minCapacity = 255)
	: minCapacity (minCapacity),
	  first (nullptr)
	{}

	~BufferChain ()
	{
		if(first)
			delete first;
	}

	/// Writes data to the buffer chain
	void append (T* data, int count)
	{
		if(first == nullptr)
		{
			first = NEW Buffer (count < minCapacity ? minCapacity : count);
		}
		// fast forward to the last
		Buffer* p = first;
		while (p->next != nullptr)
			p = p->next;
		
		int i = 0;
		while (count > 0)
		{
			int free = p->capacity - p->count;
			int actual = free <= count ? free : count;
			memcpy (&p->data[p->count], &data[i], actual * sizeof(T));
			p->count += actual;
			count -= actual;
			i += actual;

			if(count > 0)
				p->next = NEW Buffer (count);
			else
				return;

			p = p->next;
		}

	}
	
	/// Reads data
	int read (int offset, T* data, int count)
	{
		if(first == nullptr)
			return 0;

		Buffer* p = first;
		while(offset > p->count)
		{
			offset -= p->count;
			p = p->next;
			if(p == nullptr)
				return 0;
		}

		int result = 0;
		while (count > 0)
		{
			int actual = (p->count - offset) > count ? count : (p->count - offset);
			memcpy (&data[result], &p->data[offset], actual);
			count -= actual;
			result += actual;
			offset = 0;

			p = p->next;
			if(p == nullptr)
				break;
		}
		return result;
	}

	/// Return the number of elements in all buffers
	int count () const
	{
		int result = 0;
		Buffer* p = first;
		while (p)
		{
			result += p->count;
			p = p->next;
		}
		return result;
	}

	/// Reduces the buffer capacity to the fill size 
	void purge ()
	{
		Buffer* p = first;
		while (p)
		{
			if(p->data && p->capacity > p->count + 16)
			{
				T* newData = NEW T[p->count];
				memcpy (newData, p->data, p->count * sizeof(T));
				delete[] p->data;
				p->data = newData;
			}
			p = p->next;
		}		
	}
    
	/// Reduces the buffer capacity to the fill size 
	void flush ()
	{
		if(first)
			delete first;
        first = 0;
    }
    
protected:
	int minCapacity;
	Buffer* first;
};

} // namespace CCL

#endif // _ccl_bufferchain_h
