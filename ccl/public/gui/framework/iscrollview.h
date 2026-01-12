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
// Filename    : ccl/public/gui/framework/iscrollview.h
// Description : ScrollView Interface
//
//************************************************************************************************

#ifndef _ccl_iscrollview_h
#define _ccl_iscrollview_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/gui/graphics/rect.h"
#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

interface IView;
interface IParameter;

//************************************************************************************************
// ScrollView styles
//************************************************************************************************

namespace Styles
{
	enum ScrollViewStyles
	{
		// common styles (aliases)
		kScrollViewAppearanceHScrollBar		= kHorizontal,	///< horizontal scrollbar
		kScrollViewAppearanceVScrollBar		= kVertical,	///< vertical scrollbar
		kScrollViewAppearanceScrollBars		= kScrollViewAppearanceHScrollBar|kScrollViewAppearanceVScrollBar,

		// custom styles
		kScrollViewBehaviorAutoHideHBar		= 1<<0,			///< automatically shows/hides horizontal scrollbar when necessary
		kScrollViewBehaviorAutoHideVBar		= 1<<1,         ///< automatically shows/hides vertical scrollbar when necessary
		kScrollViewBehaviorAutoHideBoth		= kScrollViewBehaviorAutoHideHBar|kScrollViewBehaviorAutoHideVBar,  ///< automatically shows/hides both scrollbars when necessary

		kScrollViewAppearanceHButtons		= 1<<2,			///< horizontal scroll buttons at left & right edge
		kScrollViewAppearanceVButtons		= 1<<3,			///< vertical scroll buttons at top & bottom edge
		kScrollViewBehaviorAutoHideHButtons	= 1<<4,			///< automatically show horizontal scroll buttons only when necessary
		kScrollViewBehaviorAutoHideVButtons	= 1<<5,			///< automatically show vertical scroll buttons only when necessary

		kScrollViewBehaviorCanScrollH		= 1<<6,			///< allow horizontal scrolling even if the ScrollView has no (own) scrollbar or buttons
		kScrollViewBehaviorCanScrollV		= 1<<7,			///< allow vertical scrolling even if the ScrollView has no (own) scrollbar or buttons

		kScrollViewBehaviorExtendTarget		= 1<<8,			///< make the target at least as big as the scroll area
		kScrollViewBehaviorNoScreenScroll	= 1<<9,			///< do not use os scroll functions, just invalidate instead
		kScrollViewBehaviorLayeredScroll	= 1<<10,		///< use graphic layers for scrolling, if available
		kScrollViewBehaviorSnapToViews		= 1<<11,		///< snap to subview positions; ignores the explicit snap value		
		
		kScrollViewBehaviorSnapToViewsDeep	= (1<<12)|kScrollViewBehaviorSnapToViews, ///< snap to subview positions recursively
		
		kScrollViewBehaviorTargetLimits		= 1<<13,		///< adjust sizeLimits of ScrollView so that it can't be larger than the target's limits allow
		kScrollViewBehaviorScrollByPage		= 1<<14,		///< snap to full pages, limit touch-based scrolling to one page per gesture
		kScrollViewBehaviorMouseScroll		= 1<<15,		///< allow scrolling by dragging the mouse in scroll view area
		kScrollViewBehaviorNotifications	= 1<<16,		///< sends notifications about current postion during animation
		kScrollViewBehaviorNoTiledLayers	= 1<<17,		///< when kScrollViewBehaviorLayeredScroll is set, use a regular layer instead of a tiled one
		kScrollViewBehaviorRelativeResize	= 1<<18,		///< scrollview resizes the target, keeping the ratio of the visible and hidden area
		kScrollViewBehaviorSnappedTarget 	= 1<<19,		///< target height is multiple of snap height
		kScrollViewBehaviorOmniDirectional	= 1<<20,		///< scrollview allows scrolling in both directions at the same time
		kScrollViewBehaviorNoSwipe			= 1<<21,		///< no swipe gesture used for scrollviews
		kScrollViewBehaviorAllowZoomGesture	= 1<<22,		///< let another touch handler take over on a zoom gesture
		kScrollViewBehaviorLimitToScreen	= 1<<23,		///< don't grow larger than current monitor size when autosizing to content
		kScrollViewBehaviorLatchWheel		= 1<<24,		///< view locks the mouse wheel scroll to itself (no child handling) for a limited time after the last handled mouse wheel event
		kScrollViewAppearancePageControl	= 1<<25,		///< use page controls instead of scroll bars
		kScrollViewBehaviorCenterTarget     = 1<<26,        ///< center target if target is smaller than scroll view (do not combine with kScrollViewBehaviorExtendTarget)
		kScrollViewBehaviorVScrollSpace		= 1<<27,		///< addional reserved space for vertical scrollbar (avoid automatic horizontal scrollbar, because of space covered when vertical bar is shown)
		kScrollViewBehaviorHScrollSpace		= 1<<28 		///< addional reserved space for horizontal scrollbar (avoid resizing of clip view, when kScrollViewBehaviorAutoHideHBar is set)
	};
}

