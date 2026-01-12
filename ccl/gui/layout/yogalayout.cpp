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
// Filename    : ccl/gui/layout/yogalayout.cpp
// Description : Flexbox layout implementation with Facebook's Yoga library
//
//************************************************************************************************

#include "ccl/gui/layout/flexboxlayout.h"

#include "ccl/public/collections/map.h"
#include "ccl/public/gui/framework/skinxmldefs.h"

#include <yoga/Yoga.h>

namespace CCL {

//************************************************************************************************
// YogaLayoutNode
//************************************************************************************************

class YogaLayoutNode: public FlexItem
{
public:
	DECLARE_CLASS (YogaLayoutNode, FlexItem)
	
	YogaLayoutNode ();
	YogaLayoutNode (View* view);
	~YogaLayoutNode ();
	
	operator YGNodeRef () const;

	void insert (int index, YogaLayoutNode* child);
	void remove (YogaLayoutNode* child);
	int countChildren () const;
	YogaLayoutNode* findChild (View* view) const;
	const YogaLayoutNode& findRoot () const;
	bool isRoot () const;

	void updateLayoutTree () const;
	void calculatePreferredSize (Point& preferredSize) const;
	void setSize (const Rect& size) const;
	void onChildSized (View* childView, const Point& delta);
	
private:
	Vector<YogaLayoutNode*> children;
	YogaLayoutNode* parent;
	YGNodeRef node;

	void applyLayoutRecursively () const;
	
	void resetNodeWidth ();
	void resetNodeHeight ();
};

//************************************************************************************************
// YogaNodeDataAdapter
//************************************************************************************************

class YogaNodeDataAdapter
{
public:
	YogaNodeDataAdapter (YGNodeRef node);
	
	void setContainerData (const FlexData& flexData, const FlexItemData& flexItemData) const;
	void setItemData (const FlexItemData& flexItemData) const;
	void applySizeLimits (const FlexItemData& flexItemData) const;
	
private:
	YGNodeRef node;
	
	void applyNodeWidth (const FlexItemData& flexItemData) const;
	void applyNodeHeight (const FlexItemData& flexItemData) const;
};

//************************************************************************************************
// YogaLayoutContext
//************************************************************************************************

class YogaLayoutContext: public LayoutContext
{
public:
	DECLARE_CLASS (YogaLayoutContext, LayoutContext)
	
	YogaLayoutContext (LayoutView* parentView);
	
	PROPERTY_POINTER (YogaLayoutNode, node, Node)

