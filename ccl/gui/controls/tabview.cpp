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
// Filename    : ccl/gui/controls/tabview.cpp
// Description : Tab View
//
//************************************************************************************************

#include "tabview.h"

#include "ccl/gui/theme/renderer/tabviewrenderer.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/views/sprite.h"
#include "ccl/gui/views/viewaccessibility.h"
#include "ccl/gui/popup/extendedmenu.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/popup/parametermenubuilder.h"
#include "ccl/gui/graphics/imaging/filmstrip.h"
#include "ccl/gui/graphics/imaging/multiimage.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/system/dragndrop.h"

#include "ccl/base/message.h"
#include "ccl/base/boxedtypes.h"

#include "ccl/public/gui/framework/abstractdraghandler.h"
#include "ccl/public/gui/framework/controlsignals.h"
#include "ccl/public/gui/iviewstate.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/idatatarget.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/systemservices.h"

#define DRAG_NEEDS_CMD 0

namespace CCL {

//************************************************************************************************
// TabViewMouseHandler
//************************************************************************************************

class TabViewMouseHandler: public MouseHandler
{
public:
	TabViewMouseHandler (TabView* button = nullptr)
	: MouseHandler (button)
	{}

	void onBegin () override
	{ 
		TabView* tabView = (TabView*)view;
		tabView->mouseDown (current);
	} 
	
	void onRelease (bool canceled) override
	{ 
		TabView* tabView = (TabView*)view;
		tabView->mouseUp (current);
	}

	bool onMove (int moveFlags) override
	{
		view->onMouseMove (current);
		return true;
	}
};

//************************************************************************************************
// TabViewDragHandlerBase
//************************************************************************************************

class TabViewDragHandlerBase: public Unknown,
							  public AbstractDragHandler,
							  public IItemViewDragHandler
{
public:
	TabViewDragHandlerBase (TabView* tabView)
	: tabView (tabView),
	  tabEnterTime (0),
	  currentTab (-1),
	  insertPos (-1),
	  flags (0)
	{}

	PROPERTY_FLAG (flags, 1<<0, hiliteMouseOverTab)
	PROPERTY_FLAG (flags, 1<<1, activateMouseOverTab)
	PROPERTY_FLAG (flags, 1<<2, showInsertPosition)

	// IDragHandler
	tbool CCL_API dragEnter (const DragEvent& event) override
	{
		// position sprite
		if(showInsertPosition ())
		{
			Rect tabRect;
			tabView->getRenderer ()->getPartRect (tabView, TabView::kPartFirstTab, tabRect);

			Color color = tabView->getTheme ().getThemeColor (ThemeElements::kAlphaCursorColor);
			AutoPtr<IDrawable> drawable = NEW SolidDrawable (color);
			positionSprite = Sprite::createSprite (tabView, drawable, tabRect.setWidth (2));
		}

		tabView->setMouseDown (-1);
		return dragOver (event);
	}

	tbool CCL_API dragOver (const DragEvent& event) override
	{
		currentTab = -1;
		insertPos = -1;

		int numTabs = tabView->countTabs ();
		int partCode = tabView->getRenderer ()->hitTest (tabView, event.where, nullptr);
		if(partCode >= TabView::kPartFirstTab && partCode <= TabView::kPartLastTab)
		{
			currentTab = partCode - TabView::kPartFirstTab;
			insertPos = currentTab;
		}
		else if(partCode == TabView::kPartHeader)
			insertPos = numTabs; // after last

		// mouse over tab
		if(currentTab != tabView->getMouseOverTab ())
		{
			if(hiliteMouseOverTab ())
				tabView->setMouseOver (currentTab);

			tabEnterTime = System::GetSystemTicks ();
		}
		else if(activateMouseOverTab () && currentTab > -1)
		{
			int64 now = System::GetSystemTicks ();
			if(now - tabEnterTime > 500)
			{
				(NEW Message ("activateTab", currentTab))->post (tabView);
				tabEnterTime = 0;
			}
		}

		if(showInsertPosition ())
		{
			Rect tabRect;
			tabView->getRenderer ()->getPartRect (tabView, partCode, tabRect);

			Coord spritePos = tabRect.left;
			if(currentTab == numTabs - 1 && event.where.x > tabRect.left + .66 * tabRect.getWidth ())
			{
				// after last tab
				insertPos++;
				spritePos = tabRect.right;
			}
			else if(insertPos == numTabs)
			{
				// after last tab
				tabView->getRenderer ()->getPartRect (tabView, TabView::kPartFirstTab + numTabs - 1, tabRect);
				spritePos = tabRect.right;
			}

			spritePos = ccl_max (spritePos, 0);
			positionSprite->moveTo (Point (spritePos, 0));

			if(!positionSprite->isVisible ())
				positionSprite->show ();
		}
		return true;
	}

	tbool CCL_API drop (const DragEvent& event) override
	{
		cleanup ();
		return true;
	}

	tbool CCL_API dragLeave (const DragEvent& event) override
	{
		cleanup ();
		return true;
	}

	virtual void cleanup ()
	{
		if(positionSprite)
			positionSprite->hide ();

		tabView->setMouseOver (-1);
		tabEnterTime = 0;
	}

	// IItemViewDragHandler
	tbool CCL_API getTarget (ItemIndex& item, int& relation) override
	{
		if(showInsertPosition () && insertPos > -1)
		{
			item = ItemIndex (insertPos);
			relation = IItemViewDragHandler::kBeforeItem;
			return true;
		}
		else if(hiliteMouseOverTab () && currentTab > -1)
		{
			item = ItemIndex (currentTab);
			relation = IItemViewDragHandler::kOnItem;
			return true;
		}
		return false;
	}

	CLASS_INTERFACE2 (IDragHandler, IItemViewDragHandler, Unknown)

protected:
	TabView* tabView;
	AutoPtr<Sprite> positionSprite;
	int64 tabEnterTime;
	int currentTab;
	int insertPos;
	int flags;
};

//************************************************************************************************
// TabViewDragHandler
//************************************************************************************************

class TabViewDragHandler: public TabViewDragHandlerBase
{
public:
	TabViewDragHandler (TabView* tabView)
	: TabViewDragHandlerBase (tabView)
	{
		hiliteMouseOverTab (true);
		activateMouseOverTab (true);
	}
};

//************************************************************************************************
// TabViewDataDragHandler
//************************************************************************************************

class TabViewDataDragHandler: public TabViewDragHandlerBase
{
public:
	typedef TabViewDragHandlerBase SuperClass;

