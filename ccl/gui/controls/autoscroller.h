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
// Filename    : ccl/gui/controls/autoscroller.h
// Description : AutosScroller
//
//************************************************************************************************

#ifndef _ccl_autoscroller_h
#define _ccl_autoscroller_h

#include "ccl/base/object.h"

#include "ccl/public/gui/graphics/rect.h"
#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/gui/framework/iscrollview.h"
#include "ccl/public/gui/framework/imousehandler.h"

namespace CCL {

class View;
class DragSession;

//************************************************************************************************
// AutoScroller
/** Manages automatic scrolling, at the borders of a scrollview */
//************************************************************************************************

class AutoScroller: public Object,
					public IdleClient,
					public IAutoScroller
{
public:
	DECLARE_CLASS (AutoScroller, Object)

	AutoScroller (View* view = nullptr);

	void setTargetView (View* view);
	void setDragSession (DragSession* session);

	PROPERTY_VARIABLE (Coord, innerMargin, InnerMargin)				///< where to start scrolling at the inner view borders
	PROPERTY_VARIABLE (Coord, outerMargin, OuterMargin)				///< where to reach the maximum scroll speed outside the view
	PROPERTY_VARIABLE (Coord, outerStartMargin, OuterStartMargin)	///< mouse must be inside this limit when starting after timeout (only for dragging)
	PROPERTY_VARIABLE (float, maxSpeed, MaxSpeed)					///< maximum scroll speed in pixels per second
	PROPERTY_VARIABLE (float, minSpeed, MinSpeed)					///< minimum scroll speed in pixels per second
	PROPERTY_VARIABLE (float, turboStartFactor, TurboStartFactor)	///< turbo mode start factor
	PROPERTY_VARIABLE (float, turboBoostFactor, TurboBoostFactor)	///< factor applied to turbo factor every 100ms
	PROPERTY_VARIABLE (float, turboMaxSpeed, turboMaxSpeed)			///< maximum speed in turbo mode

	void onMouseMove (int flags); ///< triggered from outside
	bool isScrolling ();
	View* getScrollView ();

	// IdleClient
	void onIdleTimer () override;

	// IAutoScroller
	void CCL_API construct (IView* targetView) override;
	void CCL_API trigger (PointRef screenPos, int flags) override;

	CLASS_INTERFACE2 (ITimerTask, IAutoScroller, Object)

private:
	enum Timeouts
	{
		kStartTimeout  = 300,
		kScrollTimeout = 30,
		kTurboTimeout  = 100
	};

	enum State { kDisabled, kWaiting, kObserving, kScrolling };

	View* targetView;
	DragSession* dragSession;
	UnknownPtr<IScrollable> baseScrollable;

	State state;
	int64 lastTime;
	int64 nextTime;
	int64 nextTurboBoostTime;
	float turboFactor;
	int directionFlags;
	bool didScroll;
	bool inTryScrolling;
	Point lastPos;
	
	void enableTimer (bool state);
	Point getMousePos (); ///< in screen coords
	static float calcScrollFactor (Coord mouseDist, Coord range);
	IScrollable* getScrollFactors (float& x, float& y, PointRef mousePos);
	bool getScrollFactors (float& x, float& y, PointRef mousePos, IScrollable* scrollable);
	IScrollable* findScrollable ();
	void checkTurbo (float& factorX, float& factorY);
	float calcSpeed (float factor);
	void tryScrolling ();
	void tryStartScrolling ();
};

} // namespace CCL

#endif // _ccl_autoscroller_h
