//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/gui/layout/yogalayout.cpp
// Description : Flexbox layout implementation with Facebook's Yoga library
//
//************************************************************************************************

#include "ccl/gui/layout/flexboxlayout.h"
#include "ccl/gui/layout/yogashared.h"

#include "ccl/public/gui/framework/skinxmldefs.h"

namespace CCL {

//************************************************************************************************
// YogaLayoutItem
//************************************************************************************************

class YogaLayoutItem: public FlexLayoutItem,
					  public YogaNode
{
public:
	DECLARE_CLASS (YogaLayoutItem, FlexLayoutItem)
	
	YogaLayoutItem ();
	YogaLayoutItem (View* view);
	~YogaLayoutItem ();
	
	void insert (int index, YogaLayoutItem* child);
	void remove (YogaLayoutItem* child);
	int countChildren () const;
	YogaLayoutItem* findChild (View* view) const;
	YogaLayoutItem& findRoot () const;
	bool isRoot () const;

	void updateLayoutTree ();
	void calculatePreferredSize (Point& preferredSize);
	void setSize (const Rect& size);
	void onChildSized (View* childView, const Point& delta);
	
private:
	Vector<YogaLayoutItem*> children;
	YogaLayoutItem* parent;

	void applyLayoutRecursively ();
};

//************************************************************************************************
// YogaLayoutContext
//************************************************************************************************

class YogaLayoutContext: public LayoutContext
{
public:
	DECLARE_CLASS_ABSTRACT (YogaLayoutContext, LayoutContext)
	
	YogaLayoutContext (LayoutView* parentView);
	
	PROPERTY_POINTER (YogaLayoutItem, yogaItem, YogaItem)

	View* getView ();
};

//************************************************************************************************
// YogaLayout
//************************************************************************************************

class YogaLayout: public FlexboxLayout
{
public:
	DECLARE_CLASS (YogaLayout, FlexboxLayout)
	
	// FlexboxLayout
	LayoutAlgorithm* createAlgorithm (LayoutContext* context) override;
	LayoutContext* createContext (LayoutView* parent) override;
	LayoutItem* createItem (View* view = nullptr) override;
	
private:
	// Try to retrieve a layout item from the provided view. This is successful, if the view is a
	// layoutView using a YogaLayout engine and used to build the layout tree.
	static YogaLayoutItem* retrieveYogaLayoutItem (View* view);
};

//************************************************************************************************
// YogaLayoutAlgorithm
//************************************************************************************************

class YogaLayoutAlgorithm: public LayoutAlgorithm
{
public:
	YogaLayoutAlgorithm (FlexContainerData& flexData, YogaLayoutContext* context, Layout* layout);
	~YogaLayoutAlgorithm ();
	
