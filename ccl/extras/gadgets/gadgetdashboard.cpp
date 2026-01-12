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
// Filename    : ccl/extras/gadgets/gadgetdashboard.cpp
// Description : Gadget Dashboard
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/extras/gadgets/gadgetdashboard.h"
#include "ccl/extras/gadgets/gadgetmanager.h"

#include "ccl/app/controls/usercontrol.h"
#include "ccl/base/message.h"

#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/gui/framework/iskinmodel.h"
#include "ccl/public/gui/framework/skinxmldefs.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/iparameter.h"

namespace CCL {
namespace Install {

//************************************************************************************************
// DashboardView
//************************************************************************************************

class DashboardView: public UserControl
{
public:
	DECLARE_CLASS_ABSTRACT (DashboardView, UserControl)

	DashboardView (GadgetDashboard& dashboard, RectRef rect);

	PROPERTY_VARIABLE (Coord, spacing, Spacing)

private:
	GadgetDashboard& dashboard;
	ViewBox dropbox;

	void calculateSizes ();
	bool checkLayout ();

	// UserControl
	void attached (IView* parent) override;
	void onSize (PointRef delta) override;
};

} // namespace Install
} // namespace CCL

using namespace CCL;
using namespace Install;

//************************************************************************************************
// GadgetDashboard::ViewItem
//************************************************************************************************

class GadgetDashboard::ViewItem: public Component
{
public:
	ViewItem (GadgetItem* gadget);

