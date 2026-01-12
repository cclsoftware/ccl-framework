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
// Filename    : ccl/platform/win/system/winrtbuffer.h
// Description : WinRT Buffer Access
//
//************************************************************************************************

#ifndef _winrtbuffer_h
#define _winrtbuffer_h

#include "ccl/platform/win/system/cclwinrt.h"

#include <windows.storage.streams.h> // requires Windows 10 SDK to compile!

/*
	Obtaining pointers to data buffers (C++/CX)
	https://msdn.microsoft.com/en-us/windows/apps/dn182761.aspx
*/
#pragma warning (disable : 4467)  // disable 'usage of ATL attributes is deprecated'
#include <robuffer.h>

namespace CCL {
namespace WinRT {

using namespace ABI::Windows::Storage::Streams;
using namespace Windows::Storage::Streams;

//************************************************************************************************
// PlatformBuffer
//************************************************************************************************

struct PlatformBuffer
{
	ComPtr<IBuffer> buffer;

	explicit PlatformBuffer (IBuffer* _buffer = nullptr)
	{
		buffer.share (_buffer);
	}

	UINT32 getCapacity () const
	{
		UINT32 capacity = 0;
		if(buffer)
			buffer->get_Capacity (&capacity);
		return capacity;
	}

	void* getPointer ()
	{
		byte* ptr = 0;
		ComPtr<IBufferByteAccess> bufferAccessor; // from robuffer.h
		if(buffer)
			buffer->QueryInterface (__uuidof(IBufferByteAccess), bufferAccessor);
		if(bufferAccessor)
			bufferAccessor->Buffer (&ptr);
		return ptr;
	}

	UINT32 getLength () const
	{
		UINT32 length = 0;
		if(buffer)
			buffer->get_Length (&length);
		return length;
	}

	bool setLength (UINT32 length)
	{
		HRESULT hr = E_FAIL;
		if(buffer)
			hr = buffer->put_Length (length);
		return SUCCEEDED (hr);
	}
};

//************************************************************************************************
// BufferFactory
//************************************************************************************************

class BufferFactory
{
public:
	BufferFactory ()
	{
		factory = winrt_new<IBufferFactory> (RuntimeClass_Windows_Storage_Streams_Buffer);
		ASSERT (factory.isValid ())
	}

	IBuffer* create (UINT32 capacity)
	{
		IBuffer* buffer = 0;
		if(factory)
			factory->Create (capacity, &buffer);
		return buffer;
	}

protected:
	ComPtr<IBufferFactory> factory;
};

} // namespace WinRT
} // namespace CCL

#endif // _winrtbuffer_h
