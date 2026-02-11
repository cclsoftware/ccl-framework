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
// Filename    : ccl/gui/views/view.cpp
// Description : View class
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/views/view.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/views/viewaccessibility.h"
#include "ccl/gui/views/viewanimation.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/theme/thememanager.h"
#include "ccl/gui/layout/layoutprimitives.h"
#include "ccl/gui/layout/directions.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/gui/system/mousecursor.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/base/irecognizer.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/gui/framework/themeelements.h"
#include "ccl/public/gui/framework/idragndrop.h"
#include "ccl/public/gui/framework/iusercontrol.h" // for ViewEvent
#include "ccl/public/plugins/iobjecttable.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/message.h"
#include "ccl/base/kernel.h"

#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization - done here to work when CCL is linked statically
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (View, kFrameworkLevelFirst)
{
	ViewAnimationHandler::instance ().registerSelf (true);
	return true;
}

CCL_KERNEL_TERM_LEVEL (View, kFrameworkLevelFirst)
{
	ViewAnimationHandler::instance ().registerSelf (false);
}

//************************************************************************************************
// DrawViewContext
//************************************************************************************************

namespace DrawViewContext
{
	static bool isRendering = false;
	static View* currentView = nullptr;
	static Point currentOffset;
};

//************************************************************************************************
// ThemeSelector
//************************************************************************************************

Theme* ThemeSelector::currentTheme = nullptr;

//************************************************************************************************
// SizeLimitsMemento
//************************************************************************************************

