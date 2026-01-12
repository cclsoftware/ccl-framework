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
// Filename    : ccl/platform/win/direct2d/d3dsupport.cpp
// Description : Direct3D Support
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/win/direct2d/d3dsupport.h"
#include "ccl/platform/win/direct2d/d2dbitmap.h"

#include "ccl/gui/graphics/3d/shader/shaderreflection3d.h"
#include "ccl/gui/graphics/imaging/bitmap.h"

#include "ccl/public/gui/graphics/dpiscale.h"

#include "ccl/public/math/mathprimitives.h"

#include "ccl/base/storage/file.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// DXGI Format Helper
//************************************************************************************************

static constexpr struct
	{
		DataFormat3D format;
		DXGI_FORMAT dxgiFormat;
	} kDxgiFormatMap[] =
	{
		{ kR8_Int,             DXGI_FORMAT_R8_SINT             },
		{ kR8_UInt,            DXGI_FORMAT_R8_UINT             },
		{ kR16_Int,            DXGI_FORMAT_R16_SINT            },
		{ kR16_UInt,           DXGI_FORMAT_R16_UINT            },
		{ kR32_Int,            DXGI_FORMAT_R32_SINT            },
		{ kR32_UInt,           DXGI_FORMAT_R32_UINT            },
		{ kR32_Float,          DXGI_FORMAT_R32_FLOAT           },
		{ kR8G8_Int,           DXGI_FORMAT_R8G8_SINT           },
		{ kR8G8_UInt,          DXGI_FORMAT_R8G8_UINT           },
		{ kR16G16_Int,         DXGI_FORMAT_R16G16_SINT         },
		{ kR16G16_UInt,        DXGI_FORMAT_R16G16_UINT         },
		{ kR32G32_Int,         DXGI_FORMAT_R32G32_SINT         },
		{ kR32G32_UInt,        DXGI_FORMAT_R32G32_UINT         },
		{ kR32G32_Float,       DXGI_FORMAT_R32G32_FLOAT        },
		{ kR32G32B32_Int,      DXGI_FORMAT_R32G32B32_SINT      },
		{ kR32G32B32_UInt,     DXGI_FORMAT_R32G32B32_UINT      },
		{ kR32G32B32_Float,    DXGI_FORMAT_R32G32B32_FLOAT     },
		{ kR32G32B32A32_Int,   DXGI_FORMAT_R32G32B32A32_SINT   },
		{ kR32G32B32A32_UInt,  DXGI_FORMAT_R32G32B32A32_UINT   },
		{ kR32G32B32A32_Float, DXGI_FORMAT_R32G32B32A32_FLOAT  },
		{ kR8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM },
		{ kB8G8R8A8_UNORM,      DXGI_FORMAT_B8G8R8A8_UNORM }
	};

static constexpr DXGI_FORMAT getDxgiFormat (DataFormat3D format)
{
	for(const auto& entry : kDxgiFormatMap)
	{
		if(entry.format == format)
			return entry.dxgiFormat;
	}

	return DXGI_FORMAT_UNKNOWN;
}

//************************************************************************************************
// D3D11 Usage Helper
//************************************************************************************************

static constexpr D3D11_USAGE getD3D11Usage (BufferUsage3D usage)
{
	constexpr D3D11_USAGE kD3D11Values[4] =
	{
		D3D11_USAGE_DEFAULT,
		D3D11_USAGE_IMMUTABLE,
		D3D11_USAGE_DYNAMIC,
		D3D11_USAGE_STAGING
	};

	ASSERT (usage >= kBufferUsageDefault && usage <= kBufferUsageStaging)
	return kD3D11Values[usage];
}

} // namespace Win32
} // namespace CCL

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// D3DSurface
//************************************************************************************************

DEFINE_CLASS_HIDDEN (D3DSurface, Native3DSurface)

//////////////////////////////////////////////////////////////////////////////////////////////////

