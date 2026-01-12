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
// Filename    : ccl/gui/graphics/3d/scene/scene3d.cpp
// Description : 3D Scene
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/graphics/3d/scene/scene3d.h"
#include "ccl/gui/graphics/3d/model/model3d.h"
#include "ccl/gui/graphics/graphicshelper.h"

#include "ccl/gui/system/animation.h"

#include "ccl/base/message.h"

#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/gui/graphics/3d/ray3d.h"
#include "ccl/public/gui/graphics/3d/stockshader3d.h"

namespace CCL {

//************************************************************************************************
// Scene3D::Clock
//************************************************************************************************

class Scene3D::Clock: public AnimationClock
{
public:
	Clock (Scene3D& owner): owner (owner) {}
	
	// AnimationClock
	void onAnimate (bool begin) override
	{
		if(begin)
			owner.beginEditing ();
		else
			owner.endEditing (nullptr, IScene3D::kAnimationEdit);
	}

protected:
	Scene3D& owner;
};

//************************************************************************************************
// SceneConstraints3D
//************************************************************************************************

class SceneConstraints3D: public Object,
						  public ISceneConstraints3D
{
public:
	// ISceneConstraints3D
	tresult CCL_API addConstraints (ITransformConstraints3D* node) override;
	tresult CCL_API removeConstraints (ITransformConstraints3D* node) override;
	tbool CCL_API isValidTransform (Transform3DRef transform) const override;

	CLASS_INTERFACE (ISceneConstraints3D, Object)

protected:
	UnknownList constraints;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// SceneNode3D
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SceneNode3D, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneNode3D::SceneNode3D ()
: parent (nullptr),
  transformMatrixValid (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneNode3D::~SceneNode3D ()
{
	signal (Message (kDestroyed)); // signal for animation manager
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Scene3D* SceneNode3D::getScene () const
{
	return parent ? parent->getScene () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDRef CCL_API SceneNode3D::getNodeClassID () const
{
	return myClass ().getClassID ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SceneNode3D::getNodeFlags () const { return 0; }
StringID CCL_API SceneNode3D::getNodeName () const { return name; }
void CCL_API SceneNode3D::setNodeName (StringID _name) { name = _name; }
void CCL_API SceneNode3D::setNodeData (VariantRef _data) { data = _data; }
VariantRef CCL_API SceneNode3D::getNodeData () const { return data; }
IScene3D* CCL_API SceneNode3D::getRootNode () { return getScene (); }
ISceneNode3D* CCL_API SceneNode3D::getParentNode () { return parent; }
ISceneChildren3D* CCL_API SceneNode3D::getChildren () { return nullptr; }
tresult CCL_API SceneNode3D::enableHitTesting (tbool state) { return kResultNotImplemented; }
tbool CCL_API SceneNode3D::isHitTestingEnabled () const { return false; }

//////////////////////////////////////////////////////////////////////////////////////////////////

ISceneConstraints3D* CCL_API SceneNode3D::getConstraints ()
{
	if(!constraints.isValid ())
		constraints = NEW SceneConstraints3D;
	return constraints;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneNode3D::childNodeChanged (SceneNode3D* child)
{
	if(parent)
		parent->childNodeChanged (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform3DRef CCL_API SceneNode3D::getWorldTransform () const
{
	if(transformMatrixValid == false)
		updateTransform ();
	return worldTransform;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform3DRef CCL_API SceneNode3D::getInverseWorldTransform () const
{
	if(transformMatrixValid == false)
		updateTransform ();
	return inverseWorldTransform;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SceneNode3D::setWorldTransform (Transform3DRef transform)
{
	worldTransform = transform;
	Transform3D localTransform = worldTransform;
	if(parent)
		localTransform = parent->getInverseWorldTransform () * localTransform;

	if(get_flag<int> (getNodeFlags (), kHasOrientation))
	{
		float yaw = 0.f;
		float pitch = 0.f;
		float roll = 0.f;
		TransformUtils3D::getYawPitchRollAngles (yaw, pitch, roll, localTransform);
		setYawAngle (yaw);
		setPitchAngle (pitch);
		setRollAngle (roll);
	}

	if(get_flag<int> (getNodeFlags (), kHasScale))
	{
		PointF3D scale;
		localTransform.getScale (scale);
		setScaleX (scale.x);
		setScaleY (scale.y);
		setScaleZ (scale.z);
	}

	if(get_flag<int> (getNodeFlags (), kHasPosition))
	{
		PointF3D translation;
		localTransform.getTranslation (translation);
		setPosition (translation);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneNode3D::invalidateTransform ()
{
	transformMatrixValid = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneNode3D::updateTransform () const
{
	PointF3DRef position = getPosition ();

	Transform3D transform;
	transform.translate (position);
	transform *= TransformUtils3D::rotateYawPitchRoll (getYawAngle (), getPitchAngle (), getRollAngle ());
	transform.scale (getScaleX (), getScaleY (), getScaleZ ());

	if(parent)
		worldTransform = parent->getWorldTransform () * transform;
	else
		worldTransform = transform;

	inverseWorldTransform = worldTransform.getInverseTransform ();

	transformMatrixValid = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SceneNode3D::addAnimation (StringID propertyId, const IAnimation* _animation)
{
	auto animation = Animation::cast<Animation> (_animation);
	if(!animation)
		return kResultInvalidArgument;

	// all animations in scene share clock for scene edit notifications 
	Scene3D* scene = getScene ();
	ASSERT (scene)
	if(!scene)
		return kResultFailed;

	ASSERT (!animation->getClock ())
	animation->setClock (scene->getClock ());
	
	// stop potential running animation with this property
	removeAnimation (propertyId); 
	tresult result = AnimationManager::instance ().addAnimation (this, propertyId, animation);
	animation->setClock (nullptr);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SceneNode3D::removeAnimation (StringID propertyId)
{
	return AnimationManager::instance ().removeAnimation (this, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISceneNode3D* CCL_API SceneNode3D::findIntersectingNode (Ray3DRef ray, float tolerance, int flags) const
{
	float distance = -1.f;
	if(ISceneNode3D* result = findIntersectingNode (distance, ray, tolerance))
		return result;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISceneNode3D* SceneNode3D::findIntersectingNode (float& distance, Ray3DRef ray, float tolerance) const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SceneNode3D::hitTest (float& distance, Ray3DRef ray, float tolerance) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (SceneNode3D)
	DEFINE_PROPERTY_TYPE (SceneNode3D::kName, ITypeInfo::kString)
	DEFINE_PROPERTY_CLASS_ (SceneNode3D::kParent, "SceneNode3D", ITypeInfo::kReadOnly)
	DEFINE_PROPERTY_CLASS (SceneNode3D::kPosition, "UIValue")
	DEFINE_PROPERTY_TYPE (SceneNode3D::kPositionX, ITypeInfo::kFloat)
	DEFINE_PROPERTY_TYPE (SceneNode3D::kPositionY, ITypeInfo::kFloat)
	DEFINE_PROPERTY_TYPE (SceneNode3D::kPositionZ, ITypeInfo::kFloat)
	DEFINE_PROPERTY_TYPE (SceneNode3D::kYawAngle, ITypeInfo::kFloat)
	DEFINE_PROPERTY_TYPE (SceneNode3D::kPitchAngle, ITypeInfo::kFloat)
	DEFINE_PROPERTY_TYPE (SceneNode3D::kRollAngle, ITypeInfo::kFloat)
	DEFINE_PROPERTY_TYPE (SceneNode3D::kScaleX, ITypeInfo::kFloat)
	DEFINE_PROPERTY_TYPE (SceneNode3D::kScaleY, ITypeInfo::kFloat)
	DEFINE_PROPERTY_TYPE (SceneNode3D::kScaleZ, ITypeInfo::kFloat)
END_PROPERTY_NAMES (SceneNode3D)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SceneNode3D::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == kName)
	{
		MutableCString name (var.asString ());
		setNodeName (name);
		return true;
	}
	else if(propertyId == kPosition)
	{
		PointF3D p;
		if(IUIValue* value = IUIValue::toValue (var))
			value->toPointF3D (p);
		setPosition (p);
		return true;
	}
	else if(propertyId == kPositionX)
	{
		setPositionX (var.asFloat ());
		return true;
	}
	else if(propertyId == kPositionY)
	{
		setPositionY (var.asFloat ());
		return true;
	}
	else if(propertyId == kPositionZ)
	{
		setPositionZ (var.asFloat ());
		return true;
	}
	else if(propertyId == kYawAngle)
	{
		setYawAngle (var.asFloat ());
		return true;
	}
	else if(propertyId == kPitchAngle)
	{
		setPitchAngle (var.asFloat ());
		return true;
	}
	else if(propertyId == kRollAngle)
	{
		setRollAngle (var.asFloat ());
		return true;
	}
	else if(propertyId == kScaleX)
	{
		setScaleX (var.asFloat ());
		return true;
	}
	else if(propertyId == kScaleY)
	{
		setScaleY (var.asFloat ());
		return true;
	}
	else if(propertyId == kScaleZ)
	{
		setScaleZ (var.asFloat ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SceneNode3D::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == kName)
	{
		String name (getNodeName ());
		var = name;
		var.share ();
		return true;
	}
	else if(propertyId == kParent)
	{
		var.takeShared (const_cast<SceneNode3D*> (this)->getParentNode ());
		return true;
	}
	else if(propertyId == kPosition)
	{
		AutoPtr<UIValue> value = NEW UIValue;
		value->fromPointF3D (getPosition ());
		var.takeShared (value->asUnknown ());
		return true;
	}
	else if(propertyId == kPositionX)
	{
		var = getPosition ().x;
		return true;
	}
	else if(propertyId == kPositionY)
	{
		var = getPosition ().y;
		return true;
	}
	else if(propertyId == kPositionZ)
	{
		var = getPosition ().z;
		return true;
	}
	else if(propertyId == kYawAngle)
	{
		var = getYawAngle ();
		return true;
	}
	else if(propertyId == kPitchAngle)
	{
		var = getPitchAngle ();
		return true;
	}
	else if(propertyId == kRollAngle)
	{
		var = getRollAngle ();
		return true;
	}
	else if(propertyId == kScaleX)
	{
		var = getScaleX ();
		return true;
	}
	else if(propertyId == kScaleY)
	{
		var = getScaleY ();
		return true;
	}
	else if(propertyId == kScaleZ)
	{
		var = getScaleZ ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// ContainerNode3D
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ContainerNode3D, SceneNode3D)

//////////////////////////////////////////////////////////////////////////////////////////////////

ContainerNode3D::ContainerNode3D ()
{
	nodes.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API ContainerNode3D::createIterator () const
{
	return nodes.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISceneNode3D* CCL_API ContainerNode3D::findNode (StringID name) const
{
	return nodes.findIf<SceneNode3D> ([name] (const SceneNode3D& n) { return n.getName () == name; });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContainerNode3D::addNode (ISceneNode3D* _node)
{
	auto node = unknown_cast<SceneNode3D> (_node);
	if(!node)
		return kResultInvalidArgument;

	nodes.add (node);

	ASSERT (!node->getParent ())
	node->setParent (this);
	
	childNodeChanged (node);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContainerNode3D::removeNode (ISceneNode3D* _node)
{
	auto node = unknown_cast<SceneNode3D> (_node);
	if(!node)
		return kResultInvalidArgument;

	if(!nodes.remove (node))
		return kResultFailed;

	ASSERT (node->getParent () == this)
	node->setParent (nullptr);
	
	childNodeChanged (node);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISceneChildren3D* CCL_API ContainerNode3D::getChildren ()
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISceneNode3D* CCL_API ContainerNode3D::findIntersectingNode (Ray3DRef ray, float tolerance, int flags) const
{
	return SuperClass::findIntersectingNode (ray, tolerance, flags); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContainerNode3D::invalidateTransform ()
{
	SuperClass::invalidateTransform ();
	for(auto* child : iterate_as<SceneNode3D> (nodes))
		child->invalidateTransform ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISceneNode3D* ContainerNode3D::findIntersectingNode (float& minDistance, Ray3DRef ray, float tolerance) const
{
	ISceneNode3D* nearestNode = nullptr;

	float distance = -1.f;
	if(hitTest (distance, ray, tolerance))
	{
		if(distance < minDistance || minDistance < 0.f)
		{
			minDistance = distance;
			nearestNode = const_cast<ContainerNode3D*> (this);
		}
	}

	for(auto* child : iterate_as<SceneNode3D> (nodes))
	{
		if(ISceneNode3D* result = child->findIntersectingNode (distance, ray, tolerance))
		{
			if(distance < minDistance || minDistance < 0.f)
			{
				minDistance = distance;
				nearestNode = result;
			}
		}
	}

	if(nearestNode)
	{
		CCL_PRINTF ("Intersects node \"%s\" at distance %f\n", MutableCString (nearestNode->getNodeName ()).str (), minDistance)
	}
	return nearestNode;
}

//************************************************************************************************
// Scene3D
//************************************************************************************************

DEFINE_CLASS (Scene3D, ContainerNode3D)
DEFINE_CLASS_UID (Scene3D, 0xb0b9e54a, 0xa8c7, 0x45fd, 0x85, 0xe1, 0x89, 0x3a, 0x58, 0x2c, 0x1b, 0x44)

//////////////////////////////////////////////////////////////////////////////////////////////////

Scene3D::Scene3D ()
: handler (nullptr),
  clock (nullptr),
  editCounter (0),
  changeFlags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Scene3D::~Scene3D ()
{
	safe_release (clock);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnimationClock* Scene3D::getClock ()
{
	if(!clock)
		clock = NEW Clock (*this);
	return clock;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Scene3D::getChangeFlags () const 
{
	return changeFlags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Scene3D* Scene3D::getScene () const
{
	return const_cast<Scene3D*> (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scene3D::childNodeChanged (SceneNode3D* child)
{
	ASSERT (editCounter > 0)
	childNodesChanged (true);

	SuperClass::childNodeChanged (child);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Scene3D::setHandler (ISceneHandler3D* _handler)
{
	handler = _handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISceneHandler3D* CCL_API Scene3D::getHandler () const
{
	return handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scene3D::beginEditing ()
{
	ASSERT (editCounter >= 0)
	editCounter++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scene3D::endEditing (ISceneNode3D* node, int editFlags)
{
	ASSERT (editCounter > 0)
	if(editCounter > 0)
		editCounter--;
	
	addEditItem (node, editFlags);
	
	if(editCounter == 0)
	{
		if(handler)
		{
			for(EditItem& item : editItems)
				handler->sceneChanged (*this, item.node, item.editFlags);
			editItems.removeAll ();
		}

		signal (Message (kChanged));
		changeFlags = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scene3D::addEditItem (ISceneNode3D* node, int editFlags)
{
	for(EditItem& item : editItems)
	{
		if(item.node == node)
		{
			item.editFlags |= editFlags;
			return;
		}
	}
	editItems.add ({ node, editFlags });
}

//************************************************************************************************
// Camera3D
//************************************************************************************************

DEFINE_CLASS (Camera3D, SceneNode3D)
DEFINE_CLASS_UID (Camera3D, 0xd4060680, 0x59dd, 0x46c1, 0xbd, 0xb9, 0xde, 0xd4, 0xe5, 0x3, 0xc9, 0x21)

//////////////////////////////////////////////////////////////////////////////////////////////////

Camera3D::Camera3D ()
: fieldOfViewAngle (kDefaultFieldOfViewAngle)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Camera3D::lookAt (PointF3DRef p, PointF3DRef upVector)
{
	Transform3D transform = TransformUtils3D::lookAt (position, p, upVector);
	TransformUtils3D::getYawPitchRollAngles (yaw, pitch, roll, transform.getInverseTransform ());
	invalidateTransform ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API Camera3D::getFieldOfViewAngle () const
{
	return fieldOfViewAngle; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Camera3D::setFieldOfViewAngle (float angle)
{
	if(angle != fieldOfViewAngle)
	{
		fieldOfViewAngle = angle;
		invalidateTransform ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Ray3D CCL_API Camera3D::getCameraRay (PointFRef position) const
{
	// Position is a normalized device coordinate with range 0 to 1.
	// Transform to screen space with range -1 to 1, flip the y coordinate.
	PointF screenCoordinate (2.f * position.x - 1.f, 1.f - 2.f * position.y);

	// Transform to camera space.
	TransformUtils3D::screenSpaceToCameraSpace (screenCoordinate, Math::degreesToRad (fieldOfViewAngle), 1.f);

	Ray3D ray;
	ray.direction = PointF3D (screenCoordinate.x, screenCoordinate.y, 1);

	// Transform to world space
	ray.direction = worldTransform * ray.direction;
	ray.direction = ray.direction - getPosition ();
	ray.direction = ray.direction.normal ();

	ray.origin = getPosition ();

	return ray;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform3DRef Camera3D::getProjectionTransform () const
{
	return projectionTransform;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform3DRef Camera3D::getViewTransform () const
{
	return getInverseWorldTransform ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Camera3D::updateTransform () const
{
	SuperClass::updateTransform ();

	projectionTransform = TransformUtils3D::perspectiveFovLH (Math::degreesToRad (fieldOfViewAngle), 1.f, 1.f, 100.f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (Camera3D)
	DEFINE_PROPERTY_TYPE (Camera3D::kFieldOfViewAngle, ITypeInfo::kFloat)
END_PROPERTY_NAMES (Camera3D)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Camera3D::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == kFieldOfViewAngle)
	{
		setFieldOfViewAngle (var.asFloat ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Camera3D::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == kFieldOfViewAngle)
	{
		var = getFieldOfViewAngle ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// LightSource3D
//************************************************************************************************

DEFINE_CLASS (LightSource3D, SceneNode3D)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorRef CCL_API LightSource3D::getLightColor () const { return lightColor; }
tresult CCL_API LightSource3D::setLightColor (ColorRef color) { lightColor = color; return kResultOk; }

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (LightSource3D)
	DEFINE_PROPERTY_CLASS (LightSource3D::kLightColor, "UIValue")
END_PROPERTY_NAMES (LightSource3D)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LightSource3D::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == kLightColor)
	{
		Color c;
		if(IUIValue* value = IUIValue::toValue (var))
			value->toColor (c);
		setLightColor (c);
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LightSource3D::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == kLightColor)
	{
		AutoPtr<UIValue> value = NEW UIValue;
		value->fromColor (getLightColor ());
		var.takeShared (value->asUnknown ());
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// AmbientLight3D
//************************************************************************************************

DEFINE_CLASS (AmbientLight3D, LightSource3D)
DEFINE_CLASS_UID (AmbientLight3D, 0xacacdd9c, 0xcee2, 0x4765, 0x98, 0x30, 0x59, 0xa3, 0xf0, 0xa2, 0x3a, 0xa0)

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API AmbientLight3D::getLightMask () const
{
	return CCL_3D_SHADER_AMBIENTLIGHT_BIT;
}

//************************************************************************************************
// DirectionalLight3D
//************************************************************************************************

DEFINE_CLASS (DirectionalLight3D, LightSource3D)
DEFINE_CLASS_UID (DirectionalLight3D, 0xe30ff586, 0xd48, 0x453a, 0x82, 0x3a, 0x11, 0xd9, 0xc5, 0xd5, 0xa9, 0x9b)

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API DirectionalLight3D::getLightMask () const
{
	return CCL_3D_SHADER_DIRECTIONALLIGHT_BIT;
}

//************************************************************************************************
// PointLight3D
//************************************************************************************************

DEFINE_CLASS (PointLight3D, LightSource3D)
DEFINE_CLASS_UID (PointLight3D, 0xbabe86ca, 0x29d2, 0x498a, 0xa1, 0xc8, 0x32, 0xb0, 0xb8, 0x34, 0x97, 0xfb)

//////////////////////////////////////////////////////////////////////////////////////////////////

PointLight3D::PointLight3D ()
: constant (kDefaultConstantTerm),
  linear (kDefaultLinearFactor),
  quadratic (kDefaultQuadraticFactor),
  radius (kDefaultRadius),
  minimum (kDefaultMinimum)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PointLight3D::updateQuadraticFactor ()
{
	if(radius <= 0.f || minimum <= 0.f)
		return false;
	quadratic = 1.f / (radius * radius * minimum);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PointLight3D::setAttenuationRadius (float attenuationRadius)
{
	radius = attenuationRadius;
	return updateQuadraticFactor () ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API PointLight3D::getAttenuationRadius () const
{
	return radius;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PointLight3D::setAttenuationMinimum (float attenuationMinimum)
{
	minimum = attenuationMinimum;
	return updateQuadraticFactor () ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API PointLight3D::getAttenuationMinimum () const
{
	return minimum;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PointLight3D::setAttenuationLinearFactor (float linearFactor)
{
	linear = linearFactor;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API PointLight3D::getAttenuationLinearFactor () const
{
	return linear;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PointLight3D::setAttenuationConstantTerm (float constantTerm)
{
	constant = constantTerm;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API PointLight3D::getAttenuationConstantTerm () const
{
	return constant;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API PointLight3D::getLightMask () const
{
	ObjectArray pointLights;
	if(Scene3D* scene = getScene ())
		scene->collectNodesOfType<PointLight3D> (pointLights);

	int index = pointLights.index (this);
	if(index >= 0)
		return CCL_3D_SHADER_POINTLIGHT_BIT (index);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (PointLight3D)
	DEFINE_PROPERTY_TYPE (PointLight3D::kAttenuationRadius, ITypeInfo::kFloat)
	DEFINE_PROPERTY_TYPE (PointLight3D::kAttenuationMinimum, ITypeInfo::kFloat)
	DEFINE_PROPERTY_TYPE (PointLight3D::kAttenuationLinearFactor, ITypeInfo::kFloat)
	DEFINE_PROPERTY_TYPE (PointLight3D::kAttenuationConstantTerm, ITypeInfo::kFloat)
END_PROPERTY_NAMES (PointLight3D)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PointLight3D::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == kAttenuationRadius)
	{
		setAttenuationRadius (var.asFloat ());
		return true;
	}
	else if(propertyId == kAttenuationMinimum)
	{
		setAttenuationMinimum (var.asFloat ());
		return true;
	}
	else if(propertyId == kAttenuationLinearFactor)
	{
		setAttenuationLinearFactor (var.asFloat ());
		return true;
	}
	else if(propertyId == kAttenuationConstantTerm)
	{
		setAttenuationConstantTerm (var.asFloat ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PointLight3D::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == kAttenuationRadius)
	{
		var = getAttenuationRadius ();
		return true;
	}
	else if(propertyId == kAttenuationMinimum)
	{
		var = getAttenuationMinimum ();
		return true;
	}
	else if(propertyId == kAttenuationLinearFactor)
	{
		var = getAttenuationLinearFactor ();
		return true;
	}
	else if(propertyId == kAttenuationConstantTerm)
	{
		var = getAttenuationConstantTerm ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// ModelNode3D
//************************************************************************************************

DEFINE_CLASS (ModelNode3D, ContainerNode3D)
DEFINE_CLASS_UID (ModelNode3D, 0xb79b4ca4, 0xafb3, 0x45d9, 0xa2, 0x33, 0x50, 0x8c, 0x4f, 0xb1, 0x96, 0x33)

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ModelNode3D::getModelData () const
{
	return model.isValid () ? model->asUnknown () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ModelNode3D::setModelData (IUnknown* data)
{
	model = unknown_cast<Model3D> (data);
	return kResultOk; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModelNode3D::hitTest (float& distance, Ray3DRef ray, float tolerance) const
{
	if(!model.isValid () || isHitTestingEnabled () == false)
		return false;

	BoundingSphere3D boundingSphere;

	for(int i = 0; i < model->getGeometryCount (); i++)
	{
		IGeometry3D* geometry = model->getGeometryAt (i);
		if(geometry == nullptr)
			continue;

		geometry->getBoundingSphere (boundingSphere);

		// We currently only test intersection with the bounding sphere.
		// If we need more precision, we have to implement vertex-based hit testing as well.
		// If we do so, we should transform the ray to model space instead of transforming the bounding sphere to world space.
		// The value of the distance variable might need to be transformed as well.

		boundingSphere.origin = getWorldTransform () * boundingSphere.origin;

		PointF3D scale;
		getWorldTransform ().getScale (scale);
		boundingSphere.radius = boundingSphere.radius * ccl_max (scale.x, ccl_max (scale.y, scale.z));

		if(ray.intersectsSphere (distance, boundingSphere.origin, boundingSphere.radius * (1.f + tolerance)))
			return true;
	}

	return false;
}

//************************************************************************************************
// SceneConstraints3D
//************************************************************************************************

tresult CCL_API SceneConstraints3D::addConstraints (ITransformConstraints3D* transformConstraints)
{
	return constraints.add (transformConstraints) ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SceneConstraints3D::removeConstraints (ITransformConstraints3D* transformConstraints)
{
	return constraints.remove (transformConstraints) ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SceneConstraints3D::isValidTransform (Transform3DRef transform) const
{
	ForEachUnknown (constraints, unk)
		UnknownPtr<ITransformConstraints3D> transformConstraints (unk);
		if(transformConstraints.isValid () && !transformConstraints->isValidTransform (transform))
			return false;
	EndFor
	return true;
}
