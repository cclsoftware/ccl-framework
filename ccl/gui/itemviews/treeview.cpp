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
// Filename    : ccl/gui/itemviews/treeview.cpp
// Description : Tree View
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/itemviews/treeview.h"
#include "ccl/gui/itemviews/treeitem.h"
#include "ccl/gui/itemviews/headerview.h"
#include "ccl/gui/itemviews/itemviewaccessibility.h"

#include "ccl/gui/windows/window.h"
#include "ccl/gui/controls/scrollbar.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/imaging/image.h"
#include "ccl/gui/system/dragndrop.h"

#include "ccl/base/message.h"

#include "ccl/public/base/irecognizer.h"
#include "ccl/public/math/mathprimitives.h"

namespace CCL {

enum { kAutoExpandDelay = 500 }; ///< delay before automatically expanding the focus item for style kTreeViewExpandMouseItem

//************************************************************************************************
// Tree Traverser
//************************************************************************************************

struct TreeGetTotalSize: TreeVisibleTraverser
{
	int maxTitleWidth;
	int maxIndex;
	int maxInset;
	Coord totalHeight;
	Font font;
	TreeView& treeView;

	TreeGetTotalSize (TreeView& treeView)
	: TreeVisibleTraverser (treeView.getTree ()),
	  treeView (treeView),
	  maxTitleWidth (0),
	  maxIndex (0),
	  maxInset (0),
	  totalHeight (0)
	{
		treeView.getFont (font);

		StyleRef style = treeView.getStyle ();
		if(style.isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
			mode |= kHiddenRoot;
		if(style.isCustomStyle (Styles::kTreeViewAppearanceNoIcons))
			mode |= kNoIcons;	
	}

	bool visit (TreeItem* item) override
	{
		if(!checkVisible (item))
			return true;
		if(currentIndex > maxIndex)
			maxIndex = currentIndex;
		if(currentInset > maxInset)
			maxInset = currentInset;

		// determine (custom) row height for this item, if not already cached
		Coord h = item->getHeight ();
		if(h < 0)
			item->setHeight (h = treeView.determineRowHeight (ItemIndex (item)));

		totalHeight += h;
				
		int w = treeView.getStringWidth (item);
		
		if(!(mode & kNoIcons))
		{
			Point iconSize;
			treeView.determineIconSize (iconSize, currentIndex);
			w += iconSize.x + treeView.getTreeStyle ().getMarginH ();
		}
		
		if(w > maxTitleWidth)
			maxTitleWidth = w;
		return true;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct TreeResetItemSizes: TreeTraverser
{
	TreeResetItemSizes ()
	: TreeTraverser (0) {}

	bool visit (TreeItem* item) override
	{
		item->setTextWidth (-1);
		item->setHeight (-1);
		return true;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct TreeResetItemSizesAndTextLayout: TreeResetItemSizes
{
	bool visit (TreeItem* item) override
	{
		item->setTextLayout (nullptr);
		return TreeResetItemSizes::visit (item);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct TreeGetItemRect: TreeVisibleTraverser
{
	TreeItem* searchItem;
	const TreeView& treeView;
	Rect& rect;
	Coord top;

	TreeGetItemRect (Rect& rect, TreeItem* searchItem, const TreeView& treeView)
	: TreeVisibleTraverser (treeView.getTree ()),
	  treeView (treeView),
	  searchItem (searchItem),
	  rect (rect),
	  top (0)
	{
		if(treeView.getStyle ().isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
			mode |= kHiddenRoot;
	}

	bool visit (TreeItem* item) override
	{
		if(!checkVisible (item))
			return true;

		if(item == searchItem)
		{
			TreeStyle& treeStyle = treeView.getTreeStyle ();

			rect.left   = treeStyle.getMarginH ();
			rect.top    = top + treeStyle.getMarginV ();
			rect.right  = treeView.getWidth ();
			rect.bottom = rect.top + item->getHeight ();
			return false;
		}

		top += item->getHeight ();
		return true;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct TreeFindItemAtCoord: TreeVisibleTraverser
{
	Coord y;
	Coord currentPos;
	TreeItem* foundItem;
	int foundIndex;

	TreeFindItemAtCoord (Coord y, const TreeView& treeView)
	: TreeVisibleTraverser (treeView.getTree ()),
	  y (y),
	  currentPos (treeView.getTreeStyle ().getMarginV ()),
	  foundItem (nullptr),
	  foundIndex (-1)
	{
		if(treeView.getStyle ().isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
			mode |= kHiddenRoot;
	}

	bool visit (TreeItem* item) override
	{
		if(!checkVisible (item))
			return true;

		currentPos += item->getHeight ();
		if(y < currentPos)
		{
			foundItem = item;
			foundIndex = currentIndex;
			return false;
		}
		return true;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct TreeFindItems: TreeVisibleTraverser
{
	IItemSelection& selection;
	int fromIndex;
	int toIndex;
	int numFound;

	TreeFindItems (IItemSelection& selection, int fromIndex, int toIndex, Tree& tree)
	: TreeVisibleTraverser (tree),
	  selection (selection),
	  fromIndex (fromIndex),
	  toIndex (toIndex),
	  numFound (0)
	{}

	bool visit (TreeItem* item) override
	{
		if(!checkVisible (item))
			return true;
		if(currentIndex < fromIndex)
			return true;
		if(currentIndex > toIndex)
			return false;

		selection.select (item->asIndex ());
		numFound++;
		return true;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct TreeSelectRange: TreeVisibleTraverser
{
	TreeView* treeView;
	TreeItem* item1;
	TreeItem* item2;
	TreeItem* finalItem;

	TreeSelectRange (TreeView* treeView, TreeItem* item1, TreeItem* item2)
	: TreeVisibleTraverser (treeView->getTree ()),
	  treeView (treeView),
	  item1 (item1),
	  item2 (item2),
	  finalItem (nullptr)
	{}

	bool visit (TreeItem* item) override
	{
		if(!checkVisible (item) || !tree.getModel ()->canSelectItem (item))
			return true;
		if(!finalItem)
		{
			if(item == item1)
				finalItem = item2;
			else if(item == item2)
				finalItem = item1;
			else 
				return true;
		}

		if(!treeView->getSelection ().isSelected (item))
			treeView->selectItem (item, true);

		return item != finalItem;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct TreeViewSelectAll: TreeVisibleTraverser
{
	IItemSelection& selection;

	TreeViewSelectAll (IItemSelection& selection, Tree& tree)
	: TreeVisibleTraverser (tree),
	  selection (selection)
	{}

	bool visit (TreeItem* item) override
	{
		if(!checkVisible (item) || !tree.getModel ()->canSelectItem (item))
			return true;
		item->isSelected (true);
		selection.select (item->asIndex ());
		return true;
	}
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// TreeStyle
/** Style attributes for a TreeView. */
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TreeStyle, ItemStyle)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_VISUALSTYLE_CLASS (TreeStyle, ItemStyle, "TreeViewStyle")
	ADD_VISUALSTYLE_METRIC ("itemInset")	///< inset (in pixels) per tree depth level
	ADD_VISUALSTYLE_METRIC ("iconSize")		///< size of item icons (same width & height)
	ADD_VISUALSTYLE_METRIC ("expandSize")	///< size of expand symbol (same width & height)
	ADD_VISUALSTYLE_METRIC ("leafInset")	///< additional inset for leaf items
END_VISUALSTYLE_CLASS (TreeStyle)

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeStyle::TreeStyle ()
: itemInset (16),
  leafInset (9),
  iconSize (16, 16),
  expandSize (9, 9),
  selectOverlayBrush (Color (0,0,0,0))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeStyle::TreeStyle (const TreeStyle& t)
: ItemStyle (t),
  itemInset (t.itemInset),
  leafInset (t.leafInset),
  iconSize (t.iconSize),
  expandSize (t.expandSize),
  selectOverlayBrush (t.selectOverlayBrush)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeStyle::updateStyle (const VisualStyle& style)
{
	SuperClass::updateStyle (style);

	itemInset = style.getMetric<int> ("itemInset", itemInset);
	iconSize.x = style.getMetric<int> ("iconSize", iconSize.x);
	expandSize.x = style.getMetric<int> ("expandSize", expandSize.x);
	leafInset = style.getMetric<int> ("leafInset", expandSize.x); // same as expandSize by default
	setSelectOverlayBrush (style.getColor ("selectionOverlayColor", selectOverlayBrush.getColor ()));
	
	iconSize.y = iconSize.x;
	expandSize.y = expandSize.x;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeStyle::zoom (const ItemStyle& _original, float zoomFactor)
{
/*	TODO obsolete ???
	Transform t;
	Rect r;
	t.transform (r.getLeftBottom ());
*/
	SuperClass::zoom (_original, zoomFactor);

	const TreeStyle* original = ccl_cast<TreeStyle> (&_original);
	ASSERT (original != nullptr)
	if(!original)
		return;

	setItemInset  (int (zoomFactor * original->getItemInset ()));
	setLeafInset  (int (zoomFactor * original->getLeafInset ()));
	setIconSize   (original->getIconSize   () * zoomFactor);
	setExpandSize (original->getExpandSize () * zoomFactor);
}

//************************************************************************************************
// TreeControl
//************************************************************************************************

DEFINE_CLASS (TreeControl, ItemControl)
DEFINE_CLASS_UID (TreeControl, 0x1fe985df, 0x4858, 0x4ac6, 0xad, 0x33, 0xdb, 0x7f, 0x3a, 0x88, 0xf0, 0x48)

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeControl::TreeControl (const Rect& size, IItemModel* model, StyleRef treeViewStyle, StyleRef scrollViewStyle)
: ItemControl (size,
			   NEW TreeView (Rect (), model, treeViewStyle),
			   scrollViewStyle)
{}

//************************************************************************************************
// TreeView
//************************************************************************************************

BEGIN_STYLEDEF (TreeView::customStyles)
	{"noroot",		Styles::kTreeViewAppearanceNoRoot},
	{"noicons",		Styles::kTreeViewAppearanceNoIcons},
	{"autoexpand",	Styles::kTreeViewBehaviorAutoExpand},
	{"expandall",	Styles::kTreeViewBehaviorExpandAll},
	{"expandmouse",	Styles::kTreeViewBehaviorExpandMouseItem},
	{"expanddrag",	Styles::kTreeViewBehaviorExpandDragItem},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_STRINGID_MEMBER_ (TreeView, kUpdateSize, "updateSize") 

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (TreeView, ItemView)

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeView::TreeView (const Rect& size, IItemModel* model, StyleRef style)
: ItemView (size, style),
  tree (NEW Tree),  
  focusItem (nullptr),
  anchorItem (nullptr),
  dragOverItem (nullptr),
  extraHeight (0),
  avoidScrolling (false),
  ownTree (true),
  editColumn (0)
{
	setItemStyle (NEW TreeStyle);
	setModel (model);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeView::~TreeView ()
{
	setModel (nullptr); // detach while vtable intact
	setItemFilter (nullptr);
	cancelSignals ();

	if(tree)
		tree->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TreeView::setTree (ITree* newTree)
{
	if(tree)
		tree->release ();

	tree = unknown_cast<Tree> (newTree);
	ownTree = tree == nullptr;

	ASSERT (tree)
	if(tree)
		tree->retain ();
	else
		tree = NEW Tree;
	
	if(isAccessibilityEnabled ())
		if(ItemViewAccessibilityProvider* provider = ccl_cast<ItemViewAccessibilityProvider> (accessibilityProvider))
			provider->rebuildItemProviders ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Tree& TreeView::getTree () const
{ 
	ASSERT (tree != nullptr)
	return *tree;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITreeItem* CCL_API TreeView::getRootItem ()
{
	return tree;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeStyle& TreeView::getTreeStyle () const
{
	return (TreeStyle&)getItemStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TreeView::getStandardStyleIndex () const
{
	return ThemePainter::kTreeViewStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TreeView::setModel (IItemModel* model)
{
	ItemView::setModel (nullptr); // detach old model while selection intact!

	if(ownTree)
	{
		getTree ().removeAll ();
		getTree ().setModel (model);
	}
	else if(model == nullptr)
		selectAll (false); // tree items will survive us, clear their selected flag

	if(model)
	{
		ItemIndex rootIndex;
		bool result = model->getRootItem (rootIndex) != 0;
		ASSERT (result == true && rootIndex.getObject () != nullptr)
		getTree ().setData (rootIndex.getObject ());

		ItemView::setModel (model);
	}
	
	if(isAccessibilityEnabled ())
		if(ItemViewAccessibilityProvider* provider = ccl_cast<ItemViewAccessibilityProvider> (accessibilityProvider))
			provider->rebuildItemProviders ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TreeView::setItemFilter (IObjectFilter* filter)
{
	UnknownPtr<ISubject> oldFilter (getTree ().getItemFilter ());
	if(oldFilter)
		oldFilter->removeObserver (this);

	getTree ().setItemFilter (filter);

	UnknownPtr<ISubject> newFilter (filter);
	if(newFilter)
		newFilter->addObserver (this);

	extraHeight = 0;

	updateSize ();
	invalidate ();
	
	if(isAccessibilityEnabled ())
		if(ItemViewAccessibilityProvider* provider = ccl_cast<ItemViewAccessibilityProvider> (accessibilityProvider))
			provider->rebuildItemProviders ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::onVisualStyleChanged ()
{
	SuperClass::onVisualStyleChanged ();
	
	// new visualstyle might change thumbnail padding and margin
	if(style.isCustomStyle (Styles::kItemViewAppearanceThumbnails))
		updateThumbnails ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::setStyle (StyleRef style)
{
	bool thumbnailsChanged = style.isCustomStyle (Styles::kItemViewAppearanceThumbnails) != this->style.isCustomStyle (Styles::kItemViewAppearanceThumbnails);
	SuperClass::setStyle (style);

	if(thumbnailsChanged)
		updateThumbnails ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TreeView::updateThumbnails ()
{
	TreeResetItemSizes t;
	getTree ().traverse (t);

	updateSize ();
	updateClient ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TreeView::getItemTextInset (ITreeItem* _item)
{
	TreeItem* item = unknown_cast<TreeItem> (_item);
	if(item == nullptr)
		return 0;

	int parentCount = 0;
	TreeItem* tmp = item;
	while(tmp)
	{
		TreeItem* parent = tmp->getParent ();
		if(parent)
			parentCount++;
		tmp = parent;
	}
	
	if(style.isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
		parentCount--;
	
	TreeStyle& treeStyle = getTreeStyle ();
	Coord inset = (treeStyle.getItemInset () * parentCount);

	if(!style.isCustomStyle (Styles::kTreeViewBehaviorExpandAll))
	{
		if(item->canExpand ())
			inset += treeStyle.getExpandSize ().x + treeStyle.getMarginH ();
		else
			inset += treeStyle.getLeafInset () + treeStyle.getMarginH ();
	}
	if(Image* icon = getIcon (item))
	{
		Point iconSize;
		ItemIndex itemIndex (item->asIndex ());

		determineIconSize (iconSize, itemIndex);
		inset += iconSize.x + treeStyle.getMarginH ();
	}

	return inset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TreeView::setZoomFactor (float factor)
{
	if(factor != zoomFactor)
	{
		// reset all cached heights, they will be recalculated based on the new zoom
		TreeResetItemSizesAndTextLayout t;
		getTree ().traverse (t);

		SuperClass::setZoomFactor (factor);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::setRootItem (IUnknown* data)
{
	nameNavigator.reset ();

	if(getTree ().getData () != data)
	{
		extraHeight = 0;

		// keep focus & anchor item
		SharedPtr<TreeItem> oldFocus (focusItem);
		SharedPtr<TreeItem> oldAnchor (anchorItem);

		// unselect all
		focusItem = anchorItem = nullptr;
		selectAll (false);

		tree->setRootItem (data);

		// try to restore focus & anchor item
		if(oldFocus)
			focusItem = tree->findByIndex (oldFocus->getData (), false);
		if(oldAnchor)
			anchorItem = tree->findByIndex (oldAnchor->getData (), false);

		if(style.isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
			tree->expand (true);

		updateSize ();
		invalidate ();
		
		if(isAccessibilityEnabled ())
			if(ItemViewAccessibilityProvider* provider = ccl_cast<ItemViewAccessibilityProvider> (accessibilityProvider))
				provider->rebuildItemProviders ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeView::getFocusItem (ItemIndex& index) const
{
	if(focusItem)
	{
		index = focusItem->asIndex ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeView::setFocusItem (ItemIndexRef index, tbool selectExclusive)
{
	if(index.getObject ())
	{
		TreeItem* item = getTree ().findByIndex (index, false);
		if(item)
		{
			Window::UpdateCollector uc (getWindow ());

			if(TreeItem* parent = item->getParent ())
				expandItem (*parent, true, kExpandParents); // expand all parents

			setFocusTreeItem (item);
			if(!anchorItem || selectExclusive)
				anchorItem = item;

			if(selectExclusive)
			{
				selectAll (false);
				selectItem (index, true);
			}
			makeItemVisible (index);
			return true;
		}
		CCL_PRINTLN ("TreeView::setFocusItem: ITEM NOT FOUND!")
	}
	else
		setFocusTreeItem (nullptr);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::getEditContext (ItemIndex& item, Rect& cellRect, int& editColumn)
{
	if(getFocusItem (item))
	{
		editColumn = this->editColumn;		
		int editColumnIndex = toModelColumnIndex (editColumn);
		getItemRect (cellRect, item, editColumnIndex);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeView::selectItem (ItemIndexRef index, tbool state)
{
	if(state && !model->canSelectItem (index))
		return false;

	TreeItem* item = getTree ().findByIndex (index);
	if(item)
	{
		getSelection ();
		if(state)
		{
			if(style.isCustomStyle (Styles::kItemViewBehaviorSelectExclusive))
				selectAll (false);

			selection->select (item->asIndex ());
		}
		else
			selection->unselect (item->asIndex ());

		item->isSelected (state != 0);
		invalidateTreeItem (item);
		signalSelectionChanged ();
		return true;
	}
	CCL_PRINTLN ("TreeView::selectItem: ITEM NOT FOUND!")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
tbool CCL_API TreeView::selectAll (tbool state)
{
	getSelection ();
	if(state)
	{
		if(style.isCustomStyle (Styles::kItemViewBehaviorSelectExclusive))
			return false;

		// first reset the isSelected flags
		ForEachItem (*selection, idx)
			TreeItem* item = unknown_cast<TreeItem> (idx.getTreeItem ());
			ASSERT (item)
			if(item)
				item->isSelected (false);
		EndFor
		selection->unselectAll ();

		TreeViewSelectAll t (*selection, getTree ());
		getTree ().traverse (t);

		invalidate ();
	}
	else
	{
		ForEachItem (*selection, idx)
			TreeItem* item = unknown_cast<TreeItem> (idx.getTreeItem ());
			ASSERT (item)
			if(item)
			{
				item->isSelected (false);
				invalidateTreeItem (item);
			}
		EndFor
		selection->unselectAll ();
	}
	signalSelectionChanged ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeView::removeItem (ItemIndexRef index)
{
	struct UnselectDeep: TreeTraverser
	{
		IItemSelection& selection;

		UnselectDeep (IItemSelection& selection)
		: TreeTraverser (0), selection (selection)
		{}

		bool visit (TreeItem* item) override
		{
			selection.unselect (item->asIndex ());
			return true;
		}
	};

	TreeItem* item = getTree ().findByIndex (index, false);
	if(item)
	{
		// unselect deep
		getSelection ();
		UnselectDeep traverser (*selection);
		item->traverse (traverser);

		// unfocus deep
		TreeItem* focusParent = focusItem;
		while(focusParent)
		{
			if(focusParent == item)
			{
				focusItem = nullptr;
				break;
			}
			focusParent = focusParent->getParent ();
		}

		// hoist anchor deep
		TreeItem* anchorParent = anchorItem;
		while(anchorParent)
		{
			if(anchorParent == item)
			{
				anchorItem = nullptr;
				break;
			}
			anchorParent = anchorParent->getParent ();
		}

		item->remove ();

	     // calling updateSize directly takes too long if multiple items are removed
		(NEW Message (kUpdateSize))->post (this, -1);

		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeView::invalidateItem (ItemIndexRef index)
{
	TreeItem* item = getTree ().findByIndex (index);
	ASSERT (item != nullptr)
	if(!item)
		return false;

	invalidateTreeItem (item);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TreeView::refreshItem (ITreeItem* item)
{
	TreeItem* treeItem = unknown_cast<TreeItem> (item);
	if(treeItem)
	{
		// check if focusItem will be removed
		if(focusItem && focusItem->hasAncestor (treeItem))
			focusItem = nullptr;

		if(anchorItem && anchorItem->hasAncestor (treeItem))
			anchorItem = nullptr;

		// unselect all that will be removed
		ForEachItem (getSelection (), idx)
			TreeItem* item = unknown_cast<TreeItem> (idx.getTreeItem ());
			ASSERT (item)
			if(item && item->hasAncestor (treeItem))
				selection->unselect (idx);
		EndFor

		bool wasExpanded = treeItem->isExpanded ();
		treeItem->removeAll ();
		treeItem->setTextLayout (nullptr);
		if(wasExpanded)
			expandItem (*treeItem);
		else
			invalidateTreeItem (treeItem);
	}
	nameNavigator.reset ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::attached (View* parent)
{
	if(style.isCustomStyle (Styles::kTreeViewAppearanceNoRoot|Styles::kTreeViewBehaviorExpandAll))
		expandItem (getTree (), true); // root must be expanded

	SuperClass::attached (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::draw (const UpdateRgn& updateRgn)
{
	if(!model)
		return;

	GraphicsPort port (this);

	Font font;
	getFont (font);
	
	if(Image* bg = getItemStyle ().getBackgroundImage ())
	{
		Rect src (bg->getSize ());
		Rect dst;
		getClientRect (dst);
		port.drawImage (bg, src, dst);
	}
	else if(isLayerBackingEnabled () && getStyle ().isTransparent () == false)
	{
		Rect dst;
		getClientRect (dst);
		UpdateRgn targetRegion (dst);
		drawAlternatingBackground (targetRegion);
	}

	TDrawState state (port, updateRgn, font, getItemStyle ().getTextBrush ());
	state.indent (getItemStyle ().getMarginH (), getItemStyle ().getMarginV ());

	if(style.isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
		drawSubItems (&getTree (), state);
	else
		drawItem (&getTree (), state);

	View::draw (updateRgn); // draw children
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TreeView::expandItem (ITreeItem* item, tbool state, int expandMode)
{
	if(style.isCustomStyle (Styles::kTreeViewBehaviorExpandAll))
		state = true;

	TreeItem* treeItem = unknown_cast<TreeItem> (item);
	if(treeItem)
		expandItem (*treeItem, state != 0, expandMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::expandItemChecked (TreeItem& item, bool deep)
{
	if(!item.canAutoExpand ())
		return;

	item.expand (true, false);

	if(deep)
		ForEach (item, TreeItem, child)
			expandItemChecked (*child, true);
		EndFor
			
	if(isAccessibilityEnabled ())
		if(ItemViewAccessibilityProvider* provider = ccl_cast<ItemViewAccessibilityProvider> (accessibilityProvider))
			provider->rebuildItemProviders ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::expandItem (TreeItem& item, bool state, int expandMode)
{
	extraHeight = 0;
	bool focusItemCollapsed = (&item == focusItem) && !state && item.isExpanded ();

	Window::UpdateCollector uc (getWindow ());

	if(style.isCustomStyle (Styles::kTreeViewBehaviorExpandAll))
		state = true;

	bool mustUpdate = false;

	bool deep = (expandMode & kExpandChilds) != 0;
	if(deep || state != item.isExpanded ())
	{
		if(state && (expandMode & kCheckCanAutoExpand))
			expandItemChecked (item, deep);
		else
		item.expand (state, deep);
		mustUpdate = true;
	}

	if(expandMode & kExpandParents)
	{
		TreeItem* parent = item.getParent ();
		while(parent)
		{
			if(state != parent->isExpanded ())
			{
				parent->expand (state);
				mustUpdate = true;
			}
			parent = parent->getParent ();
		}
	}

	if(mustUpdate)
	{
		if(focusItemCollapsed)
		{
			ScopedVar<bool> guard (avoidScrolling, true);
			updateSize ();
		}
		else
			updateSize ();

		UnknownPtr<IObserver> observer (model);
		if(observer)
			observer->notify (this, Message (kItemExpanded, item.asUnknown (), state));

		invalidate ();
	}
	
	if(isAccessibilityEnabled ())
		if(ItemViewAccessibilityProvider* provider = ccl_cast<ItemViewAccessibilityProvider> (accessibilityProvider))
			provider->rebuildItemProviders ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::setFocusTreeItem (TreeItem* item)
{
	if(focusItem != item)
	{
		if(focusItem)
		{
			// reset expand state is item doesn't have children
			if(focusItem->isEmpty ())
				focusItem->expand (false);

			invalidateTreeItem (focusItem);
		}
		focusItem = item;
		if(focusItem)
			invalidateTreeItem (focusItem);
	}

	onItemFocused (item ? item->asIndex () : ItemIndex ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::getAnchorItem (ItemIndex& index) const
{
	TreeView* This = const_cast<TreeView*>(this);
	This->verifyAnchorItem ();

	if(anchorItem)
	{
		index = anchorItem->asIndex ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::setAnchorItem (ItemIndexRef index)
{
	anchorItem = getTree ().findByIndex (index);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::verifyAnchorItem ()
{
	if(!anchorItem)
		anchorItem = getFirstTreeItem ();
	else
	{
		int anchorPos, c;
		if(!getTree ().getItemPosition (anchorItem, anchorPos, c))
			anchorItem = focusItem;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeItem* TreeView::getFirstTreeItem ()
{
	if(style.isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
		ForEach (getTree (), TreeItem, item)
			return item;
		EndFor

	return &getTree ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TreeView::getItemIndex (const Point& where) const
{
	TreeFindItemAtCoord t (where.y, *this);
	getTree ().traverse (t);
	return t.foundIndex;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeItem* TreeView::findTreeItem (const Point& where) const
{
	TreeFindItemAtCoord t (where.y, *this);
	getTree ().traverse (t);
	return t.foundItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeView::findItems (const Rect& rect, IItemSelection& items) const
{
	if(rect.bottom < 0)
		return false;

	int from = getItemIndex (rect.getLeftTop ());
	int to   = getItemIndex (rect.getRightBottom ());
	if(from < 0)
		return false;
	if(to < 0)
		to = NumericLimits::kMaxInt - 1;

	if(style.isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
		from++, to++;

	ccl_order (from, to);
	ccl_lower_limit (from, 0);
	ccl_lower_limit (to, 0);
	
	TreeFindItems t (items, from, to, getTree ());
	getTree ().traverse (t);
	return t.numFound > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TreeView::getItemRect (Rect& rect, ItemIndexRef index, int column) const
{
	TreeItem* item = getTree ().findByIndex (index);
	if(!item ||!getItemRect (rect, item))
		rect.setEmpty ();	
	else if(column != -1)
	{
		ASSERT (columnList != nullptr || column == 0)
		if(columnList)
		{
			columnList->getColumnRange (rect.left, rect.right, column);
			if(column == 0)
				rect.left += getTreeStyle ().getMarginH ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::getItemRect (Rect& r, TreeItem* item) const
{
	TreeGetItemRect t (r, item, *this);
	return getTree ().traverse (t) == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TreeView::makeItemVisible (ItemIndexRef index)
{
	Rect itemRect;
	TreeItem* item = getTree ().findByIndex (index);
	if(item && getItemRect (itemRect, item))
	{
		Rect activeRect;
		getActiveRect (activeRect, itemRect, item);
		makeVisible (activeRect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::invalidateTreeItem (TreeItem* item)
{
	Rect rect;
	if(getItemRect (rect, item))
	{
		rect.left = 0;
		invalidate (rect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeItem* TreeView::skipItems (TreeItem* startItem, int rows, bool onlySelectable)
{
	int flags = ITreeItem::kOnlyExpanded;
	if(onlySelectable || style.isCustomStyle (Styles::kItemViewBehaviorFocusSelectable))
		flags |= ITreeItem::kOnlySelectable;
	if(style.isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
		flags |= ITreeItem::kIgnoreRoot;

	return unknown_cast<TreeItem> (startItem->navigate (rows, flags));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::selectRange (ItemIndexRef index1, ItemIndexRef index2)
{
	TreeItem* item1 = getTree ().findByIndex (index1);
	TreeItem* item2 = getTree ().findByIndex (index2);
	selectRange (item1, item2);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::selectRange (TreeItem* item1, TreeItem* item2)
{
	if(style.isCustomStyle (Styles::kItemViewBehaviorSelectExclusive))
		item2 = item1;

	TreeSelectRange t (this, item1, item2);
	getTree ().traverse (t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::getNextItem (ItemIndex& item, bool forNavigation)
{
	if(item.isValid ())
	{
		if(TreeItem* treeItem = getTree ().findByIndex (item))
		{
			TreeItem* nextTreeItem = skipItems (treeItem, 1, forNavigation);
			if(nextTreeItem != treeItem)
			{
				item = nextTreeItem;
				return true;
			}
		}
	}

	if(!forNavigation)
		return false;

	// wrap to first item
	if(TreeItem* treeItem = getFirstTreeItem ())
	{
		item = treeItem;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::navigate (int32 rows, int32 columns, NavigationMode navigationMode, bool checkOnly)
{
	if((privateFlags & kActive) == 0)
		return false;

	if(checkOnly) // todo
		return true;

	Window::UpdateCollector uc (getWindow ());

	if(!style.isCustomStyle (Styles::kItemViewBehaviorSelection))
		navigationMode = ItemView::kSkip;

	if(!focusItem)
		setFocusTreeItem (getFirstTreeItem ());

	bool noRoot = style.isCustomStyle (Styles::kTreeViewAppearanceNoRoot);

	if(columns)
	{
		if(columns == 1) // expand or go down
		{
			if(focusItem && !focusItem->isExpanded () && focusItem->canExpand ())
				expandItem (*focusItem);
			else
				rows = 1;
		}
		else if(focusItem) // collapse or move to parent
		{
			if(focusItem->isExpanded ())
			{
				if(focusItem != getRootItem () || !style.isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
					expandItem (*focusItem, false);
				return true;
			}
			else if(TreeItem* parent = focusItem->getParent ())
			{
				if(style.isCustomStyle (Styles::kTreeViewAppearanceNoRoot) && parent == getRootItem ())
					rows = -1;
				else
				{
					selectAll (false);
					selectItem (parent, true);
					setFocusTreeItem (parent);
					makeItemVisible (parent->asIndex ());
					return true;
				}
			}
		}
	}

	if(rows == NumericLimits::kMinInt)
	{
		verifyAnchorItem ();

		int anchorPos, c;

		TreeItem* firstItem = getFirstTreeItem ();
		if(firstItem && getTree ().getItemPosition (anchorItem, anchorPos, c))
		{
			if(noRoot)
				anchorPos--;
			rows = - anchorPos;
		}
	}

	if(rows)
	{
		TreeItem* newFocus = nullptr;

		switch(navigationMode)
		{
			case kSkip:
				newFocus = skipItems (focusItem, rows, false);
				anchorItem = newFocus;
				break;
			case kSelect:
				selectAll (false);
				newFocus = skipItems (focusItem, rows, false);
				if(newFocus)
				{
					selectItem (newFocus->asIndex (), true);
					anchorItem = newFocus;
				}
				break;
			case kSelectExtend:
				selectAll (false); 
				CCL_FALLTHROUGH
			case kSelectExtendAdd:
				{
					verifyAnchorItem ();
					newFocus = skipItems (focusItem, rows, true);
					if(newFocus)
						selectRange (anchorItem, newFocus);
				}
				break;
		}
		setFocusTreeItem (newFocus);
		if(newFocus)
			makeItemVisible (newFocus->asIndex ());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::getActiveRect (Rect& activeRect, RectRef itemRect, TreeItem* item)
{
	const TreeStyle& treeStyle = getTreeStyle ();

	Coord width = treeStyle.getExpandSize ().x + treeStyle.getMarginH ();
	if(getIcon (item))
	{
		ItemIndex itemIndex (item->asIndex ());
		Point iconRect;
		determineIconSize (iconRect, itemIndex);
		width += iconRect.x + treeStyle.getMarginH ();
	}

	String title;
	item->getTitle (title);
	if(!title.isEmpty ())
		width += getStringWidth (item);

	int row = 0, col = 0;
	if(!getTree ().getItemPosition (item, row, col))
		col = 0;
	if(style.isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
		col--;

	activeRect = itemRect;
	activeRect.left += col * treeStyle.getItemInset ();
	activeRect.setWidth (width);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::getColumnRange (Coord& left, Coord& right, int& columnIndex, Coord x)
{
	if(columnList)
	{
		// determine column index and coord range
		columnIndex = columnList->getColumnIndex (x);
		if(columnIndex >= 0)
		{
			Coord colLeft = 0, colRight = 0;
			columnList->getColumnRange (colLeft, colRight, columnIndex);
			left  = colLeft;
			right = colRight;
			if(columnIndex == 0)
				left += getTreeStyle ().getMarginH ();
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TreeView::getColumnIndex (PointRef where)
{
	// method name is misleading, but this is analog to what ListView does (called from ItemView::onTrackTooltip)
	if(columnList)
	{
		int index = columnList->getColumnIndex (where.x);
		return columnList->columnIndexToPosition (index, false);
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::isExpandHit (RectRef columnRect, TreeItem* item, PointRef where)
{
	// determine column in tree
	int row = 0, col = 0;
	if(!getTree ().getItemPosition (item, row, col))
		col = 0;
	if(style.isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
		col--;

	Coord inset = col * getTreeStyle ().getItemInset ();
	Coord expandLeft = columnRect.left + inset;

	int expandW = getTreeStyle ().getExpandSize ().x;
	return where.x >= expandLeft && where.x <= expandLeft + expandW;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::openItem (ItemIndexRef item, int column, const GUIEvent& editEvent, RectRef rect)
{
	if(SuperClass::openItem (item, column, editEvent, rect))
		return true;

	TreeItem* treeItem = getTree ().findByIndex (item, true);
	if(treeItem && treeItem->canExpand ())
	{
		// toggle expand state
		expandItem (*treeItem, !treeItem->isExpanded ());
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::onIdleTimer ()
{
	TreeItem* toExpand = DragSession::isInternalDragActive () ? dragOverItem : focusItem;
	if(toExpand)
		expandItem (toExpand);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::onKeyDown (const KeyEvent& event)
{
	// "collapse branch" key commands: Option + Left / Top
	if(tree && event.state.keys == KeyState::kOption && (event.vKey == VKey::kUp || event.vKey == VKey::kLeft))
	{
		bool wasSelected = focusItem && focusItem->isSelected ();
		TreeItem* newFocus = focusItem;
		while(newFocus && newFocus->getParent () != tree)
			newFocus = newFocus->getParent ();

		if(newFocus && event.vKey == VKey::kLeft)
		{
			// collapse the branch containing focus item
			expandItem (*newFocus, false, kExpandChilds);
		}
		else
		{
			// collapse all toplevel items
			ForEach (*tree, TreeItem, item)
				expandItem (*item, false, kExpandChilds);
			EndFor
		}

		setFocusTreeItem (newFocus);
		selectAll (false);

		if(newFocus)
		{
			if(wasSelected)
				selectItem (newFocus, true);

			makeItemVisible (newFocus);
		}
		return true;
	}
	return SuperClass::onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::onDragEnter (const DragEvent& event)
{
	dragOverItem = nullptr;
	return SuperClass::onDragEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::onDragOverItem (const DragEvent& event, ItemIndexRef index)
{
	SuperClass::onDragOverItem (event, index);

	if(style.isCustomStyle (Styles::kTreeViewBehaviorExpandDragItem))
	{
		// setup idle timer for automatic expand
		dragOverItem = tree->findByIndex (index, false);
		if(dragOverItem && !dragOverItem->isExpanded () && dragOverItem->canAutoExpand ())
			startTimer (kAutoExpandDelay, false);
		else
			stopTimer ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::onDragLeave (const DragEvent& event)
{
	dragOverItem = nullptr;
	return SuperClass::onDragLeave (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::onMouseEnter (const MouseEvent& event)
{
	return style.isCustomStyle (Styles::kTreeViewBehaviorExpandMouseItem) || ItemView::onMouseEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::onMouseMove (const MouseEvent& event)
{
	TreeItem* oldFocusItem = focusItem;

	SuperClass::onMouseMove (event);

	if(style.isCustomStyle (Styles::kTreeViewBehaviorExpandMouseItem))
	{
		// setup idle timer for automatic expand
		if(style.isCustomStyle (Styles::kTreeViewBehaviorAutoExpand) && focusItem != oldFocusItem && focusItem)
		{
			if(!focusItem->isExpanded () && focusItem->canAutoExpand ())	
				startTimer (kAutoExpandDelay, false);
			else
				stopTimer ();
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::onMouseDown (const MouseEvent& event)
{
	bool result = false;
	Rect itemRect;
	TreeItem* item = findTreeItem (event.where);
	if(item && getItemRect (itemRect, item) && !style.isCustomStyle (Styles::kItemViewBehaviorNoRubberband|Styles::kItemViewBehaviorSelectFullWidth)
		&& (!columnList || columnList->getCount (true) <= 1))
	{
		Rect activeRect;
		getActiveRect (activeRect, itemRect, item);
		if(!activeRect.pointInside (event.where))
			item = nullptr;
	}

	if(item)
	{
		if(!isFocused () && focusItem == item)
			if(SuperClass::onMouseDown (event)) // give child views a chance
				return true;

		setFocusTreeItem (item);

		int columnIndex = 0;
		getColumnRange (itemRect.left, itemRect.right, columnIndex, event.where.x);
		// itemRect is now column rect

		editColumn = columnList ? columnList->columnIndexToPosition (columnIndex, false) : 0; // ItemView will translate back before calling model (toModelColumnIndex)

		ItemIndex clickedItem (item->asIndex ());
		tbool canSelect = model->canSelectItem (clickedItem);
		bool editCellCalled = false;
		if(canSelect)
		{
			// for other columns than the "tree column", give editCell a chance before we try dragging
			if(columnIndex != 0)
			{
				if(editCell (clickedItem, editColumn, itemRect, event))
					return true;
				editCellCalled = true;
			}

			// try to drag the existing selection
			tbool isSelected = getSelection ().isSelected (clickedItem);
			if(isSelected)
				if(tryDrag (event))
					return true;

			// keep selection for contextmenu
			if(!(isSelected && event.keys.isSet (KeyState::kRButton)))
				doSelection (clickedItem, event);
		}

		bool canExpand = !style.isCustomStyle (Styles::kTreeViewBehaviorExpandAll) && item->canExpand ();
		if(canExpand)
		{
			bool expandClicked = columnIndex == 0 && isExpandHit (itemRect, item, event.where);

			if(expandClicked
				|| (style.isCustomStyle (Styles::kTreeViewBehaviorAutoExpand) && !getSelection ().isMultiple () && event.keys.isSet (KeyState::kLButton) && item->canAutoExpand ())
					&& (style.isCustomStyle (Styles::kItemViewBehaviorNoDrag) || !detectDrag (event))) // don't expand if going to drag
			{
				expandItem (*item, !item->isExpanded ());
				if(expandClicked)
					return true;

				result = true; // cancel mouseDown processing in parent, since scrollbars may have been removed during expand!
				canExpand = false; // prevent toggling a second time below
			}
		}

		// try editControl (child view)
		if(editControl && SuperClass::onMouseDown (event))
			return true;

		// edit cell
		if(canSelect && editCellCalled == false && editCell (clickedItem, editColumn, itemRect, event))
			return true;

		// detect double click for open or expand
		if(!style.isCustomStyle (Styles::kItemViewBehaviorNoDoubleClick))
		{
			bool rightClick = event.keys.isSet (KeyState::kRButton);
			if(!rightClick && detectDoubleClick (event))
			{
				// try to open the item
				if(canSelect && openItem (clickedItem, editColumn, event))
					return true;

				if(canExpand)
				{
					// toggle expand state
					expandItem (*item, !item->isExpanded ());
					return true;
				}
			}
		}

		if(!canSelect)
		{
			if(style.isCustomStyle (Styles::kItemViewBehaviorFocusSelectable))
				setFocusItem (0, true);
			if(tryRubberSelection (event))
				result = true;
			return result;
		}

		// drag the new selection
		if(tryDrag (event))
			return true;
	}
	else
	{
		if(tryRubberSelection (event))
			return true;

		selectAll (false);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::onGesture (const GestureEvent& event)
{
	ItemIndex index;
	if(event.getType () == GestureEvent::kSingleTap)
		return onTap (event);
	else if((event.getType () == GestureEvent::kLongPress || event.getType () == GestureEvent::kSwipe) && event.getState () == GestureEvent::kBegin)
    {
        if(model && findItem (index, event.where))
		{
			AutoPtr<DragSession> session (DragSession::create (this->asUnknown (), IDragSession::kTouchInput));
			Image* icon = nullptr;
			if(IUnknown* obj = model->createDragSessionData (index))
			{
				session->getItems ().add (obj, false); // owned by drag session!
				icon = unknown_cast<Image> (model->getItemIcon (index));
			}
			
			if(!icon && model->isItemFolder (index))
				icon = getItemStyle ().getDefaultIcon ();
			session->setDragImage (icon, getItemStyle ().getBackBrush1 ().getColor ());
			Point spritePos (event.where);
			clientToWindow (spritePos);
			session->setOffset (spritePos);
            return dragItems (*session, IDragSession::kTouchInput);
		}
    }
	
    return SuperClass::onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::onTap (const GestureEvent& event)
{
	bool result = false;
	TreeItem* item = findTreeItem (event.where);
	Rect itemRect;
	getItemRect (itemRect, item);
		
	if(item)
	{
		setFocusTreeItem (item);
		
		ItemIndex clickedItem (item->asIndex ());
		tbool canSelect = model->canSelectItem (clickedItem);
		if(canSelect)
			doSelection (clickedItem, event);
		
		int columnIndex = 0;
		getColumnRange (itemRect.left, itemRect.right, columnIndex, event.where.x);
		// itemRect is now column rect

		bool canExpand = !style.isCustomStyle (Styles::kTreeViewBehaviorExpandAll) && item->canExpand ();
		if(canExpand)
		{
			bool expandClicked = columnIndex == 0 && isExpandHit (itemRect, item, event.where);

			if(expandClicked || (style.isCustomStyle (Styles::kTreeViewBehaviorAutoExpand) && !getSelection ().isMultiple () && item->canAutoExpand ()))
			{
				expandItem (*item, !item->isExpanded ());
				if(expandClicked)
					return true;
			}
		}

		// edit cell
	    editColumn = columnList ? columnList->columnIndexToPosition (columnIndex, false) : 0; // ItemView will translate back before calling model (toModelColumnIndex)
		if(canSelect && editCell (clickedItem, editColumn, itemRect, event))
			return true;
				
		if(!canSelect)
		{
			if(style.isCustomStyle (Styles::kItemViewBehaviorFocusSelectable))
				setFocusItem (0, true);
			return result;
		}
	}
	else
	{		
		selectAll (false);
	}
	return result;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::onMove (const Point& delta)
{
	SuperClass::onMove (delta);

	if(extraHeight && delta.y > 0)
	{
		// scrolling up: we can restitute some of our extraHeight
		Coord oldExtraHeight = extraHeight;
		extraHeight = ccl_max (0, extraHeight - delta.y);
		CCL_PRINTF ("TreeView::onMove (%d): extraHeight: %d\n", delta.y, extraHeight)

		Rect rect (getSize ());
		rect.bottom -= (oldExtraHeight - extraHeight);
		setSize (rect);
		if(ScrollView* scrollView = ScrollView::getScrollView (this))
			scrollView->setTargetSize (rect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::onSize (const Point& delta)
{
	SuperClass::onSize (delta);
	signal (Message (kSizeChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::modelChanged (int changeType, ItemIndexRef item)
{
	if(changeType == kItemRemoved)
	{
		TreeItem* removedItem = unknown_cast<TreeItem> (item.getTreeItem ());

		if(anchorItem && (item.getObject () == anchorItem->getData () || anchorItem->hasAncestor (removedItem)))
			anchorItem = nullptr;
		
		if(focusItem && (item.getObject () == focusItem->getData ()  || focusItem->hasAncestor (removedItem)))
			focusItem = nullptr;

		if(selection)
		{
			// unselect all that will be removed
			selection->unselect (item);

			ForEachItem (*selection, idx)
				TreeItem* item = unknown_cast<TreeItem> (idx.getTreeItem ());
				ASSERT (item)
				if(item && item->hasAncestor (removedItem))
					selection->unselect (idx);
			EndFor
		}
	}
	else if(changeType == kModelChanged)
	{
		anchorItem = nullptr;
		focusItem = nullptr;
	}
	ItemView::modelChanged (changeType, item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TreeView::getItemHeight (ItemIndexRef index) const
{
	if(TreeItem* item = getTree ().findByIndex (index, false))
	{
		Coord height = item->getHeight ();
		if(height < 0)
			item->setHeight (height = const_cast<TreeView*> (this)->determineRowHeight (index));
		return height;
	}
	return getTreeStyle ().getRowHeight ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TreeView::getItemRow (ItemIndexRef index) const
{
	int row = -1;
	int col = -1;
	TreeItem* item = getTree ().findByIndex (index, false);
	if(item && getTree ().getItemPosition (item, row, col))
		if(style.isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
			row--;

	return row;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::getSizeInfo (SizeInfo& info)
{
	Font font;
	getFont (font);
	TreeGetTotalSize t (*this);
	getTree ().traverse (t);

	TreeStyle& treeStyle = getTreeStyle ();
	int marginH = treeStyle.getMarginH ();
	int marginV = treeStyle.getMarginV ();

	Coord w = t.maxInset * treeStyle.getItemInset ();
	w += treeStyle.getExpandSize ().x + marginH;
	w += t.maxTitleWidth;

	if(!style.isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
		t.maxIndex++;

	Coord h = t.totalHeight;

	w += 2 * marginH;
	h += 2 * marginV;
	
	if(columnList)
	{
		ColumnHeader* treeColumn = columnList->getColumnByIndex (0);
		if(treeColumn)
		{
			if(treeColumn->getMinWidth () != -1)
				columnList->setColumnMinWidth (treeColumn, w);
			else if(treeColumn->getWidth () < w)
				columnList->setColumnWidth (treeColumn, w);
			
			w = columnList->getTotalWidth ();
		}
	}

	if(avoidScrolling)
	{
		Coord treeTop = getSize ().top;
		Coord treeBottom = treeTop + h;
		if(treeTop < 0 && treeBottom >= treeStyle.getRowHeight ()) // can scroll up
		{
			if(ScrollView* scrollView = ScrollView::getScrollView (this))
			{
				Rect scrollSize;
				Coord scrollH = scrollView->getScrollSize (scrollSize).getHeight ();

				// we would scroll if the new tree bottom is inside the clip view
				// avoid this by adding some temporary etxra height
				Coord freeSpace = scrollH - treeBottom;

				// horizontal scrollbar might disappear
				if(ScrollBar* hbar = scrollView->getHScrollBar ())
					if(scrollView->getStyle ().isCustomStyle (Styles::kScrollViewBehaviorAutoHideHBar))
						if(w <= scrollSize.getWidth ())
							freeSpace += hbar->getHeight ();

				if(freeSpace > 0)
				{
					h += freeSpace;
					extraHeight = freeSpace;
					CCL_PRINTF ("TreeView::getSizeInfo: extraHeight: %d\n", extraHeight)
				}
			}
		}
	}

	info.width = w;
	info.height = h;
	info.hSnap = 1;
	info.vSnap = getTreeStyle ().isVSnapEnabled () ? treeStyle.getRowHeight () : 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* TreeView::getIcon (TreeItem* item)
{
	Image* icon = nullptr;
	TreeStyle& treeStyle = getTreeStyle ();
	if(!style.isCustomStyle (Styles::kTreeViewAppearanceNoIcons))
	{
		icon = item->getIcon ();
		if(!icon && item->isFolder ())
			icon = treeStyle.getDefaultIcon (item->isExpanded ());
	}
	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* TreeView::getThumbnail (TreeItem* item)
{
	if(style.isCustomStyle (Styles::kItemViewAppearanceThumbnails))
		return item->getThumbnail ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextLayout* TreeView::getTextLayout (TreeItem* item)
{
	if(item->getTextLayout ())
	{
		String title;
		item->getTitle (title);
		if(title != item->getTextLayoutString ())
			item->setTextLayout (nullptr);
	}

	if(item->getTextLayout () == nullptr)
	{
		String title;
		if(!item->getTitle (title).isEmpty ())
		{
			Font font;
			getFont (font);
			ItemIndex itemIndex (item->asIndex ());
			if(ItemStyle::CustomBackground* bg = getCustomBackground (model->getItemBackground (itemIndex)))
				if(bg->textFont)
					font = *bg->textFont;

			Coord height = item->getHeight ();
			if(height < 0)
				item->setHeight (height = determineRowHeight (item));

			if(Image* thumbnailImage = getThumbnail (item))
				height -= getThumbnailAreaHeight (thumbnailImage);
		
			ITextLayout* textLayout = NativeGraphicsEngine::instance ().createTextLayout ();
			textLayout->construct (title, kMaxCoord, height, font,
									ITextLayout::kSingleLine, TextFormat (Alignment::kLeftCenter));
			item->setTextLayout (textLayout);
			item->setTextLayoutString (title);
		}
	}
	return item->getTextLayout ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord TreeView::getStringWidth (TreeItem* item)
{
	if(item->getTextWidth () >= 0)
		return item->getTextWidth ();

	Rect textSize;
	if(ITextLayout* textLayout = getTextLayout (item))
	{
		textLayout->getBounds (textSize);
		item->setTextWidth (textSize.getWidth ());
	}
	return textSize.getWidth ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::drawItem (TreeItem* item, TDrawState& state)
{
	if(!tree->isItemVisible (item))
		return true;

	TreeStyle& treeStyle = getTreeStyle ();
	Coord itemHeight = item->getHeight ();
	if(itemHeight < 0)
		item->setHeight (itemHeight = determineRowHeight (item));

	Rect itemRect (state.indent.x, state.indent.y, getWidth (), state.indent.y + itemHeight);

	// break if items are out of visible range...
	if(itemRect.top > state.updateRgn.bounds.bottom)
		return false;
	
	ItemIndex itemIndex (item->asIndex ());
	if(ItemStyle::CustomBackground* bg = getCustomBackground (model->getItemBackground (itemIndex)))
		if(bg->textFont)
			state.font = *bg->textFont;
		
	if(itemRect.bottom >= state.updateRgn.bounds.top)
	{
		drawItemBackground (item, state, itemRect);

		BrushRef textBrush (item->isSelected () ? getItemStyle ().getSelectedTextBrush () : state.textBrush);
		bool selectfullWidth = style.isCustomStyle (Styles::kItemViewBehaviorSelectFullWidth);
		
		if(columnList)
		{
			Coord left = 0;
			int numColumns = columnList->getCount (false);
			for(int column = 0; column < numColumns; column++)
			{
				ColumnHeader* c = columnList->getColumnAtPosition (column, false);
				if(c && !c->isHidden ())
				{
					Coord colW;
					//the following can cause redraw issues when the treeview grows, maybe later as an option
					//if(column == numColumns - 1)
					//	colW = getWidth () - left; // full remaining width for last column
					//else
						colW = c->getWidth ();

					// tree columm can not be skipped if selection must be drawn
					bool mustDraw = selectfullWidth == false && c->getIndex () == 0 && item->isSelected (); 
					if(left + colW < state.updateRgn.bounds.left && mustDraw == false)
					{
						left += colW;
						continue;
					}

					Rect cellRect (left, itemRect.top, left + colW, itemRect.bottom);
					
					if(c->getIndex () == 0) // tree column
					{
						itemRect.left += left;
						drawItemContent (state.port, itemRect, cellRect, item, state.font, textBrush, state.updateRgn);
						itemRect.left -= left;
					}
					else
					{						
						Color adaptiveColor = (item->isSelected () && selectfullWidth) ? getItemStyle ().getSelectedIconColor () : getItemStyle ().getIconColor ();
						IItemModel::StyleInfo styleInfo = {state.font, selectfullWidth ? textBrush : state.textBrush, getItemStyle ().getBackBrush1 (), adaptiveColor};
						IItemModel::DrawInfo info = {this, state.port, cellRect, styleInfo, item->isSelected () ? IItemModel::DrawInfo::kItemSelectedState : 0};

						model->drawCell (item->asIndex (), c->getIndex (), info);
					}
					left += colW;
					if(left > state.updateRgn.bounds.right)
						break;
				}
			}
		}
		else
			drawItemContent (state.port, itemRect, itemRect, item, state.font, textBrush, state.updateRgn);

		// *** Draw Focus ***
		if(item == focusItem && style.isCustomStyle (Styles::kItemViewBehaviorSelectFullWidth))
		{
			itemRect.left = 0;
			drawFocusRect (state.port, itemRect);
		}
	}

	state.indent.y += itemHeight; // next line

	if(item->isExpanded ())
	{
		state.indent.x += treeStyle.getItemInset ();
		if(!drawSubItems (item, state))
			return false;
		state.indent.x -= treeStyle.getItemInset ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeView::drawSubItems (TreeItem* parent, TDrawState& state)
{
	IterForEach (parent->newIterator (), TreeItem, item)
		if(!drawItem (item, state))
			return false;
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::drawItemContent (GraphicsPort& port, RectRef _itemRect, RectRef cellRect, TreeItem* item, FontRef font, BrushRef textBrush, const UpdateRgn& updateRgn)
{
	Rect itemRect (_itemRect);
	int inset = itemRect.left;

	const TreeStyle& treeStyle = getTreeStyle ();
	Image* thumbnailImage = getThumbnail (item);
	if(thumbnailImage)
		itemRect.bottom -= getThumbnailAreaHeight (thumbnailImage);

	String title;
	item->getTitle (title);
	ItemIndex itemIndex (item->asIndex ());
	bool selectfullWidth = style.isCustomStyle (Styles::kItemViewBehaviorSelectFullWidth);
	
	// *** Expand Button ***
	if(!style.isCustomStyle (Styles::kTreeViewBehaviorExpandAll))
	{
		if(item->canExpand ())
		{
			Rect expandRect (treeStyle.getExpandSize ());
			expandRect.offset (inset);
			expandRect.centerV (itemRect);

			drawExpandButton (port, expandRect, (item->isSelected () && selectfullWidth), item->isExpanded ());

			inset += treeStyle.getExpandSize ().x + treeStyle.getMarginH ();
		}
		else
			inset += treeStyle.getLeafInset () + treeStyle.getMarginH ();
	}

	Rect thumbnailRect (itemRect);
	thumbnailRect.left = inset;
	
	// *** Draw Icon ***
	if(Image* icon = getIcon (item))
	{
		Point iconSize;
		determineIconSize (iconSize, itemIndex);
		ccl_upper_limit (iconSize.y, itemRect.getHeight ()); // icon must not exceed item height (iconsize from style is a single value for both directions)

		Rect iconRect (iconSize);
		iconRect.offset (inset);
		iconRect.centerV (itemRect);

		// *** Draw Overlay ***
		bool drawSelected = (item->isSelected () && selectfullWidth);
		//int state = drawSelected ? IItemModel::DrawInfo::kItemSelectedState : 0;
		IItemModel::StyleInfo styleInfo = {font, textBrush, getItemStyle ().getBackBrush1 (), getItemStyle ().getIconColor ()};
		IItemModel::DrawInfo info = {this, port, iconRect, styleInfo, 0}; // state};
		
		getTheme ().getPainter ().drawBestMatchingFrame (port, icon, iconRect, nullptr, drawSelected ? getItemStyle ().getSelectedIconColor () : getItemStyle ().getIconColor ());
		
		model->drawIconOverlay (itemIndex, info);

		inset += iconSize.x + treeStyle.getMarginH ();
	}

	Rect textRect (itemRect);
	textRect.left = inset;

	// *** Draw Selection ***
	Rect selectionRect (textRect);

	if(!selectfullWidth)
	{
		if(!title.isEmpty ())
			selectionRect.right = selectionRect.left + getStringWidth (item);
			//	todo: using the title width conflicts with model->drawCell

		if(item->isSelected ())
		{
			if(treeStyle.getSelectionBarImage ())
			{
				Rect src (0, 0, treeStyle.getSelectionBarImage ()->getWidth (), treeStyle.getSelectionBarImage ()->getHeight ());
				port.drawImage (treeStyle.getSelectionBarImage (), src, selectionRect);
			}
			else
				port.fillRect (selectionRect, treeStyle.getSelectBrush ());
		}
	}

	// *** Draw Title or other cell content ***
	textRect.right = cellRect.right;
	IItemModel::StyleInfo styleInfo = {font, textBrush, getItemStyle ().getBackBrush1 (), getItemStyle ().getIconColor ()};
	IItemModel::DrawInfo info = {this, port, textRect, styleInfo, 0};

	if(!model->drawCell (itemIndex, 0, info))
	{
		if(!title.isEmpty ())
			if(ITextLayout* textLayout = getTextLayout (item))
				port.drawTextLayout (textRect.getLeftTop (), textLayout, textBrush);
	}

	// *** Draw Selection Overlay ***
	if(treeStyle.getSelectOverlayBrush ().getColor ().alpha != 0)
		if(item->isSelected ())
			port.fillRect (selectionRect, treeStyle.getSelectOverlayBrush ());
	
	// *** Draw Focus ***
	if(!selectfullWidth && item == focusItem)
	{
		selectionRect.left--; // correction for pen width
		selectionRect.right++;
		drawFocusRect (port, selectionRect);
	}

	// *** Draw Thumbnail ***
	if(thumbnailImage)
	{
		Point pos (thumbnailRect.left + treeStyle.getThumbnailPaddingLeft (), textRect.bottom + treeStyle.getThumbnailMarginV () + treeStyle.getThumbnailPaddingTop ());
		drawThumbnail (port, *thumbnailImage, pos);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::drawItemBackground (TreeItem* item, TDrawState& state, RectRef itemRect)
{
	// draw selection or custom background
	if(style.isCustomStyle (Styles::kItemViewBehaviorSelectFullWidth))
	{
		const TreeStyle& treeStyle = getTreeStyle ();
		Rect selectionRect (itemRect);
		selectionRect.left = 0;
		if(treeStyle.getSeparatorPen ().getColor () != 0)
			selectionRect.top++; // leave one pixel for separator
		if(item->isSelected ())
		{
			if(treeStyle.getSelectionBarImage ())
			{
				Rect src (0, 0, treeStyle.getSelectionBarImage ()->getWidth (), treeStyle.getSelectionBarImage ()->getHeight ());
				state.port.drawImage (treeStyle.getSelectionBarImage (), src, selectionRect);
			}
			else
				state.port.fillRect (selectionRect, treeStyle.getSelectBrush ());
		}
		else if(!hasAlternatingBackground ()) // already drawn
		{
			ItemIndex itemIndex (item->asIndex ());
			if(ItemStyle::CustomBackground* bg = getCustomBackground (model->getItemBackground (itemIndex)))
			{
				int row = getItemRow (item->asIndex ());
				if(SolidBrush* brush = bg->brush [row % 2])
					state.port.fillRect (selectionRect, *brush);
				if(bg->separatorPen)
					state.port.drawRect (Rect (selectionRect).setHeight (1), *bg->separatorPen);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::drawExpandButton (GraphicsPort& port, const Rect& rect, bool drawSelected, bool expanded)
{
	int elementId = drawSelected ? ThemeElements::kTreeViewExpandButtonOn : ThemeElements::kTreeViewExpandButton;
	int	elementState = expanded ? ThemeElements::kTreeItemExpanded : ThemeElements::kTreeItemCollapsed;
	
	getTheme ().getPainter ().drawElement (port, rect, elementId, elementState);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeView::determineIconSize (Point& iconSize, ItemIndexRef itemIndex)
{
	ItemStyle::CustomBackground* bg = getCustomBackground (getModel ()->getItemBackground (itemIndex));
	iconSize = (bg && bg->iconSize >= 0) ? Point (bg->iconSize, bg->iconSize) : getTreeStyle ().getIconSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TreeView::notify (ISubject* subject, MessageRef msg)
{
	if((msg == kChanged && isEqualUnknown (subject, getTree ().getItemFilter ())) || msg == kUpdateSize)
	{
		updateSize ();
		invalidate ();
	}
	else if(msg == IItemModel::kNewRootItem && isEqualUnknown (model, subject))
	{
		if(model)
		{
			ItemIndex rootIndex;
			bool result = model->getRootItem (rootIndex) != 0;
			ASSERT (result && rootIndex.getObject () != nullptr)

			if(getTree ().getData () != rootIndex.getObject ())
			{
				focusItem = nullptr;
				anchorItem = nullptr;
				if(selection)
					selection->unselectAll ();

				setRootItem (rootIndex.getObject ());
			}
			else
				refreshItem (&getTree ());
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* TreeView::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW TreeViewAccessibilityProvider (*this);
	return accessibilityProvider;
}
