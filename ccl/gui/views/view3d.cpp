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
// Filename    : ccl/gui/views/view3d.cpp
// Description : 3D View class
//
//************************************************************************************************

#include "ccl/gui/views/view3d.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/3d/scene/scenerenderer3d.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/theme/visualstyleclass.h"

namespace CCL {

DECLARE_VISUALSTYLE_CLASS (SceneView3D)

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// View3D
//************************************************************************************************

DEFINE_CLASS_HIDDEN (View3D, View)

//////////////////////////////////////////////////////////////////////////////////////////////////

View3D::View3D (const Rect& size)
: View (size),
  surface (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

View3D::~View3D ()
{
	ASSERT (surface == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect View3D::getSurfaceSize () const
{
	Point offset;
	clientToWindow (offset);

	Rect surfaceSize;
	getClientRect (surfaceSize);
	surfaceSize.offset (offset);
	return surfaceSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View3D::updateSurfaceSize ()
{
	if(surface)
	{
		Rect surfaceSize = getSurfaceSize ();
		surface->setSize (surfaceSize);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View3D::attached (View* parent)
{
	auto* window = getWindow ();
	ASSERT (window && !surface)
	if(window && !surface)
	{
		if(auto* support3d = NativeGraphicsEngine::instance ().get3DSupport ())
		{
			surface = support3d->create3DSurface ();
			ASSERT (surface)
			if(surface)
			{
				Rect surfaceSize = getSurfaceSize ();
				surface->setSize (surfaceSize);
				surface->setContent (this);
				
				auto& factory3d = support3d->get3DFactory ();
				createContent (factory3d);

				window->getRenderTarget ()->add3DSurface (surface);
			}
		}
		else
		{
			CCL_WARN ("3D graphics not supported!\n", 0)
		}
	}
	
	SuperClass::attached (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View3D::removed (View* parent)
{
	SuperClass::removed (parent);

	if(surface)
	{
		auto* window = getWindow ();
		ASSERT (window)

		if(!window->isInDestroyEvent ())
			window->getRenderTarget ()->remove3DSurface (surface);
		
		releaseContent ();
		
		surface->setContent (nullptr);
		safe_release (surface);
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View3D::onSize (const Point& delta)
{
	updateSurfaceSize ();
	SuperClass::onSize (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View3D::onMove (const Point& delta)
{
	updateSurfaceSize ();
	SuperClass::onMove (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View3D::invalidate ()
{
	if(surface)
		surface->setDirty ();
	SuperClass::invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API View3D::invalidate (RectRef rect)
{
	if(surface)
		surface->setDirty ();
	SuperClass::invalidate (rect);
}

//************************************************************************************************
// UserView3D
//************************************************************************************************

DEFINE_CLASS (UserView3D, View3D)
DEFINE_CLASS_UID (UserView3D, 0x87ba8c3d, 0x8e1, 0x4e7f, 0xa8, 0x68, 0xae, 0x56, 0x9f, 0x26, 0x85, 0xb5)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserView3D::set3DContent (IUnknown* _content)
{
	content = _content;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserView3D::createContent (IGraphicsFactory3D& factory)
{
	if(content)
		return content->createContent (factory);
	else
		return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserView3D::releaseContent ()
{
	if(content)
		return content->releaseContent ();
	else
		return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserView3D::renderContent (IGraphics3D& graphics)
{
	if(content)
		return content->renderContent (graphics);
	else
		return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserView3D::getContentProperty (Variant& value, ContentProperty3D propertyId) const
{
	if(content)
		return content->getContentProperty (value, propertyId);
	else
		return kResultFalse;
}

//************************************************************************************************
// SceneView3D
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (SceneView3D, VisualStyle, "SceneView3DStyle")
	ADD_VISUALSTYLE_STRING  ("camera")
	ADD_VISUALSTYLE_METRIC  ("multisamplingfactor")
END_VISUALSTYLE_CLASS (SceneView3D)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (SceneView3D, View3D)
DEFINE_CLASS_UID (SceneView3D, 0xde8bfddc, 0x1708, 0x4de5, 0x96, 0x98, 0xd3, 0x3c, 0x8c, 0xfc, 0x5f, 0xac)

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneView3D::SceneView3D (const Rect& size)
: View3D (size),
  renderer (NEW SceneRenderer3D)
{
	wantsFocus (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneView3D::~SceneView3D ()
{
	setSceneInternal (nullptr);
	renderer->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneRenderer3D& SceneView3D::getRenderer ()
{
	return *renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISceneRenderer3D& CCL_API SceneView3D::getSceneRenderer ()
{
	return getRenderer (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneView3D::attached (View* parent)
{
	initRenderer ();

	SuperClass::attached (parent);

	if(auto* scene = renderer->getScene ())
		if(ISceneHandler3D* handler = scene->getHandler ())
			handler->rendererAttached (*renderer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneView3D::removed (View* parent)
{
	if(auto* scene = renderer->getScene ())
		if(ISceneHandler3D* handler = scene->getHandler ())
			handler->rendererDetached (*renderer);

	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SceneView3D::set3DContent (IUnknown* content)
{
	auto* scene = unknown_cast<Scene3D> (content);
	setSceneInternal (scene);

	if(isAttached ())
		initRenderer ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneView3D::initRenderer ()
{
	// select active camera
	Camera3D* camera = nullptr;
	if(auto* scene = renderer->getScene ())
	{
		CString cameraName = getVisualStyle ().getString ("camera");
		if(!cameraName.isEmpty ())
			camera = scene->findNode<Camera3D> (cameraName);
		if(!camera)
			camera = scene->getFirstOfType<Camera3D> ();
	}

	renderer->setActiveCamera (camera);

	// multisampling factor
	int multisamplingFactor = getVisualStyle ().getMetric<int> ("multisamplingfactor", SceneRenderer3D::kDefaultMultisamplingFactor);
	ccl_lower_limit (multisamplingFactor, 1); // upper limit is checked by platform
	renderer->setMultisamplingFactor (multisamplingFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneView3D::setSceneInternal (Scene3D* scene)
{
	Scene3D* oldScene = renderer->getScene ();
	if(scene != oldScene)
	{
		if(oldScene)
			oldScene->removeObserver (this);

		renderer->setScene (scene);

		if(scene)
			scene->addObserver (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SceneView3D::createContent (IGraphicsFactory3D& factory)
{
	return renderer->createContent (factory);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SceneView3D::releaseContent ()
{
	return renderer->releaseContent ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SceneView3D::renderContent (IGraphics3D& graphics)
{
	return renderer->renderContent (graphics);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SceneView3D::getContentProperty (Variant& value, ContentProperty3D propertyId) const
{
	return renderer->getContentProperty (value, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SceneView3D::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && subject == renderer->getScene ())
	{
		renderer->sceneChanged ();
		invalidate ();
	}
	else
		SuperClass::notify (subject, msg);
}
