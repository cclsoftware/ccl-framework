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
// Filename    : ccl/app/components/scenecomponent3d.cpp
// Description : 3D Scene Component
//
//************************************************************************************************

#include "ccl/app/components/scenecomponent3d.h"

using namespace CCL;

//************************************************************************************************
// SceneComponent3D
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SceneComponent3D, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneComponent3D::SceneComponent3D (StringRef name, StringRef title)
: Component (name, title),
  viewCount (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API SceneComponent3D::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "UserSceneView3D")
	{
		return *createSceneView (bounds);
	}
	return SuperClass::createView (name, data, bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API SceneComponent3D::getObject (StringID name, UIDRef classID)
{
	if(name == "Scene")
	{
		if(!scene.isValid ())
			buildScene ();
		return scene;
	}
	return SuperClass::getObject (name, classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SceneComponent3D::terminate ()
{
	releaseScene ();
	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SceneComponent3D::rendererAttached (ISceneRenderer3D& sceneRenderer)
{
	viewCount++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SceneComponent3D::rendererDetached (ISceneRenderer3D& sceneRenderer)
{
	ASSERT (viewCount > 0)

	viewCount--;
	if(viewCount <= 0)
	{
		releaseScene ();
		viewCount = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SceneComponent3D::sceneChanged (IScene3D& scene, ISceneNode3D* node, int editFlags)
{}

////////////////////////////////////////////////////////////////////////////////////////////////// 

tbool CCL_API SceneComponent3D::checkCommandCategory (CStringRef category) const
{
	if(SuperClass::checkCommandCategory (category))
		return true;
	return category == "Navigation" || category == "Zoom";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SceneComponent3D::interpretCommand (const CommandMsg& msg)
{
	if(SuperClass::interpretCommand (msg))
		return true;

	if(!mainCamera.isValid ())
		return false;

	if(msg.checkOnly ())
		return true;

	if(msg.category == "Navigation")
	{
		constexpr CoordF kMoveStep = 1.f;
		constexpr float kAngleStep = .01f;

		if(msg.name == "Left")
			turnCamera (-kAngleStep, 0);
		else if(msg.name == "Right")
			turnCamera (kAngleStep, 0);

		else if(msg.name == "Up")
			moveCameraForward (kMoveStep);
		else if(msg.name == "Down")
			moveCameraForward (-kMoveStep);

		else if(msg.name == "Up Skip") // Cmd+Up
			turnCamera (0, kAngleStep);
		else if(msg.name == "Down Skip") // Cmd+Down
			turnCamera (0, -kAngleStep);

		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////// 

void SceneComponent3D::moveCameraForward (CoordF distance)
{
	if(!mainCamera.isValid ())
		return;

	SceneEdit3D scope (scene);

	Transform3DRef cameraTransform = mainCamera->getWorldTransform ();

	PointF3D translationVector (0.f, 0.f, distance);

	Transform3D cameraRotationTransform = cameraTransform;
	cameraRotationTransform.resetTranslation ();
	cameraRotationTransform.resetScale ();

	translationVector = cameraRotationTransform * translationVector;
	Transform3D translationTransform;
	translationTransform.translate (translationVector);

	Transform3D resultTransform = translationTransform * cameraTransform;
	ITransformConstraints3D* constraints = mainCamera->getConstraints ();
	if(constraints == nullptr || constraints->isValidTransform (resultTransform))
		mainCamera->setWorldTransform (resultTransform);
}

////////////////////////////////////////////////////////////////////////////////////////////////// 

void SceneComponent3D::turnCamera (float yaw, float pitch)
{
	if(!mainCamera.isValid ())
		return;

	SceneEdit3D scope (scene);

	mainCamera->setYawAngle (mainCamera->getYawAngle () + yaw);
	mainCamera->setPitchAngle (mainCamera->getPitchAngle () + pitch);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneComponent3D::buildScene ()
{
	if(scene)
		scene->setHandler (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneComponent3D::releaseScene ()
{
	if(scene)
		scene->setHandler (nullptr);
	scene.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserSceneView3D* SceneComponent3D::createSceneView (RectRef bounds)
{
	UserSceneView3D* sceneView = NEW UserSceneView3D (bounds);
	return sceneView;
}
