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
// Filename    : core/gui/coregesturerecognition.h
// Description : Platform-independent gesture recognition
//
//************************************************************************************************

#ifndef _coregesturerecognition_h
#define _coregesturerecognition_h

#include "core/public/gui/coremultitouch.h"

#include "core/public/corevector.h"
#include "core/public/corelinkedlist.h"
#include "core/public/coremacros.h"

namespace Core {
	
//************************************************************************************************
// GestureEventArgs
/** Gesture Event Arguments.
	\ingroup gui_event core_gui */
//************************************************************************************************

struct GestureEventArgs
{
	PointF where;
	float amountX;
	float amountY;
	
	GestureEventArgs (PointFRef where = PointF (), float amountX = 1., float amountY = 1.)
	: where (where),
	  amountX (amountX),
	  amountY (amountY)
	{}
};

//************************************************************************************************
// GestureInfo
/** \ingroup core_gui */
//************************************************************************************************

class GestureInfo
{
public:
	typedef FixedSizeVector<TouchID, 16> TouchVector;
	
	PROPERTY_VARIABLE (int, type, Type)
	
	TouchVector& getTouchIDs ();
	const TouchVector& getTouchIDs () const;
	bool addTouch (TouchID touchID);
	
	bool isContinuous () const;
	bool isTap () const;

	bool wantsTouch (TouchID touchID) const;
	bool needsMoreTouches () const;
	bool hadMultipleTouches () const;
	bool canRemove () const;
	bool containsTouch (TouchID touchID) const;
	
	GestureInfo& operator = (const GestureInfo& g);

	CStringPtr getGestureName () const;

	static CStringPtr getGestureName (const GestureInfo* info);
	static CStringPtr getGestureName (int type);

protected:
	TouchVector touchIds;
	int numTaps;
	
	GestureInfo (int type)
	: type (type), numTaps (0)
	{}
};

//************************************************************************************************
// GestureRecognition
/** \ingroup core_gui */
//************************************************************************************************

class GestureRecognition
{
public:
	class Recognizer;
	struct GestureSink;
	
	GestureRecognition ();

	void setGestureSink (GestureSink* sink);
	void setLongPressDelay (int delay); // in ms (default: 500ms)
	
	void processIdle (abs_time now);
	
	virtual void onTouchBegan (const TouchInfo& touchInfo);
	virtual void onTouchChanged (const TouchInfo& touchInfo);
	virtual void onTouchEnded (const TouchInfo& touchInfo);
	
	tbool isRecognizing (const GestureInfo* gesture) const;
	void startRecognizing (GestureInfo* gesture);
	void stopRecognizing (GestureInfo* gesture);
		
private:
	LinkedList<TouchInfo> touches;
	GestureSink* gestureSink;
	FixedSizeVector<Recognizer*, 64> recognizers;
	int longPressDelay;
};

//************************************************************************************************
// GestureRecognition::GestureSink
/** \ingroup core_gui */
//************************************************************************************************

struct GestureRecognition::GestureSink
{
public:
	virtual ~GestureSink () {}
	virtual void onGesture (GestureInfo* gesture, int state, const GestureEventArgs& args) = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// GestureInfo inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline GestureInfo::TouchVector& GestureInfo::getTouchIDs ()
{ return touchIds; }

inline const GestureInfo::TouchVector& GestureInfo::getTouchIDs () const
{ return touchIds; }

inline bool GestureInfo::isContinuous () const
{ return type >= kGestureSwipe && type <= kGestureLongPress; }

inline bool GestureInfo::isTap () const
{ return type == kGestureSingleTap || type == kGestureDoubleTap; }

inline bool GestureInfo::wantsTouch (TouchID touchID) const
{
	if(containsTouch (touchID))
		return true;
	
	if(getType () == kGestureDoubleTap)
		return numTaps <= 2; // accept second touch with different id
	
	return false;
}

inline bool GestureInfo::needsMoreTouches () const
{
	// when a single tap gesture stays after its touch ended (wait for double tap),
	// it must not accept other touches
	if(type == kGestureSingleTap)
		return numTaps == 0; 

	int minTouches = (type == kGestureZoom || type == kGestureRotate || type == kGestureDoubleTap) ? 2 : 1;
	return touchIds.count () < minTouches;
}

inline bool GestureInfo::hadMultipleTouches () const
{
	return getTouchIDs ().count () >= 2 || numTaps >= 2;
}

inline bool GestureInfo::canRemove () const
{
	if(getType () == kGestureDoubleTap)
		return numTaps >= 2;
	
	return touchIds.isEmpty ();
}

inline bool GestureInfo::addTouch (TouchID touchID)
{
	if(!touchIds.contains (touchID))
	{
		touchIds.add (touchID);
		numTaps++;
		return true;
	}
	return false;
}

inline bool GestureInfo::containsTouch (TouchID touchID) const
{
	return getTouchIDs ().contains (touchID);
}

inline GestureInfo& GestureInfo::operator = (const GestureInfo& g)
{
	type = g.type;
	numTaps = g.numTaps;
	touchIds.removeAll ();
	touchIds.addAll (g.touchIds);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _coregesturerecognition_h
