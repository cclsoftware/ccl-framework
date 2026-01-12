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
// Filename    : ccl/gui/controls/variantview.h
// Description : Variant View
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/controls/variantview.h"

#include "ccl/gui/layout/layoutprimitives.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/views/viewanimation.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/controlproperties.h"

#include "ccl/base/trigger.h" // Property class

using namespace CCL;

//************************************************************************************************
// VariantView::HideViewHandler
//************************************************************************************************

class VariantView::HideViewHandler: public Object,
									public IAnimationCompletionHandler
{
public:
	HideViewHandler (View* view)
	: view (view)
	{
		view->isHidden (true);
		if(View* first = view->getFirst ())
			if(auto layer = first->getGraphicsLayer ())
				layer->setOpacity (0.f);
	}
	
	// IAnimationCompletionHandler
	void CCL_API onAnimationFinished () override
	{
		view->isHidden (false);
		if(View* first = view->getFirst ())
			if(auto layer = first->getGraphicsLayer ())
				layer->setOpacity (1.f);

		view->invalidate ();
	}
	
	CLASS_INTERFACE (IAnimationCompletionHandler, Object)
	
protected:
	SharedPtr<View> view;
};

//************************************************************************************************
// VariantView
//************************************************************************************************

BEGIN_STYLEDEF (VariantView::customStyles)
	{"boundvalue",		Styles::kVariantViewBehaviorBoundValue},
	{"selectalways", 	Styles::kVariantViewBehaviorSelectAlways},
	{"invert", 			Styles::kVariantViewBehaviorInvert},
	{"unifysizes",		Styles::kVariantViewLayoutUnifySizes},
	{"fill",			Styles::kVariantViewLayoutFill},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (VariantView, Control)
DEFINE_CLASS_UID (VariantView, 0x571f5de8, 0x5f97, 0x4f42, 0xb2, 0xc2, 0x98, 0xab, 0xb6, 0x56, 0xb2, 0xc1)

//////////////////////////////////////////////////////////////////////////////////////////////////

