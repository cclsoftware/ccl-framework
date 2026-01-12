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
// Filename    : ccl/platform/cocoa/metal/metal3dsupport.mm
// Description : Metal 3D Support
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/metal/metal3dsupport.h"

#include "ccl/platform/cocoa/metal/metalclient.h"
#include "ccl/platform/cocoa/skia/skiaengine.cocoa.h"
#include "ccl/platform/cocoa/skia/skiarendertarget.cocoa.h"
#include "ccl/platform/cocoa/macutils.h"

#include "ccl/base/storage/file.h"

#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/3d/shader/shaderreflection3d.h"

#include "ccl/public/gui/graphics/3d/vertex3d.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/math/mathprimitives.h"

#include "ccl/public/base/ccldefpush.h"
#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>
#include <QuartzCore/CAMetalLayer.h>

using namespace CCL;

#define kMetalBufferIndexMax 30
#define kMetalVertexBufferIndex kMetalBufferIndexMax

//************************************************************************************************
// MTKView
//************************************************************************************************

@interface CCL_ISOLATED (MTKView): MTKView
{
	Metal3DSurface* surface;
}

- (instancetype)initWithSurface:(Metal3DSurface*)surface;
- (void)drawRect:(CGRect)dirtyRect;
- (void)viewDidChangeBackingProperties;
#if CCL_PLATFORM_MAC
- (BOOL)acceptsFirstMouse:(NSEvent*)nsEvent;
#endif

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (MTKView)

