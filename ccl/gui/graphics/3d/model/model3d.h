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
// Filename    : ccl/gui/graphics/3d/model/model3d.h
// Description : 3D Model
//
//************************************************************************************************

#ifndef _ccl_model3d_h
#define _ccl_model3d_h

#include "ccl/gui/graphics/3d/nativegraphics3d.h"
#include "ccl/gui/graphics/imaging/bitmap.h"

#include "ccl/public/gui/graphics/3d/imodel3d.h"

namespace CCL {

//************************************************************************************************
// Material3D
//************************************************************************************************

class Material3D: public Object,
                  public IMaterial3D
{
public:
	DECLARE_CLASS (Material3D, Object)

	Material3D ();

	static constexpr uint32 kDefaultLightMask = 0xFFFFFFFF;

	virtual bool requiresTextureCoordinates () const { return false; }
	
	// IMaterial3D
	GraphicsContentHint CCL_API getMaterialHint () const override { return kGraphicsContentHintDefault; }
	IGraphicsShader3D* CCL_API getPixelShader () const override { return pixelShader; }
	void CCL_API setDepthBias (float bias) override { depthBias = bias; }
	float CCL_API getDepthBias () const override { return depthBias; }
	void CCL_API setLightMask (uint32 mask) override { lightMask = mask; }
	uint32 CCL_API getLightMask () const override { return lightMask; }
	
	// IShaderParameterProvider3D
	void CCL_API getShaderParameters (IShaderValue3D& parameters) const override;
	
	CLASS_INTERFACE2 (IMaterial3D, IShaderParameterProvider3D, Object)
	
protected:
	AutoPtr<IGraphicsShader3D> pixelShader;
	uint32 lightMask;
	float depthBias;
};

//************************************************************************************************
// SolidColorMaterial3D
//************************************************************************************************

class SolidColorMaterial3D: public Material3D,
							public ISolidColorMaterial3D
{
public:
	DECLARE_CLASS (SolidColorMaterial3D, Material3D)
	DECLARE_PROPERTY_NAMES (SolidColorMaterial3D)
	
	SolidColorMaterial3D ();
	
	// ISolidColorMaterial3D
	void CCL_API setMaterialColor (ColorRef color) override;
	ColorRef CCL_API getMaterialColor () const override;
	void CCL_API setShininess (float shininess) override;
	float CCL_API getShininess () const override;

	// Material3D
	void CCL_API getShaderParameters (IShaderValue3D& parameters) const override;
	GraphicsContentHint CCL_API getMaterialHint () const override { return materialColor.alpha != 0xFF ? kGraphicsContentTranslucent : kGraphicsContentOpaque; }
	IGraphicsShader3D* CCL_API getPixelShader () const override { return SuperClass::getPixelShader (); }
	void CCL_API setDepthBias (float bias) override { SuperClass::setDepthBias (bias); }
	float CCL_API getDepthBias () const override { return SuperClass::getDepthBias (); }
	void CCL_API setLightMask (uint32 mask) override { return SuperClass::setLightMask (mask); }
	uint32 CCL_API getLightMask () const override { return SuperClass::getLightMask (); }

	CLASS_INTERFACE (ISolidColorMaterial3D, Material3D)

protected:
	Color materialColor;
	float shininess;

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// TextureMaterial3D
//************************************************************************************************

class TextureMaterial3D: public SolidColorMaterial3D,
						 public ITextureMaterial3D
{
public:
	DECLARE_CLASS (TextureMaterial3D, SolidColorMaterial3D)
	DECLARE_PROPERTY_NAMES (TextureMaterial3D)
	
	TextureMaterial3D ();
	
	IGraphicsTexture2D* getGraphicsTexture (int textureIndex);
	
	// ITextureMaterial3D
	tresult CCL_API setTexture (int textureIndex, IBitmap* texture) override;
	IBitmap* CCL_API getTexture (int textureIndex) const override;
	tresult CCL_API setTextureFlags (int textureIndex, TextureFlags3D flags) override;
	TextureFlags3D CCL_API getTextureFlags (int textureIndex) const override;
	void CCL_API setOpacity (float _opacity) override { opacity = _opacity; }
	float CCL_API getOpacity () const override { return opacity; }
	
	// SolidColorMaterial3D
	void CCL_API getShaderParameters (IShaderValue3D& parameters) const override;
	GraphicsContentHint CCL_API getMaterialHint () const override;
	IGraphicsShader3D* CCL_API getPixelShader () const override { return SuperClass::getPixelShader (); }
	void CCL_API setDepthBias (float bias) override { SuperClass::setDepthBias (bias); }
	float CCL_API getDepthBias () const override { return SuperClass::getDepthBias (); }
	bool requiresTextureCoordinates () const override;
	void CCL_API setLightMask (uint32 mask) override { return SuperClass::setLightMask (mask); }
	uint32 CCL_API getLightMask () const override { return SuperClass::getLightMask (); }

	CLASS_INTERFACE (ITextureMaterial3D, SolidColorMaterial3D)

protected:
	struct TextureItem
	{
		SharedPtr<Bitmap> bitmap;
		TextureFlags3D flags;
		bool needsUpdate;

		AutoPtr<IGraphicsTexture2D> texture;

		TextureItem (Bitmap* bitmap = nullptr, TextureFlags3D flags = kTextureImmutable|kTextureMipmapEnabled)
		: bitmap (bitmap),
		  flags (flags),
		  needsUpdate (false)
		{}
	};

	Vector<TextureItem> textures;
	float opacity;

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// CustomMaterial3D
//************************************************************************************************

class CustomMaterial3D: public TextureMaterial3D,
						public ICustomMaterial3D
{
public:
	DECLARE_CLASS (CustomMaterial3D, TextureMaterial3D)
	
	// ICustomMaterial3D
	void CCL_API setShaderParameterProvider (IShaderParameterProvider3D* provider) override { this->provider = provider; }
	void CCL_API setPixelShader (IGraphicsShader3D* shader) override { this->pixelShader.share (shader); }
	void CCL_API setMaterialHint (GraphicsContentHint hint) override { this->customHint = hint; }
	
	// TextureMaterial3D
	void CCL_API getShaderParameters (IShaderValue3D& parameters) const override;
	GraphicsContentHint CCL_API getMaterialHint () const override;
	IGraphicsShader3D* CCL_API getPixelShader () const override { return SuperClass::getPixelShader (); }
	void CCL_API setDepthBias (float bias) override { SuperClass::setDepthBias (bias); }
	float CCL_API getDepthBias () const override { return SuperClass::getDepthBias (); }
	void CCL_API setLightMask (uint32 mask) override { return SuperClass::setLightMask (mask); }
	uint32 CCL_API getLightMask () const override { return SuperClass::getLightMask (); }

	CLASS_INTERFACE (ICustomMaterial3D, TextureMaterial3D)
	
protected:
	SharedPtr<IShaderParameterProvider3D> provider;
	static const GraphicsContentHint kHintNotSet = -1;
	GraphicsContentHint customHint = kHintNotSet;
};

//************************************************************************************************
// BaseGeometry3D
//************************************************************************************************

class BaseGeometry3D: public Object,
					  public IGraphicsResource3D,
					  public IGeometry3D
{
public:
	DECLARE_CLASS_ABSTRACT (BaseGeometry3D, Object)

	BaseGeometry3D ();

	// IGraphicsResource3D
	tbool CCL_API isGPUAccessible () const override { return false; }
	tresult CCL_API upload (IBufferAllocator3D& allocator) override { return kResultFailed; }
	void CCL_API discard () override {}

	// IGeometry3D
	uint32 CCL_API getVertexCount () const override { return 0; }
	uint32 CCL_API getIndexCount () const override { return 0; }
	const PointF3D* CCL_API getPositions () const override { return nullptr; }
	const PointF3D* CCL_API getNormals () const override { return nullptr; }
	const PointF* CCL_API getTextureCoords () const override { return nullptr; }
	const uint32* CCL_API getIndices () const override { return nullptr; }
	tresult CCL_API setVertexData (const PointF3D positions[], const PointF3D normals[], const PointF textureCoords[], uint32 count) override { return kResultFailed; }
	tresult CCL_API setIndices (const uint32 indices[], uint32 count) override { return kResultFailed; }
	void CCL_API setPrimitiveTopology (PrimitiveTopology3D topology) override {}
	PrimitiveTopology3D CCL_API getPrimitiveTopology () const override { return kPrimitiveTopologyTriangleList; }
	IBufferSegment3D* CCL_API getVertexBufferSegment () const override { return nullptr; }
	IBufferSegment3D* CCL_API getIndexBufferSegment () const override { return nullptr; }
	tresult CCL_API getBoundingSphere (BoundingSphere3D& sphere) override;
	tresult CCL_API setCustomBoundingSphere (BoundingSphere3DRef sphere) override;

	CLASS_INTERFACE3 (IGeometrySource3D, IGeometry3D, IGraphicsResource3D, Object)

protected:
	BoundingSphere3D boundingSphere;
	bool boundingSphereCustom;
	bool boundingSphereDirty;

	virtual void recomputeBoundingSphere () = 0;
};

//************************************************************************************************
// Geometry3D
//************************************************************************************************

class Geometry3D: public BaseGeometry3D
{
public:
	DECLARE_CLASS (Geometry3D, BaseGeometry3D)

	Geometry3D ();

	// BaseGeometry3D
	tbool CCL_API isGPUAccessible () const override;
	tresult CCL_API upload (IBufferAllocator3D& allocator) override;
	void CCL_API discard () override;
	uint32 CCL_API getVertexCount () const override;
	uint32 CCL_API getIndexCount () const override;
	const PointF3D* CCL_API getPositions () const override;
	const PointF3D* CCL_API getNormals () const override;
	const PointF* CCL_API getTextureCoords () const override;
	const uint32* CCL_API getIndices () const override;
	tresult CCL_API setVertexData (const PointF3D positions[], const PointF3D normals[], const PointF textureCoords[], uint32 count) override;
	tresult CCL_API setIndices (const uint32 indices[], uint32 count) override;
	void CCL_API setPrimitiveTopology (PrimitiveTopology3D topology) override;
	PrimitiveTopology3D CCL_API getPrimitiveTopology () const override;
	IBufferSegment3D* CCL_API getVertexBufferSegment () const override;
	IBufferSegment3D* CCL_API getIndexBufferSegment () const override;

private:
	PrimitiveTopology3D topology;
	AutoPtr<IBufferSegment3D> vertexBuffer;
	AutoPtr<IBufferSegment3D> indexBuffer;
	Vector<PointF3D> positions;
	Vector<PointF3D> normals;
	Vector<PointF> textureCoords;
	Vector<uint32> indices;

	template<typename T> tresult uploadVertices (IBufferAllocator3D& allocator);
	template<typename T> void getVertex (T& vertex, int index) const;

	// BaseGeometry3D
	void recomputeBoundingSphere () override;
};

//************************************************************************************************
// Billboard3D
//************************************************************************************************

class Billboard3D: public BaseGeometry3D
{
public:
	DECLARE_CLASS (Billboard3D, BaseGeometry3D)

	Billboard3D ();

	// BaseGeometry3D
	tbool CCL_API isGPUAccessible () const override;
	tresult CCL_API upload (IBufferAllocator3D& allocator) override;
	uint32 CCL_API getVertexCount () const override { return kVertexCount; }
	PrimitiveTopology3D CCL_API getPrimitiveTopology () const override { return kPrimitiveTopologyTriangleStrip; }
	IBufferSegment3D* CCL_API getVertexBufferSegment () const override { return vertexBuffer; }

protected:
	static const int kVertexCount = 4;

	AutoPtr<IBufferSegment3D> vertexBuffer;

	// BaseGeometry3D
	void recomputeBoundingSphere () override;
};

//************************************************************************************************
// Model3D
//************************************************************************************************

class Model3D: public Object,
			   public IModel3D
{
public:
	DECLARE_CLASS (Model3D, Object)

	static Model3D* loadFromFile (UrlRef path);

	// IModel3D
	tresult CCL_API createInstance (UIDRef cid, UIDRef iid, void** obj) override;
	tresult CCL_API addGeometry (IGeometry3D* geometry, IMaterial3D* material = nullptr) override;
	int CCL_API getGeometryCount () const override;
	IGeometry3D* CCL_API getGeometryAt (int index) const override;
	tresult CCL_API setGeometryAt (int index, IGeometry3D* geometry) override;
	IMaterial3D* CCL_API getMaterialAt (int index) const override;
	tresult CCL_API setMaterialAt (int index, IMaterial3D* material) override;

	CLASS_INTERFACE2 (IModel3D, IClassAllocator, Object)

private:
	struct GeometryItem
	{
		AutoPtr<BaseGeometry3D> geometry;
		AutoPtr<IMaterial3D> material;
	};
	Vector<GeometryItem> geometries;
};

} // namespace CCL

#endif // _ccl_model3d_h
