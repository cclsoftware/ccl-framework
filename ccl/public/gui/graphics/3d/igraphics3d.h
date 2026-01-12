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
// Filename    : ccl/public/gui/graphics/3d/igraphics3d.h
// Description : 3D Graphics Interface
//
//************************************************************************************************

#ifndef _ccl_igraphics3d_h
#define _ccl_igraphics3d_h

#include "ccl/public/base/variant.h"

#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/gui/graphics/3d/vertex3d.h"
#include "ccl/public/gui/graphics/3d/transform3d.h"
#include "ccl/public/gui/graphics/3d/ibufferallocator3d.h"

namespace CCL {

interface ITypeInfo;
interface IBitmap;
interface IGeometry3D;

//************************************************************************************************
// PrimitiveTopology3D
//************************************************************************************************

DEFINE_ENUM (PrimitiveTopology3D)
{
	kPrimitiveTopologyTriangleList,
	kPrimitiveTopologyTriangleStrip,
	kPrimitiveTopologyTriangleFan
};

//************************************************************************************************
// FillMode3D
//************************************************************************************************

DEFINE_ENUM (FillMode3D)
{
	kFillModeSolid,
	kFillModeWireframe
};

//************************************************************************************************
// TextureFlags3D
//************************************************************************************************

DEFINE_ENUM (TextureFlags3D)
{
	kTextureClampToEdge = 0,		///< When sampling pixels outside of the textures boundaries, use edge pixels (default)
	kTextureClampToBorder = 1<<1,	///< When sampling pixels outside of the textures boundaries, use border color
	kTextureRepeat = 1<<2,			///< When sampling pixels outside of the textures boundaries, repeat the texture
	kTextureMirror = 1<<3,			///< When sampling pixels outside of the textures boundaries, mirror the texture

	kTextureMipmapEnabled = 1<<4, 	///< Automatically generate mipmaps on creation and whenever the texture is updated.
	kTextureImmutable = 1<<5		///< The texture is immutable. It's data cannot be changed.
};

//************************************************************************************************
// IVertexFormat3D
/** Interface for describing the memory layout of a vertex format
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IVertexFormat3D: IUnknown
{
	DECLARE_IID (IVertexFormat3D)
};

DEFINE_IID (IVertexFormat3D, 0x277ea06f, 0x29e4, 0x4e78, 0x82, 0xfe, 0xe, 0x99, 0x0, 0x72, 0xf7, 0x66)

//************************************************************************************************
// IGraphicsTexture2D
/** 2D texture interface.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IGraphicsTexture2D: IUnknown
{
	/**	Update texture data.
		Dimensions of the texture and the new bitmap must match.
		The texture must not be immutable, see kTextureImmutable.
		If the bitmap object provides data in multiple resolutions, the highest resolution bitmap is used. */
	virtual tresult CCL_API copyFromBitmap (IBitmap* bitmap) = 0;

	DECLARE_IID (IGraphicsTexture2D)
};

DEFINE_IID (IGraphicsTexture2D, 0x76b10c3, 0x962a, 0x4ae4, 0xa8, 0x9f, 0x71, 0xe0, 0xae, 0x47, 0x71, 0x14)

