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
// Filename    : ccl/public/gui/graphics/3d/iscene3d.h
// Description : 3D Scene Interfaces
//
//************************************************************************************************

#ifndef _ccl_iscene3d_h
#define _ccl_iscene3d_h

#include "ccl/public/collections/iunknownlist.h"

#include "ccl/public/gui/graphics/color.h"
#include "ccl/public/gui/graphics/3d/ray3d.h"
#include "ccl/public/gui/graphics/3d/itransformconstraints3d.h"

namespace CCL {

interface IAnimation;
interface IScene3D;
interface ISceneChildren3D;
interface ISceneHandler3D;
interface ISceneConstraints3D;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** 3D scene [ISceneNode3D] */
	DEFINE_CID (Scene3D, 0xb0b9e54a, 0xa8c7, 0x45fd, 0x85, 0xe1, 0x89, 0x3a, 0x58, 0x2c, 0x1b, 0x44)

	/** 3D camera [ICamera3D] */
	DEFINE_CID (Camera3D, 0xd4060680, 0x59dd, 0x46c1, 0xbd, 0xb9, 0xde, 0xd4, 0xe5, 0x3, 0xc9, 0x21)

	/** Ambient light [ILightSource3D] */
	DEFINE_CID (AmbientLight3D, 0xacacdd9c, 0xcee2, 0x4765, 0x98, 0x30, 0x59, 0xa3, 0xf0, 0xa2, 0x3a, 0xa0)

	/** Directional light [ILightSource3D] */
	DEFINE_CID (DirectionalLight3D, 0xe30ff586, 0xd48, 0x453a, 0x82, 0x3a, 0x11, 0xd9, 0xc5, 0xd5, 0xa9, 0x9b)

	/** Point light [ILightSource3D] */
	DEFINE_CID (PointLight3D, 0xbabe86ca, 0x29d2, 0x498a, 0xa1, 0xc8, 0x32, 0xb0, 0xb8, 0x34, 0x97, 0xfb)

	/** 3D model node [IModelNode] */
	DEFINE_CID (ModelNode3D, 0xb79b4ca4, 0xafb3, 0x45d9, 0xa2, 0x33, 0x50, 0x8c, 0x4f, 0xb1, 0x96, 0x33)
}

//************************************************************************************************
// Scene Constants
//************************************************************************************************

namespace SceneConstants
{
	static const PointF3D kWorldUpVector = PointF3D (0.f, 1.f, 0.f);
}

