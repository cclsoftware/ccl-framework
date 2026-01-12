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
// Filename    : ccl/public/gui/graphics/3d/ibufferallocator3d.h
// Description : 3D graphics buffer allocator
//
//************************************************************************************************

#ifndef _ccl_ibufferallocator3d_h
#define _ccl_ibufferallocator3d_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IBufferSegment3D;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** 3D graphics buffer allocator [IBufferAllocator3D] */
	DEFINE_CID (BufferAllocator3D, 0x68876528, 0xf6a4, 0x40c0, 0xa0, 0x73, 0xbf, 0x4b, 0x9, 0x1a, 0xa6, 0xfb)
}

//************************************************************************************************
// BufferUsage3D
//************************************************************************************************

DEFINE_ENUM (BufferUsage3D)
{
	kBufferUsageDefault,
	kBufferUsageImmutable,
	kBufferUsageDynamic,
	kBufferUsageStaging
};

//************************************************************************************************
// IGraphicsBuffer3D
/** Interface for a buffer containing 3D data suitable for rendering
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IGraphicsBuffer3D: IUnknown
{
	DEFINE_ENUM (Type)
	{
		kVertexBuffer,
		kIndexBuffer,
		kConstantBuffer,
		kShaderResource
	};

	virtual Type CCL_API getType () const = 0;

	virtual void* CCL_API map () = 0;
	
	virtual void CCL_API unmap () = 0;

	virtual IBufferSegment3D* CCL_API createSegment (uint32 count, uint32 stride) = 0;

	DECLARE_IID (IGraphicsBuffer3D)
};

DEFINE_IID (IGraphicsBuffer3D, 0x7b6d11c3, 0x1c63, 0x403c, 0x9c, 0xf7, 0x41, 0xc, 0x0, 0xed, 0xd6, 0x1)

typedef IGraphicsBuffer3D::Type GraphicsBuffer3DType;

//************************************************************************************************
// IBufferSegment3D
/** Buffer segment within a GPU buffer
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IBufferSegment3D: IUnknown
{
	virtual IGraphicsBuffer3D* CCL_API getBuffer () const = 0;

	/** Byte offset */
	virtual uint32 CCL_API getOffset () const = 0;

	/** Size in bytes */
	virtual uint32 CCL_API getSize () const = 0;

	/** Size in bytes of a single element stored in this segment */
	virtual uint32 CCL_API getStride () const = 0;

	DECLARE_IID (IBufferSegment3D)

	//////////////////////////////////////////////////////////////////////////////////////////////

	bool isEqual (const IBufferSegment3D& other) const
	{
		return getBuffer () == other.getBuffer () && getSize () == other.getSize () &&
			   getOffset () == other.getOffset () && getStride () == other.getStride ();
	}
};

DEFINE_IID (IBufferSegment3D, 0x2c5883e4, 0x2ca2, 0x4709, 0x94, 0xe3, 0xac, 0x49, 0x74, 0x84, 0xb7, 0x1c)

//************************************************************************************************
// IBufferAllocator3D
/** 3D graphics memory allocator
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IBufferAllocator3D: IUnknown
{
	virtual IBufferSegment3D* CCL_API allocateBuffer (GraphicsBuffer3DType type, BufferUsage3D usage,
	                                                  uint32 count, uint32 strideInBytes) = 0;

	DECLARE_IID (IBufferAllocator3D)
};

DEFINE_IID (IBufferAllocator3D, 0x8d53ef09, 0x5ff5, 0x485e, 0x80, 0xb5, 0xa8, 0xef, 0x4c, 0xdc, 0x4b, 0x31)

//************************************************************************************************
// IGraphicsResource3D
/** Base interface for objects that need to reference graphics resources
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IGraphicsResource3D: IUnknown
{
	virtual tbool CCL_API isGPUAccessible () const = 0;

	virtual tresult CCL_API upload (IBufferAllocator3D& allocator) = 0;

	virtual void CCL_API discard () = 0;

	DECLARE_IID (IGraphicsResource3D)
};

DEFINE_IID (IGraphicsResource3D, 0xc4f4bb50, 0x96e7, 0x43dd, 0xaa, 0x19, 0x76, 0x86, 0x99, 0xdb, 0xfb, 0xca)

//************************************************************************************************
// MappedBuffer3D
/** System memory mapped buffer
	\ingroup gui_graphics3d */
//************************************************************************************************

template <typename Type>
class MappedBuffer3D
{
public:
	explicit MappedBuffer3D (const IBufferSegment3D& segment)
	: buffer (segment.getBuffer ()),
	  items (nullptr),
	  size (segment.getSize ())
	{
		if(buffer != nullptr)
		{
			if(unsigned char* memory = static_cast<unsigned char*> (buffer->map ()))
				items = reinterpret_cast<Type*> (memory + segment.getOffset ());
		}
	}

	~MappedBuffer3D ()
	{
		if(items != nullptr)
			buffer->unmap ();
	}

	bool isValid () const
	{
		return items != nullptr;
	}

	uint32 getSize () const
	{
		return size;
	}

	Type& operator [] (uint32 index)
	{
		ASSERT (index * sizeof(Type) < size)
		return items[index];
	}

	const Type& operator [] (uint32 index) const
	{
		ASSERT (index * sizeof(Type) < size)
		return items[index];
	}

private:
	IGraphicsBuffer3D* buffer;
	Type* items;
	uint32 size;
};

} // namespace CCL

#endif // _ccl_ibufferallocator3d_h