D3DSurface::D3DSurface ()
: multisamplingDesc {1, 0},
  scaleFactor (1.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D3DSurface::create (float _scaleFactor)
{
	scaleFactor = _scaleFactor;

	ASSERT (!bitmap.isValid ())
	ASSERT (!renderTargetView.isValid ())
	ASSERT (!depthStencilView.isValid ())
	ASSERT (!offscreenTexture.isValid ())
	ASSERT (!resolveTexture.isValid ())

	DXGIEngine& engine = DXGIEngine::instance ();
	ID3D11Device* device = engine.getDirect3dDevice ();

	viewPortRect = PixelRect (size, scaleFactor);

	if(content)
		applyMultisampling (content->getMultisampling ());

	D3D11_TEXTURE2D_DESC textureDesc {};
	textureDesc.Width = viewPortRect.getWidth ();
	textureDesc.Height = viewPortRect.getHeight ();
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	textureDesc.SampleDesc = multisamplingDesc;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = (multisamplingDesc.Count > 1) ? D3D11_BIND_RENDER_TARGET : D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	
	HRESULT hr = device->CreateTexture2D (&textureDesc, nullptr, &offscreenTexture);
	if(FAILED (hr))
		return false;

	textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	ComPtr<ID3D11Texture2D> depthStencilBuffer;
	hr = device->CreateTexture2D (&textureDesc, nullptr, &depthStencilBuffer);
	if(FAILED (hr))
		return false;

	if(multisamplingDesc.Count > 1)
	{
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		HRESULT hr = device->CreateTexture2D (&textureDesc, nullptr, &resolveTexture);
		if(FAILED (hr))
			return false;
	}

	D3D11_RENDER_TARGET_VIEW_DESC renderDesc {};
	renderDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	renderDesc.ViewDimension = (multisamplingDesc.Count == 1) ? D3D11_RTV_DIMENSION_TEXTURE2D : D3D11_RTV_DIMENSION_TEXTURE2DMS;
	renderDesc.Texture2D.MipSlice = 0;
	hr = device->CreateRenderTargetView (offscreenTexture, &renderDesc, renderTargetView);
	if(FAILED (hr))
		return false;

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDesc {};
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.ViewDimension = (multisamplingDesc.Count == 1) ? D3D11_DSV_DIMENSION_TEXTURE2D : D3D11_DSV_DIMENSION_TEXTURE2DMS;
	depthStencilDesc.Texture2D.MipSlice = 0;
	hr = device->CreateDepthStencilView (depthStencilBuffer, &depthStencilDesc, &depthStencilView);
	if(FAILED (hr))
		return false;

	ComPtr<IDXGISurface1> dxgiSurface;
	hr = (multisamplingDesc.Count > 1) ? resolveTexture.as (dxgiSurface) : offscreenTexture.as (dxgiSurface);
	if(FAILED (hr))
		return false;

	D2D1_BITMAP_PROPERTIES bitmapProperties {};
	bitmapProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	bitmapProperties.dpiX = DpiScale::getDpi (scaleFactor);
	bitmapProperties.dpiY = DpiScale::getDpi (scaleFactor);
	hr = engine.getDirect2dDeviceContext ()->CreateSharedBitmap (__uuidof (IDXGISurface1), reinterpret_cast<void*> (static_cast<IDXGISurface1*> (dxgiSurface)), &bitmapProperties, &bitmap);
	if(FAILED (hr))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D3DSurface::destroy ()
{
	bitmap.release ();
	renderTargetView.release ();
	depthStencilView.release ();
	resolveTexture.release ();
	offscreenTexture.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D3DSurface::isValid () const
{
	return renderTargetView.isValid () && depthStencilView.isValid () && bitmap.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D3DSurface::setContent (IGraphicsContent3D* _content)
{
	SuperClass::setContent (_content);
	int sampleCount = content ? content->getMultisampling () : 1;
	if(sampleCount != multisamplingDesc.Count)
	{
		destroy (); // D3D objects need to be recreated. The render target will call create in the next render call.
		applyMultisampling (sampleCount);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D3DSurface::setSize (const Rect& _size)
{
	if(_size == size)
		return;
	SuperClass::setSize (_size);
	destroy (); // D3D objects need to be recreated. The render target will call create in the next render call.
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D3DSurface::applyMultisampling (int sampleCount)
{
	sampleCount = ccl_upperPowerOf2 (sampleCount / scaleFactor);

	if(sampleCount == multisamplingDesc.Count)
		return;

	DXGIEngine& engine = DXGIEngine::instance ();
	ID3D11Device* device = engine.getDirect3dDevice ();

	UINT qualityLevels = 0;
	if(!FAILED (device->CheckMultisampleQualityLevels (DXGI_FORMAT_B8G8R8A8_UNORM, sampleCount, &qualityLevels)) && qualityLevels != 0)
	{
		multisamplingDesc.Count = sampleCount;
		multisamplingDesc.Quality = qualityLevels - 1;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D3DSurface::blendToBackbuffer (ID2D1DeviceContext* context)
{
	ASSERT (bitmap.isValid ())
	if(bitmap.isValid ())
		context->DrawBitmap (bitmap, D2DInterop::toRectF (getSize ()), 1.f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, D2DInterop::toRectF (getSize ().getSize ()));
}

//************************************************************************************************
// D3DSupport
//************************************************************************************************

void D3DSupport::shutdown3D ()
{
	D3DResourceManager::instance ().shutdown ();
	D3DGraphicsFactory::getD3DInstance ().discardCachedResources ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D3DSupport::handleError3D ()
{
	D3DGraphicsFactory::getD3DInstance ().discardCachedResources ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DGraphicsFactory& D3DSupport::get3DFactory ()
{
	return D3DGraphicsFactory::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DSurface* D3DSupport::create3DSurface ()
{
	return NEW D3DSurface;
}

//************************************************************************************************
// D3DVertexFormat
//************************************************************************************************

DEFINE_CLASS_HIDDEN (D3DVertexFormat, Native3DVertexFormat)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D3DVertexFormat::create (const VertexElementDescription description[], uint32 count, const IGraphicsShader3D* shader)
{
	if(count == 0)
		return true;

	Vector<D3D11_INPUT_ELEMENT_DESC> d3dDescription;
	d3dDescription.setCount (count);

	for(uint32 i = 0; i < count; i++)
	{
		d3dDescription[i].SemanticName = description[i].semanticName;
		d3dDescription[i].SemanticIndex = 0;
		d3dDescription[i].Format = getDxgiFormat (description[i].format);
		d3dDescription[i].InputSlot = 0;
		d3dDescription[i].AlignedByteOffset = i == 0 ? 0 : D3D11_APPEND_ALIGNED_ELEMENT;
		d3dDescription[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		d3dDescription[i].InstanceDataStepRate = 0;

		ASSERT (d3dDescription[i].Format != DXGI_FORMAT_UNKNOWN);
	}

	ID3D11Device* device = DXGIEngine::instance ().getDirect3dDevice ();
	HRESULT hr = device->CreateInputLayout (d3dDescription.getItems (), d3dDescription.count (),
											shader->getBlobAddress (), shader->getBlobSize (), &inputLayout);
	return SUCCEEDED (hr);
}

//************************************************************************************************
// D3DBuffer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (D3DBuffer, Native3DGraphicsBuffer)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D3DBuffer::create (Type _type, BufferUsage3D usage, uint32 sizeInBytes, uint32 strideInBytes, const void* initialData)
{
	type = _type;

	uint32 offset = 0;
	if(!ensureSegmentAlignment (offset, sizeInBytes, strideInBytes))
		return false;

	D3D11_BUFFER_DESC desc {};
	desc.ByteWidth = sizeInBytes;
	desc.Usage = getD3D11Usage (usage);

	switch(_type)
	{
	case kVertexBuffer:
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		break;

	case kIndexBuffer:
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		break;

	case kConstantBuffer:
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		break;

	case kShaderResource:
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		break;

	default:
		return nullptr;
	}

	desc.CPUAccessFlags = (usage == kBufferUsageDynamic) ? D3D11_CPU_ACCESS_WRITE : 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = strideInBytes;

	ID3D11Device* device = DXGIEngine::instance ().getDirect3dDevice ();
	HRESULT hr = S_OK;

	if(initialData == nullptr)
		hr = device->CreateBuffer (&desc, nullptr, &buffer);
	else
	{
		D3D11_SUBRESOURCE_DATA subresourceData = {initialData, 0, 0};
		hr = device->CreateBuffer (&desc, &subresourceData, &buffer);
	}

	if(FAILED (hr))
		return false;

	capacity = sizeInBytes;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* D3DBuffer::map ()
{
	ID3D11DeviceContext* context = DXGIEngine::instance ().getDirect3dDeviceContext ();

	D3D11_MAPPED_SUBRESOURCE mappedSubresource {};
	HRESULT hr = context->Map (buffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedSubresource);
	if(FAILED (hr))
		return nullptr;

	return mappedSubresource.pData;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D3DBuffer::unmap ()
{
	ID3D11DeviceContext* context = DXGIEngine::instance ().getDirect3dDeviceContext ();
	context->Unmap (buffer, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D3DBuffer::ensureSegmentAlignment (uint32& byteOffset, uint32& size, uint32 stride) const
{
	if(stride == 0)
		return false;

	if(type == kConstantBuffer)
	{
		// Direct3D has some special requirements regarding the number and alignment of shader constants
		// within a constant buffer (See docs of ID3D11DeviceContext1::PSSetConstantBuffers1).

		uint32 elementCount = (size + stride - 1) / stride;

		byteOffset = ccl_align_to (byteOffset, kConstantByteAlignment);
		stride = ccl_align_to (stride, kConstantByteAlignment);

		size = elementCount * stride;
	}
	else if(type == kVertexBuffer)
	{
		byteOffset = ccl_align_to (byteOffset, stride);
	}
	return true;
}

//************************************************************************************************
// D3DTexture2D
//************************************************************************************************

DEFINE_CLASS_HIDDEN (D3DTexture2D, Native3DTexture2D)

//////////////////////////////////////////////////////////////////////////////////////////////////

D3DTexture2D::D3DTexture2D ()
: immutable (false),
  addressMode (D3D11_TEXTURE_ADDRESS_BORDER),
  mipLevels (1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D3DTexture2D::create (uint32 width, uint32 height, uint32 bytesPerRow, DataFormat3D format, TextureFlags3D flags, const void* initialData)
{
	mipLevels = 1;
	if(get_flag<TextureFlags3D> (flags, kTextureMipmapEnabled))
		mipLevels = getMipLevels (width, height);

	D3D11_TEXTURE2D_DESC desc {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = mipLevels;
	desc.ArraySize = 1;
	desc.Format = getDxgiFormat (format);
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = getD3D11Usage (kBufferUsageDefault);
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	ID3D11Device* device = DXGIEngine::instance ().getDirect3dDevice ();
	ID3D11DeviceContext1* context = DXGIEngine::instance ().getDirect3dDeviceContext ();
	HRESULT hr = S_OK;

	if(initialData == nullptr)
		hr = device->CreateTexture2D (&desc, nullptr, &texture);
	else
	{
		Vector<D3D11_SUBRESOURCE_DATA> subresourceData (mipLevels);
		subresourceData.setCount (mipLevels);
		subresourceData.fill ({initialData, bytesPerRow, 0});
		hr = device->CreateTexture2D (&desc, subresourceData, &texture);
	}

	if(SUCCEEDED (hr))
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC resourceViewDescription {};
		resourceViewDescription.Format = desc.Format;
		resourceViewDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		resourceViewDescription.Texture2D.MipLevels = mipLevels;
		resourceViewDescription.Texture2D.MostDetailedMip = 0;
		hr = device->CreateShaderResourceView (texture, &resourceViewDescription, &resourceView);
	}

	if(SUCCEEDED (hr))
		context->GenerateMips (resourceView);

	addressMode = D3D11_TEXTURE_ADDRESS_CLAMP;
	if(get_flag<TextureFlags3D> (flags, kTextureClampToBorder))
		addressMode = D3D11_TEXTURE_ADDRESS_BORDER;
	else if(get_flag<TextureFlags3D> (flags, kTextureRepeat))
		addressMode = D3D11_TEXTURE_ADDRESS_WRAP;
	else if(get_flag<TextureFlags3D> (flags, kTextureMirror))
		addressMode = D3D11_TEXTURE_ADDRESS_MIRROR;

	return SUCCEEDED (hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D3DTexture2D::copyFromBitmap (IBitmap* bitmap)
{
	IMultiResolutionBitmap::RepSelector selector (UnknownPtr<IMultiResolutionBitmap> (bitmap), getHighestResolutionIndex (bitmap));
	BitmapDataLocker locker (bitmap, IBitmap::kRGBAlpha, IBitmap::kLockRead);
	if(locker.result != kResultOk)
		return locker.result;

	ID3D11DeviceContext1* context = DXGIEngine::instance ().getDirect3dDeviceContext ();

	context->UpdateSubresource (texture, 0, nullptr, locker.data.scan0, locker.data.rowBytes, 0);
	if(mipLevels > 1)
		context->GenerateMips (resourceView);
	
	return kResultOk;
}

//************************************************************************************************
// D3DShader
//************************************************************************************************

const FileType D3DShader::kHlslFileType ("High Level Shading Language File", "hlsl");
const FileType D3DShader::kCsoFileType ("Compiled Shader Object", "cso");

DEFINE_CLASS_HIDDEN (D3DShader, Native3DGraphicsShader)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D3DShader::create (GraphicsShader3DType type, UrlRef path)
{
	ASSERT (this->blob == nullptr)

	this->path = path;
	this->type = type;

	if(!load ())
		return false;

	ID3D11Device* device = DXGIEngine::instance ().getDirect3dDevice ();
	HRESULT hr = S_OK;
	ID3D11DeviceChild* d3dShader = nullptr;

	switch(type)
	{
	case kVertexShader:
		hr = device->CreateVertexShader (blob->getAddress (), blob->getSize (), nullptr, reinterpret_cast<ID3D11VertexShader**> (&d3dShader));
		break;

	case kPixelShader:
		hr = device->CreatePixelShader (blob->getAddress (), blob->getSize (), nullptr, reinterpret_cast<ID3D11PixelShader**> (&d3dShader));
		break;

	default:
		return false;
	}

	if(FAILED (hr))
	{
		CCL_WARN ("Failed to load shader %s: %d\n", MutableCString (UrlDisplayString (path)).str (), hr)
		return false;
	}

	this->blob = blob;
	shader.assign (d3dShader);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D3DShader::load ()
{
	const FileType& type = path.getFileType ();

	if(type == kHlslFileType)
	{
		AutoPtr<IMemoryStream> stream = File::loadBinaryFile (path);
		if(stream == nullptr)
			return false;

		return compile (stream->getMemoryAddress (), stream->getBytesWritten ());
	}
	else if(type == kCsoFileType)
	{
		AutoPtr<IMemoryStream> stream = File::loadBinaryFile (path);
		if(stream == nullptr)
			return false;

		blob = NEW Buffer (stream->getMemoryAddress (), stream->getBytesWritten ());
		return true;
	}
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D3DShader::compile (const void* buffer, uint32 sizeOfBuffer)
{
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef DEBUG
	flags |= D3DCOMPILE_DEBUG;
#endif

	CStringPtr entryPoint = "main";
	CStringPtr target = nullptr;

	switch(type)
	{
	case kVertexShader:
		target = kDefaultVertexShaderTarget;
		break;

	case kPixelShader:
		target = kDefaultPixelShaderTarget;
		break;

	default:
		return false;
	}

	const D3D_SHADER_MACRO defines[1] =
	{
		// currently unused
		{ nullptr, nullptr }
	};

	ComPtr<ID3DBlob> shaderBlob;
	ComPtr<ID3DBlob> errorBlob;

	HRESULT hr = ::D3DCompile2 (
		buffer, sizeOfBuffer, nullptr, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint, target, flags, 0, 0, nullptr, 0, &shaderBlob, &errorBlob);

	if(SUCCEEDED (hr))
	{
		blob = NEW Buffer (shaderBlob->GetBufferPointer (), shaderBlob->GetBufferSize ());
		return true;
	}
	else
	{
		if(errorBlob)
		{
			CStringPtr errorMessage = reinterpret_cast<const char*> (errorBlob->GetBufferPointer ());
			CCL_WARN (errorMessage, 0)
		}

		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID3D11ShaderReflection* D3DShader::getReflection ()
{
	if(!shaderReflection && blob)
	{
		HRESULT hr = ::D3DReflect (blob->getAddress (), blob->getSize (), IID_ID3D11ShaderReflection, shaderReflection);
		ASSERT (SUCCEEDED (hr))
		if(FAILED (hr))
		{
			CCL_WARN ("D3D shader reflection not available: %d\n", hr)
		}
	}
	return shaderReflection;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::ITypeInfo* CCL_API D3DShader::getBufferTypeInfo (int bufferIndex)
{
	if(bufferTypeInfos.isEmpty ())
	{
		if(getReflection () == nullptr)
			return nullptr;

		D3D11_SHADER_DESC shaderDescription {};
		HRESULT hr = shaderReflection->GetDesc (&shaderDescription);
		if(FAILED (hr))
		{
			CCL_WARN ("D3D shader description not available: %d\n", hr)
			return nullptr;
		}

		for(int bufferIndex = 0; bufferIndex < shaderDescription.ConstantBuffers; bufferIndex++)
		{
			ID3D11ShaderReflectionConstantBuffer* constantBuffer = shaderReflection->GetConstantBufferByIndex (bufferIndex);
			if(constantBuffer == nullptr)
				continue;

			D3D11_SHADER_BUFFER_DESC bufferDesc {};
			HRESULT hr = constantBuffer->GetDesc (&bufferDesc);
			if(FAILED (hr))
			{
				CCL_WARN ("D3D shader buffer description not available at index %d: %d\n", bufferIndex, hr)
				continue;
			}

			D3D11_SHADER_INPUT_BIND_DESC bindDescription {};
			hr = shaderReflection->GetResourceBindingDescByName (bufferDesc.Name, &bindDescription);
			if(FAILED (hr))
			{
				CCL_WARN ("D3D resource binding description not available for %s: %d\n", bufferDesc.Name, hr)
				continue;
			}

			auto bufferTypeInfo = NEW ShaderTypeInfo3D;
			ASSERT (bufferDesc.Size > 0)
			if(bufferDesc.Size == 0)
			{
				CCL_WARN ("Invalid shader buffer struct size\n", 0)
			}
			bufferTypeInfo->setStructSize (bufferDesc.Size);
			bufferTypeInfo->setStructName (bufferDesc.Name);
			bufferTypeInfo->setBindingIndex (bindDescription.BindPoint);

			bufferTypeInfos.addSorted (bufferTypeInfo);

			for(UINT i = 0; i < bufferDesc.Variables; i++)
			{
				ID3D11ShaderReflectionVariable* variable = constantBuffer->GetVariableByIndex (i);
				if(variable == nullptr)
					continue;

				D3D11_SHADER_VARIABLE_DESC variableDesc {};
				hr = variable->GetDesc (&variableDesc);

				const auto addTypeInfoRecursive = [&] (ID3D11ShaderReflectionType* type) -> void
				{
					auto addTypeInfo = [&] (ID3D11ShaderReflectionType* type, CStringRef name, ShaderTypeInfo3D* typeInfo, ShaderVariable3D* parent, auto& recurse) mutable -> void
					{
						D3D11_SHADER_TYPE_DESC typeDesc {};
						if(type)
							hr = type->GetDesc (&typeDesc);

						if(name)
						{
							UINT elementSize = typeDesc.Elements > 0 ? variableDesc.Size / typeDesc.Elements : variableDesc.Size;
							elementSize = ccl_align_to (elementSize, D3DBuffer::kConstantSize);

							AutoPtr<ShaderVariable3D> v = NEW ShaderVariable3D;
							v->setName (name);
							v->setOffset (typeDesc.Offset + (parent ? parent->getOffset () : variableDesc.StartOffset));
							v->setSize (variableDesc.Size);
							CString typeName (typeDesc.Name);
							if(typeDesc.Class == D3D_SVC_STRUCT)
							{
								v->setType (ShaderVariable3D::kStruct);
								AutoPtr<ShaderTypeInfo3D> structTypeInfo = NEW ShaderTypeInfo3D;
								v->setStructType (structTypeInfo);
								for(UINT memberIndex = 0; memberIndex < typeDesc.Members; memberIndex++)
								{
									ID3D11ShaderReflectionType* memberType = type->GetMemberTypeByIndex (memberIndex);
									CString memberName (type->GetMemberTypeName (memberIndex));
									recurse (memberType, memberName, structTypeInfo, v, recurse);
								}
							}
							else if(typeName == "float")
							{
								v->setType (ShaderVariable3D::kFloat);
								v->setSize (sizeof(float));
							}
							else if(typeName == "float4")
							{
								v->setType (ShaderVariable3D::kFloat4);
								v->setSize (sizeof(float) * 4);
							}
							else if(typeName == "float4x4")
							{
								v->setType (ShaderVariable3D::kFloat4x4);
								v->setSize (sizeof(float) * 4 * 4);
							}
							else if(typeName == "int")
							{
								v->setType (ShaderVariable3D::kInt);
								v->setSize (sizeof(int32));
							}
							else
								v->setType (ShaderVariable3D::kUnknown);

							v->setArrayElementCount (typeDesc.Elements);
							v->setArrayElementStride (elementSize);

							if(typeInfo)
								typeInfo->addVariable (v.detach ());
						}
					};
					addTypeInfo (type, variableDesc.Name, bufferTypeInfo, nullptr, addTypeInfo);
				};

				ID3D11ShaderReflectionType* type = variable->GetType ();
				addTypeInfoRecursive (type);
			}
		}
	}
	return SuperClass::getBufferTypeInfo (bufferIndex);
}

//************************************************************************************************
// D3DResourceManager
//************************************************************************************************

DEFINE_CLASS_HIDDEN (D3DResourceManager, Native3DResourceManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

D3DResourceManager::D3DResourceManager ()
{
	samplers.setCount (kNumAddressModes);
	for(int i = 0; i < samplers.count (); i++)
	{
		samplers[i].setCount (Native3DShaderParameterSet::kMaxTextureCount);
		samplers[i].zeroFill ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D3DResourceManager::shutdown ()
{
	for(int i = 0; i < samplers.count (); i++)
	{
		for(int j = 0; j < samplers[i].count (); j++)
			samplers[i][j].release ();
		samplers[i].zeroFill ();
	}
	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID3D11SamplerState* D3DResourceManager::getSampler (D3D11_TEXTURE_ADDRESS_MODE addressMode, int textureIndex) const
{
	ASSERT (addressMode < samplers.count ())
	if(addressMode >= samplers.count ())
		return nullptr;

	if(textureIndex < samplers[addressMode].count () && !samplers[addressMode][textureIndex].isValid ())
	{
		D3D11_SAMPLER_DESC samplerDescription {};
		samplerDescription.Filter = D3D11_FILTER_ANISOTROPIC;
		samplerDescription.AddressU = addressMode;
		samplerDescription.AddressV = addressMode;
		samplerDescription.AddressW = addressMode;
		samplerDescription.MaxAnisotropy = 16;
		samplerDescription.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDescription.BorderColor[0] = 0.f;
		samplerDescription.BorderColor[1] = 0.f;
		samplerDescription.BorderColor[2] = 0.f;
		samplerDescription.BorderColor[3] = 0.f;
		samplerDescription.MinLOD = 0;
		samplerDescription.MaxLOD = D3D11_FLOAT32_MAX;

		DXGIEngine::instance ().getDirect3dDevice ()->CreateSamplerState (&samplerDescription, &samplers[addressMode][textureIndex]);
	}

	return samplers[addressMode].at (textureIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DGraphicsShader* D3DResourceManager::loadShader (UrlRef _path, GraphicsShader3DType type)
{
	AutoPtr<D3DShader> shader = NEW D3DShader;
	Url path (_path);
	path.setFileType (D3DShader::kCsoFileType);
	if(!shader->create (type, path))
		return nullptr;

	return shader.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DTexture2D* D3DResourceManager::loadTexture (Bitmap* bitmap, TextureFlags3D flags)
{
	BitmapDataLocker locker (bitmap, IBitmap::kRGBAlpha, IBitmap::kLockRead);
	if(locker.result != kResultOk)
		return nullptr;

	AutoPtr<D3DTexture2D> texture = NEW D3DTexture2D;
	
	uint32 sizeInBytes = locker.data.rowBytes * locker.data.height;
	if(texture->create (bitmap->getPixelSize ().x, bitmap->getPixelSize ().y, locker.data.rowBytes, kB8G8R8A8_UNORM, flags, locker.data.scan0))
		return texture.detach ();

	return nullptr;
}

//************************************************************************************************
// D3DPipeline
//************************************************************************************************

DEFINE_CLASS_HIDDEN (D3DPipeline, Native3DGraphicsPipeline)

//////////////////////////////////////////////////////////////////////////////////////////////////

D3DPipeline::D3DPipeline ()
: d3dTopology (D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED),
  fillMode (kFillModeSolid),
  depthBias (0)
{
	setFillMode (kFillModeSolid);
	depthStencilState = D3DGraphicsFactory::getD3DInstance ().createDepthStencilState (true, true);
	updateRasterizerState ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D3DPipeline::setPrimitiveTopology (PrimitiveTopology3D topology)
{
	switch(topology)
	{
	case kPrimitiveTopologyTriangleList:
		d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;

	case kPrimitiveTopologyTriangleStrip:
		d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		break;

	default:
		return kResultInvalidArgument;
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D3DPipeline::setFillMode (FillMode3D mode)
{
	if(mode != fillMode)
	{
		fillMode = mode;
		updateRasterizerState ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D3DPipeline::setVertexFormat (IVertexFormat3D* _format)
{
	D3DVertexFormat* format = unknown_cast<D3DVertexFormat> (_format);
	if(format == nullptr)
		return kResultInvalidArgument;

	vertexFormat = format;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D3DPipeline::setVertexShader (IGraphicsShader3D* _shader)
{
	D3DShader* shader = unknown_cast<D3DShader> (_shader);
	if(shader == nullptr || shader->getType () != IGraphicsShader3D::kVertexShader)
		return kResultInvalidArgument;

	d3dVertexShader.fromUnknown (shader->getShader ());
	if(!d3dVertexShader.isValid ())
		return kResultInvalidArgument;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D3DPipeline::setPixelShader (IGraphicsShader3D* _shader)
{
	D3DShader* shader = unknown_cast<D3DShader> (_shader);
	if(shader == nullptr || shader->getType () != IGraphicsShader3D::kPixelShader)
		return kResultInvalidArgument;

	d3dPixelShader.fromUnknown (shader->getShader ());
	if(!d3dPixelShader.isValid ())
		return kResultInvalidArgument;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D3DPipeline::setDepthTestParameters (const DepthTestParameters3D& parameters)
{
	if(parameters.bias != depthBias)
	{
		depthBias = ccl_to_int (parameters.bias);
		updateRasterizerState ();
	}
	depthStencilState = D3DGraphicsFactory::getD3DInstance ().createDepthStencilState (parameters.testEnabled, parameters.writeEnabled);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D3DPipeline::updateRasterizerState ()
{
	if(depthBias == 0)
		rasterizerState.share (D3DGraphicsFactory::getD3DInstance ().getRasterizerStateForMode (fillMode));
	else
		rasterizerState = D3DGraphicsFactory::getD3DInstance ().createRasterizerState (fillMode, depthBias);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D3DPipeline::applyTo (ID3D11DeviceContext* deviceContext) const
{
	deviceContext->IASetPrimitiveTopology (d3dTopology);

	deviceContext->OMSetDepthStencilState (depthStencilState, 1);

	ID3D11InputLayout* inputLayout = vertexFormat ? vertexFormat->getInputLayout () : nullptr;
	deviceContext->IASetInputLayout (inputLayout);

	deviceContext->VSSetShader (d3dVertexShader, nullptr, 0);
	deviceContext->PSSetShader (d3dPixelShader, nullptr, 0);

	deviceContext->RSSetState (rasterizerState);
}

//************************************************************************************************
// D3DGraphicsFactory
//************************************************************************************************

DEFINE_CLASS_HIDDEN (D3DGraphicsFactory, Native3DGraphicsFactory)
DEFINE_EXTERNAL_SINGLETON (Native3DGraphicsFactory, D3DGraphicsFactory)

//////////////////////////////////////////////////////////////////////////////////////////////////

D3DGraphicsFactory& D3DGraphicsFactory::getD3DInstance ()
{
	return static_cast<D3DGraphicsFactory&> (instance ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID3D11RasterizerState* D3DGraphicsFactory::createRasterizerState (FillMode3D mode, int depthBias)
{
	D3D11_RASTERIZER_DESC rsDesc {};
	rsDesc.FillMode = mode == kFillModeWireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_BACK;
	rsDesc.FrontCounterClockwise = TRUE;
	rsDesc.DepthBias = 0;
	rsDesc.DepthBiasClamp = 0;
	rsDesc.SlopeScaledDepthBias = 0;
	rsDesc.DepthClipEnable = TRUE;
	rsDesc.ScissorEnable = FALSE;
	rsDesc.MultisampleEnable = TRUE;
	rsDesc.AntialiasedLineEnable = FALSE;

	ComPtr<ID3D11RasterizerState> rasterizerState;
	ID3D11Device* device = DXGIEngine::instance ().getDirect3dDevice ();
	HRESULT hr = device->CreateRasterizerState (&rsDesc, &rasterizerState);
	ASSERT (SUCCEEDED (hr))
	return rasterizerState.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID3D11RasterizerState* D3DGraphicsFactory::getRasterizerStateForMode (FillMode3D mode)
{
	if(mode == kFillModeSolid)
	{
		if(!rasterizerStateSolid)
			rasterizerStateSolid = createRasterizerState (kFillModeSolid, 0);
		return rasterizerStateSolid;
	}
	else if(mode == kFillModeWireframe)
	{
		if(!rasterizerStateWireframe)
			rasterizerStateWireframe = createRasterizerState (kFillModeWireframe, 0);
		return rasterizerStateWireframe;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID3D11BlendState* D3DGraphicsFactory::getBlendState ()
{
	if(!blendState.isValid ())
	{
		D3D11_BLEND_DESC blendDescription {};
		blendDescription.AlphaToCoverageEnable = FALSE;
		blendDescription.IndependentBlendEnable = FALSE;

		blendDescription.RenderTarget[0].BlendEnable = TRUE;
		blendDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDescription.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		blendDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDescription.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		ID3D11Device* device = DXGIEngine::instance ().getDirect3dDevice ();
		HRESULT hr = device->CreateBlendState (&blendDescription, &blendState);
		ASSERT (SUCCEEDED (hr))
	}

	return blendState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID3D11DepthStencilState* D3DGraphicsFactory::createDepthStencilState (bool depthTestEnabled, bool depthWriteEnabled)
{
	D3D11_DEPTH_STENCIL_DESC dsDesc {};
	dsDesc.DepthEnable = depthTestEnabled;
	dsDesc.DepthWriteMask = depthWriteEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	dsDesc.StencilEnable = false;

	ComPtr<ID3D11DepthStencilState> depthStencilState;
	ID3D11Device* device = DXGIEngine::instance ().getDirect3dDevice ();
	HRESULT hr = device->CreateDepthStencilState (&dsDesc, &depthStencilState);
	ASSERT (SUCCEEDED (hr))
	return depthStencilState.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D3DGraphicsFactory::discardCachedResources ()
{
	rasterizerStateSolid.release ();
	rasterizerStateWireframe.release ();
	blendState.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IVertexFormat3D* CCL_API D3DGraphicsFactory::createVertexFormat (const VertexElementDescription description[], uint32 count, const IGraphicsShader3D* shader)
{
	AutoPtr<D3DVertexFormat> buffer = NEW D3DVertexFormat;
	if(!buffer->create (description, count, shader))
		return nullptr;

	return buffer.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsBuffer3D* CCL_API D3DGraphicsFactory::createBuffer (GraphicsBuffer3DType type, BufferUsage3D usage, 
								uint32 sizeInBytes, uint32 strideInBytes, const void* initialData /* = nullptr */)
{
	AutoPtr<D3DBuffer> buffer = NEW D3DBuffer;
	if(!buffer->create (type, usage, sizeInBytes, strideInBytes, initialData))
		return nullptr;

	return buffer.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsTexture2D* CCL_API D3DGraphicsFactory::createTexture (IBitmap* _bitmap, TextureFlags3D flags)
{
	auto* bitmap = unknown_cast<Bitmap> (_bitmap);
	if(bitmap == nullptr)
		return nullptr;

	D3DResourceManager& manager = D3DResourceManager::instance ();
	return return_shared (manager.getTexture (bitmap, flags));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsShader3D* CCL_API D3DGraphicsFactory::createShader (GraphicsShader3DType type, UrlRef path)
{
	D3DResourceManager& manager = D3DResourceManager::instance ();
	return return_shared (manager.getShader (path, type));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsShader3D* CCL_API D3DGraphicsFactory::createStockShader (GraphicsShader3DType type, StringID name)
{
	ResourceUrl url {String (name)};
	url.setFileType (D3DShader::kCsoFileType, false);

	D3DResourceManager& manager = D3DResourceManager::instance ();
	return return_shared (manager.getShader (url, type));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsPipeline3D* CCL_API D3DGraphicsFactory::createPipeline ()
{
	return NEW D3DPipeline;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IShaderParameterSet3D* CCL_API D3DGraphicsFactory::createShaderParameterSet ()
{
	return NEW Native3DShaderParameterSet;
}

//************************************************************************************************
// D3DGraphicsContext
//************************************************************************************************

DEFINE_CLASS_HIDDEN (D3DGraphicsContext, Native3DGraphicsDevice)

//////////////////////////////////////////////////////////////////////////////////////////////////

D3DGraphicsContext::D3DGraphicsContext (const D3DSurface& surface)
: surface (surface),
  deviceContext (DXGIEngine::instance ().getDirect3dDeviceContext ()),
  renderTargetView (surface.getRenderTargetView ()),
  depthStencilView (surface.getDepthStencilView ()),
  oldBlendFactors {0.f, 0.f, 0.f, 0.f},
  oldSampleMask (0)
{
	ASSERT (renderTargetView && deviceContext)

	deviceContext->OMSetRenderTargets (1, &renderTargetView, depthStencilView);

	deviceContext->OMGetBlendState (&oldBlendState, oldBlendFactors, &oldSampleMask);
	deviceContext->OMSetBlendState (D3DGraphicsFactory::getD3DInstance ().getBlendState (), oldBlendFactors, oldSampleMask);

	D3D11_VIEWPORT viewport {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = surface.getViewPortRect ().getWidth ();
	viewport.Height = surface.getViewPortRect ().getHeight ();
	viewport.MinDepth = D3D11_MIN_DEPTH;
	viewport.MaxDepth = D3D11_MAX_DEPTH;

	deviceContext->RSSetViewports (1, &viewport);

	if(surface.hasClearColor ())
	{
		ColorF clearColor (surface.getClearColor ());
		deviceContext->ClearRenderTargetView (renderTargetView, clearColor.values);
	}
	deviceContext->ClearDepthStencilView (depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

D3DGraphicsContext::~D3DGraphicsContext ()
{
	ID3D11Resource* resolveTexture = surface.getResolveTexture ();
	ID3D11Resource * offscreenTexture = surface.getOffscreenTexture ();
	if(resolveTexture && offscreenTexture)
	{
		deviceContext->ResolveSubresource (resolveTexture, 0, offscreenTexture, 0, DXGI_FORMAT_B8G8R8A8_UNORM);
	}

	deviceContext->OMSetBlendState (oldBlendState, oldBlendFactors, oldSampleMask);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D3DGraphicsContext::setPipeline (IGraphicsPipeline3D* _pipeline)
{
	D3DPipeline* pipeline = unknown_cast<D3DPipeline> (_pipeline);
	if(pipeline == nullptr)
		return kResultInvalidArgument;

	pipeline->applyTo (deviceContext);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D3DGraphicsContext::setVertexBuffer (IGraphicsBuffer3D* _buffer, uint32 stride)
{
	D3DBuffer* buffer = unknown_cast<D3DBuffer> (_buffer);
	if(buffer == nullptr || buffer->getType () != IGraphicsBuffer3D::kVertexBuffer)
		return kResultInvalidArgument;

	ID3D11Buffer* const d3dBuffers[1] = {buffer->getBuffer ()};
	CCL_PRINTF ("IASetVertexBuffers (0, 1, %p, %u)\n", d3dBuffers[0], stride)
	uint32 offset = 0;
	deviceContext->IASetVertexBuffers (0, 1, d3dBuffers, &stride, &offset);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D3DGraphicsContext::setIndexBuffer (IGraphicsBuffer3D* _buffer, DataFormat3D _format)
{
	D3DBuffer* buffer = unknown_cast<D3DBuffer> (_buffer);
	if(buffer == nullptr || buffer->getType () != IGraphicsBuffer3D::kIndexBuffer)
		return kResultInvalidArgument;

	DXGI_FORMAT format = getDxgiFormat (_format);
	if(format == DXGI_FORMAT_UNKNOWN)
		return kResultInvalidArgument;

	CCL_PRINTF ("IASetIndexBuffer (%p, %u)\n", buffer->getBuffer (), format)
	deviceContext->IASetIndexBuffer (buffer->getBuffer (), format, 0);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D3DGraphicsContext::setShaderParameters (IShaderParameterSet3D* _parameters)
{
	auto* parameterSet = unknown_cast<Native3DShaderParameterSet> (_parameters);
	if(parameterSet == nullptr)
		return kResultInvalidArgument;

	const Vector<Native3DShaderParameters>& vsParameters = parameterSet->getVertexShaderParameters ();
	for(const Native3DShaderParameters& parameters : vsParameters)
	{
		if(parameters.segment == nullptr)
			continue;
		
		if(auto* parameterBuffer = unknown_cast<D3DBuffer> (parameters.segment->getBuffer ()))
		{
			ID3D11Buffer* vsBuffer = parameterBuffer->getBuffer ();
			UINT offset = parameters.segment->getOffset () / D3DBuffer::kConstantSize;
			UINT size = parameters.segment->getSize () / D3DBuffer::kConstantSize;
			deviceContext->VSSetConstantBuffers1 (parameters.bufferIndex, 1, &vsBuffer, &offset, &size);
		}
	}

	const Vector<Native3DShaderParameters>& psParameters = parameterSet->getPixelShaderParameters ();
	for(const Native3DShaderParameters& parameters : psParameters)
	{
		if(parameters.segment == nullptr)
			continue;
		
		if(auto* parameterBuffer = unknown_cast<D3DBuffer> (parameters.segment->getBuffer ()))
		{
			ID3D11Buffer* psBuffer = parameterBuffer->getBuffer ();
			UINT offset = parameters.segment->getOffset () / D3DBuffer::kConstantSize;
			UINT size = parameters.segment->getSize () / D3DBuffer::kConstantSize;
			deviceContext->PSSetConstantBuffers1 (parameters.bufferIndex, 1, &psBuffer, &offset, &size);
		}
	}

	for(int i = 0; i < Native3DShaderParameterSet::kMaxTextureCount; i++)
	{
		auto* texture = unknown_cast<D3DTexture2D> (parameterSet->getTexture (i));
		if(texture)
		{
			ID3D11SamplerState* sampler = D3DResourceManager::instance ().getSampler (texture->getAddressMode (), i);
			deviceContext->PSSetSamplers (i, 1, &sampler);
			ID3D11ShaderResourceView* resourceView = texture->getResourceView ();
			deviceContext->PSSetShaderResources (i, 1, &resourceView);
		}
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D3DGraphicsContext::draw (uint32 startVertex, uint32 vertexCount)
{
	deviceContext->Draw (vertexCount, startVertex);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D3DGraphicsContext::drawIndexed (uint32 startIndex, uint32 indexCount, int32 baseVertex)
{
	CCL_PRINTF ("DrawIndexed (%u, %u, %u)\n", startIndex, indexCount, baseVertex)
	deviceContext->DrawIndexed (indexCount, startIndex, baseVertex);
	return kResultOk;
}
