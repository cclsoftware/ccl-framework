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
// Filename    : ccl/platform/shared/opengles/opengles3dsupport.h
// Description : OpenGLES 3D Support
//
//************************************************************************************************

#ifndef _opengles3dsupport_h
#define _opengles3dsupport_h

#include "ccl/platform/shared/opengles/openglesimage.h"
#include "ccl/platform/shared/skia/skiaglue.h"

#include "ccl/gui/graphics/3d/nativegraphics3d.h"

namespace CCL {

class OpenGLES3DGraphicsContext;

//************************************************************************************************
// OpenGLES3DVertexFormat
//************************************************************************************************

class OpenGLES3DVertexFormat: public Native3DVertexFormat
{
public:
	DECLARE_CLASS (OpenGLES3DVertexFormat, Native3DVertexFormat)

	OpenGLES3DVertexFormat ();

	void apply (uint64 offset, uint32 stride);
	bool create (const VertexElementDescription description[], uint32 count, 
				 const IGraphicsShader3D* shader);
	int countAttributes () const;

private:
	struct ElementFormat
	{
		uint32 index = 0;
		int32 size = 0;
		uint32 type = 0;
		uint32 offset = 0;
	};
	Vector<ElementFormat> elements;
};

//************************************************************************************************
// OpenGLES3DBuffer
//************************************************************************************************

class OpenGLES3DBuffer: public Native3DGraphicsBuffer
{
public:
	DECLARE_CLASS (OpenGLES3DBuffer, Native3DGraphicsBuffer)

	OpenGLES3DBuffer ();
	~OpenGLES3DBuffer ();

	PROPERTY_VARIABLE (uint32, bufferId, BufferID)
	PROPERTY_BOOL (usingGPUMemory, UsingGPUMemory)
	PROPERTY_AUTO_POINTER (Buffer, memory, Memory)

	bool create (GraphicsBuffer3DType type, BufferUsage3D usage,
				 uint32 sizeInBytes, uint32 strideInBytes, const void* initialData);
	void destroy ();

	// Native3DGraphicsBuffer
	void* CCL_API map () override;
	void CCL_API unmap () override;

private:
	uint32 target;
	uint32 bufferUsage;
	uint32 memoryAlignment;
	int mapCount;
	
	// Native3DGraphicsBuffer
	bool ensureSegmentAlignment (uint32& byteOffset, uint32& size, uint32 stride) const override;
};

//************************************************************************************************
// OpenGLES3DTexture2D
//************************************************************************************************

class OpenGLES3DTexture2D: public Native3DTexture2D
{
public:
	DECLARE_CLASS (OpenGLES3DTexture2D, Native3DTexture2D)
	
	OpenGLES3DTexture2D ();
	
	PROPERTY_OBJECT (OpenGLESImage, image, Image)

	void destroy ();
	
	// Native3DTexture2D
	tresult CCL_API copyFromBitmap (IBitmap* bitmap) override;
	using Native3DTexture2D::create;
	bool create (uint32 width, uint32 height, uint32 bytesPerRow, DataFormat3D format, TextureFlags3D flags, const void* initialData) override;
};

//************************************************************************************************
// OpenGLES3DShader
//************************************************************************************************

class OpenGLES3DShader: public Native3DGraphicsShader
{
public:
	DECLARE_CLASS (OpenGLES3DShader, Native3DGraphicsShader)

	OpenGLES3DShader ();
	~OpenGLES3DShader ();

	static const FileType kFileType;
		
	PROPERTY_VARIABLE (uint32, shaderId, ShaderID)
	
	bool create (GraphicsShader3DType type, UrlRef path);
	
	// Native3DGraphicsShader
	ITypeInfo* CCL_API getBufferTypeInfo (int bufferIndex) override;
	
private:
	bool load ();
	void reset ();
};

//************************************************************************************************
// OpenGLES3DPipeline
//************************************************************************************************

class OpenGLES3DPipeline: public Native3DGraphicsPipeline
{
public:
	DECLARE_CLASS (OpenGLES3DPipeline, Native3DGraphicsPipeline)

	OpenGLES3DPipeline ();
	~OpenGLES3DPipeline ();

	PROPERTY_SHARED_POINTER (OpenGLES3DShader, vertexShader, VertexShader)
	PROPERTY_SHARED_POINTER (OpenGLES3DShader, pixelShader, PixelShader)
	PROPERTY_SHARED_POINTER (OpenGLES3DVertexFormat, vertexFormat, VertexFormat)

	void applyTo (OpenGLES3DGraphicsContext& context, uint64 vertexOffset, uint32 vertexStride) const;

	// IGraphicsPipeline3D
	tresult CCL_API setPrimitiveTopology (PrimitiveTopology3D topology) override;
	tresult CCL_API setFillMode (FillMode3D mode) override;
	tresult CCL_API setVertexFormat (IVertexFormat3D* format) override;
	tresult CCL_API setVertexShader (IGraphicsShader3D* shader) override;
	tresult CCL_API setPixelShader (IGraphicsShader3D* shader) override;
	tresult CCL_API setDepthTestParameters (const DepthTestParameters3D& parameters) override;

private:
	uint32 topology;
	uint32 programId;
	bool enableDepthTest;
	bool enableDepthWrite;
	float depthBias;

