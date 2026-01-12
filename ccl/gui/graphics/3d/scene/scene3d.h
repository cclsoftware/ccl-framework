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
// Filename    : ccl/gui/graphics/3d/scene/scene3d.h
// Description : 3D Scene
//
//************************************************************************************************

#ifndef _ccl_scene3d_h
#define _ccl_scene3d_h

#include "ccl/gui/graphics/3d/model/model3d.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/gui/graphics/3d/iscene3d.h"

namespace CCL {

class Scene3D;
class Camera3D;
class AnimationClock;

//************************************************************************************************
// 3D Node implementation macros
//************************************************************************************************

#define IMPLEMENT_SCENENODE3D_POSITION \
protected: PointF3D position; public: \
PointF3DRef CCL_API getPosition () const override { return position; } \
tresult CCL_API setPosition (PointF3DRef p) override { position = p; invalidateTransform (); return kResultOk; } \
tresult CCL_API setPositionX (float x) override { position.x = x; invalidateTransform (); return kResultOk; } \
tresult CCL_API setPositionY (float y) override { position.y = y; invalidateTransform (); return kResultOk; } \
tresult CCL_API setPositionZ (float z) override { position.z = z; invalidateTransform (); return kResultOk; }

#define IMPLEMENT_SCENENODE3D_NO_POSITION \
PointF3DRef CCL_API getPosition () const override { static const PointF3D null3d; return null3d; } \
tresult CCL_API setPosition (PointF3DRef p) override { return kResultNotImplemented; } \
tresult CCL_API setPositionX (float x) override { return kResultNotImplemented; } \
tresult CCL_API setPositionY (float y) override { return kResultNotImplemented; } \
tresult CCL_API setPositionZ (float z) override { return kResultNotImplemented; }

#define IMPLEMENT_SCENENODE3D_ORIENTATION \
protected: float yaw = kDefaultAngle; float pitch = kDefaultAngle; float roll = kDefaultAngle; public: \
float CCL_API getYawAngle () const override { return yaw; } \
tresult CCL_API setYawAngle (float angle) override { yaw = angle; invalidateTransform (); return kResultOk; } \
float CCL_API getPitchAngle () const override { return pitch; } \
tresult CCL_API setPitchAngle (float angle) override { pitch = angle; invalidateTransform (); return kResultOk; } \
float CCL_API getRollAngle () const override { return roll; } \
tresult CCL_API setRollAngle (float angle) override { roll = angle; invalidateTransform (); return kResultOk; }

#define IMPLEMENT_SCENENODE3D_NO_ORIENTATION \
float CCL_API getYawAngle () const override { return kDefaultAngle; } \
tresult CCL_API setYawAngle (float angle) override { return kResultNotImplemented; } \
float CCL_API getPitchAngle () const override { return kDefaultAngle; } \
tresult CCL_API setPitchAngle (float angle) override { return kResultNotImplemented; } \
float CCL_API getRollAngle () const override { return kDefaultAngle; } \
tresult CCL_API setRollAngle (float angle) override { return kResultNotImplemented; }

#define IMPLEMENT_SCENENODE3D_SCALE \
protected: float scaleX = kDefaultScale; float scaleY = kDefaultScale; float scaleZ = kDefaultScale; public: \
float CCL_API getScaleX () const override { return scaleX; } \
tresult CCL_API setScaleX (float factor) override { scaleX = factor; invalidateTransform (); return kResultOk; } \
float CCL_API getScaleY () const override { return scaleY; } \
tresult CCL_API setScaleY (float factor) override { scaleY = factor; invalidateTransform (); return kResultOk; } \
float CCL_API getScaleZ () const override { return scaleZ; } \
tresult CCL_API setScaleZ (float factor) override { scaleZ = factor; invalidateTransform (); return kResultOk; }

#define IMPLEMENT_SCENENODE3D_NO_SCALE \
float CCL_API getScaleX () const override { return kDefaultScale; } \
tresult CCL_API setScaleX (float factor) override { return kResultNotImplemented; } \
float CCL_API getScaleY () const override { return kDefaultScale; } \
tresult CCL_API setScaleY (float factor) override { return kResultNotImplemented; } \
float CCL_API getScaleZ () const override { return kDefaultScale; } \
tresult CCL_API setScaleZ (float factor) override { return kResultNotImplemented; }

#define IMPLEMENT_SCENENODE3D_INTERACTION \
bool hitTestingEnabled = false; \
tresult CCL_API enableHitTesting (tbool state) override { hitTestingEnabled = state; return kResultOk; } \
tbool CCL_API isHitTestingEnabled () const override { return hitTestingEnabled; }

#define IMPLEMENT_SCENENODE3D_NO_INTERACTION \
tresult CCL_API enableHitTesting (tbool state) override { return kResultFailed; } \
tbool CCL_API isHitTestingEnabled () const override { return false; }

#define IMPLEMENT_SCENENODE3D_BASICS(type, flags, BaseClass) \
NodeType CCL_API getNodeType () const override { return type; } \
UIDRef CCL_API getNodeClassID () const override { return myClass ().getClassID (); } \
int CCL_API getNodeFlags () const override { return flags; } \
StringID CCL_API getNodeName () const override { return this->name; } \
void CCL_API setNodeName (StringID name) override { this->name = name; } \
void CCL_API setNodeData (VariantRef data) override { this->data = data; } \
VariantRef CCL_API getNodeData () const override { return this->data; } \
ISceneNode3D* CCL_API getParentNode () override { return parent; } \
IScene3D* CCL_API getRootNode () override { return getScene (); } \
ISceneChildren3D* CCL_API getChildren () override { return BaseClass::getChildren (); } \
ISceneConstraints3D* CCL_API getConstraints () override { return BaseClass::getConstraints (); } \
tresult CCL_API addAnimation (StringID propertyId, const IAnimation* animation) override \
{ return BaseClass::addAnimation (propertyId, animation); } \
tresult CCL_API removeAnimation (StringID propertyId) override { return BaseClass::removeAnimation (propertyId); } \
ISceneNode3D* CCL_API findIntersectingNode (Ray3DRef ray, float tolerance, int findFlags) const override \
{ return BaseClass::findIntersectingNode (ray, tolerance, findFlags); } \
Transform3DRef CCL_API getWorldTransform () const override { return BaseClass::getWorldTransform (); } \
Transform3DRef CCL_API getInverseWorldTransform () const override { return BaseClass::getInverseWorldTransform (); } \
void CCL_API setWorldTransform (Transform3DRef transform) override { BaseClass::setWorldTransform (transform); }

//************************************************************************************************
// SceneNode3D
//************************************************************************************************

class SceneNode3D: public Object,
				   public ISceneNode3D
{
public:
	DECLARE_CLASS_ABSTRACT (SceneNode3D, Object)
	DECLARE_PROPERTY_NAMES (SceneNode3D)

	SceneNode3D ();
	~SceneNode3D ();

	static constexpr float kDefaultScale = 1.f;
	static constexpr float kDefaultAngle = 0.f;

	PROPERTY_MUTABLE_CSTRING (name, Name)
	PROPERTY_OBJECT (Variant, data, Data)
	PROPERTY_POINTER (SceneNode3D, parent, Parent)
	virtual Scene3D* getScene () const;

	virtual void childNodeChanged (SceneNode3D* child);

	// ISceneNode3D
	UIDRef CCL_API getNodeClassID () const override;
	IMPLEMENT_SCENENODE3D_NO_POSITION
	IMPLEMENT_SCENENODE3D_NO_ORIENTATION
	IMPLEMENT_SCENENODE3D_NO_SCALE
	Transform3DRef CCL_API getWorldTransform () const override;
	Transform3DRef CCL_API getInverseWorldTransform () const override;
	void CCL_API setWorldTransform (Transform3DRef transform) override;
	int CCL_API getNodeFlags () const override;
	StringID CCL_API getNodeName () const override;
	void CCL_API setNodeName (StringID name) override;
	void CCL_API setNodeData (VariantRef data) override;
	VariantRef CCL_API getNodeData () const override;
	IScene3D* CCL_API getRootNode () override;
	ISceneNode3D* CCL_API getParentNode () override;
	ISceneChildren3D* CCL_API getChildren () override;
	ISceneConstraints3D* CCL_API getConstraints () override;
	tresult CCL_API addAnimation (StringID propertyId, const IAnimation* animation) override;
	tresult CCL_API removeAnimation (StringID propertyId) override;
	tresult CCL_API enableHitTesting (tbool state) override;
	tbool CCL_API isHitTestingEnabled () const override;
	ISceneNode3D* CCL_API findIntersectingNode (Ray3DRef ray, float tolerance, int flags) const override;

	CLASS_INTERFACE (ISceneNode3D, Object)

protected:
	friend class ContainerNode3D;

	AutoPtr<ISceneConstraints3D> constraints;

	mutable bool transformMatrixValid;
	mutable Transform3D worldTransform;
	mutable Transform3D inverseWorldTransform;

	virtual void invalidateTransform ();
	virtual void updateTransform () const;

	virtual ISceneNode3D* findIntersectingNode (float& distance, Ray3DRef ray, float tolerance) const;
	virtual bool hitTest (float& distance, Ray3DRef ray, float tolerance) const;

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// ContainerNode3D
//************************************************************************************************

class ContainerNode3D: public SceneNode3D,
					   public ISceneChildren3D
{
public:
	DECLARE_CLASS_ABSTRACT (ContainerNode3D, SceneNode3D)

	ContainerNode3D ();

	const Container& getChildNodes () const { return nodes; }

	template <class T>
	T* findNode (StringID name)
	{
		return unknown_cast<T> (getChildren ()->findNode (name));
	}

	template <class T>
	T* getFirstOfType (bool deep = true)
	{
		for(auto n : nodes)
		{
			if(auto t = ccl_cast<T> (n))
				return t;
			if(deep)
			{
				auto* containerNode = ccl_cast<ContainerNode3D> (n);
				T* result = containerNode ? containerNode->getFirstOfType<T> (true) : nullptr;
				if(result)
					return result;
			}
		}
		return nullptr;
	}

	template <class T>
	void collectNodesOfType (Container& resultList, bool deep = true)
	{
		for(auto n : nodes)
		{
			if(auto t = ccl_cast<T> (n))
				resultList.add (t);

			if(deep)
			{
				if(auto* containerNode = ccl_cast<ContainerNode3D> (n))
					containerNode->collectNodesOfType<T> (resultList, true);
			}
		}
	}

	// ISceneChildren3D
	IUnknownIterator* CCL_API createIterator () const override;
	ISceneNode3D* CCL_API findNode (StringID name) const override;
	tresult CCL_API addNode (ISceneNode3D* node) override;
	tresult CCL_API removeNode (ISceneNode3D* node) override;

	// SceneNode3D
	ISceneChildren3D* CCL_API getChildren () override;
	ISceneNode3D* CCL_API findIntersectingNode (Ray3DRef ray, float tolerance, int flags) const override;

	CLASS_INTERFACE (ISceneChildren3D, SceneNode3D)

protected:
	ObjectArray nodes;

	// SceneNode3D
	void invalidateTransform () override;
	ISceneNode3D* findIntersectingNode (float& distance, Ray3DRef ray, float tolerance) const override;
};

//************************************************************************************************
// Scene3D
//************************************************************************************************

class Scene3D: public ContainerNode3D,
			   public IScene3D
{
public:
	DECLARE_CLASS (Scene3D, ContainerNode3D)

	Scene3D ();
	~Scene3D ();

	AnimationClock* getClock ();

	static constexpr int kChildrenChanged = 1 << 0;
	PROPERTY_READONLY_FLAG (changeFlags, kChildrenChanged, childrenChanged)
	int getChangeFlags () const;

	// ContainerNode3D
	Scene3D* getScene () const override;
	void childNodeChanged (SceneNode3D* child) override;

	// IScene3D
	IMPLEMENT_SCENENODE3D_NO_POSITION
	IMPLEMENT_SCENENODE3D_NO_ORIENTATION
	IMPLEMENT_SCENENODE3D_NO_SCALE
	IMPLEMENT_SCENENODE3D_NO_INTERACTION
	IMPLEMENT_SCENENODE3D_BASICS (kScene, 0, ContainerNode3D)
	void CCL_API setHandler (ISceneHandler3D* handler) override;
	ISceneHandler3D* CCL_API getHandler () const override;
	void CCL_API beginEditing () override;
	void CCL_API endEditing (ISceneNode3D* node = nullptr, int editFlags = 0) override;

	CLASS_INTERFACE (IScene3D, ContainerNode3D)

protected:
	class Clock;

	struct EditItem
	{
		ISceneNode3D* node = nullptr;
		int editFlags = 0;
	};
	
	Vector<EditItem> editItems;
	ISceneHandler3D* handler;
	AnimationClock* clock;
	int editCounter;
	int changeFlags;

	PROPERTY_FLAG (changeFlags, kChildrenChanged, childNodesChanged)
	
	void addEditItem (ISceneNode3D* node, int editFlags);
};

//************************************************************************************************
// Camera3D
//************************************************************************************************

class Camera3D: public SceneNode3D,
				public ICamera3D
{
public:
	DECLARE_CLASS (Camera3D, SceneNode3D)
	DECLARE_PROPERTY_NAMES (Camera3D)

	Camera3D ();

	static constexpr float kDefaultFieldOfViewAngle = 30.f;

	Transform3DRef getProjectionTransform () const;
	Transform3DRef getViewTransform () const;

	// SceneNode3D
	IMPLEMENT_SCENENODE3D_BASICS (kCamera, kHasPosition|kHasOrientation, SceneNode3D)
	IMPLEMENT_SCENENODE3D_POSITION
	IMPLEMENT_SCENENODE3D_ORIENTATION
	IMPLEMENT_SCENENODE3D_NO_INTERACTION
	IMPLEMENT_SCENENODE3D_NO_SCALE

	// ICamera3D
	tresult CCL_API lookAt (PointF3DRef p, PointF3DRef upVector = SceneConstants::kWorldUpVector) override;
	float CCL_API getFieldOfViewAngle () const override;
	tresult CCL_API setFieldOfViewAngle (float angle) override;
	Ray3D CCL_API getCameraRay (PointFRef position) const override;

	CLASS_INTERFACE (ICamera3D, SceneNode3D)

protected:

	float fieldOfViewAngle;

	mutable Transform3D projectionTransform;

	// SceneNode3D
	void updateTransform () const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// LightSource3D
//************************************************************************************************

class LightSource3D: public SceneNode3D,
					 public ILightSource3D
{
public:
	DECLARE_CLASS_ABSTRACT (LightSource3D, SceneNode3D)
	DECLARE_PROPERTY_NAMES (LightSource3D)

	// SceneNode3D
	IMPLEMENT_SCENENODE3D_BASICS (kLight, 0, SceneNode3D)

	// ILightSource3D
	ColorRef CCL_API getLightColor () const override;
	tresult CCL_API setLightColor (ColorRef color) override;

	CLASS_INTERFACE (ILightSource3D, SceneNode3D)

protected:
	Color lightColor = Colors::kWhite;

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// AmbientLight3D
//************************************************************************************************

class AmbientLight3D: public LightSource3D
{
public:
	DECLARE_CLASS (AmbientLight3D, LightSource3D)
	
	// LightSource3D
	IMPLEMENT_SCENENODE3D_BASICS (kLight, 0, SceneNode3D)
	IMPLEMENT_SCENENODE3D_NO_POSITION
	IMPLEMENT_SCENENODE3D_NO_ORIENTATION
	IMPLEMENT_SCENENODE3D_NO_SCALE
	IMPLEMENT_SCENENODE3D_NO_INTERACTION
	uint32 CCL_API getLightMask () const override;
};

//************************************************************************************************
// DirectionalLight3D
//************************************************************************************************

class DirectionalLight3D: public LightSource3D
{
public:
	DECLARE_CLASS (DirectionalLight3D, LightSource3D)

	// LightSource3D
	IMPLEMENT_SCENENODE3D_BASICS (kLight, kHasOrientation, LightSource3D)
	IMPLEMENT_SCENENODE3D_NO_POSITION
	IMPLEMENT_SCENENODE3D_ORIENTATION
	IMPLEMENT_SCENENODE3D_NO_SCALE
	IMPLEMENT_SCENENODE3D_NO_INTERACTION
	uint32 CCL_API getLightMask () const override;
};

//************************************************************************************************
// PointLight3D
//************************************************************************************************

class PointLight3D: public LightSource3D,
					public IPointLight3D
{
public:
	DECLARE_CLASS (PointLight3D, LightSource3D)
	DECLARE_PROPERTY_NAMES (PointLight3D)
	
	PointLight3D ();

	static constexpr float kDefaultQuadraticFactor = 0.01f;

	PROPERTY_VARIABLE (float, constant, ConstantTerm)
	PROPERTY_VARIABLE (float, linear, LinearFactor)
	PROPERTY_VARIABLE (float, quadratic, QuadraticFactor)
	PROPERTY_VARIABLE (float, radius, Radius)
	PROPERTY_VARIABLE (float, minimum, Minimum)

	// LightSource3D
	IMPLEMENT_SCENENODE3D_BASICS (kLight, kHasPosition|kHasScale, SceneNode3D)
	IMPLEMENT_SCENENODE3D_POSITION
	IMPLEMENT_SCENENODE3D_NO_ORIENTATION
	IMPLEMENT_SCENENODE3D_SCALE
	IMPLEMENT_SCENENODE3D_NO_INTERACTION
	ColorRef CCL_API getLightColor () const override { return SuperClass::getLightColor (); }
	tresult CCL_API setLightColor (ColorRef color) override { return SuperClass::setLightColor (color); }
	uint32 CCL_API getLightMask () const override;
	
	// IPointLight3D
	tresult CCL_API setAttenuationRadius (float radius = kDefaultRadius) override;
	float CCL_API getAttenuationRadius () const override;
	tresult CCL_API setAttenuationMinimum (float minimum = kDefaultMinimum) override;
	float CCL_API getAttenuationMinimum () const override;
	tresult CCL_API setAttenuationLinearFactor (float linearFactor = kDefaultLinearFactor) override;
	float CCL_API getAttenuationLinearFactor () const override;
	tresult CCL_API setAttenuationConstantTerm (float constantTerm = kDefaultConstantTerm) override;
	float CCL_API getAttenuationConstantTerm () const override;
		
	CLASS_INTERFACE (IPointLight3D, LightSource3D)

protected:
	bool updateQuadraticFactor ();

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// ModelNode3D
//************************************************************************************************

class ModelNode3D: public ContainerNode3D,
				   public IModelNode3D
{
public:
	DECLARE_CLASS (ModelNode3D, ContainerNode3D)

	Model3D* getModel () { return model; }

	// ContainerNode3D
	IMPLEMENT_SCENENODE3D_BASICS (kModel, kHasPosition|kHasOrientation|kHasScale|kIsInteractive, ContainerNode3D)
	IMPLEMENT_SCENENODE3D_POSITION
	IMPLEMENT_SCENENODE3D_ORIENTATION
	IMPLEMENT_SCENENODE3D_SCALE
	IMPLEMENT_SCENENODE3D_INTERACTION

	// IModelNode3D
	IUnknown* CCL_API getModelData () const override;
	tresult CCL_API setModelData (IUnknown* data) override;

	CLASS_INTERFACE (IModelNode3D, ContainerNode3D)

protected:
	SharedPtr<Model3D> model;

	// ContainerNode3D
	bool hitTest (float& distance, Ray3DRef ray, float tolerance) const override;
};

} // namespace CCL

#endif // _ccl_scene3d_h
