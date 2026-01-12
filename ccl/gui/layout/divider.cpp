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
// Filename    : ccl/gui/layout/divider.cpp
// Description : Divider
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/layout/divider.h"
#include "ccl/gui/layout/anchorlayout.h"
#include "ccl/gui/layout/directions.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/theme/renderer/dividerrenderer.h"
#include "ccl/gui/layout/dividergroup.h"
#include "ccl/gui/layout/layoutprimitives.h"

#include "ccl/base/boxedtypes.h"

#include "ccl/public/gui/framework/controlsignals.h"
#include "ccl/public/gui/framework/viewfinder.h"

#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// Divider::LayoutContext
//************************************************************************************************

struct Divider::Context
{
	AnchorLayoutView* layoutView;
	View* leftView;
	View* rightView;
	AnchorLayoutItem* leftItem;
	AnchorLayoutItem* rightItem;

	Context ()
	: layoutView (nullptr), leftView (nullptr), rightView (nullptr), leftItem (nullptr), rightItem (nullptr)
	{}

	template<class Direction>
	bool find (Divider& divider)
	{
		if(layoutView = divider.findLayoutContext<Direction> (leftView, rightView))
		{
			leftItem = static_cast<AnchorLayoutItem*> (layoutView->findLayoutItem (leftView));
			rightItem = static_cast<AnchorLayoutItem*> (layoutView->findLayoutItem (rightView));
			if(leftItem && rightItem)
			{
				leftItem->updateSizeLimits ();
				rightItem->updateSizeLimits ();
				return true;
			}
		}
		return false;
	}

	inline bool isValid () const { return leftItem && rightItem; }
};

//************************************************************************************************
// Divider::DividerMouseHandler
//************************************************************************************************

template<int direction> 
class Divider::DividerMouseHandler: public MouseHandler
{
public:
	DividerMouseHandler (Divider* divider, PointRef clickOffset)
	: MouseHandler (divider, kAutoScroll),
	  clickOffset (clickOffset),
	  divider (divider),
	  pushDirection (0)
	{}

	// MouseHandler
	void onBegin () override
	{
		divider->getParameter ()->beginEdit ();
		divider->setMouseState (View::kMouseDown);
	}

	void onRelease (bool canceled) override
	{
		divider->setMouseState (View::kMouseNone);
		divider->getParameter ()->endEdit ();
	}

	bool onMove (int moveFlags) override
	{
		typedef DirectionTraits<direction> Direction;

		Point oldPos (divider->getSize ().getLeftTop ());
		Point p (current.where - clickOffset);
		p += divider->getSize ().getLeftTop (); // to parent coords

		divider->moveTo (Direction::getCoord (p));

		if(divider->getStyle ().isCustomStyle (Styles::kDividerBehaviorPush))
		{
			int mouseDirection = ccl_sign (Direction::getCoord (current.where - first.where));

			Point mouseDelta (current.where - previous.where);
			Point dividerDelta (divider->getSize ().getLeftTop () - oldPos);

			Coord possiblePush = 0;
			Divider::Context context;
			if(context.find<Direction> (*divider))
			{
				// the limits of the view on the other side of this divider determine the maximum push amount
				// (don't push the other divider further then this one can move)
				if(mouseDirection > 0)
					possiblePush = Direction::getMax (context.leftItem->sizeLimits) - Direction::getLength (context.leftView->getSize ());
				else
					possiblePush = Direction::getMax (context.rightItem->sizeLimits) - Direction::getLength (context.rightView->getSize ());
			}

			if(Direction::getCoord (dividerDelta) == 0 && Direction::getCoord (mouseDelta) != 0) // mouse moved but divider couldn't
			{
				if(!pushDivider && possiblePush > 0)
				{
					pushDivider = findPushDivider (mouseDirection);
					if(pushDivider)
					{
						pushOffset = Point ();
						pushDivider->clientToWindow (pushOffset);
						divider->windowToClient (pushOffset);

						pushStartPosition = pushDivider->getSize ().getLeftTop ();
						pushDirection = mouseDirection;
						CCL_PRINTF ("Begin push divider at %d, offset %d\n", Direction::getCoord (pushStartPosition), Direction::getCoord (pushOffset))
					}
				}
			}
			else if(pushDivider)
			{
				// stop pushing when reaching the start position again
				Coord pushed = Direction::getCoord (pushDivider->getSize ().getLeftTop () - pushStartPosition);
				if(ccl_sign (pushed) != pushDirection)
					pushDivider.release ();
			}

			if(pushDivider && possiblePush > 0)
			{
				Point pushPosition (p + pushOffset);
				
				if(pushDirection > 0)
					ccl_upper_limit (Direction::getCoord (pushPosition), Direction::getCoord (pushDivider->getSize ().getLeftTop ()) + possiblePush);
				else
					ccl_lower_limit (Direction::getCoord (pushPosition), Direction::getCoord (pushDivider->getSize ().getLeftTop ()) - possiblePush);

				pushDivider->moveTo (Direction::getCoord (pushPosition));

				// try again after push
				divider->moveTo (Direction::getCoord (p));
			}
		}

		Window::UpdateCollector uc (divider->getWindow ());
		System::GetSignalHandler ().flush (); // keep dependent dividers in sync
		return true;
	}

protected:
	SharedPtr<Divider> divider;
	SharedPtr<Divider> pushDivider;
	Point clickOffset;
	Point pushOffset;
	Point pushStartPosition;
	int pushDirection;