	View* getView ();
	
private:
	using LayoutContext::LayoutContext;
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
	// Try to retrieve a layout node from the provided view. This is successful, if the view is a
	// layoutView using a YogaLayout engine and used to build the layout tree.
	static YogaLayoutNode* retrieveYogaLayoutNode (View* view);
};

//************************************************************************************************
// YogaLayoutAlgorithm
//************************************************************************************************

class YogaLayoutAlgorithm: public LayoutAlgorithm
{
public:
	YogaLayoutAlgorithm (FlexData& flexData, YogaLayoutContext* context, Layout* layout);
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
	FlexData& flexData;
	YogaLayoutContext* context;
	Layout* layout;
	AutoPtr<YogaLayoutNode> node;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Type Conversions
//////////////////////////////////////////////////////////////////////////////////////////////////

static KeyValue<FlexDirection, YGFlexDirection> flexDirectionMapData[] =
{
	{FlexDirection::kRow, 			YGFlexDirectionRow},
	{FlexDirection::kColumn, 		YGFlexDirectionColumn},
	{FlexDirection::kRowReverse, 	YGFlexDirectionRowReverse},
	{FlexDirection::kColumnReverse, YGFlexDirectionColumnReverse}
};

static KeyValue<FlexWrap, YGWrap> flexWrapMapData[] =
{
	{FlexWrap::kNoWrap, 			YGWrapNoWrap},
	{FlexWrap::kWrap, 				YGWrapWrap},
	{FlexWrap::kWrapReverse, 		YGWrapWrapReverse}
};

static KeyValue<FlexJustify, YGJustify> flexJustifyMapData[] =
{
	{FlexJustify::kFlexStart, 		YGJustifyFlexStart},
	{FlexJustify::kFlexEnd, 		YGJustifyFlexEnd},
	{FlexJustify::kCenter, 			YGJustifyCenter},
	{FlexJustify::kSpaceBetween, 	YGJustifySpaceBetween},
	{FlexJustify::kSpaceAround, 	YGJustifySpaceAround},
	{FlexJustify::kSpaceEvenly, 	YGJustifySpaceEvenly}
};

static KeyValue<FlexAlign, YGAlign> flexAlignMapData[] =
{
	{FlexAlign::kFlexStart, 		YGAlignFlexStart},
	{FlexAlign::kFlexEnd, 			YGAlignFlexEnd},
	{FlexAlign::kCenter, 			YGAlignCenter},
	{FlexAlign::kStretch, 			YGAlignStretch}
};

static KeyValue<FlexAlignSelf, YGAlign> flexAlignSelfMapData[] =
{
	{FlexAlignSelf::kAuto, 			YGAlignAuto},
	{FlexAlignSelf::kFlexStart, 	YGAlignFlexStart},
	{FlexAlignSelf::kFlexEnd, 		YGAlignFlexEnd},
	{FlexAlignSelf::kCenter, 		YGAlignCenter},
	{FlexAlignSelf::kStretch, 		YGAlignStretch}
};

static KeyValue<FlexPositionType, YGPositionType> flexPositionTypeMapData[] =
{
	{FlexPositionType::kRelative,	YGPositionTypeRelative},
	{FlexPositionType::kAbsolute,	YGPositionTypeAbsolute}
};

static ConstMap<FlexDirection, YGFlexDirection> flexDirectionMap (flexDirectionMapData, ARRAY_COUNT (flexDirectionMapData));
static ConstMap<FlexWrap, YGWrap> flexWrapMap (flexWrapMapData, ARRAY_COUNT (flexWrapMapData));
static ConstMap<FlexJustify, YGJustify> flexJustifyMap (flexJustifyMapData, ARRAY_COUNT (flexJustifyMapData));
static ConstMap<FlexAlign, YGAlign> flexAlignMap (flexAlignMapData, ARRAY_COUNT (flexAlignMapData));
static ConstMap<FlexAlignSelf, YGAlign> flexAlignSelfMap (flexAlignSelfMapData, ARRAY_COUNT (flexAlignSelfMapData));
static ConstMap<FlexPositionType, YGPositionType> flexPositionTypeMap (flexPositionTypeMapData, ARRAY_COUNT (flexPositionTypeMapData));

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
// YogaLayoutNode
//************************************************************************************************

DEFINE_CLASS (YogaLayoutNode, FlexItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaLayoutNode::YogaLayoutNode ()
: FlexItem (),
  node (YGNodeNew ()),
  parent (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaLayoutNode::YogaLayoutNode (View* view)
: FlexItem (view),
  node (YGNodeNew ()),
  parent (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaLayoutNode::~YogaLayoutNode ()
{
	YGNodeFree (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaLayoutNode::operator YGNodeRef () const
{
	return node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutNode::insert (int index, YogaLayoutNode* child)
{
	ASSERT (child != nullptr)
	if(child == nullptr)
		return;
	
	size_t childCount = YGNodeGetChildCount (node);
	bool indexIsValid = index >= 0 && index <= int(childCount);
	
	ASSERT (indexIsValid)
	if(!indexIsValid)
		return;

	YGNodeInsertChild (node, child->node, index);
	child->parent = this;
	children.insertAt (index, child);
	
	YGNodeSetHasNewLayout (node, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutNode::remove (YogaLayoutNode* child)
{
	children.removeIf ([this, child] (YogaLayoutNode* _child)
	{
		if(_child != child)
			return false;
		
		YGNodeRemoveChild (node, child->node);
		child->parent = nullptr;
		
		YGNodeSetHasNewLayout (node, true);
		return true;
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int YogaLayoutNode::countChildren () const
{
	return children.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaLayoutNode* YogaLayoutNode::findChild (View* view) const
{
	if(YogaLayoutNode** child = children.findIf ([view] (YogaLayoutNode* child) {return child->view == view; }))
		return *child;
	
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const YogaLayoutNode& YogaLayoutNode::findRoot () const
{
	if(isRoot ())
		return *this;
	
	return parent->findRoot ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool YogaLayoutNode::isRoot () const
{
	return parent == nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutNode::updateLayoutTree () const
{
	const YogaLayoutNode& root = findRoot ();
	YGNodeCalculateLayout (root, YGUndefined, YGUndefined, YGDirectionLTR);
	root.applyLayoutRecursively ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutNode::calculatePreferredSize (Point& preferredSize) const
{
	const YogaLayoutNode& root = findRoot ();
	YGNodeCalculateLayout (root.node, YGUndefined, YGUndefined, YGDirectionLTR);
	
	preferredSize.x = YGNodeLayoutGetWidth (node);
	preferredSize.y = YGNodeLayoutGetHeight (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutNode::setSize (const Rect& size) const
{
	Coord width = size.getWidth ();
	if(width <= 0)
		YGNodeStyleSetWidthAuto (node);
	else
		YGNodeStyleSetWidth (node, float(width));
	
	Coord height = size.getHeight ();
	if(height <= 0)
		YGNodeStyleSetHeightAuto (node);
	else
		YGNodeStyleSetHeight (node, float(height));
	
	YGNodeSetHasNewLayout (node, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutNode::onChildSized (View* childView, const Point& delta)
{
	if(YogaLayoutNode* child = findChild (childView))
	{
		YGValue width = YGNodeStyleGetWidth (*child);
		if(width.unit != YGUnitAuto)
			YGNodeStyleSetWidth (*child, childView->getWidth ());
		
		YGValue height = YGNodeStyleGetHeight (*child);
		if(height.unit != YGUnitAuto)
			YGNodeStyleSetHeight (*child, childView->getHeight ());
		
		YGNodeSetHasNewLayout (*child, true);
		
		resetNodeWidth ();
		resetNodeHeight ();
		
		YGNodeSetHasNewLayout (node, true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutNode::applyLayoutRecursively () const
{
	if(!YGNodeGetHasNewLayout (node))
	  return;
	
	YGNodeSetHasNewLayout (node, false);
	
	// Use left, top, width and height
	// Yoga's YGNodeLayoutGet<Left, Right...> are distances to the corresponding parent edge
	
	Coord left = Coord(YGNodeLayoutGetLeft (node));
	Coord top = Coord(YGNodeLayoutGetTop (node));
	Coord width = Coord(YGNodeLayoutGetWidth (node));
	Coord height = Coord(YGNodeLayoutGetHeight (node));
	
	Rect itemSize (left, top, left + width, top + height);
	
	if(view != nullptr && !isRoot ())
		view->setSize (itemSize);
	
	for(auto& child : children)
		child->applyLayoutRecursively ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutNode::resetNodeWidth ()
{
	bool isAttachedToExternalLayout = getView ()->isAttached () && isRoot ();
	bool hfit = getView ()->getSizeMode () & IView::kHFitSize;

	if((!isAttachedToExternalLayout || hfit) && flexItemData.width.isAuto ())
		YGNodeStyleSetWidthAuto (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutNode::resetNodeHeight ()
{
	bool isAttachedToExternalLayout = getView ()->isAttached () && isRoot ();
	bool vfit = getView ()->getSizeMode () & IView::kVFitSize;
	
	if((!isAttachedToExternalLayout || vfit) && flexItemData.height.isAuto ())
		YGNodeStyleSetHeightAuto (node);
}

//************************************************************************************************
// YogaNodeDataAdapter
//************************************************************************************************

YogaNodeDataAdapter::YogaNodeDataAdapter (YGNodeRef node)
: node (node)
{}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaNodeDataAdapter::setContainerData (const FlexData& flexData, const FlexItemData& flexItemData) const
{
	YGNodeStyleSetFlexDirection (node, flexDirectionMap [flexData.direction]);
	YGNodeStyleSetFlexWrap (node, flexWrapMap [flexData.wrap]);
	YGNodeStyleSetJustifyContent (node, flexJustifyMap [flexData.justify]);
	YGNodeStyleSetAlignItems (node, flexAlignMap [flexData.align]);
	
	// Padding
	auto setYGNodeStylePadding = [this] (const DesignCoord& flexCoord, YGEdge edge)
	{
		if(flexCoord.isCoord ())
			YGNodeStyleSetPadding (node, edge, float(flexCoord.value));
	};
	
	setYGNodeStylePadding (flexData.padding.left, YGEdgeLeft);
	setYGNodeStylePadding (flexData.padding.top, YGEdgeTop);
	setYGNodeStylePadding (flexData.padding.right, YGEdgeRight);
	setYGNodeStylePadding (flexData.padding.bottom, YGEdgeBottom);
	
	// Gap
	auto setYGNodeStyleGap = [this] (const DesignCoord& flexCoord, YGGutter gutter)
	{
		if(flexCoord.isCoord ())
			YGNodeStyleSetGap (node, gutter, float(flexCoord.value));
	};
	
	setYGNodeStyleGap (flexData.gap.row, YGGutterRow);
	setYGNodeStyleGap (flexData.gap.column, YGGutterColumn);
	
	applyNodeWidth (flexItemData);
	applyNodeHeight (flexItemData);
	
	YGNodeSetHasNewLayout (node, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaNodeDataAdapter::setItemData (const FlexItemData& flexItemData) const
{
	YGNodeStyleSetFlexGrow (node, flexItemData.grow);
	YGNodeStyleSetFlexShrink (node, flexItemData.shrink);
	
	YGNodeStyleSetAlignSelf (node, flexAlignSelfMap [flexItemData.alignSelf]);
	YGNodeStyleSetPositionType (node, flexPositionTypeMap [flexItemData.positionType]);
	
	if(flexItemData.flexBasis.isAuto ())
		YGNodeStyleSetFlexBasisAuto (node);
	else if(flexItemData.flexBasis.isPercent ())
		YGNodeStyleSetFlexBasisPercent (node, float(flexItemData.flexBasis.value));
	else if(flexItemData.flexBasis.isCoord ())
		YGNodeStyleSetFlexBasis (node, float(flexItemData.flexBasis.value));
	
	auto setYGNodeStyleMargin = [this] (const DesignCoord& flexCoord, YGEdge edge)
	{
		if(flexCoord.isAuto ())
			YGNodeStyleSetMarginAuto (node, edge);
		else if(flexCoord.isCoord ())
			YGNodeStyleSetMargin (node, edge, float(flexCoord.value));
	};
	
	setYGNodeStyleMargin (flexItemData.margin.top, YGEdgeTop);
	setYGNodeStyleMargin (flexItemData.margin.right, YGEdgeRight);
	setYGNodeStyleMargin (flexItemData.margin.bottom, YGEdgeBottom);
	setYGNodeStyleMargin (flexItemData.margin.left, YGEdgeLeft);
	
	auto setYGNodeStyleInset = [this] (const DesignCoord& flexCoord, YGEdge edge)
	{
		if(flexCoord.isCoord ())
			YGNodeStyleSetPosition (node, edge, float(flexCoord.value));
	};
	
	setYGNodeStyleInset (flexItemData.inset.top, YGEdgeTop);
	setYGNodeStyleInset (flexItemData.inset.right, YGEdgeRight);
	setYGNodeStyleInset (flexItemData.inset.bottom, YGEdgeBottom);
	setYGNodeStyleInset (flexItemData.inset.left, YGEdgeLeft);
	
	applySizeLimits (flexItemData);
	applyNodeWidth (flexItemData);
	applyNodeHeight (flexItemData);
	
	YGNodeSetHasNewLayout (node, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaNodeDataAdapter::applySizeLimits (const FlexItemData& flexItemData) const
{
	if(flexItemData.minWidth.isCoord ())
		YGNodeStyleSetMinWidth (node, flexItemData.minWidth.value);
	if(flexItemData.minHeight.isCoord ())
		YGNodeStyleSetMinHeight (node, flexItemData.minHeight.value);
	if(flexItemData.maxWidth.isCoord ())
		YGNodeStyleSetMaxWidth (node, flexItemData.maxWidth.value);
	if(flexItemData.maxWidth.isCoord ())
		YGNodeStyleSetMaxHeight (node, flexItemData.maxHeight.value);
		
	YGNodeSetHasNewLayout (node, true);
}
//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaNodeDataAdapter::applyNodeWidth (const FlexItemData& flexItemData) const
{
	if(flexItemData.width.isAuto ())
		YGNodeStyleSetWidthAuto (node);
	else if (flexItemData.width.isPercent ())
		YGNodeStyleSetWidthPercent(node, flexItemData.width.value);
	else
		YGNodeStyleSetWidth (node, flexItemData.width.value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaNodeDataAdapter::applyNodeHeight (const FlexItemData& flexItemData) const
{
	if(flexItemData.height.isAuto ())
		YGNodeStyleSetHeightAuto (node);
	else if (flexItemData.height.isPercent ())
		YGNodeStyleSetHeightPercent(node, flexItemData.height.value);
	else
		YGNodeStyleSetHeight (node, flexItemData.height.value);
}


//************************************************************************************************
// YogaLayoutContext
//************************************************************************************************

DEFINE_CLASS (YogaLayoutContext, LayoutContext)

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaLayoutContext::YogaLayoutContext (LayoutView* parentView)
: LayoutContext (parentView),
  node (nullptr)
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
		return NEW YogaLayoutNode;
	
	YogaLayoutNode* childNode = retrieveYogaLayoutNode (view);
	if(childNode == nullptr)
		return NEW YogaLayoutNode (view);

	// The node is shared between a YogaLayoutAlgorithm instance and the managing LayoutView.
	// In order to avoid changes to the existing layout architecture, we manually share the
	// object by increasing the reference count.

	childNode->retain ();
	return childNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaLayoutNode* YogaLayout::retrieveYogaLayoutNode (View* view)
{
	auto* layoutView = ccl_cast<LayoutView> (view);
	if(layoutView == nullptr)
		return nullptr;
	
	auto* childContext = ccl_cast<YogaLayoutContext> (layoutView->getContext ());
	if(childContext == nullptr)
		return nullptr;
	
	return childContext->getNode ();
}

//************************************************************************************************
// YogaLayoutAlgorithm
//************************************************************************************************

YogaLayoutAlgorithm::YogaLayoutAlgorithm (FlexData& flexData, YogaLayoutContext* context, Layout* layout)
: flexData (flexData),
  context (context),
  layout (layout),
  node (NEW YogaLayoutNode (context->getView ()))
{
	context->setNode (node);
	YogaNodeDataAdapter (*node).setContainerData (flexData, node->getFlexItemData ());
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
	node->calculatePreferredSize (preferredSize);
	return LayoutAlgorithm::getPreferredSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::doLayout ()
{
	node->updateLayoutTree ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::onSize (const Point& delta)
{
	// Only the root node can be sized from outside the yoga layout tree (e.g. by other layout systems)
	if(node->isRoot ())
		node->setSize (context->getLayoutRect ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::onChildSized (View* childView, const Point& delta)
{
	node->onChildSized (childView, delta);
	onChildLimitsChanged (childView);
	context->requestAutoSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::onChildLimitsChanged (View* childView)
{
	if(YogaLayoutNode* child = node->findChild (childView))
	{
		child->updateSizeLimits ();
		const FlexItemData& flexItemData = child->getFlexItemData ();
		YogaNodeDataAdapter (*child).applySizeLimits (flexItemData);
		node->updateLayoutTree ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::onItemAdded (LayoutItem* item)
{
	onItemInserted (node->countChildren (), item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::onItemInserted (int index, LayoutItem* item)
{
	auto* child = ccl_cast<YogaLayoutNode> (item);
	if(child != nullptr)
	{
		const FlexItemData& flexItemData = child->getFlexItemData ();
		YogaNodeDataAdapter (*child).setItemData (flexItemData);
		node->insert (index, child);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::onItemRemoved (LayoutItem* item)
{
	auto* yogaNode = ccl_cast<YogaLayoutNode> (item);
	if(yogaNode != nullptr)
		node->remove (yogaNode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaLayoutAlgorithm::onItemChanged (LayoutItem* item)
{
	if(auto* child = ccl_cast<YogaLayoutNode> (item))
	{
		const FlexItemData& flexItemData = child->getFlexItemData ();
		YogaNodeDataAdapter (*child).setItemData (flexItemData);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API YogaLayoutAlgorithm::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kPropertyChanged)
		YogaNodeDataAdapter (*node).setContainerData (flexData, node->getFlexItemData ());
}
