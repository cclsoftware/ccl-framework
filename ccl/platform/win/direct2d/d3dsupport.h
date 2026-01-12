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
// Filename    : ccl/platform/win/direct2d/d3dsupport.h
// Description : Direct3D Support
//
//************************************************************************************************

#ifndef _ccl_d3dsupport_h
#define _ccl_d3dsupport_h

#include "ccl/gui/graphics/3d/nativegraphics3d.h"

#include "ccl/platform/win/direct2d/dxgiengine.h"
#include "ccl/platform/win/direct2d/d2dinterop.h"

#include <d3dcompiler.h>

namespace CCL {
namespace Win32 {

//************************************************************************************************
// D3DVertexFormat
//************************************************************************************************

class D3DVertexFormat: public Native3DVertexFormat
{
public:
	DECLARE_CLASS (D3DVertexFormat, Native3DVertexFormat)

	bool create (const VertexElementDescription description[], uint32 count, 
				 const IGraphicsShader3D* shader);

	ID3D11InputLayout* getInputLayout () { return inputLayout; }

private:
	ComPtr<ID3D11InputLayout> inputLayout;
};

//************************************************************************************************
// D3DBuffer
//************************************************************************************************

class D3DBuffer: public Native3DGraphicsBuffer
{
public:
	DECLARE_CLASS (D3DBuffer, Native3DGraphicsBuffer)

	/** Size, in bytes, per shader constant */
	static constexpr uint32 kConstantSize = 16;

	/** Alignment requirement for offsets and counts within a constant buffer */
	static constexpr uint32 kConstantByteAlignment = kConstantSize * 16;

	bool create (Type type, BufferUsage3D usage, 
				 uint32 sizeInBytes, uint32 strideInBytes, const void* initialData);
	
	ID3D11Buffer* getBuffer () { return buffer; }

	// Native3DGraphicsBuffer
	void* CCL_API map () override;
	void CCL_API unmap () override;

private:
	ComPtr<ID3D11Buffer> buffer;

	// Native3DGraphicsBuffer
	bool ensureSegmentAlignment (uint32& byteOffset, uint32& size, uint32 stride) const override;
};

//************************************************************************************************
// D3DTexture2D
//************************************************************************************************

class D3DTexture2D: public Native3DTexture2D
{
public:
	DECLARE_CLASS (D3DTexture2D, Native3DTexture2D)

	D3DTexture2D ();

	PROPERTY_VARIABLE (D3D11_TEXTURE_ADDRESS_MODE, addressMode, AddressMode)

	bool create (uint32 width, uint32 height, uint32 bytesPerRow, DataFormat3D format,
					TextureFlags3D flags, const void* initialData) override;

	ID3D11ShaderResourceView* getResourceView () { return resourceView; }

	// Native3DTexture2D
	tresult CCL_API copyFromBitmap (IBitmap* bitmap) override;

private:
	ComPtr<ID3D11Texture2D> texture;
	ComPtr<ID3D11ShaderResourceView> resourceView;
	uint32 mipLevels;
	bool immutable;
};

//************************************************************************************************
// D3DShader
//************************************************************************************************

class D3DShader: public Native3DGraphicsShader
{
public:
	DECLARE_CLASS (D3DShader, Native3DGraphicsShader)

	static const FileType kHlslFileType;
	static const FileType kCsoFileType;

	bool create (GraphicsShader3DType type, UrlRef path);
	
	ID3D11DeviceChild* getShader () { return shader; }

	// Native3DGraphicsShader
	ITypeInfo* CCL_API getBufferTypeInfo (int bufferIndex) override;

private:
	static constexpr CStringPtr kDefaultVertexShaderTarget = "vs_5_0";
	static constexpr CStringPtr kDefaultPixelShaderTarget = "ps_5_0";

	ComPtr<ID3D11ShaderReflection> shaderReflection;
	ComPtr<ID3D11DeviceChild> shader;

	bool load ();
	bool compile (const void* buffer, uint32 sizeOfBuffer);
	ID3D11ShaderReflection* getReflection ();
};

//************************************************************************************************
// D3DResourceManager
//************************************************************************************************

class D3DResourceManager: public Native3DResourceManager,
                          public StaticSingleton<D3DResourceManager>
{
public:
	D3DResourceManager ();

	void shutdown ();
	ID3D11SamplerState* getSampler (D3D11_TEXTURE_ADDRESS_MODE addressMode, int textureIndex) const;

	DECLARE_CLASS (D3DResourceManager, Native3DResourceManager)

private:
	static const int kNumAddressModes = 4;
	mutable FixedSizeVector<FixedSizeVector<ComPtr<ID3D11SamplerState>, Native3DShaderParameterSet::kMaxTextureCount>, kNumAddressModes> samplers;

	// Native3DResourceManager
	Native3DGraphicsShader* loadShader (UrlRef path, GraphicsShader3DType type) override;
	Native3DTexture2D* loadTexture (Bitmap* bitmap, TextureFlags3D flags) override;
};

//************************************************************************************************
// D3DPipeline
//************************************************************************************************

class D3DPipeline: public Native3DGraphicsPipeline
{
public:
	DECLARE_CLASS (D3DPipeline, Native3DGraphicsPipeline)

	D3DPipeline ();

	void applyTo (ID3D11DeviceContext* deviceContext) const;