	Divider* findPushDivider (int searchDirection) const
	{
		if(Window* window = divider->getWindow ())
		{
			// find other divider in same window in search direction
			Point p;
			divider->clientToWindow (p);

			constexpr Coord kMaxDistance = 30;

			for(int i = 0; i< kMaxDistance; i++)
			{
				DirectionTraits<direction>::getCoord (p) += searchDirection;

				View* candidate = window->findView (p, true);
				while(candidate)
				{
					auto* pushDivider = ccl_cast<Divider> (candidate);
					if(pushDivider && pushDivider != divider && pushDivider->getStyle ().isHorizontal () == divider->getStyle ().isHorizontal ())
						return pushDivider;

					// try other views covered by the first found view
					candidate = unknown_cast<View> (ViewFinder (candidate).findNextView (window, p));
				}
			}
		}
		return nullptr;
	}
};

//************************************************************************************************
// Divider::Limits
//************************************************************************************************

struct Divider::Limits: public Object
{
	DECLARE_CLASS (Limits, Object)
	
	Coord minDiff;
	Coord maxDiff;
	Divider* invoker;

	inline Limits (Divider* invoker = nullptr)
	: invoker (invoker), minDiff (kMinCoord), maxDiff (kMaxCoord)
	{}

	bool isValid () const { return minDiff <= maxDiff;}
	bool canMove () const { return minDiff < maxDiff;}

	inline void include (Divider& divider)
	{
		if(divider.getStyle ().isCommonStyle (Styles::kVertical))
			include<VerticalDirection> (divider);
		else
			include<HorizontalDirection> (divider);
	}

	template<class Direction>
	inline void include (Divider& divider)
	{
		Context context;
		if(context.find<Direction> (divider))
			include<Direction> (context);
	}

	template<class Direction>
	void include (const Context& context)
	{
		Coord leftLen = Direction::getLength (context.leftView->getSize ());
		Coord rightLen = Direction::getLength (context.rightView->getSize ());

		// determine the acceptable range of diff from the min & max sizes of both views
		ccl_lower_limit (minDiff, Direction::getMin (context.leftItem->sizeLimits) - leftLen);
		ccl_upper_limit (maxDiff, Direction::getMax (context.leftItem->sizeLimits) - leftLen);
		ccl_lower_limit (minDiff, rightLen - Direction::getMax (context.rightItem->sizeLimits));
		ccl_upper_limit (maxDiff, rightLen - Direction::getMin (context.rightItem->sizeLimits));
	}
};

DEFINE_CLASS_HIDDEN (Divider::Limits, Object)

//************************************************************************************************
// Divider::PreferredSizes
//************************************************************************************************

struct Divider::PreferredSizes: public Object
{
	DECLARE_CLASS (PreferredSizes, Object)
	
	Point topLeft;
	Point bottomRight;
};	

DEFINE_CLASS_HIDDEN (Divider::PreferredSizes, Object)