//************************************************************************************************
// ISceneNode3D
/** Node in a 3D scene.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface ISceneNode3D: IUnknown
{
	DEFINE_ENUM (NodeType)
	{
		kScene,
		kCamera,
		kLight,
		kModel
	};

	enum NodeFlags
	{
		kHasPosition = 1<<0,
		kHasOrientation = 1<<1,
		kHasScale = 1<<2,
		kIsInteractive = 1<<3
	};

	/** Get node type. */
	virtual NodeType CCL_API getNodeType () const = 0;

	/** Get node class identifier (also available via IObject/ITypeInfo). */
	virtual UIDRef CCL_API getNodeClassID () const = 0;

	/** Get node flags. */
	virtual int CCL_API getNodeFlags () const = 0;

	/** Get node name. */
	virtual StringID CCL_API getNodeName () const = 0;

	/** Set node name. */
	virtual void CCL_API setNodeName (StringID name) = 0;

	/** Set associated application data (optional). */
	virtual void CCL_API setNodeData (VariantRef data) = 0;

	/** Get associated application data. */
	virtual VariantRef CCL_API getNodeData () const = 0;

	/** Get position in parent space (optional). */
	virtual PointF3DRef CCL_API getPosition () const = 0;

	/** Set position in parent space (optional). */
	virtual tresult CCL_API setPosition (PointF3DRef p) = 0;

	/** Set X position in parent space (optional). */
	virtual tresult CCL_API setPositionX (float x) = 0;

	/** Set Y position in parent space (optional). */
	virtual tresult CCL_API setPositionY (float y) = 0;

	/** Set Z position in parent space (optional). */
	virtual tresult CCL_API setPositionZ (float z) = 0;

	/** Get yaw angle (radians) in parent space (optional). */
	virtual float CCL_API getYawAngle () const = 0;

	/** Set yaw angle (radians) in parent space (optional). */
	virtual tresult CCL_API setYawAngle (float angle) = 0;

	/** Get pitch angle (radians) in parent space (optional). */
	virtual float CCL_API getPitchAngle () const = 0;

	/** Set pitch angle (radians) in parent space (optional). */
	virtual tresult CCL_API setPitchAngle (float angle) = 0;

	/** Get roll angle (radians) in parent space (optional). */
	virtual float CCL_API getRollAngle () const = 0;

	/** Set roll angle (radians) in parent space (optional). */
	virtual tresult CCL_API setRollAngle (float angle) = 0;

	/** Get scale (x component) in parent space (optional). */
	virtual float CCL_API getScaleX () const = 0;

	/** Set scale (x component) in parent space (optional). */
	virtual tresult CCL_API setScaleX (float factor) = 0;

	/** Get scale (y component) in parent space (optional). */
	virtual float CCL_API getScaleY () const = 0;

	/** Set scale (y component) in parent space (optional). */
	virtual tresult CCL_API setScaleY (float factor) = 0;

	/** Get scale (z component) in parent space (optional). */
	virtual float CCL_API getScaleZ () const = 0;

	/** Set scale (z component) in parent space (optional). */
	virtual tresult CCL_API setScaleZ (float factor) = 0;

	/** Get the world transform matrix. */
	virtual Transform3DRef CCL_API getWorldTransform () const = 0;

	/** Get the inverse world transform matrix. */
	virtual Transform3DRef CCL_API getInverseWorldTransform () const = 0;

	/** Set the node's world transform matrix. */
	virtual void CCL_API setWorldTransform (Transform3DRef transform) = 0;

	/** Get scene (root) node. */
	virtual IScene3D* CCL_API getRootNode () = 0;

	/** Get parent node. */
	virtual ISceneNode3D* CCL_API getParentNode () = 0;

	/** Get interface to manage child nodes (optional, can be null). */
	virtual ISceneChildren3D* CCL_API getChildren () = 0;

	/** Get interface to manage node constraints. */
	virtual ISceneConstraints3D* CCL_API getConstraints () = 0;

	/** Add animation for node property. */
	virtual tresult CCL_API addAnimation (StringID propertyId, const IAnimation* animation) = 0;

	/** Remove animation for node property. */
	virtual tresult CCL_API removeAnimation (StringID propertyId) = 0;

	/** Enable hit testing. */
	virtual tresult CCL_API enableHitTesting (tbool state) = 0;

	/** Check if hit testing is enabled. */
	virtual tbool CCL_API isHitTestingEnabled () const = 0;

	/** Find a node that intersects with a given ray. Only returns nodes which have hit testing enabled. */
	virtual ISceneNode3D* CCL_API findIntersectingNode (Ray3DRef ray, float tolerance = 0.f, int flags = 0) const = 0;

	// Property identifiers
	DECLARE_STRINGID_MEMBER (kName)			///< String
	DECLARE_STRINGID_MEMBER (kParent)		///< ISceneNode3D
	DECLARE_STRINGID_MEMBER (kPosition)		///< IUIValue (PointF3D)
	DECLARE_STRINGID_MEMBER (kPositionX)	///< float
	DECLARE_STRINGID_MEMBER (kPositionY)	///< float
	DECLARE_STRINGID_MEMBER (kPositionZ)	///< float
	DECLARE_STRINGID_MEMBER (kYawAngle)		///< float
	DECLARE_STRINGID_MEMBER (kPitchAngle)	///< float
	DECLARE_STRINGID_MEMBER (kRollAngle)	///< float
	DECLARE_STRINGID_MEMBER (kScaleX)		///< float
	DECLARE_STRINGID_MEMBER (kScaleY)		///< float
	DECLARE_STRINGID_MEMBER (kScaleZ)		///< float
	
	DECLARE_IID (ISceneNode3D)
};