	TabViewDataDragHandler (TabView* tabView, IDataTarget* dataTarget)
	: TabViewDragHandlerBase (tabView)
	{
		setDataTarget (dataTarget);
		showInsertPosition (true);
	}

	PROPERTY_SHARED_AUTO (IDataTarget, dataTarget, DataTarget)

	// IDragHandler
	tbool CCL_API afterDrop (const DragEvent& event) override
	{
		if(dataTarget)
			dataTarget->insertData (event.session.getItems (), &event.session, insertPos);
		return true;
	}
};

//************************************************************************************************
// ReorderTabsDragHandler
//************************************************************************************************

class ReorderTabsDragHandler: public TabViewDragHandlerBase
{
public:
	typedef TabViewDragHandlerBase SuperClass;

	ReorderTabsDragHandler (TabView* tabView)
	: TabViewDragHandlerBase (tabView),
	  dragTabIndex (-1)
	{
		showInsertPosition (true);
	}

	bool prepare ()
	{
		if(IParameter* param = tabView->getParameter ())
			targetController = param->getController ();

		return targetController.isValid ();
	}

	// IDragHandler
	tbool CCL_API dragEnter (const DragEvent& event) override
	{
		dragTabIndex = AttributeAccessor (event.session.getAttributes ()).getInt ("tabIndex");
		if(dragTabIndex < 0)
			return false;

		if(tabView->getStyle ().isTransparent () == false)
		{
			Rect tabRect;
			tabView->getRenderer ()->getPartRect (tabView, TabView::kPartFirstTab + dragTabIndex, tabRect);

			float contentScaleFactor = 1.f;
			if(Window* window = tabView->getWindow ())
				contentScaleFactor = window->getContentScaleFactor ();

			// tab sprite
			Coord tabW = tabRect.getWidth ();
			Coord tabH = tabRect.getHeight ();
			AutoPtr<Bitmap> bitmap (NEW Bitmap (tabW, tabH, Bitmap::kRGB, contentScaleFactor));
			UnknownPtr<ITabViewRenderer> tabRenderer (ccl_as_unknown (tabView->getRenderer ()));
			if(tabRenderer)
			{
				BitmapGraphicsDevice port (bitmap);
				tabRenderer->drawTab (tabView, port, Rect (0, 0, tabW, tabH), dragTabIndex);
			}
			AutoPtr<IDrawable> drawable = NEW ImageDrawable (bitmap, .5f);
		
			View* spriteView = tabView;
			if(View* window = tabView->getWindow ())
			{
				spriteView = window;
				tabView->clientToWindow (offset);
			}
			tabSprite = NEW FloatingSprite (spriteView, drawable, tabRect);
		}
		event.session.setResult (IDragSession::kDropMove);
		return SuperClass::dragEnter (event);
	}

