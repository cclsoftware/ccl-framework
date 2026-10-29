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
	auto* gradientObject = unknown_cast<Object> (gradient);
	#if DEBUG
	if(auto* nativeGradient = ccl_cast<NativeGradient> (gradientObject))
	{
		CCL_DEBUGGER ("This case is no longer expected\n")
		return nativeGradient;
	}
	else
	#endif
	if(auto* colorGradient = ccl_cast<ColorGradient> (gradientObject))
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
	ASSERT (stops.count () + count <= IGradient::kMaxStopCount)
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

void ColorGradientStopCollection::getPlainStops (GradientStopVector& plainStops, float opacity) const
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

float ColorGradient::getOpacity () const
{
	return opacity;
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

//////////////////////////////////////////////////////////////////////////////////////////////////

Color ColorGradient::getColorAt (float position) const
{
	GradientStopVector plainStops;
	if(stops) stops->getPlainStops (plainStops, opacity);
	return interpolateStops (plainStops, plainStops.count (), position);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorGradient::blendStopsInto (ColorGradient& other, float t) const
{
	GradientStopVector ownStops, otherStops;
	if(stops)
		stops->getPlainStops (ownStops, opacity);
	if(other.stops)
		other.stops->getPlainStops (otherStops, other.opacity);

	// combined stop positions from both gradients, deduplicated and sorted
	FixedSizeVector<float, kMaxStopCount> positions;
	for(Stop& stop : otherStops)
		positions.addOnce (stop.position);

	for(Stop& stop : ownStops)
		positions.addOnce (stop.position);

	positions.sort (LAMBDA_VECTOR_COMPARE_OBJECT (float, a, b)
		return ccl_compare (*a, *b);
	});

	GradientStopVector blendedStops;
	for(float position : positions)
		blendedStops.add ({position, Color::linearGradient (other.getColorAt (position), getColorAt (position), t)});

	other.setPlainStops (blendedStops.getItems (), blendedStops.count ());
	safe_release (other.nativeGradient);
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

void LinearColorGradient::setSize (PointFRef _startPoint, PointFRef _endPoint)
{
	if(_startPoint != startPoint || _endPoint != endPoint)
	{
		startPoint = _startPoint;
		endPoint = _endPoint;
		safe_release (nativeGradient);
	}
}

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
			GradientStopVector plainStops;
			if(stops) stops->getPlainStops (plainStops, opacity);
			linearGradient->construct (startPoint, endPoint, plainStops, plainStops.count ());
		}
	}
	return nativeGradient;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinearColorGradient::blendInto (ColorGradient& _other, float t) const
{
	if(LinearColorGradient* other = ccl_cast<LinearColorGradient> (&_other))
	{
		other->startPoint.x += (startPoint.x - other->startPoint.x) * t;
		other->startPoint.y += (startPoint.y - other->startPoint.y) * t;
		other->endPoint.x += (endPoint.x - other->endPoint.x) * t;
		other->endPoint.y += (endPoint.y - other->endPoint.y) * t;
	}
	blendStopsInto (_other, t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorGradient* LinearColorGradient::cloneWithSolidColor (ColorRef color) const
{
	LinearColorGradient* result = NEW LinearColorGradient (*this);
	Stop stops[2] = {{0.f, color}, {1.f, color}};
	result->setPlainStops (stops, 2);
	safe_release (result->nativeGradient);
	return result;
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

void RadialColorGradient::setSize (PointFRef _center, float _radius)
{
	if(center != _center || radius != _radius)
	{
		center = _center;
		radius = _radius;
		safe_release (nativeGradient);
	}
}

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
			GradientStopVector plainStops;
			if(stops) stops->getPlainStops (plainStops, opacity);
			radialGradient->construct (center, radius, plainStops, plainStops.count ());
		}
	}
	return nativeGradient;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RadialColorGradient::blendInto (ColorGradient& _other, float t) const
{
	if(RadialColorGradient* other = ccl_cast<RadialColorGradient> (&_other))
	{
		other->center.x += (center.x - other->center.x) * t;
		other->center.y += (center.y - other->center.y) * t;
		other->radius += (radius - other->radius) * t;
	}
	blendStopsInto (_other, t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorGradient* RadialColorGradient::cloneWithSolidColor (ColorRef color) const
{
	RadialColorGradient* result = NEW RadialColorGradient (*this);
	Stop stops[2] = {{0.f, color}, {1.f, color}};
	result->setPlainStops (stops, 2);
	safe_release (result->nativeGradient);
	return result;
}
