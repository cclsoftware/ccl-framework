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
// Filename    : ccl/gui/graphics/3d/nativegraphics3d.cpp
// Description : Native 3D Graphics classes
//
//************************************************************************************************

#include "ccl/gui/graphics/3d/nativegraphics3d.h"
#include "ccl/gui/graphics/3d/bufferallocator3d.h"
#include "ccl/gui/graphics/3d/shader/shaderreflection3d.h"
#include "ccl/gui/graphics/3d/model/model3d.h"

#include "ccl/gui/graphics/graphicshelper.h"

namespace CCL {

//************************************************************************************************
// Native3DResourceManager::TextureItem
//************************************************************************************************

class Native3DResourceManager::TextureItem: public Object
{
public:
	DECLARE_CLASS (TextureItem, Object)
	
	TextureItem (Native3DTexture2D* texture = nullptr, Bitmap* bitmap = nullptr, TextureFlags3D flags = 0);
	
	PROPERTY_AUTO_POINTER (Native3DTexture2D, texture, Texture)
	PROPERTY_VARIABLE (TextureFlags3D, flags, Flags)
	PROPERTY_SHARED_POINTER (Bitmap, bitmap, Bitmap)
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Native3DVertexFormat
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Native3DVertexFormat, Object)

//************************************************************************************************
// Native3DGraphicsBuffer
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Native3DGraphicsBuffer, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DGraphicsBuffer::Native3DGraphicsBuffer (uint32 capacity)
: type (kVertexBuffer),
  capacity (capacity),
  offset (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBufferSegment3D* CCL_API Native3DGraphicsBuffer::createSegment (uint32 count, uint32 stride)
{
	uint32 size = count * stride;
	if(size > 0 && !ensureSegmentAlignment (offset, size, stride))
		return nullptr;
	if(offset + size > capacity)
		return nullptr;
	BufferSegment3D* segment = NEW BufferSegment3D (this, offset, size, stride);
	offset += size;
	return segment;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Native3DGraphicsBuffer::ensureSegmentAlignment (uint32& byteOffset, uint32& size, uint32 stride) const
{
	return true;
}

//************************************************************************************************
// Native3DTexture2D
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Native3DTexture2D, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Native3DTexture2D::create (IBitmap* bitmap, TextureFlags3D flags)
{
	IMultiResolutionBitmap::RepSelector selector (UnknownPtr<IMultiResolutionBitmap> (bitmap), getHighestResolutionIndex (bitmap));
	BitmapDataLocker locker (bitmap, IBitmap::kRGBAlpha, IBitmap::kLockRead);
	if(locker.result != kResultOk)
		return false;

	return create (locker.data.width, locker.data.height, locker.data.rowBytes, kB8G8R8A8_UNORM, flags, locker.data.scan0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 Native3DTexture2D::getMipLevels (uint32 width, uint32 height)
{
	uint32 size = ccl_max (width, height);
	uint32 mipLevels = 1;
	while(size >>= 1)
		mipLevels++;
	return mipLevels;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Native3DTexture2D::getHighestResolutionIndex (IBitmap* bitmap)
{
	UnknownPtr<IMultiResolutionBitmap> multiResolutionBitmap (bitmap);
	int index = 0;
	float maxResolution = 0.f;
	if(multiResolutionBitmap)
	{
		for(int i = 0; i < multiResolutionBitmap->getRepresentationCount (); i++)
		{
			IMultiResolutionBitmap::RepSelector selector (multiResolutionBitmap, i);
			if(bitmap->getContentScaleFactor () > maxResolution)
			{
				maxResolution = bitmap->getContentScaleFactor ();
				index = i;
			}
		}
	}
	return index;
}

//************************************************************************************************
// Native3DGraphicsShader
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Native3DGraphicsShader, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DGraphicsShader::Native3DGraphicsShader (Type type)
: type (type)
{
	bufferTypeInfos.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void const* Native3DGraphicsShader::getBlobAddress () const
{
	if(blob == nullptr)
		return nullptr;
	else
		return blob->getAddress ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 Native3DGraphicsShader::getBlobSize () const
{
	if(blob == nullptr)
		return 0;
	else
		return blob->getSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITypeInfo* CCL_API Native3DGraphicsShader::getBufferTypeInfo (int bufferIndex)
{
	for(auto* bufferTypeInfo : iterate_as<ShaderTypeInfo3D> (bufferTypeInfos))
	{
		if(bufferTypeInfo && bufferTypeInfo->getBindingIndex () == bufferIndex)
			return bufferTypeInfo;
	}
	return nullptr;
}

//************************************************************************************************
// Native3DShaderParameterSet
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Native3DShaderParameterSet, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DShaderParameterSet::Native3DShaderParameterSet ()
{
	textures.setCount (kMaxTextureCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Native3DShaderParameterSet::setVertexShaderParameters (int bufferIndex, IBufferSegment3D* parameters)
{
	if(Native3DShaderParameters* shaderParameters = findVertexShaderParameters (bufferIndex))
	{
		shaderParameters->segment = parameters;
		return kResultOk;
	}
	vertexShaderParameters.addSorted ({ bufferIndex, parameters });
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Native3DShaderParameterSet::setPixelShaderParameters (int bufferIndex, IBufferSegment3D* parameters)
{
	if(Native3DShaderParameters* shaderParameters = findPixelShaderParameters (bufferIndex))
	{
		shaderParameters->segment = parameters;
		return kResultOk;
	}
	pixelShaderParameters.addSorted ({ bufferIndex, parameters });
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Native3DShaderParameterSet::setTexture (int textureIndex, IGraphicsTexture2D* texture)
{
	if(textureIndex < 0 || textureIndex >= kMaxTextureCount)
		return kResultInvalidArgument;
	textures[textureIndex] = texture;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DShaderParameters* Native3DShaderParameterSet::findVertexShaderParameters (int bufferIndex) const
{
	return vertexShaderParameters.findIf ([&] (const Native3DShaderParameters& parameters) { return parameters.bufferIndex == bufferIndex; });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DShaderParameters* Native3DShaderParameterSet::findPixelShaderParameters (int bufferIndex) const
{
	return pixelShaderParameters.findIf ([&] (const Native3DShaderParameters& parameters) { return parameters.bufferIndex == bufferIndex; });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<Native3DShaderParameters>& Native3DShaderParameterSet::getVertexShaderParameters () const
{
	return vertexShaderParameters;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<Native3DShaderParameters>& Native3DShaderParameterSet::getPixelShaderParameters () const
{
	return pixelShaderParameters;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsTexture2D* Native3DShaderParameterSet::getTexture (int textureIndex) const
{
	return textures.at (textureIndex);
}

//************************************************************************************************
// Native3DShaderParameters
//************************************************************************************************

Native3DShaderParameters::Native3DShaderParameters (int bufferIndex, IBufferSegment3D* segment)
: bufferIndex (bufferIndex),
  segment (segment)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Native3DShaderParameters::operator > (const Native3DShaderParameters& other) const
{
	return other.bufferIndex > bufferIndex;
}

//************************************************************************************************
// Native3DGraphicsPipeline
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Native3DGraphicsPipeline, Object)

//************************************************************************************************
// Native3DResourceManager
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Native3DResourceManager, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DResourceManager::Native3DResourceManager ()
{
	shaderList.objectCleanup (true);
	textureList.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DGraphicsShader* Native3DResourceManager::getShader (UrlRef path, GraphicsShader3DType type)
{
	Native3DGraphicsShader* shader = findShader (path);
	if(shader == nullptr)
	{
		shader = loadShader (path, type);
		if(shader)
			shaderList.add (shader);
	}
	return shader;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DTexture2D* Native3DResourceManager::getTexture (Bitmap* bitmap, TextureFlags3D flags)
{
	Native3DTexture2D* texture = findTexture (bitmap, flags);
	if(texture == nullptr)
	{
		texture = loadTexture (bitmap, flags);
		if(texture)
			textureList.add (NEW TextureItem (texture, bitmap, flags));
	}
	return texture;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Native3DResourceManager::removeAll ()
{
	shaderList.removeAll ();
	textureList.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DGraphicsShader* Native3DResourceManager::findShader (UrlRef path) const
{
	return shaderList.findIf<Native3DGraphicsShader> ([&path] (const Native3DGraphicsShader& s)
		{
			return s.getPath ().isEqualUrl (path); 
		});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DTexture2D* Native3DResourceManager::findTexture (Bitmap* bitmap, TextureFlags3D flags) const
{
	if(bitmap == nullptr)
		return nullptr;

	TextureItem* item = textureList.findIf<TextureItem> ([bitmap, flags] (const TextureItem& item)
		{
			return item.getBitmap () == bitmap && item.getFlags () == flags;
		});
	return item ? item->getTexture () : nullptr;
}

//************************************************************************************************
// Native3DResourceManager::TextureItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Native3DResourceManager::TextureItem, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DResourceManager::TextureItem::TextureItem (Native3DTexture2D* texture, Bitmap* bitmap, TextureFlags3D flags)
: texture (texture),
  bitmap (bitmap),
  flags (flags)
{}

//************************************************************************************************
// Native3DSurface
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Native3DSurface, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DSurface::Native3DSurface ()
: dirty (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Native3DSurface::hasClearColor () const
{
	return content ? (content->getContentHint () != kGraphicsContentOpaque) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Color Native3DSurface::getClearColor () const
{
	return content ? content->getBackColor () : Colors::kTransparentBlack;
}

//************************************************************************************************
// Native3DGraphicsFactory
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Native3DGraphicsFactory, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

IShaderBufferWriter3D* CCL_API Native3DGraphicsFactory::createShaderBufferWriter ()
{
	return NEW ShaderBufferWriter3D;
}

//************************************************************************************************
// Native3DGraphicsDevice
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Native3DGraphicsDevice, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Native3DGraphicsDevice::drawGeometry (IGeometry3D* geometry)
{
	if(!geometry)
		return kResultInvalidPointer;

	auto* vertexBuffer = geometry->getVertexBufferSegment ();
	auto* indexBuffer = geometry->getIndexBufferSegment ();
	if(vertexBuffer == nullptr)
		return kResultFailed;

	setVertexBuffer (vertexBuffer->getBuffer (), vertexBuffer->getStride ());
	uint32 startVertex = vertexBuffer->getStride () > 0 ? vertexBuffer->getOffset () / vertexBuffer->getStride () : 0;
	ASSERT (vertexBuffer->getStride () == 0 || vertexBuffer->getOffset () % vertexBuffer->getStride () == 0)

	if(indexBuffer != nullptr)
	{
		setIndexBuffer (indexBuffer->getBuffer (), kR16_UInt);
		uint32 startIndex = indexBuffer->getOffset () / indexBuffer->getStride ();
		ASSERT (indexBuffer->getOffset () % indexBuffer->getStride () == 0)

		return drawIndexed (startIndex, geometry->getIndexCount (), startVertex);
	}

	return draw (startVertex, geometry->getVertexCount ());
}
