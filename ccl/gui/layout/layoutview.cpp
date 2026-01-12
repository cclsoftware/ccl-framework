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
// Filename    : ccl/gui/layout/layoutview.cpp
// Description : A set of base classes for layout views
//
//************************************************************************************************

#include "ccl/gui/layout/layoutview.h"

#include "ccl/base/message.h"
#include "ccl/public/gui/framework/skinxmldefs.h"

using namespace CCL;

//************************************************************************************************
// LayoutContext
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LayoutContext, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutContext::LayoutContext ()
: parentView (nullptr)
{
	ASSERT (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutContext::LayoutContext (LayoutView* parentView)
: parentView (parentView)
{
	ASSERT (parentView != nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectArray& LayoutContext::getLayoutItems () const
{
	return parentView->getLayoutItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutItem* LayoutContext::findLayoutItem (View* view) const
{
	return parentView->findLayoutItem (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RectRef LayoutContext::getLayoutRect () const
{
	return parentView->getSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord LayoutContext::getLayoutWidth () const
{
	return parentView->getWidth ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord LayoutContext::getLayoutHeight () const
{
	return parentView->getHeight ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int LayoutContext::getSizeMode () const
{
	return parentView->getSizeMode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutContext::requestAutoSize (tbool horizontal, tbool vertical) const
{
	if(!parentView->isResizing ())
		parentView->autoSize (horizontal, vertical);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutContext::requestResetSizeLimits (bool checkExplicit) const
{
	if(checkExplicit && parentView->hasExplicitSizeLimits ())
		return;

	parentView->resetSizeLimits ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutContext::hideItem (LayoutItem* item) const
{
	if(!item->isHidden ())
	{
		item->isHidden (true);
		parentView->View::removeView (item->getView ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutContext::showItem (LayoutItem* item) const
{
	if(item->isHidden ())
	{
		ASSERT (item->getView () && !item->getView ()->getParent ())

		// determine index
		int index = 0;
		ArrayForEachFast (parentView->getLayoutItems (), LayoutItem, i)
			if(item == i)
			{
				item->isHidden (false);
				parentView->View::insertView (index, item->getView ());
				return;
			}

			if(!i->isHidden ())
				index++;
		EndFor
	}
}

//************************************************************************************************
// Layout
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Layout, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Layout::setProperty (MemberID propertyId, const Variant& var)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Layout::getProperty (Variant& var, MemberID propertyId) const
{
	return false;
}

//************************************************************************************************
// LayoutView::AttachSuspender
/**
	View::addView () / insertView () calls attached () for the child view and then onViewsChanged (), where we do the layout.
	But we want to perform the initial layout _before_ the view is attached, so we supend the attached () call while adding a view
	and call it afterwards. See LayoutView::isAttached ().

	(This e.g. avoids a contained PlugInView (with a platform ChildWindow) from briefly becoming visible at a wrong position) */
//************************************************************************************************

class LayoutView::AttachSuspender
{
public:
	AttachSuspender (LayoutView& layoutView, View& childView)
	: layoutView (layoutView),
	  childView (childView),
	  wasAttachSuspended (layoutView.isAttachSuspended ())
	{
		layoutView.isAttachSuspended (true);
	}

	~AttachSuspender ()
	{
		layoutView.isAttachSuspended (wasAttachSuspended);

		if(layoutView.isAttached ())
			childView.attached (&layoutView);
	}

private:
	LayoutView& layoutView;
	View& childView;
	bool wasAttachSuspended;
};

//************************************************************************************************
// LayoutView
//************************************************************************************************

DEFINE_CLASS (LayoutView, View)
DEFINE_CLASS_UID (LayoutView, 0x53176234, 0xB55E, 0x1749, 0xB7, 0x77, 0x76, 0x27, 0x11, 0xED, 0x7F, 0xDC)

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutView::LayoutView ()
: View (),
  internalCall (false)
{
	layoutItems.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutView::LayoutView (const Rect& size, StyleRef style, Layout* layout)
: View (size, style),
  internalCall (false)
{
	onViewCreated ();
	layoutItems.objectCleanup (true);
	setLayout (layout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutView::~LayoutView ()
{
	// release hidden views
	ArrayForEachFast (layoutItems, LayoutItem, item)
		item->removeObserver (this);
		
		if(item->isHidden ())
			if(View* view = item->getView ())
			{
				ASSERT (view)
				view->release ();
			}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Layout* LayoutView::getLayout ()
{
	return layout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutView::setLayout (Layout* newLayout)
{
	algorithm.release ();

	layout.share (newLayout);
	if(!layout)
		return;

	context = layout->createContext (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutContext* LayoutView::getContext ()
{
	return context;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Point& LayoutView::getPreferredSize ()
{
	return getAlgorithm ()->getPreferredSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutView::onViewCreated ()
{
	initialSize = getSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutItem* LayoutView::findLayoutItem (View* view) const
{
	LayoutItem* item = layoutItems.findIf<LayoutItem> ([view] (const LayoutItem& item)
													   { return item.getView () == view; });
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LayoutView::getLayoutAttributes (IAttributeList& attributes) const
{
	if(!layout)
		return false;
	
	MutableSkinAttributes skinAttributes;
	bool result = layout->getAttributes (skinAttributes);
	attributes.addFrom (skinAttributes.getAttributes ());
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LayoutView::getChildLayoutAttributes (IAttributeList& attributes, IView* view) const
{
	View* v = unknown_cast<View> (view);
	if(v == nullptr)
		return false;
	
	LayoutItem* item = findLayoutItem (v);
	if(item == nullptr)
		return false;
	
	MutableSkinAttributes itemAttributes;
	item->getAttributes (itemAttributes);
	attributes.addFrom (itemAttributes.getAttributes ());
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutView::onSize (const Point& delta)
{
	if(!layout)
		return;

	{
		ScopedVar<bool> guard (internalCall, true);
		getAlgorithm ()->onSize (delta);
	}

	doLayout ();
	checkInvalidate (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutView::onViewsChanged ()
{
	if(parent)
		parent->onChildLimitsChanged (this);

	if(layout)
		doLayout ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutView::onChildSized (View* child, const Point& delta)
{
	if(!layout)
		return;

	if((privateFlags & kExplicitSizeLimits) == 0)
		privateFlags &= ~kSizeLimitsValid;

	if(!internalCall)
	{
		{
			ScopedVar<bool> guard (internalCall, true);
			getAlgorithm ()->onChildSized (child, delta);
		}

		if(parent)
			parent->onChildLimitsChanged (this);

		doLayout ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutView::onChildLimitsChanged (View* child)
{
	if(!layout)
		return;

	getAlgorithm ()->onChildLimitsChanged (child);

	if((privateFlags & kExplicitSizeLimits) == 0)
		privateFlags &= ~kSizeLimitsValid;

	doLayout ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutView::calcAutoSize (Rect& rect)
{
	rect = getAlgorithm ()->getPreferredSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutView::passDownSizeLimits ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LayoutView::addView (View* view)
{
	if((privateFlags & kExplicitSizeLimits) == 0)
		privateFlags &= ~kSizeLimitsValid;

	if(layout)
	{
		auto* item = layout->createItem (view);
		layoutItems.add (item);
		item->addObserver (this);

		ScopedVar<bool> guard (internalCall, true);
		getAlgorithm ()->onItemAdded (item);
	}

	AttachSuspender scope (*this, *view);
	return SuperClass::addView (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LayoutView::insertView (int index, View* view)
{
	if((privateFlags & kExplicitSizeLimits) == 0)
		privateFlags &= ~kSizeLimitsValid;

	if(layout)
	{
		auto* item = layout->createItem (view);

		if(!layoutItems.insertAt (index, item))
			layoutItems.add (item);
		
		item->addObserver (this);
		
		ScopedVar<bool> guard (internalCall, true);
		getAlgorithm ()->onItemInserted (index, item);
	}

	AttachSuspender scope (*this, *view);
	return SuperClass::insertView (index, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LayoutView::removeView (View* view)
{
	if((privateFlags & kExplicitSizeLimits) == 0)
		privateFlags &= ~kSizeLimitsValid;

	if(LayoutItem* item = findLayoutItem (view))
	{
		item->removeObserver (this);
		layoutItems.remove (item);
		ScopedVar<bool> guard (internalCall, true);
		getAlgorithm ()->onItemRemoved (item);
		item->release ();
	}

	return SuperClass::removeView (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LayoutView::isAttached ()
{
	if(isAttachSuspended ())
		return false;

	return SuperClass::isAttached ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutView::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kPropertyChanged && layout)
	{
		LayoutItem* item = unknown_cast<LayoutItem> (subject);
		if(!layoutItems.contains (item))
			return;
		
		getAlgorithm ()->onItemChanged (item);
		doLayout ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectArray& LayoutView::getLayoutItems ()
{
	return layoutItems;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutAlgorithm* LayoutView::getAlgorithm ()
{
	if(algorithm)
		return algorithm;

	if(layout)
	{
		LayoutAlgorithm* newAlgorithm = layout->createAlgorithm (context);
		setAlgorithm (newAlgorithm);
	}
	else
		setAlgorithm (NEW LayoutAlgorithm ());

	return algorithm;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutView::setAlgorithm (LayoutAlgorithm* newAlgorithm)
{
	if(newAlgorithm != algorithm)
	{
		ASSERT (newAlgorithm)
		algorithm.assign (newAlgorithm);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutView::doLayout ()
{
	if(internalCall)
		return;

	ScopedVar<bool> guard (internalCall, true);
	getAlgorithm ()->doLayout ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LayoutView::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == ATTR_LAYOUTCLASS)
	{
		if(layout)
		{
			var = LayoutFactory::instance ().getLayoutName (layout);
			return true;
		}
		return false;
	}
	else if(layout && layout->getProperty (var, propertyId))
		return true;
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LayoutView::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == ATTR_LAYOUTCLASS)
	{
		MutableCString layoutName (var);
		if(!layoutName.isEmpty ())
		{
			AutoPtr<Layout> newLayout (LayoutFactory::instance ().createLayout (layoutName));
			if(newLayout)
				setLayout (newLayout);
		}
		return true;
	}
	else if(layout && layout->setProperty (propertyId, var))
		return true;
	return SuperClass::setProperty (propertyId, var);
}

//************************************************************************************************
// LayoutView::ViewIterator
//************************************************************************************************

LayoutView::ViewIterator::ViewIterator (LayoutView& layoutView)
: ObjectArrayIterator (layoutView.getLayoutItems ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* LayoutView::ViewIterator::getView (Object* item)
{
	return ((LayoutItem*)item)->getView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* LayoutView::ViewIterator::next ()
{
	return getView (ObjectArrayIterator::next ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* LayoutView::ViewIterator::previous ()
{
	return getView (ObjectArrayIterator::previous ());
}

//************************************************************************************************
// LayoutItem
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (LayoutItem, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutItem::LayoutItem ()
: view (nullptr),
  flags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutItem::LayoutItem (View* view)
: view (view),
  initialSize (view->getSize ()),
  flags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& LayoutItem::getInitialSize ()
{
	return initialSize;
}

//************************************************************************************************
// LayoutAlgorithm
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LayoutAlgorithm, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

const Point& LayoutAlgorithm::getPreferredSize ()
{
	return preferredSize;
}

//************************************************************************************************
// LayoutClassEntry
//************************************************************************************************

namespace {

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
} // namespace

//************************************************************************************************
// LayoutFactory
//************************************************************************************************

LayoutFactory& LayoutFactory::instance ()
{
	static LayoutFactory theFactory;
	return theFactory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutFactory::LayoutFactory ()
: EnumTypeInfo ("LayoutClasses")
{
	layoutClasses.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API LayoutFactory::getEnumeratorCount () const
{
	return layoutClasses.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LayoutFactory::getEnumerator (MutableCString& name, Variant& value, int index) const
{
	LayoutClassEntry* entry = (LayoutClassEntry*)layoutClasses.at (index);
	ASSERT (entry != nullptr)
	if(entry == nullptr)
		return false;

	name = entry->layoutName;
	value = index;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutFactory::registerLayout (StringID layoutName, MetaClassRef metaClass)
{
	ForEach (layoutClasses, LayoutClassEntry, entry)
		if(entry->layoutName == layoutName)
		{
			if(metaClass.isClass (entry->metaClass))
				return;

			layoutClasses.remove (entry);
			break;
		}
	EndFor

	layoutClasses.add (NEW LayoutClassEntry (layoutName, metaClass));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Layout* LayoutFactory::createLayout (StringID layoutName)
{
	ForEach (layoutClasses, LayoutClassEntry, entry)
		if(entry->layoutName == layoutName)
		{
			AutoPtr<Object> o (entry->metaClass.createObject ());
			Layout* layout = ccl_cast<Layout> (o);
			if(layout)
			{
				layout->retain ();
				return layout;
			}
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID LayoutFactory::getLayoutName (Layout* layout)
{
	const CCL::MetaClass& metaClass = layout->myClass ();
	ForEach (layoutClasses, LayoutClassEntry, entry)
		if(entry->metaClass == metaClass)
			return entry->layoutName;
	EndFor
	return CString::kEmpty;
}