DEFINE_IID (ISceneNode3D, 0x60cde392, 0x6874, 0x4f7c, 0xa1, 0x48, 0xff, 0x90, 0x48, 0xc9, 0x57, 0x5)
DEFINE_STRINGID_MEMBER (ISceneNode3D, kName, "name")
DEFINE_STRINGID_MEMBER (ISceneNode3D, kParent, "parent")
DEFINE_STRINGID_MEMBER (ISceneNode3D, kPosition, "position")
DEFINE_STRINGID_MEMBER (ISceneNode3D, kPositionX, "positionX")
DEFINE_STRINGID_MEMBER (ISceneNode3D, kPositionY, "positionY")
DEFINE_STRINGID_MEMBER (ISceneNode3D, kPositionZ, "positionZ")
DEFINE_STRINGID_MEMBER (ISceneNode3D, kYawAngle, "yawAngle")
DEFINE_STRINGID_MEMBER (ISceneNode3D, kPitchAngle, "pitchAngle")
DEFINE_STRINGID_MEMBER (ISceneNode3D, kRollAngle, "rollAngle")
DEFINE_STRINGID_MEMBER (ISceneNode3D, kScaleX, "scaleX")
DEFINE_STRINGID_MEMBER (ISceneNode3D, kScaleY, "scaleY")
DEFINE_STRINGID_MEMBER (ISceneNode3D, kScaleZ, "scaleZ")

//************************************************************************************************
// ISceneChildren3D
/** Interface to manage children in a 3D scene node.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface ISceneChildren3D: IContainer
{
	/** Find child node. */
	virtual ISceneNode3D* CCL_API findNode (StringID name) const = 0;

	/** Add child node. */
	virtual tresult CCL_API addNode (ISceneNode3D* node) = 0;

	/** Remove child node. */
	virtual tresult CCL_API removeNode (ISceneNode3D* node) = 0;

	DECLARE_IID (ISceneChildren3D)
};

DEFINE_IID (ISceneChildren3D, 0xb4bbd298, 0xaca4, 0x40ec, 0xbb, 0x30, 0xb5, 0x66, 0x4c, 0x9e, 0xe9, 0x81)

//************************************************************************************************
// ISceneConstraints3D
/** Interface to manage transform constraints in a 3D scene node.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface ISceneConstraints3D: ITransformConstraints3D
{
	/** Add constraints. */
	virtual tresult CCL_API addConstraints (ITransformConstraints3D* transformConstraints) = 0;

	/** Remove constraints. */
	virtual tresult CCL_API removeConstraints (ITransformConstraints3D* transformConstraints) = 0;

	DECLARE_IID (ISceneConstraints3D)
};

DEFINE_IID (ISceneConstraints3D, 0x5b470584, 0xb31e, 0x47df, 0xb4, 0x6f, 0xe6, 0xbd, 0x22, 0x6a, 0x39, 0x88)

//************************************************************************************************
// IScene3D
/** Root node in a 3D scene.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IScene3D: ISceneNode3D
{
	enum EditFlags
	{
		kUserEdit = 1<<0, ///< a scene has been edited by user interaction
		kAnimationEdit = 1<<1 ///< a scene node has been edited as a result of an animation
	};
	
	/** Set scene handler (optional, not shared). */
	virtual void CCL_API setHandler (ISceneHandler3D* handler) = 0;

	/** Get scene handler. */
	virtual ISceneHandler3D* CCL_API getHandler () const = 0;

	/**	Start editing a scene.
		Changing visible properties of scene nodes or adding/removing nodes to/from a scene
		is only allowed within a beginEditing/endEditing block. */
	virtual void CCL_API beginEditing () = 0;

	/** End editing a scene, signals a kChanged message.
		\param node A node that has beeen edited. If nullptr, any node may have been edited.
		\param editFlags Edit flags. */
	virtual void CCL_API endEditing (ISceneNode3D* node = nullptr, int editFlags = 0) = 0;

	DECLARE_IID (IScene3D)
};

DEFINE_IID (IScene3D, 0xe15c35d1, 0x401e, 0x4976, 0x85, 0x50, 0x9f, 0x6a, 0xf4, 0x38, 0x51, 0xe)

//************************************************************************************************
// ISceneResource3D
/**	Scene resource interface.
	3D scenes defined in Skin XML can be accessed by name via ITheme::getResource().
	\ingroup gui_graphics3d */
//************************************************************************************************

interface ISceneResource3D: IUnknown
{
	/** Create new scene instance. */
	virtual IScene3D* CCL_API createScene () = 0;

