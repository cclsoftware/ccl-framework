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
// Filename    : ccl/app/editing/scale.cpp
// Description : Scale
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/editing/scale.h"

#include "ccl/app/params.h"
#include "ccl/base/message.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IScale, 0x601fb0c8, 0xa5e1, 0x490e, 0xa8, 0xf8, 0x57, 0x30, 0x9, 0x17, 0xbb, 0x1f)

//************************************************************************************************
// ScaleZoomer
//************************************************************************************************

ScaleZoomer::ScaleZoomer (Scale* scale)
: scale (scale),
  zoomLock (0),
  startOffset (scale->getOffset ()),
  startZoom (scale->getZoomFactor ()),
  startUnitsPerPixel (scale->getUnitsPerPixel ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScaleZoomer::setZoomLock (PointRef where)
{
	setZoomLock (scale->getOrientation () == Scale::kHorizontal ? where.x : where.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScaleZoomer::setZoomLock (RectRef rect)
{
	if(scale->getOrientation () == Scale::kHorizontal)
		return setZoomLock (rect.left, rect.right);
	else
		return setZoomLock (rect.top, rect.bottom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScaleZoomer::setZoomLock (Coord start, Coord end)
{
	// visble scale range
	Coord scaleVisible = scale->getVisibleLength ();
	Coord scaleStart = 0;
	Coord scaleEnd = scaleVisible;

	if(start <= scaleEnd && end >= scaleStart)
	{
		// clip range to visible range
		Coord s = ccl_max (start, scaleStart);
		Coord e = ccl_min (end, scaleEnd);
		
		Coord lock;

		if(start < scaleStart && end > scaleEnd)
		{
			// both start and end of the range are not visible (zoomed in): use first visible coord
			lock = s;
		}
		else
		{
			Coord rangeVisible = e - s;
			double scaleCenter = (scaleStart + scaleEnd) / 2;

			if(scaleVisible == rangeVisible)
				lock = ccl_to_int<Coord> (scaleCenter);
			else
			{
				// calculate zoom lock so that as much as possible of the given range will be visible
				double rangeCenter =  (s + e) / 2;
				double centerOffset = rangeCenter - scaleCenter;
				lock = ccl_to_int<Coord> (scaleCenter + centerOffset * double(scaleVisible) / (scaleVisible - rangeVisible));
			}
		}

		setZoomLock (ccl_bound (lock, scaleStart, scaleEnd));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScaleZoomer::zoom (float deltaZoom, Coord deltaScroll)
{
	double newUnitsPerPixel = startUnitsPerPixel * (1.0 - deltaZoom / 10.0);

	double minUnitsPerPixel = 0.0;
	double maxUnitsPerPixel = 0.0;
	scale->getMinMaxUnitsPerPixel (minUnitsPerPixel, maxUnitsPerPixel);
	setUnitsPerPixel (ccl_bound (newUnitsPerPixel, minUnitsPerPixel, maxUnitsPerPixel));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScaleZoomer::setZoomFactor (float newZoom, Coord deltaScroll)
{
	newZoom = ccl_bound (newZoom, 0.f, 1.f);
	scale->setZoomFactor (newZoom);

	double newUnitsPerPixel = scale->getUnitsPerPixel ();
	int rev = scale->isReversed () ? 1 : -1;

	Coord newOffset = (Coord) (((zoomLock + rev * startOffset) * (startUnitsPerPixel / newUnitsPerPixel)) - (zoomLock + deltaScroll)) * rev;
	scale->setOffset (newOffset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScaleZoomer::setUnitsPerPixel (double newUnitsPerPixel)
{
	scale->setUnitsPerPixel (newUnitsPerPixel);
	int rev = scale->isReversed () ? 1 : -1;

	Coord newOffset = (Coord) (((zoomLock + rev * startOffset) * (startUnitsPerPixel / newUnitsPerPixel)) - (zoomLock)) * rev;
	scale->setOffset (newOffset);
}

//************************************************************************************************
// Scale
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Scale, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Scale::Scale (Unit numUnits, double unitsPerPixel, Coord visibleLength, Coord offset, bool reversed, Orientation orientation)
: numUnits (numUnits),
  unitsPerPixel (unitsPerPixel),
  visibleLength (visibleLength),
  offset (offset),
  reversed (reversed),
  orientation (orientation),
  independentResolution (false),
  scrollParam (nullptr),
  zoomParam (nullptr),
  minZoom (-1),
  maxZoom (-1),
  previousScrollPosition (0.0)
{
	ASSERT (unitsPerPixel > 0)
	ASSERT (visibleLength > 0)
	ASSERT (offset >= 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Scale::~Scale ()
{
	cancelSignals ();

	if(scrollParam)
		scrollParam->release ();
	if(zoomParam)
		zoomParam->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* Scale::getScrollParam ()
{
	if(scrollParam == nullptr)
	{
		scrollParam = NEW ScrollParam;
		scrollParam->setName (CSTR ("scaleScroll"));
		scrollParam->connect (this, 'Scrl');
		scrollParam->setRange (getTotalLength () - visibleLength, (float)visibleLength / (float)getTotalLength ());
		scrollParam->setValue (reversed ? offset : -offset);
	}
	return scrollParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* Scale::getZoomParam ()
{
	if(zoomParam == nullptr)
	{
		zoomParam = NEW FloatParam (0.f, 1.f);
		zoomParam->setName (CSTR ("scaleZoom"));
		zoomParam->connect (this, 'Zoom');
		zoomParam->setValue (getZoomFactor ());
		zoomParam->setCurve (AutoPtr<ConcaveCurve> (NEW ConcaveCurve));
	}
	return zoomParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Scale::paramChanged (IParameter* param)
{
	if(scrollParam && scrollParam == param)
	{
		Coord paramValue = reversed ? scrollParam->getValue ().asInt () : -scrollParam->getValue ().asInt ();
		if(paramValue != getOffset ()) 
			setOffset (paramValue);
	}
	else if(zoomParam && zoomParam == param)
	{
		int old = getCenter ();
		setZoomFactor (zoomParam->getValue ());
		center (old);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Scale::paramEdit (IParameter* param, tbool begin)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::setNumUnits (int units)
{
	if(scrollParam)
	{
		double scrollRange = scrollParam->getMax ().asDouble () - scrollParam->getMin ().asDouble ();
		double scrollPosition = scrollParam->getValue ().asDouble () / scrollRange;
		Coord totalLength = getTotalLength ();
		if(totalLength > visibleLength && ccl_abs (scrollPosition - previousScrollPosition) > 1.0 / (totalLength - visibleLength))
			previousScrollPosition = scrollPosition;
	}

	numUnits = units;

	if(scrollParam)
	{
		Coord totalLength = getTotalLength ();
		scrollParam->setRange (ccl_max (0, totalLength - visibleLength), ccl_min (1.f, (float)visibleLength / (float)totalLength));
		double scrollRange = scrollParam->getMax ().asDouble () - scrollParam->getMin ().asDouble ();
		offset = Coord(previousScrollPosition * scrollRange);
		scrollParam->setValue (offset, true);
	}

	if(zoomParam) // update zoom param since ZoomFactor depends on numUnits
		zoomParam->setValue (getZoomFactor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::setTotalLength (Coord newLength)
{
	if(newLength < 1)
		newLength = 1;

	offset = 0;
	visibleLength = newLength;
	unitsPerPixel = (double)numUnits / (double)newLength;
	if(zoomParam) // update zoom param since ZoomFactor depends on visibleLength
		zoomParam->setValue (getZoomFactor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord Scale::getTotalLength () const
{
	return ccl_to_int<Coord>((double)numUnits / unitsPerPixel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord Scale::boundOffset (Coord newOffset) const
{
	if(reversed)
		return ccl_bound<Coord> (newOffset, 0, getMaxOffset ());
	else
		return ccl_bound<Coord> (newOffset, -getMaxOffset (), 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::setOffset (Coord _newOffset)
{
	Coord newOffset = boundOffset (_newOffset);
	if(newOffset != offset)
	{
		offset = newOffset;
		deferChanged ();
		
		if(scrollParam)
			scrollParam->setValue (reversed ? _newOffset : -_newOffset, true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord Scale::getOffset () const
{
	return offset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord Scale::getMaxOffset () const
{
	Coord maxOffset = getTotalLength () - visibleLength;
	if(maxOffset < 0)
		maxOffset = 0;
	return maxOffset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float Scale::getOffsetNormalized () const
{
	Coord maxOffset = getMaxOffset ();
	return maxOffset == 0 ? 0.f : (float)getOffset () / (float)maxOffset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::setOffsetNormalized (float newNormOffset)
{
	newNormOffset = ccl_bound<float> (newNormOffset, 0.f, 1.f);
	Coord newOffset = (Coord)(newNormOffset * getMaxOffset () + .5f);
	setOffset (newOffset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::setVisibleLength (Coord newLength)
{
	if(newLength == getVisibleLength ())
		return;

	if(newLength <= 0)
		newLength = 1;

	Coord newOffset = offset;
	double newUnitsPerPixel = unitsPerPixel;
	Coord totalLength = getTotalLength ();

	Coord maxVisibleLength = totalLength - offset;
	if(newLength > maxVisibleLength)
	{
		// 1) try to adjust offset...
		Coord diff = newLength - maxVisibleLength;
		newOffset -= diff;
		if(newOffset < 0)
			newOffset = 0;
		maxVisibleLength = totalLength - newOffset;
		if(newLength > maxVisibleLength && independentResolution == false)
			newUnitsPerPixel = (double)numUnits / (double)newLength; // ... or 2) adjust resolution
	}

	if(newLength != visibleLength || newOffset != offset || newUnitsPerPixel != unitsPerPixel)
	{
		visibleLength = newLength;
		offset = newOffset;
		unitsPerPixel = newUnitsPerPixel;
		deferChanged ();

		if(zoomParam)
			zoomParam->setValue (getZoomFactor ());

		if(scrollParam)
		{
			scrollParam->setRange (totalLength - visibleLength, (float)visibleLength / (float)totalLength);
			scrollParam->setValue (reversed ? offset : -offset, true);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord Scale::getVisibleLength () const
{
	return visibleLength;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::setUnitsPerPixel (double newUnitsPerPixel)
{
	// check if offset is within range...
	Coord newOffset = offset;
	Coord newTotalLength = ccl_to_int<Coord> ((double)numUnits / newUnitsPerPixel);
	Coord currentLength = visibleLength + offset;
	if(currentLength > newTotalLength)
	{
		Coord diff = currentLength - newTotalLength;
		newOffset -= diff;
		if(newOffset < 0)
			newOffset = 0;
	}

	if(newUnitsPerPixel != unitsPerPixel || newOffset != offset)
	{
		unitsPerPixel = newUnitsPerPixel;
		offset = boundOffset (newOffset);
		deferChanged ();

		if(zoomParam)
			zoomParam->setValue (getZoomFactor ());

		if(scrollParam)
		{
			scrollParam->setRange (newTotalLength - visibleLength, (float)visibleLength / (float)newTotalLength);
			scrollParam->setValue (reversed ? offset : -offset, true);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double Scale::getUnitsPerPixel () const
{
	return unitsPerPixel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::setPixelPerUnit (Coord pixelPerUnit)
{
	if(pixelPerUnit <= 0)
		pixelPerUnit = 1;

	setUnitsPerPixel (1. / (double)pixelPerUnit);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord Scale::getPixelPerUnit () const
{
	Coord c = ccl_to_int<Coord> (1. / unitsPerPixel);
	return c < 1 ? 1 : c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::getMinMaxUnitsPerPixel (double& minUnitsPerPixel, double& maxUnitsPerPixel) const
{
	maxUnitsPerPixel = (double)numUnits / (double)visibleLength; // fully zoomed out, everything visible
	minUnitsPerPixel = 1. / 100.;								 // fully zoomed in, 100 pixels per unit
	if(minZoom >= 0 && maxZoom > minZoom)
	{
		minUnitsPerPixel = minZoom;
		if(independentResolution) // if resolution is not independent, the smallest zoom level is defined as showing all units
			maxUnitsPerPixel = maxZoom;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::getVisibleUnits (Unit& start, Unit& end)
{
	start = pixelToUnit (1);
	end = pixelToUnit (visibleLength - 1);
	if(isReversed ())
		ccl_swap (start, end);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::setVisibleStartUnit (Unit start)
{
	Coord startPixel = unitToPixel (start); // where is start now ?
	if(isReversed ())
	{
		Coord unitWidth = ccl_to_int (1 / unitsPerPixel);		
		setOffset (offset + startPixel + unitWidth - visibleLength); 	
	}
	else
		setOffset (offset - startPixel); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::setVisibleEndUnit (Unit end)
{
	Coord endPixel = unitToPixel (end); // where is end now ?
	if(isReversed ())
		setOffset (offset + endPixel); 	
	else
	{
		Coord unitWidth = ccl_to_int (1 / unitsPerPixel);		
		setOffset (offset - (endPixel + unitWidth) + visibleLength); 
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::makeUnitVisible (Unit unit)
{
	Unit start = 0, end = 0;
	getVisibleUnits (start, end);
	if(unit <= start || unit >= end)
	{
		center (unit);
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::setZoomFactor (float newZoom)
{
	double minUnitsPerPixel = 0.0;
	double maxUnitsPerPixel = 0.0;
	getMinMaxUnitsPerPixel (minUnitsPerPixel, maxUnitsPerPixel);
	setUnitsPerPixel (minUnitsPerPixel + (maxUnitsPerPixel - minUnitsPerPixel) * newZoom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float Scale::getZoomFactor () const
{
	// ZoomFactor is a value betweeen 0 and 1 that is relative to minUnitsPerPixel and maxUnitsPerPixel
	double minUnitsPerPixel = 0.0;
	double maxUnitsPerPixel = 0.0;
	getMinMaxUnitsPerPixel (minUnitsPerPixel, maxUnitsPerPixel);
	return (float)((unitsPerPixel - minUnitsPerPixel) / (maxUnitsPerPixel - minUnitsPerPixel)); // factor in [0, 1]
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::center (int unit, Coord pixelOffset)
{
	Unit maxValue = numUnits - 1;
	double visibleUnits = getVisibleLength () * unitsPerPixel / 2;
	double offsetUnits;

	if(isReversed ())
	{
		offsetUnits = maxValue - unit - visibleUnits;
		pixelOffset++; // try to match getCenter
	}
	else
	{
		offsetUnits = visibleUnits - unit;
		pixelOffset--; // try to match getCenter
	}

	setOffset ((Coord)ccl_round<0> (offsetUnits / unitsPerPixel) + pixelOffset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Scale::getCenter () const
{
	return pixelToUnit (getVisibleLength () / 2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord Scale::unitToPixel (Unit value) const
{
	Unit unitOffset = isReversed () ? -1 : 0;

	Unit minValue = unitOffset;
	Unit maxValue = numUnits + unitOffset;
	if(isReversed ())
		value = maxValue - value;

	value = ccl_bound<Unit> (value, minValue, maxValue);

	Coord position = ccl_to_int<Coord> ((double)value / unitsPerPixel);
	position += isReversed () ? -offset : offset;

	return position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Scale::Unit Scale::pixelToUnit (Coord position) const
{
	position -= isReversed () ? -offset : offset;

	Unit value = (Unit)(position * unitsPerPixel);

	Unit maxValue = numUnits - 1;
	if(isReversed ())
		value = maxValue - value;

	value = ccl_bound<Unit> (value, 0, maxValue);

	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::getExtent (Unit startUnit, Unit endUnit, Coord& startCoord, Coord& endCoord) const
{
	// "flesh out"
	if(isReversed ())
		startUnit--;
	else
		endUnit++;

	startCoord = unitToPixel (startUnit);
	endCoord = unitToPixel (endUnit);

	CCL_PRINTF ("Scale::getExtent %d %d %d %d\n", startUnit, endUnit, startCoord, endCoord)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Scale::Unit Scale::getNumUnits () const
{
	return numUnits;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFormatter* Scale::createFormatter () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::storeSettings (Attributes& a) const
{
	a.set ("zoomFactor", getZoomFactor ());
	a.set ("normalizedOffset", getOffsetNormalized ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::restoreSettings (Attributes& a)
{
	if(a.contains ("zoomFactor"))
		setZoomFactor ((float)a.getFloat ("zoomFactor"));
	if(a.contains ("normalizedOffset"))
		setOffsetNormalized ((float)a.getFloat ("normalizedOffset"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Scale::applyMouseWheel (const MouseWheelEvent& event)
{
	if(event.keys.getModifiers () == KeyState::kCommand)
	{
		// zoom with command key
		float delta = event.isContinuous () ? event.deltaY / 10.f : ccl_sign (event.delta) * 5.f;
		ScaleZoomer zoomer (this);
		zoomer.setZoomLock (event.where);
		zoomer.zoom (delta);
	}
	else
	{
		// scroll
		Coord delta = event.isContinuous () ? Coord (ccl_abs (event.delta)) : getPixelPerUnit ();
		
		if(isReversed ())
			delta *= -1;
		
		Coord newOffset = getOffset ();
		
		if(event.eventType == MouseWheelEvent::kWheelUp || event.eventType == MouseWheelEvent::kWheelLeft)
			newOffset += delta;
		else
			newOffset -= delta;
		
		setOffset (newOffset);
	}
}
