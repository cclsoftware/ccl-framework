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
// Filename    : ccl/app/controls/usersceneview3d.cpp
// Description : User control wrapping a ISceneView3D
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/controls/usersceneview3d.h"

using namespace CCL;

//************************************************************************************************
// UserSceneView3D
//************************************************************************************************

DEFINE_CLASS_HIDDEN (UserSceneView3D, UserControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserSceneView3D::UserSceneView3D (RectRef size, StyleRef style, StringRef title)
: UserControl (size, style, title),
  upAxisFixed (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMouseHandler* CCL_API UserSceneView3D::createMouseHandler (const MouseEvent& event)
{
	if(sceneView.isValid () && !focusNode.isValid ())
		focusNode = sceneView->getSceneRenderer ().getActiveICamera ();

	if(sceneView.isValid () && focusNode.isValid ())
	{
		ArcballMouseHandler* mouseHandler = nullptr;
		if(focusNode == sceneView->getSceneRenderer ().getActiveICamera ())
			mouseHandler = NEW CameraMouseHandler (this);
		else
			mouseHandler = NEW ArcballMouseHandler (this, focusNode);
		mouseHandler->setUpAxisFixed (upAxisFixed);
		return mouseHandler;
	}
	return SuperClass::createMouseHandler (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserSceneView3D::onViewsChanged ()
{
	sceneView.release ();
	ForEachChildView (this, childView)
		sceneView = childView;
		if(sceneView.isValid ())
			break;
	EndFor

	SuperClass::onViewsChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISceneNode3D* UserSceneView3D::findNodeAt (PointRef position) const
{
	ICamera3D* camera = sceneView.isValid () ? sceneView->getSceneRenderer ().getActiveICamera () : nullptr;
	IScene3D* scene = sceneView.isValid () ? sceneView->getSceneRenderer ().getIScene () : nullptr;
	if(camera == nullptr || scene == nullptr)
		return nullptr;

	PointF normalizedPosition (float (position.x) / getWidth (), float (position.y) / getHeight ());
	Ray3D cameraRay = camera->getCameraRay (normalizedPosition);

	return scene->findIntersectingNode (cameraRay);
}

//************************************************************************************************
// UserSceneView3D::ArcballMouseHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (UserSceneView3D::ArcballMouseHandler, UserControl::MouseHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserSceneView3D::ArcballMouseHandler::ArcballMouseHandler (UserSceneView3D* userSceneView, ISceneNode3D* node, float speed, int flags)
: MouseHandler (userSceneView, flags),
  radius (0.f),
  node (node),
  speedFactor (speed),
  upAxisFixed (false)
{
	ISceneView3D* sceneView = userSceneView->getSceneView ();
	if(sceneView)
	{
		ISceneRenderer3D& sceneRenderer = sceneView->getSceneRenderer ();
		scene = sceneRenderer.getIScene ();
		camera = sceneRenderer.getActiveICamera ();
	}

	ASSERT (scene && camera)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserSceneView3D::ArcballMouseHandler::onBegin ()
{
	radius = ccl_min (control->getWidth () / 2.f, control->getHeight () / 2.f);

	cameraTransform = camera->getWorldTransform ();
	cameraTransform.resetTranslation ();

	nodeTransform = node->getWorldTransform ();
	initialScale.x = node->getScaleX ();
	initialScale.y = node->getScaleY ();
	initialScale.z = node->getScaleZ ();

	lastValidTransform = nodeTransform;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserSceneView3D::ArcballMouseHandler::onMove (int moveFlags)
{
	Point delta (current.where - previous.where);

	if(!delta.isNull ())
	{
		Transform3D transform = getTransform (previous.where, current.where);

		ITransformConstraints3D* constraints = node->getConstraints ();
		if(constraints != nullptr && !constraints->isValidTransform (transform))
		{
			PointF interpolated = PointF (current.where.x, current.where.y);
			float factor = 1.f;
			while(factor > 0.001f)
			{
				factor /= 2.f;
				if(constraints->isValidTransform (transform))
					interpolated += PointF (factor * delta.x, factor * delta.y);
				else
					interpolated -= PointF (factor * delta.x, factor * delta.y);
				transform = getTransform (previous.where, Point (ccl_to_int (interpolated.x), ccl_to_int (interpolated.y)));
			}
		}

		if(constraints == nullptr || constraints->isValidTransform (transform))
		{
			SceneEdit3D scope (scene, node, IScene3D::kUserEdit);
		
			node->setWorldTransform (transform);
			node->setScaleX (initialScale.x);
			node->setScaleY (initialScale.y);
			node->setScaleZ (initialScale.z);
			nodeTransform = node->getWorldTransform ();
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D UserSceneView3D::ArcballMouseHandler::getProjection (PointRef point) const
{
	PointF3D projection;
	projection.x = control->getWidth () / 2.f - point.x;
	projection.y = point.y - control->getHeight () / 2.f;
	float xySquared = projection.x * projection.x + projection.y * projection.y;
	projection.z = (xySquared <= radius * radius) ? ::sqrtf (radius * radius - xySquared) : 0;
	projection = projection.normal ();
	return projection;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform3D UserSceneView3D::ArcballMouseHandler::getTransform (Point first, Point last) const
{
	Transform3D rotation = getRotationTransform (first, last, upAxisFixed);
	Transform3D transform = rotation * nodeTransform;
	if(upAxisFixed)
	{
		PointF3D upVector = (transform * SceneConstants::kWorldUpVector).normal ();
		PointF3D upVector2 = PointF3D (SceneConstants::kWorldUpVector.x, upVector.y, upVector.z).normal ();
		PointF3D axis = upVector.cross (upVector2);
		if(axis.length () > 0.001f)
		{
			axis = axis.normal ();
			float angle = ::acosf (upVector.dot (upVector2));
			transform = TransformUtils3D::rotateAroundAxis (axis, angle) * transform;
		}
	}
	return transform;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform3D UserSceneView3D::ArcballMouseHandler::getRotationTransform (Point first, Point last, bool fixUpAxis) const
{
	if(fixUpAxis)
	{
		if((first - last).isNull ())
			return Transform3D ();

		PointF3D xAxis (1.f, 0.f, 0.f);
		PointF3D yAxis (0.f, 1.f, 0.f);

		cameraTransform.transform (xAxis);
		nodeTransform.transform (cameraTransform.transform (yAxis));

		float xAngle = speedFactor * float (first.y - last.y) / control->getHeight () * Math::Constants<float>::kPi;
		float yAngle = speedFactor * float (first.x - last.x) / control->getWidth () * Math::Constants<float>::kPi;

		return TransformUtils3D::rotateAroundAxis (xAxis, xAngle) * TransformUtils3D::rotateAroundAxis (yAxis, yAngle);
	}
	else
	{
		PointF3D a = getProjection (first);
		PointF3D b = getProjection (last);
		if((a - b).length () < 0.001f)
			return Transform3D ();

		PointF3D axis = a.cross (b) * (1.f / a.length ()) * (1.f / b.length ());

		Transform3D transform = cameraTransform;
		transform.transform (axis);
		axis = axis.normal ();

		float dotProduct = a.dot (b);
		float angle = speedFactor * ::acosf (dotProduct);
		Transform3D rotation = TransformUtils3D::rotateAroundAxis (axis, angle);

		return rotation;
	}
}

//************************************************************************************************
// UserSceneView3D::CameraMouseHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (UserSceneView3D::CameraMouseHandler, UserControl::MouseHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserSceneView3D::CameraMouseHandler::CameraMouseHandler (UserSceneView3D* userSceneView, PointF3DRef initialLookAt, int flags)
: ArcballMouseHandler (userSceneView, nullptr, flags),
  initialLookAt (initialLookAt)
{
	node = camera;
	speedFactor = -1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserSceneView3D::CameraMouseHandler::onMove (int moveFlags)
{
	Point delta (current.where - first.where);

	if(current.keys.isSet (KeyState::kOption))
	{
		// modify look at position
		constexpr float kMeterPerPixel = 0.025f;

		PointF3D lookAt (initialLookAt);
		lookAt.x += float (delta.x * kMeterPerPixel);
		lookAt.y += float (delta.y * kMeterPerPixel);
		
		SceneEdit3D scope (scene, camera, IScene3D::kUserEdit);
		camera->lookAt (lookAt);
	}
	else
	{
		return SuperClass::onMove (moveFlags);
	}
	return true;
}
