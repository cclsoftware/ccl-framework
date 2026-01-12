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
// Filename    : core/gui/coregesturerecognition.impl.h
// Description : Platform-independent gesture recognition
//
//************************************************************************************************

#include "core/gui/coregesturerecognition.h"

#include "core/public/corestringbuffer.h"

#include <math.h>

#define RECOGNIZER_POOL_SIZE 16
#define DEFINE_RECOGNIZERPOOL(Class) \
typedef RecognizerPool<Class, RECOGNIZER_POOL_SIZE> Class##Pool; \
typedef PooledRecognizer<Class, Class##Pool, RECOGNIZER_POOL_SIZE> Pooled##Class; \
template<> Class##Pool Pooled##Class::pool = Class##Pool (); \
static FixedSizeVector<Pooled##Class, RECOGNIZER_POOL_SIZE> Class##Pool##instance;

namespace Core {

//************************************************************************************************
// GestureInfo
//************************************************************************************************

CStringPtr GestureInfo::getGestureName (const GestureInfo* info)
{
	return getGestureName (info->getType ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr GestureInfo::getGestureName () const
{
	return getGestureName (getType ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr GestureInfo::getGestureName (int type)
{
	static CStringBuffer<128> name;

	switch(type & kGestureTypeMask)
	{
	case kGestureSwipe: name = "Swipe"; break;
	case kGestureZoom: name = "Zoom"; break;
	case kGestureRotate: name = "Rotate"; break;
	case kGestureLongPress: name = "LongPress"; break;
	case kGestureSingleTap: name = "Tap"; break;
	case kGestureDoubleTap: name = "Double Tap"; break;
	}
	switch(type & kGestureConstraintsMask)
	{
	case kGestureHorizontal : name += " horizontal"; break;
	case kGestureVertical: name += " vertical"; break;
	}
	switch(type & kGestureStatesMask)
	{
	case kGestureBegin : name += " (begin)"; break;
	case kGestureChanged : name += " (changed)"; break;
	case kGestureEnd : name += " (end)"; break;
	case kGestureFailed : name += " (failed)"; break;
	case kGesturePossible : name += " (possible)"; break;
	}
	return name;
}

//************************************************************************************************
// GestureRecognition::Recognizer
//************************************************************************************************

class GestureRecognition::Recognizer
{
public:
	Recognizer (GestureInfo* info = nullptr);
	virtual ~Recognizer () {}
	
	int getType () const;
	GestureInfo* getGestureInfo () const;
	
	virtual bool addTouch (const TouchInfo& touchInfo);
	virtual bool removeTouch (const TouchInfo& touchInfo);
	virtual bool changeTouch (const TouchInfo& touchInfo);

	void reset (GestureInfo* info);
	void setMinimumTime (int time);
	
	GestureEventType getInternalState () const;
	GestureEventType getExternalState () const;
	void setExternalState (GestureEventType state);	
	abs_time getTimeGestureStarted () const;
	bool miniumTimeElapsed (abs_time now) const;
	virtual GestureEventArgs getEventArgs () const;
	
	virtual void release () { delete this; }
	
	static const int64 maxTimeDoubleTap;
	static const CoordF maxDistDoubleTap;
	
protected:
	GestureInfo* gestureInfo;
	LinkedList<TouchInfo> touches;
	GestureEventType internalState;
	GestureEventType externalState;
	int64 minimumTime;
	int64 timeGestureStarted;
	
	PointF getCenter () const;
};

//************************************************************************************************
// SwipeRecognizer
//************************************************************************************************

class SwipeRecognizer: public GestureRecognition::Recognizer
{
public:
	SwipeRecognizer (GestureInfo* info = nullptr);
	
	// Recognizer
	bool addTouch (const TouchInfo& touchInfo) override;
	bool changeTouch (const TouchInfo& touchInfo) override;
	bool removeTouch (const TouchInfo& touchInfo) override;
	
	GestureEventArgs getEventArgs () const override;
	
protected:
	PointF lastPoint;
	abs_time lastPointTime;
	PointF velocity;
	FixedSizeVector<PointF, 3> lastPositions;
	int lastPositionIndex;
	
	Coord margin;
};

//************************************************************************************************
// ZoomRecognizer
//************************************************************************************************

class ZoomRecognizer: public GestureRecognition::Recognizer
{
public:
	ZoomRecognizer (GestureInfo* info = nullptr);
	
	// Recognizer
	bool addTouch (const TouchInfo& touchInfo) override;
	bool changeTouch (const TouchInfo& touchInfo) override;
	bool removeTouch (const TouchInfo& touchInfo) override;
	
	GestureEventArgs getEventArgs () const override;
	
protected:
	float initialDistance;
	float margin;
	mutable PointF center;
	mutable float amount;
	static float getDistance (const TouchInfo& touch1, const TouchInfo& touch2);
};

//************************************************************************************************
// RotateRecognizer
//************************************************************************************************

class RotateRecognizer: public GestureRecognition::Recognizer
{
public:
	RotateRecognizer (GestureInfo* info = nullptr);
	
	// Recognizer
	bool addTouch (const TouchInfo& touchInfo) override;
	bool changeTouch (const TouchInfo& touchInfo) override;
	bool removeTouch (const TouchInfo& touchInfo) override;
	
	GestureEventArgs getEventArgs () const override;
	
protected:
	float initialAngle;
	float margin;	
	static float getAngle (const TouchInfo& touch1, const TouchInfo& touch2);
};

//************************************************************************************************
// LongPressRecognizer
//************************************************************************************************

class LongPressRecognizer: public GestureRecognition::Recognizer
{
public:
	LongPressRecognizer (GestureInfo* info = nullptr);
	
	// Recognizer
	bool addTouch (const TouchInfo& touchInfo) override;
	bool changeTouch (const TouchInfo& touchInfo) override;
	bool removeTouch (const TouchInfo& touchInfo) override;
	
	GestureEventArgs getEventArgs () const override;
};

//************************************************************************************************
// SingleTapRecognizer
//************************************************************************************************

class SingleTapRecognizer: public GestureRecognition::Recognizer
{
public:
	SingleTapRecognizer (GestureInfo* info = nullptr);
	
	// Recognizer
	bool addTouch (const TouchInfo& touchInfo) override;
	bool changeTouch (const TouchInfo& touchInfo) override;
	bool removeTouch (const TouchInfo& touchInfo) override;
	
	GestureEventArgs getEventArgs () const override;
};

//************************************************************************************************
// DoubleTapRecognizer
//************************************************************************************************

class DoubleTapRecognizer: public GestureRecognition::Recognizer
{
public:
	DoubleTapRecognizer (GestureInfo* info = nullptr);
	
	// Recognizer
	bool addTouch (const TouchInfo& touchInfo) override;
	bool removeTouch (const TouchInfo& touchInfo) override;
	
	GestureEventArgs getEventArgs () const override;

private:
	int numTaps;
};

//************************************************************************************************
// RecognizerPool
//************************************************************************************************

template <class T, int size> class RecognizerPool 
{	
public:
	T* pop ()
	{
		if(T* recognizer = recognizers.first ())
		{
			recognizers.removeFirst ();
			return recognizer;
		}
		else
			return nullptr;
	}
			
	void push (GestureRecognition::Recognizer* recognizer)
	{
		recognizers.add ((T*)recognizer);
	}	
	
protected:
	FixedSizeVector<T*, size> recognizers;
};
	
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T, class Pool, int size> class PooledRecognizer: public T
{
public:
	PooledRecognizer ()
	{
		pool.push (this);
	}

	static T* pool_new ()
	{
		return pool.pop ();
	}

	void release () override
	{
		pool.push (this);
	}
	
private:
	static Pool pool;
};

DEFINE_RECOGNIZERPOOL (SwipeRecognizer)
DEFINE_RECOGNIZERPOOL (ZoomRecognizer)
DEFINE_RECOGNIZERPOOL (RotateRecognizer)	
DEFINE_RECOGNIZERPOOL (LongPressRecognizer)
DEFINE_RECOGNIZERPOOL (SingleTapRecognizer)
DEFINE_RECOGNIZERPOOL (DoubleTapRecognizer)

//************************************************************************************************
// GestureRecognition::Recognizer
//************************************************************************************************

const int64 GestureRecognition::Recognizer::maxTimeDoubleTap = 500;
const CoordF GestureRecognition::Recognizer::maxDistDoubleTap = 50;

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureRecognition::Recognizer::Recognizer (GestureInfo* info)
: minimumTime (0)
{
	reset (info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GestureRecognition::Recognizer::reset (GestureInfo* info)
{
	gestureInfo = info;
	internalState = kGesturePossible;
	externalState = kGesturePossible;
	timeGestureStarted = 0;
	touches.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GestureRecognition::Recognizer::setMinimumTime (int time)
{
	minimumTime = time;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int GestureRecognition::Recognizer::getType () const
{
	return gestureInfo->getType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureInfo* GestureRecognition::Recognizer::getGestureInfo () const
{
	return gestureInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GestureRecognition::Recognizer::addTouch (const TouchInfo& touchInfo)
{
	if(!touches.contains (touchInfo))
	{	
		touches.append (touchInfo);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GestureRecognition::Recognizer::removeTouch (const TouchInfo& touchInfo)
{
	return touches.remove (touchInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GestureRecognition::Recognizer::changeTouch (const TouchInfo& touchInfo)
{
	ListIterator<TouchInfo> iter = ListIterator<TouchInfo> (touches);
	while(!iter.done ())
		if(iter.next () == touchInfo)
		{
			touches.replace (iter, touchInfo);
			return  true;
		}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureEventType GestureRecognition::Recognizer::getInternalState () const
{
	return internalState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////


GestureEventType GestureRecognition::Recognizer::getExternalState () const
{
	return externalState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GestureRecognition::Recognizer::setExternalState (GestureEventType state)
{
	externalState = state;
	internalState = externalState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

abs_time GestureRecognition::Recognizer::getTimeGestureStarted () const
{
	return timeGestureStarted;
}	

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GestureRecognition::Recognizer::miniumTimeElapsed (abs_time now) const
{
	return ((now - timeGestureStarted) > minimumTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointF GestureRecognition::Recognizer::getCenter () const
{
	PointF center;
	int numTouches = touches.count ();
	if(numTouches == 0)
		return center;
	if(numTouches == 1)
		return touches.getFirst ().whereF;
	
	ListIterator<TouchInfo> iter = ListIterator<TouchInfo> (touches);
	while(!iter.done ())
		center += iter.next().whereF;
	center *= 1.f / numTouches;
	
	return center;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureEventArgs GestureRecognition::Recognizer::getEventArgs () const
{
	return  GestureEventArgs ();
}

//************************************************************************************************
// SwipeRecognizer
//************************************************************************************************

SwipeRecognizer::SwipeRecognizer (GestureInfo* info)
: Recognizer (info),
  lastPointTime (0),
  margin (10),
  lastPositionIndex (0)
{
	minimumTime  = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SwipeRecognizer::addTouch (const TouchInfo& touchInfo)
{
	if(touches.count () == 0 && internalState == kGesturePossible)
	{
		Recognizer::addTouch (touchInfo);
		lastPoint = touchInfo.whereF;
		lastPointTime = touchInfo.time;	
		lastPositions.removeAll ();
		lastPositionIndex = 0;
		velocity = PointF ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SwipeRecognizer::changeTouch (const TouchInfo& touchInfo)
{
	bool result = Recognizer::changeTouch (touchInfo);
	if(result)
	{	
		if(internalState == kGesturePossible)
		{		
			bool swipeH = fabs (touchInfo.whereF.x - lastPoint.x) > margin;
			bool swipeV = fabs (touchInfo.whereF.y - lastPoint.y) > margin;
			if(swipeH || swipeV)
			{
				internalState = kGestureBegin;
				timeGestureStarted = touchInfo.time;
			}
		}
		else if(internalState == kGestureBegin && externalState == kGestureBegin)
			internalState = kGestureChanged;
		
		if(internalState != kGesturePossible)
		{
			abs_time deltaT = (touchInfo.time - lastPointTime);
			PointF deltaP (float (touchInfo.whereF.x - lastPoint.x), float (touchInfo.whereF.y - lastPoint.y));
			if(deltaT != 0)
			{
				velocity = deltaP * (1000.f / deltaT);

				if(fabsf (deltaP.x) < 1)
					velocity.x = 0;
				if(fabsf (deltaP.y) < 1)
					velocity.y = 0;
			}

			// keep a history of last positions
			if(lastPositions.isFull ())
			{
				if(lastPositionIndex >= lastPositions.getCapacity ())
					lastPositionIndex = 0;

				lastPositions[lastPositionIndex++] = touchInfo.whereF;
			}
			else
				lastPositions.add (touchInfo.whereF);

			lastPoint = touchInfo.whereF;
			lastPointTime = touchInfo.time;
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SwipeRecognizer::removeTouch (const TouchInfo& touchInfo)
{
	if(touches.contains (touchInfo))
	{	
		if(externalState != kGesturePossible)
			internalState = kGestureEnd;
		else
			internalState = kGestureFailed;

		// reset velocity if recent positions were inside a tolerance
		static const CoordF kTolerance = 4;
		bool hasMovedX = false;
		bool hasMovedY = false;
				
		VectorForEach (lastPositions, PointF, p)
			if(fabs (touchInfo.whereF.x - p.x) > kTolerance)
				hasMovedX = true;
			if(fabs (touchInfo.whereF.y - p.y) > kTolerance)
				hasMovedY = true;
		EndFor

		if(!hasMovedX)
			velocity.x = 0;

		if(!hasMovedY)
			velocity.y = 0;

		lastPoint.x = lastPoint.y = 0; 
		lastPointTime = 0;
		timeGestureStarted = 0;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureEventArgs SwipeRecognizer::getEventArgs () const
{
	return GestureEventArgs (getCenter (), velocity.x, velocity.y);
}

//************************************************************************************************
// ZoomRecognizer
//************************************************************************************************
	
ZoomRecognizer::ZoomRecognizer (GestureInfo* info)
: Recognizer (info),
  initialDistance (0.f),
  margin (0.05f),
  amount (1.f)
{
	minimumTime = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZoomRecognizer::addTouch (const TouchInfo& touchInfo)
{
	bool result = false;
	if(touches.count () < 2)
	{
		result = Recognizer::addTouch (touchInfo);

		center = getCenter ();
		amount = 1.f;
	}
	if(touches.count () == 2)
		initialDistance = getDistance (touches.at (0), touches.at (1));
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZoomRecognizer::changeTouch (const TouchInfo& touchInfo)
{
	bool result = Recognizer::changeTouch (touchInfo);
	if(result)
	{	
		if(touches.count () == 2)
		{
			if(internalState == kGesturePossible)
			{
				if(initialDistance != 0.f)
					if(fabsf (1.f - (getDistance (touches.at (0), touches.at (1)) / initialDistance)) > margin)
					{
						internalState = kGestureBegin;
						timeGestureStarted = touchInfo.time;
					}
			}
		}	
		if(internalState == kGestureBegin && externalState == kGestureBegin)
			internalState = kGestureChanged;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZoomRecognizer::removeTouch (const TouchInfo& touchInfo)
{
	bool result = Recognizer::removeTouch (touchInfo);
	if(result && touches.count () == 1)
	{	
		initialDistance = 0.f;
		timeGestureStarted = 0;
		if(externalState != kGesturePossible)
			internalState = kGestureEnd;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureEventArgs ZoomRecognizer::getEventArgs () const
{
	if(touches.count () == 2 && initialDistance != 0.f)
	{
		center = getCenter ();
		amount = getDistance (touches.at (0), touches.at (1)) / initialDistance;
		// kGestureEnd event will use the last calculated values
	}
	return GestureEventArgs (center, amount, amount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float ZoomRecognizer::getDistance (const TouchInfo& touch1, const TouchInfo& touch2)
{
	float dX = touch1.whereF.x - touch2.whereF.x;
	float dY = touch1.whereF.y - touch2.whereF.y;
	return sqrtf (dX * dX + dY * dY);
}

//************************************************************************************************
// RotateRecognizer
//************************************************************************************************

RotateRecognizer::RotateRecognizer (GestureInfo* info) 
: Recognizer (info),
  initialAngle (0.f),
  margin (/*M_PI*/3.14159265358979323846f / 180 * 5)
{
	minimumTime = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RotateRecognizer::addTouch (const TouchInfo& touchInfo)
{
	bool result = false;
	if(touches.count () < 2)
		result = Recognizer::addTouch (touchInfo);
	
	if(touches.count () == 2)
	{
		initialAngle = getAngle (touches.at (0), touches.at (1));
		internalState = kGestureBegin;
		timeGestureStarted = touchInfo.time;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RotateRecognizer::changeTouch (const TouchInfo& touchInfo)
{
	bool result = Recognizer::changeTouch (touchInfo);
	if(result)
	{	
		if(touches.count () == 2)
		{
			if(internalState == kGesturePossible)
			{
				if(fabsf (getAngle (touches.at (0), touches.at (1)) - initialAngle) > margin)
				{
					internalState = kGestureBegin;
					timeGestureStarted = touchInfo.time;
				}
			}
		}	
		if(internalState == kGestureBegin && externalState == kGestureBegin)
			internalState = kGestureChanged;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RotateRecognizer::removeTouch (const TouchInfo& touchInfo)
{
	bool result = Recognizer::removeTouch (touchInfo);
	if(result && touches.count () == 1)
	{	
		initialAngle = 0.f;
		timeGestureStarted = 0;
		if(externalState != kGesturePossible)
			internalState = kGestureEnd;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureEventArgs RotateRecognizer::getEventArgs () const
{
	if(touches.count () == 2)
		return  GestureEventArgs (getCenter (), getAngle (touches.at (0), touches.at (1)) - initialAngle);
	else
		return  GestureEventArgs (getCenter ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float RotateRecognizer::getAngle (const TouchInfo& touch1, const TouchInfo& touch2)
{
	float dX = touch1.whereF.x - touch2.whereF.x;
	float dY = touch1.whereF.y - touch2.whereF.y;
	if(dX == 0.f && dY == 0.f)
		return 0.f;
	return atan2f (dY, dX); 
}

//************************************************************************************************
// LongPressRecognizer
//************************************************************************************************
	
LongPressRecognizer::LongPressRecognizer (GestureInfo* info)
: Recognizer (info)
{
	minimumTime = 500;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LongPressRecognizer::addTouch (const TouchInfo& touchInfo)
{
	if(touches.count () == 0 && internalState == kGesturePossible)
	{
		Recognizer::addTouch (touchInfo);
		internalState = kGestureBegin;
		timeGestureStarted = touchInfo.time;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LongPressRecognizer::changeTouch (const TouchInfo& touchInfo)
{
	bool result = Recognizer::changeTouch (touchInfo);
	if(result)
	{	
		if(internalState == kGestureBegin && externalState == kGestureBegin)
			internalState = kGestureChanged;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LongPressRecognizer::removeTouch (const TouchInfo& touchInfo)
{
	bool result = touches.contains (touchInfo);
	if(result && externalState != kGesturePossible)
		internalState = kGestureEnd;
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureEventArgs LongPressRecognizer::getEventArgs () const
{
	return GestureEventArgs (getCenter ());
}

//************************************************************************************************
// SingleTapRecognizer
//************************************************************************************************

SingleTapRecognizer::SingleTapRecognizer (GestureInfo* info)
: Recognizer (info)
{
	minimumTime = 200;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SingleTapRecognizer::addTouch (const TouchInfo& touchInfo)
{
	if(touches.count () == 0 && internalState == kGesturePossible)
	{
		Recognizer::addTouch (touchInfo);
		internalState = kGestureBegin;
		timeGestureStarted = touchInfo.time;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SingleTapRecognizer::changeTouch (const TouchInfo& touchInfo)
{
	bool result = Recognizer::changeTouch (touchInfo);
	if(result)
	{	
		if(internalState == kGestureBegin && externalState == kGestureBegin)
			internalState = kGestureChanged;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SingleTapRecognizer::removeTouch (const TouchInfo& touchInfo)
{
	if(touches.contains (touchInfo))
	{
		internalState = kGestureEnd;
		return true;
	}	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureEventArgs SingleTapRecognizer::getEventArgs () const
{
	return GestureEventArgs (getCenter ());
}

//************************************************************************************************
// DoubleTapRecognizer
//************************************************************************************************
	
DoubleTapRecognizer::DoubleTapRecognizer (GestureInfo* info)
: Recognizer (info),
  numTaps (0)
{
	minimumTime = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DoubleTapRecognizer::addTouch (const TouchInfo& touchInfo)
{
	if(internalState == kGesturePossible)
	{
		if(touches.count () == 0)
		{
			Recognizer::addTouch (touchInfo);			
			timeGestureStarted = touchInfo.time;
			numTaps = 1;
			return true;
		}
		if((touches.count () == 1) && (touchInfo.time - timeGestureStarted <= maxTimeDoubleTap))
		{
			// second tap must be inside a maximum distance
			PointF diff = touchInfo.whereF - touches.getFirst ().whereF;
			CoordF distance = get_max (fabs (diff.x), fabs (diff.y));
			if(distance <= maxDistDoubleTap)
			{
				Recognizer::addTouch (touchInfo);
				numTaps++;
				return true;
			}
		}
		internalState = kGestureFailed; // time or distance check failed
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DoubleTapRecognizer::removeTouch (const TouchInfo& touchInfo)
{
	if(touches.contains (touchInfo) && numTaps == 2)
	{
		internalState = kGestureBegin;
		return true;
	}	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureEventArgs DoubleTapRecognizer::getEventArgs () const
{
	return GestureEventArgs (getCenter ());
}

//************************************************************************************************
// GestureRecognition
//************************************************************************************************

GestureRecognition::GestureRecognition ()
: gestureSink (nullptr),
  longPressDelay (500)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GestureRecognition::setGestureSink (GestureSink* sink)
{
	gestureSink = sink;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GestureRecognition::setLongPressDelay (int delay)
{
	longPressDelay = delay;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GestureRecognition::processIdle (abs_time now)
{
	bool recognizeSwipe = false;
	bool recognizeZoom = false;
	bool recognizeRotate = false;
	bool recognizeLongPress = false;
	bool recognizeDoubleTap = false;
	
	VectorForEach (recognizers, Recognizer*, r)
		if(r->getType () == kGestureSwipe)
			recognizeSwipe = true;
		if(r->getType () == kGestureZoom)
			recognizeZoom = true;
		if(r->getType () == kGestureRotate)
			recognizeRotate = true;
		if(r->getType () == kGestureLongPress)
			recognizeLongPress = true;
		if(r->getType () == kGestureDoubleTap && r->getInternalState () != kGestureFailed)
			recognizeDoubleTap = true;
	EndFor
	
	int currentTouches = touches.count ();
	
	VectorForEach (recognizers, Recognizer*, r)
		int type = r->getType ();
		GestureEventType state = r->getInternalState ();
		bool ignoreTiming = type == kGestureSingleTap && (!recognizeSwipe && !recognizeZoom && !recognizeRotate && !recognizeLongPress && !recognizeDoubleTap);
		
		if(!r->getGestureInfo ()->isContinuous () && r->getInternalState () == kGestureChanged)
		{
			r->setExternalState (kGesturePossible);
			gestureSink->onGesture (r->getGestureInfo (), kGestureEnd, r->getEventArgs ());	
			break;
		}	
		
		if(state == kGestureBegin && (ignoreTiming || r->miniumTimeElapsed (now)))
		{
			if(type == kGestureSingleTap && recognizeLongPress)
				continue;
			if(type == kGestureSwipe && recognizeZoom && currentTouches > 1)
				continue;
			if(type == kGestureLongPress && recognizeZoom && currentTouches > 1)
				continue;
			r->setExternalState (r->getGestureInfo ()->isContinuous () ? kGestureChanged : kGestureEnd);
			gestureSink->onGesture (r->getGestureInfo (), kGestureBegin, r->getEventArgs ());
			break;
		}		
		
		if(state == kGestureEnd)
		{
			if(type == kGestureSingleTap && r->getExternalState () == kGesturePossible)
			{
				if(!recognizeDoubleTap || (now - r->getTimeGestureStarted ()) > r->maxTimeDoubleTap)
				{
					VectorForEach (recognizers, Recognizer*, longTapFinder)
						// When a single tap ends, we must cancel a potential long press gesture.
						// Otherwise, we get stuck when the single tap opens a pop-up with an own run loop.
						if(longTapFinder->getType () == kGestureLongPress)
						{
							stopRecognizing (longTapFinder->getGestureInfo ());
							break;
						}
					EndFor
					
					r->setExternalState (kGestureChanged);
					gestureSink->onGesture (r->getGestureInfo (), kGestureBegin, r->getEventArgs ());
					break;
				}				
			}
			else if(r->getExternalState () != kGestureEnd)
			{
				r->setExternalState (kGesturePossible);
				gestureSink->onGesture (r->getGestureInfo (), kGestureEnd, r->getEventArgs ());
				break;					
			}
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GestureRecognition::onTouchBegan (const TouchInfo& touchInfo)
{
	VectorForEach (recognizers, Recognizer*, r)
		r->addTouch (touchInfo);
	EndFor
	if(!touches.contains (touchInfo))
		touches.append (touchInfo);
	processIdle (touchInfo.time);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GestureRecognition::onTouchChanged (const TouchInfo& touchInfo)
{
	VectorForEach (recognizers, Recognizer*, r)
		r->changeTouch (touchInfo);
	EndFor
		
	if(gestureSink)
	{
		VectorForEach (recognizers, Recognizer*, r)
			if(r->getGestureInfo()->getTouchIDs ().contains (touchInfo.id))
				if(r->getInternalState () == kGestureChanged)
				{
					r->setExternalState (kGestureChanged);
					gestureSink->onGesture (r->getGestureInfo (), kGestureChanged, r->getEventArgs ());
				}
		EndFor
	}

}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GestureRecognition::onTouchEnded (const TouchInfo& touchInfo)
{
	VectorForEach (recognizers, Recognizer*, r)
		r->removeTouch (touchInfo);
	EndFor	
	touches.remove (touchInfo);
	processIdle (touchInfo.time);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool GestureRecognition::isRecognizing (const GestureInfo* gesture) const
{
	VectorForEachFast (recognizers, Recognizer*, recognizer)
		if(recognizer->getGestureInfo () == gesture)
			return true;
	EndFor	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GestureRecognition::startRecognizing (GestureInfo* gesture)
{
	Recognizer* recognizer = nullptr;
	switch(gesture->getType ())
	{
	case kGestureSwipe:
		recognizer = PooledSwipeRecognizer::pool_new ();
		break;
	case kGestureZoom:
		recognizer = PooledZoomRecognizer::pool_new ();
		break;
	case kGestureRotate:
		recognizer = PooledRotateRecognizer::pool_new ();
		break;		
	case kGestureLongPress:
		recognizer = PooledLongPressRecognizer::pool_new ();
		recognizer->setMinimumTime (longPressDelay);
		break;
	case kGestureSingleTap:
		recognizer = PooledSingleTapRecognizer::pool_new ();
		break;
	case kGestureDoubleTap:
		recognizer = PooledDoubleTapRecognizer::pool_new ();
		break;
	}
	if(recognizer)
	{
		recognizer->reset (gesture);
		recognizers.add (recognizer);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GestureRecognition::stopRecognizing (GestureInfo* gesture)
{
	VectorForEachFast (recognizers, Recognizer*, recognizer)
		if(recognizer->getGestureInfo () == gesture)
		{
			recognizers.remove (recognizer);
			recognizer->release ();
			break;
		}
	EndFor
}

} // namespace Core
