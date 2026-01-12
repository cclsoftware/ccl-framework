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
// Filename    : ccl/gui/graphics/3d/bufferallocator3d.h
// Description : 3D graphics memory allocator
//
//************************************************************************************************

#ifndef _ccl_bufferallocator3d_h
#define _ccl_bufferallocator3d_h

#include "ccl/gui/graphics/3d/nativegraphics3d.h"

namespace CCL {

class BufferSegment3D;

//************************************************************************************************
// BufferAllocator3D
//************************************************************************************************

class BufferAllocator3D: public Object,
                         public IBufferAllocator3D
{
public:
	DECLARE_CLASS (BufferAllocator3D, Object)

	BufferAllocator3D ();

	// IAllocator3D
	IBufferSegment3D* CCL_API allocateBuffer (GraphicsBuffer3DType type, BufferUsage3D usage,
	                                          uint32 count, uint32 strideInBytes) override;

	CLASS_INTERFACE (IBufferAllocator3D, BufferAllocator3D)

private:
	ObjectArray pools;
};

//************************************************************************************************
// BufferSegment3D
//************************************************************************************************

class BufferSegment3D: public Object,
                       public IBufferSegment3D
{
public:
	DECLARE_CLASS_ABSTRACT (BufferSegment3D, Object)

	BufferSegment3D (IGraphicsBuffer3D* buffer, uint32 offset, uint32 size, uint32 strideInBytes);
	~BufferSegment3D ();

	// IBufferSegment3D
	IGraphicsBuffer3D* CCL_API getBuffer () const override;
	uint32 CCL_API getOffset () const override;
	uint32 CCL_API getSize () const override;
	uint32 CCL_API getStride () const override;

	CLASS_INTERFACE (IBufferSegment3D, BufferSegment3D)

private:
	SharedPtr<IGraphicsBuffer3D> buffer;
	uint32 offset;
	uint32 size;
	uint32 stride;
};

} // namespace CCL

#endif // _ccl_bufferallocator3d_h