void SizeLimitsMemento::store (View& view)
{
	limits = view.getSizeLimits ();
	isExplicit = (view.privateFlags & View::kExplicitSizeLimits) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SizeLimitsMemento::restore (View& view)
{
	view.sizeLimits = limits;
	if(isExplicit)
		view.privateFlags |= (View::kExplicitSizeLimits|View::kSizeLimitsValid);
	else
		view.privateFlags |= View::kSizeLimitsValid;
}

//************************************************************************************************
// View
//************************************************************************************************

BEGIN_STYLEDEF (View::commonStyles)
	{"horizontal",   Styles::kHorizontal},
	{"vertical",     Styles::kVertical},
	{"border",       Styles::kBorder},
	{"transparent",  Styles::kTransparent},
	{"directupdate", Styles::kDirectUpdate},
	{"composited",   Styles::kComposited|Styles::kDirectUpdate},
	{"translucent",  Styles::kTranslucent},
	{"trigger",		 Styles::kTrigger},
	{"small",        Styles::kSmall},
	{"left",         Styles::kLeft},
	{"right",        Styles::kRight},
	{"middle",       Styles::kMiddle},
	{"layerupdate",  Styles::kLayerUpdate},
	{"nohelp",		 Styles::kNoHelpId},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (View::resizeStyles)
	{"all",		kAttachAll},
	{"left",	kAttachLeft},
	{"top",		kAttachTop},
	{"right",	kAttachRight},
	{"bottom",	kAttachBottom},
	{"hcenter",	kHCenter},
	{"vcenter",	kVCenter},
	{"hfit",	kHFitSize},
	{"vfit",	kVFitSize},
	{"fitsize",	kFitSize},
	{"prefercurrent", kPreferCurrentSize},
	{"fill",	kFill},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (View::propertyNames)
	{"mousestate",	View::kMouseState}, // keep most used on top!
	{"style",		View::kVisualStyle},
	{"enabled",		View::kInputEnabled},
	{"name",		View::kName},
	{"title",		View::kTitle},
	{"tooltip",		View::kTooltip},
	{"theme",		View::kTheme},
	{"controller",	View::kController},
	// TODO: expose more view attributes via IObject...
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (View)
	DEFINE_METHOD_NAME ("invalidate")
	DEFINE_METHOD_NAME ("takeFocus")
	DEFINE_METHOD_NAME ("makeVisible")
END_METHOD_NAMES (View)

//////////////////////////////////////////////////////////////////////////////////////////////////

Theme& View::getDefaultTheme ()
{
	return ThemeManager::instance ().getDefaultTheme ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::isRendering ()
{
	return DrawViewContext::isRendering;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (View, Linkable)
DEFINE_CLASS_UID (View, 0xbc8b82de, 0x3695, 0x42b5, 0xa5, 0xd7, 0x84, 0xae, 0x9b, 0x35, 0x5d, 0xaf)

//////////////////////////////////////////////////////////////////////////////////////////////////

View::View (const Rect& size, StyleRef style, StringRef title)
: parent (nullptr),
  size (size),
  style (style),
  title (title),
  sizeLimits (0, 0, kMaxCoord, kMaxCoord),
  sizeMode (0),
  mouseState (0),
  privateFlags (kAccessible),
  zoomFactor (1.f),
  graphicsDevice (nullptr),
  accessibilityProvider (nullptr)  
{
	setTheme (ThemeSelector::currentTheme);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View::~View ()
{
	privateFlags |= kWasDestroyed;
	ASSERT (graphicsDevice == nullptr)

	safe_release (accessibilityProvider);

	if(visualStyle)
		visualStyle->unuse (this);

	GUI.viewDestroyed (this);

	removeAll ();

	if(privateFlags & kWasObserved) // skip useless lookup in SignalHandler if nobody cares
		signal (Message (kDestroyed));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API View::addObserver (IObserver* observer)
{
	privateFlags |= kWasObserved;

	SuperClass::addObserver (observer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* View::getParent (MetaClassRef typeId) const
{
	View* p = getParent ();
	while(p)
	{
		if(p->canCast (typeId))
			return p;
		p = p->getParent ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API View::getParentByClass (UIDRef cid) const
{
	View* p = getParent ();
	while(p)
	{
		if(p->myClass ().getClassID () == cid)
			return p;
		p = p->getParent ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::isEmpty () const
{
	return views.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API View::removeAll ()
{
	ListForEachLinkable (views, View, v)
		removeView (v);
		v->release ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* View::getWindow ()
{
	return parent ? parent->getWindow () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* View::getWindowForUpdate (WindowUpdateInfo& updateInfo)
{
	if(graphicsLayer)
		updateInfo.collectUpdates = true;

	updateInfo.offset.x += size.left;
	updateInfo.offset.y += size.top;

	return parent ? parent->getWindowForUpdate (updateInfo) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* CCL_API View::getIWindow () const
{
	return const_cast<View*> (this)->getWindow ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::isAttached ()
{
	Window* w = getWindow ();
	return w && w->isAttached ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::addView (View* view)
{
	ASSERT (view->getParent () == nullptr)
	if(view->getParent ())
		return false;

	view->parent = this;
	views.append (view);
	LayoutPrimitives::checkCenter (size, *view);

	if((privateFlags & kExplicitSizeLimits) == 0)
		privateFlags &= ~kSizeLimitsValid;

	if(isAttached ())
	{
		SharedPtr<View> lifeGuard (view);
		view->attached (this);
		if(view->parent != this)
			return false;
	}

	if(privateFlags & kActive)
		view->onActivate (true);

	view->invalidate ();

	onViewsChanged ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::insertView (int index, View* view)
{
	ASSERT (view->getParent () == nullptr)
	if(view->getParent ())
		return false;

	view->parent = this;

	View* viewBefore = (View*)views.at (index);
	if(viewBefore)
		views.insertBefore (viewBefore, view);
	else
		views.append (view);

	if((privateFlags & kExplicitSizeLimits) == 0)
		privateFlags &= ~kSizeLimitsValid;

	if(isAttached ())
		view->attached (this);

	if(privateFlags & kActive)
		view->onActivate (true);

	view->invalidate ();

	onViewsChanged ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::removeView (View* view)
{
	ASSERT (view->getParent () == this)
	if(view->getParent () != this)
		return false;

	if(isAttached ())
		view->removed (this);

	views.remove (view);
	view->parent = nullptr;

	if(privateFlags & kWasDestroyed)
		return true;
	
	struct HasBeenDrawnResetter
	{
		static void resetDeep (View* view)
		{
			view->hasBeenDrawn (false);
			ListForEachLinkableFast (view->views, View, v)
				resetDeep (v);
			EndFor
		}
	};
	HasBeenDrawnResetter::resetDeep (view);

	if((privateFlags & kExplicitSizeLimits) == 0)
		privateFlags &= ~kSizeLimitsValid;

	invalidate (view->getSize ());

	if(Window* window = getWindow ())
		window->onViewRemoved (view);

	onViewsChanged ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::moveViewBefore (View* view, View* before)
{
	ASSERT (view->getParent () == this) // sanity checks like this save our asses sometimes
	if(view->getParent () != this)
		return false;

	if(before)
	{
		ASSERT (before->getParent () == this)
		if(before->getParent () != this)
			return false;
	}

	views.remove (view);
	if(before && views.insertBefore (before, view))
		return true;

	views.append (view);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::onViewsChanged ()
{
	checkFitSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::toFront (View* view)
{
	ASSERT (view->getParent () == this)
	if(views.remove (view))
	{
		views.append (view);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::toBack (View* view)
{
	ASSERT (view->getParent () == this)
	if(views.remove (view))
	{
		views.prepend (view);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::attached (View* parent)
{
	if(isLayerBackingEnabled ())
		makeGraphicsLayer (true);

	makeAccessibilityProvider (privateFlags & kAccessible);

	ListForEachLinkableFast (views, View, v)
		v->attached (this);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::removed (View* parent)
{
	ListForEachLinkableFast (views, View, v)
		v->removed (this);
	EndFor

	makeAccessibilityProvider (false);

	if(isLayerBackingEnabled ())
		makeGraphicsLayer (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::onActivate (bool state)
{
	if(state)
		privateFlags |= kActive;
	else
		privateFlags &= ~kActive;

	ListForEachLinkableFast (views, View, v)
		v->onActivate (state);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::isChild (View* view, bool deep) const
{
	ListForEachLinkableFast (views, View, v)
		if(v == view)
			return true;

		if(deep && v->isChild (view, true))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* View::findView (const Point& where, bool deep) const
{
	ListForEachLinkableFastReverse (views, View, v)
		Point where2 (where);
		where2.offset (-v->getSize ().left, -v->getSize ().top);
		if(v->isInsideClient (where2))
		{
			if(deep)
			{
				View* result = v->findView (where2, true);
				if(result)
					return result;
			}
			return v;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::findAllViews (Container& cont, const Point& where, bool deep) const
{
	// traverse in draw order: first parent, then childs
	ListForEachLinkableFast (views, View, v)
		Point where2 (where);
		where2.offset (-v->getSize ().left, -v->getSize ().top);
		if(v->isInsideClient (where2))
		{
			cont.add (v);
			if(deep)
				v->findAllViews (cont, where2, deep);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* View::findView (const IRecognizer& recognizer) const
{
	if(recognizer.recognize (const_cast<View*> (this)->asUnknown ()))
		return const_cast<View*> (this);

	ListForEachLinkableFastReverse (views, View, view)
		if(View* child = view->findView (recognizer))
			return child;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RectRef CCL_API View::getSize () const
{
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API View::setSize (RectRef newSize, tbool doInvalidate)
{
	if(newSize != size)
	{
		CCL_PRINTF ("%s::setSize %d %d %d %d\n", myClass ().getPersistentName (), newSize.left, newSize.top, newSize.right, newSize.bottom)

		ScopedFlag<kResizing> flagSetter (privateFlags);

		int deltaX = newSize.getWidth () - size.getWidth ();
		int deltaY = newSize.getHeight () - size.getHeight ();
		int moveX  = newSize.left - size.left;
		int moveY  = newSize.top - size.top;

		bool isWindow = this == getWindow ();
		bool sized = deltaX != 0 || deltaY != 0;
		bool moved = moveX  != 0 || moveY  != 0;

		// don't invalidate window while moving!
		if(doInvalidate && !isWindow && moved)
			invalidate ();

		size = newSize;

		if(doInvalidate && !isWindow && moved)
			invalidate ();

		if(sized)
		{
			Point delta (deltaX, deltaY);
			onSize (delta);

			if(sizeMode & kAttachDisabledOnce) // reset kAttachDisabled flag
				sizeMode &= ~(kAttachDisabled|kAttachDisabledOnce);

			// notify parent
			if(parent)
				parent->onChildSized (this, delta);
		}

		// moving window does not cause onMove!
		if(moved && !isWindow)
			onMove (Point (moveX, moveY));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::setPosition (const Point& pos)
{
	Rect r (0, 0, getWidth (), getHeight ());
	r.moveTo (pos);
	setSize (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& View::getPosition (Point& pos) const
{
	pos (size.left, size.top);
	return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point View::getPosition () const
{
	Point pos;
	getPosition (pos);
	return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& View::getClientRect (Rect& cr) const
{
	cr.left = cr.top = 0;
	cr.right = size.getWidth ();
	cr.bottom = size.getHeight ();
	return cr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::getVisibleClient (Rect& r) const
{
	View* p = parent;
	Coord hoffset = -size.left, voffset = -size.top;
	r = size;

	while(p)
	{
		Rect& psize = p->size;
		Coord pw = psize.getWidth ();
		Coord ph = psize.getHeight ();

		if(r.left < 0)
			r.left = 0;
		if(r.top < 0)
			r.top = 0;
		if(r.right > pw)
			r.right = pw;
		if(r.bottom > ph)
			r.bottom = ph;

		if(r.isEmpty ())
			return false;

		r.offset (psize.left, psize.top);
		hoffset -= psize.left;
		voffset -= psize.top;

		p = p->parent;
	}

	r.offset (hoffset, voffset);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool View::getVisibleClientForUpdate (Rect& r) const
{
	if(isLayerBackingEnabled ())
	{
		r (0, 0, size.getWidth (), size.getHeight ()); // can draw on the full layer
		return true;
	}

	// otherwise draw only in the area visible on screen, until we reach a parent layer (copied from getVisibleClient)
	View* p = parent;
	Coord hoffset = -size.left, voffset = -size.top;
	r = size;

	while(p)
	{
		Rect& psize = p->size;
		Coord pw = psize.getWidth ();
		Coord ph = psize.getHeight ();

		if(r.left < 0)
			r.left = 0;
		if(r.top < 0)
			r.top = 0;
		if(r.right > pw)
			r.right = pw;
		if(r.bottom > ph)
			r.bottom = ph;

		if(r.isEmpty ())
			return false;

		if(p->isLayerBackingEnabled ())
			break;

		r.offset (psize.left, psize.top);
		hoffset -= psize.left;
		voffset -= psize.top;

		p = p->parent;
	}

	r.offset (hoffset, voffset);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API View::setZoomFactor (float factor)
{
	zoomFactor = factor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API View::getZoomFactor () const
{
	return zoomFactor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::disableSizeMode (bool state)
{
	if(state)
		sizeMode |= kAttachDisabled;
	else
		sizeMode &= ~kAttachDisabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::checkInvalidate (const Point& delta)
{
	if(delta.x > 0)
	{
		Rect rect;
		getClientRect (rect);
		rect.left = rect.right - delta.x;
		invalidate (rect);
	}
	else if(delta.x < 0 && parent)
	{
		Rect rect (size);
		rect.left = rect.right;
		rect.right = rect.left - delta.x;
		parent->invalidate (rect);
	}

	if(delta.y > 0)
	{
		Rect rect;
		getClientRect (rect);
		rect.top = rect.bottom - delta.y;
		invalidate (rect);
	}
	else if(delta.y < 0 && parent)
	{
		Rect rect (size);
		rect.top = rect.bottom;
		rect.bottom = rect.top - delta.y;
		parent->invalidate (rect);

		if(delta.x < 0)
		{
			// shrinked in both directions: also invalidate the bottom right corner
			rect (size.right, size.bottom, size.right - delta.x, size.bottom - delta.y);
			parent->invalidate (rect);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::onSize (const Point& delta)
{
	if(graphicsLayer)
		graphicsLayer->setSize (size.getWidth (), size.getHeight ());

	checkInvalidate (delta);

	if((sizeMode & kAttachDisabled) == 0)
	{
		ListForEachLinkableFast (views, View, v)
			int a = v->sizeMode;
			if((a & (kAttachAll|kHCenter|kVCenter)) != 0)
			{
				Rect r (v->getSize ());

				if((a & kAttachLeft) && (a & kAttachRight))
					r.right += delta.x;
				else if((a & kAttachRight) != 0)
					r.offset (delta.x);
				else if(a & kHCenter)
				{
					Coord w = r.getWidth ();
					r.left = (size.getWidth () - w) / 2;
					r.setWidth (w);
				}

				if((a & kAttachTop) && (a & kAttachBottom))
					r.bottom += delta.y;
				else if((a & kAttachBottom) != 0)
					r.offset (0, delta.y);
				else if(a & kVCenter)
				{
					Coord h = r.getHeight ();
					r.top = (size.getHeight () - h) / 2;
					r.setHeight (h);
				}

				if(r != v->getSize ())
					v->setSize (r);
			}
		EndFor
	}
	else
	{
		// center must be checked anyway
		ListForEachLinkableFast (views, View, v)
			LayoutPrimitives::checkCenter (size, *v);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::checkFitSize ()
{
	tbool fitH = (getSizeMode () & kHFitSize) != 0;
	tbool fitV = (getSizeMode () & kVFitSize) != 0;
	if(fitH || fitV)
		autoSize (fitH, fitV);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::constrainSize (Rect& rect) const
{
	// delegate if there is only one child that always fills the full client area
	if(!views.isMultiple ())
		if(View* child = getFirst ())
			if(child->getSizeMode () == (kAttachAll|kFitSize) && child->getSize () == Rect (0, 0, getWidth (), getHeight ()))
				child->constrainSize (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::flushLayout ()
{
	ListForEachLinkableFast (views, View, v)
		v->flushLayout ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::onMove (const Point& delta)
{
	// Note: some views need to know when their absolute position changes!
	ListForEachLinkableFast (views, View, v)
		v->onMove (Point ());
	EndFor

	if(graphicsLayer)
	{
		Point offset;
		getParentLayer (offset);
		graphicsLayer->setOffset (offset);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::onChildSized (View* child, const Point& delta)
{
	if((privateFlags & kExplicitSizeLimits) == 0)
		privateFlags &= ~kSizeLimitsValid;

	if(!isResizing ()) // not if the child is being sized by ourselves in onSize
	{
		checkFitSize ();
		LayoutPrimitives::checkCenter (size, *child);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::onChildLimitsChanged (View* child)
{
//	CCL_PRINTF ("%s::onChildLimitsChanged:  %s\n" ,myClass ().getPersistentName (), child->myClass ().getPersistentName ())
	if((privateFlags & kExplicitSizeLimits) == 0)
	{
		privateFlags &= ~kSizeLimitsValid;
		if(parent)
			parent->onChildLimitsChanged (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::calcAutoSize (Rect& r)
{
	// join size of subviews
	if(!views.isEmpty ())
	{
		ListForEachLinkableFast (views, View, v)
			r.join (v->getSize ());
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API View::autoSize (tbool horizontal, tbool vertical)
{
	Rect calculated;
	calcAutoSize (calculated);

	if(privateFlags & kExplicitSizeLimits)
		sizeLimits.makeValid (calculated);

	Rect r (size);
	if(horizontal)
		r.setWidth (calculated.getWidth ());
	if(vertical)
		r.setHeight (calculated.getHeight ());

	if(r != getSize ())
	{
		sizeMode |= kAttachDisabled|kAttachDisabledOnce;
		if(parent)
		{
			if(sizeMode & kHCenter)
			{
				Coord w = r.getWidth ();
				r.left = (parent->getWidth () - w) / 2;
				r.setWidth (w);
			}
			if(sizeMode & kVCenter)
			{
				Coord h = r.getHeight ();
				r.top = (parent->getHeight () - h) / 2;
				r.setHeight (h);
			}
		}

		setSize (r);
		disableSizeMode (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const SizeLimit& CCL_API View::getSizeLimits ()
{
	if((privateFlags & kSizeLimitsValid) == 0)
	{
		calcSizeLimits ();
		privateFlags |= kSizeLimitsValid;
	}
	return sizeLimits;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API View::setSizeLimits (const SizeLimit& sizeLimits)
{
	getSizeLimits ();

	// ignore negative coords
	if(sizeLimits.minWidth >= 0)
		this->sizeLimits.minWidth = sizeLimits.minWidth;
	if(sizeLimits.minHeight >= 0)
		this->sizeLimits.minHeight = sizeLimits.minHeight;
	if(sizeLimits.maxWidth >= 0)
		this->sizeLimits.maxWidth = sizeLimits.maxWidth;
	if(sizeLimits.maxHeight >= 0)
		this->sizeLimits.maxHeight = sizeLimits.maxHeight;

	privateFlags |= kSizeLimitsValid|kExplicitSizeLimits;

	passDownSizeLimits ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::passDownSizeLimits ()
{
	// if we have a fitsize mode, pass our limits to the childs
	bool fitH = (getSizeMode () & kHFitSize) != 0;
	bool fitV = (getSizeMode () & kVFitSize) != 0;
	if(fitH || fitV)
	{
		ListForEachLinkableFast (views, View, v)
			if(!v->hasExplicitSizeLimits ())
			{
				SizeLimit childLimits (v->getSizeLimits ());
				if(fitH)
					LayoutPrimitives::calcSizeLimitsFromParent<HorizontalDirection> (childLimits, sizeLimits, v->getSize ().left);
				if(fitV)
					LayoutPrimitives::calcSizeLimitsFromParent<VerticalDirection> (childLimits, sizeLimits, v->getSize ().top);
				v->setSizeLimits (childLimits);
				v->checkSizeLimits ();
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::hasExplicitSizeLimits () const
{
	return (privateFlags & kExplicitSizeLimits) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::calcSizeLimits ()
{
	// join size limits of subviews
	if(!views.isEmpty ())
	{
		sizeLimits.setUnlimited ();
		ListForEachLinkableFast (views, View, v)
			LayoutPrimitives::joinSubViewLimits (getSize (), sizeLimits, v);
		EndFor

		if((sizeMode & kHFitSize) && ((sizeMode & (kAttachLeft|kAttachRight)) == 0))
			sizeLimits.setFixedWidth (LayoutPrimitives::getMaxCoord<HorizontalDirection> (this));
		if((sizeMode & kVFitSize) && ((sizeMode & (kAttachTop|kAttachBottom)) == 0))
			sizeLimits.setFixedHeight (LayoutPrimitives::getMaxCoord<VerticalDirection> (this));
	}
	else
		sizeLimits.setUnlimited ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::resetSizeLimits ()
{
	privateFlags &= ~(kSizeLimitsValid|kExplicitSizeLimits);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::checkSizeLimits ()
{
	Rect rect (size);
	setSize (getSizeLimits ().makeValid (rect));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::setName (StringRef _name)
{
	name = _name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::setTitle (StringRef _title)
{
	title = _title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::setStyle (StyleRef _style)
{
	style = _style;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Theme& View::getTheme () const
{
	if(theme)
		return *theme;

	// TODO
	CCL_PRINTLN ("Warning: No theme assigned to View!!")
	return getDefaultTheme ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::setTheme (Theme* _theme)
{
	theme = _theme;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IVisualStyle& View::getVisualStyle () const
{
	if(visualStyle)
		return *visualStyle;
	return VisualStyle::emptyStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::setVisualStyle (VisualStyle* _visualStyle)
{
	if(visualStyle)
		visualStyle->unuse (this);

	visualStyle = _visualStyle;

	if(visualStyle)
		visualStyle->use (this);

	onVisualStyleChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::onVisualStyleChanged ()
{
	// apply trigger
	if(visualStyle && visualStyle->getTrigger ())
		visualStyle->getTrigger ()->applyTrigger (this);
	
	// invalidate if not in draw event (can happen if controls call "setVisualStyle" in "getVisualStyle" during draw) 
	if(Window* window = getWindow ())
		if(window->isAttached () && window->isInDrawEvent () == false) 
			invalidate (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int View::getThemeElementState () const
{
	int state = ThemeElements::kNormal;
	if(!isEnabled ())
		state = ThemeElements::kDisabled;
	else if(isMouseDown ())
		state = ThemeElements::kPressed;
	else if(isMouseOver ())
		state = ThemeElements::kMouseOver;
	//	else if(this->isFocused ())
	//		state = ThemeElements::kFocused;
	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef View::getHelpIdentifier () const
{
	if(!style.isCommonStyle (Styles::kNoHelpId) && parent)
		return parent->getHelpIdentifier ();

	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::setHelpIdentifier (StringRef id)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API View::getController () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::setController (IUnknown* controller)
{
	// to be implemented by subclass!
	ASSERT (0)
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsDevice* View::setGraphicsDevice (GraphicsDevice* device)
{
	GraphicsDevice* oldDevice = graphicsDevice;
	graphicsDevice = device;
	return oldDevice;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsDevice* View::getGraphicsDevice (Point& offset)
{
	if(graphicsDevice)
	{
		if(DrawViewContext::currentView == this)
			offset.offset (DrawViewContext::currentOffset);

		graphicsDevice->retain ();
		return graphicsDevice;
	}

	if(parent)
	{
		offset.offset (size.left, size.top);
		return parent->getGraphicsDevice (offset);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::invalidate ()
{
	Rect r;
	getClientRect (r);
	invalidate (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::invalidateSubLayers ()
{
	ListForEachLinkableFast (views, View, v)
		if(IGraphicsLayer* layer = v->getGraphicsLayer ())
			layer->setUpdateNeeded ();
			
		v->invalidateSubLayers ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API View::invalidate (RectRef _rect)
{
	Rect r;
	getClientRect (r);

	if(r.bound (_rect))
	{
		if(graphicsLayer)
			graphicsLayer->setUpdateNeeded (r);
		else
		{
			r.offset (size.left, size.top);
			if(parent)
				parent->invalidate (r);
		}

		if(style.isCommonStyle (Styles::kLayerUpdate))
			invalidateSubLayers ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::draw (const UpdateRgn& updateRgn)
{
	ListForEachLinkableFast (views, View, v)
		if(v->isHidden () || (v->isLayerBackingEnabled () && !DrawViewContext::isRendering)) // view will be drawn via drawLayer()
			continue;
		if(updateRgn.rectVisible (v->getSize ()))
		{
			UpdateRgn subRegion (updateRgn, v->getSize ());
			if(!subRegion.isEmpty ())
			{
				if(!DrawViewContext::isRendering)
					v->hasBeenDrawn (true);
				v->draw (subRegion);
			}
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::updateClient ()
{
	Rect r;
	getClientRect (r);
	updateClient (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API View::updateClient (RectRef rect)
{
	if(!hasBeenDrawn ())
		return;

	WindowUpdateInfo updateInfo;
	Window* window = getWindowForUpdate (updateInfo);
	if(window == nullptr)
		return;

	if(updateInfo.collectUpdates || window->hasBeenDrawn () == false)
	{
		invalidate (rect);
	}
	else if(style.isDirectUpdate ())
	{
		Rect updateRect (rect);
		Rect visibleClient;
		if(getVisibleClientForUpdate (visibleClient) && updateRect.bound (visibleClient))
		{
			draw (UpdateRgn (updateRect));
			updateInfo.addDirtyRect (updateRect);
		}
	}
	else
	{
		window->redrawView (this, rect);
		updateInfo.addDirtyRect (rect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API View::redraw ()
{
	if(isLayerBackingEnabled ())
		return;

	Window* w = getWindow ();
	if(w)
		w->redraw ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API View::scrollClient (RectRef rect, PointRef delta)
{
	ASSERT (isLayerBackingEnabled () == false)
	if(isLayerBackingEnabled ())
		return;

	ASSERT (parent != nullptr)
	if(parent)
	{
		Rect r (rect);
		r.offset (size.left, size.top);
		parent->scrollClient (r, delta);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::setLayerBackingEnabled (bool state)
{
	if(state != isLayerBackingEnabled ())
	{
		if(state)
		{
			privateFlags |= kLayerBacking;
			if(isAttached ())
				makeGraphicsLayer (true);
		}
		else
		{
			if(isAttached ())
				makeGraphicsLayer (false);
			privateFlags &= ~kLayerBacking;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* View::addGraphicsSublayer (IUnknown* content)
{
	Point offset;
	IGraphicsLayer* subLayer = nullptr;
	if(IGraphicsLayer* parentLayer = getParentLayer (offset))
	{
		Rect bounds (size);
		bounds.moveTo (offset);
		UIDRef layerClass = isTiledLayerMode () ? ClassID::TiledLayer : ClassID::GraphicsLayer;
		subLayer = NativeGraphicsEngine::instance ().createGraphicsLayer (layerClass);
		ASSERT (subLayer != nullptr)
		if(subLayer)
		{
			int mode = IGraphicsLayer::kClipToBounds;
			if((style.isTransparent () || style.isComposited () || style.isTranslucent ()) == false)
			{
				mode |= IGraphicsLayer::kIgnoreAlpha;
				const Color& color = getVisualStyle ().getColor (StyleID::kBackColor, Colors::kTransparentBlack);
				if(color.getAlphaF () == 1)
					subLayer->setBackColor (color);
			}

			subLayer->construct (content, bounds, mode, getWindow ()->getContentScaleFactor ());
			if(int tileSize = getVisualStyle ().getMetric<int> ("tilesize", 0))
				subLayer->setTileSize (tileSize);

			parentLayer->addSublayer (subLayer);

			if(parentLayer->getPreviousSibling (subLayer) != nullptr)
			{
				// our sublayer was added above an existing layer
				// make sure layers are in correct order (matching the view hierarchy)

				View* parentLayerHost = nullptr;
				for(View* currentView = getParent (); currentView; currentView = currentView->getParent ())
				{
					if(currentView->getGraphicsLayer () == parentLayer)
					{
						parentLayerHost = currentView;
						break;
					}
				}

				if(parentLayerHost)
				{
					View* nextLayerSibling = nullptr;
					AutoPtr<IRecognizer> recognizer = Recognizer::create ([&] (IUnknown* obj)
					{
						View* view = unknown_cast<View> (obj);
						if(view == this)
							return true;
						if(view->getGraphicsLayer () != nullptr)
							nextLayerSibling = view;
						return false;
					});
					parentLayerHost->findView (*recognizer);
					if(nextLayerSibling)
					{
						IGraphicsLayer* sibling = nextLayerSibling->getGraphicsLayer ();
						while(sibling && sibling->getParentLayer () != subLayer->getParentLayer ())
							sibling = sibling->getParentLayer ();
						if(sibling)
							parentLayer->placeBelow (subLayer, sibling);
					}
				}
			}
		}
	}
	return subLayer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::makeGraphicsLayer (bool state)
{
	if(state)
	{
		if(!graphicsLayer)
			graphicsLayer = addGraphicsSublayer (this->asUnknown ());

		if(graphicsLayer == nullptr) // remove flag if layers unavailable
			privateFlags &= ~kLayerBacking;
	}
	else
	{
		if(graphicsLayer)
		{
			if(IGraphicsLayer* parentLayer = graphicsLayer->getParentLayer ())
				parentLayer->removeSublayer (graphicsLayer);
			graphicsLayer.release ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* CCL_API View::getParentLayer (Point& offset) const
{
	offset.offset (size.left, size.top);

	if(const View* p = getParent ())
	{
		if(p->getGraphicsLayer ())
			return p->getGraphicsLayer ();

		return p->getParentLayer (offset);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API View::drawLayer (IGraphics& graphics, const UpdateRgn& updateRgn, PointRef offset)
{
	GraphicsDevice* device = unknown_cast<GraphicsDevice> (&graphics);
	ASSERT (device != nullptr)
	if(device == nullptr)
		return;

	ScopedVar<View*> scope1 (DrawViewContext::currentView, this);
	ScopedVar<Point> scope2 (DrawViewContext::currentOffset, offset);
	GraphicsDevice* oldDevice = setGraphicsDevice (device);
	ASSERT (oldDevice == nullptr)

	hasBeenDrawn (true);
	draw (updateRgn);

	setGraphicsDevice (oldDevice);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayerContent::LayerHint CCL_API View::getLayerHint () const
{
	return kGraphicsContentHintDefault;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::renderTo (GraphicsDevice& device, const UpdateRgn& updateRgn, PointRef offset)
{
	bool wasDrawn = hasBeenDrawn ();
	GraphicsDevice* oldDevice = setGraphicsDevice (&device);
	ASSERT (oldDevice == nullptr)

	ScopedVar<View*> scope1 (DrawViewContext::currentView, this);
	ScopedVar<Point> scope2 (DrawViewContext::currentOffset, offset);
	ScopedVar<bool> scope3 (DrawViewContext::isRendering, true);
	draw (updateRgn);

	setGraphicsDevice (oldDevice);
	hasBeenDrawn (wasDrawn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::onDisplayPropertiesChanged (const DisplayChangedEvent& event)
{
	if(isLayerBackingEnabled ())
		if(IGraphicsLayer* layer = getGraphicsLayer ())
		{
			if(event.eventType == DisplayChangedEvent::kResolutionChanged)
				layer->setContentScaleFactor (event.contentScaleFactor);
		}

	ListForEachLinkableFast (views, View, v)
		v->onDisplayPropertiesChanged (event);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	ListForEachLinkableFast (views, View, v)
		v->onColorSchemeChanged (event);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::setMouseState (int state)
{
	if(state != mouseState)
	{
		mouseState = state;
		propertyChanged ("mousestate");
		invalidate ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int View::getMouseState () const
{
	return mouseState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onMouseDown (const MouseEvent& event)
{
	ListForEachLinkableFastReverse (views, View, v)
		if(v->isEnabled () && v->getSize ().pointInside (event.where))
		{
			MouseEvent e2 (event);
			e2.where.offset (-v->getSize ().left, -v->getSize ().top);

			if(v->onMouseDown (e2))
				return true;
		}
		else if(!v->isEnabled ())
		{
			CCL_PRINTLN ("View disabled");
		}
	EndFor

	// try to create mouse handler...
	return tryMouseHandler (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::tryMouseHandler (const MouseEvent& event)
{
	if(event.keys.isSet (KeyState::kLButton))
	{
		MouseHandler* handler = createMouseHandler (event);
		if(handler)
		{
			Window* window = getWindow (); // view could be detached during createMouseHandler!
			if(!window || handler->isNullHandler ()) // swallow mouse click here
				handler->release ();
			else
			{
				window->setMouseHandler (handler);
				handler->begin (event);
			}
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onMouseUp (const MouseEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onMouseWheel (const MouseWheelEvent& event)
{
	ListForEachLinkableFastReverse (views, View, v)
		if(v->isEnabled () && v->getSize ().pointInside (event.where))
		{
			MouseWheelEvent e2 (event);
			e2.where.offset (-v->getSize ().left, -v->getSize ().top);

			if(v->onMouseWheel (e2))
				return true;
		}
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onMouseEnter (const MouseEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onMouseMove (const MouseEvent& event)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onMouseLeave (const MouseEvent& event)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* View::createMouseHandler (const MouseEvent& event)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onContextMenu (const ContextMenuEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onTrackTooltip (const TooltipEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* View::createTouchHandler (const TouchEvent& event)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onGesture (const GestureEvent& event)
{
	ListForEachLinkableFastReverse (views, View, v)
		if(v->isEnabled () && (v->getSize ().pointInside (event.where) || event.getType () == GestureEvent::kPenPrimary))
		{
			GestureEvent e2 (event);
			e2.where.offset (-v->getSize ().left, -v->getSize ().top);

			if(v->onGesture (e2))
				return true;
		}
	EndFor

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Keyboard Events
//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onFocus (const FocusEvent& event)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onKeyDown (const KeyEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onKeyUp (const KeyEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Drag & Drop
//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* View::createDragHandler (const DragEvent& event)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onDragEnter (const DragEvent& event)
{
	if(IDragHandler* dragHandler = createDragHandler (event))
	{
		if(dragHandler->dragEnter (event))
		{
			event.session.setDragHandler (dragHandler);
			dragHandler->release ();
			return true;
		}
		else
			dragHandler->release ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onDragOver (const DragEvent& event)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onDragLeave (const DragEvent& event)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onDrop (const DragEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Accessibility
//////////////////////////////////////////////////////////////////////////////////////////////////

void View::setAccessibilityEnabled (bool state)
{
	if(state)
	{
		if(isAttached ())
			makeAccessibilityProvider (true);
		privateFlags |= kAccessible;
	}
	else
	{
		privateFlags &= ~kAccessible;
		if(isAttached ())
			makeAccessibilityProvider (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* View::getAccessibilityProvider ()
{
	if(!accessibilityProvider && isAccessibilityEnabled ())
		accessibilityProvider = NEW ViewAccessibilityProvider (*this);
	return accessibilityProvider;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* View::getParentAccessibilityProvider ()
{
	if(parent)
	{
		if(AccessibilityProvider* provider = parent->getAccessibilityProvider ())
			return provider;

		return parent->getParentAccessibilityProvider ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::makeAccessibilityProvider (bool state)
{
	if(state)
	{
		if(!accessibilityProvider)
		{
			if(AccessibilityManager::isEnabled ()) // accessibility must be enabled for application
				if(AccessibilityProvider* provider = getAccessibilityProvider ())
					if(AccessibilityProvider* parentProvider = getParentAccessibilityProvider ())
						parentProvider->addChildProvider (provider);
		}
	}
	else
	{
		if(accessibilityProvider)
		{
			accessibilityProvider->disconnect ();
			if(AccessibilityProvider* parentProvider = accessibilityProvider->getParentProvider ())
				parentProvider->removeChildProvider (accessibilityProvider);
			
			safe_release (accessibilityProvider);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Coordinates
//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API View::clientToWindow (Point& p) const
{
	p.x += size.left;
	p.y += size.top;

	if(parent)
		parent->clientToWindow (p);
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API View::windowToClient (Point& p) const
{
	Point ofs;
	clientToWindow (ofs);

	p.x -= ofs.x;
	p.y -= ofs.y;
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API View::clientToScreen (Point& p) const
{
	clientToWindow (p);
	Window* w = const_cast<View*> (this)->getWindow ();
	if(w)
		w->clientToScreen (p);
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API View::screenToClient (Point& p) const
{
	if(parent)
		parent->screenToClient (p);

	p.x -= size.left;
	p.y -= size.top;
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::isInsideClient (const Point& where) const
{
	Rect r;
	if(getVisibleClient (r))
		return r.pointInside (where);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Misc. utilities
//////////////////////////////////////////////////////////////////////////////////////////////////

View* View::enterMouse (const MouseEvent& event, View* currentMouseView)
{
	ListForEachLinkableFastReverse (views, View, v)
		MouseEvent e2 (event);
		e2.where.offset (-v->getSize ().left, -v->getSize ().top);
		if(v->isEnabled () && v->isInsideClient (e2.where))
		{
			// try sub-views first...
			View* result = v->enterMouse (e2, currentMouseView);
			if(result)
				return result;

			// ...then try this view
			if(v == currentMouseView)
			{
				e2.eventType = MouseEvent::kMouseMove;
				if(v->onMouseMove (e2))
					return v;
			}
			else if(v->onMouseEnter (e2))
				return v;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* View::dragEnter (const DragEvent& event)
{
	ListForEachLinkableFastReverse (views, View, v)
		DragEvent e2 (event);
		e2.where.offset (-v->getSize ().left, -v->getSize ().top);
		if(v->isEnabled () && v->isInsideClient (e2.where))
		{
			// try sub-views first...
			View* result = v->dragEnter (e2);
			if(result)
				return result;

			// ...then try this view
			if(v->onDragEnter (e2))
				return v;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* View::findFocusView (const MouseEvent& event)
{
	ListForEachLinkableFastReverse (views, View, v)
		MouseEvent e2 (event);
		e2.where.offset (-v->getSize ().left, -v->getSize ().top);
		if(v->isEnabled () && v->isInsideClient (e2.where))
		{
			// try sub-views first...
			View* result = v->findFocusView (e2);
			if(result)
				return result;

			// ...then try this view
			if(v->wantsFocus ())
				return v;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* View::findTooltipView (const MouseEvent& event)
{
	ListForEachLinkableFastReverse (views, View, v)
		MouseEvent e2 (event);
		e2.where.offset (-v->getSize ().left, -v->getSize ().top);
		if(v->isInsideClient (e2.where))
		{
			// try sub-views first...
			View* result = v->findTooltipView (e2);
			if(result)
				return result;

			// ...then try this view
			bool hasTooltip = !v->getTooltip ().isEmpty () || v->isTooltipTrackingEnabled ();
			if(hasTooltip)
				return v;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API View::setCursor (IMouseCursor* cursor)
{
	setCursor (unknown_cast<MouseCursor> (cursor));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::setCursor (MouseCursor* cursor)
{
	GUI.setCursor (cursor, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::detectDrag (const MouseEvent& event)
{
	switch(event.dragged)
	{
	case 0:
		return false;
	case 1:
		return true;
	default:
		event.dragged = GUI.detectDrag (this, event.where) ? 1 : 0;
		return event.dragged == 1;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::detectDoubleClick (const MouseEvent& event)
{
	switch(event.doubleClicked)
	{
	case 0:
		return false;
	case 1:
		return true;
	default:
		event.doubleClicked = GUI.detectDoubleClick (this, event.where) ? 1 : 0;
		return event.doubleClicked == 1;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::makeVisible (RectRef rect, tbool relaxed)
{
	if(parent)
	{
		Rect r (rect);
		r.offset (size.left, size.top);
		return parent->makeVisible (r, relaxed);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::takeFocus (tbool directed)
{
	Window* w = getWindow ();
	return w ? w->setFocusView (this, directed != 0) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::killFocus ()
{
	Window* w = getWindow ();
	if(w && w->getFocusView () == this)
		w->killFocusView ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::getViewAttribute (Variant& value, AttrID id) const
{
	bool result = true;
	switch(id)
	{
	case kName :
		value = getName ();
		break;

	case kTitle :
		value = getTitle ();
		break;

	case kTooltip :
		value = getTooltip ();
		break;

	case kStyleFlags :
		value = getStyle ().toLargeInt ();
		break;

	case kTheme :
		value = (ITheme*)&getTheme ();
		break;

	case kVisualStyle :
		if(visualStyle)
			value = visualStyle->asUnknown ();
		else
			value = ccl_const_cast (VisualStyle::emptyStyle).asUnknown ();
		break;

	case kController :
		value = getController ();
		break;

	case kSizeMode :
		value = getSizeMode ();
		break;

	case kSizeModeDisabled :
		value = (sizeMode & kAttachDisabled) ? 1 : 0;
		break;

	case kInputEnabled :
		value = isEnabled ();
		break;

	case kMouseState :
		value = getMouseState ();
		break;

	case kThemeElementState :
		value = getThemeElementState ();
		break;

	case kFocusEnabled :
		value = wantsFocus ();
		break;

	case kTooltipTrackingEnabled :
		value = isTooltipTrackingEnabled ();
		break;

	case kLayerBackingEnabled :
		value = isLayerBackingEnabled ();
		break;

	case kGraphicsLayer :
		value = getGraphicsLayer ();
		break;

	case kAccessibilityEnabled :
		value = isAccessibilityEnabled ();
		break;

	default :
		result = false;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::setViewAttribute (AttrID id, VariantRef value)
{
	bool result = true;
	switch(id)
	{
	case kName :
		setName (value);
		break;

	case kTitle :
		setTitle (value);
		break;

	case kTooltip :
		setTooltip (value);
		break;

	case kStyleFlags :
		setStyle (StyleFlags ().fromLargeInt (value));
		break;

	case kTheme :
		setTheme (unknown_cast<Theme> (value));
		break;

	case kVisualStyle :
		setVisualStyle (unknown_cast<VisualStyle> (value));
		break;

	case kController :
		result = setController (value.isString () ? System::GetObjectTable ().getObjectByUrl (Url (value.asString ())) : value.asUnknown ()) != 0;
		break;

	case kSizeMode :
		setSizeMode (value);
		break;

	case kSizeModeDisabled :
		disableSizeMode (value.asBool ());
		break;

	case kInputEnabled :
		enable (value.asBool ());
		invalidate ();
		break;

	case kMouseState :
		mouseState = value; // do not invalidate!
		break;

	case kThemeElementState :
		ASSERT (0) // can not be set!
		break;

	case kFocusEnabled :
		wantsFocus (value);
		break;

	case kTooltipTrackingEnabled :
		isTooltipTrackingEnabled (value);
		break;

	case kLayerBackingEnabled :
		setLayerBackingEnabled (value);
		break;

	case kAccessibilityEnabled :
		setAccessibilityEnabled (value);
		break;

	default :
		result = false;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::propertyChanged (StringID propertyId)
{
	if(style.isTrigger ())
		signal (Message (kPropertyChanged, String (propertyId)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::getProperty (Variant& var, MemberID propertyId) const
{
	for(int i = 0; i < ARRAY_COUNT (propertyNames); i++)
		if(propertyId == propertyNames[i].name)
			return getViewAttribute (var, propertyNames[i].value);

	if(propertyId == "parent")
	{
		var = ccl_as_unknown (parent);
		return true;
	}

	MutableCString arrayKey;
	if(propertyId.getBetween (arrayKey, "children[", "]"))
	{
		View* result = nullptr;
		String childName (arrayKey);
		ListForEachLinkableFast (views, View, v)
			if(v->getName () == childName)
			{
				result = v;
				break;
			}
		EndFor

		var = ccl_as_unknown (result);
		return true;
	}

	if(propertyId == "window")
	{
		var = getIWindow ();
		return true;
	}

	if(propertyId == "Host")
	{
		var = &System::GetScriptingManager ().getHost ();
		return true;
	}

	if(propertyId.getBetween (arrayKey, "parent[", "]"))
	{
		// access parent of given class name
		View* result = nullptr;
		if(const MetaClass* metaClass = Kernel::instance ().getClassRegistry ().findType (arrayKey))
			result = getParent (*metaClass);

		var = ccl_as_unknown (result);
		return true;
	}

	if(propertyId == kHelpId)
	{
		var = getHelpIdentifier ();
		return true;
	}

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::setProperty (MemberID propertyId, const Variant& var)
{
	for(int i = 0; i < ARRAY_COUNT (propertyNames); i++)
		if(propertyId == propertyNames[i].name)
			return setViewAttribute (propertyNames[i].value, var);

	if(graphicsLayer && propertyId == IGraphicsLayer::kOpacity)
	{
		graphicsLayer->setOpacity (var.asFloat ());
		return true;
	}

	if(propertyId == kHelpId)
		return setHelpIdentifier (var.asString ());

	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "invalidate")
	{
		invalidate ();
		return true;
	}
	if(msg == "takeFocus")
	{
		takeFocus ();
		return true;
	}
	if(msg == "makeVisible")
	{
		Rect rect;
		getClientRect (rect);
		makeVisible (rect);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::add (IView* view)
{
	return addView (unknown_cast<View> (view));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::insert (int index, IView* view)
{
	return insertView (index, unknown_cast<View> (view));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::remove (IView* view)
{
	return removeView (unknown_cast<View> (view));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::moveBefore (IView* view, IView* before)
{
	return moveViewBefore (unknown_cast<View> (view), unknown_cast<View> (before));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::isChildView (IView* view, tbool deep) const
{
	View* v = unknown_cast<View> (view);
	return v != nullptr && isChild (v, deep != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API View::findChildView (PointRef where, tbool deep) const
{
	return findView (where, deep != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API View::delegateEvent (const GUIEvent& event)
{
	bool result = false;
	switch(event.eventClass)
	{
	// *** View Events ***
	case GUIEvent::kViewEvent :
		{
			switch(event.eventType)
			{
			case ViewEvent::kDraw :
				View::draw (((DrawEvent&)event).updateRgn);
				result = true;
				break;

			case ViewEvent::kSized :
				View::onSize (((ViewSizeEvent&)event).delta);
				result = true;
				break;

			case ViewEvent::kViewsChanged :
				onViewsChanged (); // must call method of derived class!
				result = true;
				break;
			}
		}
		break;

	// *** Mouse Events ***
	case GUIEvent::kMouseEvent :
		{
			switch(event.eventType)
			{
			case MouseEvent::kMouseDown :
				result = View::onMouseDown ((MouseEvent&)event);
				break;
			}
		}
		break;

	case GUIEvent::kMouseWheelEvent :
		result = View::onMouseWheel ((MouseWheelEvent&)event);
		break;

	case GUIEvent::kGestureEvent :
		result = View::onGesture ((GestureEvent&)event);
		break;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#if DEBUG
void View::log (const char* indent, int direction)
{
	if(indent)
		Debugger::print (indent);
	Debugger::printf ("%s", myClass ().getPersistentName ());
	if(!getTitle ().isEmpty ())
		Debugger::printf (" \"%s\"", MutableCString (getTitle ()).str ());
	if(!getName ().isEmpty ())
		Debugger::printf (" '%s'", MutableCString (getName ()).str ());
	RectRef r (getSize ());
	Debugger::printf (" (%d,%d,%d,%d)", r.left, r.top, r.right, r.bottom);

	const SizeLimit& limits = getSizeLimits ();
	if(limits.isValid ())
		Debugger::printf ("    Limits: H (%d .. %d)   V (%d .. %d)\n", limits.minWidth, limits.maxWidth, limits.minHeight, limits.maxHeight);
	else
		Debugger::println ("");

	if(direction > 0)
	{
		MutableCString childIndent ("   ");
		if(indent)
			childIndent.append (indent);

		ListForEachLinkableFast (views, View, v)
			v->log (childIndent, direction);
		EndFor
	}
	else if(direction < 0)
	{
		MutableCString childIndent ("   ");
		if(indent)
			childIndent.append (indent);
		if(getParent ())
			getParent ()->log (childIndent, direction);
	}
}
#endif