//************************************************************************************************
// IGraphicsShader3D
/** Interface for a shader that runs on the GPU
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IGraphicsShader3D: IUnknown
{
	DEFINE_ENUM (Type)
	{
		kVertexShader,
		kPixelShader
	};

	virtual Type CCL_API getType () const = 0;

	virtual const void* CCL_API getBlobAddress () const = 0;
	
	virtual uint32 CCL_API getBlobSize () const = 0;

	virtual ITypeInfo* CCL_API getBufferTypeInfo (int bufferIndex) = 0;

	DECLARE_IID (IGraphicsShader3D)
};

DEFINE_IID (IGraphicsShader3D, 0xed0272e8, 0x352e, 0x4323, 0xad, 0x8e, 0xaf, 0xc5, 0x25, 0x19, 0x8e, 0x8b)

typedef IGraphicsShader3D::Type GraphicsShader3DType;

//************************************************************************************************
// IShaderValue3D
/** Interface representing a value in a shader constant buffer.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IShaderValue3D: IUnknown
{
	virtual tresult CCL_API setValue (VariantRef value) = 0;

	virtual tresult CCL_API setValue (Transform3DRef transform) = 0;

	virtual tresult CCL_API setValue (PointF4DRef point) = 0;

	virtual tresult CCL_API setValue (ColorFRef color) = 0;

	virtual IShaderValue3D& CCL_API getMember (StringID name) = 0;

	virtual IShaderValue3D& CCL_API getElementAt (int index) = 0;

	DECLARE_IID (IShaderValue3D)

	//////////////////////////////////////////////////////////////////////////////////////////////

	IShaderValue3D& operator = (VariantRef value) { setValue (value); return *this; }
	IShaderValue3D& operator = (Transform3DRef transform) { setValue (transform); return *this; }
	IShaderValue3D& operator = (PointF4DRef point) { setValue (point); return *this; }
	IShaderValue3D& operator = (ColorFRef color) { setValue (color); return *this; }
	IShaderValue3D& operator [] (StringID name) { return getMember (name); }
	IShaderValue3D& operator [] (int index) { return getElementAt (index); }
};

DEFINE_IID (IShaderValue3D, 0xd3bb818f, 0xfcd7, 0x44d1, 0x89, 0x53, 0xad, 0xe2, 0x15, 0xbb, 0x30, 0x68)

//************************************************************************************************
// IShaderBufferWriter3D
/** Interface for writing values to shader constant buffer.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IShaderBufferWriter3D: IUnknown
{
	virtual tresult CCL_API setBufferTypeInfo (ITypeInfo* typeInfo) = 0;

	virtual tresult CCL_API setBuffer (IBufferSegment3D* buffer) = 0;

	virtual IShaderValue3D& CCL_API asValue () = 0;

	DECLARE_IID (IShaderBufferWriter3D)
};

DEFINE_IID (IShaderBufferWriter3D, 0x2fd70260, 0xc68b, 0x45e4, 0xba, 0xb4, 0xcc, 0x30, 0xa7, 0x43, 0xba, 0x62)

//************************************************************************************************
// IShaderParameterSet3D
/** Set of shader parameter buffers
	\ingroup gui_graphics */
//************************************************************************************************

interface IShaderParameterSet3D: IUnknown
{
	static constexpr int kMaxTextureCount = 5;

	virtual tresult CCL_API setVertexShaderParameters (int bufferIndex, IBufferSegment3D* parameters) = 0;

	virtual tresult CCL_API setPixelShaderParameters (int bufferIndex, IBufferSegment3D* parameters) = 0;

	virtual tresult CCL_API setTexture (int textureIndex, IGraphicsTexture2D* texture) = 0;

	DECLARE_IID (IShaderParameterSet3D)
};

DEFINE_IID (IShaderParameterSet3D, 0x15665f2d, 0x270f, 0x4129, 0xa4, 0x55, 0x01, 0x8d, 0x14, 0x4b, 0x22, 0xbe)

//************************************************************************************************
// DepthTestParameters3D
/** Depth test parameters.
	\ingroup gui_graphics */
//************************************************************************************************

struct DepthTestParameters3D
{
	tbool testEnabled = false;
	tbool writeEnabled = false;
	float bias = 0.f;
};

//************************************************************************************************
// IGraphicsPipeline3D
/** Interface for a 3D graphics pipeline
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IGraphicsPipeline3D: IUnknown
{
	virtual tresult CCL_API setPrimitiveTopology (PrimitiveTopology3D topology) = 0;
	
	virtual tresult CCL_API setFillMode (FillMode3D mode) = 0;

	virtual tresult CCL_API setVertexFormat (IVertexFormat3D* format) = 0;
	
	virtual tresult CCL_API setVertexShader (IGraphicsShader3D* shader) = 0;
	
	virtual tresult CCL_API setPixelShader (IGraphicsShader3D* shader) = 0;

	virtual tresult CCL_API setDepthTestParameters (const DepthTestParameters3D& parameters) = 0;

	DECLARE_IID (IGraphicsPipeline3D)
};

DEFINE_IID (IGraphicsPipeline3D, 0xf9262449, 0x0078, 0x4277, 0xae, 0x46, 0x0f, 0x66, 0x12, 0xd9, 0x4d, 0x56)

//************************************************************************************************
// IGraphicsFactory3D
/** Factory interface for 3D resources.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IGraphicsFactory3D: IUnknown
{
	virtual IGraphicsShader3D* CCL_API createShader (GraphicsShader3DType type, UrlRef path) = 0;

	virtual IGraphicsShader3D* CCL_API createStockShader (GraphicsShader3DType type, StringID name) = 0;

	virtual IVertexFormat3D* CCL_API createVertexFormat (const VertexElementDescription description[], uint32 count,
														 const IGraphicsShader3D* shader) = 0;
	
	virtual IGraphicsBuffer3D* CCL_API createBuffer (GraphicsBuffer3DType type, BufferUsage3D usage, 
													 uint32 sizeInBytes, uint32 strideInBytes, 
													 const void* initialData = nullptr) = 0;

	/**	Create a new texture.
		If called multiple times using the same bitmap and the kTextureImmutable flag, this method returns the same instance. */
	virtual IGraphicsTexture2D* CCL_API createTexture (IBitmap* bitmap, TextureFlags3D flags) = 0;

	virtual IGraphicsPipeline3D* CCL_API createPipeline () = 0;

	virtual IShaderParameterSet3D* CCL_API createShaderParameterSet () = 0;

	virtual IShaderBufferWriter3D* CCL_API createShaderBufferWriter () = 0;
	
	DECLARE_IID (IGraphicsFactory3D)
};