	DECLARE_IID (ISceneResource3D)
};

DEFINE_IID (ISceneResource3D, 0x51d337d6, 0xd4b4, 0x49f2, 0xb9, 0x81, 0x11, 0x4, 0xa6, 0xf7, 0x89, 0x31)

//************************************************************************************************
// SceneEdit3D
/** 3D scene editing scope
	\ingroup gui_graphics3d */
//************************************************************************************************

struct SceneEdit3D
{
	IScene3D* scene;
	ISceneNode3D* node;
	int editFlags;
	
	SceneEdit3D (IScene3D* scene, ISceneNode3D* node = nullptr, int editFlags = 0)
	: scene (scene),
	  node (node),
	  editFlags (editFlags)
	{
		if(scene)
			scene->beginEditing ();
	}
	
	~SceneEdit3D ()
	{
		if(scene)
			scene->endEditing (node, editFlags);
	}
};

//************************************************************************************************
// ICamera3D
/** 3D camera interface.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface ICamera3D: ISceneNode3D
{
	/** Set the camera orientation, so that it looks at a given point. */
	virtual tresult CCL_API lookAt (PointF3DRef p, PointF3DRef upVector = SceneConstants::kWorldUpVector) = 0;

	/** Get angle for field of view in degrees. */
	virtual float CCL_API getFieldOfViewAngle () const = 0;

	/** Set angle for field of view in degrees. */
	virtual tresult CCL_API setFieldOfViewAngle (float angle) = 0;

	/** Get a ray in world space pointing from a normalized 2D coordinate (range 0 to 1) into the scene. */
	virtual Ray3D CCL_API getCameraRay (PointFRef position) const = 0;

	// property identifiers
	DECLARE_STRINGID_MEMBER (kFieldOfViewAngle)	///< float

	DECLARE_IID (ICamera3D)
};

DEFINE_IID (ICamera3D, 0xceaaf29a, 0x9cb7, 0x4924, 0xb9, 0x7c, 0x78, 0xcc, 0xe, 0xb, 0xe, 0xb4)
DEFINE_STRINGID_MEMBER (ICamera3D, kFieldOfViewAngle, "fieldOfViewAngle")

//************************************************************************************************
// ILightSource3D
/** 3D light source interface.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface ILightSource3D: ISceneNode3D
{
	/** Get light color. */
	virtual ColorRef CCL_API getLightColor () const = 0;

	/** Set light color. */
	virtual tresult CCL_API setLightColor (ColorRef color) = 0;

	/** Get mask for this light source. Mask corresponds to the stock shader implementation.
		See also IMaterial3D::setLightMask. */
	virtual uint32 CCL_API getLightMask () const = 0;

	// Property identifiers
	DECLARE_STRINGID_MEMBER (kLightColor)	///< IUIValue (Color)

	DECLARE_IID (ILightSource3D)
};

DEFINE_IID (ILightSource3D, 0xceaaf29a, 0x9cb7, 0x4924, 0xb9, 0x7c, 0x78, 0xcc, 0xe, 0xb, 0xe, 0xb4)
DEFINE_STRINGID_MEMBER (ILightSource3D, kLightColor, "lightColor")

//************************************************************************************************
// IPointLight3D
/** 3D point light interface.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IPointLight3D: ILightSource3D
{
	static constexpr float kDefaultRadius = 100.f;
	static constexpr float kDefaultMinimum = 0.01f;
	static constexpr float kDefaultLinearFactor = 0.1f;
	static constexpr float kDefaultConstantTerm = 1.f;

	/** Set light attenuation radius. */
	virtual tresult CCL_API setAttenuationRadius (float radius = kDefaultRadius) = 0;

	/** Get light attenuation radius. */
	virtual float CCL_API getAttenuationRadius () const = 0;

	/** Set light intensity at attenuation radius. */
	virtual tresult CCL_API setAttenuationMinimum (float minimum = kDefaultMinimum) = 0;

	/** Get light intensity at attenuation radius. */
	virtual float CCL_API getAttenuationMinimum () const = 0;

	/** Set attenuation linear factor. */
	virtual tresult CCL_API setAttenuationLinearFactor (float linearFactor = kDefaultLinearFactor) = 0;

	/** Get attenuation linear factor. */
	virtual float CCL_API getAttenuationLinearFactor () const = 0;

	/** Set attenuation constant term. */
	virtual tresult CCL_API setAttenuationConstantTerm (float constantTerm = kDefaultConstantTerm) = 0;

	/** Get attenuation constant term. */
	virtual float CCL_API getAttenuationConstantTerm () const = 0;

	// Property identifiers
	DECLARE_STRINGID_MEMBER (kAttenuationRadius)		///< float
	DECLARE_STRINGID_MEMBER (kAttenuationMinimum)		///< float
	DECLARE_STRINGID_MEMBER (kAttenuationLinearFactor)	///< float
	DECLARE_STRINGID_MEMBER (kAttenuationConstantTerm)	///< float

	DECLARE_IID (IPointLight3D)
};

