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
// Filename    : ccl/gui/graphics/3d/bufferallocator3d.cpp
// Description : 3D graphics memory allocator
//
//************************************************************************************************

// The current strategy is a bump pointer allocator. It is very simple, however, memory cannot be
// freed. Moving forward, a Two-Level Segregated Fit (TLSF) allocator may result in a more
// sophisticated implementation.

#define DEBUG_LOG 0

#include "ccl/gui/graphics/3d/bufferallocator3d.h"

namespace CCL {

//************************************************************************************************
// BufferPool3D
//************************************************************************************************

class BufferPool3D: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (BufferPool3D, Object)

	BufferPool3D (GraphicsBuffer3DType bufferType, BufferUsage3D bufferUsage);

	IBufferSegment3D* allocate (uint32 count, uint32 strideInBytes);

	GraphicsBuffer3DType getType () const { return bufferType; }
	BufferUsage3D getUsage () const { return bufferUsage; }

private:
	static constexpr uint32 kMinBufferSize = 256 * 1024; // minimum size in bytes for vertex, index and resource buffers

	GraphicsBuffer3DType bufferType;
	BufferUsage3D bufferUsage;
	Vector<AutoPtr<IGraphicsBuffer3D>> buffers;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// BufferPool3D
//************************************************************************************************

DEFINE_CLASS_HIDDEN (BufferPool3D, Object)

///////////////////////////////////////////////////////////////////////////////////////////////////

BufferPool3D::BufferPool3D (GraphicsBuffer3DType bufferType, BufferUsage3D bufferUsage)
: bufferType (bufferType),
  bufferUsage (bufferUsage)
{}

///////////////////////////////////////////////////////////////////////////////////////////////////

IBufferSegment3D* BufferPool3D::allocate (uint32 count, uint32 strideInBytes)
{
	for(const auto& availableBuffer : buffers)
	{
		if(IBufferSegment3D* segment = availableBuffer->createSegment (count, strideInBytes))
			return segment;
	}

	auto& factory = Native3DGraphicsFactory::instance ();
	
	uint32 bufferCapacity = count * strideInBytes;
	
	// don't overallocate constant buffers, as these need to be updated frequently
	if(bufferType != IGraphicsBuffer3D::kConstantBuffer)
		bufferCapacity = ccl_max<uint32> (count, (kMinBufferSize + strideInBytes - 1) / strideInBytes) * strideInBytes;

	IGraphicsBuffer3D* gpuBuffer = factory.createBuffer (bufferType, bufferUsage, bufferCapacity, strideInBytes);
	if(!gpuBuffer)
		return nullptr;

	buffers.add (gpuBuffer);

	return gpuBuffer->createSegment (count, strideInBytes);
}

//************************************************************************************************
// BufferAllocator3D
//************************************************************************************************

DEFINE_CLASS (BufferAllocator3D, Object)
DEFINE_CLASS_UID (BufferAllocator3D, 0x68876528, 0xf6a4, 0x40c0, 0xa0, 0x73, 0xbf, 0x4b, 0x9, 0x1a, 0xa6, 0xfb)

///////////////////////////////////////////////////////////////////////////////////////////////////

BufferAllocator3D::BufferAllocator3D ()
{
	pools.objectCleanup (true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

IBufferSegment3D* CCL_API BufferAllocator3D::allocateBuffer (GraphicsBuffer3DType type, BufferUsage3D usage,
                                                             uint32 count, uint32 strideInBytes)
{
	ASSERT (usage != kBufferUsageImmutable)
	if(usage == kBufferUsageImmutable)
	{
		// We cannot reasonably handle immutable buffers within the allocator.
		// The caller will have to create a buffer manually.
		return nullptr;
	}

	BufferPool3D* pool = pools.findIf<BufferPool3D> ([type, usage] (const BufferPool3D& obj)
								{
									return obj.getType () == type && obj.getUsage () == usage;
								});

	if(!pool)
	{
		pool = NEW BufferPool3D (type, usage);
		pools.add (pool);
	}

	return pool->allocate (count, strideInBytes);
}

//************************************************************************************************
// BufferSegment3D
//************************************************************************************************

DEFINE_CLASS_HIDDEN (BufferSegment3D, Object)

///////////////////////////////////////////////////////////////////////////////////////////////////

BufferSegment3D::BufferSegment3D (IGraphicsBuffer3D* buffer, uint32 offset, uint32 size, uint32 stride)
: buffer (buffer),
  offset (offset),
  size (size),
  stride (stride)
{
	CCL_PRINTF ("%s: %p, offset %d, size %d, stride %d, next free %d\n", __FUNCTION__, buffer, offset, size, stride, offset + size)
}

///////////////////////////////////////////////////////////////////////////////////////////////////

BufferSegment3D::~BufferSegment3D ()
{}

///////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsBuffer3D* CCL_API BufferSegment3D::getBuffer () const
{
	return buffer;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API BufferSegment3D::getOffset () const
{
	return offset;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API BufferSegment3D::getSize () const
{
	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API BufferSegment3D::getStride () const
{
	return stride;
}