	tbool CCL_API dragOver (const DragEvent& event) override
	{
		SuperClass::dragOver (event);

		if(targetController)
		{
			Boxed::Variant canReorder (true);
			Message msg (CCL::Signals::kTabViewCanReorder, Variant (tabView->getParameter ()->getName ()), dragTabIndex, insertPos, static_cast<IVariant*> (&canReorder));
			targetController->notify (tabView, msg);

			event.session.setResult (canReorder.asVariant ().asBool () ? IDragSession::kDropMove : IDragSession::kDropNone);
		}

		if(insertPos >= 0)
		{
			if(tabSprite)
			{
				Point pos = event.where + offset;
				pos.y = offset.y; // keep vertically aligned
				tabSprite->moveTo (pos);
			}

			if(insertPos > dragTabIndex)
				insertPos--;

			if(tabSprite && !tabSprite->isVisible ())
				tabSprite->show ();
		}
		return true;
	}

	tbool CCL_API afterDrop (const DragEvent& event) override
	{
		ASSERT (targetController)
		if(targetController && insertPos >= 0)
		{
			Message msg (Signals::kTabViewReorder, Variant (tabView->getParameter ()->getName ()), dragTabIndex, insertPos);
			targetController->notify (tabView, msg);
		}
		return SuperClass::afterDrop (event);
	}