//************************************************************************************************
// Divider
//************************************************************************************************

BEGIN_STYLEDEF (Divider::customStyles)
	{"master",	Styles::kDividerBehaviorMaster},
	{"slave",	Styles::kDividerBehaviorSlave},
	{"reverse",	Styles::kDividerBehaviorReverse},
	{"outreachleft",	Styles::kDividerBehaviorOutreachLeft},
	{"outreachright",	Styles::kDividerBehaviorOutreachRight},
	{"outreachtop",		Styles::kDividerBehaviorOutreachTop},
	{"outreachbottom",	Styles::kDividerBehaviorOutreachBottom},
	{"childrenontop",	Styles::kDividerBehaviorChildrenOnTop},
	{"push",			Styles::kDividerBehaviorPush},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (Divider, Control)
DEFINE_CLASS_UID (Divider, 0xb64968e8, 0x151c, 0x4053, 0x96, 0xe3, 0xba, 0x2b, 0x7b, 0xb, 0x77, 0xd)
DEFINE_STRINGID_MEMBER_ (Divider, kQueryDividerLimits, "queryDividerLimits")
DEFINE_STRINGID_MEMBER_ (Divider, kQueryPreferredSizes, "queryPreferredSizes")
DEFINE_STRINGID_MEMBER_ (Divider, kHasLayoutState, "hasLayoutState")
DEFINE_STRINGID_MEMBER_ (Divider, kSyncSlaves, "syncSlaves")

//////////////////////////////////////////////////////////////////////////////////////////////////

