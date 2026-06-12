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
// Filename    : ccl/gui/blocks/blockview.h
// Description : Block View
//
//************************************************************************************************

#ifndef _ccl_blockview_h
#define _ccl_blockview_h

#include "ccl/gui/views/view.h"
#include "ccl/gui/theme/visualstyleclass.h"
#include "ccl/gui/blocks/blocklayout.h"
#include "ccl/gui/graphics/mutableregion.h"

#include "ccl/base/math/mathrange.h"

#include "ccl/public/gui/framework/iscrollview.h"
#include "ccl/public/gui/icommandhandler.h"

namespace CCL {

class ScrollBar;
class BlockSelection;
class Url;

//************************************************************************************************
// BlockView
//************************************************************************************************

class BlockView: public View,
				 public IScrollable
{
public:
	DECLARE_CLASS (BlockView, View)

	BlockView (const Rect& size = Rect (), StyleRef style = 0);
	~BlockView ();

	void setContent (BlockContentRoot* content);

	BlockLayoutNode* findLayoutNode (PointRef where) const;
	BlockLayoutNode* findLayoutNodeAndTextPos (int& textPosition, PointRef where) const;
	const Url* findLink (PointRef where) const;
	String copyText (const BlockLayoutNode& parentNode) const;

	BlockSelection& getSelection () const;
	String getSelectedText () const;
	void selectRange (BlockLayoutNode* startNode, int startTextPosition, BlockLayoutNode* endNode, int endTextPosition);
	void selectAll (bool state);

	void makeContentNodeVisible (BlockContentNode* contentNode);

	void setVScrollParam (IParameter* param);

	// IScrollable
	void CCL_API scrollByH (Coord offset) override;
	void CCL_API scrollByV (Coord offset) override;
	Rect& CCL_API getClipViewRect (Rect& bounds) const override;
	tbool CCL_API makeVisible (RectRef rect, tbool relaxed = false) override;
	IParameter* CCL_API getVScrollParam () override;
	IParameter* CCL_API getHScrollParam () override;

	// View
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	const IVisualStyle& CCL_API getVisualStyle () const override;
	IUnknown* CCL_API getController () const override;
	void attached (View* parent) override;
	void removed (View* parent) override;
	void draw (const UpdateRgn& updateRgn) override;
	void onSize (const Point& delta) override;
	void onVisualStyleChanged () override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	bool onContextMenu (const ContextMenuEvent& event) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;

	CLASS_INTERFACE (IScrollable, View)

protected:
	static void setScrollParamRange (IParameter* param, int range, float pageSize);

	BlockLayoutRoot* layout;
	BlockContentRoot* content;
	mutable AutoPtr<ICommandHandler> controller;
	bool visualStyleApplied;
	PointF scrollPosition;
	IParameter* vScrollParam;
	ScrollBar* vScrollBar;

	BlockSelection* selection;
	SelectionRegion selectionRegion;
	Color selectionColor;

	class SelectionMouseHandler;
	class LinkMouseHandler;
	class ScrollTouchHandler;
	friend class BlockViewController;

	enum UpdateFlags
	{
		kContentChanged = 1<<0,
		kVisualStyleChanged = 1<<1,
		kLayoutWidthChanged = 1<<2
	};

	void updateLayout (int updateFlags);
	void resetLayout ();
	void updateSelection ();
	void copySelection () const;

	void updateScrollParams ();
	float getScrollSpeedV () const;
	void setScrollPositionV (CoordF position);
	void addVScrollBar ();
	Coord getScrollBarSize () const;

	void invalidateLayoutNode (BlockLayoutNode* contentNode);
	BlockLayoutNode* getLayoutNode (BlockContentNode* contentNode) const;
};

DECLARE_VISUALSTYLE_CLASS (BlockView)

//************************************************************************************************
// BlockSelection 
//************************************************************************************************

class BlockSelection: public Object
{
public:
	BlockSelection ();
	
	class Iterator;
	using TextRange = Math::Range<int>;

	void select (BlockLayoutNode* layoutNode, const TextRange& range = TextRange ());
	bool unselect (BlockLayoutNode* layoutNode);
	void unselectAll ();
	bool isEmpty () const;
	bool isSelected (const BlockLayoutNode* layoutNode) const;

	void updateLayoutNodes (const BlockLayoutNode& root);

private:
	ObjectList blocks;

	class Item;
	Item* findItem (const BlockLayoutNode* layoutNode) const;
};

//************************************************************************************************
// BlockViewController
/** Controller that handles commands for a BlockView. */
//************************************************************************************************

class BlockViewController: public Unknown,
						   public ICommandHandler
{
public:
	BlockViewController (BlockView* view);

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	CLASS_INTERFACE (ICommandHandler, Unknown)

protected:
	BlockView* blockView;

	bool onEditCopy (const CommandMsg& msg);
	bool onSelectAll (const CommandMsg& msg, bool state);
};

} // namespace CCL

#endif // _ccl_blockview_h
