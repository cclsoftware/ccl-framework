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
// Filename    : ccl/gui/layout/dockpanel.cpp
// Description : Docking Panel
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/layout/dockpanel.h"
#include "ccl/gui/layout/anchorlayout.h"
#include "ccl/public/gui/iviewfactory.h"

#include "ccl/base/message.h"

#include "ccl/gui/windows/window.h"

#include "ccl/app/params.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IDockPanelItem, 0xa9a4b86f, 0x796b, 0x40c1, 0x87, 0xd0, 0xf6, 0xc6, 0xbf, 0x61, 0xfb, 0x92)
DEFINE_IID_ (IDockPanelView, 0x4f802007, 0x7e3a, 0x4aad, 0xb2, 0x6c, 0xf2, 0x3b, 0x1e, 0x51, 0x3d, 0x6a)

//************************************************************************************************
// DockPanelItem
//************************************************************************************************

DEFINE_CLASS (DockPanelItem, ObjectNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

DockPanelItem::DockPanelItem (StringRef _name)
: ObjectNode (_name),
  state (0),
  visible (nullptr),
  view (nullptr),
  controller (nullptr),
  viewFactory (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DockPanelItem::DockPanelItem (const DockPanelItem& item)
: ObjectNode (item),
  state (item.state & ~kVisible),
  visible (nullptr),
  view (nullptr),
  controller (nullptr),
  viewFactory (nullptr) //???
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DockPanelItem::~DockPanelItem ()
{
	if(viewFactory)
		viewFactory->release ();

	signal (Message (kDestroyed));

	if(visible)
		visible->release ();

	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DockPanelItem::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IController)
	QUERY_INTERFACE (IParamObserver)
	QUERY_INTERFACE (IDockPanelItem)
	return ObjectNode::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDRef CCL_API DockPanelItem::getObjectUID () const
{
	UnknownPtr<IObjectNode> iNode = controller;
	if(iNode)
		return iNode->getObjectUID ();
	return SuperClass::getObjectUID ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DockPanelItem::init (StringRef name, IUnknown* controller, int state, IParameter* titleParam)
{
	setName (name);
	setController (controller);
	this->state = state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DockPanelItem::setViewFactory (IViewFactory* factory)
{
	take_shared<IViewFactory> (viewFactory, factory);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* DockPanelItem::getVisible () const
{
	if(!isHidable ())
		return nullptr;

	if(!visible)
	{
		visible = NEW Parameter;
		visible->connect (const_cast<DockPanelItem*> (this), 100);
		if(getView ())
			visible->setValue (1);
	}
	return visible;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DockPanelItem* DockPanelItem::getParentItem () const
{
	return getParentNode<DockPanelItem> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* DockPanelItem::getParentView () const
{
	DockPanelItem* parentItem = getParentItem ();
	return parentItem ? parentItem->getView () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DockPanelItem::traverse (IDockPanelItemVisitor& visitor)
{
	visitor.visit (*this);

	// recursion
	ForEach (*this, DockPanelItem, child)
		child->traverse (visitor);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DockPanelItem::countParameters () const
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API DockPanelItem::getParameterAt (int index) const
{
	return index == 0 ? getVisible () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API DockPanelItem::findParameter (StringID name) const
{
	if(name == "visible")
		return getVisible ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DockPanelItem::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "show")
	{
		show ();
	}
	else if(msg == "hide")
	{
		hide ();
	}
	else if(msg == kChanged)
	{
		UnknownPtr<ISubject> root = getRoot ();
		if(root)
			root->signal (Message(kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DockPanelItem::paramChanged (IParameter* param)
{
	if(param == getVisible ())
	{
		Message* m = NEW Message (param->getValue () ? "show" : "hide");
		m->post (this);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DockPanelItem::reset ()
{
	if(countChildren () > 0)
	{
		ForEach (getChildren (), DockPanelItem, item)
			item->reset ();
		EndFor
	}
	if(view != nullptr)
		view = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DockPanelItem::collectItemsFlat (Container& container)
{
	if(countChildren () > 0)
	{
		ForEach (getChildren (), DockPanelItem, item)
			item->collectItemsFlat (container);
		EndFor
	}
	else
		container.add (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DockPanelItem::show ()
{
	bool shown = view != nullptr;
	if(!shown)
	{
		DockPanelItem* parent = getParentItem ();
		if(parent)
		{
			// recursively show parents
			if(!parent->isVisible ())
				parent->show ();

			if(parent->getView ())
			{
				int index = parent->getIndex (this);
				ASSERT (index != -1)

				view = createView (parent->getView ()->getTheme ());
				if(view)
				{
					parent->getView ()->insertView (index, view);
					shown = true;
				}
			}
		}
		else
		{
			CCL_NOT_IMPL ("DockPanelItem without parent!!!")
		}
	}

	isVisible (shown);
	if(visible)
		visible->setValue (shown ? 1 : 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DockPanelItem::hide ()
{
	ASSERT (isHidable () == true)

	if(view)
	{
		if(view->getParent ())
		{
			view->getParent ()->removeView (view);

			// recursively hide empty parents
			DockPanelItem* parent = getParentItem ();
			if(parent && parent->isHidable ())
			{
				View* parentView = parent->getView ();
				if(parentView && parentView->isEmpty ())
					parent->hide ();
			}
		}
		
		if(view)
		{
			view->release ();
			view = nullptr;
		}
	}

	isVisible (false);
	if(visible)
		visible->setValue (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DockPanelItem::hideAll ()
{
	struct Helper
	{
		static void resetViewPointers (DockPanelItem& item)
		{
			if(ccl_cast<Window> (item.getView ())) // stop at (popup) window boundaries (skip unrelated view trees)
				return;

			item.setView (nullptr);

			// recursion
			ForEach (item, DockPanelItem, child)
				resetViewPointers (*child);
			EndFor
		}
	};

	// first remove & destroy this view with all childs (avoid useless resizing of moribund views when removing them bottom-up in hide ())
	if(view && !ccl_cast<Window> (view))
	{
		if(view->getParent ())
			view->getParent ()->removeView (view);

		View* thisView = view;

		// reset all view pointers of this and child items
		Helper::resetViewPointers (*this);

		// destroy view (including all childs)
		thisView->release ();
	}

	if(countChildren () > 0)
		ForEach (getChildren (), DockPanelItem, item)
			item->hideAll ();
		EndFor

	if(isHidable ())
		hide ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DockPanelItem::kill ()
{
	if(viewFactory)
	{
		viewFactory->release ();
		viewFactory = nullptr;
	}

	hide ();

	UnknownPtr<ISubject> root = getRoot ();

	DockPanelItem* parentItem = getParentItem ();
	if(parentItem)
		parentItem->removeChild (this);

	if(root)
		root->signal (Message (kChanged));

	release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* DockPanelItem::createView (Theme& theme)
{
	if(viewFactory)
		return unknown_cast<View> (viewFactory->createView (MutableCString (getName ()), Variant (getController ()), Rect()));
	return unknown_cast<View> (theme.createView (MutableCString (getName ()), getController ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DockPanelItem::setViewAndState (View* view)
{
	setView (view);
	isVisible (view != nullptr);
	if(visible)
		visible->setValue (view ? 1 : 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DockPanelItem::getIndex (DockPanelItem* searchItem, bool onlyVisible) const
{
	int idx = 0;
	if(countChildren () > 0)
		ForEach (getChildren (), DockPanelItem, item)
			if(item == searchItem)
				return idx;

			if(!onlyVisible || (onlyVisible && item->isVisible ()))
				idx++;
		EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DockPanelItem::addItem (IDockPanelItem* _item)
{
	DockPanelItem* item = unknown_cast<DockPanelItem> (_item);
	ASSERT (item != nullptr)
	if(!item)
		return false;

	tbool result = addChild (item);

	UnknownPtr<ISubject> root = getRoot ();
	if(root)
		root->signal (Message (kChanged));

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DockPanelItem::removeItems ()
{
	hide ();
	removeAll ();
	reset ();

	UnknownPtr<ISubject> root = getRoot ();
	if(root)
		root->signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDockPanelItem* CCL_API DockPanelItem::findItem (IUnknown* controller, tbool deep)
{
	if(countChildren () > 0)
		ForEach (getChildren (), DockPanelItem, item)
			if(isEqualUnknown (item->getController (), controller))
				return item;

			if(deep)
			{
				IDockPanelItem* result = item->findItem (controller, true);
				if(result)
					return result;
			}
		EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DockPanelItem* DockPanelItem::findChildItem (IRecognizer& recognizer) const
{
	ForEach (getChildren (), DockPanelItem, item)
		if(recognizer.recognize (item->asUnknown ()))
			return item;

		if(DockPanelItem* child = item->findChildItem (recognizer)) // recursion
			return child;
	EndFor
	return nullptr;
}

//************************************************************************************************
// DockPanelGroup
//************************************************************************************************

DEFINE_CLASS (DockPanelGroup, DockPanelItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

DockPanelGroup::DockPanelGroup (StringRef name)
: DockPanelItem (name),
  style (Styles::kHorizontal)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* DockPanelGroup::createView (Theme& theme)
{
	ThemeSelector selector (theme);
	BoxLayoutView* frame = NEW BoxLayoutView (Rect (), style);
	frame->setSizeMode (View::kFitSize);
	frame->setSpacing (0);
	frame->setMargin (0);
	ForEach (getChildren (), DockPanelItem, item)
		if(item->isVisible ())
		{
			ASSERT (item->getView () == nullptr)

			View* v = item->createView (theme);
			if(v)
			{
				item->setViewAndState (v);
				frame->addView (v);
			}
		}
	EndFor
	//frame->autoSize ();
	return frame;
}

//************************************************************************************************
// DockPanelRoot
//************************************************************************************************

DEFINE_CLASS (DockPanelRoot, DockPanelGroup)

//////////////////////////////////////////////////////////////////////////////////////////////////

DockPanelRoot::DockPanelRoot (StringRef name)
: DockPanelGroup (name),
  ownerView (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DockPanelRoot::DockPanelRoot (const DockPanelRoot& root)
: DockPanelGroup (root),
  ownerView (nullptr)
{}

//************************************************************************************************
// DockPanelView
//************************************************************************************************

DEFINE_CLASS (DockPanelView, View)

//////////////////////////////////////////////////////////////////////////////////////////////////

DockPanelView::DockPanelView (const Rect& size, StyleRef style)
: ImageView (nullptr, size, style),
  items (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DockPanelView::~DockPanelView ()
{
	items->reset ();
	if(items)
		items->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DockPanelView::setItems (IDockPanelItem* _items)
{
	DockPanelItem* group = unknown_cast<DockPanelItem> (_items);
	ASSERT (group != nullptr)
	if(!group)
		return;

	// remove old views
	removeAll ();

	take_shared<DockPanelItem> (items, group);
	if(!items)
		return;

	View* v = items->createView (getTheme ());
	if(v)
	{
		items->setViewAndState (v);
		items->isHidable (false); 
		// Note: root must not be hidable, otherwise it would never ever appear again if hidden once!!!

		addView (v);

		if(style.isCustomStyle (kFitSize))
			autoSize ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDockPanelItem* CCL_API DockPanelView::getItems ()
{
	return items; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DockPanelView::onChildSized (View* child, const Point& delta)
{
	SuperClass::onChildSized (child, delta);

	signal (Message (kChanged));
}
