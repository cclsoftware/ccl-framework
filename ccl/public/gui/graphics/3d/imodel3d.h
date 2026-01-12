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
// Filename    : ccl/public/gui/graphics/3d/imodel3d.h
// Description : 3D Model Interfaces
//
//************************************************************************************************

#ifndef _ccl_imodel3d_h
#define _ccl_imodel3d_h

#include "ccl/public/gui/graphics/3d/igraphics3d.h"
#include "ccl/public/gui/graphics/3d/igeometrysource3d.h"

namespace CCL {

class FileType;

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Class category for 3D model importer. */
#define PLUG_CATEGORY_MODELIMPORTER3D CCLSTR ("ModelImporter3D")

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** 3D solid color material [ISolidColorMaterial3D] */
	DEFINE_CID (SolidColorMaterial3D, 0x1c21f76f, 0xf4a, 0x489b, 0x81, 0x30, 0x12, 0x5d, 0x7a, 0x2a, 0xcd, 0xce)

	/** 3D texture material [ITextureMaterial3D] */
	DEFINE_CID (TextureMaterial3D, 0x0e272e3c, 0x7917, 0x4b0f, 0x98, 0xb1, 0x61, 0x05, 0x61, 0xc2, 0x40, 0xa8)

	/** 3D custom material [ICustomMaterial3D, ITextureMaterial3D] */
	DEFINE_CID (CustomMaterial3D, 0xc3648bad, 0xca80, 0x4c06, 0xa3, 0xfa, 0x4e, 0x04, 0x25, 0xb1, 0xcd, 0xcf)

	/** 3D geometry [IGeometry3D] */
	DEFINE_CID (Geometry3D, 0x3237c2db, 0x80e1, 0x437f, 0x89, 0xdb, 0x48, 0x11, 0xa6, 0xdd, 0xfb, 0x14)

	/** 3D billboard (sprite) [IGeometry3D] */
	DEFINE_CID (Billboard3D, 0xf14a69e8, 0x9864, 0x4cc8, 0x9b, 0xbd, 0x45, 0x5a, 0xb1, 0xe4, 0xd7, 0xfa)

	/** 3D model [IModel3D] */
	DEFINE_CID (Model3D, 0x6999877c, 0x645b, 0x4bac, 0xa7, 0x20, 0xce, 0x17, 0x1d, 0xcc, 0x63, 0x74)
}

//************************************************************************************************
// IShaderParameterProvider3D
/** 3D shader parameter provider interface.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IShaderParameterProvider3D: IUnknown
{	
	virtual void CCL_API getShaderParameters (IShaderValue3D& parameters) const = 0;
		
	DECLARE_IID (IShaderParameterProvider3D)
};

DEFINE_IID (IShaderParameterProvider3D, 0x388f9fcc, 0xa62d, 0x4a74, 0xbe, 0x36, 0x2b, 0xc6, 0x58, 0x61, 0x9a, 0x8a)

//************************************************************************************************
// IMaterial3D
/** 3D material interface.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IMaterial3D: IShaderParameterProvider3D
{
	/** Get material hint (empty, opaque, translucent). */
	virtual GraphicsContentHint CCL_API getMaterialHint () const = 0;

	/** Get the pixel shader that is used to render this material. */
	virtual IGraphicsShader3D* CCL_API getPixelShader () const = 0;

	/** Set the depth bias. For geometries that are rendered at the same position, this bias determines which material is rendered first. */
	virtual void CCL_API setDepthBias (float bias) = 0;

	/** Get the depth bias. */
	virtual float CCL_API getDepthBias () const = 0;

	/** Set the light mask. This controls which light sources affect this material. */
	virtual void CCL_API setLightMask (uint32 mask) = 0;
	
	/** Get the light mask. */
	virtual uint32 CCL_API getLightMask () const = 0;

	DECLARE_IID (IMaterial3D)
};

DEFINE_IID (IMaterial3D, 0xe7c6692f, 0xe5b, 0x4e79, 0x91, 0xf9, 0x18, 0x3d, 0x3f, 0xb8, 0x16, 0x8f)

