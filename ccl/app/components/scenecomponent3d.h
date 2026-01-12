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
// Filename    : ccl/app/components/scenecomponent3d.h
// Description : 3D Scene Component
//
//************************************************************************************************

#ifndef _ccl_scenecomponent3d_h
#define _ccl_scenecomponent3d_h

#include "ccl/app/component.h"

#include "ccl/app/controls/usersceneview3d.h"

namespace CCL {

//************************************************************************************************
// SceneComponent3D
//************************************************************************************************

class SceneComponent3D: public Component,
						public ISceneHandler3D
{
public:
	DECLARE_CLASS (SceneComponent3D, Component)

	SceneComponent3D (StringRef name = nullptr, StringRef title = nullptr);

	PROPERTY_SHARED_POINTER (ICamera3D, mainCamera, MainCamera)

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	tresult CCL_API terminate () override;
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	// ISceneHandler3D
	void CCL_API rendererAttached (ISceneRenderer3D& sceneRenderer) override;
	void CCL_API rendererDetached (ISceneRenderer3D& sceneRenderer) override;
	void CCL_API sceneChanged (IScene3D& scene, ISceneNode3D* node, int editFlags) override;

	CLASS_INTERFACE (ISceneHandler3D, Component)

protected:
	AutoPtr<IScene3D> scene;
	int viewCount;

	virtual void buildScene ();
	virtual void releaseScene ();
	virtual UserSceneView3D* createSceneView (RectRef bounds);

	virtual void moveCameraForward (CoordF distance); ///< moves camera towards look at position
	virtual void turnCamera (float yaw, float pitch); ///< rotates camera
};

} // namespace CCL

#endif // _ccl_scenecomponent3d_h