	void cleanup () override
	{
		if(tabSprite)
			tabSprite->hide ();

		return SuperClass::cleanup ();
	}

protected:
	AutoPtr<Sprite> tabSprite;
	UnknownPtr<IObserver> targetController;
	int dragTabIndex;
	Point offset;
};

//************************************************************************************************
// TabViewCommandHandler
//************************************************************************************************

class TabViewCommandHandler: public Unknown,
							 public ICommandHandler
{
public:
	TabViewCommandHandler (TabView& tabView, int tabIndex)
	: tabView (tabView),
	  tabIndex (tabIndex)
	{}

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override
	{
		return true;
	}

	tbool CCL_API interpretCommand (const CommandMsg& msg) override
	{
		if(!msg.checkOnly ())
			tabView.activateTab (tabIndex);
		return true;
	}

	CLASS_INTERFACE (ICommandHandler, Unknown)

protected:
	TabView& tabView;
	int tabIndex;
};

//************************************************************************************************
// TabViewAccessibilityProvider
//************************************************************************************************

class TabViewAccessibilityProvider: public ViewAccessibilityProvider,
									public IAccessibilitySelectionContainerProvider
{
public:
	DECLARE_CLASS_ABSTRACT (TabViewAccessibilityProvider, ViewAccessibilityProvider)

	TabViewAccessibilityProvider (TabView& tabView);

	void rebuildTabProviders ();
	
	void getElementName (String& name, int tabIndex) const;
	void getElementBounds (Rect& rect, int tabIndex) const;
	int getActiveIndex () const;
	int countTabs () const;
	void select (int index);

	// ViewAccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;

	// IAccessibilitySelectionContainerProvider
	tresult CCL_API getSelectionProviders (IUnknownList& selection) const override;
	tbool CCL_API isSelectionRequired () const override { return true; }
	tbool CCL_API canSelectMultiple () const override { return false; }

	CLASS_INTERFACE (IAccessibilitySelectionContainerProvider, ViewAccessibilityProvider)

private:
	TabView& getTabView () const;
};

//************************************************************************************************
// TabItemAccessibilityProvider
//************************************************************************************************

class TabItemAccessibilityProvider: public AccessibilityProvider,
									public IAccessibilitySelectionProvider
{
public:
	DECLARE_CLASS_ABSTRACT (TabItemAccessibilityProvider, AccessibilityProvider)

	TabItemAccessibilityProvider (TabViewAccessibilityProvider& parent, int index);

	PROPERTY_VARIABLE (int, index, Index)

	// AccessibilityProvider
	AccessibilityProvider* findElementProvider (AccessibilityDirection direction) const override;
	void CCL_API getElementName (String& name) const override;
	AccessibilityElementRole CCL_API getElementRole () const override;
	tresult CCL_API getElementBounds (Rect& bounds, AccessibilityCoordSpace space) const override;
	View* getView () const override;

	// IAccessibilitySelectionProvider
	tbool CCL_API isSelected () const override;
	tresult CCL_API getPosition (int& index, int& total) const override;
	tresult CCL_API select (tbool state, int flags) override;
	IAccessibilityProvider* CCL_API getSelectionContainerProvider () const override;

	CLASS_INTERFACE (IAccessibilitySelectionProvider, AccessibilityProvider)

private:
	TabViewAccessibilityProvider& parent;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// TabView::TouchMouseHandler
//************************************************************************************************

class TabView::TouchMouseHandler: public MouseHandler
{
public:
	TouchMouseHandler (TabView* tabView)
	: MouseHandler (tabView)
	{}

	void onRelease (bool canceled) override
	{ 
		ASSERT (current.wasTouchEvent ())
		if(canceled)
			return;

		TabView* tabView = (TabView*)view;
		int partCode = tabView->getRenderer ()->hitTest (tabView, current.where, nullptr);
		if(partCode >= TabView::kPartFirstTab && partCode <= TabView::kPartLastTab)
		{
			int tabIndex = partCode - TabView::kPartFirstTab;

			tabView->mouseDown (current);
			tabView->redraw ();

			bool isTabMenu = false;
			if(tabView->getStyle ().isCustomStyle (Styles::kTabViewBehaviorTabMenu) && tabIndex == tabView->getActiveIndex ())
			{
				Rect tabMenuRect;
				isTabMenu = tabView->getRenderer ()->getPartRect (tabView, TabView::kPartTabMenu + tabIndex, tabMenuRect) && tabMenuRect.pointInside (current.where);
			}

			// defer opening the menu (finish touch handling first)
			if(isTabMenu)
				(NEW Message ("showMenu", tabIndex))->post (tabView);
			else
				tabView->mouseUp (current);
		}
	} 
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (ITabViewRenderer, 0x410AEEBA, 0x29BD, 0x4F31, 0x8C, 0x1C, 0x1B, 0xC9, 0x28, 0xEB, 0x04, 0xAA)

BEGIN_STYLEDEF (TabView::customStyles)
	{"drag",	Styles::kTabViewBehaviorCanDragTabData},
	{"reorder",	Styles::kTabViewBehaviorCanReorderTabs},
	{"nomenu",	Styles::kTabViewBehaviorNoMenu},
	{"extendtabs", Styles::kTabViewBehaviorExtendTabs},
	{"tabmenu",	Styles::kTabViewBehaviorTabMenu},
	{"nowheel",	Styles::kTabViewBehaviorNoWheel},
	{"nohoveractivate",	Styles::kTabViewBehaviorNoActivateOnHover}, // style name must not contain "drag" (this also sets kTabViewCanDragTabData)
	{"fitallviews",	Styles::kTabViewBehaviorFitAllViews},
	{"centered", Styles::kTabViewAppearanceCentered},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (TabView, Control)

//////////////////////////////////////////////////////////////////////////////////////////////////

TabView::TabView (const Rect& size, IParameter* param, StyleRef style)
: Control (size, param, style),
  renderer (nullptr),
  scrollOffset (0),
  centerOffset (0),
  fillWidth (0),
  menu (false),
  mouseOverTab (-1),
  mouseDownTab (-1),
  preferIcon (false)
{
	if(param == nullptr)
	{
		setParameter (nullptr); // release parameter of base class
		enable (true);
	}
	setWheelEnabled (style.isCustomStyle (Styles::kTabViewBehaviorNoWheel) == false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TabView::~TabView ()
{
	cancelSignals ();

	View* view;
	while((view = views.getFirst ()) != nullptr)
	{
		views.remove (view);
		view->release ();
	}
	if(renderer)
		renderer->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabView::canDragTabs (const MouseEvent& event) const
{
	#if DRAG_NEEDS_CMD
	if(!event.keys.isSet (KeyState::kCommand))
		return false;
	#endif
	return getStyle ().isCustomStyle (Styles::kTabViewBehaviorCanReorderTabs|Styles::kTabViewBehaviorCanDragTabData);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::draw (const UpdateRgn& updateRgn)
{
	ThemeRenderer* renderer = getRenderer ();
	if(renderer)
		renderer->draw (this, updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* TabView::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kTabViewRenderer, visualStyle);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::invalidateHeader ()
{
	Rect rect;
	getRenderer ()->getPartRect (this, kPartHeader, rect);
	invalidate (rect);
	
	if(isAccessibilityEnabled ())
		if(TabViewAccessibilityProvider* provider = ccl_cast<TabViewAccessibilityProvider> (accessibilityProvider))
			provider->rebuildTabProviders ();

}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::onSize (const Point& delta)
{
	invalidate ();
	SuperClass::onSize (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabView::addView (View* view)
{
	views.append (view);
	
	if(isAccessibilityEnabled ())
		if(TabViewAccessibilityProvider* provider = ccl_cast<TabViewAccessibilityProvider> (accessibilityProvider))
			provider->rebuildTabProviders ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabView::removeView (View* view)
{
	views.remove (view);
	invalidateHeader ();
	if(view == getFirst ())
	{
		View::removeView (view);
		view->release ();

		if(views.count () > 0)
			activateTab (0);
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* TabView::getViewState (bool create)
{
	if(!persistenceID.isEmpty ())
		if(ILayoutStateProvider* provider = GetViewInterfaceUpwards<ILayoutStateProvider> (this))
			return provider->getLayoutState (persistenceID, create);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::attached (View* parent)
{
	// call baseclass first to avoid double-attaching our visible child!
	SuperClass::attached (parent);

	// select initial tab
	init (getViewState (false));
	updateStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::updateStyle ()
{
	const IVisualStyle& vs = getVisualStyle ();
	preferIcon = vs.getMetric<bool> ("prefericon", preferIcon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::removed (View* parent)
{
	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	// forward to hidden views
	View* activeView = getFirst ();
	ListForEach (views, View*, v)
		if(v != activeView)
			v->onColorSchemeChanged (event);
	EndFor

	SuperClass::onColorSchemeChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::init (IAttributeList* savedState)
{
	if(param)
		paramChanged ();
	else
	{
		int tabIndex = 0;
		if(savedState)
		{
			if(AttributeAccessor (*savedState).getInt (tabIndex, "tabIndex"))
				tabIndex = ccl_bound (tabIndex, 0, countTabs ()-1);
		}
		
		activateTab (tabIndex);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::activateTab (int index)
{
	View* view = views.at (index);
	View* first = getFirst ();

	if(view == first && view != nullptr)
	{
		invalidateHeader (); // same param index, but values (e.g. order) might have changed
		return;
	}

	if(first != nullptr)
	{
		View::removeView (first);
		first->release ();
	}

	if(view)
	{
		Rect size;
		getViewSize (size);
		view->setSize (size);
		view->setSizeMode (View::kAttachAll);

		View::addView (view);
		view->retain ();
	}

	invalidateHeader ();

	if(param)
		param->setValue (index, true);

	if((privateFlags & kExplicitSizeLimits) == 0)
	{
		privateFlags &= ~kSizeLimitsValid;
		if(parent)
			parent->onChildLimitsChanged (this);
	}

	// save active tab
	if(IAttributeList* a = getViewState (true))
		AttributeAccessor (*a).set ("tabIndex", getActiveIndex ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::paramChanged ()
{
	activateTab (param->getValue ().asInt ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* TabView::getTabView (int index) const
{
	return views.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef TabView::getTabTitle (String& title, int index) const
{
	if(getTabIcon (index) && preferIcon)
		return String::kEmpty;
		
	if(View* view = getTabView (index))
		title = view->getTitle ();
	else if(param)
		param->getString (title, index);
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* TabView::getTabIcon (int index) const
{
	#if 0 // try visual style (unused yet)
	if(visualStyle)
	{
		MutableCString iconName;
		iconName.appendFormat ("tabIcon%d", index);
		if(IImage* icon = visualStyle->getImage (iconName))
			return icon;
	}
	#endif

	// try parameter
	UnknownPtr<IListParameter> listParam (param);
	if(listParam)
	{
		Variant v = listParam->getValueAt (index);
		if(v.isObject ())
			return UnknownPtr<IImage> (v.asUnknown ());
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TabView::countTabs () const
{
	return param ? param->getMax ().asInt () - param->getMin ().asInt () + 1 : views.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TabView::getActiveIndex () const
{
	if(param)
		return param->getValue ().asInt ();

	View* activeView = getFirst ();
	int index = 0;
	ListForEach (views, View*, v)
		if(v == activeView)
			return index;
		index++;
	EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TabView::findTab (const Point& where)
{
	int partCode = getRenderer ()->hitTest (this, where, nullptr);
	if(partCode >= kPartFirstTab && partCode <= kPartLastTab)
		return partCode - kPartFirstTab;
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TabView::getMouseOverTab () const
{
	return mouseOverTab;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TabView::getMouseDownTab () const
{
	return mouseDownTab;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::getViewSize (Rect& size)
{
	ThemeRenderer* renderer = getRenderer ();
	if(renderer == nullptr)
		return;
	renderer->getPartRect (this, kPartContent, size);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabView::onMouseEnter (const MouseEvent& event)
{
	return onMouseMove (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabView::onMouseLeave (const MouseEvent& event)
{
	setMouseOver (-1);
	setMouseDown (-1);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabView::onMouseMove (const MouseEvent& event)
{
	int partCode = getRenderer ()->hitTest (this, event.where, nullptr);
	bool isOverTab = partCode >= kPartFirstTab && partCode <= kPartLastTab;

	bool isDragging = DragSession::isInternalDragActive ();
	#if DRAG_NEEDS_CMD
	if(!isDragging && isOverTab && canDragTabs (event))
		setCursor (getTheme ().getCursor ("GrabCursor"));
	else
		setCursor ((MouseCursor*)0);
	#endif

	if(isOverTab)
	{
		if(!isDragging && event.keys.isSet (KeyState::kLButton))
			setMouseDown (partCode - kPartFirstTab);
		else		
			setMouseOver (partCode - kPartFirstTab);
		return true;
	}

	setMouseOver (-1);
	setMouseDown (-1);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabView::onMouseWheel (const MouseWheelEvent& event)
{
	if(getRenderer ()->hitTest (this, event.where, nullptr) == TabView::kPartContent)
		return View::onMouseWheel (event);
	
	return SuperClass::onMouseWheel (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::setMouseOver (int tab)
{
	if(tab != mouseOverTab)
	{
		invalidateTab (mouseOverTab);
		invalidateTab (tab);
		mouseOverTab = tab;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::setMouseDown (int tab)
{
	if(tab != mouseDownTab)
	{
		invalidateTab (mouseDownTab);
		invalidateTab (tab);
		mouseDownTab = tab;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::invalidateTab (int index)
{
	if(index < 0)
		return;

	Rect rect;
	getRenderer ()->getPartRect (this, kPartFirstTab + index, rect);
	invalidate (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabView::mouseDown (const MouseEvent& event)
{
	int partCode = getRenderer ()->hitTest (this, event.where, nullptr);
	if(partCode >= kPartFirstTab && partCode <= kPartLastTab)
	{
		onMouseMove (event);
		return true;
	}
	return SuperClass::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabView::mouseUp (const MouseEvent& event)
{
	int partCode = getRenderer ()->hitTest (this, event.where, nullptr);
	if(partCode >= kPartFirstTab && partCode <= kPartLastTab)
	{
		if(partCode == kPartMenuTab)
			(NEW Message ("showMenu"))->post (this);
		else
			activateTab (partCode - kPartFirstTab);

		setMouseDown (-1);
		return true;
	}
	return SuperClass::onMouseUp (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::dragTab (int tabIndex)
{
	if(!isAttached ())
		return;

	AutoPtr<DragSession> session (DragSession::create (this->asUnknown ()));
	session->setSource (this->asUnknown ());
	setCursor ((MouseCursor*)nullptr);
	AttributeAccessor (session->getAttributes ()).set ("tabIndex", tabIndex);

	if(getStyle ().isCustomStyle (Styles::kTabViewBehaviorCanDragTabData))
	{
		IParameter* param = getParameter ();
		UnknownPtr<IObserver> controller (param ? param->getController () : nullptr);
		if(controller)
		{
			Message msg (Signals::kTabViewBeforeDrag, Variant (param->getName ()), tabIndex, Variant (session->asUnknown ()));
			controller->notify (this, msg);
		}
	}
	session->drag ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* TabView::createMouseHandler (const MouseEvent& event)
{
	int partCode = getRenderer ()->hitTest (this, event.where, nullptr);
	if(partCode >= kPartFirstTab && partCode <= kPartLastTab)
	{
		if(event.wasTouchEvent ())
			return NEW TouchMouseHandler (this);

		SharedPtr<IUnknown> holder (this->asUnknown ()); // view might get removed during drag & drop

		int tabIndex = partCode - kPartFirstTab;
		#if DRAG_NEEDS_CMD
		if(canDragTabs (event))
		{
			dragTab (tabIndex);
			return NEW NullMouseHandler (this);
		}

		if(tabIndex == getActiveIndex ())
			return 0; // behave transparently when clicked on the active tab (e.g. move window)

		if(event.keys.isSet (KeyState::kLButton))
			return NEW TabViewMouseHandler (this);

		#else
		mouseDown (event);
		redraw ();

		bool isTabMenu = false;
		if(getStyle ().isCustomStyle (Styles::kTabViewBehaviorTabMenu) && tabIndex == getActiveIndex ())
		{
			Rect tabMenuRect;
			isTabMenu = renderer->getPartRect (this, kPartTabMenu + tabIndex, tabMenuRect) && tabMenuRect.pointInside (event.where);
		}

		if(isTabMenu)
		{
			showMenu (tabIndex);

			// TabView might have been removed during execution of a menu command: avoid querying the parameter during invalidateTab (renderer calls getTabIcon)
			// the parameter's controller might have been destroyed already, so an "abandoned" ListParam might contain references to already destroyed objects
			if(isAttached ())
				setMouseDown (-1);
		}
		else if(canDragTabs (event) && detectDrag (event))
			dragTab (tabIndex);
		else
			mouseUp (event);

		return NEW NullMouseHandler (this);
		#endif
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* TabView::createDragHandler (const DragEvent& event)
{
	if(event.session.getSource () == this->asUnknown () && getStyle ().isCustomStyle (Styles::kTabViewBehaviorCanReorderTabs))
	{
		ReorderTabsDragHandler* reorderHandler = NEW ReorderTabsDragHandler (this);
		if(reorderHandler->prepare ())
			return reorderHandler;
		else
			reorderHandler->release ();
	}

	IParameter* param = getParameter ();
	UnknownPtr<IObserver> controller (param ? param->getController () : nullptr);
	if(controller)
	{
		Boxed::Variant result;
		Message msg (Signals::kTabViewGetDataTarget, Variant (param->getName ()), static_cast<IVariant*> (&result), &event.session.getItems (), &event.session);
		controller->notify (this, msg);

		UnknownPtr<IDataTarget> dataTarget (result.asVariant ());

		if(dataTarget && dataTarget->canInsertData (event.session.getItems (), &event.session, this))
		{
			if(event.session.getResult () == DragSession::kDropNone)
				event.session.setResult (DragSession::kDropCopyReal);

			// use dragHandler provided by dataTarget, or own handler that calls feeds dataTarget on drop
			if(IDragHandler* dragHandler = event.session.getDragHandler ())
				return return_shared (dragHandler);
			else
				return NEW TabViewDataDragHandler (this, dataTarget);
		}
	}


	if(getStyle ().isCustomStyle (Styles::kTabViewBehaviorNoActivateOnHover) == false)
		return NEW TabViewDragHandler (this);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* CCL_API TabView::createDragHandler (int flags, IItemDragVerifier* verifier)
{
	TabViewDragHandlerBase* handler = NEW TabViewDragHandlerBase (this);
	handler->showInsertPosition (flags & IItemView::kCanDragBetweenItems);
//	handler->hiliteMouseOverTab (flags & IItemView::kCanDragOnItem); // todo (handler must decide if between or on tab)
	return handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::showMenu (int tabIndex)
{
	AutoPtr<Menu> menu = NEW ExtendedMenu;

	PopupSizeInfo sizeInfo (this, PopupSizeInfo::kBottom);
	sizeInfo.canFlipParentEdge (true);
		
	const IVisualStyle& vs = getVisualStyle ();
	if(Coord tabHeight = vs.getMetric ("tabHeight", 0))
	{
		sizeInfo.flags |= PopupSizeInfo::kHasOffset;
		Coord menuShift = getHeight () - tabHeight;
		sizeInfo.where (0, -menuShift);
	}
		
	if(tabIndex < 0) // tab selection menu
	{
		int tabCount = countTabs ();
		if(param)
		{
			AutoPtr<ParameterMenuBuilder> menuBuilder (NEW ParameterMenuBuilder (param));
			menuBuilder->setDefaultTitleEnabled (false);
			menuBuilder->setExtensionEnabled (false);
			menuBuilder->buildMenu (menu);
		}
		else
		{
			int activeIndex = getActiveIndex ();

			for(int i = 0; i < tabCount; i++)
			{
				String tabTitle;
				getTabTitle (tabTitle, i);
				AutoPtr<ICommandHandler> handler = NEW TabViewCommandHandler (*this, i);
				IMenuItem* menuItem = static_cast<IMenu*> (menu)->addCommandItem (tabTitle, "Tab View", "Activate Tab", handler);
				if(menuItem && i == activeIndex)
					menuItem->setItemAttribute (IMenuItem::kItemChecked, true);
			}
		}

		ASSERT (menu->countItems () == countTabs ())

		// assign tab icons
		for(int i = 0; i < menu->countItems (); i++)
		{
			MenuItem* menuItem = menu->at (i);
			if(Image* icon = unknown_cast<Image> (getTabIcon (i)))
			{
				// check for special frame for menu
				if(Filmstrip* filmstrip = ccl_cast<Filmstrip> (icon))
				{
					if(Image* menuIcon = filmstrip->getSubFrame ("menu"))
						icon = menuIcon;
				}
				else if(MultiImage* multiImage = ccl_cast<MultiImage> (icon))
				{
					if(Image* menuIcon = multiImage->getFrame (multiImage->getFrameIndex ("menu")))
						icon = menuIcon;
				}
					
				menuItem->setIcon (icon);
			}
		}

		sizeInfo.flags |= PopupSizeInfo::kRight;
	}
	else
	{
		// let controller build menu for tab
		UnknownPtr<IObserver> controller (param ? param->getController () : nullptr);
		if(controller)
		{
			Message msg (Signals::kTabViewTabMenu, Variant (param->getName ()), tabIndex, menu->asUnknown ());
			controller->notify (this, msg);
		}
	
		Rect rect;
		getRenderer ()->getPartRect (this, kPartFirstTab + tabIndex, rect);
		sizeInfo.flags = 0;
		sizeInfo.where = rect.getLeftBottom ();
	}

	if(menu->isEmpty ())
		return;

	PopupSelector popupSelector;
	popupSelector.setTheme (getTheme ());
	popupSelector.setVisualStyle (getTheme ().getStandardStyle (ThemePainter::kPopupMenuStyle));
	popupSelector.popup (menu, sizeInfo, MenuPresentation::kTree);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TabView::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "activeView")
	{
		String viewName (var.asString ());
		int i = 0;
		ListForEach (views, View*, v)
			if(v->getName () == viewName)
			{
				activateTab (i);
				break;
			}
			i++;
		EndFor
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TabView::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "showMenu")
	{
		int tabIndex = msg.getArgCount () > 0 ? msg[0].asInt () : -1;
		showMenu (tabIndex);
	}
	else if(msg == "activateTab")
		activateTab (msg[0]);
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabView::calcAutoSize (Rect& r)
{
	if(style.isCustomStyle (Styles::kTabViewBehaviorFitAllViews))
	{
		// largest size of all content views
		r.setEmpty ();
		for(auto view : views)
			r.join (view->getSize ());

		// add header 
		Rect header;
		ThemeRenderer* renderer = getRenderer ();
		if(renderer && renderer->getPartRect (this, kPartHeader, header))
		{
			if(style.isVertical ())
				r.right += header.getWidth ();
			else
				r.bottom += header.getHeight ();
		}
		return;
	}

	init ();

	if(views.isEmpty ())
	{
		ThemeRenderer* renderer = getRenderer ();
		if(renderer == nullptr)
			return;
		renderer->getPartRect (this, kPartViewSize, r);
	}
	else
		SuperClass::calcAutoSize (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabView::onContextMenu (const ContextMenuEvent& event)
{
	int partCode = getRenderer ()->hitTest (this, event.where, nullptr);
	if(partCode >= kPartFirstTab && partCode <= kPartLastTab)
	{
		int tabIndex = partCode - kPartFirstTab;

		MutableCString contextID ("TabView:");
		if(param)
		{
			contextID += param->getName ();
			contextID += ":";
		}
		contextID.appendFormat ("%d", tabIndex);
		event.contextMenu.setContextID (contextID);
	}
	return SuperClass::onContextMenu (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* TabView::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW TabViewAccessibilityProvider (*this);
	return accessibilityProvider;
}

//************************************************************************************************
// TabViewAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TabViewAccessibilityProvider, ViewAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

TabViewAccessibilityProvider::TabViewAccessibilityProvider (TabView& tabView)
: ViewAccessibilityProvider (tabView)
{
	rebuildTabProviders ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewAccessibilityProvider::rebuildTabProviders ()
{
	ArrayForEachReverse (getChildren (), AccessibilityProvider, item)
		if(TabItemAccessibilityProvider* provider = ccl_cast<TabItemAccessibilityProvider> (item))
			removeChildProvider (item);
	EndFor
	
	ASSERT (AccessibilityManager::isEnabled ())
	
	const TabView& tabView = getTabView ();
	for(int i = 0; i < countTabs (); i++)
	{
		AutoPtr<TabItemAccessibilityProvider> child = NEW TabItemAccessibilityProvider (*this, i);
		addChildProvider (child);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API TabViewAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kTabView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewAccessibilityProvider::getElementName (String& name, int tabIndex) const
{
	getTabView ().getTabTitle (name, tabIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewAccessibilityProvider::getElementBounds (Rect& rect, int tabIndex) const
{
	if(getTabView ().getRenderer ()->getPartRect (&getTabView (), TabView::kPartFirstTab + tabIndex, rect))
	{
		Rect clipping;
		getTabView ().getVisibleClient (clipping);
		rect.bound (clipping);
		Point screenOffset;
		getTabView ().clientToScreen (screenOffset);
		rect.offset (screenOffset);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TabViewAccessibilityProvider::getActiveIndex () const
{
	 return getTabView ().getActiveIndex ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TabViewAccessibilityProvider::countTabs () const
{
	return getTabView ().countTabs ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TabView& TabViewAccessibilityProvider::getTabView () const
{
	return static_cast<TabView&> (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TabViewAccessibilityProvider::getSelectionProviders (IUnknownList& selection) const
{
	int activeIndex = getActiveIndex ();
	for(AccessibilityProvider* provider : iterate_as<AccessibilityProvider> (children))
	{
		TabItemAccessibilityProvider* itemProvider = ccl_cast<TabItemAccessibilityProvider> (provider);
		if(itemProvider == nullptr)
			continue;
		if(itemProvider->getIndex () == activeIndex)
		{
			selection.add (itemProvider->asUnknown (), true);
			break;
		}
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewAccessibilityProvider::select (int index)
{
	getTabView ().activateTab (index);
}

//************************************************************************************************
// TabItemAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TabItemAccessibilityProvider, AccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

TabItemAccessibilityProvider::TabItemAccessibilityProvider (TabViewAccessibilityProvider& parent, int index)
: parent (parent),
  index (index)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* TabItemAccessibilityProvider::findElementProvider (AccessibilityDirection direction) const
{
	if(direction == AccessibilityDirection::kParent)
		return &parent;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TabItemAccessibilityProvider::getElementName (String& name) const
{
	parent.getElementName (name, index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API TabItemAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kTabItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TabItemAccessibilityProvider::getElementBounds (Rect& b, AccessibilityCoordSpace space) const
{
	parent.getElementBounds (b, index);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* TabItemAccessibilityProvider::getView () const
{
	return parent.getView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TabItemAccessibilityProvider::isSelected () const
{
	return index == parent.getActiveIndex ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TabItemAccessibilityProvider::getPosition (int& position, int& total) const
{
	position = index;
	total = parent.countTabs ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TabItemAccessibilityProvider::select (tbool state, int flags)
{
	if(state == false || !get_flag<int> (flags, kExclusive))
		return kResultInvalidArgument;
	parent.select (index);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAccessibilityProvider* CCL_API TabItemAccessibilityProvider::getSelectionContainerProvider () const
{
	return parentProvider;
}