Divider::Divider (const Rect& size, IParameter* param, StyleRef style)
: Control (size, param ? param : AutoPtr<IntParam> (NEW IntParam (0, kMaxCoord)), style)
{
	outreach = getTheme ().getThemeMetric (ThemeElements::kDividerOutreach);

	if(auto dp = unknown_cast<DividerGroups::DividerParam> (param))
		dp->setDividerConnected (true); 
	
	ignoresFocus (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* Divider::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kDividerRenderer, visualStyle);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* Divider::createMouseHandler (const MouseEvent& event)
{
	if(detectDoubleClick (event))
	{
		UnknownPtr<IObserver> controller (param ? param->getController () : nullptr);
		if(controller)
		{
			// first give controller a chance to handle the gesture
			Boxed::Variant result;
			Message msg (Signals::kDividerDoubleClick, param, static_cast<IVariant*> (&result));
			controller->notify (this, msg);
			if(result.asVariant ().asBool ())
				return NEW NullMouseHandler (this);
		}

		// quick fix: other dividers are not synced in onMove due to the kMouseDown check
		if(style.isCustomStyle (Styles::kDividerBehaviorSlave))
			return NEW NullMouseHandler (this);

		// jump to extreme positions on double click; default: up/left, toggle with shift; center with command
		int direction = event.keys.isSet (KeyState::kCommand) ? 0 : (event.keys.isSet (KeyState::kShift) ? 1 : -1);
		jump (direction, true);

		return NEW NullMouseHandler (this);
	}

	MouseHandler* handler = nullptr;
	if(getStyle ().isCommonStyle (Styles::kVertical))
		handler = NEW DividerMouseHandler<Styles::kVertical> (this, event.where);
	else
		handler = NEW DividerMouseHandler<Styles::kHorizontal> (this, event.where);
	return handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Divider::onMouseEnter (const MouseEvent& event)
{
	if(!canResizeViews ())
		return false;

	setMouseState (kMouseOver);
	return onMouseMove (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Divider::onMouseMove (const MouseEvent& event)
{
	if(!getStyle ().isCustomStyle (Styles::kDividerBehaviorChildrenOnTop) || getChildren ().findChildView (event.where) == nullptr)
	{
		ThemeCursorID cursor = getStyle ().isCommonStyle (Styles::kVertical) ? ThemeElements::kSizeVerticalCursor : ThemeElements::kSizeHorizontalCursor;
		setCursor (getTheme ().getThemeCursor (cursor));
	}
	else
		setCursor ((MouseCursor*)nullptr);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Divider::onMouseLeave (const MouseEvent& event)
{
	setMouseState (kMouseNone);
	setCursor ((MouseCursor*)nullptr);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord Divider::getPosition () const
{
	return getStyle ().isCommonStyle (Styles::kVertical) ? getSize ().top : getSize ().left;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Divider::positionToValue (Coord position) const
{
	if(getStyle ().isCustomStyle (Styles::kDividerBehaviorReverse) && parent)
	{
		Coord parentSize = getStyle ().isCommonStyle (Styles::kVertical) ? parent->getSize ().getHeight () : parent->getSize ().getWidth ();
		return parentSize - position;
	}
	return position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord Divider::valueToPosition (int value) const
{
	if(getStyle ().isCustomStyle (Styles::kDividerBehaviorReverse) && parent)
	{
		Coord parentSize = getStyle ().isCommonStyle (Styles::kVertical) ? parent->getSize ().getHeight () : parent->getSize ().getWidth ();
		return parentSize - value;
	}
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Divider::triggerSyncSlaves ()
{
	(NEW Message (kSyncSlaves, Variant (this->asUnknown (), true)))->post (this, -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Divider::canResizeViews () const
{
	Context context;
	Limits limits (ccl_const_cast (this));
	if(getStyle ().isCommonStyle (Styles::kVertical))
	{
		if(context.find<VerticalDirection> (ccl_const_cast (*this)))
		{
			limits.include<VerticalDirection> (context);
			return limits.canMove ();
		}
	}
	else
	{
		if(context.find<HorizontalDirection> (ccl_const_cast (*this)))
		{
			limits.include<HorizontalDirection> (context);
			return limits.canMove ();
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Divider::attached (View* parent)
{
	CCL_PRINTF ("Divider %s attached: pos %d (param %d)\n", MutableCString (name).str (), getPosition (), param->getValue ().asInt ());

	if(style.isCustomStyle (Styles::kDividerBehaviorMaster))
		triggerSyncSlaves ();

	else if(style.isCustomStyle (Styles::kDividerBehaviorSlave))
	{
		ScopedFlag<kIsSyncing> syncing (privateFlags);
		moveTo (valueToPosition (param->getValue ()));
	}
	SuperClass::attached (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Divider::onMove (const Point& delta)
{
	CCL_PRINTF ("Divider %s onMove: delta (%d, %d) -> %d\n", MutableCString (name).str (), delta.x, delta.y, getPosition ());

	if(param && (!style.isCustomStyle (Styles::kDividerBehaviorSlave) || getMouseState () == kMouseDown))
		param->setValue (positionToValue (getPosition ()), true);

	SuperClass::onMove (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Divider::onSize (const Point& delta)
{
	SuperClass::onSize (delta);

	if(getRenderer ()->needsRedraw (this, delta))
		invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Divider::paramChanged ()
{
	CCL_PRINTF ("Divider %s paramChanged: %d (pos: %d)\n", MutableCString (name).str (), param->getValue ().asInt (), getPosition ());
	ScopedFlag<kIsSyncing> syncing (privateFlags);

	moveTo (valueToPosition (param->getValue ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Divider::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IParameter::kEndEdit)
		onManipulationDone ();
	else if(msg == kQueryDividerLimits)
	{
		if(Limits* limits = unknown_cast<Limits> (msg[0].asUnknown ()))
			if(limits->invoker != this)
				if(isAttached ())
					limits->include (*this);
	}
	else if(msg == kQueryPreferredSizes)
	{
		if(PreferredSizes* sizes = unknown_cast<PreferredSizes> (msg[0].asUnknown ()))
		{
			Context context;			
			if(getStyle ().isCommonStyle (Styles::kVertical) ? context.find<VerticalDirection> (*this) : context.find<HorizontalDirection> (*this))
			{
				sizes->topLeft = context.leftItem->preferredSize;
				sizes->bottomRight = context.rightItem->preferredSize;
			}
		}
	}
	else if(msg == kSyncSlaves)
	{
		UnknownPtr<ISubject> subject (param);
		System::GetSignalHandler ().performSignal (subject, Message (kChanged));
	}
	else if(msg == kHasLayoutState)
	{
		if(AnchorLayoutView* layoutView = ccl_cast<AnchorLayoutView> (parent))
			if(bool* hasLayoutState = reinterpret_cast<bool*> (msg[0].asIntPointer ()))
				*hasLayoutState = layoutView->hasSavedState ();
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Divider::onManipulationDone ()
{
	View* leftView = nullptr;
	View* rightView = nullptr;

	AnchorLayoutView* layoutView = getStyle ().isCommonStyle (Styles::kVertical)
		? findLayoutContext<VerticalDirection> (leftView, rightView)
		: findLayoutContext<HorizontalDirection> (leftView, rightView);
	if(layoutView)
		layoutView->onManipulationDone ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Divider::moveTo (Coord newPos)
{
	Coord oldPos = getPosition ();
	moveBy (newPos - oldPos);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Divider::moveBy (Coord offset)
{
	if(getStyle ().isCommonStyle (Styles::kVertical))
		moveBy<VerticalDirection> (offset);
	else
		moveBy<HorizontalDirection> (offset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Direction>
static inline bool isBoundEmpty (const SizeLimit& limits)
{
	return Direction::getMin (limits) == 0 && Direction::getMax (limits) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Direction>
AnchorLayoutView* Divider::findLayoutContext (View*& leftView, View*& rightView)
{
	leftView = rightView = nullptr;
	if(View* p = getParent ())
	{
		AnchorLayoutView* layoutView = ccl_cast<AnchorLayoutView> (p);
		if(layoutView)
		{
			// get the two neighbours in parent layout
			int index = layoutView->index (this);
			int leftIndex = index - 1; 
			int rightIndex = index + 1;

			leftView = layoutView->getChild (leftIndex);
			rightView = layoutView->getChild (rightIndex);

			// skip non-resizable neighbours (they will be moved)
			while(View* l = layoutView->getChild (leftIndex--))
			{
				// check if the view can be sized
				Coord leftLen = Direction::getLength (l->getSize ());
				Coord minDiff = ccl_max<Coord> (kMinCoord, Direction::getMin (l->getSizeLimits ()) - leftLen);
				Coord maxDiff = ccl_min<Coord> (kMaxCoord, Direction::getMax (l->getSizeLimits ()) - leftLen);
				if(minDiff < maxDiff)
				{
					leftView = l;
					break;
				}
			}

			while(View* r = layoutView->getChild (rightIndex++))
			{
				// check if the view can be sized
				Coord rightLen = Direction::getLength (r->getSize ());
				Coord minDiff = ccl_max<Coord> (kMinCoord, rightLen - Direction::getMax (r->getSizeLimits ()));
				Coord maxDiff = ccl_min<Coord> (kMaxCoord, rightLen - Direction::getMin (r->getSizeLimits ()));
				if(minDiff < maxDiff)
				{
					rightView = r;
					break;
				}
			}

			if(leftView && rightView)
				return layoutView;
		}
		else
		{
			// check if divider is first or last child in a simple view
			leftView = rightView = this; // just something != 0 (see below)
			if(this == p->getFirst ())
				leftView = nullptr; // search for a sibling left
			if(this == p->getLast ())
				rightView = nullptr; // search for a sibling right
		}

		// the divider is the first or last layout item, try to move parent & it's sibling in another layout (upwards)
		if(leftView == nullptr)
		{
			View* child = getParent ();
			while(View* parent = child->getParent ())
			{
				if(layoutView = ccl_cast<AnchorLayoutView> (parent))
				{
					rightView = child;

					// search for a right layout sibling that can be sized
					for(int index = layoutView->index (child) - 1; leftView = layoutView->getChild (index); index--)
						if(leftView
							&& !isBoundEmpty<Direction> (leftView->getSizeLimits ())
							&& LayoutPrimitives::isSizeMode<Direction::kAttachStart|Direction::kAttachEnd> (leftView))
							return layoutView;
				}
				child = parent;
			}
		}
		if(rightView == nullptr)
		{
			View* child = getParent ();
			while(View* parent = child->getParent ())
			{
				if(layoutView = ccl_cast<AnchorLayoutView> (parent))
				{
					leftView  = child;

					// search for a left layout sibling that can be sized
					for(int index = layoutView->index (child) + 1; rightView = layoutView->getChild (index); index++)
						if(rightView
							&& !isBoundEmpty<Direction> (rightView->getSizeLimits ())
							&& LayoutPrimitives::isSizeMode<Direction::kAttachStart|Direction::kAttachEnd> (rightView))
							return layoutView;

				}
				child = parent;
			}
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Direction>
void Divider::moveBy (Coord offset)
{
	Limits limits (this);
	ASSERT (limits.isValid ())
	Context context;
	if(context.find<Direction> (*this))
	{
		limits.include<Direction> (context); // always get own limits
		ASSERT (limits.isValid ())
		
		auto group = unknown_cast<DividerGroups::Group> (param->getController ());
		bool noGroupButSyncing = (group == nullptr) && ((privateFlags & kIsSyncing) == 0);  
		auto dividerParam = unknown_cast<DividerGroups::DividerParam> (param);
		auto alignmentParam = (group && dividerParam && !dividerParam->isInitialized ()) ? group->getAlignmentParam () : nullptr;

		// new dividers of a group can get out of sync on resize, because their workrects might differ. 
		// We need to copy the preferred size from the alignment dividers layoutView
		bool shouldCopyPreferredSizes = (alignmentParam != nullptr);
		PreferredSizes alignmentSizes;
		
		if(shouldCopyPreferredSizes)
		{
			UnknownPtr<ISubject> paramSubject (alignmentParam->asUnknown ());
			if(paramSubject)
				System::GetSignalHandler ().performSignal (paramSubject, (Message (kQueryPreferredSizes, alignmentSizes.asUnknown ())));
		}
		else if(noGroupButSyncing) // include limits from other dividers synced via the same parameter
		{
			UnknownPtr<ISubject> paramSubject (param);
			if(paramSubject)
				System::GetSignalHandler ().performSignal (paramSubject, (Message (kQueryDividerLimits, limits.asUnknown ())));
		}

		CCL_PRINTF ("Divider::moveBy %d (min = %d, max = %d)\n", offset, limits.minDiff, limits.maxDiff);

		if((offset != 0) && limits.isValid ())
		{
			offset = ccl_bound (offset, limits.minDiff, limits.maxDiff);
			if(offset != 0)
			{
				Rect leftRect  (context.leftView->getSize ());
				Rect rightRect (context.rightView->getSize ());

				Direction::getEndCoord (leftRect)    += offset;
				Direction::getStartCoord (rightRect) += offset;

				// freeze current sizes as the preferred ones (since the user just intervened)
				context.layoutView->makeCurrentSizesPreferred ();
				
				// temporarily disable fitsize for the layout view
				int sizeMode = context.layoutView->getSizeMode ();
				context.layoutView->setSizeMode (sizeMode &~ kFitSize);

				context.leftItem->preferredSize (leftRect.getWidth (), leftRect.getHeight ());
				context.rightItem->preferredSize (rightRect.getWidth (), rightRect.getHeight ());
				context.leftView->setSize (leftRect);
				context.rightView->setSize (rightRect);

				context.layoutView->setSizeMode (sizeMode);
			}
		}
		
		if(shouldCopyPreferredSizes)
		{
			context.leftItem->preferredSize (alignmentSizes.topLeft.x, alignmentSizes.topLeft.y);
			context.rightItem->preferredSize (alignmentSizes.bottomRight.x, alignmentSizes.bottomRight.y);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Divider::jump (int direction, bool canInvert)
{
	Coord delta = 0;
	
	if(direction == 0)
	{
		// center (of possible range, not necessarily equal size for both sides)
		Limits limits (this);
		limits.include (*this);
		delta = (limits.minDiff + limits.maxDiff) / 2;
	}
	else
	{
		// extreme position
		delta = direction > 0 ? NumericLimits::kMaxInt : -NumericLimits::kMaxInt;
	}

	Coord oldPos = getPosition ();
	moveBy (delta);

	// try other direction if nothing happened
	if(canInvert && getPosition () == oldPos)
		moveBy (-delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Divider)
	DEFINE_METHOD_NAME ("jump")
END_METHOD_NAMES (Divider)

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Divider::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "jump")
	{
		int direction = msg[0].asInt ();
		bool canInvert = msg.getArgCount () > 1 ? msg[1].asBool () : false;
		jump (direction, canInvert);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