DEFINE_IID (IGraphicsFactory3D, 0x7f1b6988, 0xcee7, 0x4c4e, 0xac, 0x86, 0x8f, 0x8, 0xcf, 0xb5, 0x84, 0x31)

//************************************************************************************************
// IGraphics3D
/** Interface provided by framework for drawing 3D primitives with GPU hardware-acceleration.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IGraphics3D: IUnknown
{	
	virtual tresult CCL_API setPipeline (IGraphicsPipeline3D* pipeline) = 0;
	
	virtual tresult CCL_API setVertexBuffer (IGraphicsBuffer3D* buffer, uint32 stride) = 0;

	virtual tresult CCL_API setIndexBuffer (IGraphicsBuffer3D* buffer, DataFormat3D format) = 0;

	virtual tresult CCL_API setShaderParameters (IShaderParameterSet3D* parameters) = 0;

	virtual tresult CCL_API draw (uint32 startVertex, uint32 vertexCount) = 0;

	virtual tresult CCL_API drawIndexed (uint32 startIndex, uint32 indexCount, int32 baseVertex) = 0;

	virtual tresult CCL_API drawGeometry (IGeometry3D* geometry) = 0;

	DECLARE_IID (IGraphics3D)
};

DEFINE_IID (IGraphics3D, 0x62965490, 0xd4f7, 0x4f2a, 0x80, 0x8a, 0xf7, 0x5c, 0x5e, 0xee, 0x40, 0x3c)

//************************************************************************************************
// IGraphicsContent3D
/** Interface implemented by the application for rendering 3D content.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IGraphicsContent3D: IUnknown
{
	/**	Set up graphics resources for rendering 3D content.
		The callee may hold a reference to the factory in order to create additional graphics resources at a later stage. */
	virtual tresult CCL_API createContent (IGraphicsFactory3D& factory) = 0;

	/** Release all previously created graphics resources as well as the factory. */
	virtual tresult CCL_API releaseContent () = 0;
	
	/**	Render 3D content.
		The callee should not use the factory in this call.
		All graphics resources should have been created at an earlier stage. */
	virtual tresult CCL_API renderContent (IGraphics3D& graphics) = 0;

	/** 3D content properties. */
	DEFINE_ENUM (ContentProperty3D)
	{
		kContentHint,	///< GraphicsContentHint, same as in IGraphicsLayer
		kBackColor,		///< Background color as uint32
		kMultisampling	///< Sample count (integer, 1...n)
	};

	/** Get 3D content property. */
	virtual tresult CCL_API getContentProperty (Variant& value, ContentProperty3D propertyId) const = 0;

	DECLARE_IID (IGraphicsContent3D)

	//////////////////////////////////////////////////////////////////////////////////////////////

	GraphicsContentHint getContentHint () const;
	Color getBackColor () const;
	int getMultisampling () const;
};

DEFINE_IID (IGraphicsContent3D, 0xff8d8514, 0x28bf, 0x4f4f, 0x87, 0x60, 0x58, 0x98, 0x86, 0xf3, 0x6c, 0x15)

//////////////////////////////////////////////////////////////////////////////////////////////////
// IGraphicsContent3D
//////////////////////////////////////////////////////////////////////////////////////////////////

inline GraphicsContentHint IGraphicsContent3D::getContentHint () const
{ 
	Variant v = kGraphicsContentHintDefault; 
	getContentProperty (v, kContentHint);
	return v.asInt (); 
}

inline Color IGraphicsContent3D::getBackColor () const
{
	Variant v = uint32 (Colors::kTransparentBlack);
	getContentProperty (v, kBackColor);
	return Color::fromInt (v.asUInt ()); 
}

inline int IGraphicsContent3D::getMultisampling () const
{
	Variant v = 1; 
	getContentProperty (v, kMultisampling);
	return v.asInt (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_igraphics3d_h
