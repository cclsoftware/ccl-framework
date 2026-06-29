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
// Filename    : ccl/gui/blocks/blockview.cpp
// Description : Block View
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/blocks/blockview.h"

#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/system/clipboard.h"
#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/system/systemshell.h"
#include "ccl/gui/controls/scrollbar.h"
#include "ccl/gui/theme/theme.h"
#include "ccl/gui/gui.h"

#include "ccl/app/params.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/icontextmenu.h"

using namespace CCL;

//************************************************************************************************
// BlockView::SelectionMouseHandler
//************************************************************************************************

class BlockView::SelectionMouseHandler: public MouseHandler
{
public:
	SelectionMouseHandler (BlockView& blockView)
	: MouseHandler (&blockView, MouseHandler::kAutoScrollV),
	  startPosition (0)
	{}

	// MouseHandler
	void onBegin () override
	{
		startNode = getBlockView ().findLayoutNodeAndTextPos (startPosition, first.where);

		SuperClass::onBegin ();
	}

	bool onMove (int moveFlags) override
	{
		int endPosition = 0;
		BlockLayoutNode* endNode = getBlockView ().findLayoutNodeAndTextPos (endPosition, current.where);
		if(startNode && endNode)
			getBlockView ().selectRange (startNode, startPosition, endNode, endPosition);

		return SuperClass::onMove (moveFlags);
	}

private:
	SharedPtr<BlockLayoutNode> startNode;
	int startPosition;

	BlockView& getBlockView () const { return *static_cast<BlockView*> (getView ()); }

	using SuperClass = MouseHandler;
};

//************************************************************************************************
// BlockView::LinkMouseHandler
//************************************************************************************************

class BlockView::LinkMouseHandler: public MouseHandler
{
public:
	LinkMouseHandler (BlockView& blockView, UrlRef url)
	: MouseHandler (&blockView),
	  url (url)
	{}

	// MouseHandler
	void CCL_API finish (const MouseEvent& event, tbool canceled = false) override
	{
		if(!canceled)
			SystemShell::instance ().openUrl (url);

		SuperClass::finish (event, canceled);
	}

private:
	Url url;

	using SuperClass = MouseHandler;
};

//************************************************************************************************
// BlockView::ScrollTouchHandler
//************************************************************************************************

class BlockView::ScrollTouchHandler: public TouchHandler
{
public:
    ScrollTouchHandler (BlockView* blockView)
    : TouchHandler (blockView)
    {
		addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kVertical, GestureEvent::kPriorityHigh);
	}

	tbool CCL_API onGesture (const GestureEvent& event) override
	{
		BlockView* blockView = static_cast<BlockView*> (view);
		if(event.getType () == GestureEvent::kSwipe)
		{
			switch(event.getState ())
			{
			case GestureEvent::kBegin :
				startPosition = event.where;
				startScrollPosition = blockView->scrollPosition;
				break;

			case GestureEvent::kChanged :
				{
					Point delta (event.where - startPosition);
					PointF newScrollPosition (startScrollPosition);
					blockView->setScrollPositionV (startScrollPosition.y + delta.y);
				}
				break;
			}
		}
		return true;
	}

	tbool CCL_API addTouch (const TouchEvent& event) override
	{
		return true; // swallow other touches
	}

private:
	Point startPosition;
	PointF startScrollPosition;
};

//************************************************************************************************
// BlockViewController
//************************************************************************************************