DEFINE_IID (IPointLight3D, 0x71011d93, 0xed69, 0x4c51, 0x9b, 0x79, 0x7b, 0x6f, 0xbb, 0xc3, 0x1e, 0x4f)
DEFINE_STRINGID_MEMBER (IPointLight3D, kAttenuationRadius, "attenuationRadius")
DEFINE_STRINGID_MEMBER (IPointLight3D, kAttenuationMinimum, "attenuationMinimum")
DEFINE_STRINGID_MEMBER (IPointLight3D, kAttenuationLinearFactor, "attenuationLinearFactor")
DEFINE_STRINGID_MEMBER (IPointLight3D, kAttenuationConstantTerm, "attenuationConstantTerm")

//************************************************************************************************
// IModelNode3D
/** 3D model node interface.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IModelNode3D: ISceneNode3D
{
	/** Get model data. */
	virtual IUnknown* CCL_API getModelData () const = 0;

	/** Set model data. */
	virtual tresult CCL_API setModelData (IUnknown* data) = 0;

	DECLARE_IID (IModelNode3D)
};

DEFINE_IID (IModelNode3D, 0xf82063cb, 0x8ac5, 0x4489, 0x93, 0xaa, 0x5f, 0x2b, 0x6, 0xce, 0xd7, 0x5b)

//************************************************************************************************
// ISceneRenderer3D
/** 3D scene renderer interface provided by the framework.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface ISceneRenderer3D: IUnknown
{
	/** Get 3D scene. */
	virtual IScene3D* CCL_API getIScene () const = 0;
		
	/** Get active camera. */
	virtual ICamera3D* CCL_API getActiveICamera () const = 0;
	
	/** Set active camera. */
	virtual tresult CCL_API setActiveICamera (ICamera3D* camera) = 0;

	/** Get multisampling factor. */
	virtual int CCL_API getMultisamplingFactor () const = 0;

	/** Set multisampling factor. */
	virtual tresult CCL_API setMultisamplingFactor (int factor) = 0;

	DECLARE_IID (ISceneRenderer3D)
};

DEFINE_IID (ISceneRenderer3D, 0x3e2df2db, 0x1670, 0x4569, 0x85, 0x52, 0xfe, 0xcb, 0x16, 0xfc, 0x9e, 0xb6)

//************************************************************************************************
// ISceneHandler3D
/** Scene handler interface provided by the application.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface ISceneHandler3D: IUnknown
{
	/** Scene renderer has been attached. */
	virtual void CCL_API rendererAttached (ISceneRenderer3D& sceneRenderer) = 0;

	/** Scene renderer has been detached. */
	virtual void CCL_API rendererDetached (ISceneRenderer3D& sceneRenderer) = 0;

	/** Scene change notification
		\param node A node that has changed. If nullptr, any node may have changed.
		\param editFlags Edit flags, see IScene3D::EditFlags. */
	virtual void CCL_API sceneChanged (IScene3D& scene, ISceneNode3D* node = nullptr, int editFlags = 0) = 0;

	DECLARE_IID (ISceneHandler3D)
};

DEFINE_IID (ISceneHandler3D, 0x27ef5f93, 0xcd9a, 0x4c4b, 0xb1, 0x21, 0x30, 0x30, 0xf, 0x7e, 0x6e, 0x5e)

} // namespace CCL

#endif // _ccl_iscene3d_h
