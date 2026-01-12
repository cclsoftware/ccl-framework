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
// Filename    : ccl/app/editing/scaleview.cpp
// Description : Scale View
//
//************************************************************************************************

#include "ccl/app/editing/scaleview.h"
#include "ccl/app/editing/scale.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/gui/framework/ipresentable.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Scale")
	XSTRING (Zoom, "Zoom")
	XSTRING (Move, "Move")
	XSTRING (MoveZoom, "Move, Zoom")
END_XSTRINGS

namespace CCL {

//************************************************************************************************
// ScaleScrollHandler
//************************************************************************************************

class ScaleScrollHandler: public UserControl::MouseHandler
{
public:
	ScaleScrollHandler (ScaleView* view)
	: MouseHandler (view, kAutoScroll),
	  zoomer (view->getScale ())
	{}

	void onBegin () override
	{
		zoomer.setZoomLock (first.where);

		if(Scale* scale = ((ScaleView*)control)->getScale ())
			scale->getZoomParam ()->beginEdit ();
	}

	bool onMove (int moveFlags) override
	{
		Scale* scale = ((ScaleView*)control)->getScale ();

		int deltaX = first.where.x - current.where.x;
		int deltaY = current.where.y - first.where.y;
		if(scale->getOrientation () == Scale::kHorizontal)
		{
			ccl_swap<int> (deltaX, deltaY);
			deltaY *= -1;
			deltaX *= -1;
		}

		float deltaZ = (float)deltaX / 300.f;
		zoomer.zoom (deltaZ, deltaY);
		return true;
	}

	void onRelease (bool canceled) override
	{
		if(Scale* scale = ((ScaleView*)control)->getScale ())
			scale->getZoomParam ()->endEdit ();

		MouseHandler::onRelease (canceled);
	}

protected:
	ScaleZoomer zoomer;
};

} //  namespace CCL

using namespace CCL;

//************************************************************************************************
// ScaleView
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ScaleView, UserControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScaleView::ScaleView (Scale* _scale, RectRef size, StyleRef style, StringRef title)
: UserControl (size, style, title),
  scale (nullptr)
{
	if(_scale)
		setScale (_scale);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScaleView::~ScaleView ()
{
	setScale (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScaleView::setScale (Scale* _scale)
{
	share_and_observe<Scale> (this, scale, _scale);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Scale* ScaleView::getScale () const
{
	return scale;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScaleView::draw (const DrawEvent& event)
{
	//...
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScaleView::getHelp (IHelpInfoBuilder& helpInfo, bool canDragZoom)
{
	if(canDragZoom)
		helpInfo.addOption (KeyState::kShift|KeyState::kDrag, nullptr, XSTR (MoveZoom));

	// see Scale::applyMouseWheel
	helpInfo.addOption (KeyState::kWheel, nullptr, XSTR (Move));
	helpInfo.addOption (KeyState::kWheel|KeyState::kCommand, nullptr, XSTR (Zoom));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScaleView::updateHelp (const MouseEvent& event)
{
	if(System::GetHelpManager ().hasInfoViewers ())
	{
		AutoPtr<IHelpInfoBuilder> helpInfo;
		if(event.eventType != MouseEvent::kMouseLeave)
		{
			helpInfo = ccl_new<IHelpInfoBuilder> (CCL::ClassID::HelpInfoBuilder);
			if(getHelp (*helpInfo))
				System::GetHelpManager ().showInfo (UnknownPtr<IPresentable> (helpInfo));
		}
		System::GetHelpManager ().showInfo (UnknownPtr<IPresentable> (helpInfo));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScaleView::onMouseEnter (const MouseEvent& event)
{
	SuperClass::onMouseEnter (event);
	updateHelp (event);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScaleView::onMouseMove (const MouseEvent& event)
{
	SuperClass::onMouseMove (event);
	updateHelp (event);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScaleView::onMouseLeave (const MouseEvent& event)
{
	SuperClass::onMouseLeave (event);
	updateHelp (event);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScaleView::onMouseWheel (const MouseWheelEvent& event)
{
	if(SuperClass::onMouseWheel (event))
		return true;

	scale->applyMouseWheel (event);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMouseHandler* CCL_API ScaleView::createMouseHandler (const MouseEvent& event)
{
	if(scale)
		return NEW ScaleScrollHandler (this);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScaleView::notify (ISubject* subject, MessageRef msg)
{
	if(subject == scale)
	{
		if(getStyle ().isDirectUpdate ())
			updateClient ();
		else
			invalidate ();
	}
}
