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
// Filename    : ccl/app/controls/usersceneview3d.h
// Description : User control wrapping a ISceneView3D
//
//************************************************************************************************

#ifndef _ccl_usersceneview3d_h
#define _ccl_usersceneview3d_h

#include "ccl/app/controls/usercontrol.h"

#include "ccl/public/math/mathprimitives.h"

#include "ccl/public/gui/framework/iview3d.h"
#include "ccl/public/gui/graphics/3d/iscene3d.h"

namespace CCL {

//************************************************************************************************
// UserSceneView3D
//************************************************************************************************

class UserSceneView3D: public UserControl
{
public:
	DECLARE_CLASS (UserSceneView3D, UserControl)

	UserSceneView3D (RectRef size = Rect (), StyleRef style = 0, StringRef title = nullptr);

	PROPERTY_SHARED_POINTER (ISceneNode3D, focusNode, FocusNode)
	PROPERTY_BOOL (upAxisFixed, UpAxisFixed)

	ISceneView3D* getSceneView () const { return sceneView; }

	// UserControl
	IMouseHandler* CCL_API createMouseHandler (const MouseEvent& event) override;
	void onViewsChanged () override;

	CLASS_INTERFACE (IUserControl, Object)

protected:
	class ArcballMouseHandler;
	class CameraMouseHandler;
	
	UnknownPtr<ISceneView3D> sceneView;

	ISceneNode3D* findNodeAt (PointRef position) const;
};

//********************************************************************************************
// UserSceneView3D::ArcballMouseHandler
/** Rotates a node using a virtual sphere in the camera plane. */
//********************************************************************************************

class UserSceneView3D::ArcballMouseHandler: public MouseHandler
{
public:
	DECLARE_CLASS (ArcballMouseHandler, MouseHandler)

	ArcballMouseHandler (UserSceneView3D* userSceneView = nullptr, ISceneNode3D* node = nullptr, float speed = 1.f, int flags = 0);

	PROPERTY_BOOL (upAxisFixed, UpAxisFixed)

	// MouseHandler
	void onBegin () override;
	bool onMove (int moveFlags) override;

protected:
	SharedPtr<IScene3D> scene;
	SharedPtr<ICamera3D> camera;
	SharedPtr<ISceneNode3D> node;

	PointF3D initialScale;
	float radius;

	Transform3D cameraTransform;
	Transform3D nodeTransform;
	Transform3D lastValidTransform;

	float speedFactor;

	PointF3D getProjection (PointRef point) const;
	Transform3D getTransform (Point first, Point last) const;
	Transform3D getRotationTransform (Point first, Point last, bool fixUpAxis = true) const;
	
	void signalFocusNodeChanged () const;
};

//********************************************************************************************
// UserSceneView3D::CameraMouseHandler
/** Rotates the camera using a virtual sphere in the camera plane. */
//********************************************************************************************

class UserSceneView3D::CameraMouseHandler: public ArcballMouseHandler
{
public:
	DECLARE_CLASS (CameraMouseHandler, ArcballMouseHandler)

	CameraMouseHandler (UserSceneView3D* userSceneView = nullptr, PointF3DRef initialLookAt = PointF3D (), int flags = 0);

	// MouseHandler
	bool onMove (int moveFlags) override;

protected:
	PointF3D initialLookAt;
};

} // namespace CCL

#endif // _ccl_usersceneview3d_h