VariantView::VariantView (IUnknown* controller, const Rect& size, IParameter* param, StyleRef style)
: Control (size, param, style),
  controller (controller),
  currentIndex (-2),
  transitionType (Styles::kTransitionNone),
  suppressTransition (false)
{
	setWheelEnabled (false);
	setContextMenuEnabled (false);
	enable (true); // always enabled
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VariantView::VariantView (IUnknown* controller, const Rect& size, CStringRef propertyId, StyleRef style)
: Control (size, nullptr, style),
  controller (controller),
  propertyId (propertyId),
  currentIndex (-2),
  transitionType (Styles::kTransitionNone),
  suppressTransition (false)
{
	setWheelEnabled (false);
	setContextMenuEnabled (false);
	enable (true); // always enabled

	// observe the controller for property changes
	if(isPropertyMode ())
	{
		UnknownPtr<ISubject> subject (controller);
		if(subject)
			subject->addObserver (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VariantView::VariantView ()
: currentIndex (-2),
  transitionType (Styles::kTransitionNone),
  suppressTransition (false)
{
	setWheelEnabled (false);
	setContextMenuEnabled (false);
	enable (true); // always enabled
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VariantView::~VariantView ()
{
	if(View* view = getFirst ())
		removeView (view);

	variants.objectCleanup (true);
	variants.removeAll ();

	if(isPropertyMode ())
	{
		UnknownPtr<ISubject> subject (controller);
		if(subject)
			subject->removeObserver (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::onChildsAdded ()
{
	updateSelectedElement  (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API VariantView::getController () const
{
	return controller;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef VariantView::getHelpIdentifier () const
{
	return View::getHelpIdentifier ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VariantView::isPropertyMode ()
{
	return !propertyId.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::attached (View* parent)
{
	// Call base class first to avoid double-attaching our visible child!
	SuperClass::attached (parent);

	// select a view
	ScopedVar<bool> scope (suppressTransition, true);
	updateSelectedElement (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::removed (View* parent)
{
	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	// forward to hidden views
	View* activeView = getFirst ();
	ForEach (variants, View, v)
		if(v != activeView)
			v->onColorSchemeChanged (event);
	EndFor

	SuperClass::onColorSchemeChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::draw (const UpdateRgn& updateRgn)
{
	View::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::updateSelectedElement (bool observedChanged)
{
	int index = 0;
	if(isPropertyMode ())
	{
		UnknownPtr<IObject> iObject (controller);
		Variant value;
		Property (iObject, propertyId).get (value);
		index = value.asInt ();
	}
	else
	{
		if(param->getType () == IParameter::kString)
			index = !param->getValue ().asString ().isEmpty ();
		else
			index = param->getValue ().asInt ();
	}

	if(getStyle ().isCustomStyle (Styles::kVariantViewBehaviorBoundValue))
		index = ccl_bound (index, 0, variants.count () - 1);
	else
	{
		// if there is only one view, we assume on/off behavior
		if(variants.count () == 1)
			index -= 1;
	}

	if(getStyle ().isCustomStyle (Styles::kVariantViewBehaviorInvert))
		index = index ? 0 : 1;

	Window::UpdateCollector uc (getWindow ()); // avoid scroll-redraw problems
	Window::SizeChangeCollector sizeChangeCollector (getWindow ()); // prevent multiple size changes

	selectElement (index, observedChanged);

	enable (true); // always enabled
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::paramChanged ()
{
	SharedPtr<Unknown> lifeGuard (this);
	updateSelectedElement (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VariantView::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kPropertyChanged && isEqualUnknown (subject, controller))
	{
		if(isPropertyMode () && msg.getArgCount () > 0) // filter other properties
			if(msg.getArg (0).asString () != String (propertyId))
				return;
		
		updateSelectedElement (true);
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VariantView::addView (View* view)
{
	variants.add (view);
	//if(variants.count () == 1)
	//	selectElement (0);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::onSize (const Point& delta)
{	
	if(getStyle ().isCustomStyle (Styles::kVariantViewLayoutFill))
	{
		// only resize selected view
		if(!isSizeModeDisabled ())
			if(View* view = getFirst ())
				view->setSize (Rect (0, 0, getWidth (), getHeight ()));
	}
	else
	{
		checkInvalidate (delta);

		// we must adjust all variants 
		LayoutPrimitives::resizeChildViews (variants, size, delta, isSizeModeDisabled ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::onViewsChanged ()
{
	// don't checkFitSize here, will be done finally in selectElement
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::calcSizeLimits ()
{
	sizeLimits.setUnlimited ();

	if(View* view = getFirst ())
	{
		const SizeLimit& childLimits = view->getSizeLimits ();

		if((view->getSizeMode () & (kAttachLeft|kAttachRight)) == (kAttachLeft|kAttachRight))
		{
			// child gets resized with us, so we promote it's limits upwards
			sizeLimits.minWidth = childLimits.minWidth;
			sizeLimits.maxWidth = childLimits.maxWidth;
		}
		else if(getSizeMode () & kHFitSize) // we are bound to the childs current size
			sizeLimits.setFixedWidth (view->getWidth ());

		if((view->getSizeMode () & (kAttachTop|kAttachBottom)) == (kAttachTop|kAttachBottom))
		{
			sizeLimits.minHeight = childLimits.minHeight;
			sizeLimits.maxHeight = childLimits.maxHeight;
		}
		else if(getSizeMode () & kVFitSize)
			sizeLimits.setFixedHeight (view->getHeight ());
	}
	else
	{
		if(getSizeMode () & kHFitSize)
			sizeLimits.setFixedWidth (0);
		if(getSizeMode () & kVFitSize)
			sizeLimits.setFixedHeight (0);
	}

	#if DEBUG_LOG
	MutableCString t (getTitle ());
	CCL_PRINTF ("VariantView %s: sizeLimits (%d, %d, %d, %d)\n", t.str (), sizeLimits.minWidth,sizeLimits.minHeight,sizeLimits.maxWidth,sizeLimits.maxHeight);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TransitionType VariantView::getTransitionType (int index) const
{
	bool needsInverse = (index < variants.index (getFirst ()));
	return needsInverse ? ViewAnimator::getInverseTransition (transitionType) : transitionType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantView::selectElement (int index, bool observedChanged)
{
	if(index == currentIndex)
	{
		if(observedChanged == false || getStyle ().isCustomStyle (Styles::kVariantViewBehaviorSelectAlways) == false)
			return;
	}

	AutoPtr<ViewAnimator> animator;
	if(suppressTransition == false)
		animator = ViewAnimator::create (this, getTransitionType (index));
	
	if(isHidden ())			// reset hidden state of variantView (set by HideViewHandler)
		isHidden (false); 	// This might be case, when a previous animation is still running
	
	View* newView = nullptr;
	View* oldView = getFirst ();
	if(oldView)
	{
		if(animator)
			animator->snipFromView (oldView);
		removeView (oldView);
	}

	if(index >= 0)
	{
		newView = (View*)variants.at (index);
		if(newView)
		{
			if(getStyle ().isCustomStyle (Styles::kVariantViewLayoutFill))
			{
				Rect size (0, 0, getWidth (), getHeight ());
				newView->setSize (size);
			}
			else if(getStyle ().isCustomStyle (Styles::kVariantViewLayoutUnifySizes))
			{							
				// take width / height from old view (or use variant view if there is no old view)
				Rect size (newView->getSize ());
				if(getStyle ().isHorizontal ())
				{
					if(oldView)
						size.setWidth (oldView->getWidth ());
					else
						size.setWidth (getWidth ());
				}
				if(getStyle ().isVertical ())
				{
					if(oldView)
						size.setHeight (oldView->getHeight ());
					else
						size.setHeight (getHeight ());
				}

				newView->setSize (size);
			}

			SuperClass::addView (newView);
			if(animator)
				animator->snipToView (newView);
		}
	}

	if((privateFlags & kExplicitSizeLimits) == 0)
		privateFlags &= ~kSizeLimitsValid;

	checkFitSize ();

	currentIndex = index;

	if(animator)
	{
		if(hideDuringAnimation (animator, newView, oldView))
			animator->setCompletionHandler (AutoPtr<IAnimationCompletionHandler> (NEW HideViewHandler (this)));
		
		animator->makeTransition ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VariantView::hideDuringAnimation (ViewAnimator* animator, View* newView, View* oldView)
{
	if(animator->isFromLayerAnimationOnly ()) // newView is already onscreen (beneath)
		return false;
	else
	{
		auto transparentOrTranslucent = [](View* view)
		{
			return view && (view->getStyle ().isTranslucent () || view->getStyle ().isTransparent ());
		};
		
		return transparentOrTranslucent (newView) || transparentOrTranslucent (oldView);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VariantView::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == kVariantViewTransitionType)
	{
		setTransitionType (var);
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}
