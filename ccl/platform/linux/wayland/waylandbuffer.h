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
// Filename    : ccl/platform/linux/wayland/waylandbuffer.h
// Description : Wayland Shared Memory Buffer
//
//************************************************************************************************

#ifndef _ccl_waylandbuffer_h
#define _ccl_waylandbuffer_h

#include "ccl/platform/linux/wayland/waylandclient.h"

#include "ccl/public/gui/graphics/point.h"
#include "ccl/public/text/cstring.h"

namespace CCL {
class Bitmap;

namespace Linux {

//************************************************************************************************
// WaylandBuffer
//************************************************************************************************

class WaylandBuffer
{
public:
	WaylandBuffer ();
	~WaylandBuffer ();

	bool ready ();
	void attach (wl_surface* surface, int x = 0, int y = 0);
	void detach ();
	
	bool fromBitmap (Bitmap& bitmap);
	
	bool resize (PointRef size, int stride);
	void* getData () const;
	int getByteSize () const;
	
protected:	
	struct Listener: public wl_buffer_listener
	{
		Listener (WaylandBuffer& buffer);
		
		static void onRelease (void* data, wl_buffer* wl_buffer);
		
	protected:
		WaylandBuffer& buffer;
	};
	Listener listener;
	
	MutableCString name;
	Point size;
	void* data;
	wl_buffer* buffer;
	int byteSize;
	int allocatedSize;
	bool bufferAttached;
	int fd;
	wl_shm_pool* pool;
};

} // namespace Linux
} // namespace CCL

#endif // _ccl_waylandbuffer_h
