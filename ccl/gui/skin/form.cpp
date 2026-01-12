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
// Filename    : ccl/gui/skin/form.cpp
// Description : Form class
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/skin/form.h"

#include "ccl/gui/windows/popupwindow.h"
#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/skin/skinwizard.h"
#include "ccl/gui/skin/skinmodel.h"
#include "ccl/public/base/iobjectnode.h"
#include "ccl/public/gui/iviewfactory.h"

using namespace CCL;
using namespace SkinElements;

//************************************************************************************************
// Form
//************************************************************************************************

DEFINE_CLASS (Form, ImageView)
DEFINE_CLASS_UID (Form, 0x1528b171, 0xcd36, 0x44d3, 0x81, 0xd7, 0xeb, 0x5f, 0xcd, 0xa8, 0x62, 0x1b)

//////////////////////////////////////////////////////////////////////////////////////////////////

Form::Form (SkinWizard* wizard, const Rect& size, StyleRef style, StringRef title)
: ImageView (nullptr, size, style, title),
  wizard (wizard),
  windowStyle (Styles::defaultWindowStyle)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Form::~Form ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Form::getController () const
{ 
	return controller; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Form::setController (IUnknown* c)
{ 
	controller = c; 
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Form::setSkinElement (FormElement* e)
{
	skinElement = e;

	if(skinElement)
		windowStyle = skinElement->getWindowStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinElements::FormElement* Form::getSkinElement () const
{ 
	return skinElement;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API Form::getFormName () const
{
	return skinElement ? skinElement->getName () : CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StyleRef CCL_API Form::getWindowStyle () const
{ 
	return windowStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Form::setWindowStyle (StyleRef style)
{
	windowStyle = style;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Form::calcSizeLimits ()
{
	if(skinElement && skinElement->getSizeLimits ().isValid ())
	{
		sizeLimits = skinElement->getSizeLimits ();

		// allow specifying -1 in skin for kMaxCoord
		if(sizeLimits.maxWidth < 0)
			sizeLimits.maxWidth = kMaxCoord;
		if(sizeLimits.maxHeight < 0)
			sizeLimits.maxHeight = kMaxCoord;
	}
	else
		ImageView::calcSizeLimits ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef Form::getHelpIdentifier () const
{
	if(skinElement && !skinElement->getHelpIdentifier ().isEmpty () && !getStyle ().isCommonStyle (Styles::kNoHelpId))
		return skinElement->getHelpIdentifier ();
	return SuperClass::getHelpIdentifier ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* Form::findFirstFocusView (View* parent, StringRef firstFocus)
{
	if(parent->getName () == firstFocus)
		return parent;

	// recursion for each child view
	ForEachViewFast (*parent, view)
		if(View* child = findFirstFocusView (view, firstFocus))
			return child;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* Form::findFirstFocusView ()
{
	if(!firstFocus.isEmpty ())
		return findFirstFocusView (this, firstFocus);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* CCL_API Form::openWindow (IWindow* parentWindow)
{
	return open (unknown_cast<Window> (parentWindow));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Form::closeWindow ()
{
	close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Form::reload ()
{
	removeAll ();

	if(skinElement && controller && wizard)
	{
		if(SkinWizard::isReloadingSkin ()) // don't crash during skin reload
			return;

		// while creating child views, we must temporarily take our original (potentially zoomed) size from skin,
		// because childs are described relative to that size
		Rect oldSize (getSize ());
		Rect skinSize (skinElement->getSize ());
		skinSize.zoom (wizard->getZoomFactor ());

		// "decouple" from parent while creating child views
		View* savedParent = parent;
		if(parent)
		{
			removed (parent);
			parent = nullptr;
		}

		skinSize.moveTo (oldSize.getLeftTop ());
		setSize (skinSize);

		wizard->createChildElements (skinElement, controller, this, skinElement);

		parent = savedParent;
		if(parent)
			attached (parent);

		tbool autoH = skinSize.getWidth () <= 0;
		tbool autoV = skinSize.getHeight () <= 0;
		if(autoH || autoV)
			autoSize (autoH, autoV);

		setSize (oldSize);
	}

	if(!hasExplicitSizeLimits ())
		resetSizeLimits ();

	checkFitSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* Form::open (Window* parentWindow)
{
	if(getWindow ())
		return getWindow ();

	Point pos (getSize ().getLeftTop ());	
	StyleFlags wStyle (getWindowStyle ());
	ThemeSelector selector (getTheme ());

	Window* w = NEW PopupWindow (getSize (), wStyle, getTitle (), parentWindow);

	setPosition (Point ()); // move this to (0,0)
	setSizeMode (getSizeMode () | kAttachAll);

	if(hasVisualStyle ())
		w->setVisualStyle (visualStyle);
	else
		w->setVisualStyle (getTheme ().getStandardStyle (ThemePainter::kBackgroundRenderer));

	w->addView (this);
	w->checkSizeLimits ();
	w->setController (controller);
	w->setName (name);
	w->setHelpIdentifier (getHelpIdentifier ());

	if(pos.isNull ()) // if at (0,0) then center window
		w->center ();
		
	w->show ();
	w->addToDesktop ();
	w->activate ();
	return w;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Form::close ()
{
	retain (); // increment refcount

	Window* w = getWindow ();
	if(w && !w->close ())
	{
		release (); // decrement refcount
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISkinElement* CCL_API Form::getISkinElement () const
{
	return skinElement;
}

//************************************************************************************************
// FormDelegateView
//************************************************************************************************

BEGIN_STYLEDEF (FormDelegateView::customStyles)
	{"deferredremove",	Styles::kFormDelegateViewBehaviorDeferredRemove},
	{"keepview",		Styles::kFormDelegateViewBehaviorKeepView},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (FormDelegateView, View)

//////////////////////////////////////////////////////////////////////////////////////////////////

FormDelegateView::FormDelegateView (SkinWizard* wizard, const Rect& size, StyleRef style, StringRef title)
: View (size, style, title),
  wizard (wizard)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FormDelegateView::sizeChild (View* view)
{
	Rect childRect (view->getSize ());

	// stretch child if attached to us
	if((view->getSizeMode () & (kAttachLeft|kAttachRight)) == (kAttachLeft|kAttachRight))
		childRect.setWidth (getWidth ());
	if((view->getSizeMode () & (kAttachTop|kAttachBottom)) == (kAttachTop|kAttachBottom))
		childRect.setHeight (getHeight ());

	// but respect sizeLimits of child
	view->getSizeLimits ().makeValid (childRect);
	view->setSize (childRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FormDelegateView::attached (View* parent)
{
	SuperClass::attached (parent);
	isAttachedInternal (true);

	CCL_PRINTF ("%sFormDelegateView %s %x attached\n", CCL_INDENT, formName.str (), this)
	CCL_ADD_INDENT (2)

	ASSERT (getChildren ().isEmpty () || style.isCustomStyle (Styles::kFormDelegateViewBehaviorDeferredRemove|Styles::kFormDelegateViewBehaviorKeepView))

	if(style.isCustomStyle (Styles::kFormDelegateViewBehaviorKeepView) && getFirst ())
		return;

    IUnknown* controller = formController;
    if(!subControllerName.isEmpty ())
    {
        UnknownPtr<IObjectNode> iNode (formController);
		ASSERT (iNode)
        controller = iNode ? iNode->lookupChild (String (subControllerName)) : nullptr;
		if(controller == nullptr)
		{
			SKIN_WARNING (nullptr, "Controller not found for Delegate '%s': '%s'", formName.str (), subControllerName.str ())
			CCL_DEBUGGER ("Controller not found for Delegate.\n");
			return;
		}
    }
	
	View* view = nullptr;
	
	UnknownPtr<IViewFactory> viewFactory (controller);

	ASSERT (wizard)
	if(viewFactory && wizard)
	{
		SkinArgumentScope scope (*wizard, &formArguments);
		view = unknown_cast<View> (viewFactory->createView (formName, Variant (), getSize ()));
	}

	if(!view)
	{
		Theme::ZoomFactorScope scope (getTheme (), getZoomFactor ()); // apply our zoom factor during view creation
		view = unknown_cast<View> (getTheme ().createView (formName, controller, &formArguments));
	}
	
	ASSERT (view)
	if(view)
	{
		sizeChild (view);
		
		// resize to child, but only if we don't have to maintain our own attachment
		Rect delegateRect (getSize ());
		if((getSizeMode () & (kAttachLeft|kAttachRight)) != (kAttachLeft|kAttachRight))
			delegateRect.setWidth (view->getWidth ());
		if((getSizeMode () & (kAttachTop|kAttachBottom)) != (kAttachTop|kAttachBottom))
			delegateRect.setHeight (view->getHeight ());

		setSize (delegateRect);

		sizeChild (view); // size child again (setSize can lead to adjustments from parents, which are not applied in onSize before view is actually added)

		if(!isAttachedInternal ())
		{
			// removed () could have been called already during setSize (), e.g. when this is inside a layout with style kLayoutHidePriority
			view->release ();
			return;
		}

		if(style.isCustomStyle (Styles::kFormDelegateViewBehaviorDeferredRemove))
		{
			removeAll ();

			if(!hasExplicitSizeLimits ())
				resetSizeLimits ();
		}

		addView (view);
		CCL_PRINTF ("%s-> delegate: (%d,%d) child: (%d,%d)\n", CCL_INDENT, getWidth (), getHeight (), view->getWidth (), view->getHeight ())
	}

	if(parent)
		parent->onChildLimitsChanged (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FormDelegateView::removed (View* parent)
{
	CCL_PRINTF ("%sFormDelegateView %s %x removed\n", CCL_INDENT, formName.str (), this)
	if(!style.isCustomStyle (Styles::kFormDelegateViewBehaviorDeferredRemove|Styles::kFormDelegateViewBehaviorKeepView))
	{
		removeAll (); // remove children first to avoid doubled removed() call

		if(!hasExplicitSizeLimits ())
			resetSizeLimits ();
	}

	SuperClass::removed (parent);
	isAttachedInternal (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FormDelegateView::onSize (const Point& delta)
{
	if(View* view = getFirst ())
	{
		// resolve conflicts between child sizeLimits and attachment flags:
		// bypass attachment and size the child to the best of our knowledge
		sizeChild (view);
		CCL_PRINTF ("%sFormDelegateView %s onSize (%d, %d) delegate: (%d,%d) child: (%d,%d)\n", CCL_INDENT, formName.str (), delta.x, delta.y, getWidth (), getHeight (), view->getWidth (), view->getHeight ())
	}

	ScopedFlag<kAttachDisabled> disableAttach (sizeMode);
	SuperClass::onSize (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FormDelegateView::calcSizeLimits ()
{
	// limits of our only child
	if(View* view = getFirst ())
		sizeLimits = view->getSizeLimits ();
	else
		sizeLimits.setUnlimited ();
}
