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
// Filename    : ccl/gui/views/scrollview.h
// Description : Scroll View
//
//************************************************************************************************

#ifndef _ccl_scrollview_h
#define _ccl_scrollview_h

#include "ccl/gui/views/view.h"
#include "ccl/gui/theme/visualstyleclass.h"

#include "ccl/public/gui/framework/iscrollview.h"

namespace CCL {

class ScrollBar;

//************************************************************************************************
// ScrollView
/** Used to show a part of larger view with the ability to scroll the visble part. 
A Scrollview has a so called clip view, that contains the content (target view). 
It can also have Scroll bars or scroll buttons for controlling the visible area.

For each direction, scrollbars can be added permanently (options "horizontal" , "vertical") 
or only when required (options "autohideh", "autohidev", "autohideboth").

Scroll buttons can be used as an alternative, e.g. when there is not enough space for scroll bars 
(options "hbuttons", "vbuttons", "autobuttonsh", "autobuttonsv"). */
//************************************************************************************************

class ScrollView: public View,
				  public IScrollView,
				  public IScrollable
{
public:
	DECLARE_CLASS (ScrollView, View)

	ScrollView (const Rect& size = Rect (), View* target = nullptr, StyleRef style = 0, VisualStyle* visualStyle = nullptr, float zoomFactor = 1.f);
	~ScrollView ();

	DECLARE_STYLEDEF (customStyles)

	static ScrollView* getScrollView (const View* targetView);	///< get scroll view from its target view

	// Scrollbars
	void initScrollBars ();
	void checkPosition ();
	void checkAutoHide ();

	// Header
	void setHeader (View* header);
	View* getHeader () const;

	// Scroll target
	View* getTarget () const;
	Rect& CCL_API getScrollSize (Rect& r) const override;		///< [IScrollView]
	void CCL_API setTargetSize (const Rect& r) override;		///< [IScrollView]

	// Snapping
	const Point& CCL_API getSnap () const override;				///< [IScrollView]
	void CCL_API setSnap (const Point& snap) override;			///< [IScrollView]
	void setHSnap (int snapH);
	void setVSnap (int snapV);
	void snapTargetPos (Point& targetPos, PointRef direction);

	// Scrollbars
	void CCL_API setVScrollParam (IParameter* param) override;	///< [IScrollView]
	void CCL_API setHScrollParam (IParameter* param) override;	///< [IScrollView]
	IParameter* CCL_API getVScrollParam () override;			///< [IScrollView]
	IParameter* CCL_API getHScrollParam () override;			///< [IScrollView]
	ScrollBar* getVScrollBar ();
	ScrollBar* getHScrollBar ();
	void setVScrollBarStyle (VisualStyle* visualStyle);
	void setHScrollBarStyle (VisualStyle* visualStyle);

	PROPERTY_MUTABLE_CSTRING (persistenceID, PersistenceID)

	// IScrollable
	void CCL_API scrollByV (Coord offset) override; ///< scroll up (offset < 0) or down (offset > 0)
	void CCL_API scrollByH (Coord offset) override; ///< scroll left (offset < 0) or right (offset > 0)
	Rect& CCL_API getClipViewRect (Rect& bounds) const override;

	// IObserver
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// View overrides:
	void onSize (const Point& delta) override;
	void setStyle (StyleRef style) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
    bool onGesture (const GestureEvent &event) override;
	bool onDragEnter (const DragEvent& event) override;
	bool onDrop (const DragEvent& event) override;
	void onViewsChanged () override;
	void onChildSized (View* child, const Point& delta) override;
	tbool CCL_API makeVisible (RectRef rect, tbool relaxed = false) override;
	void passDownSizeLimits () override;
	void calcSizeLimits () override;
	void calcAutoSize (Rect& rect) override;
	void CCL_API autoSize (tbool horizontal, tbool vertical) override;
	void draw (const UpdateRgn& updateRgn) override;
	void attached (View* parent) override;
	void removed (View* parent) override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	AccessibilityProvider* getAccessibilityProvider () override;

	CLASS_INTERFACE2 (IScrollView, IScrollable, View)

	void syncHeader (Coord scrollPos);

	void CCL_API scrollTo (PointRef targetPos, double duration = 0., float velocity= 0.) override;	///< [IScrollView]
	void CCL_API getPosition (Point& targetPos) const override; ///< [IScrollView]
	void CCL_API stopAnimations () override;
	
	bool isScrollByPageEnabled () const;
	PointRef getScrollByPageSize (Point& size) const;

	PROPERTY_FLAG (privateFlags, kManipulating, isManipulating)
	PROPERTY_FLAG (privateFlags, kAnimatingX, isAnimatingX)
	PROPERTY_FLAG (privateFlags, kAnimatingY, isAnimatingY)

	static constexpr Coord kMinimalPagingMovement = 100; ///< minimal delta to show next page
	static constexpr double kScrollWheelLatchDelay = 0.5; ///< delay time for unlatching the scroll wheel target after last scroll event
	
	void setManipulation (bool begin);	///< manipulation: user is interacting with the scrollView (touch/mouse)
	void setScrolling (bool begin);		///< scrolling: manipulation or animated scrolling afterwards

protected:
	View* clipView;
	View* target;
	View* header;
	ScrollBar* vBar;
	ScrollBar* hBar;
	IParameter* vParam;
	IParameter* hParam;
	Point snap;
	mutable Coord scrollBarSize;
	mutable Coord scrollButtonSize;
	mutable Coord scrollButtonSpacing;
	mutable Coord borderSize;
	SharedPtr<VisualStyle> hBarStyle;
	SharedPtr<VisualStyle> vBarStyle;
	Point savedTargetSize;
	Point savedScrollPos;
	Rect makeVisibleRect;
	float relativeResizeRatio;
	bool scrollWheelLatched;
	double lastScrollWheelEventTime;

	friend class ScrollAnimationCompletionHandler;
	
	enum PrivateFlags
	{
		kResizingTarget	= 1<<(kLastPrivateFlag + 1),
		kManipulating	= 1<<(kLastPrivateFlag + 2),
		kAnimatingX 	= 1<<(kLastPrivateFlag + 3),
		kAnimatingY		= 1<<(kLastPrivateFlag + 4),
		kSyncingAnimation = 1<<(kLastPrivateFlag + 5),
		kSimulateLayeredScroll = 1<<(kLastPrivateFlag + 6)
	};
	PROPERTY_FLAG (privateFlags, kSimulateLayeredScroll, simulateLayeredScroll)

	void construct ();
	bool isConstructed () const;
	void addHScrollBar (RectRef rect);
	void addVScrollBar (RectRef rect);
	void removeHScrollBar ();
	void removeVScrollBar ();
	void stopVerticalAnimation ();
	void stopHorizontalAnimation ();

	Coord getScrollBarSize () const;
	Coord getScrollButtonSize () const;
	Coord getScrollButtonSpacing () const;
	Coord getBorderSize () const;
	Rect& calcClipRect (Rect& targetClip, Rect& headerClip) const;
	SizeLimit getClipViewLimits () const;
	void checkPosition (Rect& targetRect);
	void scrollClientInternal (RectRef rect, PointRef delta);
	void scrollClientToTargetRect (RectRef newTarget);

	IAttributeList* getViewState (tbool create);
	void savePosition ();
	void restorePosition ();

	void addScrollButtonUp (RectRef rect);
	void addScrollButtonDown (RectRef r);
	void addScrollButtonLeft (RectRef r);
	void addScrollButtonRight (RectRef r);
	void addScrollButton (RectRef rect, IParameter* param, VisualStyle* visualStyle, int orientation, int partCode, int sizemode);
	void removeScrollButtons (bool horizontal);

	void signalAnimation (StringID messageId, IParameter* param, IAnimation* animation);
	void synchronizeAnimation (ISubject* subject, MessageRef msg);

	void limitToScreenSize (Rect& scrollViewSize);
	void resizeTargetRelative (const Rect& rect);
	void checkClientSnapSize ();
	void onContinuousWheelEnded (Point& direction);
	
public:
	bool canScrollV () const;
	bool canScrollH () const;
	bool canScrollOmniDirectional () const;
	Point getScrollRange () const;

	// scroll speed factors (mousewheel) in pixels per scroll event unit
	virtual float getScrollSpeedV () const;
	virtual float getScrollSpeedH () const;

	virtual void drawBackground (const UpdateRgn& updateRgn);

	// IScrollView
	tresult CCL_API construct (IView* target) override;
	IView* CCL_API getTargetView () const override { return getTarget (); }

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

DECLARE_VISUALSTYLE_CLASS (ScrollViewStyle)

//////////////////////////////////////////////////////////////////////////////////////////////////
// ScrollView inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline View* ScrollView::getHeader () const				{ return header; }
inline View* ScrollView::getTarget () const				{ return target; }
inline void ScrollView::setHSnap (int snapH)			{ setSnap (Point (snapH, snap.y)); }
inline void ScrollView::setVSnap (int snapV)			{ setSnap (Point (snap.x, snapV)); }

//////////////////////////////////////////////////////////////////////////////////////////////////


//************************************************************************************************
// ScrollManipulator
//************************************************************************************************

class ScrollManipulator
{
public:
	ScrollManipulator (ScrollView* scrollView);
	
	void begin (PointRef where, bool force = false);
	void move (Point current);
	void end (float velocityX, float velocityY);
	void end (PointRef delta);
	void push (PointRef delta);
	
protected:
	ScrollView* scrollView;
	Point initialTargetPos;
	Point first;
	PointF smoothedPos;
	double duration;
	float velocity;
	int direction;
	
	ScrollView* getScrollView () const;
};

} // namespace CCL

#endif // _ccl_scrollview_h