//************************************************************************************************
// IScrollView
/** Scroll view interface. 
	\ingroup gui_view */
//************************************************************************************************

interface IScrollView: IUnknown
{
	/** Initialize with target view. */
	virtual tresult CCL_API construct (IView* target) = 0;

	/** Get target view. */
	virtual IView* CCL_API getTargetView () const = 0;
	
	/** Get size of scroll area. */
	virtual Rect& CCL_API getScrollSize (Rect& r) const = 0;

	/** Resize target. */
	virtual void CCL_API setTargetSize (const Rect& r) = 0;

	/** Get snap. */
	virtual const Point& CCL_API getSnap () const = 0;

	/** Set snap. */
	virtual void CCL_API setSnap (const Point& snap) = 0;
	
	/** Set vertical scroll parameter. */
	virtual void CCL_API setVScrollParam (IParameter* param) = 0;

	/** Set horizontal scroll parameter. */
	virtual void CCL_API setHScrollParam (IParameter* param) = 0;

	/** Get vertical scroll parameter. */
	virtual IParameter* CCL_API getVScrollParam () = 0;

	/** Get horizontal scroll parameter. */
	virtual IParameter* CCL_API getHScrollParam () = 0;

	/** Scroll target to position, animated if duration > 0. */
	virtual void CCL_API scrollTo (PointRef targetPos, double duration = 0., float velocity= 0.) = 0;

	/** Get current target position, could be animated! */
	virtual void CCL_API getPosition (Point& targetPos) const = 0;

	/** Stop running animations. */
	virtual void CCL_API stopAnimations () = 0;

	// scroll notifications
	DECLARE_STRINGID_MEMBER (kOnScrollBegin)
	DECLARE_STRINGID_MEMBER (kOnScrollUpdate)	///< msg[0]: targetPos.x, msg[1]: targetPos.y
	DECLARE_STRINGID_MEMBER (kOnScrollEnd)
	
	DECLARE_IID (IScrollView)
};

DEFINE_IID (IScrollView, 0xb1641082, 0x7c04, 0x41f2, 0x8d, 0x20, 0x5d, 0xb8, 0x81, 0x7d, 0xf7, 0x37)
DEFINE_STRINGID_MEMBER (IScrollView, kOnScrollBegin, "onScrollBegin")
DEFINE_STRINGID_MEMBER (IScrollView, kOnScrollEnd, "onScrollEnd")
DEFINE_STRINGID_MEMBER (IScrollView, kOnScrollUpdate, "onScrollUpdate")

//************************************************************************************************
// IScrollable
/** Interface used by the AutoScroller.
	Implemented by Scrollview and views that implement scrolling by other means. 
	\ingroup gui_view */
//************************************************************************************************

interface IScrollable: IUnknown
{
	/** Get the rect of the clip view in screen coords. */
	virtual Rect& CCL_API getClipViewRect (Rect& bounds) const = 0;

	/** Scroll horizontally by given offset. */
	virtual void CCL_API scrollByH (Coord offset) = 0;

	/** Scroll vertically by given offset. */
	virtual void CCL_API scrollByV (Coord offset) = 0;

	/** Try to make part of client area visible; relaxed: don't scroll if the rect is already partly visible.*/
	virtual tbool CCL_API makeVisible (RectRef rect, tbool relaxed = false) = 0;

	/** Get vertical scroll parameter. */
	virtual IParameter* CCL_API getVScrollParam () = 0;

	/** Get horizontal scroll parameter. */
	virtual IParameter* CCL_API getHScrollParam () = 0;

	DECLARE_IID (IScrollable)
};

DEFINE_IID (IScrollable, 0xf0c57a6, 0x804d, 0x4015, 0xba, 0xa, 0x25, 0x74, 0x8, 0x1d, 0xe8, 0xd7)

} // namespace CCL

#endif // _ccl_iscrollview_h