//************************************************************************************************
// ISolidColorMaterial3D
/** 3D solid color material.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface ISolidColorMaterial3D: IMaterial3D
{
	virtual void CCL_API setMaterialColor (ColorRef color) = 0;

	virtual ColorRef CCL_API getMaterialColor () const = 0;

	virtual void CCL_API setShininess (float shininess) = 0;

	virtual float CCL_API getShininess () const = 0;

	// Property identifiers
	DECLARE_STRINGID_MEMBER (kMaterialColor)	///< IUIValue (color)
	DECLARE_STRINGID_MEMBER (kShininess)		///< float

	DECLARE_IID (ISolidColorMaterial3D)
};

DEFINE_IID (ISolidColorMaterial3D, 0x74a63707, 0x85da, 0x447f, 0xa1, 0x9e, 0x43, 0x2d, 0xe3, 0x82, 0xd4, 0x14)
DEFINE_STRINGID_MEMBER (ISolidColorMaterial3D, kMaterialColor, "materialColor")
DEFINE_STRINGID_MEMBER (ISolidColorMaterial3D, kShininess, "shininess")

//************************************************************************************************
// ITextureMaterial3D
/** 3D texture material interface.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface ITextureMaterial3D: IMaterial3D
{
	/** Set texture (shared). */
	virtual tresult CCL_API setTexture (int textureIndex, IBitmap* texture) = 0;

	/** Get texture. */
	virtual IBitmap* CCL_API getTexture (int textureIndex) const = 0;

	/** Set texture flags. */
	virtual tresult CCL_API setTextureFlags (int textureIndex, TextureFlags3D flags) = 0;

	/** Get texture flags. */
	virtual TextureFlags3D CCL_API getTextureFlags (int textureIndex) const = 0;

	/** Set opacity. */
	virtual void CCL_API setOpacity (float opacity) = 0;

	/** Get opacity. */
	virtual float CCL_API getOpacity () const = 0;

	// Property identifiers
	DECLARE_STRINGID_MEMBER (kOpacity)	///< float

	DECLARE_IID (ITextureMaterial3D)
};

DEFINE_IID (ITextureMaterial3D, 0xcbf9553c, 0x4e20, 0x44a5, 0xaa, 0x60, 0x61, 0x53, 0x13, 0xbe, 0xea, 0x1d)
DEFINE_STRINGID_MEMBER (ITextureMaterial3D, kOpacity, "opacity")

//************************************************************************************************
// ICustomMaterial3D
/** 3D custom material interface.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface ICustomMaterial3D: IMaterial3D
{
	/** Set parameter provider (shared). */
	virtual void CCL_API setShaderParameterProvider (IShaderParameterProvider3D* provider) = 0;
	
	/** Set pixel shader (shared). */
	virtual void CCL_API setPixelShader (IGraphicsShader3D* shader) = 0;
	
	/** Set material hint. */
	virtual void CCL_API setMaterialHint (GraphicsContentHint hint) = 0;

	DECLARE_IID (IMaterial3D)
};
							   
DEFINE_IID (ICustomMaterial3D, 0x7cb60076, 0x7ea8, 0x4a42, 0x85, 0x55, 0x12, 0xe7, 0x86, 0x06, 0xf6, 0x1e)

//************************************************************************************************
// BoundingSphere3D
/** 3D bounding sphere.
	\ingroup gui_graphics3d */
//************************************************************************************************

struct BoundingSphere3D
{
	PointF3D origin;
	float radius = 0;

	bool isValid () const
	{
		return radius > 0;
	}
};

/** Bounding sphere reference type */
typedef const BoundingSphere3D& BoundingSphere3DRef;

//************************************************************************************************
// IGeometry3D
/** 3D geometry interface.
	Classes implementing this interface also implement IGraphicsResource3D.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IGeometry3D: IGeometrySource3D
{
	/** Set vertex data */
	virtual tresult CCL_API setVertexData (const PointF3D positions[], const PointF3D normals[],
										   const PointF textureCoords[], uint32 count) = 0;

	/** Set indices */
	virtual tresult CCL_API setIndices (const uint32 indices[], uint32 count) = 0;

	/** Set primitive type */
	virtual void CCL_API setPrimitiveTopology (PrimitiveTopology3D topology) = 0;

	/** Get primitive type */
	virtual PrimitiveTopology3D CCL_API getPrimitiveTopology () const = 0;

	/** Get vertex buffer segment, valid when uploaded to graphics memory. */
	virtual IBufferSegment3D* CCL_API getVertexBufferSegment () const = 0;

	/** Get index buffer, valid when uploaded to graphics memory. */
	virtual IBufferSegment3D* CCL_API getIndexBufferSegment () const = 0;

	/** Get a sphere encasing all vertices of this geometry. */
	virtual tresult CCL_API getBoundingSphere (BoundingSphere3D& sphere) = 0;

	/** Set a custom bounding sphere. */
	virtual tresult CCL_API setCustomBoundingSphere (BoundingSphere3DRef sphere) = 0;

	DECLARE_IID (IGeometry3D)

	//////////////////////////////////////////////////////////////////////////////////////////////

	void copyFrom (const IGeometrySource3D& source);
};

