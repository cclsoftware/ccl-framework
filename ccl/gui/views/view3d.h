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
// Filename    : ccl/gui/views/view3d.h
// Description : 3D View class
//
//************************************************************************************************

#ifndef _ccl_view3d_h
#define _ccl_view3d_h

#include "ccl/gui/views/view.h"

#include "ccl/public/gui/graphics/3d/igraphics3d.h"
#include "ccl/public/gui/framework/iview3d.h"
#include "ccl/public/gui/icommandhandler.h"

namespace CCL {

class Native3DSurface;
class SceneRenderer3D;
class Scene3D;
class Camera3D;

//************************************************************************************************
// View3D
//************************************************************************************************

class View3D: public View,
			  public IView3D,
			  public IGraphicsContent3D
{
public:
	DECLARE_CLASS_ABSTRACT (View3D, View)

	View3D (const Rect& size = Rect ());
	~View3D ();

	// View
	void attached (View* parent) override;
	void removed (View* parent) override;
	void onSize (const Point& delta) override;
	void onMove (const Point& delta) override;
	void invalidate () override;
	void CCL_API invalidate (RectRef rect) override;

	CLASS_INTERFACE2 (IView3D, IGraphicsContent3D, View)

protected:
	Native3DSurface* surface;

	Rect getSurfaceSize () const;
	void updateSurfaceSize ();
};

//************************************************************************************************
// UserView3D
//************************************************************************************************

class UserView3D: public View3D
{
public:
	DECLARE_CLASS (UserView3D, View3D)

	// View3D
	tresult CCL_API set3DContent (IUnknown* content) override;
	tresult CCL_API createContent (IGraphicsFactory3D& factory) override;
	tresult CCL_API releaseContent () override;
	tresult CCL_API renderContent (IGraphics3D& graphics) override;
	tresult CCL_API getContentProperty (Variant& value, ContentProperty3D propertyId) const override;

protected:
	UnknownPtr<IGraphicsContent3D> content;
};

//************************************************************************************************
// SceneView3D
//************************************************************************************************

class SceneView3D: public View3D,
				   public ISceneView3D
{
public:
	DECLARE_CLASS (SceneView3D, View3D)

	SceneView3D (const Rect& size = Rect ());
	~SceneView3D ();

	SceneRenderer3D& getRenderer ();

	// View3D
	void attached (View* parent) override;
	void removed (View* parent) override;
	tresult CCL_API set3DContent (IUnknown* content) override;
	tresult CCL_API createContent (IGraphicsFactory3D& factory) override;
	tresult CCL_API releaseContent () override;
	tresult CCL_API renderContent (IGraphics3D& graphics) override;
	tresult CCL_API getContentProperty (Variant& value, ContentProperty3D propertyId) const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// ISceneView3D
	ISceneRenderer3D& CCL_API getSceneRenderer () override;

	CLASS_INTERFACE (ISceneView3D, View3D)

protected:
	class CameraMouseHandler;

	SceneRenderer3D* renderer;

	void initRenderer ();
	void setSceneInternal (Scene3D* scene);
};

} // namespace CCL

#endif // _ccl_view3d_h
