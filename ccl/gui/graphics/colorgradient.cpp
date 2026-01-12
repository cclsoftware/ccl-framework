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
// Filename    : ccl/gui/graphics/colorgradient.cpp
// Description : Color Gradient
//
//************************************************************************************************

#include "ccl/gui/graphics/colorgradient.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/gui/theme/colorscheme.h"

#include "ccl/base/message.h"

using namespace CCL;

//************************************************************************************************
// NativeGradient
//************************************************************************************************

NativeGradient* NativeGradient::resolve (IGradient* gradient)
{
	auto gradientObject = unknown_cast<Object> (gradient);
	if(auto nativeGradient = ccl_cast<NativeGradient> (gradientObject))
	{
		CCL_DEBUGGER ("This case is no longer expected\n")
		return nativeGradient;
	}
	else if(auto colorGradient = ccl_cast<ColorGradient> (gradientObject))
		return colorGradient->getNativeGradient ();
	else
		return nullptr;
}

//************************************************************************************************
// ColorGradientStopCollection
//************************************************************************************************

ColorGradientStopCollection::~ColorGradientStopCollection ()
{
	for(auto scheme : colorSchemeObserverList)
	{
		scheme->removeObserver (this);
		scheme->release ();
	}
	colorSchemeObserverList.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorGradientStopCollection::addStop (const ColorGradientStop& stop)
{
	stops.add (stop);
	
	if(auto scheme = stop.scheme)
		if(!colorSchemeObserverList.contains (scheme))
		{
			colorSchemeObserverList.add (scheme);
			scheme->addObserver (this);
			scheme->retain ();
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorGradientStopCollection::addPlainStops (const IGradient::Stop plainStops[], int count)
{
	stops.resize (stops.count () + count);
	for(int i = 0; i < count; i++)
	{
		ColorGradientStop stop;
		stop.colorValue = plainStops[i].color;
		stop.position = plainStops[i].position;
		stops.add (stop);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorGradientStopCollection::hasReferences (IColorScheme* scheme) const
{
	for(auto& s : stops)
		if(s.scheme == scheme)
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorGradientStopCollection::getPlainStops (Vector<IGradient::Stop>& plainStops, float opacity) const
{
	for(auto& s : stops)
	{
		Color color = s.scheme ? s.scheme->getColor (s.nameInScheme) : s.colorValue;
		if(opacity != 1.f)
			color.scaleAlpha (opacity);
		plainStops.add ({s.position, color});
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorGradientStopCollection::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
		signal (Message (kChanged));
}

//************************************************************************************************
// ColorGradient
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ColorGradient, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorGradient::ColorGradient (ColorGradientStopCollection* _stops)
: stops (nullptr),
  opacity (1.f),
  nativeGradient (nullptr)
{
	setStops (_stops);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorGradient::ColorGradient (const ColorGradient& other)
: stops (nullptr),
  opacity (other.opacity),
  nativeGradient (nullptr)
{
	setStops (other.stops);
	take_shared (nativeGradient, other.nativeGradient);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorGradient::~ColorGradient ()
{
	setStops (nullptr);
	safe_release (nativeGradient);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorGradientStopCollection* ColorGradient::getStops ()
{
	return stops;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorGradient::setStops (ColorGradientStopCollection* _stops)
{
	share_and_observe<ColorGradientStopCollection> (this, stops, _stops);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorGradient::setPlainStops (const Stop stops[], int stopCount)
{
	AutoPtr<ColorGradientStopCollection> colorStops = NEW ColorGradientStopCollection;
	colorStops->addPlainStops (stops, stopCount);
	setStops (colorStops);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorGradient::reset ()
{
	setStops (nullptr);
	opacity = 1.f;
	safe_release (nativeGradient);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorGradient::hasReferences (IColorScheme* scheme) const
{
	return stops ? stops->hasReferences (scheme) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorGradient::setOpacity (float newOpacity)
{
	if(opacity != newOpacity)
	{
		opacity = newOpacity;
		safe_release (nativeGradient);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorGradient::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
		safe_release (nativeGradient);
}

//************************************************************************************************
// LinearColorGradient
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LinearColorGradient, ColorGradient)

//////////////////////////////////////////////////////////////////////////////////////////////////

LinearColorGradient::LinearColorGradient (ColorGradientStopCollection* stops, 
										  PointFRef startPoint, PointFRef endPoint)
: ColorGradient (stops),
  startPoint (startPoint),
  endPoint (endPoint)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinearColorGradient::LinearColorGradient (const LinearColorGradient& other)
: ColorGradient (other),
  startPoint (other.startPoint),
  endPoint (other.endPoint)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinearColorGradient::construct (PointFRef _startPoint, PointFRef _endPoint, 
												const Stop stops[], int stopCount,
												IGradient* _other)
{
	reset ();
	startPoint = _startPoint;
	endPoint = _endPoint;
	if(auto other = unknown_cast<ColorGradient> (_other))
		setStops (other->getStops ());
	else
		setPlainStops (stops, stopCount);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinearColorGradient::scale (float sx, float sy)
{
	startPoint.x *= sx;
	startPoint.y *= sy;
	endPoint.x *= sx;
	endPoint.y *= sy;
	safe_release (nativeGradient);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGradient* LinearColorGradient::getNativeGradient ()
{
	// check if native gradient needs to be rebuilt
	if(nativeGradient && !nativeGradient->isValid ())
		safe_release (nativeGradient);

	if(!nativeGradient)
	{
		nativeGradient = NativeGraphicsEngine::instance ().createGradient (kLinearGradient);
		if(UnknownPtr<ILinearGradient> linearGradient = ccl_as_unknown (nativeGradient))
		{
			Vector<Stop> plainStops;
			if(stops) stops->getPlainStops (plainStops, opacity);
			linearGradient->construct (startPoint, endPoint, plainStops, plainStops.count ());
		}
	}
	return nativeGradient;
}

//************************************************************************************************
// RadialColorGradient
//************************************************************************************************

DEFINE_CLASS_HIDDEN (RadialColorGradient, ColorGradient)

//////////////////////////////////////////////////////////////////////////////////////////////////

RadialColorGradient::RadialColorGradient (ColorGradientStopCollection* stops, 
										  PointFRef center, float radius)
: ColorGradient (stops),
  center (center),
  radius (radius)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

RadialColorGradient::RadialColorGradient (const RadialColorGradient& other)
: ColorGradient (other),
  center (other.center),
  radius (other.radius)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API RadialColorGradient::construct (PointFRef _center, float _radius, const Stop stops[], int stopCount,
												IGradient* _other)
{
	reset ();
	center = _center;
	radius = _radius;
	if(auto other = unknown_cast<ColorGradient> (_other))
		setStops (other->getStops ());
	else
		setPlainStops (stops, stopCount);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RadialColorGradient::scale (float sx, float sy)
{
	center.x *= sx;
	center.y *= sy;
	radius *= ccl_max (sx, sy);	
	safe_release (nativeGradient);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGradient* RadialColorGradient::getNativeGradient ()
{
	// check if native gradient needs to be rebuilt
	if(nativeGradient && !nativeGradient->isValid ())
		safe_release (nativeGradient);

	if(!nativeGradient)
	{
		nativeGradient = NativeGraphicsEngine::instance ().createGradient (kRadialGradient);
		if(UnknownPtr<IRadialGradient> radialGradient = ccl_as_unknown (nativeGradient))
		{
			Vector<Stop> plainStops;
			if(stops) stops->getPlainStops (plainStops, opacity);
			radialGradient->construct (center, radius, plainStops, plainStops.count ());
		}
	}
	return nativeGradient;
}