- (instancetype)initWithSurface:(Metal3DSurface*)_surface
{
	if(self = [super init])
		surface = _surface;

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)drawRect:(CGRect)dirtyRect
{
	if(surface == nullptr || surface->isDirty () == false)
		return;
	
	if(IGraphicsContent3D* content = surface->getContent ())
	{
		Metal3DGraphicsContext context (surface->getView ());
		content->renderContent (context);
	}
	surface->setDirty (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)viewDidChangeBackingProperties
{
	surface->updateSampleCount ();
	surface->setDirty (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if CCL_PLATFORM_MAC
- (BOOL)acceptsFirstMouse:(NSEvent*)nsEvent
{
	return YES;
}
#endif

@end

//************************************************************************************************
// MetalFormatMap
//************************************************************************************************

struct MetalFormatMap
{
	DataFormat3D format;
	MTLVertexFormat vertexFormat;
	MTLPixelFormat pixelFormat;
	int size;
};

static constexpr MetalFormatMap kMetalFormatMap[] =
{
	{ kR8_Int,             MTLVertexFormatChar,		MTLPixelFormatR8Sint,			1  },
	{ kR8_UInt,            MTLVertexFormatUChar,	MTLPixelFormatR8Uint,			1  },
	{ kR16_Int,            MTLVertexFormatShort,	MTLPixelFormatR16Sint,			2  },
	{ kR16_UInt,           MTLVertexFormatUShort,	MTLPixelFormatR16Uint,			2  },
	{ kR32_Int,            MTLVertexFormatInt, 		MTLPixelFormatR32Sint,			4  },
	{ kR32_UInt,           MTLVertexFormatUInt,   	MTLPixelFormatR32Uint,			4  },
	{ kR32_Float,          MTLVertexFormatFloat,	MTLPixelFormatR32Float,			4  },
	{ kR8G8_Int,           MTLVertexFormatChar2, 	MTLPixelFormatRG8Sint,			2  },
	{ kR8G8_UInt,          MTLVertexFormatUChar2,   MTLPixelFormatRG8Uint,			2  },
	{ kR16G16_Int,         MTLVertexFormatShort2,   MTLPixelFormatR32Sint,			4  },
	{ kR16G16_UInt,        MTLVertexFormatUShort2,  MTLPixelFormatR32Uint,			4  },
	{ kR32G32_Int,         MTLVertexFormatInt2,     MTLPixelFormatRG32Sint,			8  },
	{ kR32G32_UInt,        MTLVertexFormatUInt2,    MTLPixelFormatRG32Uint,			8  },
	{ kR32G32_Float,       MTLVertexFormatFloat2,   MTLPixelFormatRG32Float,		8  },
	{ kR32G32B32_Int,      MTLVertexFormatInt3,     MTLPixelFormatInvalid,			12 },
	{ kR32G32B32_UInt,     MTLVertexFormatUInt3,    MTLPixelFormatInvalid,			12 },
	{ kR32G32B32_Float,    MTLVertexFormatFloat3,   MTLPixelFormatInvalid,			12 },
	{ kR32G32B32A32_Int,   MTLVertexFormatInt4,     MTLPixelFormatRGBA32Sint,		16 },
	{ kR32G32B32A32_UInt,  MTLVertexFormatUInt4,    MTLPixelFormatRGBA32Uint,		16 },
	{ kR32G32B32A32_Float, MTLVertexFormatFloat4,   MTLPixelFormatRGBA32Float,		16 },
	{ kR8G8B8A8_UNORM,	   MTLVertexFormatChar4,    MTLPixelFormatRGBA8Unorm,		4  },
	{ kB8G8R8A8_UNORM,     MTLVertexFormatChar4,	MTLPixelFormatBGRA8Unorm,		4  }
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static constexpr MTLVertexFormat getMetalVertexFormat (DataFormat3D format)
{
	for(const auto& entry : kMetalFormatMap)
		if(entry.format == format)
			return entry.vertexFormat;

	return MTLVertexFormatInvalid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static constexpr MTLPixelFormat getMetalPixelFormat (DataFormat3D format)
{
	for(const auto& entry : kMetalFormatMap)
		if(entry.format == format)
			return entry.pixelFormat;

	return MTLPixelFormatInvalid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static constexpr int getMetalFormatSize (DataFormat3D format)
{
	for(const auto& entry : kMetalFormatMap)
		if(entry.format == format)
			return entry.size;

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static constexpr MTLResourceOptions getResourceOptions (BufferUsage3D usage)
{
	MTLResourceOptions resourceOptions = MTLResourceStorageModeShared;
	switch(usage)
	{
	case kBufferUsageDefault :
	case kBufferUsageDynamic :
	case kBufferUsageStaging :
		resourceOptions = MTLResourceStorageModeShared;
		break;
	case kBufferUsageImmutable :
		resourceOptions = MTLResourceStorageModePrivate;
		break;
	}
		
	return resourceOptions;
}

//************************************************************************************************
// Metal3DSurface
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Metal3DSurface, Native3DSurface)

//////////////////////////////////////////////////////////////////////////////////////////////////

Metal3DSurface::Metal3DSurface ()
: firstDraw (true)
{
	view = [[CCL_ISOLATED (MTKView) alloc] initWithSurface:this];
	[view setDevice:MetalClient::instance ().getDevice ()];

	if(MacOS::MetalGraphicsInfo::instance ().isSkiaEnabled ())
	{
		// sync to the explicitly timed drawing of MetalUpdater used with Skia
		[view setEnableSetNeedsDisplay:NO];
		[view setPaused:YES];
		MetalUpdater::instance ().addSurface (this);
	}

	[view setClearColor:MTLClearColorMake (0., 0., 0., 0.)];
	[view setDepthStencilPixelFormat:MTLPixelFormatDepth32Float];
	[view setClearDepth:1.0];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Metal3DSurface::~Metal3DSurface ()
{
	if(MacOS::MetalGraphicsInfo::instance ().isSkiaEnabled ())
		MetalUpdater::instance ().removeSurface (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Metal3DSurface::setContent (IGraphicsContent3D* _content)
{
	SuperClass::setContent (_content);
	if(content)
	{
		updateSampleCount ();
		GraphicsContentHint hint =  content->getContentHint ();
		if(hint == kGraphicsContentTranslucent)
			[[view layer] setOpaque:NO];
		else
			[[view layer] setOpaque:YES];
		Color backColor = content->getBackColor ();
		#if CCL_PLATFORM_IOS
		[[view layer] setBackgroundColor:[[UIColor colorWithRed:backColor.getRedF () green:backColor.getGreenF () blue:backColor.getBlueF () alpha:backColor.getAlphaF ()] CGColor]];
		#else
		[[view layer] setBackgroundColor:[[NSColor colorWithRed:backColor.getRedF () green:backColor.getGreenF () blue:backColor.getBlueF () alpha:backColor.getAlphaF ()] CGColor]];
		#endif
	}
	firstDraw = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Metal3DSurface::setSize (const Rect& _size)
{
	Native3DSurface::setSize (_size);
	[view setFrame:CGRectMake (size.left, size.top, size.getWidth (), size.getHeight ())];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Metal3DSurface::applyMultisampling (int sampleCount)
{
	CGFloat scaleFactor = 1;
	#if CCL_PLATFORM_MAC
	scaleFactor = [view layer].contentsScale;
	#else
	scaleFactor = [view contentScaleFactor];
	#endif
	if(scaleFactor)
		sampleCount = ccl_upperPowerOf2 (sampleCount / scaleFactor);
	
	id<MTLDevice> device = MetalClient::instance ().getDevice ();
	if([device supportsTextureSampleCount:sampleCount])
		[view setSampleCount:sampleCount];
	else
		[view setSampleCount:4]; // 1 and 4 are supported by all Metal devices
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Metal3DSurface::updateSampleCount ()
{
	if(content)
		applyMultisampling (content->getMultisampling ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Metal3DSurface::draw ()
{
	if(firstDraw)
	{
		updateSampleCount ();
		firstDraw = false;
	}
	[view draw];
}

//************************************************************************************************
// Metal3DSupport
//************************************************************************************************

void Metal3DSupport::shutdown3D ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DGraphicsFactory& Metal3DSupport::get3DFactory ()
{
	return factory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DSurface* Metal3DSupport::create3DSurface ()
{
	return NEW Metal3DSurface;
}

//************************************************************************************************
// Metal3DVertexFormat
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Metal3DVertexFormat, Native3DVertexFormat)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Metal3DVertexFormat::create (const VertexElementDescription description[], uint32 count, const IGraphicsShader3D* shader)
{
	vertexDescriptor = [[MTLVertexDescriptor vertexDescriptor] retain];
	int stride = 0;
	for(NSUInteger i = 0; i < count; i++)
	{
		NSObj<MTLVertexAttributeDescriptor> attributeDesc = [MTLVertexAttributeDescriptor new];
		[attributeDesc setFormat:getMetalVertexFormat (description[i].format)];
		[attributeDesc setOffset:stride];
		[attributeDesc setBufferIndex:kMetalVertexBufferIndex];
		[[vertexDescriptor attributes] setObject:attributeDesc atIndexedSubscript:i];
		stride += getMetalFormatSize (description[i].format);
	}
	NSObj<MTLVertexBufferLayoutDescriptor> layoutDesc = [MTLVertexBufferLayoutDescriptor new];
	[layoutDesc setStride:stride];
	[[vertexDescriptor layouts] setObject:layoutDesc atIndexedSubscript:kMetalVertexBufferIndex];

	return true;
}

//************************************************************************************************
// Metal3DBuffer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Metal3DBuffer, Native3DGraphicsBuffer)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Metal3DBuffer::create (Type _type, BufferUsage3D usage, uint32 sizeInBytes, uint32 strideInBytes, const void* initialData)
{
	NSString* label = nil;
	switch(_type)
	{
	case kVertexBuffer :
		label = @"Vertices";
		break;

	case kIndexBuffer :
		label = @"Indices";
		break;

	case kConstantBuffer :
		label = @"Constants";
		break;

	case kShaderResource :
		label = @"Shader Resources";
		break;

	default:
		return false;
	}
	
    type = _type;
	
	uint32 offset = 0;
	if(!ensureSegmentAlignment (offset, sizeInBytes, strideInBytes))
		return false;
	
	id<MTLDevice> device = MetalClient::instance ().getDevice ();
	if(initialData != nullptr)
		metalBuffer = [device newBufferWithBytes:initialData length:sizeInBytes options:getResourceOptions (usage)];
	else
		metalBuffer = [device newBufferWithLength:sizeInBytes options:getResourceOptions (usage)];
    
    capacity = sizeInBytes;
	[metalBuffer setLabel:label];
	
	return (metalBuffer != nil);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* Metal3DBuffer::map ()
{
	return [metalBuffer contents];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Metal3DBuffer::unmap ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Metal3DBuffer::ensureSegmentAlignment (uint32& byteOffset, uint32& size, uint32 stride) const
{
	uint32 alignment = stride;
	if(getType () == kConstantBuffer || getType () == kVertexBuffer)
	{
		uint32 bufferAlignment = 16;
		// see documentation of MTLRenderCommandEncoder setVertexBuffer:offset:atIndex and setFragmentBuffer:offset:atIndex
		#if CCL_PLATFORM_MAC || TARGET_OS_SIMULATOR
		bufferAlignment = 256;
		#endif
		alignment = ccl_lowest_common_multiple<uint32> (alignment, bufferAlignment);
	}
	
	byteOffset = ccl_align_to<uint32> (byteOffset, alignment);
	size = ccl_align_to<uint32> (size, alignment);
	
	return true;
}

//************************************************************************************************
// Metal3DTexture2D
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Metal3DTexture2D, Native3DTexture2D)

Metal3DTexture2D::Metal3DTexture2D ()
: bytesPerRowSource (0),
  width (0),
  height (0),
  hasMipmaps (false),
  immutable (false),
  lock (System::CreateAdvancedLock (ClassID::ReadWriteLock))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Metal3DTexture2D::create (uint32 _width, uint32 _height, uint32 bytesPerRow, DataFormat3D format, TextureFlags3D flags, const void* initialData)
{
	id<MTLDevice> device = MetalClient::instance ().getDevice ();

	hasMipmaps = get_flag<TextureFlags3D> (flags, kTextureMipmapEnabled);
	immutable = get_flag<TextureFlags3D> (flags, kTextureImmutable);
	width = _width;
	height = _height;
	MTLPixelFormat pixelFormat = getMetalPixelFormat (format);
	ASSERT (pixelFormat != MTLPixelFormatInvalid)
	bytesPerRowSource = width * getMetalFormatSize (format);

	MTLTextureDescriptor* textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat width:width height:height mipmapped:hasMipmaps];

	#if 0 // TODO check if this can be implemented using a MTLBuffer backed MTLTexture
	if(immutable)
		[textureDescriptor setStorageMode:MTLStorageModePrivate];
	#endif
	
	texture = [device newTextureWithDescriptor:textureDescriptor];
	setPixels (initialData);
	
	NSObj<MTLSamplerDescriptor> descriptor = [MTLSamplerDescriptor new];
	[descriptor setMinFilter:MTLSamplerMinMagFilterLinear];
	[descriptor setMagFilter:MTLSamplerMinMagFilterLinear];
	[descriptor setMipFilter:MTLSamplerMipFilterLinear];
	
	MTLSamplerAddressMode addressMode = MTLSamplerAddressModeClampToEdge;
	if(get_flag<TextureFlags3D> (flags, kTextureClampToBorder))
	{
		[descriptor setBorderColor:MTLSamplerBorderColorTransparentBlack];
		addressMode = MTLSamplerAddressModeClampToBorderColor;
	}
	else if(get_flag<TextureFlags3D> (flags, kTextureRepeat))
		addressMode = MTLSamplerAddressModeRepeat;
	else if(get_flag<TextureFlags3D> (flags, kTextureMirror))
		addressMode = MTLSamplerAddressModeMirrorRepeat;

	[descriptor setRAddressMode:addressMode];
	[descriptor setSAddressMode:addressMode];
	[descriptor setTAddressMode:addressMode];

	sampler = [device newSamplerStateWithDescriptor:descriptor];

	return (texture != nil);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Metal3DTexture2D::copyFromBitmap (IBitmap* bitmap)
{
	IMultiResolutionBitmap::RepSelector selector (UnknownPtr<IMultiResolutionBitmap> (bitmap), getHighestResolutionIndex (bitmap));
	BitmapDataLocker locker (bitmap, IBitmap::kRGBAlpha, IBitmap::kLockRead);
	if(locker.result != kResultOk)
		return locker.result;

	setPixels (locker.data.scan0);
	if(hasMipmaps)
		generateMipmaps ();
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Metal3DTexture2D::setPixels (const void* pixelData)
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kWrite);
	
	MTLRegion region { {0, 0, 0}, { width, height, 1 } };
	[texture replaceRegion:region mipmapLevel:0 withBytes:pixelData bytesPerRow:bytesPerRowSource];
	if(hasMipmaps)
		generateMipmaps ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Metal3DTexture2D::generateMipmaps ()
{
	id<MTLCommandBuffer> commandBuffer = [MetalClient::instance ().getQueue () commandBuffer];
	id<MTLBlitCommandEncoder> encoder = [commandBuffer blitCommandEncoder];
	[encoder generateMipmapsForTexture:texture];
	[encoder endEncoding];
	[commandBuffer commit];
}

//************************************************************************************************
// Metal3DShader
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Metal3DShader, Native3DGraphicsShader)

const FileType Metal3DShader::kMetalSourceFileType ("Metal Shading Language source file", "metal");
const FileType Metal3DShader::kMetalCompiledFileType ("Metal Shading Language compiled library", "metallib");

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Metal3DShader::create (GraphicsShader3DType _type, UrlRef _path)
{
	ASSERT (library == nil)

	path = _path;
	type = _type;

	Url metalShaderPath (path);
	metalShaderPath.setFileType (Metal3DShader::kMetalCompiledFileType);
	if(!System::GetFileSystem ().fileExists (metalShaderPath))
		return false;
	
	const FileType& fileType = metalShaderPath.getFileType ();
	AutoPtr<IMemoryStream> stream = File::loadBinaryFile (metalShaderPath);
	if(stream == nullptr)
		return false;

	id<MTLDevice> device = MetalClient::instance ().getDevice ();
	if(fileType == kMetalSourceFileType)
	{
		NSObj<NSString> source = [[NSString alloc] initWithBytes:stream->getMemoryAddress () length:stream->getBytesWritten () encoding:NSUTF8StringEncoding];
	    NSError* error = nil;
		library = [device newLibraryWithSource:source options:nil error:&error]; // this can block a long time; there is also an asynchronous version, but we better use compiled files for production anyway
		if(error)
			return false;
	}
	else if(fileType == kMetalCompiledFileType)
	{
		dispatch_data_t data = dispatch_data_create (stream->getMemoryAddress (), stream->getBytesWritten (), nullptr, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
		NSError* error = nil;
		library = [device newLibraryWithData:data error:&error];
		dispatch_release (data);
		if(error)
			return false;
	}
		
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITypeInfo* CCL_API Metal3DShader::getBufferTypeInfo (int bufferIndex)
{
	if(bufferIndex >= kMetalBufferIndexMax)
	{
		ASSERT (0)
		return nullptr;
	}

	if(SuperClass::getBufferTypeInfo (bufferIndex) == nullptr)
	{
		NSObj<NSObject<MTLFunction>> shader = [library newFunctionWithName:@"function"];
		if(shader)
		{
			MTLAutoreleasedArgument reflection;
			NSObj<NSObject<MTLArgumentEncoder>> encoder = [shader newArgumentEncoderWithBufferIndex:bufferIndex reflection:&reflection];
			if(encoder && reflection.type == MTLArgumentTypeBuffer && reflection.bufferDataType == MTLDataTypeStruct)
			{
				auto bufferTypeInfo = NEW ShaderTypeInfo3D;
				bufferTypeInfos.add (bufferTypeInfo);
				bufferTypeInfo->setStructSize (static_cast<uint32> (reflection.bufferDataSize));
				bufferTypeInfo->setStructName ([reflection.name cStringUsingEncoding:NSUTF8StringEncoding]);
				bufferTypeInfo->setBindingIndex (bufferIndex);
				for(MTLStructMember* member in [reflection bufferStructType].members)
				{
					const auto addTypeInfoRecursive = [&] (MTLStructMember* member) -> void
					{
						auto addTypeInfo = [&] (MTLStructMember* member, CStringRef name, ShaderTypeInfo3D* typeInfo, ShaderVariable3D* parent, auto& recurse) mutable -> void
						{
							AutoPtr<ShaderVariable3D> v = NEW ShaderVariable3D;
							v->setName (name);
							v->setOffset (static_cast<uint32> (member.offset + (parent ? parent->getOffset () : 0)));
							MTLDataType type = member.dataType == MTLDataTypeArray ? member.arrayType.elementType : member.dataType;
							if(member.dataType == MTLDataTypeArray)
							{
								v->setArrayElementCount (static_cast<uint32> (member.arrayType.arrayLength));
								v->setArrayElementStride (static_cast<uint32> (member.arrayType.stride));
							}
							
							if(type == MTLDataTypeFloat)
							{
								v->setType (ShaderVariable3D::kFloat);
								v->setSize (sizeof(float));
							}
							else if(type == MTLDataTypeFloat4)
							{
								v->setType (ShaderVariable3D::kFloat4);
								v->setSize (sizeof(float) * 4);
							}
							else if(type == MTLDataTypeFloat4x4)
							{
								v->setType (ShaderVariable3D::kFloat4x4);
								v->setSize (sizeof(float) * 4 * 4);
							}
							else if(type == MTLDataTypeInt)
							{
								v->setType (ShaderVariable3D::kInt);
								v->setSize (sizeof(int32));
							}
							else if(type == MTLDataTypeStruct)
							{
								v->setType (ShaderVariable3D::kStruct);
								AutoPtr<ShaderTypeInfo3D> structTypeInfo = NEW ShaderTypeInfo3D;
								v->setStructType (structTypeInfo);
								MTLStructType* structType = member.dataType == MTLDataTypeArray ? member.arrayType.elementStructType : member.structType;
								for(MTLStructMember* submember in structType.members)
									recurse (submember, [submember.name cStringUsingEncoding:NSUTF8StringEncoding], structTypeInfo, v, recurse);
							}
							else
							{
								ASSERT (false)
								v->setType (ShaderVariable3D::kUnknown);
								v->setSize (0);
							}
							
							if(typeInfo)
								typeInfo->addVariable (v.detach ());
						};
						addTypeInfo (member, [member.name cStringUsingEncoding:NSUTF8StringEncoding], bufferTypeInfo, nullptr, addTypeInfo);
					};
					addTypeInfoRecursive (member);
				}
			}
			else
			{
				ASSERT (false)
			}
		}
	}
	
	return SuperClass::getBufferTypeInfo (bufferIndex);
}

//************************************************************************************************
// Metal3DResourceManager
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Metal3DResourceManager, Native3DResourceManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DGraphicsShader* Metal3DResourceManager::loadShader (UrlRef path, GraphicsShader3DType type)
{
	AutoPtr<Metal3DShader> shader = NEW Metal3DShader;
	if(!shader->create (type, path))
		return nullptr;

	return shader.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DTexture2D* Metal3DResourceManager::loadTexture (Bitmap* bitmap, TextureFlags3D flags)
{
	AutoPtr<Metal3DTexture2D> texture = NEW Metal3DTexture2D;
	
	if(texture->create (bitmap, flags))
		return texture.detach ();
		
	return nullptr;
}

//************************************************************************************************
// Metal3DPipeline
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Metal3DPipeline, Native3DGraphicsPipeline)

//////////////////////////////////////////////////////////////////////////////////////////////////

Metal3DPipeline::Metal3DPipeline ()
: changed (true),
  state (nil),
  primitiveType (MTLPrimitiveTypeTriangle),
  fillMode (MTLTriangleFillModeFill),
  depthTestEnabled (true),
  depthWriteEnabled (true),
  depthBias (0),
  sampleCount (1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

id<MTLRenderPipelineState> Metal3DPipeline::getPipeline (MTKView* view)
{
	if(sampleCount != view.sampleCount)
	{
		changed = true;
		sampleCount = view.sampleCount;
	}
		
	if(changed)
	{
		state = createPipeline (view);
		changed = false;
	}
	
	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Metal3DShader* Metal3DPipeline::getShader (int index) const
{
	return shaderList.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Metal3DPipeline::setVertexShader (IGraphicsShader3D* shader)
{
	if(shader == nullptr || shader->getType () != IGraphicsShader3D::kVertexShader)
		return kResultInvalidArgument;

	return setShader (kVertexShaderIndex, shader);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Metal3DPipeline::setPixelShader (IGraphicsShader3D* shader)
{
	if(shader == nullptr || shader->getType () != IGraphicsShader3D::kPixelShader)
		return kResultInvalidArgument;

	return setShader (kPixelShaderIndex, shader);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Metal3DPipeline::setVertexFormat (IVertexFormat3D* _format)
{
	Metal3DVertexFormat* format = unknown_cast<Metal3DVertexFormat> (_format);
	if(format == nullptr)
		return kResultInvalidArgument;
	
	vertexFormat = format;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Metal3DPipeline::setPrimitiveTopology (PrimitiveTopology3D primitiveTopology)
{
	switch(primitiveTopology)
	{
	case kPrimitiveTopologyTriangleList:
		primitiveType = MTLPrimitiveTypeTriangle;
		break;
	case kPrimitiveTopologyTriangleStrip:
		primitiveType = MTLPrimitiveTypeTriangleStrip;
		break;
	default:
		return kResultInvalidArgument;
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Metal3DPipeline::setFillMode (FillMode3D mode)
{
	switch(mode)
	{
	case kFillModeSolid:
		fillMode = MTLTriangleFillModeFill;
		break;
	case kFillModeWireframe:
		fillMode = MTLTriangleFillModeLines;
		break;
	default:
		return kResultInvalidArgument;
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

id<MTLRenderPipelineState> Metal3DPipeline::createPipeline (MTKView* view)
{
	NSObj<MTLRenderPipelineDescriptor> descriptor = [MTLRenderPipelineDescriptor new];
	[descriptor setLabel: @"Metal3DPipeline"];
	[descriptor setVertexDescriptor:vertexFormat->getVertexDescriptor ()];
	
	for(int i = 0; i < shaderList.count (); i++)
	{
		Metal3DShader* shader = shaderList.at (i);
		if(shader == nullptr)
			continue;
				
		if(shader->getType () == IGraphicsShader3D::kVertexShader)
			[descriptor setVertexFunction: [shader->getLibrary () newFunctionWithName:@"function"]];
		else if(shader->getType () == IGraphicsShader3D::kPixelShader)
			[descriptor setFragmentFunction: [shader->getLibrary () newFunctionWithName:@"function"]];
		else
			continue;
	}
	MTLRenderPipelineColorAttachmentDescriptor *attachment = [descriptor colorAttachments][0];
	attachment.pixelFormat = view.colorPixelFormat;
	attachment.blendingEnabled = YES;
	attachment.rgbBlendOperation = MTLBlendOperationAdd;
	attachment.alphaBlendOperation = MTLBlendOperationAdd;
	attachment.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
	attachment.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
	attachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	attachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	
	[descriptor setRasterSampleCount:view.sampleCount];
	[descriptor setDepthAttachmentPixelFormat:view.depthStencilPixelFormat];
	
	NSError* error = nil;
	state = [view.device newRenderPipelineStateWithDescriptor:descriptor error:&error];
	if(error)
		return nil;

	stencilState = createDepthStencil (view);

	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Metal3DPipeline::setShader (int index, IGraphicsShader3D* shader)
{
	if(shader == nullptr && index < shaderList.count ())
	{
		shaderList[index].release ();
		changed = true;
		return kResultOk;
	}
	
	Metal3DShader* newShader = unknown_cast<Metal3DShader> (shader);
	if(newShader == nullptr)
		return kResultInvalidArgument;
	
	if(index >= shaderList.count ())
	{
		shaderList.setCount (index + 1);
		changed = true;
	}
	
	if(shaderList[index] != newShader)
		changed = true;
	shaderList[index] = newShader;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

id<MTLDepthStencilState> Metal3DPipeline::createDepthStencil (MTKView* view)
{
	if(view.depthStencilPixelFormat != MTLPixelFormatDepth32Float)
		return nil;
	
	NSObj<MTLDepthStencilDescriptor> depthDescriptor = [MTLDepthStencilDescriptor new];
	[depthDescriptor setDepthCompareFunction:depthTestEnabled ? MTLCompareFunctionLessEqual : MTLCompareFunctionAlways];
	[depthDescriptor setDepthWriteEnabled:depthWriteEnabled];
	return [view.device newDepthStencilStateWithDescriptor:depthDescriptor];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Metal3DPipeline::setDepthTestParameters (const DepthTestParameters3D& parameters)
{
	depthTestEnabled = parameters.testEnabled;
	depthWriteEnabled = parameters.writeEnabled;
	depthBias = parameters.bias;
	
	return kResultOk;
}

//************************************************************************************************
// Metal3DGraphicsFactory
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Metal3DGraphicsFactory, Native3DGraphicsFactory)
DEFINE_EXTERNAL_SINGLETON (Native3DGraphicsFactory, Metal3DGraphicsFactory)

//////////////////////////////////////////////////////////////////////////////////////////////////

IVertexFormat3D* CCL_API Metal3DGraphicsFactory::createVertexFormat (const VertexElementDescription description[], uint32 count, const IGraphicsShader3D* shader)
{
	AutoPtr<Metal3DVertexFormat> format = NEW Metal3DVertexFormat;
	if(!format->create (description, count, shader))
		return nullptr;

	return format.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsBuffer3D* CCL_API Metal3DGraphicsFactory::createBuffer (GraphicsBuffer3DType type, BufferUsage3D usage,
								uint32 sizeInBytes, uint32 strideInBytes, const void* initialData /* = nullptr */)
{
	AutoPtr<Metal3DBuffer> buffer = NEW Metal3DBuffer;
	if(!buffer->create (type, usage, sizeInBytes, strideInBytes, initialData))
		return nullptr;

	return buffer.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsTexture2D* CCL_API Metal3DGraphicsFactory::createTexture (IBitmap* _bitmap, TextureFlags3D flags)
{
	auto* bitmap = unknown_cast<Bitmap> (_bitmap);
	if(bitmap == nullptr)
		return nullptr;

	Metal3DResourceManager& manager = Metal3DResourceManager::instance ();
	return return_shared (manager.getTexture (bitmap, flags));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsShader3D* CCL_API Metal3DGraphicsFactory::createShader (GraphicsShader3DType type, UrlRef path)
{
	Metal3DResourceManager& manager = Metal3DResourceManager::instance ();
	return return_shared (manager.getShader (path, type));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsShader3D* CCL_API Metal3DGraphicsFactory::createStockShader (GraphicsShader3DType type, StringID name)
{
	ResourceUrl url {String (name)};
	Metal3DResourceManager& manager = Metal3DResourceManager::instance ();
	return return_shared (manager.getShader (url, type));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsPipeline3D* CCL_API Metal3DGraphicsFactory::createPipeline ()
{
	return NEW Metal3DPipeline;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IShaderParameterSet3D* CCL_API Metal3DGraphicsFactory::createShaderParameterSet ()
{
	return NEW Native3DShaderParameterSet;
}

//************************************************************************************************
// Metal3DGraphicsContext
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Metal3DGraphicsContext, Native3DGraphicsDevice)

//////////////////////////////////////////////////////////////////////////////////////////////////

Metal3DGraphicsContext::Metal3DGraphicsContext (MTKView* _view)
: view (_view),
  commandBuffer (nil),
  encoder (nil),
  vertexBufferStride (0),
  indexBufferFormat (kR32_UInt),
  activePipeline (nullptr)
{
	ASSERT (_view)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Metal3DGraphicsContext::~Metal3DGraphicsContext ()
{
	if(encoder)
		[encoder endEncoding];
	if(commandBuffer)
	{
		[commandBuffer presentDrawable:view.currentDrawable];
		[commandBuffer commit];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Metal3DGraphicsContext::setPipeline (IGraphicsPipeline3D* _graphicsPipeline)
{
	Metal3DPipeline* graphicsPipeline = unknown_cast<Metal3DPipeline> (_graphicsPipeline);
	if(graphicsPipeline == nullptr)
		return kResultInvalidArgument;

	pipeline = graphicsPipeline;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Metal3DGraphicsContext::setVertexBuffer (IGraphicsBuffer3D* _buffer, uint32 stride)
{
	Metal3DBuffer* buffer = unknown_cast<Metal3DBuffer> (_buffer);
	if(buffer == nullptr)
		return kResultInvalidArgument;

	vertexBuffer = buffer;
	vertexBufferStride = stride;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Metal3DGraphicsContext::setIndexBuffer (IGraphicsBuffer3D* _buffer, DataFormat3D format)
{
	Metal3DBuffer* buffer = unknown_cast<Metal3DBuffer> (_buffer);
	if(buffer == nullptr)
		return kResultInvalidArgument;

	indexBuffer = buffer;
	indexBufferFormat = format;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Metal3DGraphicsContext::setShaderParameters (IShaderParameterSet3D* _parameters)
{
	auto* parameterSet = unknown_cast<Native3DShaderParameterSet> (_parameters);
	if(parameterSet == nullptr)
		return kResultInvalidArgument;

	shaderParameters = parameterSet;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Metal3DGraphicsContext::draw (uint32 startVertex, uint32 vertexCount)
{
	if(vertexBuffer == nullptr)
		return kResultFailed;

	prepareEncoder ();
	[encoder drawPrimitives:static_cast<MTLPrimitiveType> (pipeline->getPrimitiveType ()) vertexStart:startVertex vertexCount:vertexCount];
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Metal3DGraphicsContext::drawIndexed (uint32 startIndex, uint32 indexCount, int32 baseVertex)
{
	if(vertexBuffer == nullptr || indexBuffer == nullptr)
		return kResultFailed;
	
	prepareEncoder ();
	
	MTLIndexType indexType = MTLIndexTypeUInt16;
	NSUInteger offset = 0;
	if(indexBufferFormat == kR16_UInt)
	{
		indexType = MTLIndexTypeUInt16;
		offset = startIndex * sizeof(uint16);
	}
	else if(indexBufferFormat == kR32_UInt)
	{
		indexType = MTLIndexTypeUInt32;
		offset = startIndex * sizeof(uint32);
	}
	else
		return kResultNotImplemented;
		
	[encoder drawIndexedPrimitives:static_cast<MTLPrimitiveType> (pipeline->getPrimitiveType ()) indexCount:indexCount indexType:indexType indexBuffer:indexBuffer->getBuffer () indexBufferOffset:offset instanceCount:1 baseVertex:baseVertex baseInstance:0];
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Metal3DGraphicsContext::prepareEncoder ()
{
	if(commandBuffer == nil)
	{
		commandBuffer = [MetalClient::instance ().getQueue () commandBuffer];
		[commandBuffer enqueue];
	}
	ASSERT (commandBuffer)
	if(encoder == nil)
		encoder = [commandBuffer renderCommandEncoderWithDescriptor:view.currentRenderPassDescriptor];
	
	if(encoder == nil)
		return;
	
	encoder.label = @"Metal3DGraphicsContext";
	if(pipeline != activePipeline)
	{
		[encoder setRenderPipelineState:pipeline->getPipeline (view)];
		[encoder setCullMode:MTLCullModeBack];
		[encoder setFrontFacingWinding:MTLWindingCounterClockwise];
		
		if(id<MTLDepthStencilState> depthStencil = pipeline->getDepthStencil ())
			[encoder setDepthStencilState:depthStencil];
		float depthBias = pipeline->getDepthBias ();
		if(depthBias != 0)
			[encoder setDepthBias:depthBias slopeScale:0 clamp:0];

		[encoder setTriangleFillMode:static_cast<MTLTriangleFillMode> (pipeline->getFillMode ())];
		activePipeline = pipeline;
	}
	
	[encoder setVertexBuffer:vertexBuffer->getBuffer () offset:0 atIndex:kMetalVertexBufferIndex];
	if(shaderParameters)
	{
		const Vector<Native3DShaderParameters>& vsParameters = shaderParameters->getVertexShaderParameters ();
		for(const Native3DShaderParameters& parameters : vsParameters)
		{
			Metal3DBuffer* constantBuffer = unknown_cast<Metal3DBuffer> (parameters.segment->getBuffer ());
			if(constantBuffer)
			{
				uint32 offset = parameters.segment->getOffset ();
				[encoder setVertexBuffer:constantBuffer->getBuffer () offset:offset atIndex:parameters.bufferIndex];
			}
		}
		const Vector<Native3DShaderParameters>& psParameters = shaderParameters->getPixelShaderParameters ();
		for(const Native3DShaderParameters& parameters : psParameters)
		{
			Metal3DBuffer* constantBuffer = unknown_cast<Metal3DBuffer> (parameters.segment->getBuffer ());
			if(constantBuffer)
			{
				uint32 offset = parameters.segment->getOffset ();
				[encoder setFragmentBuffer:constantBuffer->getBuffer () offset:offset atIndex:parameters.bufferIndex];
			}
		}
		for(int i = 0; i < Native3DShaderParameterSet::kMaxTextureCount; i++)
			if(auto* texture = unknown_cast<Metal3DTexture2D> (shaderParameters->getTexture (i)))
			{
				texture->getLock ()->lock (Threading::ILockable::kRead);
				[encoder setFragmentTexture:texture->getTexture () atIndex:i];
				[encoder setFragmentSamplerState:texture->getSampler () atIndex:i];
				[commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer)
				{
					texture->getLock ()->unlock (Threading::ILockable::kRead);
				}];
			}
	}
}