	PROPERTY_SHARED_AUTO  (GadgetItem, gadget, Gadget)
	PROPERTY_VARIABLE (Coord, minHeight, MinHeight)
	PROPERTY_VARIABLE (Coord, totalMinHeight, TotalMinHeight) ///< min. height of the whole dashboard when this view is separate

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// GadgetDashboard::FillItem
/** Represents the fillView in DropBox. */
//************************************************************************************************

class GadgetDashboard::FillItem: public Component
{
public:
	FillItem (StringRef viewName)
	: Component (viewName) {}

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override
	{
		return getTheme ()->createView (MutableCString (getName ()), this->asUnknown ());
	}
};

//************************************************************************************************
// GadgetDashboard::ViewItem
//************************************************************************************************

GadgetDashboard::ViewItem::ViewItem (GadgetItem* gadget)
: minHeight (0),
  totalMinHeight (0)
{
	setGadget (gadget);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API GadgetDashboard::ViewItem::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == ("GadgetView"))
	{
		// called from decorating skin view to get the plain gadget view
		if(gadget)
			return gadget->createDashboardView ();
		else
			return ViewBox (ClassID::View, Rect ()); // empty view for measurement of surrounding decor
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GadgetDashboard::ViewItem::getProperty (Variant& var, MemberID propertyId) const
{
	if(gadget && gadget->getProperty (var, propertyId))
		return true;

	return false;
}

//************************************************************************************************
// GadgetDashboard
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (GadgetDashboard, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

GadgetDashboard::GadgetDashboard ()
: Component ("Dashboard"),
  numTabs (0),
  initDone (false)
{
	viewItems.objectCleanup ();

	paramList.addList ("dashboardTab", kDashboardTab)->setStorable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& GadgetDashboard::getViewItems () const
{
	return viewItems; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GadgetDashboard::ViewItem* GadgetDashboard::getViewItem (int index) const
{
	return (ViewItem*)viewItems.at ((int)index); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GadgetDashboard::initialize (IUnknown* context)
{
	// restore selected tab
	paramList.restoreSettings ("Dashboard");
	
	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GadgetDashboard::terminate ()
{
	if(getContext () != nullptr) // otherwise it's an early program exit
	{
		// store selected tab
		paramList.storeSettings ("Dashboard");
	}
	
	viewItems.removeAll ();
	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GadgetDashboard::addGadget (GadgetItem* gadget)
{
	viewItems.add (NEW ViewItem (gadget));

	UnknownPtr<IListParameter> tabParam (paramList.byTag (kDashboardTab));
	tabParam->appendString (gadget->getDashboardTitle ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GadgetDashboard::getSubItems (IUnknownList& items, ItemIndexRef index)
{
	if(numTabs > 0)
	{
		ASSERT (numTabs > 1)
		items.add (this->asUnknown (), true);	// we will create view for tab group
	}

	// separated view items
	ArrayForEach (getViewItems (), ViewItem, viewItem)
		if(__iter >= numTabs)
			items.add (viewItem->asUnknown (), true);
	EndFor

	if(fillItem)
		items.add (fillItem->asUnknown (), true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GadgetDashboard::setTabCount (int numTabs)
{
	if(numTabs != this->numTabs)	
	{
		CCL_PRINTF ("numTabs %d\n", numTabs)
		if(this->numTabs != 0 && numTabs != 0)
		{
			// force recreating first view
			this->numTabs = 0;
			signal (Message (kChanged));
		}

		ASSERT (numTabs != 1)
		this->numTabs = numTabs;
		
		paramList.byTag (kDashboardTab)->setMax (numTabs - 1);

		signal (Message (kChanged));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GadgetDashboard::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "numTabs")
	{
		var = numTabs;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API GadgetDashboard::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "DashboardBoxItem")
	{
		// called by DropBox for separate item or the tab group
		if(data.asUnknown () == this->asUnknown ())
		{
			// create view for tab group
			return getTheme ()->createView ("DashboardTabGroup", this->asUnknown ());
		}
		else if(data.asUnknown () == ccl_as_unknown (fillItem))
		{
			return fillItem->createView (name, data, bounds);
		}
		else
		{
			// separated view: first try decorating form from skin, then try plain gadget view
			IView* view = getTheme ()->createView ("DashboardItem", data);
			if(!view)
			{
				UnknownPtr<IViewFactory> viewItem (data);
				if(viewItem)
					view = viewItem->createView ("GadgetView", data, bounds);
			}
			if(view)
			{
				const SizeLimit& limits (view->getSizeLimits ());
				bool sizable = limits.minHeight != limits.maxHeight;
				ViewBox (view).setSizeMode (sizable ? IView::kAttachAll|IView::kFill : IView::kAttachLeft|IView::kAttachRight);
			}
			return view;
		}
	}
	else if(name.startsWith ("DashboardView"))
	{
		int64 index = 0;
		name.subString (13).getIntValue (index);

		if(ViewItem* viewItem = getViewItem ((int)index))
			return viewItem->createView ("GadgetView", data, bounds);
	}
	else if(name == "DashboardTabView")
	{
		ViewBox container (ClassID::AnchorLayoutView, Rect (), StyleFlags (Styles::kHorizontal));
		container.setAttribute (ATTR_MARGIN, 0);
		container.setAttribute (ATTR_SPACING, 0);

		ITheme* theme = getTheme ();
		const IVisualStyle& tabL = theme->getStyle ("Gadgets.TabL");
		const IVisualStyle& tabR = theme->getStyle ("Gadgets.TabR");
		const IVisualStyle& tabC = theme->getStyle ("Gadgets.TabC");
		Coord height = bounds.getHeight ();

		int i = 0;
		ArrayForEach (viewItems, ViewItem, viewItem)
			if(i >= numTabs)
				break;

			ControlBox tabButton (ClassID::ToolButton, paramList.byTag (kDashboardTab), Rect (0, 0, 0, height), 0, viewItem->getGadget ()->getDashboardTitle ());
			tabButton.setAttribute ("value", i);

			const IVisualStyle* tabStyle = &tabC;
			if(numTabs > 1)
			{
				if(i == 0)
					tabStyle = &tabL;
				else if(i == numTabs - 1)
					tabStyle = &tabR;
			}
				
			tabButton.setVisualStyle (*tabStyle);

			tabButton.setSizeMode (IView::kAttachLeft|IView::kAttachRight);
			container.getChildren ().add (tabButton);
			i++;
		EndFor

		return container;
	}
	else if(name == "Dashboard")
	{
		if(!initDone)
		{
			initDone = true;

			UnknownPtr<ISkinCreateArgs> args (data.asUnknown ());
			if(args)
			{
				String fillViewName;
				args->getElement ()->getDataDefinition (fillViewName, "fillView");
				if(!fillViewName.isEmpty ())
					fillItem = NEW FillItem (fillViewName);
			}
		}
		return *NEW DashboardView (*this, bounds);
	}
	return nullptr;
}

//************************************************************************************************
// DashboardView
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DashboardView, UserControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

DashboardView::DashboardView (GadgetDashboard& dashboard, RectRef rect)
: UserControl (rect),
  dashboard (dashboard),
  spacing (1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DashboardView::calculateSizes ()
{
	dashboard.setTabCount (0);

	Coord decorSizeTabs = 0;
	Coord decorSizeSeparated = 0;
	Coord fillViewSize = 0;
	Coord fillViewMinSize = 0;
	Coord fillViewMaxSize = 0;

	// create dummy instances of skin views DashboardItem & DashboardTabGroup (with no gadget content) to measure their decorating size
	AutoPtr<IView> view;
	GadgetDashboard::ViewItem dummyItem (nullptr);
	if(view = getTheme ().createView ("DashboardItem", dummyItem.asUnknown ()))
		decorSizeSeparated = view->getSize ().getHeight ();

	if(view = getTheme ().createView ("DashboardTabGroup", dashboard.asUnknown ()))
		decorSizeTabs = view->getSize ().getHeight ();

	if(Component* fillItem = dashboard.getFillItem ())
		if(view = fillItem->createView (nullptr, 0, Rect ()))
		{
			// use initial fillView size for layout considerations; fillView limits for calculating our total limits
			fillViewSize = view->getSize ().getHeight () + spacing;
			fillViewMinSize = view->getSizeLimits ().minHeight + spacing;
			fillViewMaxSize = view->getSizeLimits ().maxHeight + spacing;
		}

	// determine sizes of gadget views
	ForEach (dashboard.getViewItems (), GadgetDashboard::ViewItem, viewItem)
		AutoPtr<IView> view = viewItem->getGadget ()->createDashboardView ();
		if(view)
			viewItem->setMinHeight (view->getSize ().getHeight ()); // if items sizable: view->getSizeLimits ().minHeight...
	EndFor

	int numViews = dashboard.getViewItems ().count ();

	// calculate min height when all views are in tabs, or all views are separated
	Coord minSizeAllTabs = 0;
	Coord minSizeAllSeparated = ccl_max (0, numViews - 1) * spacing;
	ForEach (dashboard.getViewItems (), GadgetDashboard::ViewItem, viewItem)
		Coord minH = viewItem->getMinHeight ();
		minSizeAllSeparated += minH + decorSizeSeparated;
		ccl_lower_limit (minSizeAllTabs, minH + decorSizeTabs);
	EndFor

	// calculate total min. height required to show each item (and all following) separated
	Coord  minSizeSeparated = minSizeAllSeparated;
	Coord minSizeTabs = 0;
	ForEach (dashboard.getViewItems (), GadgetDashboard::ViewItem, viewItem)
		// this view and all following are separated
		viewItem->setTotalMinHeight (minSizeTabs + minSizeSeparated + fillViewSize);

		// for the next variant, this view will be in a tab
		Coord minH = viewItem->getMinHeight ();
		ccl_lower_limit (minSizeTabs, minH + decorSizeTabs);
		minSizeSeparated -= (minH + decorSizeSeparated + spacing);
		CCL_PRINTF ("%d: (height: %d) totalMinH: %d\n", MutableCString (viewItem->getGadget ()->getName ()).str (), minH, viewItem->getTotalMinHeight ())
	EndFor

	SizeLimit limits;
	limits.setUnlimited ();
	if(numViews > 0)
	{
		limits.minHeight = minSizeAllTabs + fillViewMinSize;
		limits.maxHeight = minSizeAllSeparated + fillViewMaxSize;
	}
	setSizeLimits (limits);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DashboardView::checkLayout ()
{
	int height = getSize ().getHeight ();
	int numViews = dashboard.getViewItems ().count ();
	int numTabs = numViews;

	for(int i = 0; i < numViews; i++)
	{
		// view i is first separated: i views are in tabs
		if(i != 1) // 1 view in a tab container doesn't make sense
		{
			GadgetDashboard::ViewItem* viewItem = dashboard.getViewItem (i);
			if(viewItem->getTotalMinHeight () <= height)
			{
				numTabs = i;
				break;
			}
		}
	}

	return dashboard.setTabCount (numTabs == 1 ? 0 : numTabs);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DashboardView::attached (IView* parent)
{
	SuperClass::attached (parent);

	if(!dropbox) // first time
	{
		const IVisualStyle& style = getVisualStyle ();
		spacing = style.getMetric<Coord> ("spacing", spacing);

		calculateSizes ();

		// vertical dropbox as container for gadgets
		Rect size;
		getClientRect (size);
		dropbox = ViewBox (ClassID::DropBox, size, StyleFlags (Styles::kVertical));
		dropbox.setSizeMode (IView::kAttachAll);
		dropbox.setName ("DashboardBox");

		ViewBox verticalLayout (dropbox.getChildren ().getFirstView ());
		verticalLayout.setSizeMode (IView::kAttachAll);
		verticalLayout.setAttribute (ATTR_SPACING, spacing);

		SizeLimit none;
		none.setUnlimited ();
		dropbox.setSizeLimits (none);

		UnknownPtr<IItemView> itemView (dropbox);
		itemView->setModel (&dashboard);
		getChildren ().add (dropbox);
	}

	if(!checkLayout ())
		dashboard.signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DashboardView::onSize (PointRef delta)
{
	checkLayout ();
	SuperClass::onSize (delta);
}
