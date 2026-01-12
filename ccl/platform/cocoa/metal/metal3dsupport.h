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
// Filename    : ccl/platform/cocoa/metal/metal3dsupport.h
// Description : Metal 3D Support
//
//************************************************************************************************

#ifndef _metal3dsupport_h
#define _metal3dsupport_h

#include "ccl/gui/graphics/3d/nativegraphics3d.h"

#include "ccl/base/singleton.h"
#include "ccl/platform/cocoa/macutils.h"

#include "ccl/public/system/ilockable.h"

@protocol MTLDevice;
@protocol MTLLibrary;
@protocol MTLRenderPipelineState;
@protocol MTLCommandQueue;
@protocol MTLBuffer;
@protocol MTLRenderCommandEncoder;
@protocol MTLCommandBuffer;
@protocol MTLDepthStencilState;
@protocol MTLTexture;
@protocol MTLSamplerState;

@class MTLRenderPassDescriptor;
@class MTLRenderPipelineDescriptor;
@class MTKView;
@class MTLVertexDescriptor;
@class MetalViewDelegate;

namespace CCL {

class MetalRenderTarget;

//************************************************************************************************
// Metal3DVertexFormat
//************************************************************************************************

class Metal3DVertexFormat: public Native3DVertexFormat
{
public:
	DECLARE_CLASS (Metal3DVertexFormat, Native3DVertexFormat)

	bool create (const VertexElementDescription description[], uint32 count, 
				 const IGraphicsShader3D* shader);
	MTLVertexDescriptor* getVertexDescriptor () const { return vertexDescriptor; }
	
private:
	NSObj<MTLVertexDescriptor> vertexDescriptor;
};

//************************************************************************************************
// Metal3DBuffer
//************************************************************************************************

class Metal3DBuffer: public Native3DGraphicsBuffer
{
public:
	DECLARE_CLASS (Metal3DBuffer, Native3DGraphicsBuffer)

	bool create (GraphicsBuffer3DType type, BufferUsage3D usage,
				 uint32 sizeInBytes, uint32 strideInBytes, const void* initialData);
	id<MTLBuffer> getBuffer () const { return metalBuffer; }
	
	// Native3DGraphicsBuffer
	void* CCL_API map () override;
	void CCL_API unmap () override;
    bool ensureSegmentAlignment (uint32& byteOffset, uint32& size, uint32 stride) const override;

private:
	NSObj<NSObject<MTLBuffer>> metalBuffer;
};

//************************************************************************************************
// Metal3DTexture2D
//************************************************************************************************

class Metal3DTexture2D: public Native3DTexture2D
{
public:
	DECLARE_CLASS (Metal3DTexture2D, Native3DTexture2D)
	
	Metal3DTexture2D ();
	
	id<MTLTexture> getTexture () const { return texture; }
	id<MTLSamplerState> getSampler () const { return sampler; }
	
	PROPERTY_SHARED_AUTO (Threading::ILockable, lock, Lock)
	
	// Native3DTexture2D
	tresult CCL_API copyFromBitmap (IBitmap* bitmap) override;
	using Native3DTexture2D::create;
	bool create (uint32 width, uint32 height, uint32 bytesPerRow, DataFormat3D format, TextureFlags3D flags, const void* initialData) override;

private:
	NSObj<NSObject<MTLTexture>> texture;
	NSObj<NSObject<MTLSamplerState>> sampler;
	
	NSUInteger bytesPerRowSource;
	uint32 width;
	uint32 height;
	bool hasMipmaps;
	bool immutable;

	void setPixels (const void* pixelData);
	void generateMipmaps ();
};

//************************************************************************************************
// Metal3DShader
//************************************************************************************************

class Metal3DShader: public Native3DGraphicsShader
{
public:
	DECLARE_CLASS (Metal3DShader, Native3DGraphicsShader)
	
	static const FileType kMetalSourceFileType;
	static const FileType kMetalCompiledFileType;

	bool create (GraphicsShader3DType type, UrlRef path);
	id<MTLLibrary> getLibrary () { return library; }

	// Native3DGraphicsShader
	ITypeInfo* CCL_API getBufferTypeInfo (int bufferIndex) override;
	
private:
	NSObj<NSObject<MTLLibrary>> library;
};

//************************************************************************************************
// Metal3DResourceManager
//************************************************************************************************

class Metal3DResourceManager: public Native3DResourceManager,
							  public StaticSingleton<Metal3DResourceManager>
{
public:
	DECLARE_CLASS (Metal3DResourceManager, Native3DResourceManager)

private:	
	// Native3DResourceManager
	Native3DGraphicsShader* loadShader (UrlRef path, GraphicsShader3DType type) override;
	Native3DTexture2D* loadTexture (Bitmap* bitmap, TextureFlags3D flags) override;
};

//************************************************************************************************
// Metal3DPipeline
//************************************************************************************************

class Metal3DPipeline: public Native3DGraphicsPipeline
{
public:
	DECLARE_CLASS (Metal3DPipeline, Native3DGraphicsPipeline)

	Metal3DPipeline ();
	
	id<MTLRenderPipelineState> getPipeline (MTKView* view);
	id<MTLDepthStencilState> getDepthStencil () const { return stencilState; }
	Metal3DShader* getShader (int index) const;
	NSUInteger getPrimitiveType () const { return primitiveType; }
	NSUInteger getFillMode () const { return fillMode; }
	float getDepthBias () const { return depthBias; }
	
