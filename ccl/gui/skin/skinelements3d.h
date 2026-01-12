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
// Filename    : ccl/gui/skin/skinelements3d.h
// Description : 3D Skin Elements
//
//************************************************************************************************

#ifndef _ccl_skinelements3d_h
#define _ccl_skinelements3d_h

#include "ccl/gui/skin/skinmodel.h"

#include "ccl/public/gui/graphics/3d/point3d.h"

namespace CCL {

class Material3D;
class SceneNode3D;

namespace SkinElements {

//************************************************************************************************
// Model3DElement
//************************************************************************************************

class Model3DElement: public ResourceObjectElement
{
public:
	DECLARE_SKIN_ELEMENT (Model3DElement, ResourceObjectElement)

	// ResourceObjectElement
	bool loadObject (SkinModel& model) override;
};

//************************************************************************************************
// Material3DElement
//************************************************************************************************

class Material3DElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT_ABSTRACT (Material3DElement, Element)

	Material3DElement ();

	DECLARE_STYLEDEF (lightMaskFlags)

	PROPERTY_VARIABLE (float, depthBias, DepthBias)
	PROPERTY_VARIABLE (uint32, lightMask, LightMask)

	virtual Material3D* createMaterial () const = 0;
	virtual void applyMaterialAttributes (Material3D* material) const;

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// SolidColorMaterial3DElement
//************************************************************************************************

class SolidColorMaterial3DElement: public Material3DElement
{
public:
	DECLARE_SKIN_ELEMENT (SolidColorMaterial3DElement, Material3DElement)

	SolidColorMaterial3DElement ();

	PROPERTY_VARIABLE (float, shininess, Shininess)
	PROPERTY_MUTABLE_CSTRING (colorString, ColorString)

	// Material3DElement
	Material3D* createMaterial () const override;
	void applyMaterialAttributes (Material3D* material) const override;
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// TextureMaterial3DElement
//************************************************************************************************

class TextureMaterial3DElement: public Material3DElement
{
public:
	DECLARE_SKIN_ELEMENT (TextureMaterial3DElement, Material3DElement)

	TextureMaterial3DElement ();

	PROPERTY_VARIABLE (float, opacity, Opacity)

	// Material3DElement
	Material3D* createMaterial () const override;
	void applyMaterialAttributes (Material3D* material) const override;
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// TextureMaterial3DTextureElement
//************************************************************************************************

class TextureMaterial3DTextureElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (TextureMaterial3DTextureElement, Element)

	TextureMaterial3DTextureElement ();

	DECLARE_STYLEDEF (textureOptions)

	PROPERTY_VARIABLE (int, options, Options)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// Scene3DElement
//************************************************************************************************

class Scene3DElement: public ResourceObjectElement
{
public:
	DECLARE_SKIN_ELEMENT (Scene3DElement, ResourceObjectElement)

	// ResourceObjectElement
	bool loadObject (SkinModel& model) override;
};

//************************************************************************************************
// SceneNode3DElement
//************************************************************************************************

class SceneNode3DElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT_ABSTRACT (SceneNode3DElement, Element)

	SceneNode3DElement ();

	enum NodeOptions
	{
		kHitTestingEnabled = 1<<0
	};

	DECLARE_STYLEDEF (nodeOptions)

	PROPERTY_VARIABLE (int, options, Options)
	PROPERTY_OBJECT (PointF3D, position, Position)
	PROPERTY_OBJECT (PointF3D, orientation, Orientation)
	PROPERTY_OBJECT (PointF3D, scale, Scale)

	virtual SceneNode3D* createSceneNode () const = 0;
	virtual void applyNodeAttributes (SceneNode3D* node) const;

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// Camera3DElement
//************************************************************************************************

class Camera3DElement: public SceneNode3DElement
{
public:
	DECLARE_SKIN_ELEMENT (Camera3DElement, SceneNode3DElement)

	Camera3DElement ();

	PROPERTY_OBJECT (PointF3D, lookAtPosition, LookAtPosition)
	PROPERTY_OBJECT (PointF3D, lookAtUpVector, LookAtUpVector)
	PROPERTY_VARIABLE (float, fieldOfViewAngle, FieldOfViewAngle)

	// SceneNode3DElement
	SceneNode3D* createSceneNode () const override;
	void applyNodeAttributes (SceneNode3D* node) const override;
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// LightSource3DElement
//************************************************************************************************

class LightSource3DElement: public SceneNode3DElement
{
public:
	DECLARE_SKIN_ELEMENT_ABSTRACT (LightSource3DElement, SceneNode3DElement)

	PROPERTY_MUTABLE_CSTRING (colorString, ColorString)

	// SceneNode3DElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	void applyNodeAttributes (SceneNode3D* node) const override;

protected:
	Color resolveLightColor () const;
};

//************************************************************************************************
// AmbientLight3DElement
//************************************************************************************************

class AmbientLight3DElement: public LightSource3DElement
{
public:
	DECLARE_SKIN_ELEMENT (AmbientLight3DElement, LightSource3DElement)

	// LightSource3DElement
	SceneNode3D* createSceneNode () const override;
};

//************************************************************************************************
// DirectionalLight3DElement
//************************************************************************************************

class DirectionalLight3DElement: public LightSource3DElement
{
public:
	DECLARE_SKIN_ELEMENT (DirectionalLight3DElement, LightSource3DElement)

	// LightSource3DElement
	SceneNode3D* createSceneNode () const override;
};

//************************************************************************************************
// PointLight3DElement
//************************************************************************************************

class PointLight3DElement: public LightSource3DElement
{
public:
	DECLARE_SKIN_ELEMENT (PointLight3DElement, LightSource3DElement)

	PointLight3DElement ();

	PROPERTY_VARIABLE (float, attenuationRadius, AttenuationRadius)
	PROPERTY_VARIABLE (float, attenuationMinimum, AttenuationMinimum)
	PROPERTY_VARIABLE (float, attenuationLinearFactor, AttenuationLinearFactor)
	PROPERTY_VARIABLE (float, attenuationConstantTerm, AttenuationConstantTerm)

	// LightSource3DElement
	SceneNode3D* createSceneNode () const override;
	void applyNodeAttributes (SceneNode3D* node) const override;
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// ModelNode3DElement
//************************************************************************************************

class ModelNode3DElement: public SceneNode3DElement
{
public:
	DECLARE_SKIN_ELEMENT (ModelNode3DElement, SceneNode3DElement)

	PROPERTY_MUTABLE_CSTRING (modelName, ModelName)

	// LightSource3DElement
	SceneNode3D* createSceneNode () const override;
	void applyNodeAttributes (SceneNode3D* node) const override;
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// SceneView3DElement
//************************************************************************************************

class SceneView3DElement: public ViewElement
{
public:
	DECLARE_SKIN_ELEMENT (SceneView3DElement, ViewElement)

	// ViewElement
	View* createView (const CreateArgs& args, View* view) override;
};

} // namespace SkinElements
} // namespace CCL

#endif // _ccl_skinelements3d_h
