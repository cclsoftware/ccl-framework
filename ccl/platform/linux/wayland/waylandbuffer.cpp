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
// Filename    : ccl/platform/linux/wayland/waylandbuffer.cpp
// Description : Wayland Shared Memory Buffer
//
//************************************************************************************************
 
#include "ccl/platform/linux/wayland/waylandbuffer.h"

#include "ccl/gui/graphics/imaging/bitmap.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// WaylandBuffer
//************************************************************************************************

WaylandBuffer::WaylandBuffer ()
: listener (*this),
  data (MAP_FAILED),
  buffer (nullptr),
  byteSize (0),
  allocatedSize (0),
  bufferAttached (false),
  fd (-1),
  pool (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WaylandBuffer::~WaylandBuffer ()
{
	if(pool && WaylandClient::instance ().isInitialized ())
		wl_shm_pool_destroy (pool);
	
	if(fd >= 0)
		::close (fd);
	
	if(buffer && WaylandClient::instance ().isInitialized ())
		wl_buffer_destroy (buffer);
	
	if(data != MAP_FAILED)
		::munmap (data, byteSize);
	data = nullptr;
	
	byteSize = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WaylandBuffer::resize (PointRef newSize, int stride)
{
	if(bufferAttached)
		return false;
	
	if(size == newSize)
		return true;
	size = newSize;
	
	wl_shm* shm = WaylandClient::instance ().getSharedMemory ();
	if(shm == nullptr)
		return false;
	
	if(buffer && WaylandClient::instance ().isInitialized ())
		wl_buffer_destroy (buffer);
	buffer = nullptr;
	
	byteSize = stride * size.y;
	
	if(byteSize > allocatedSize)
	{
		int newSize = byteSize * 1.5;
		
		if(pool && WaylandClient::instance ().isInitialized ())
			wl_shm_pool_destroy (pool);
		pool = nullptr;
		
		if(fd >= 0)
			::close (fd);
		fd = -1;
		
		if(data != MAP_FAILED)
		{
			::munmap (data, allocatedSize);
			data = MAP_FAILED;
		}
		
		for(int i = 0; i < 100 && fd < 0; i++)
		{
			name.empty ();
			name.appendFormat ("/ccl_wl_buffer-%d", ::rand ());
			fd = ::shm_open (name.str (), O_RDWR | O_CREAT | O_EXCL, 0600);
			if(fd >= 0)
			{
				::shm_unlink (name.str ());
				break;
			}
		}
		
		int result = 0;
		do
		{
			result = ::ftruncate (fd, newSize);
		} while (result < 0 && errno == EINTR);
		
		if(result < 0)
		{
			::close (fd);
			return false;
		}
		
		data = ::mmap (nullptr, newSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if(data == MAP_FAILED)
		{
			::close (fd);
			return false;
		}
		
		allocatedSize = newSize;
		
		pool = wl_shm_create_pool (shm, fd, allocatedSize);
	}
	
	buffer = wl_shm_pool_create_buffer (pool, 0, size.x, size.y, stride, WL_SHM_FORMAT_ARGB8888);
	
	wl_buffer_add_listener (buffer, &listener, &listener);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WaylandBuffer::fromBitmap (Bitmap& bitmap)
{
	if(bufferAttached)
		return false;
	
	BitmapDataLocker locker (&bitmap, IBitmap::kRGBAlpha, IBitmap::kLockRead);
	if(locker.result != kResultOk)
		return false;
	
	if(resize ({locker.data.width, locker.data.height}, locker.data.rowBytes))
	{
		::memcpy (data, locker.data.scan0, byteSize);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WaylandBuffer::ready ()
{
	return !bufferAttached;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandBuffer::attach (wl_surface* surface, int x, int y)
{
	// When the bound wl_surface version is 5 or higher, passing any non-zero x or y is a protocol violation
	#ifdef WL_SURFACE_OFFSET_SINCE_VERSION
	if(wl_surface_get_version (surface) >= WL_SURFACE_OFFSET_SINCE_VERSION)
	{
		wl_surface_offset (surface, x, y);
		wl_surface_attach (surface, buffer, 0, 0);
	}
	else
	#endif
	{
		wl_surface_attach (surface, buffer, x, y);
	}
	
	wl_surface_damage_buffer (surface, 0, 0, size.x, size.y);
	wl_surface_commit (surface);
	bufferAttached = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandBuffer::detach ()
{
	bufferAttached = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* WaylandBuffer::getData () const
{
	return data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WaylandBuffer::getByteSize () const
{
	return byteSize;
}

//************************************************************************************************
// WaylandBuffer::Listener
//************************************************************************************************

WaylandBuffer::Listener::Listener (WaylandBuffer& buffer)
: buffer (buffer)
{
	wl_buffer_listener::release = onRelease;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandBuffer::Listener::onRelease (void* data, wl_buffer* wl_buffer)
{
	Listener* This = static_cast<Listener*> (data);
	This->buffer.detach ();
}