BlockViewController::BlockViewController (BlockView* blockView)
: blockView (blockView)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BlockViewController::checkCommandCategory (CStringRef category) const
{
	return category == "Edit";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BlockViewController::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == "Edit")
	{
		if(msg.name == "Copy")
			return onEditCopy (msg);

		else if(msg.name == "Select All")
			return onSelectAll (msg, true);

		else if(msg.name == "Deselect All")
			return onSelectAll (msg, false);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockViewController::onEditCopy (const CommandMsg& msg)
{
	if(blockView->content->getChildArray ().isEmpty ())
		return false;

	if(!msg.checkOnly ())
	{
		if(!blockView->getSelection ().isEmpty ())
			blockView->copySelection ();
		else
		{
			// copy full text
			if(const BlockLayoutNode* layoutNode = blockView->layout)
			{
				String text (blockView->copyText (*layoutNode));
				Clipboard::instance ().setText (text);
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockViewController::onSelectAll (const CommandMsg& msg, bool state)
{
	if(!msg.checkOnly ())
		blockView->selectAll (state);

	return true;
}

//************************************************************************************************
// BlockSelection::Item
//************************************************************************************************

class BlockSelection::Item: public Object
{
public:
	PROPERTY_SHARED_AUTO (BlockContentNode, contentNode, ContentNode)
	PROPERTY_OBJECT (TextRange, selectionRange, SelectionRange) // for TextLayoutNode
};

//************************************************************************************************
// BlockSelection::Iterator
//************************************************************************************************

class BlockSelection::Iterator
{
public:
	Iterator (const BlockSelection& selection)
	: iterator (selection.blocks)
	{}

	const BlockLayoutNode* nextBlockRange (TextRange& range, BlockLayoutRoot& layout)
	{
		while(Item* item = static_cast<Item*> (iterator.next ()))
		{
			const BlockLayoutNode* layoutNode = layout.findLayoutNodeForContent (item->getContentNode ());
			if(!layoutNode)
				continue;

			range = item->getSelectionRange ();
			return layoutNode;
		}
		return nullptr;
	}

private:
	ObjectListIterator iterator;
};

//************************************************************************************************
// BlockSelection 
//************************************************************************************************

BlockSelection::BlockSelection ()
{
	blocks.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockSelection::Item* BlockSelection::findItem (const BlockLayoutNode* layoutNode) const
{
	return static_cast<Item*> (blocks.findIf ([&] (Object* obj)
		{ return static_cast<Item*> (obj)->getContentNode () == layoutNode->getContentNode (); }));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockSelection::select (BlockLayoutNode* layoutNode, const TextRange& range)
{
	Item* item = findItem (layoutNode);
	if(item)
		ccl_const_cast (item->getSelectionRange ()).join (range);
	else if(BlockContentNode* contentNode = layoutNode->getContentNode ())
	{
		auto* item = NEW Item;
		item->setContentNode (contentNode);
		item->setSelectionRange (range);
		blocks.add (item);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockSelection::unselect (BlockLayoutNode* layoutNode)
{
	Item* item = findItem (layoutNode);
	if(item)
	{
		blocks.remove (item);
		item->release ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockSelection::unselectAll ()
{
	blocks.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockSelection::isEmpty () const
{
	return blocks.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockSelection::isSelected (const BlockLayoutNode* layoutNode) const
{
	return findItem (layoutNode) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockSelection::updateLayoutNodes (const BlockLayoutNode& layoutRoot)
{
	// after layout has been rebuilt, remove selection items with content nodes that have no layout node anymore
	ObjectListIterator iterator (blocks);
	while(auto* item = static_cast<Item*> (iterator.next ()))
	{
		BlockContentNode* contentNode = item->getContentNode ();
		BlockLayoutNode* newLayoutNode = contentNode ? layoutRoot.findLayoutNodeForContent (contentNode) : nullptr;
		if(!newLayoutNode)
		{
			if(blocks.remove (iterator))
				item->release ();
		}
	}
}

//************************************************************************************************
// BlockView
//************************************************************************************************

DEFINE_CLASS (BlockView, View)
DEFINE_CLASS_UID (BlockView, 0xf293110d, 0x125c, 0x44a5, 0x95, 0x55, 0xe9, 0x99, 0x2a, 0x96, 0x6a, 0xe6)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_VISUALSTYLE_CLASS (BlockView, VisualStyle, "BlockViewStyle")
	ADD_VISUALSTYLE_COLOR  ("selectioncolor") ///< background color for selected ranges
END_VISUALSTYLE_CLASS (BlockView)

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockView::BlockView (const Rect& size, StyleRef style)
: View (size, style),
  layout (NEW BlockLayoutRoot),
  content (nullptr),
  visualStyleApplied (false),
  selection (NEW BlockSelection),
  vScrollParam (nullptr),
  vScrollBar (nullptr)
{
	wantsFocus (true);

	AutoPtr<IParameter> scrollParam (NEW ScrollParam (0, "vscroll")); // TODO: create a float variant
	setVScrollParam (scrollParam);
	addVScrollBar ();

	layout->addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockView::~BlockView ()
{
	cancelSignals ();

	setContent (nullptr);

	layout->removeObserver (this);
	safe_release (layout);

	safe_release (selection);
	setVScrollParam (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::setContent (BlockContentRoot* _content)
{
	share_and_observe (this, content, _content);
	if(content)
		updateLayout (kContentChanged);
	else
		resetLayout ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockSelection& BlockView::getSelection () const
{
	return *selection;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BlockView::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && subject)
	{
		 if(subject == content)
			updateLayout (kContentChanged);
		 else if(isEqualUnknown (subject, vScrollParam))
		 {
			 scrollPosition.y = -vScrollParam->getValue ().asFloat ();
			 invalidate ();
		 }
	}
	else if(msg == BlockContentRoot::kMakeNodeVisible)
	{
		auto* contentNode = unknown_cast<BlockContentNode> (msg[0]);
		if(contentNode)
			makeContentNodeVisible (contentNode);
	}
	else if(msg == BlockLayoutRoot::kInvalidateNode)
	{
		auto* layoutNode = unknown_cast<BlockLayoutNode> (msg[0]);
		if(layoutNode)
			invalidateLayoutNode (layoutNode);
	}
	else if(msg == "updateSelectionNodes")
	{
		getSelection ().updateLayoutNodes (*layout);
		updateSelection ();
		invalidate ();
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API BlockView::getController () const
{
	if(!controller)
		controller = NEW BlockViewController (ccl_const_cast (this));

	return controller;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IVisualStyle& CCL_API BlockView::getVisualStyle () const
{
	if(visualStyle)
		return *visualStyle;

	if(VisualStyle* standardStyle = ThemePainter::getStandardStyle (ThemePainter::kBlockViewStyle))
		return *standardStyle;

	return VisualStyle::emptyStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::onVisualStyleChanged ()
{
	updateLayout (kVisualStyleChanged);
	SuperClass::onVisualStyleChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	updateLayout (kVisualStyleChanged);
	SuperClass::onColorSchemeChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::updateLayout (int updateFlags)
{
	bool styleChanged = false;
	if(get_flag<int> (updateFlags, kVisualStyleChanged))
	{
		VisualStyle* newStyle = unknown_cast<VisualStyle> (&getVisualStyle ());
		layout->setStyle (newStyle);
	
		selectionColor = getVisualStyle ().getColor ("selectioncolor", getTheme ().getThemeColor (ThemeElements::kSelectionColor));
		
		visualStyleApplied = true;
		styleChanged = true;
	}	

	if(get_flag<int> (updateFlags, kLayoutWidthChanged))
	{
		CoordF width = ccl_max<CoordF> (1, vScrollBar ? getWidth () - getScrollBarSize () : getWidth ());
		if(isAttached ())
			layout->setLayoutWidth (width);
	}

	if(visualStyleApplied == true) // suppress early layout builds
		if(styleChanged || get_flag<int> (updateFlags, kContentChanged))
		{
			CCL_PRINTLN ("BlockView rebuilding layout")

			layout->nodeDetached ();
			layout->removeNodes ();

			if(content)
			{
				BlockLayoutBuilder builder (*layout);
				builder.buildLayout (*content);
				layout->nodeAttached ();
				(NEW Message ("updateSelectionNodes"))->post (this);
			}
		}

	updateScrollParams ();
	updateSelection ();
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::resetLayout ()
{
	layout->nodeDetached ();
	layout->removeNodes ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockLayoutNode* BlockView::findLayoutNode (PointRef where) const
{
	return layout->findNode (pointIntToF (where) - scrollPosition, BlockLayoutNode::kFindNodeDeep);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockLayoutNode* BlockView::findLayoutNodeAndTextPos (int& textPosition, PointRef where) const
{
	BlockLayoutNode* node = layout->findNode (pointIntToF (where) - scrollPosition, BlockLayoutNode::kFindNodeDeep|BlockLayoutNode::kFindNodeAcceptPrevious);

	if(auto* textNode = ccl_cast<TextLayoutNode> (node))
		if(ITextLayout* textLayout = textNode->getTextLayout ())
		{
			PointF p (pointIntToF (where) - scrollPosition);
			node->rootToLocal (p);
			Coord textMargin = getVisualStyle ().getMetric<float> ("text.margin", 0);
			PointF textOffset (-textMargin, -textMargin);
			p.offset (textOffset);

			RectF nodeSize;
			textNode->getBounds (nodeSize);

			if(p.y >= nodeSize.getHeight ()) // below node: end of text
				textPosition = textLayout->getText ().length ();
			else
				textLayout->hitTest (textPosition, p);
		}
	return node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Url* BlockView::findLink (PointRef where) const
{
	int textPosition = 0;
	if(auto* textNode = ccl_cast<TextLayoutNode> (findLayoutNodeAndTextPos (textPosition, where)))
	{
		const FormattedText* text = textNode->getFormattedText ();
		const FormattedText::FormatRange* range = text ? text->findFormatRange (textPosition) : nullptr;

		if(range && range->getType () == Text::kLink)
			return unknown_cast<Url> (range->getArgument ());
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::updateSelection ()
{
	selectionRegion.setEmpty ();

	// collect selection rectangles from layout nodes
	BlockSelection::Iterator iterator (*selection);
	BlockSelection::TextRange textRange;
	CoordF textMargin = getVisualStyle ().getMetric<float> ("text.margin", 0.f);
	PointF textOffset (textMargin, textMargin);
	while(const BlockLayoutNode* layoutNode = iterator.nextBlockRange (textRange, *layout))
	{
		if(auto* textLayoutNode = ccl_cast<TextLayoutNode> (layoutNode))
			if(ITextLayout* textLayout = textLayoutNode->getTextLayout ())
			{
				// the rectangles provided by TextLayout must be shifted to root node coordinates
				PointF position;
				textLayoutNode->localToRoot (position);
				position.offset (textOffset);
				RegionWithOffset textBounds (selectionRegion, pointFToInt (position));
				textLayout->getTextBounds (textBounds, {textRange.start, textRange.getLength ()});
			}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::selectRange (BlockLayoutNode* startNode, int startTextPosition, BlockLayoutNode* endNode, int endTextPosition)
{
	selection->unselectAll ();

	if(startNode == endNode)
		ccl_order (startTextPosition, endTextPosition);

	bool inRange = false;
	auto visitNode = [&] (BlockLayoutNode& node)
	{
		if(&node == startNode)
			inRange = true;
		else if(!inRange && &node == endNode)
		{
			ccl_swap (startNode, endNode);
			ccl_swap (startTextPosition, endTextPosition);
			inRange = true;
		}

		if(inRange)
		{
			if(auto* textNode = ccl_cast<TextLayoutNode> (&node))
			{
				// full text range for nodes between startNode and endNode
				BlockSelection::TextRange range;
				range.start = textNode == startNode ? startTextPosition : 0;
				range.end = textNode == endNode ? endTextPosition : (textNode->getTextLayout () ? textNode->getTextLayout ()->getText ().length () : 0);
				selection->select (textNode, range);
			}

			if(&node == endNode)
				return false;
		}
		return true;
	};

	layout->visitChildren (visitNode, true);
	updateSelection ();
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::selectAll (bool state)
{
	if(state)
	{
		BlockLayoutNode* startNode = ccl_cast<BlockLayoutNode> (layout->getChildArray ().first ());
		BlockLayoutNode* endNode = nullptr;
		selectRange (startNode, 0, endNode, NumericLimits::kMaxInt);
	}
	else
	{
		selection->unselectAll ();
		updateSelection ();
		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::copySelection () const
{
	Clipboard::instance ().setText (getSelectedText ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String BlockView::copyText (const BlockLayoutNode& parentNode) const
{
	String text;
	auto visitNode = [&] (const BlockLayoutNode& node)
	{
		if(auto* textNode = ccl_cast<TextLayoutNode> (&node))
		{
			if(const FormattedText* formattedText = textNode->getFormattedText ())
			{
				String nodeText (formattedText->getText ());
				nodeText.trimWhitespace ();

				if(!text.isEmpty ())
					text << String::getLineEnd () << String::getLineEnd ();
				text.append (nodeText);
			}
		}
		return true;
	};

	visitNode (parentNode);
	parentNode.visitChildren (visitNode, true);
	return text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String BlockView::getSelectedText () const
{
	String text;

	BlockSelection::Iterator iterator (*selection);
	BlockSelection::TextRange textRange;
	while(const BlockLayoutNode* layoutNode = iterator.nextBlockRange (textRange, *layout))
	{
		if(auto* textLayoutNode = ccl_cast<TextLayoutNode> (layoutNode))
			if(ITextLayout* textLayout = textLayoutNode->getTextLayout ())
			{
				if(!text.isEmpty ())
					text << String::getLineEnd ();
				text.append (textLayout->getText ().subString (textRange.start, textRange.getLength ()));
			}
	}
	return text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord BlockView::getScrollBarSize () const
{
	Coord scrollBarSize = getTheme ().getThemeMetric (ThemeElements::kScrollBarSize);
	if(getStyle ().isSmall ())
		scrollBarSize /= 2;
	return scrollBarSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::setVScrollParam (IParameter* param)
{
	if(vScrollParam)
	{
		UnknownPtr<ISubject> (vScrollParam)->removeObserver (this);
		vScrollParam->release ();
	}
	vScrollParam = param;
	if(vScrollParam)
	{
		vScrollParam->retain ();
		UnknownPtr<ISubject> (vScrollParam)->addObserver (this);
	}
	updateScrollParams ();

	if(vScrollBar)
		vScrollBar->setParameter (vScrollParam);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::addVScrollBar ()
{
	StyleFlags barStyle (Styles::kVertical);
	if(style.isSmall ())
		barStyle.common |= Styles::kSmall;

	Rect barRect;
	getClientRect (barRect);
	barRect.left = barRect.right - getScrollBarSize ();

	vScrollBar = NEW ScrollBar (barRect, vScrollParam, barStyle);
	vScrollBar->setSizeMode (kAttachTop|kAttachBottom|kAttachRight);
	vScrollBar->setZoomFactor (getZoomFactor ());
	vScrollBar->setName ("vbar");
	if(theme)
		vScrollBar->setTheme (theme);
	insertView (0, vScrollBar);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BlockView::scrollByH (Coord offset)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BlockView::scrollByV (Coord offset)
{
	setScrollPositionV (scrollPosition.y - offset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& CCL_API BlockView::getClipViewRect (Rect& bounds) const
{
	Point p;
	clientToScreen (p);
	getClientRect (bounds);
	bounds.offset (p);
	return bounds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BlockView::makeVisible (RectRef rect, tbool relaxed)
{
	RectF viewport (-scrollPosition.x, -scrollPosition.y, pointIntToF (getSize ().getSize ())); // currently visible rect

	CoordF beyondTop = viewport.top - rect.top;
	CoordF beyondBottom = rect.bottom - viewport.bottom;

	if(beyondBottom > 0)
		scrollByV (beyondBottom);
	else if(beyondTop > 0)
		scrollByV (-beyondTop);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockLayoutNode* BlockView::getLayoutNode (BlockContentNode* contentNode) const
{
	BlockLayoutNode* layoutNode = layout->findLayoutNodeForContent (contentNode);
	if(!layoutNode)
	{
		// try again with last child in case no layout node was created for an intermediate content node
		auto* lastchild = ccl_cast<BlockContentNode> (contentNode->getChildArray ().last ());
		layoutNode = lastchild ? layout->findLayoutNodeForContent (lastchild) : nullptr;
	}
	return layoutNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::makeContentNodeVisible (BlockContentNode* contentNode)
{
	if(BlockLayoutNode* layoutNode = getLayoutNode (contentNode))
	{
		RectF rect;
		layoutNode->getBounds (rect);
		rect.moveTo (PointF ());

		// convert from node-local to layout-root coordinates
		PointF position;
		rect.offset (layoutNode->localToRoot (position));

		makeVisible (rectFToInt (rect));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::invalidateLayoutNode (BlockLayoutNode* layoutNode)
{
	if(layoutNode)
	{
		RectF layoutRect;
		layoutNode->getBounds (layoutRect);
		layoutRect.moveTo (PointF ());

		// transform from layout coordinates to view coordinates:
		// 1. convert from node-local to layout-root coordinates
		PointF positionInRoot;
		layoutNode->localToRoot (positionInRoot);
		layoutRect.offset (positionInRoot);

		// 2. apply scroll offset to convert layout-root to view coordinates
		layoutRect.offset (scrollPosition);

		invalidate (rectFToInt (layoutRect));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API BlockView::getVScrollParam ()
{
	return vScrollParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API BlockView::getHScrollParam ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float BlockView::getScrollSpeedV () const
{
	return 15;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::updateScrollParams ()
{
	if(vScrollParam)
	{
		RectF contentSize;
		layout->getBounds (contentSize);

		CoordF viewHeight = getHeight ();
		CoordF contentHeight = contentSize.getHeight ();

		CoordF range = ccl_max (0.f, contentHeight - viewHeight);
		CoordF pageSize = contentHeight != 0 ? viewHeight / contentHeight : 0;
		if(contentHeight > 0)
		{
			setScrollParamRange (vScrollParam, range, pageSize);
			vScrollParam->setValue (-scrollPosition.y);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::setScrollParamRange (IParameter* param, int range, float pageSize)
{
	UnknownPtr<IScrollParameter> scrollParam (param);
	if(scrollParam)
		scrollParam->setRange (range, pageSize);
	else
	{
		param->setMin (0);
		param->setMax ((float)range);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::setScrollPositionV (CoordF position)
{
	ccl_upper_limit (position, 0.f);
	if(vScrollParam)
		ccl_lower_limit (position, -vScrollParam->getMax ().asFloat ());

	if(position != scrollPosition.y)
	{
		scrollPosition.y = position;

		if(vScrollParam)
			vScrollParam->setValue (-scrollPosition.y);

		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::attached (View* parent)
{
	onVisualStyleChanged ();
	updateLayout (kLayoutWidthChanged);
	SuperClass::attached (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::removed (View* parent)
{
	resetLayout ();
	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::draw (const UpdateRgn& updateRgn)
{
	GraphicsPort graphics (this);
	RectF updateRect (rectIntToF (updateRgn.bounds));

	{
		// apply scroll offset
		Transform transform;
		transform.translate (scrollPosition.x, scrollPosition.y);
		TransformSetter ts (graphics, transform);
		updateRect.offset (scrollPosition * -1);

		// draw selection
		Rect updateRectInt (rectFToEnclosingInt (updateRect));
		SolidBrush selectionBrush (selectionColor);
		for(Rect rect : selectionRegion.getRects ())
			if(rect.intersect (updateRectInt))
				graphics.fillRect (rect, selectionBrush);

		// draw layout
		layout->drawNode (graphics, updateRect);
	}

	SuperClass::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockView::onSize (const Point& delta)
{
	if(delta.x != 0 && isAttached ())
		updateLayout (kLayoutWidthChanged);
	else
		updateScrollParams ();

	SuperClass::onSize (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockView::onContextMenu (const ContextMenuEvent& event)
{
	BlockLayoutNode* layoutNode = findLayoutNode (event.where);

	if(event.wasKeyPressed || layoutNode)
	{
		// if nothing selected, copy text of clicked node or URL
		if(!event.wasKeyPressed && getSelection ().isEmpty ())
		{
			String text;
			if(const Url* url = findLink (event.where))
				text = UrlDisplayString (*url); 
			else if(layoutNode)
				text = copyText (*layoutNode);

			AutoPtr<ICommandHandler> copyNodetext (makeCommandDelegate ([text] (CmdArgs args, VariantRef data)
			{
				if(text.isEmpty ())
					return false;

				if(!args.checkOnly ())
					Clipboard::instance ().setText (text);
				return true;
			}, nullptr));

			event.contextMenu.addCommandItem (CommandRegistry::find ("Edit", "Copy"), copyNodetext);
			return true;
		}

		// copy selected or all text
		event.contextMenu.addCommandItem (CommandRegistry::find ("Edit", "Copy"), controller);
		return true;
	}
	else
		return SuperClass::onContextMenu (event);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockView::onMouseWheel (const MouseWheelEvent& event)
{
	if(SuperClass::onMouseWheel (event))
		return true;

	if(event.isVertical ())
	{
		CoordF delta = -event.delta;
		if(!event.isContinuous ())
			delta = -event.delta * getScrollSpeedV ();

		setScrollPositionV (scrollPosition.y - delta);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockView::onMouseEnter (const MouseEvent& event)
{
	return onMouseMove (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockView::onMouseMove (const MouseEvent& event)
{
	const Url* url = findLink (event.where);

	ThemeElements::CursorID cursor = url ? ThemeElements::kPointhandCursor : ThemeElements::kArrowCursor;
	setCursor (getTheme ().getThemeCursor (cursor));

	// link title as tooltip
	String tooltip;
	if(auto* urlWithTitle = ccl_cast<UrlWithTitle> (url))
		tooltip = urlWithTitle->getTitle ();

	if(tooltip != getTooltip ())
	{
		setTooltip (tooltip);
		GUI.retriggerTooltip (this);
	}

	return SuperClass::onMouseEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockView::onMouseLeave (const MouseEvent& event)
{
	setTooltip (String::kEmpty);

	return SuperClass::onMouseLeave (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* BlockView::createMouseHandler (const MouseEvent& event)
{
	if(event.keys.isSet (KeyState::kLButton))
	{
		const Url* url = findLink (event.where);
		if(url && !detectDrag (event))
			return NEW LinkMouseHandler (*this, *url);

		bool clearSelection = !event.keys.isSet (KeyState::kShift);
		if(clearSelection)
			selectRange (nullptr, 0, nullptr, 0);

		if(detectDoubleClick (event))
		{
			// select word
			int textPosition = 0;
			auto* textNode = ccl_cast<TextLayoutNode> (findLayoutNodeAndTextPos (textPosition, event.where));
			if(textNode)
			{
				ITextLayout::Range range (0, 0);
				if(textNode->getTextLayout ()->getWordRange (range, textPosition) == kResultOk)
					selectRange (textNode, range.start, textNode, range.start + range.length);

				return NEW NullMouseHandler (this);
			}
		}
		return NEW SelectionMouseHandler (*this);
	}
	return SuperClass::createMouseHandler (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* BlockView::createTouchHandler (const TouchEvent& event)
{
	if(event.touches.getTouchCount () > 0)
	{
		// check if scrolling is possible
		if(vScrollParam && vScrollParam->getMax () > vScrollParam->getMin ())
			return NEW ScrollTouchHandler (this);
	}
	return nullptr;
}