	// LayoutAlgorithm
	const Point& getPreferredSize () override;
	void doLayout () override;
	void onSize (const Point& delta) override;
	void onChildSized (View* childView, const Point& delta) override;
	void onChildLimitsChanged (View* childView) override;
	void onItemAdded (LayoutItem* item) override;
	void onItemInserted (int index, LayoutItem* item) override;
	void onItemRemoved (LayoutItem* item) override;
	void onItemChanged (LayoutItem* item) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	
protected:
	FlexContainerData& flexData;
	YogaLayoutContext* context;
	Layout* layout;
	AutoPtr<YogaLayoutItem> yogaItem;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Layout Registration
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (FlexboxLayout, kFrameworkLevelFirst)
{
	LayoutFactory::instance ().registerLayout (LAYOUTCLASS_FLEXBOX, ccl_typeid<YogaLayout> ());
	return true;
}

//************************************************************************************************
// YogaLayoutItem
//************************************************************************************************

DEFINE_CLASS (YogaLayoutItem, FlexLayoutItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaLayoutItem::YogaLayoutItem ()
: parent (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaLayoutItem::YogaLayoutItem (View* view)
: FlexLayoutItem (view),
  parent (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaLayoutItem::~YogaLayoutItem ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutItem::insert (int index, YogaLayoutItem* child)
{
	if(!YogaNode::insertNode (index, child))
		return;

	child->parent = this;
	children.insertAt (index, child);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutItem::remove (YogaLayoutItem* child)
{
	if(!YogaNode::removeNode (child))
		return;

	children.remove (child);
	child->parent = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int YogaLayoutItem::countChildren () const
{
	return children.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaLayoutItem* YogaLayoutItem::findChild (View* view) const
{
	if(YogaLayoutItem** child = children.findIf ([view] (YogaLayoutItem* child) { return child->view == view; }))
		return *child;
	
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaLayoutItem& YogaLayoutItem::findRoot () const
{
	if(isRoot ())
		return const_cast<YogaLayoutItem&> (*this);
	
	return parent->findRoot ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool YogaLayoutItem::isRoot () const
{
	return parent == nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutItem::updateLayoutTree ()
{
	YogaLayoutItem& root = findRoot ();
	root.calculateLayout ();
	root.applyLayoutRecursively ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutItem::calculatePreferredSize (Point& preferredSize)
{
	YogaLayoutItem& root = findRoot ();
	root.calculateLayout ();

	preferredSize.x = Coord(YogaNode::getLayoutWidth ());
	preferredSize.y = Coord(YogaNode::getLayoutHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutItem::setSize (const Rect& size)
{
	CoordF width = CoordF(size.getWidth ());
	CoordF height = CoordF(size.getHeight ());

	YogaNode::applySize (width, height);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutItem::onChildSized (View* childView, const Point& delta)
{
	if(YogaLayoutItem* child = findChild (childView))
	{
		YGValue width = YGNodeStyleGetWidth (*child);
		if(width.unit != YGUnitAuto)
			YGNodeStyleSetWidth (*child, childView->getWidth ());
		
		YGValue height = YGNodeStyleGetHeight (*child);
		if(height.unit != YGUnitAuto)
			YGNodeStyleSetHeight (*child, childView->getHeight ());
		
		child->setHasNewLayout (true);
		
		// reset node width/height
		bool isAttachedToExternalLayout = getView ()->isAttached () && isRoot ();
		bool hfit = getView ()->getSizeMode () & IView::kHFitSize;
		bool vfit = getView ()->getSizeMode () & IView::kVFitSize;

		if((!isAttachedToExternalLayout || hfit) && flexItemData.width.isAuto ())
			YGNodeStyleSetWidthAuto (node);
	
		if((!isAttachedToExternalLayout || vfit) && flexItemData.height.isAuto ())
			YGNodeStyleSetHeightAuto (node);

		YogaNode::setHasNewLayout (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutItem::applyLayoutRecursively ()
{
	if(!YogaNode::hasNewLayout ())
		return;

	YogaNode::setHasNewLayout (false);

	if(view != nullptr && !isRoot ())
	{
		RectF itemSize;
		YogaNode::getLayoutSize (itemSize);
		view->setSize (rectFToInt (itemSize));
	}
	
	for(auto& child : children)
		child->applyLayoutRecursively ();
}

//************************************************************************************************
// YogaLayoutContext
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (YogaLayoutContext, LayoutContext)

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaLayoutContext::YogaLayoutContext (LayoutView* parentView)
: LayoutContext (parentView),
  yogaItem (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* YogaLayoutContext::getView ()
{
	return parentView;
}

//************************************************************************************************
// YogaLayout
//************************************************************************************************

DEFINE_CLASS (YogaLayout, FlexboxLayout)

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutAlgorithm* YogaLayout::createAlgorithm (LayoutContext* context)
{
	auto* yogaLayoutContext = ccl_cast<YogaLayoutContext> (context);
	return NEW YogaLayoutAlgorithm (flexData, yogaLayoutContext, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutContext* YogaLayout::createContext (LayoutView* parent)
{
	return NEW YogaLayoutContext (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutItem* YogaLayout::createItem (View* view)
{
	if(view == nullptr)
		return NEW YogaLayoutItem;
	
	YogaLayoutItem* childItem = retrieveYogaLayoutItem (view);
	if(childItem == nullptr)
		return NEW YogaLayoutItem (view);

	// The item is shared between a YogaLayoutAlgorithm instance and the managing LayoutView.
	// In order to avoid changes to the existing layout architecture, we manually share the
	// object by increasing the reference count.

	childItem->retain ();
	return childItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaLayoutItem* YogaLayout::retrieveYogaLayoutItem (View* view)
{
	auto* layoutView = ccl_cast<LayoutView> (view);
	if(layoutView == nullptr)
		return nullptr;
	
	auto* childContext = ccl_cast<YogaLayoutContext> (layoutView->getContext ());
	if(childContext == nullptr)
		return nullptr;
	
	return childContext->getYogaItem ();
}

//************************************************************************************************
// YogaLayoutAlgorithm
//************************************************************************************************

YogaLayoutAlgorithm::YogaLayoutAlgorithm (FlexContainerData& flexData, YogaLayoutContext* context, Layout* layout)
: flexData (flexData),
  context (context),
  layout (layout),
  yogaItem (NEW YogaLayoutItem (context->getView ()))
{
	context->setYogaItem (yogaItem);
	yogaItem->applyContainerData (flexData, yogaItem->getFlexItemData ());
	layout->addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaLayoutAlgorithm::~YogaLayoutAlgorithm ()
{
	layout->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Point& YogaLayoutAlgorithm::getPreferredSize ()
{
	yogaItem->calculatePreferredSize (preferredSize);
	return LayoutAlgorithm::getPreferredSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::doLayout ()
{
	yogaItem->updateLayoutTree ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::onSize (const Point& delta)
{
	// Only the root item can be sized from outside the yoga layout tree (e.g. by other layout systems)
	if(yogaItem->isRoot ())
		yogaItem->setSize (context->getLayoutRect ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::onChildSized (View* childView, const Point& delta)
{
	yogaItem->onChildSized (childView, delta);
	onChildLimitsChanged (childView);
	context->requestAutoSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::onChildLimitsChanged (View* childView)
{
	if(YogaLayoutItem* child = yogaItem->findChild (childView))
	{
		child->updateSizeLimits ();
		const FlexItemData& flexItemData = child->getFlexItemData ();
		child->applySizeLimits (flexItemData);
		yogaItem->updateLayoutTree ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::onItemAdded (LayoutItem* item)
{
	onItemInserted (yogaItem->countChildren (), item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::onItemInserted (int index, LayoutItem* item)
{
	auto* child = ccl_cast<YogaLayoutItem> (item);
	if(child != nullptr)
	{
		const FlexItemData& flexItemData = child->getFlexItemData ();
		child->applyItemData (flexItemData);
		yogaItem->insert (index, child);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::onItemRemoved (LayoutItem* item)
{
	auto* child = ccl_cast<YogaLayoutItem> (item);
	if(child != nullptr)
		yogaItem->remove (child);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::onItemChanged (LayoutItem* item)
{
	if(auto* child = ccl_cast<YogaLayoutItem> (item))
	{
		const FlexItemData& flexItemData = child->getFlexItemData ();
		child->applyItemData (flexItemData);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API YogaLayoutAlgorithm::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kPropertyChanged)
		yogaItem->applyContainerData (flexData, yogaItem->getFlexItemData ());
}