	void updateProgram ();
};

//************************************************************************************************
// OpenGLES3DGraphicsContext
//************************************************************************************************

class OpenGLES3DGraphicsContext: public Native3DGraphicsDevice
{
public:
	DECLARE_CLASS (OpenGLES3DGraphicsContext, Native3DGraphicsDevice)

	OpenGLES3DGraphicsContext ();

	PROPERTY_VARIABLE (uint32, topology, Topology)
	
	// Native3DGraphicsContext
	tresult CCL_API setPipeline (IGraphicsPipeline3D* pipeline) override;
	tresult CCL_API setVertexBuffer (IGraphicsBuffer3D* buffer, uint32 stride) override;
	tresult CCL_API setIndexBuffer (IGraphicsBuffer3D* buffer, DataFormat3D format) override;
	tresult CCL_API setShaderParameters (IShaderParameterSet3D* parameters) override;
	tresult CCL_API draw (uint32 startVertex, uint32 vertexCount) override;
	tresult CCL_API drawIndexed (uint32 startIndex, uint32 indexCount, int32 baseVertex) override;

protected:
	SharedPtr<OpenGLES3DPipeline> pipeline;
	SharedPtr<OpenGLES3DBuffer> vertexBuffer;
	SharedPtr<OpenGLES3DBuffer> indexBuffer;
	SharedPtr<Native3DShaderParameterSet> shaderParameters;
	
	uint32 bufferStride;
	DataFormat3D indexBufferFormat;

	tresult prepareDrawing (uint32 startVertex);
	void bindPipeline (uint64 vertexOffset, uint32 vertexStride);
	void bindDescriptorSet ();
};

//************************************************************************************************
// OpenGLES3DResourceManager
//************************************************************************************************

class OpenGLES3DResourceManager: public Native3DResourceManager,
							     public StaticSingleton<OpenGLES3DResourceManager>
{
public:
	DECLARE_CLASS (OpenGLES3DResourceManager, Native3DResourceManager)

	void shutdown ();
	
private:
	// Native3DResourceManager
	Native3DGraphicsShader* loadShader (UrlRef path, GraphicsShader3DType type) override;
	Native3DTexture2D* loadTexture (Bitmap* bitmap, TextureFlags3D flags) override;
};

//************************************************************************************************
// OpenGLES3DGraphicsFactory
//************************************************************************************************

class OpenGLES3DGraphicsFactory: public Native3DGraphicsFactory
{
public:
	DECLARE_CLASS (OpenGLES3DGraphicsFactory, Native3DGraphicsFactory)

	// Native3DGraphicsFactory
	IVertexFormat3D* CCL_API createVertexFormat (const VertexElementDescription* description, uint32 count, const IGraphicsShader3D* shader) override;
	IGraphicsBuffer3D* CCL_API createBuffer (GraphicsBuffer3DType type, BufferUsage3D usage, uint32 sizeInBytes, uint32 strideInBytes, const void* initialData = nullptr) override;
	IGraphicsTexture2D* CCL_API createTexture (IBitmap* bitmap, TextureFlags3D flags) override;
	IGraphicsShader3D* CCL_API createShader (GraphicsShader3DType type, UrlRef filename) override;
	IGraphicsShader3D* CCL_API createStockShader (GraphicsShader3DType type, StringID name) override;
	IGraphicsPipeline3D* CCL_API createPipeline () override;
	IShaderParameterSet3D* CCL_API createShaderParameterSet () override;
};

//************************************************************************************************
// OpenGLES3DSurface
//************************************************************************************************

class OpenGLES3DSurface: public Native3DSurface
{
public:
	DECLARE_CLASS (OpenGLES3DSurface, Native3DSurface)
	
	OpenGLES3DSurface ();
	~OpenGLES3DSurface ();

	bool create (GrRecordingContext* skiaContext, float scaleFactor);
	void destroy ();
	bool isValid () const;
	
	void render (OpenGLES3DGraphicsContext& context);
	sk_sp<SkImage> getSkiaImage () const { return textureImage; }
	
	// Native3DSurface
	void setContent (IGraphicsContent3D* content) override;
	void setSize (const Rect& size) override;
	void applyMultisampling (int sampleCount) override;
	
protected:
	uint32 sampleCount;
	OpenGLESImage texture;
	sk_sp<SkImage> textureImage;
	GrBackendTexture backendTexture;
	uint32 depthBufferId;
	Rect viewPortRect;
	float scaleFactor;

	void updateSkiaImage ();
};

//************************************************************************************************
// OpenGLES3DSupport
//************************************************************************************************

class OpenGLES3DSupport: public INative3DSupport,
					     public StaticSingleton<OpenGLES3DSupport>
{
public:
	void shutdown3D ();

	// INative3DSupport
	Native3DGraphicsFactory& get3DFactory () override;
	Native3DSurface* create3DSurface () override;

protected:
	OpenGLES3DGraphicsFactory factory;
};

} // namespace CCL

#endif // _opengles3dsupport_h

