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
// Filename    : ccl/gui/skin/skinelements3d.cpp
// Description : 3D Skin Elements
//
//************************************************************************************************

#include "ccl/gui/skin/skinelements3d.h"
#include "ccl/gui/skin/skinattributes.h"

#include "ccl/gui/graphics/3d/model/model3d.h"
#include "ccl/gui/graphics/3d/scene/scene3d.h"
#include "ccl/gui/views/view3d.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/framework/skinxmldefs.h"
#include "ccl/public/gui/graphics/3d/shaderconstants3d.h"

namespace CCL {
namespace SkinElements {

//************************************************************************************************
// SceneResource3D
//************************************************************************************************

class SceneResource3D: public Object,
					   public ISceneResource3D
{
public:
	DECLARE_CLASS_ABSTRACT (SceneResource3D, Object)

	SceneResource3D (const Scene3DElement& sceneElement);

	// ISceneResource3D
	IScene3D* CCL_API createScene () override;

	CLASS_INTERFACE (ISceneResource3D, Object)

protected:
	const Scene3DElement& sceneElement;

	void addSceneNodesRecursive (ContainerNode3D& parentNode, const Element& parentElement);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void linkSkinElements3D () {} // force linkage of this file

} // namespace SkinElements
} // namespace CCL

using namespace CCL;
using namespace SkinElements;

//************************************************************************************************
// SceneResource3D
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (SceneResource3D, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneResource3D::SceneResource3D (const Scene3DElement& sceneElement)
: sceneElement (sceneElement)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IScene3D* CCL_API SceneResource3D::createScene ()
{
	auto* scene = NEW Scene3D;
	scene->setName (sceneElement.getName ());	
	SceneEdit3D scope (scene);
	addSceneNodesRecursive (*scene, sceneElement);
	return scene;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneResource3D::addSceneNodesRecursive (ContainerNode3D& parentNode, const Element& parentElement)
{
	ArrayForEach (parentElement, Element, e)
		if(auto* sceneElement = ccl_cast<SceneNode3DElement> (e))
			if(auto* sceneNode = sceneElement->createSceneNode ())
			{
				sceneElement->applyNodeAttributes (sceneNode);
				parentNode.addNode (sceneNode);

				if(auto* childContainer = ccl_cast<ContainerNode3D> (sceneNode))
					addSceneNodesRecursive (*childContainer, *e);
			}
	EndFor
}

//************************************************************************************************
// Model3DElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (Model3DElement, ResourceObjectElement, TAG_MODEL3D, DOC_GROUP_3D, 0)

BEGIN_SKIN_ELEMENT_ATTRIBUTES (Model3DElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_RESOURCES)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_MODEL3DCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (Model3DElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Model3DElement::loadObject (SkinModel& model)
{
	if(!object)
	{
		Url modelUrl;
		makeSkinUrl (modelUrl, url);
		
		auto* model = Model3D::loadFromFile (modelUrl);
		if(model)
		{
			// apply material (optional)
			if(auto* materialElement = findElement<Material3DElement> ())
				if(AutoPtr<Material3D> material = materialElement->createMaterial ())
				{
					materialElement->applyMaterialAttributes (material);
					model->setMaterialForGeometries (material);
				}
		}

		object = model;
	}
	return true;
}

//************************************************************************************************
// Material3DElement
//************************************************************************************************

BEGIN_STYLEDEF (Material3DElement::lightMaskFlags)
	{TAG_AMBIENTLIGHT3D, CCL_3D_SHADER_AMBIENTLIGHT_BIT},
	{TAG_DIRECTIONALIGHT3D, CCL_3D_SHADER_DIRECTIONALLIGHT_BIT},
	{TAG_POINTLIGHT3D "_0", CCL_3D_SHADER_POINTLIGHT_BIT(0)},	// must match CCL_3D_SHADER_MAX_POINTLIGHT_COUNT
	{TAG_POINTLIGHT3D "_1", CCL_3D_SHADER_POINTLIGHT_BIT(1)},
	{TAG_POINTLIGHT3D "_2", CCL_3D_SHADER_POINTLIGHT_BIT(2)},
	{TAG_POINTLIGHT3D "_3", CCL_3D_SHADER_POINTLIGHT_BIT(3)},
	{TAG_POINTLIGHT3D "_4", CCL_3D_SHADER_POINTLIGHT_BIT(4)},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (Material3DElement, Element, TAG_MATERIAL3D, DOC_GROUP_3D, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_DEPTHBIAS, TYPE_FLOAT) ///< depth bias
	ADD_SKIN_ELEMENT_MEMBER (ATTR_LIGHTMASK, TYPE_ENUM)	///< light mask
END_SKIN_ELEMENT_WITH_MEMBERS (Material3DElement)
DEFINE_SKIN_ENUMERATION (TAG_MATERIAL3D, ATTR_LIGHTMASK, Material3DElement::lightMaskFlags)

BEGIN_SKIN_ELEMENT_ATTRIBUTES (Material3DElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_MODEL3DCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (Material3DElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

Material3DElement::Material3DElement ()
: depthBias (0.f),
  lightMask (Material3D::kDefaultLightMask)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Material3DElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);	
	depthBias = a.getFloat (ATTR_DEPTHBIAS);
	lightMask = uint32(a.getOptions (ATTR_LIGHTMASK, lightMaskFlags, false, int(Material3D::kDefaultLightMask)));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Material3DElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setFloat (ATTR_DEPTHBIAS, depthBias);
	if(lightMask != Material3D::kDefaultLightMask)
		a.setOptions (ATTR_LIGHTMASK, StyleFlags (0, int(lightMask)), lightMaskFlags);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Material3DElement::applyMaterialAttributes (Material3D* material) const
{
	if(material)
	{
		material->setDepthBias (depthBias);
		material->setLightMask (lightMask);
	}
}

//************************************************************************************************
// SolidColorMaterial3DElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (SolidColorMaterial3DElement, Material3DElement, TAG_SOLIDCOLORMATERIAL3D, DOC_GROUP_3D, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_COLOR, TYPE_COLOR) ///< material color
	ADD_SKIN_ELEMENT_MEMBER (ATTR_SHININESS, TYPE_FLOAT) ///< material shininess
END_SKIN_ELEMENT_WITH_MEMBERS (SolidColorMaterial3DElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

SolidColorMaterial3DElement::SolidColorMaterial3DElement ()
: shininess (0.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SolidColorMaterial3DElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);	
	colorString = a.getCString (ATTR_COLOR);
	shininess = a.getFloat (ATTR_SHININESS);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SolidColorMaterial3DElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setString (ATTR_COLOR, colorString);
	a.setFloat (ATTR_SHININESS, shininess);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material3D* SolidColorMaterial3DElement::createMaterial () const
{
	return NEW SolidColorMaterial3D;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SolidColorMaterial3DElement::applyMaterialAttributes (Material3D* material) const
{
	SuperClass::applyMaterialAttributes (material);

	if(auto* solidColorMaterial = ccl_cast<SolidColorMaterial3D> (material))
	{
		ColorValueReference reference;
		SkinModel::getColorFromString (reference, colorString, this);
	
		ASSERT (reference.scheme == nullptr)
		if(reference.scheme != nullptr)
			SKIN_WARNING (this, "Color scheme references not supported in 3D material!", 0)

		solidColorMaterial->setMaterialColor (reference.colorValue);
		solidColorMaterial->setShininess (shininess);
	}
}

//************************************************************************************************
// TextureMaterial3DElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (TextureMaterial3DElement, Material3DElement, TAG_TEXTUREMATERIAL3D, DOC_GROUP_3D, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_OPACITY, TYPE_FLOAT) ///< texture opacity
END_SKIN_ELEMENT_WITH_MEMBERS (TextureMaterial3DElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

TextureMaterial3DElement::TextureMaterial3DElement ()
: opacity (1.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextureMaterial3DElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);
	opacity = a.getFloat (ATTR_OPACITY);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextureMaterial3DElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setFloat (ATTR_OPACITY, opacity);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material3D* TextureMaterial3DElement::createMaterial () const
{
	return NEW TextureMaterial3D;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextureMaterial3DElement::applyMaterialAttributes (Material3D* material) const
{
	SuperClass::applyMaterialAttributes (material);

	if(auto* textureMaterial = ccl_cast<TextureMaterial3D> (material))
	{
		int textureIndex = 0;
		ForEach (*this, Element, e)
			if(auto* textureElement = ccl_cast<TextureMaterial3DTextureElement> (e))
			{
				if(textureIndex >= Native3DShaderParameterSet::kMaxTextureCount)
				{
					SKIN_WARNING (this, "Too many textures in 3D material.", 0)
					break;
				}

				CString imageName (textureElement->getName ());
				ASSERT (!imageName.isEmpty ())
				auto* skinModel = SkinModel::getModel (this);
				Image* image = skinModel ? skinModel->getImage (imageName, const_cast<TextureMaterial3DElement*> (this)) : nullptr;
				if(image)
				{
					AutoPtr<Bitmap> bitmap;
					bitmap.share (ccl_cast<Bitmap> (image));				
					if(!bitmap && image) // must convert to bitmap
					{
						float scaleFactor = Bitmap::getDefaultContentScaleFactor ();
						bitmap = NEW Bitmap (image->getWidth (), image->getHeight (), Bitmap::kRGBAlpha, scaleFactor);
						
						BitmapGraphicsDevice device (bitmap);
						ImageMode mode (ImageMode::kInterpolationHighQuality);
						device.drawImage (image, Point (), &mode);
					}
				
					textureMaterial->setTexture (textureIndex, bitmap);
					textureMaterial->setTextureFlags (textureIndex, textureElement->getOptions ());
				}

				textureIndex++;
			}
		EndFor

		textureMaterial->setOpacity (opacity);
	}
}

//************************************************************************************************
// TextureMaterial3DTextureElement
//************************************************************************************************

BEGIN_STYLEDEF (TextureMaterial3DTextureElement::textureOptions)
	{"clamptoedge", kTextureClampToEdge},
	{"clamptoborder", kTextureClampToBorder},
	{"repeat", kTextureRepeat},
	{"mirror", kTextureMirror},
	{"mipmapenabled", kTextureMipmapEnabled},
	{"immutable", kTextureImmutable},
END_STYLEDEF

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (TextureMaterial3DTextureElement, Element, TAG_TEXTUREMATERIAL3D_TEXTURE, DOC_GROUP_3D, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_OPTIONS, TYPE_ENUM) ///< texture options
END_SKIN_ELEMENT_WITH_MEMBERS (TextureMaterial3DTextureElement)
DEFINE_SKIN_ENUMERATION (TAG_TEXTUREMATERIAL3D_TEXTURE, ATTR_OPTIONS, TextureMaterial3DTextureElement::textureOptions)

BEGIN_SKIN_ELEMENT_ATTRIBUTES (TextureMaterial3DTextureElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TEXTUREMATERIAL3DCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (TextureMaterial3DTextureElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

TextureMaterial3DTextureElement::TextureMaterial3DTextureElement ()
: options (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextureMaterial3DTextureElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);
	options = a.getOptions (ATTR_OPTIONS, textureOptions);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextureMaterial3DTextureElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setOptions (ATTR_OPTIONS, options, textureOptions);
	return true;
}

//************************************************************************************************
// Scene3DElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (Scene3DElement, ResourceObjectElement, TAG_SCENE3D, DOC_GROUP_3D, 0)

BEGIN_SKIN_ELEMENT_ATTRIBUTES (Scene3DElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_RESOURCES)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_SCENE3DCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (Scene3DElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Scene3DElement::loadObject (SkinModel& model)
{
	object = NEW SceneResource3D (*this);
	return true;
}

//************************************************************************************************
// SceneNode3DElement
//************************************************************************************************

BEGIN_STYLEDEF (SceneNode3DElement::nodeOptions)
	{"hittesting", kHitTestingEnabled},
END_STYLEDEF

BEGIN_SKIN_ELEMENT_ABSTRACT_WITH_MEMBERS (SceneNode3DElement, Element, TAG_SCENENODE3D, DOC_GROUP_3D, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_POSITION, TYPE_POINT3D) ///< 3D position (x, y, z)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ORIENTATION, TYPE_POINT3D) ///< 3D orientation (yaw, pitch, roll in radians)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_SCALE, TYPE_POINT3D) ///< 3D scale (x, y, z)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_OPTIONS, TYPE_ENUM) ///< node options
END_SKIN_ELEMENT_WITH_MEMBERS (SceneNode3DElement)
DEFINE_SKIN_ENUMERATION (TAG_SCENENODE3D, ATTR_OPTIONS, SceneNode3DElement::nodeOptions)

BEGIN_SKIN_ELEMENT_ATTRIBUTES (SceneNode3DElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_SCENE3DCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (SceneNode3DElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneNode3DElement::SceneNode3DElement ()
: options (0),
  orientation (SceneNode3D::kDefaultAngle, SceneNode3D::kDefaultAngle, SceneNode3D::kDefaultAngle),
  scale (SceneNode3D::kDefaultScale, SceneNode3D::kDefaultScale, SceneNode3D::kDefaultScale)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SceneNode3DElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);	
	options = a.getOptions (ATTR_OPTIONS, nodeOptions);
	a.getPointF3D (position, ATTR_POSITION);
	a.getPointF3D (orientation, ATTR_ORIENTATION);
	a.getPointF3D (scale, ATTR_SCALE);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SceneNode3DElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setOptions (ATTR_OPTIONS, options, nodeOptions);
	a.setPointF3D (ATTR_POSITION, position);
	a.setPointF3D (ATTR_ORIENTATION, orientation);
	a.setPointF3D (ATTR_SCALE, scale);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneNode3DElement::applyNodeAttributes (SceneNode3D* node) const
{
	if(node)
	{
		node->setName (getName ());

		if(get_flag<int> (node->getNodeFlags (), SceneNode3D::kHasPosition))
			node->setPosition (position);

		if(get_flag<int> (node->getNodeFlags (), SceneNode3D::kHasOrientation))
		{
			node->setYawAngle (orientation.x);
			node->setPitchAngle (orientation.y);
			node->setRollAngle (orientation.z);
		}

		if(get_flag<int> (node->getNodeFlags (), SceneNode3D::kHasScale))
		{
			node->setScaleX (scale.x);
			node->setScaleY (scale.y);
			node->setScaleZ (scale.z);
		}

		if(get_flag<int> (options, kHitTestingEnabled))
			node->enableHitTesting (true);
	}
}

//************************************************************************************************
// Camera3DElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (Camera3DElement, SceneNode3DElement, TAG_CAMERA3D, DOC_GROUP_3D, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_LOOKAT_POSITION, TYPE_POINT3D) ///< camera 'look at' position (3D)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_LOOKAT_UPVECTOR, TYPE_POINT3D) ///< camera 'look at' up vector (3D)	
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FIELDOFVIEW_ANGLE, TYPE_FLOAT) ///< camera field of view angle in degrees
END_SKIN_ELEMENT_WITH_MEMBERS (Camera3DElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

Camera3DElement::Camera3DElement ()
: lookAtUpVector (SceneConstants::kWorldUpVector),
  fieldOfViewAngle (Camera3D::kDefaultFieldOfViewAngle)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Camera3DElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);	
	a.getPointF3D (lookAtPosition, ATTR_LOOKAT_POSITION);
	a.getPointF3D (lookAtUpVector, ATTR_LOOKAT_UPVECTOR);
	fieldOfViewAngle = a.getFloat (ATTR_FIELDOFVIEW_ANGLE, Camera3D::kDefaultFieldOfViewAngle);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Camera3DElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setPointF3D (ATTR_LOOKAT_POSITION, lookAtPosition);
	a.setPointF3D (ATTR_LOOKAT_UPVECTOR, lookAtUpVector);
	a.setFloat (ATTR_FIELDOFVIEW_ANGLE, fieldOfViewAngle);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneNode3D* Camera3DElement::createSceneNode () const
{
	return NEW Camera3D;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Camera3DElement::applyNodeAttributes (SceneNode3D* node) const
{
	SuperClass::applyNodeAttributes (node);

	if(auto* camera = ccl_cast<Camera3D> (node))
	{
		camera->lookAt (lookAtPosition, lookAtUpVector);
		camera->setFieldOfViewAngle (fieldOfViewAngle);
	}
}

//************************************************************************************************
// LightSource3DElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_ABSTRACT_WITH_MEMBERS (LightSource3DElement, SceneNode3DElement, TAG_LIGHTSOURCE3D, DOC_GROUP_3D, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_COLOR, TYPE_COLOR) ///< light color
END_SKIN_ELEMENT_WITH_MEMBERS (LightSource3DElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LightSource3DElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);	
	colorString = a.getCString (ATTR_COLOR);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LightSource3DElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setString (ATTR_COLOR, colorString);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Color LightSource3DElement::resolveLightColor () const
{
	ColorValueReference reference;
	SkinModel::getColorFromString (reference, colorString, this);
	
	ASSERT (reference.scheme == nullptr)
	if(reference.scheme != nullptr)
		SKIN_WARNING (this, "Color scheme references not supported in 3D light source!", 0)

	return reference.colorValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LightSource3DElement::applyNodeAttributes (SceneNode3D* node) const
{
	SuperClass::applyNodeAttributes (node);
	
	if(auto* lightSource = ccl_cast<LightSource3D> (node))
	{
		lightSource->setLightColor (resolveLightColor ());
	}
}

//************************************************************************************************
// AmbientLight3DElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (AmbientLight3DElement, LightSource3DElement, TAG_AMBIENTLIGHT3D, DOC_GROUP_3D, 0)

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneNode3D* AmbientLight3DElement::createSceneNode () const
{
	return NEW AmbientLight3D;
}

//************************************************************************************************
// DirectionalLight3DElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (DirectionalLight3DElement, LightSource3DElement, TAG_DIRECTIONALIGHT3D, DOC_GROUP_3D, 0)

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneNode3D* DirectionalLight3DElement::createSceneNode () const
{
	return NEW DirectionalLight3D;
}

//************************************************************************************************
// PointLight3DElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (PointLight3DElement, LightSource3DElement, TAG_POINTLIGHT3D, DOC_GROUP_3D, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ATTENUATIONRADIUS, TYPE_FLOAT) ///< attenuation radius
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ATTENUATIONMINIMUM, TYPE_FLOAT) ///< attenuation minimum
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ATTENUATIONFACTOR, TYPE_FLOAT) ///< attenuation linear factor
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ATTENUATIONCONSTANT, TYPE_FLOAT) ///< attenuation constant term
END_SKIN_ELEMENT_WITH_MEMBERS (PointLight3DElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

PointLight3DElement::PointLight3DElement ()
: attenuationRadius (PointLight3D::kDefaultRadius),
  attenuationMinimum (PointLight3D::kDefaultMinimum),
  attenuationLinearFactor (PointLight3D::kDefaultLinearFactor),
  attenuationConstantTerm (PointLight3D::kDefaultConstantTerm)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PointLight3DElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);	
	attenuationRadius = a.getFloat (ATTR_ATTENUATIONRADIUS, PointLight3D::kDefaultRadius);
	attenuationMinimum = a.getFloat (ATTR_ATTENUATIONMINIMUM, PointLight3D::kDefaultMinimum);
	attenuationLinearFactor = a.getFloat (ATTR_ATTENUATIONFACTOR, PointLight3D::kDefaultLinearFactor);
	attenuationConstantTerm = a.getFloat (ATTR_ATTENUATIONCONSTANT, PointLight3D::kDefaultConstantTerm);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PointLight3DElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setFloat (ATTR_ATTENUATIONRADIUS, attenuationRadius);
	a.setFloat (ATTR_ATTENUATIONMINIMUM, attenuationMinimum);
	a.setFloat (ATTR_ATTENUATIONFACTOR, attenuationLinearFactor);
	a.setFloat (ATTR_ATTENUATIONCONSTANT, attenuationConstantTerm);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneNode3D* PointLight3DElement::createSceneNode () const
{
	return NEW PointLight3D;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PointLight3DElement::applyNodeAttributes (SceneNode3D* node) const
{
	SuperClass::applyNodeAttributes (node);
	
	if(auto* pointLight = ccl_cast<PointLight3D> (node))
	{
		pointLight->setAttenuationRadius (attenuationRadius);
		pointLight->setAttenuationMinimum (attenuationMinimum);
		pointLight->setAttenuationLinearFactor (attenuationLinearFactor);
		pointLight->setAttenuationConstantTerm (attenuationConstantTerm);
	}
}

//************************************************************************************************
// ModelNode3DElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ModelNode3DElement, SceneNode3DElement, TAG_MODELNODE3D, DOC_GROUP_3D, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_MODEL, TYPE_STRING) ///< model name
END_SKIN_ELEMENT_WITH_MEMBERS (ModelNode3DElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModelNode3DElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);	
	modelName = a.getCString (ATTR_MODEL);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModelNode3DElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setString (ATTR_MODEL, modelName);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneNode3D* ModelNode3DElement::createSceneNode () const
{
	return NEW ModelNode3D;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ModelNode3DElement::applyNodeAttributes (SceneNode3D* node) const
{
	SuperClass::applyNodeAttributes (node);

	if(auto* modelNode = ccl_cast<ModelNode3D> (node))
	{
		auto* skinModel = SkinModel::getModel (this);
		auto* model3d = skinModel ? ccl_cast<Model3D> (skinModel->getResource (modelName)) : nullptr;
		ASSERT (model3d != nullptr)	
		if(model3d == nullptr)
			SKIN_WARNING (this, "3D model not found: %s", modelName.str ())

		modelNode->setModelData (ccl_as_unknown (model3d));
	}
}

//************************************************************************************************
// SceneView3DElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (SceneView3DElement, ViewElement, TAG_SCENEVIEW3D, DOC_GROUP_3D, 0)

//////////////////////////////////////////////////////////////////////////////////////////////////

View* SceneView3DElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		auto sceneView = NEW SceneView3D (size);
		view = sceneView;

		Scene3D* scene = nullptr;
		if(UnknownPtr<IController> controller = args.controller)
			scene = unknown_cast<Scene3D> (controller->getObject (getName (), ClassID::Scene3D));
		sceneView->set3DContent (scene->asUnknown ());
	}

	return SuperClass::createView (args, view);
}
