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
// Filename    : ccl/gui/layout/anchorlayout.cpp
// Description : Base classes for anchor layouts
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/layout/anchorlayout.h"
#include "ccl/gui/layout/directions.h"
#include "ccl/gui/layout/layoutprimitives.h"
#include "ccl/gui/layout/directions.h"
#include "ccl/gui/layout/divider.h"

#include "ccl/gui/layout/boxlayout.h"
#include "ccl/gui/layout/clipperlayout.h"
#include "ccl/gui/layout/sizevariantlayout.h"
#include "ccl/gui/layout/tablelayout.h"

#include "ccl/public/gui/iviewstate.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Layout Registration
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (Layout, kFrameworkLevelFirst)
{
	LayoutFactory::instance ().registerLayout (LAYOUTCLASS_BOX, ccl_typeid<BoxLayout> ());
	LayoutFactory::instance ().registerLayout (LAYOUTCLASS_CLIPPER, ccl_typeid<ClipperLayout> ());
	LayoutFactory::instance ().registerLayout (LAYOUTCLASS_SIZEVARIANT, ccl_typeid<SizeVariantLayout> ());
	LayoutFactory::instance ().registerLayout (LAYOUTCLASS_TABLE, ccl_typeid<TableLayout> ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{

//************************************************************************************************
// LayoutClassEntry
//************************************************************************************************

class LayoutClassEntry: public Object
{
public:
	LayoutClassEntry (StringID layoutName, MetaClassRef metaClass)
	: layoutName (layoutName),
	  metaClass (metaClass)
	{}
	CString layoutName;
	MetaClassRef metaClass;
};

//************************************************************************************************
// LayoutState
//************************************************************************************************

class LayoutState
{
public:
	static void store (AnchorLayoutView& layoutView, IAttributeList& attribs);
	static void restore (AnchorLayoutView& layoutView, IAttributeList& attribs);

private:
	struct ItemID;
};

} // namespace

//************************************************************************************************
// LayoutState
//************************************************************************************************

struct LayoutState::ItemID: public MutableCString
{
	/// This ID helps ensuring that we don't apply stored sizes to the wrong view.
	/// It's built from the view's class and name, so it's not always unique
	ItemID (AnchorLayoutItem* item)
	{
		if(View* view = item->getView ())
		{
			append (view->myClass ().getPersistentName ());
			if(!view->getName ().isEmpty ())
			{
				append (" ");
				append (view->getName ());
			}
		}
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutState::store (AnchorLayoutView& layoutView, IAttributeList& attribs)
{
	AttributeAccessor a (attribs);

	char attribID[32];
	int i = 0;
	ForEach (layoutView.getLayoutItems (), AnchorLayoutItem, item)
		ItemID itemID (item);
		snprintf (attribID, sizeof(attribID), "i%d", i);
		a.set (attribID, itemID);

		attribID[0] = 'x';
		a.set (attribID, item->preferredSize.x);

		attribID[0] = 'y';
		a.set (attribID, item->preferredSize.y);

		CCL_PRINTF ("store LayoutItem %d (%s) %d, %d\n", i, itemID.str (), item->preferredSize.x, item->preferredSize.y)
		i++;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutState::restore (AnchorLayoutView& layoutView, IAttributeList& attribs)
{
	AttributeAccessor a (attribs);

	char attribID[32];
	int i = 0;
	ForEach (layoutView.getLayoutItems (), AnchorLayoutItem, item)
		snprintf (attribID, sizeof(attribID), "i%d", i);

		ItemID itemID (item);
		MutableCString savedID = a.getCString (attribID);
		if(savedID == itemID)
		{
			Coord x = 0, y = 0;
			attribID[0] = 'x';
			if(a.getInt (x, attribID))
			{
				attribID[0] = 'y';
				if(a.getInt (y, attribID))
				{
					item->preferredSize.x = x;
					item->preferredSize.y = y;
					item->sizeLimits.makeValid (item->preferredSize);
					CCL_PRINTF ("restore LayoutItem %d (%s) %d, %d\n", i, itemID.str (), item->preferredSize.x, item->preferredSize.y)
				}
			}
		}
		i++;
	EndFor	
}

//************************************************************************************************
// AnchorLayoutContext
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AnchorLayoutContext, LayoutContext)

//////////////////////////////////////////////////////////////////////////////////////////////////

float AnchorLayoutContext::getZoomFactor () const
{
	return parentView->getZoomFactor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorLayoutContext::isSizeModeDisabled () const
{
	return parentView->isSizeModeDisabled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StyleRef AnchorLayoutContext::getStyle () const
{
	return parentView->getStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef AnchorLayoutContext::getTitle () const
{
	return parentView->getTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutContext::requestAlgorithm (LayoutAlgorithm* newAlgorithm)
{
	if(auto* anchorLayoutView = ccl_cast<AnchorLayoutView> (parentView))
		anchorLayoutView->replaceAlgorithm (newAlgorithm);
}

//************************************************************************************************
// AnchorLayout
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AnchorLayout, Layout)

//////////////////////////////////////////////////////////////////////////////////////////////////

AnchorLayoutData& AnchorLayout::getLayoutData ()
{
	return layoutData;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const StyleDef* AnchorLayout::getCustomStyles () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorLayout::setAttributes (const SkinAttributes& a)
{
	layoutData.spacing = a.getInt (ATTR_SPACING, kMinCoord);
	layoutData.margin = a.getInt (ATTR_MARGIN, kMinCoord);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorLayout::getAttributes (SkinAttributes& a) const
{
	if(layoutData.spacing != kMinCoord)
		a.setInt (ATTR_SPACING, layoutData.spacing);
	else
		a.setString (ATTR_SPACING, String::kEmpty);

	if(layoutData.margin != kMinCoord)
		a.setInt (ATTR_MARGIN, layoutData.margin);
	else
		a.setString (ATTR_MARGIN, String::kEmpty);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutItem* AnchorLayout::createItem (View* view)
{
	if(view != nullptr)
		return NEW AnchorLayoutItem (view);

	return NEW AnchorLayoutItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutContext* AnchorLayout::createContext (LayoutView* parent)
{
	return NEW AnchorLayoutContext (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AnchorLayout::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == ATTR_SPACING)
	{
		layoutData.spacing = var;
		return true;
	}
	else if(propertyId == ATTR_MARGIN)
	{
		layoutData.margin = var;
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AnchorLayout::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == ATTR_SPACING)
	{
		var = layoutData.spacing;
		return true;
	}
	else if(propertyId == ATTR_MARGIN)
	{
		var = layoutData.margin;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// AnchorLayoutAlgorithm
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AnchorLayoutAlgorithm, LayoutAlgorithm)

//////////////////////////////////////////////////////////////////////////////////////////////////

AnchorLayoutAlgorithm::AnchorLayoutAlgorithm (AnchorLayoutContext* context, AnchorLayoutData& layoutData)
: context (context),
  layoutData (layoutData)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorLayoutAlgorithm::isSizeMode (int flags) const
{
	return (context->getSizeMode () & flags) == flags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutAlgorithm::onItemAdded (LayoutItem* item)
{
	onItemInserted (-1, item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutAlgorithm::onItemInserted (int index, LayoutItem* item)
{
	if(AnchorLayoutAlgorithm* newAlgo = onViewAdded (index, static_cast<AnchorLayoutItem*> (item)))
		context->requestAlgorithm (newAlgo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutAlgorithm::onItemRemoved (LayoutItem* item)
{
	if(AnchorLayoutAlgorithm* newAlgo = onViewRemoved (static_cast<AnchorLayoutItem*> (item)))
		context->requestAlgorithm (newAlgo);
}

//************************************************************************************************
// AnchorLayoutView
//************************************************************************************************

DEFINE_CLASS (AnchorLayoutView, LayoutView)
DEFINE_CLASS_UID (AnchorLayoutView, 0x4e41fb28, 0xcd63, 0x4f3c, 0xb1, 0x88, 0x4, 0x5e, 0xa0, 0xae, 0x83, 0x64)

//////////////////////////////////////////////////////////////////////////////////////////////////

AnchorLayoutView::AnchorLayoutView ()
: LayoutView (),
  layoutSuspended (false)
{
	setProperty (ATTR_LAYOUTCLASS, CCLSTR (LAYOUTCLASS_BOX));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnchorLayoutView::AnchorLayoutView (const Rect& size, StyleRef style, AnchorLayout* layout)
: LayoutView (size, style, layout),
  layoutSuspended (false)
{
	// The base class constructor can't call our setLayout implementation since it doesn't exist yet.
	setLayout (layout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutView::setLayout (Layout* newLayout)
{
	SuperClass::setLayout (newLayout);
	if(!layout)
		return;

	auto* anchorLayout = static_cast<AnchorLayout*> (layout.as_plain ());
	AnchorLayoutData& layoutData = anchorLayout->getLayoutData ();
	if(layoutData.spacing == kMinCoord)
		layoutData.spacing = getTheme ().getThemeMetric (ThemeElements::kLayoutSpacing);
	if(layoutData.margin == kMinCoord)
		layoutData.margin = getTheme ().getThemeMetric (ThemeElements::kLayoutMargin);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutView::makeCurrentSizesPreferred ()
{
	ArrayForEachFast (layoutItems, AnchorLayoutItem, item)
		if(!item->preferredSizeLocked ())
		{
			Rect r (item->getView ()->getSize ());
			item->preferredSize (r.getWidth (), r.getHeight ());
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutView::forceSize (View* view, PointRef size)
{
	AnchorLayoutItem* item = static_cast<AnchorLayoutItem*> (findLayoutItem (view));
	if(item)
	{
		SizeLimitsMemento savedLimits (*view);

		Point validSize (size);
		view->getSizeLimits ().makeValid (validSize);

		// temporarily set limits fixed to given size
		item->sizeLimits.setFixed (validSize);
		item->preferredSize = validSize;

		LayoutPrimitives::applySizeLimitsShallow (*view, item->sizeLimits);

		doLayout ();

		makeCurrentSizesPreferred ();

		item->sizeLimits = savedLimits;
		savedLimits.restore (*view);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutView::onManipulationDone ()
{
	// store layout state
	if(IAttributeList* attribs = getLayoutState (true))
		LayoutState::store (*this, *attribs);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorLayoutView::hasSavedState ()
{
	return getLayoutState (false) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorLayoutView::isLayoutSuspended () const
{
	return layoutSuspended;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutView::setLayoutSuspended (bool state)
{
	if(state != layoutSuspended)
	{
		layoutSuspended = state;
		if(state == false)
			doLayout ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutView::flushLayout ()
{
	SuperClass::flushLayout ();
	if(auto* anchorLayoutAlgorithm = ccl_cast<AnchorLayoutAlgorithm> (algorithm))
		anchorLayoutAlgorithm->flushLayout ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutView::setStyle (StyleRef _style)
{
	if(algorithm)
		algorithm.release ();

	SuperClass::setStyle (_style);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* AnchorLayoutView::getLayoutState (tbool create)
{
	if(!persistenceID.isEmpty ())
		if(ILayoutStateProvider* provider = GetViewInterfaceUpwards<ILayoutStateProvider> (this))
			return provider->getLayoutState (persistenceID, create);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutView::attached (View* parent)
{
	// call baseclass first to avoid double-attaching views added in doLayout (via showItem)
	SuperClass::attached (parent);

	// restore layout state
	if(IAttributeList* attribs = getLayoutState (false))
	{
		LayoutState::restore (*this, *attribs);
		doLayout ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutView::onChildLimitsChanged (View* child)
{
	if(isAttached ())
	{
		if(AnchorLayoutItem* item = static_cast<AnchorLayoutItem*> (findLayoutItem (child)))
		{
			if(item->updateSizeLimits ())
			{
				if((privateFlags & kExplicitSizeLimits) == 0)
				{
					privateFlags &= ~kSizeLimitsValid;
					if(parent)
						parent->onChildLimitsChanged (this);
				}
				doLayout ();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutView::calcSizeLimits ()
{
	if(auto* anchorLayoutAlgorithm = ccl_cast<AnchorLayoutAlgorithm> (getAlgorithm ()))
		anchorLayoutAlgorithm->calcSizeLimits (sizeLimits);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutView::passDownSizeLimits ()
{
	if(!layout)
		return;

	// if we have a fitsize mode, pass our limits to the childs
	bool fitH = (getSizeMode () & kHFitSize) != 0;
	bool fitV = (getSizeMode () & kVFitSize) != 0;
	if(fitH || fitV)
	{
		auto* anchorLayout = static_cast<AnchorLayout*> (layout.as_plain ());
		Coord margins = 2 * anchorLayout->getLayoutData ().margin;
		ListForEachLinkableFast (views, View, v)
			if(!v->hasExplicitSizeLimits ())
			{
				SizeLimit childLimits (v->getSizeLimits ());
				if(fitH)
					LayoutPrimitives::calcSizeLimitsFromParent<HorizontalDirection> (childLimits, sizeLimits, margins);
				if(fitV)
					LayoutPrimitives::calcSizeLimitsFromParent<VerticalDirection> (childLimits, sizeLimits, margins);
				v->setSizeLimits (childLimits);
				v->checkSizeLimits ();
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutView::constrainSize (Rect& rect) const
{
	if(auto* anchorLayoutAlgorithm = ccl_cast<AnchorLayoutAlgorithm> (algorithm))
		anchorLayoutAlgorithm->constrainSize (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Divider* AnchorLayoutView::findNearDivider (MouseEvent& event)
{
	if(View* view = findView (event.where, false))
	{
		int reachOutBoost = event.wasTouchEvent () ? 2 : 1;
		auto getOutreach = [&] (Divider* divider)
		{
			return divider->getOutreach () * reachOutBoost;
		};

		int idx = index (view);

		if(getStyle ().isCommonStyle (Styles::kVertical))
		{
			if(Divider* divider = ccl_cast<Divider> (getChild (idx + 1)))
				if(divider->getSize ().top - event.where.y <= getOutreach (divider) && divider->isEnabled () && !divider->getStyle ().isCustomStyle (Styles::kDividerBehaviorOutreachBottom))
				{
					event.where.offset (-divider->getSize ().left, -divider->getSize ().top);
					return divider;
				}

			if(Divider* divider = ccl_cast<Divider> (getChild (idx - 1)))
				if(event.where.y - divider->getSize ().bottom <= getOutreach (divider) && divider->isEnabled () && !divider->getStyle ().isCustomStyle (Styles::kDividerBehaviorOutreachTop))
				{
					event.where.offset (-divider->getSize ().left, -divider->getSize ().top);
					return divider;
				}
		}
		else
		{
			if(Divider* divider = ccl_cast<Divider> (getChild (idx + 1)))
				if(divider->getSize ().left - event.where.x <= getOutreach (divider) && divider->isEnabled () && !divider->getStyle ().isCustomStyle (Styles::kDividerBehaviorOutreachRight))
				{
					event.where.offset (-divider->getSize ().left, -divider->getSize ().top);
					return divider;
				}

			if(Divider* divider = ccl_cast<Divider> (getChild (idx - 1)))
				if(event.where.x - divider->getSize ().right <= getOutreach (divider) && divider->isEnabled () && !divider->getStyle ().isCustomStyle (Styles::kDividerBehaviorOutreachLeft))
				{
					event.where.offset (-divider->getSize ().left, -divider->getSize ().top);
					return divider;
				}
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutView::doLayout ()
{
	if(!layoutSuspended)
		SuperClass::doLayout ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutView::replaceAlgorithm (LayoutAlgorithm* newAlgorithm)
{
	setAlgorithm (newAlgorithm);
	initAlgorithm ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutView::initAlgorithm ()
{
	if(layoutItems.isEmpty ())
		return;

	// temporarily remove existing items
	ObjectArray tmpItems;
	int numItems = layoutItems.count ();
	for(int i = numItems - 1; i >= 0; i--)
	{
		Object* item = layoutItems.at (i);
		tmpItems.add (item);
		layoutItems.removeAt (i);
	}

	// add them again and notify the new algorithm
	ForEachReverse (tmpItems, AnchorLayoutItem, item)
		layoutItems.add (item);
		getAlgorithm ()->onItemAdded (item);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorLayoutView::onMouseEnter (const MouseEvent& event)
{
	MouseEvent e2 (event);
	if(Divider* divider = findNearDivider (e2))
	{
		if(divider->canResizeViews ())
		{
			ThemeCursorID cursor = getStyle ().isCommonStyle (Styles::kVertical) ? ThemeElements::kSizeVerticalCursor : ThemeElements::kSizeHorizontalCursor;
			setCursor (getTheme ().getThemeCursor (cursor));
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorLayoutView::onMouseMove (const MouseEvent& event)
{
	MouseEvent e2 (event);
	if(Divider* divider = findNearDivider (e2))
	{
		if(divider->canResizeViews ())
		{
			ThemeCursorID cursor = getStyle ().isCommonStyle (Styles::kVertical) ? ThemeElements::kSizeVerticalCursor : ThemeElements::kSizeHorizontalCursor;
			setCursor (getTheme ().getThemeCursor (cursor));
			return true;
		}
	}

	setCursor ((MouseCursor*)nullptr);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorLayoutView::onMouseDown (const MouseEvent& event)
{
#if DEBUG_LOG
	dump ();
#endif

	MouseEvent e2 (event);
	Divider* divider = findNearDivider (e2);
	if(divider && divider->canResizeViews () && divider->tryMouseHandler (e2))
		return true;

	return SuperClass::onMouseDown (event);
}

//************************************************************************************************
// BoxLayoutView
//************************************************************************************************

DEFINE_CLASS (BoxLayoutView, AnchorLayoutView)
DEFINE_CLASS_UID (BoxLayoutView, 0xa1d58be3, 0x5501, 0x4997, 0xbe, 0x70, 0xe, 0x2c, 0x26, 0x89, 0x59, 0xf1)

//////////////////////////////////////////////////////////////////////////////////////////////////

BoxLayoutView::BoxLayoutView (const Rect& rect, StyleRef style)
: AnchorLayoutView (rect, style, AutoPtr<BoxLayout> (NEW BoxLayout))
{}

//************************************************************************************************
// AnchorLayoutItem
//************************************************************************************************

DEFINE_CLASS (AnchorLayoutItem, LayoutItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

AnchorLayoutItem::AnchorLayoutItem ()
: fillFactor (0),
  priority (0),
  flags (0)
{
	sizeLimits.setUnlimited ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnchorLayoutItem::AnchorLayoutItem (View* view)
: LayoutItem (view),
  fillFactor ((view->getSizeMode () & View::kFill) ? 1.f : 0.f),
  priority (0),
  flags (0)
{
	const SizeLimit& sizeLimits = view->getSizeLimits ();

	// take the current size as the preferred one
	preferredSize (view->getWidth (), view->getHeight ());
	if(preferredSize.x == 0)
		preferredSize.x = sizeLimits.minWidth;
	if(preferredSize.y == 0)
		preferredSize.y = sizeLimits.minHeight;

	setSizeLimits (sizeLimits);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutItem::setSizeLimits (const SizeLimit& newLimits)
{
	sizeLimits = newLimits;

	// the view can only resize if attached to 2 opposite edges
	if((view->getSizeMode () & (View::kAttachLeft | View::kAttachRight)) != (View::kAttachLeft | View::kAttachRight))
		sizeLimits.minWidth = sizeLimits.maxWidth = preferredSize.x;
	if((view->getSizeMode () & (View::kAttachTop | View::kAttachBottom)) != (View::kAttachTop | View::kAttachBottom))
		sizeLimits.minHeight = sizeLimits.maxHeight = preferredSize.y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutItem::updateSize ()
{
	if(view)
	{
		workRect = view->getSize ();
		preferredSize (workRect.getWidth (), workRect.getHeight ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnchorLayoutItem::updatePreferredSize ()
{
	if(view)
		preferredSize (view->getWidth (), view->getHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorLayoutItem::updateSizeLimits ()
{
	if(view)
	{
		const SizeLimit& newLimits = view->getSizeLimits ();
		if(!(sizeLimits == newLimits))
		{
			setSizeLimits (newLimits);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorLayoutItem::setAttributes (const SkinAttributes& a)
{
	MutableCString priorityString (a.getString (ATTR_LAYOUTPRIORITY));
	if(priorityString == LAYOUTPRIORITY_GROUPDECOR)
	{
		isGroupDecorItem (true);
		priority = -1;
	}
	else
	{
		priority = a.getInt (ATTR_LAYOUTPRIORITY);
	}

	fillFactor = a.getFloat (ATTR_FILL);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorLayoutItem::getAttributes (SkinAttributes& a) const
{
	if(isGroupDecorItem ())
		a.setString (ATTR_LAYOUTPRIORITY, String (LAYOUTPRIORITY_GROUPDECOR));
	else
		a.setInt (ATTR_LAYOUTPRIORITY, priority);

	a.setFloat (ATTR_FILL, fillFactor);
	return true;
}