DEFINE_IID (IGeometry3D, 0x63117c05, 0x5b75, 0x4a62, 0xbb, 0x2f, 0xf3, 0x99, 0x9e, 0x1a, 0x65, 0x4f)

//************************************************************************************************
// IModel3D
/**	3D model interface.
	Models defined in Skin XML can be accessed by name via ITheme::getResource().
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IModel3D: IClassAllocator
{
	/** Add geometry to model (takes ownership). */
	virtual tresult CCL_API addGeometry (IGeometry3D* geometry, IMaterial3D* material = nullptr) = 0;

	/** Get number of geometries. */
	virtual int CCL_API getGeometryCount () const = 0;

	/** Get geometry at given index. */
	virtual IGeometry3D* CCL_API getGeometryAt (int index) const = 0;

	/** Assign a material to the geometry at given index (shared). */
	virtual tresult CCL_API setGeometryAt (int index, IGeometry3D* geometry) = 0;

	/** Get material assigned to geometry at given index. */
	virtual IMaterial3D* CCL_API getMaterialAt (int index) const = 0;
	
	/** Assign a material to the geometry at given index (shared). */
	virtual tresult CCL_API setMaterialAt (int index, IMaterial3D* material) = 0;

	DECLARE_IID (IModel3D)

	//////////////////////////////////////////////////////////////////////////////////////////////

	IGeometry3D* createGeometry ();
	IGeometry3D* createBillboard ();
	ISolidColorMaterial3D* createSolidColorMaterial ();
	ITextureMaterial3D* createTextureMaterial ();

	IMaterial3D* getFirstMaterial () const;
	void setMaterialForGeometries (IMaterial3D* material);

private:
	template <typename IFace, UIDRef cid> IFace* create ();
};

DEFINE_IID (IModel3D, 0x609d444b, 0x97e2, 0x4705, 0x86, 0xec, 0x8c, 0x5f, 0x18, 0x11, 0x80, 0x2e)

//************************************************************************************************
// IModelImporter3D
/** 3D model importer interface.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IModelImporter3D: IUnknown
{
	/** Get model file type. */
	virtual const FileType& CCL_API getFileType () const = 0;

	/** Import model from file. */
	virtual tresult CCL_API importModel (IModel3D& model, UrlRef path) = 0;

	DECLARE_IID (IModelImporter3D)
};

DEFINE_IID (IModelImporter3D, 0xabea63de, 0xede7, 0x4e3e, 0x84, 0x56, 0x85, 0x41, 0x2a, 0x89, 0x57, 0xb1)

//////////////////////////////////////////////////////////////////////////////////////////////////
// IGeometry3D
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void IGeometry3D::copyFrom (const IGeometrySource3D& source)
{
	setVertexData (source.getPositions (), source.getNormals (), source.getTextureCoords (), source.getVertexCount ());
	setIndices (source.getIndices (), source.getIndexCount ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IModel3D
//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename IFace, UIDRef cid>
IFace* IModel3D::create ()
{
	AutoPtr<IFace> instance;
	createInstance (cid, ccl_iid<IFace> (), instance.as_ppv ());
	return instance.detach ();
}

inline IGeometry3D* IModel3D::createGeometry ()
{ return create<IGeometry3D, ClassID::Geometry3D> (); }

inline IGeometry3D* IModel3D::createBillboard ()
{ return create<IGeometry3D, ClassID::Billboard3D> (); }

inline ISolidColorMaterial3D* IModel3D::createSolidColorMaterial ()
{ return create<ISolidColorMaterial3D, ClassID::SolidColorMaterial3D> (); }

inline ITextureMaterial3D* IModel3D::createTextureMaterial ()
{ return create<ITextureMaterial3D, ClassID::TextureMaterial3D> (); }

inline IMaterial3D* IModel3D::getFirstMaterial () const
{
	return getMaterialAt (0);
}

inline void IModel3D::setMaterialForGeometries (IMaterial3D* material)
{
	for(int i = 0, count = getGeometryCount (); i < count; i++)
		setMaterialAt (i, material);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_imodel3d_h