	// IGraphicsPipeline3D
	tresult CCL_API setPrimitiveTopology (PrimitiveTopology3D topology) override;
	tresult CCL_API setFillMode (FillMode3D mode) override;
	tresult CCL_API setVertexFormat (IVertexFormat3D* format) override;
	tresult CCL_API setVertexShader (IGraphicsShader3D* shader) override;
	tresult CCL_API setPixelShader (IGraphicsShader3D* shader) override;
	tresult CCL_API setDepthTestParameters (const DepthTestParameters3D& parameters) override;

private:
	D3D_PRIMITIVE_TOPOLOGY d3dTopology;
	ComPtr<ID3D11RasterizerState> rasterizerState;
	ComPtr<ID3D11DepthStencilState> depthStencilState;
	SharedPtr<D3DVertexFormat> vertexFormat;
	ComPtr<ID3D11VertexShader> d3dVertexShader;
	ComPtr<ID3D11PixelShader> d3dPixelShader;
	FillMode3D fillMode;
	int depthBias;

	void updateRasterizerState ();
};

//************************************************************************************************
// D3DGraphicsFactory
//************************************************************************************************

class D3DGraphicsFactory: public Native3DGraphicsFactory
{
public:
	DECLARE_CLASS (D3DGraphicsFactory, Native3DGraphicsFactory)

	static D3DGraphicsFactory& getD3DInstance ();

	ID3D11RasterizerState* createRasterizerState (FillMode3D mode, int depthBias);
	ID3D11RasterizerState* getRasterizerStateForMode (FillMode3D mode);
	ID3D11BlendState* getBlendState ();
	ID3D11DepthStencilState* createDepthStencilState (bool depthTestEnabled, bool depthWriteEnabled);
	void discardCachedResources ();

	// Native3DGraphicsFactory
	IVertexFormat3D* CCL_API createVertexFormat (const VertexElementDescription description[], uint32 count,
												 const IGraphicsShader3D* shader) override;
	IGraphicsBuffer3D* CCL_API createBuffer (GraphicsBuffer3DType type, BufferUsage3D usage,
											 uint32 sizeInBytes, uint32 strideInBytes, const void* initialData = nullptr) override;
	IGraphicsTexture2D* CCL_API createTexture (IBitmap* bitmap, TextureFlags3D flags) override;
	IGraphicsShader3D* CCL_API createShader (GraphicsShader3DType type, UrlRef filename) override;
	IGraphicsShader3D* CCL_API createStockShader (GraphicsShader3DType type, StringID name) override;
	IGraphicsPipeline3D* CCL_API createPipeline () override;
	IShaderParameterSet3D* CCL_API createShaderParameterSet () override;

protected:
	ComPtr<ID3D11BlendState> blendState;
	ComPtr<ID3D11RasterizerState> rasterizerStateSolid;
	ComPtr<ID3D11RasterizerState> rasterizerStateWireframe;
};

//************************************************************************************************
// D3DSurface
//************************************************************************************************

class D3DSurface: public Native3DSurface
{
public:
	DECLARE_CLASS (D3DSurface, Native3DSurface)

	D3DSurface ();

	bool create (float scaleFactor);
	void destroy ();
	bool isValid () const;
	void blendToBackbuffer (ID2D1DeviceContext* context);

	ID3D11RenderTargetView* getRenderTargetView () const { return renderTargetView; }
	ID3D11DepthStencilView* getDepthStencilView () const { return depthStencilView; }
	ID3D11Texture2D* getOffscreenTexture () const { return offscreenTexture; }
	ID3D11Texture2D* getResolveTexture () const { return resolveTexture; }
	RectRef getViewPortRect () const { return viewPortRect; }

	// Native3DSurface
	void setContent (IGraphicsContent3D* content) override;
	void setSize (const Rect& size) override;
	void applyMultisampling (int sampleCount) override;

protected:
	Rect viewPortRect;
	DXGI_SAMPLE_DESC multisamplingDesc;
	float scaleFactor;

	ComPtr<ID3D11Texture2D> offscreenTexture;
	ComPtr<ID3D11Texture2D> resolveTexture;
	ComPtr<ID3D11RenderTargetView> renderTargetView;
	ComPtr<ID3D11DepthStencilView> depthStencilView;
	ComPtr<ID2D1Bitmap> bitmap;
};

//************************************************************************************************
// D3DGraphicsContext
//************************************************************************************************

class D3DGraphicsContext: public Native3DGraphicsDevice
{
public:
	DECLARE_CLASS_ABSTRACT (D3DGraphicsContext, Native3DGraphicsDevice)

	D3DGraphicsContext (const D3DSurface& surface);
	~D3DGraphicsContext ();

	// Native3DGraphicsContext
	tresult CCL_API setPipeline (IGraphicsPipeline3D* pipeline) override;
	tresult CCL_API setVertexBuffer (IGraphicsBuffer3D* buffer, uint32 stride) override;
	tresult CCL_API setIndexBuffer (IGraphicsBuffer3D* buffer, DataFormat3D format) override;
	tresult CCL_API setShaderParameters (IShaderParameterSet3D* parameters) override;
	tresult CCL_API draw (uint32 startVertex, uint32 vertexCount) override;
	tresult CCL_API drawIndexed (uint32 startIndex, uint32 indexCount, int32 baseVertex) override;

protected:
	const D3DSurface& surface;
	ID3D11DeviceContext1* deviceContext;
	ID3D11RenderTargetView* renderTargetView;
	ID3D11DepthStencilView* depthStencilView;

	ComPtr<ID3D11BlendState> oldBlendState;
	float oldBlendFactors[4];
	uint32 oldSampleMask;
};

//************************************************************************************************
// D3DSupport
//************************************************************************************************

class D3DSupport: public INative3DSupport
{
public:
	// INative3DSupport
	Native3DGraphicsFactory& get3DFactory () override;
	Native3DSurface* create3DSurface () override;

protected:
	void shutdown3D ();
	void handleError3D ();
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_d3dsupport_h
