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
// Filename    : ccl/public/gui/framework/viewbox.cpp
// Description : View Box
//
//************************************************************************************************

#include "ccl/public/gui/framework/viewbox.h"

#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/iform.h"
#include "ccl/public/gui/graphics/igraphicslayer.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/base/iobject.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// FormBox
//************************************************************************************************

bool FormBox::isForm (IView* view)
{
	return UnknownPtr<IForm> (view) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FormBox::FormBox (IView* view)
: ViewBox (view)
{
	ASSERT (getForm () != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FormBox::FormBox (RectRef size, StyleRef windowStyle, StringRef title)
{
	IForm* form = ccl_new<IForm> (ClassID::Form);
	ASSERT (form != nullptr)
	if(!form)
		return;

	view = UnknownPtr<IView> (form);
	ASSERT (view != nullptr)
	construct (size, 0, title);
	form->setWindowStyle (windowStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IForm* FormBox::getForm () const
{
	return UnknownPtr<IForm> (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StyleFlags FormBox::getWindowStyle () const
{
	return getForm ()->getWindowStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FormBox::setWindowStyle (StyleRef style)
{
	getForm ()->setWindowStyle (style);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FormBox::setController (IUnknown* controller)
{
	getForm ()->setController (controller);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* FormBox::openWindow ()
{
	return getForm ()->openWindow ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FormBox::closeWindow ()
{
	getForm ()->closeWindow ();
}

//************************************************************************************************
// ControlBox
//************************************************************************************************

ControlBox::ControlBox (IView* view)
: ViewBox (view)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ControlBox::ControlBox (UIDRef classID, IParameter* param, RectRef size, StyleRef style, StringRef title)
: ViewBox (classID, size, style, title)
{
	if(param)
		setParameter (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlBox::setParameter (IParameter* param)
{
	UnknownPtr<IControl> control (view);
	ASSERT (control != nullptr)
	if(control)
		control->setParameter (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ControlBox::getParameter () const
{
	UnknownPtr<IControl> control (view);
	ASSERT (control != nullptr)
	return control ? control->getParameter () : nullptr;
}

//************************************************************************************************
// ViewBox
//************************************************************************************************

ITheme* ViewBox::moduleTheme = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::setModuleTheme (ITheme* theme)
{
	moduleTheme = theme;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITheme* ViewBox::getModuleTheme ()
{
	return moduleTheme;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewBox::ViewBox (UIDRef classID, RectRef size, StyleRef style, StringRef title)
{
	view = ccl_new<IView> (classID);
	ASSERT (view != nullptr)
	if(!view)
		return;

	construct (size, style, title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewBox::ViewBox (IUnknown* unknown)
: view (UnknownPtr<IView> (unknown))
{
	ASSERT (view != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::construct (RectRef size, StyleRef style, StringRef title)
{
	// assign theme of current module
	if(moduleTheme)
		setTheme (moduleTheme);

	Coord w = size.getWidth ();
	Coord h = size.getHeight ();
	if(w > 0 && h > 0)
		setSize (size);
	else if(w > 0)
		setSize (Rect (getSize ()).setWidth (w));
	else if(h > 0)
		setSize (Rect (getSize ()).setHeight (h));

	if(!style.isEmpty ())
		setStyle (style);
	if(!title.isEmpty ())
		setTitle (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDRef ViewBox::getClassID () const
{
	UnknownPtr<IObject> iObject (view);
	return iObject ? iObject->getTypeInfo ().getClassID () : kNullUID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::setName (StringRef name)
{
	view->setViewAttribute (IView::kName, name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ViewBox::getName () const
{
	Variant v;
	view->getViewAttribute (v, IView::kName);
	return v.asString ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::setTitle (StringRef title)
{
	view->setViewAttribute (IView::kTitle, title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ViewBox::getTitle () const
{
	Variant v;
	view->getViewAttribute (v, IView::kTitle);
	return v.asString ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::setTooltip (StringRef title)
{
	view->setViewAttribute (IView::kTooltip, title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ViewBox::getTooltip () const
{
	Variant v;
	view->getViewAttribute (v, IView::kTooltip);
	return v.asString ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::setTooltipTrackingEnabled (bool state)
{
	view->setViewAttribute (IView::kTooltipTrackingEnabled, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewBox::isTooltipTrackingEnabled () const
{
	Variant v;
	view->getViewAttribute (v, IView::kTooltipTrackingEnabled);
	return v.asBool ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::setStyle (StyleRef style)
{
	view->setViewAttribute (IView::kStyleFlags, style.toLargeInt ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITheme& ViewBox::getTheme () const
{
	Variant v;
	view->getViewAttribute (v, IView::kTheme);
	ITheme* theme = UnknownPtr<ITheme> (v);
	ASSERT (theme != nullptr)
	return *theme;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::setTheme (ITheme* theme)
{
	view->setViewAttribute (IView::kTheme, theme);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::setVisualStyle (IVisualStyle* visualStyle)
{
	view->setViewAttribute (IView::kVisualStyle, visualStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::setVisualStyle (const IVisualStyle& visualStyle)
{
	setVisualStyle (const_cast<IVisualStyle*> (&visualStyle));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ViewBox::getController () const
{
	Variant v;
	view->getViewAttribute (v, IView::kController);
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* ViewBox::getWindow () const
{
	return view->getIWindow ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ViewBox::getSizeMode () const
{
	Variant v;
	view->getViewAttribute (v, IView::kSizeMode);
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::setSizeMode (int flags)
{
	view->setViewAttribute (IView::kSizeMode, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::disableSizeMode (bool state)
{
	view->setViewAttribute (IView::kSizeModeDisabled, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewBox::isEnabled () const
{
	Variant v;
	view->getViewAttribute (v, IView::kInputEnabled);
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::enable (bool state)
{
	view->setViewAttribute (IView::kInputEnabled, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ViewBox::getMouseState () const
{
	Variant v;
	view->getViewAttribute (v, IView::kMouseState);
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::setMouseState (int state)
{
	int oldState = getMouseState ();
	if(state != oldState)
	{
		view->setViewAttribute (IView::kMouseState, state);
		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ViewBox::getThemeElementState () const
{
	Variant v;
	view->getViewAttribute (v, IView::kThemeElementState);
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewBox::wantsFocus () const
{
	Variant v;
	view->getViewAttribute (v, IView::kFocusEnabled);
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::wantsFocus (bool state)
{
	view->setViewAttribute (IView::kFocusEnabled, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewBox::isLayerBackingEnabled () const
{
	Variant v;
	view->getViewAttribute (v, IView::kLayerBackingEnabled);
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::setLayerBackingEnabled (bool state)
{
	view->setViewAttribute (IView::kLayerBackingEnabled, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* ViewBox::getGraphicsLayer () const
{
	Variant v;
	view->getViewAttribute (v, IView::kGraphicsLayer);
	return UnknownPtr<IGraphicsLayer> (v.asUnknown ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewBox::isAccessibilityEnabled () const
{
	Variant v;
	view->getViewAttribute (v, IView::kAccessibilityEnabled);
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::setAccessibilityEnabled (bool state)
{
	view->setViewAttribute (IView::kAccessibilityEnabled, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewBox::getAttribute (Variant& value, const char* id) const
{
	UnknownPtr<IObject> iObject (view);
	if(iObject)
		return iObject->getProperty (value, id) != 0;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewBox::setAttribute (const char* id, VariantRef value)
{
	UnknownPtr<IObject> iObject (view);
	if(iObject)
		return iObject->setProperty (id, value) != 0;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewBox::setHelpIdentifier (StringRef id)
{
	return setAttribute (IView::kHelpId, id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& ViewBox::getClientRect (Rect& r) const
{
	Rect size (view->getSize ());
	r (0, 0, size.getWidth (), size.getHeight ());
	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::setPosition (PointRef pos)
{
	Rect r (getSize ());
	r.moveTo (pos);
	setSize (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::invalidate ()
{
	Rect r;
	getClientRect (r);
	invalidate (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::updateClient ()
{
	Rect r;
	getClientRect (r);
	updateClient (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBox::scrollClient (RectRef rect, PointRef delta)
{
	view->scrollClient (rect, delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewBox::makeVisible (RectRef rect, tbool relaxed)
{
	return view->makeVisible (rect, relaxed) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& ViewBox::clientToWindow (Point& p) const
{
	return view->clientToWindow (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& ViewBox::windowToClient (Point& p) const
{
	return view->windowToClient (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& ViewBox::clientToScreen (Point& p) const
{
	return view->clientToScreen (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& ViewBox::screenToClient (Point& p) const
{
	return view->screenToClient (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* ViewBox::getViewAt (int index) const
{
	int i = 0;
	ForEachChildView (view, child)
		if(i == index)
			return child;
		i++;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ViewBox::getViewIndex (const IView* childView) const
{
	int i = 0;
	ForEachChildView (view, child)
		if(child == childView)
			return i;
		i++;
	EndFor
	return -1;
}
