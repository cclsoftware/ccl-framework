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
// Filename    : ccl/platform/linux/gui/native3dsupport.linux.cpp
// Description : Linux 3D Graphics Support using Vulkan or OpenGL ES 2
//
//************************************************************************************************

#include "ccl/platform/linux/skia/skiaengine.linux.h"

#include "ccl/gui/graphics/3d/nativegraphics3d.h"

namespace CCL {

//************************************************************************************************
// Linux3DGraphicsFactory
//************************************************************************************************

class Linux3DGraphicsFactory: public Native3DGraphicsFactory
{
public:
	DECLARE_CLASS (Linux3DGraphicsFactory, Native3DGraphicsFactory)

	Linux3DGraphicsFactory ();

	// Native3DGraphicsFactory
	IVertexFormat3D* CCL_API createVertexFormat (const VertexElementDescription* description, uint32 count, const IGraphicsShader3D* shader) override;
	IGraphicsBuffer3D* CCL_API createBuffer (GraphicsBuffer3DType type, BufferUsage3D usage, uint32 sizeInBytes, uint32 strideInBytes, const void* initialData = nullptr) override;
	IGraphicsTexture2D* CCL_API createTexture (IBitmap* bitmap, TextureFlags3D flags) override;
	IGraphicsShader3D* CCL_API createShader (GraphicsShader3DType type, UrlRef filename) override;
	IGraphicsShader3D* CCL_API createStockShader (GraphicsShader3DType type, StringID name) override;
	IGraphicsPipeline3D* CCL_API createPipeline () override;
	IShaderParameterSet3D* CCL_API createShaderParameterSet () override;

private:
	AutoPtr<Native3DGraphicsFactory> factory;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Linux3DGraphicsFactory
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (Native3DGraphicsFactory, Linux3DGraphicsFactory)
DEFINE_CLASS_HIDDEN (Linux3DGraphicsFactory, Native3DGraphicsFactory)

//////////////////////////////////////////////////////////////////////////////////////////////////

Linux3DGraphicsFactory::Linux3DGraphicsFactory ()
: factory (nullptr)
{
	LinuxSkiaEngine* engine = LinuxSkiaEngine::getInstance ();
	if(engine)
		factory = engine->create3DGraphicsFactory ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IVertexFormat3D* CCL_API Linux3DGraphicsFactory::createVertexFormat (const VertexElementDescription description[], uint32 count, const IGraphicsShader3D* shader)
{
	if(factory)
		return factory->createVertexFormat (description, count, shader);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsBuffer3D* CCL_API Linux3DGraphicsFactory::createBuffer (GraphicsBuffer3DType type, BufferUsage3D usage,
								uint32 sizeInBytes, uint32 strideInBytes, const void* initialData)
{
	if(factory)
		return factory->createBuffer (type, usage, sizeInBytes, strideInBytes, initialData);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsTexture2D* CCL_API Linux3DGraphicsFactory::createTexture (IBitmap* bitmap, TextureFlags3D flags)
{
	if(factory)
		return factory->createTexture (bitmap, flags);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsShader3D* CCL_API Linux3DGraphicsFactory::createShader (GraphicsShader3DType type, UrlRef path)
{
	if(factory)
		return factory->createShader (type, path);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsShader3D* CCL_API Linux3DGraphicsFactory::createStockShader (GraphicsShader3DType type, StringID name)
{
	if(factory)
		return factory->createStockShader (type, name);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsPipeline3D* CCL_API Linux3DGraphicsFactory::createPipeline ()
{
	if(factory)
		return factory->createPipeline ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IShaderParameterSet3D* CCL_API Linux3DGraphicsFactory::createShaderParameterSet ()
{
	if(factory)
		return factory->createShaderParameterSet ();
	return nullptr;
}
