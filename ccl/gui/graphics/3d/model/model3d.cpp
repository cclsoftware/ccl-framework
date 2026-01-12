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
// Filename    : ccl/gui/graphics/3d/model/model3d.cpp
// Description : 3D Model
//
//************************************************************************************************

#include "ccl/gui/graphics/3d/model/model3d.h"
#include "ccl/gui/graphics/3d/bufferallocator3d.h"
#include "ccl/gui/graphics/graphicshelper.h"

#include "ccl/base/singleton.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/gui/graphics/3d/stockshader3d.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// Model3DStatics
//************************************************************************************************

class Model3DStatics: public Object,
					  public Singleton<Model3DStatics>
{
public:
	Model3DStatics ();

	Model3D* loadFromFile (UrlRef path);

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	Vector<IModelImporter3D*> importerList;

	void loadImporter ();
	void unloadImporter ();
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Model3DStatics
//************************************************************************************************

DEFINE_SINGLETON (Model3DStatics)

//////////////////////////////////////////////////////////////////////////////////////////////////

Model3DStatics::Model3DStatics ()
{
	loadImporter ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Model3DStatics::loadImporter ()
{
	ForEachPlugInClass (PLUG_CATEGORY_MODELIMPORTER3D, description)
		if(auto importer = ccl_new<IModelImporter3D> (description.getClassID ()))
			importerList.add (importer);
	EndFor

	SignalSource::addObserver (Signals::kPlugIns, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Model3DStatics::unloadImporter ()
{
	for(auto importer : importerList)
		ccl_release (importer);
	importerList.removeAll ();

	SignalSource::removeObserver (Signals::kPlugIns, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Model3DStatics::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kTerminatePlugIns)
		unloadImporter ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Model3D* Model3DStatics::loadFromFile (UrlRef path)
{
	for(auto importer : importerList)
		if(importer->getFileType () == path.getFileType ())
		{
			AutoPtr<Model3D> model = NEW Model3D;
			tresult result = importer->importModel (*model, path);
			if(result == kResultOk)
				return model.detach ();
		}
	return nullptr;
}

//************************************************************************************************
// Material3D
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Material3D, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Material3D::Material3D ()
: depthBias (0.f),
  lightMask (kDefaultLightMask)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Material3D::getShaderParameters (IShaderValue3D& parameters) const
{
	parameters[ParamName3D::kLightMask] = Variant (lightMask);
}

//************************************************************************************************
// SolidColorMaterial3D
//************************************************************************************************

DEFINE_CLASS (SolidColorMaterial3D, Material3D)
DEFINE_CLASS_UID (SolidColorMaterial3D, 0x1c21f76f, 0xf4a, 0x489b, 0x81, 0x30, 0x12, 0x5d, 0x7a, 0x2a, 0xcd, 0xce)

//////////////////////////////////////////////////////////////////////////////////////////////////

SolidColorMaterial3D::SolidColorMaterial3D ()
: shininess (0.f)
{
	pixelShader = Native3DGraphicsFactory::instance ().createStockShader (IGraphicsShader3D::kPixelShader, StockShaders::kSolidColorMaterialShader);
	ASSERT (pixelShader.isValid ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SolidColorMaterial3D::setMaterialColor (ColorRef color) { materialColor = color; }
ColorRef CCL_API SolidColorMaterial3D::getMaterialColor () const { return materialColor; }
void CCL_API SolidColorMaterial3D::setShininess (float value) { shininess = value; }
float CCL_API SolidColorMaterial3D::getShininess () const { return shininess; }

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SolidColorMaterial3D::getShaderParameters (IShaderValue3D& parameters) const
{
	parameters[ParamName3D::kMaterialColor] = ColorF (materialColor);
	parameters[ParamName3D::kShininess] = Variant (shininess);
	SuperClass::getShaderParameters (parameters);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (SolidColorMaterial3D)
	DEFINE_PROPERTY_CLASS (SolidColorMaterial3D::kMaterialColor, "UIValue")
	DEFINE_PROPERTY_TYPE (SolidColorMaterial3D::kShininess, ITypeInfo::kFloat)
END_PROPERTY_NAMES (SolidColorMaterial3D)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SolidColorMaterial3D::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == kMaterialColor)
	{
		Color c;
		if(IUIValue* value = IUIValue::toValue (var))
			value->toColor (c);
		setMaterialColor (c);
		return true;
	}
	else if(propertyId == kShininess)
	{
		setShininess (var.asFloat ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SolidColorMaterial3D::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == kMaterialColor)
	{
		AutoPtr<UIValue> value = NEW UIValue;
		value->fromColor (getMaterialColor ());
		var.takeShared (value->asUnknown ());
		return true;
	}
	else if(propertyId == kShininess)
	{
		var = getShininess ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// TextureMaterial3D
//************************************************************************************************

DEFINE_CLASS (TextureMaterial3D, SolidColorMaterial3D)
DEFINE_CLASS_UID (TextureMaterial3D, 0x0e272e3c, 0x7917, 0x4b0f, 0x98, 0xb1, 0x61, 0x05, 0x61, 0xc2, 0x40, 0xa8)

//////////////////////////////////////////////////////////////////////////////////////////////////

TextureMaterial3D::TextureMaterial3D ()
: opacity (1.f)
{
	pixelShader = Native3DGraphicsFactory::instance ().createStockShader (IGraphicsShader3D::kPixelShader, StockShaders::kTextureMaterialShader);	
	ASSERT (pixelShader.isValid ())
	
	textures.setCount (Native3DShaderParameterSet::kMaxTextureCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsTexture2D* TextureMaterial3D::getGraphicsTexture (int textureIndex)
{
	if(textureIndex < 0 || textureIndex >= textures.count ())
		return nullptr;

	TextureItem& item = textures[textureIndex];
	if(item.needsUpdate)
	{
		if(!item.texture.isValid () || item.texture->copyFromBitmap (item.bitmap) != kResultOk)
			item.texture = Native3DGraphicsFactory::instance ().createTexture (item.bitmap, item.flags);
		item.needsUpdate = false;
	}
	return item.texture;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TextureMaterial3D::setTexture (int textureIndex, IBitmap* texture)
{
	if(textureIndex < 0 || textureIndex >= textures.count ())
		return kResultInvalidArgument;
	auto* textureBitmap = unknown_cast<Bitmap> (texture);
	if(textureBitmap == nullptr)
		return kResultInvalidArgument;
	textures[textureIndex].bitmap = textureBitmap;
	textures[textureIndex].needsUpdate = true;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmap* CCL_API TextureMaterial3D::getTexture (int textureIndex) const 
{
	if(textureIndex < 0 || textureIndex >= textures.count ())
		return nullptr;
	return textures[textureIndex].bitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TextureMaterial3D::setTextureFlags (int textureIndex, TextureFlags3D flags)
{
	if(textureIndex < 0 || textureIndex >= textures.count ())
		return kResultInvalidArgument;
	textures[textureIndex].flags = flags;
	textures[textureIndex].needsUpdate = true;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextureFlags3D CCL_API TextureMaterial3D::getTextureFlags (int textureIndex) const
{
	if(textureIndex < 0 || textureIndex >= textures.count ())
		return 0;
	return textures[textureIndex].flags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextureMaterial3D::getShaderParameters (IShaderValue3D& parameters) const
{
	parameters[ParamName3D::kOpacity] = Variant (opacity);
	SuperClass::getShaderParameters (parameters);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsContentHint CCL_API TextureMaterial3D::getMaterialHint () const
{
	if(opacity < 1.f)
		return kGraphicsContentTranslucent;
	return SuperClass::getMaterialHint ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextureMaterial3D::requiresTextureCoordinates () const
{
	for(const TextureItem& item : textures)
	{
		if(item.bitmap != nullptr)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (TextureMaterial3D)
	DEFINE_PROPERTY_TYPE (TextureMaterial3D::kOpacity, ITypeInfo::kFloat)
END_PROPERTY_NAMES (TextureMaterial3D)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextureMaterial3D::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == kOpacity)
	{
		setOpacity (var.asFloat ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextureMaterial3D::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == kOpacity)
	{
		var = getOpacity ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// CustomMaterial3D
//************************************************************************************************

DEFINE_CLASS (CustomMaterial3D, TextureMaterial3D)
DEFINE_CLASS_UID (CustomMaterial3D, 0xc3648bad, 0xca80, 0x4c06, 0xa3, 0xfa, 0x4e, 0x04, 0x25, 0xb1, 0xcd, 0xcf)

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CustomMaterial3D::getShaderParameters (IShaderValue3D& parameters) const
{
	SuperClass::getShaderParameters (parameters);
	if(provider.isValid ())
		provider->getShaderParameters (parameters); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsContentHint CCL_API CustomMaterial3D::getMaterialHint () const
{
	if(customHint != kHintNotSet)
		return customHint;
	return SuperClass::getMaterialHint (); 
}

//************************************************************************************************
// BaseGeometry3D
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (BaseGeometry3D, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

BaseGeometry3D::BaseGeometry3D ()
: boundingSphereCustom (false),
  boundingSphereDirty (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BaseGeometry3D::getBoundingSphere (BoundingSphere3D& sphere)
{
	if(!boundingSphereCustom && boundingSphereDirty)
	{
		recomputeBoundingSphere ();
		boundingSphereDirty = false;
	}

	if(boundingSphere.isValid ())
	{
		sphere = boundingSphere;
		return kResultOk;
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BaseGeometry3D::setCustomBoundingSphere (BoundingSphere3DRef sphere)
{
	if(sphere.isValid ())
	{
		boundingSphere = sphere;
		boundingSphereCustom = true;
		return kResultOk;
	}
	return kResultInvalidArgument;
}

//************************************************************************************************
// Geometry3D
//************************************************************************************************

DEFINE_CLASS (Geometry3D, BaseGeometry3D)
DEFINE_CLASS_UID (Geometry3D, 0x3237c2db, 0x80e1, 0x437f, 0x89, 0xdb, 0x48, 0x11, 0xa6, 0xdd, 0xfb, 0x14)

//////////////////////////////////////////////////////////////////////////////////////////////////

Geometry3D::Geometry3D ()
: topology (kPrimitiveTopologyTriangleList)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Geometry3D::isGPUAccessible () const
{
	return vertexBuffer.isValid () && indexBuffer.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Geometry3D::upload (IBufferAllocator3D& allocator)
{
	uint32 vertexCount = positions.count ();
	bool useNormals = !normals.isEmpty ();
	ASSERT (useNormals == false || normals.count () == vertexCount)
	bool useTextureCoords = !textureCoords.isEmpty ();
	ASSERT (useTextureCoords == false || textureCoords.count () == vertexCount)
	if(vertexCount > 0)
	{
		tresult result = kResultOk;
		if(useTextureCoords)
			result = uploadVertices<VertexPNT> (allocator);
		else if(useNormals)
			result = uploadVertices<VertexPN> (allocator);
		else
			result = uploadVertices<VertexP> (allocator);
		if(result != kResultOk)
			return result;
	}

	uint32 indexCount = indices.count ();
	if(indexCount > 0)
	{
		indexBuffer = allocator.allocateBuffer (IGraphicsBuffer3D::kIndexBuffer, kBufferUsageDynamic, indexCount, sizeof(uint16));
		if(!indexBuffer.isValid ())
			return kResultOutOfMemory;

		MappedBuffer3D<uint16> buffer (*indexBuffer);
		if(!buffer.isValid ())
			return kResultFailed;

		for(uint32 i = 0; i < indexCount; i++)
			buffer[i] = uint16(indices[i]);
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
tresult Geometry3D::uploadVertices (IBufferAllocator3D& allocator)
{
	uint32 vertexCount = positions.count ();
	vertexBuffer = allocator.allocateBuffer (IGraphicsBuffer3D::kVertexBuffer, kBufferUsageDynamic, vertexCount, sizeof(T));
	if(!vertexBuffer.isValid ())
		return kResultOutOfMemory;

	MappedBuffer3D<T> buffer (*vertexBuffer);
	if(!buffer.isValid ())
		return kResultFailed;
	
	for(uint32 i = 0; i < vertexCount; i++)
		getVertex<T> (buffer[i], i);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
void Geometry3D::getVertex<VertexP> (VertexP& vertex, int index) const
{
	vertex.position = positions[index];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
void Geometry3D::getVertex<VertexPN> (VertexPN& vertex, int index) const
{
	vertex.position = positions[index];
	vertex.normal = normals[index];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
void Geometry3D::getVertex<VertexPNT> (VertexPNT& vertex, int index) const
{
	vertex.position = positions[index];
	vertex.normal = normals[index];
	vertex.textureCoordinate = textureCoords[index];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Geometry3D::discard ()
{
	vertexBuffer.release ();
	indexBuffer.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API Geometry3D::getVertexCount () const
{
	return positions.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API Geometry3D::getIndexCount () const
{
	return indices.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Geometry3D::setVertexData (const PointF3D positions[], const PointF3D normals[],
										   const PointF textureCoords[], uint32 count)
{
	if(positions != nullptr)
		this->positions.copyVector (positions, count);
	else
		return kResultInvalidArgument;

	if(normals != nullptr)
		this->normals.copyVector (normals, count);
	else
		this->normals.empty ();

	if(textureCoords != nullptr)
		this->textureCoords.copyVector (textureCoords, count);
	else
		this->textureCoords.empty ();

	boundingSphereDirty = true;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PointF3D* CCL_API Geometry3D::getPositions () const
{
	return positions.getItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PointF3D* CCL_API Geometry3D::getNormals () const
{
	return normals.getItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PointF* CCL_API Geometry3D::getTextureCoords () const
{
	return textureCoords.isEmpty () ? nullptr : textureCoords.getItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Geometry3D::setIndices (const uint32 indices[], uint32 count)
{
	this->indices.copyVector (indices, count);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const uint32* CCL_API Geometry3D::getIndices () const
{
	return indices.getItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PrimitiveTopology3D CCL_API Geometry3D::getPrimitiveTopology () const
{
	return topology;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Geometry3D::setPrimitiveTopology (PrimitiveTopology3D topology)
{
	this->topology = topology;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBufferSegment3D* CCL_API Geometry3D::getVertexBufferSegment () const
{
	return vertexBuffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBufferSegment3D* CCL_API Geometry3D::getIndexBufferSegment () const
{
	return indexBuffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Geometry3D::recomputeBoundingSphere ()
{
	boundingSphere.origin = {};
	boundingSphere.radius = 0.f;

	if(positions.count () < 2)
		return;

	// Ritter's bounding sphere algorithm V2
	// Source: https://www.researchgate.net/publication/242453691_An_Efficient_Bounding_Sphere

	// For each dimension, find the pair of points with the maximum span in that dimension
	const PointF3D* minXPoint = nullptr;
	const PointF3D* minYPoint = nullptr;
	const PointF3D* minZPoint = nullptr;
	const PointF3D* maxXPoint = nullptr;
	const PointF3D* maxYPoint = nullptr;
	const PointF3D* maxZPoint = nullptr;
	{
		float minX = NumericLimits::kMaximumFloat;
		float minY = NumericLimits::kMaximumFloat;
		float minZ = NumericLimits::kMaximumFloat;
		float maxX = -NumericLimits::kMaximumFloat;
		float maxY = -NumericLimits::kMaximumFloat;
		float maxZ = -NumericLimits::kMaximumFloat;
		for(PointF3DRef point : positions)
		{
			if(point.x < minX)
			{
				minX = point.x;
				minXPoint = &point;
			}
			if(point.y < minY)
			{
				minY = point.y;
				minYPoint = &point;
			}
			if(point.z < minZ)
			{
				minZ = point.z;
				minZPoint = &point;
			}
			if(point.x > maxX)
			{
				maxX = point.x;
				maxXPoint = &point;
			}
			if(point.y > maxY)
			{
				maxY = point.y;
				maxYPoint = &point;
			}
			if(point.z > maxZ)
			{
				maxZ = point.z;
				maxZPoint = &point;
			}
		}
	}
	if(!minXPoint || !minYPoint || !minZPoint || !maxXPoint || !maxYPoint || !maxZPoint)
	{
		Debugger::printf ("Error: %s: geometry contains points at infinity\n", __func__);
		return;
	}

	// Pick the pair with the maximum point-to-point separation
	const PointF3D* minPoint = minZPoint;
	const PointF3D* maxPoint = maxZPoint;
	const float xSquared = minXPoint->distanceToSquared (*maxXPoint);
	const float ySquared = minYPoint->distanceToSquared (*maxYPoint);
	const float zSquared = minZPoint->distanceToSquared (*maxZPoint);
	if(xSquared > ySquared && xSquared > zSquared)
	{
		minPoint = minXPoint;
		maxPoint = maxXPoint;
	}
	else if(ySquared > zSquared)
	{
		minPoint = minYPoint;
		maxPoint = maxYPoint;
	}
	PointF3D origin = *minPoint + (*maxPoint - *minPoint) * 0.5f;
	float radiusSquared = minPoint->distanceToSquared (origin);
	float radius = sqrtf (radiusSquared);

	// check if all points are inside the sphere and enlarge the sphere if necessary
	for(PointF3DRef point : positions)
	{
		const float distanceSquared = point.distanceToSquared (origin);
		if(distanceSquared > radiusSquared)
		{
			const float distance = sqrtf (distanceSquared);
			const float newRadius = 0.5f * (radius + distance);
			origin += (point - origin) * ((newRadius - radius) / distance);
			radius = newRadius;
			radiusSquared = radius * radius;
		}
	}

	boundingSphere.origin = origin;
	boundingSphere.radius = radius;
}

//************************************************************************************************
// Billboard3D
//************************************************************************************************

DEFINE_CLASS (Billboard3D, BaseGeometry3D)
DEFINE_CLASS_UID (Billboard3D, 0xf14a69e8, 0x9864, 0x4cc8, 0x9b, 0xbd, 0x45, 0x5a, 0xb1, 0xe4, 0xd7, 0xfa)

//////////////////////////////////////////////////////////////////////////////////////////////////

Billboard3D::Billboard3D ()
{
	boundingSphereDirty = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Billboard3D::isGPUAccessible () const
{
	return vertexBuffer.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Billboard3D::upload (IBufferAllocator3D& allocator)
{
	vertexBuffer = allocator.allocateBuffer (IGraphicsBuffer3D::kVertexBuffer, kBufferUsageDynamic, kVertexCount, sizeof(VertexPT));
	if(!vertexBuffer.isValid ())
		return kResultOutOfMemory;

	MappedBuffer3D<VertexPT> buffer (*vertexBuffer);
	if(!buffer.isValid ())
		return kResultFailed;
	
	buffer[0].position = {-.5f, -.5f, 0.f};
	buffer[1].position = {.5f, -.5f, 0.f};
	buffer[2].position = {-.5f, .5f, 0.f};
	buffer[3].position = {.5f, .5f, 0.f};
	
	buffer[0].textureCoordinate = {0, 1};
	buffer[1].textureCoordinate = {1, 1};
	buffer[2].textureCoordinate = {0, 0};
	buffer[3].textureCoordinate = {1, 0};

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Billboard3D::recomputeBoundingSphere ()
{
	boundingSphere.origin = { 0, 0, 0 };
	boundingSphere.radius = Math::Constants<float>::kSqrtTwo / 2.f;
}

//************************************************************************************************
// Model3D
//************************************************************************************************

Model3D* Model3D::loadFromFile (UrlRef path)
{
	return Model3DStatics::instance ().loadFromFile (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (Model3D, Object)
DEFINE_CLASS_UID (Model3D, 0x6999877c, 0x645b, 0x4bac, 0xa7, 0x20, 0xce, 0x17, 0x1d, 0xcc, 0x63, 0x74)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Model3D::createInstance (UIDRef cid, UIDRef iid, void** obj)
{
	AutoPtr<Object> instance;

	if(cid == ClassID::Geometry3D)
		instance = NEW Geometry3D;
	if(cid == ClassID::Billboard3D)
		instance = NEW Billboard3D;
	else if(cid == ClassID::SolidColorMaterial3D)
		instance = NEW SolidColorMaterial3D;
	else if(cid == ClassID::TextureMaterial3D)
		instance = NEW TextureMaterial3D;
	
	if(instance)
		return instance->queryInterface (iid, obj);
	else
	{
		*obj = nullptr;
		return kResultClassNotFound;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Model3D::addGeometry (IGeometry3D* _geometry, IMaterial3D* material)
{
	auto* geometry = unknown_cast<BaseGeometry3D> (_geometry);
	if(geometry == nullptr)
		return kResultInvalidArgument;

	geometries.add ({ geometry, material });
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Model3D::getGeometryCount () const
{
	return geometries.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGeometry3D* CCL_API Model3D::getGeometryAt (int index) const
{
	return geometries.at (index).geometry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Model3D::setGeometryAt (int index, IGeometry3D* _geometry)
{
	if(index < 0 || index >= geometries.count ())
		return kResultInvalidArgument;
	auto* geometry = unknown_cast<BaseGeometry3D> (_geometry);
	if(geometry == nullptr)
		return kResultInvalidArgument;
	geometries[index].geometry = return_shared (geometry);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMaterial3D* CCL_API Model3D::getMaterialAt (int index) const
{
	return geometries.at (index).material;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Model3D::setMaterialAt (int index, IMaterial3D* material)
{
	if(index < 0 || index >= geometries.count ())
		return kResultInvalidArgument;
	geometries[index].material = return_shared (material);
	return kResultOk;
}