	// IGraphicsPipeline3D
	tresult CCL_API setPrimitiveTopology (PrimitiveTopology3D topology) override;
	tresult CCL_API setFillMode (FillMode3D mode) override;
	tresult CCL_API setVertexFormat (IVertexFormat3D* format) override;
	tresult CCL_API setVertexShader (IGraphicsShader3D* shader) override;
	tresult CCL_API setPixelShader (IGraphicsShader3D* shader) override;
	tresult CCL_API setDepthTestParameters (const DepthTestParameters3D& parameters) override;

private:
	enum ShaderIndex
	{
		kVertexShaderIndex = 0,
		kPixelShaderIndex
	};
	
	Vector<SharedPtr<Metal3DShader>> shaderList;
	SharedPtr<Metal3DVertexFormat> vertexFormat;
	id<MTLRenderPipelineState> state;
	NSUInteger primitiveType;
	NSUInteger fillMode;
	NSObj<NSObject<MTLDepthStencilState>> stencilState;
	bool depthTestEnabled;
	bool depthWriteEnabled;
	float depthBias;
	NSUInteger sampleCount;
	
	bool changed;
	void reset ();
	id<MTLRenderPipelineState> createPipeline (MTKView* view);
	tresult setShader (int index, IGraphicsShader3D* shader);
	id<MTLDepthStencilState> createDepthStencil (MTKView* view);
};

//************************************************************************************************
// Metal3DGraphicsFactory
//************************************************************************************************

class Metal3DGraphicsFactory: public Native3DGraphicsFactory
{
public:
	DECLARE_CLASS (Metal3DGraphicsFactory, Native3DGraphicsFactory)

	// Native3DGraphicsFactory
	IVertexFormat3D* CCL_API createVertexFormat (const VertexElementDescription* description, uint32 count, 
												const IGraphicsShader3D* shader) override;
	IGraphicsBuffer3D* CCL_API createBuffer (GraphicsBuffer3DType type, BufferUsage3D usage,
												uint32 sizeInBytes, uint32 strideInBytes, const void* initialData = nullptr) override;
	IGraphicsTexture2D* CCL_API createTexture (IBitmap* bitmap, TextureFlags3D flags) override;
	IGraphicsShader3D* CCL_API createShader (GraphicsShader3DType type, UrlRef filename) override;
	IGraphicsShader3D* CCL_API createStockShader (GraphicsShader3DType type, StringID name) override;
	IGraphicsPipeline3D* CCL_API createPipeline () override;
	IShaderParameterSet3D* CCL_API createShaderParameterSet () override;
};

//************************************************************************************************
// Metal3DGraphicsContext
//************************************************************************************************

class Metal3DGraphicsContext: public Native3DGraphicsDevice
{
public:
	DECLARE_CLASS_ABSTRACT (Metal3DGraphicsContext, Native3DGraphicsDevice)

	Metal3DGraphicsContext (MTKView* view);
	~Metal3DGraphicsContext ();
		
	// Native3DGraphicsContext
	tresult CCL_API setPipeline (IGraphicsPipeline3D* pipeline) override;
	tresult CCL_API setVertexBuffer (IGraphicsBuffer3D* buffer, uint32 stride) override;
	tresult CCL_API setIndexBuffer (IGraphicsBuffer3D* buffer, DataFormat3D format) override;
	tresult CCL_API setShaderParameters (IShaderParameterSet3D* parameters) override;
	tresult CCL_API draw (uint32 startVertex, uint32 vertexCount) override;
	tresult CCL_API drawIndexed (uint32 startIndex, uint32 indexCount, int32 baseVertex) override;

protected:
	MTKView* view;
	id<MTLCommandBuffer> commandBuffer;
	id<MTLRenderCommandEncoder> encoder;
	
	SharedPtr<Metal3DPipeline> pipeline;
	Metal3DPipeline* activePipeline;
	SharedPtr<Metal3DBuffer> vertexBuffer;
	SharedPtr<Metal3DBuffer> indexBuffer;
	SharedPtr<Native3DShaderParameterSet> shaderParameters;
	
	uint32 vertexBufferStride;
	DataFormat3D indexBufferFormat;
	
	void prepareEncoder ();
};

//************************************************************************************************
// Metal3DSurface
//************************************************************************************************

class Metal3DSurface: public Native3DSurface
{
public:
	DECLARE_CLASS (Metal3DSurface, Native3DSurface)

	Metal3DSurface ();
	~Metal3DSurface ();

	MTKView* getView () const { return view; }
	void updateSampleCount ();
	void draw ();

	// Native3DSurface
	void setContent (IGraphicsContent3D* content) override;
	void setSize (const Rect& size) override;
	void applyMultisampling (int sampleCount) override;

private:
	bool firstDraw;
	NSObj<MTKView> view;
};

//************************************************************************************************
// Metal3DSupport
//************************************************************************************************

class Metal3DSupport: public INative3DSupport,
					  public StaticSingleton<Metal3DSupport>
{
public:
	void shutdown3D ();

	// INative3DSupport
	Native3DGraphicsFactory& get3DFactory () override;
	Native3DSurface* create3DSurface () override;

protected:
	Metal3DGraphicsFactory factory;
};

} // namespace CCL

#endif // _metal3dsupport_h

