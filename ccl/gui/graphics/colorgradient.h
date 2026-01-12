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
// Filename    : ccl/gui/graphics/colorgradient.h
// Description : Color Gradient
//
//************************************************************************************************

#ifndef _ccl_colorgradient_h
#define _ccl_colorgradient_h

#include "ccl/base/object.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/gui/graphics/igradient.h"

#include "ccl/gui/theme/colorreference.h"

namespace CCL {

class NativeGradient;
interface IColorScheme;

//************************************************************************************************
// ColorGradientStop
//************************************************************************************************

struct ColorGradientStop: ColorValueReference
{
	float position = 0.f;
};

//************************************************************************************************
// ColorGradientStopCollection
/** Gradient stop collection with optional references to color scheme. */
//************************************************************************************************

class ColorGradientStopCollection: public Object
{
public:
	~ColorGradientStopCollection ();

	void addStop (const ColorGradientStop& stop);
	void addPlainStops (const IGradient::Stop plainStops[], int count);
	bool hasReferences (IColorScheme* scheme) const;
	
	void getPlainStops (Vector<IGradient::Stop>& plainStops, float opacity = 1.f) const;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	Vector<ColorGradientStop> stops;
	Vector<ColorScheme*> colorSchemeObserverList;
};

//************************************************************************************************
// ColorGradient
//************************************************************************************************

class ColorGradient: public Object,
					 public IGradient
{
public:
	DECLARE_CLASS_ABSTRACT (ColorGradient, Object)

	ColorGradient (ColorGradientStopCollection* stops = nullptr);
	ColorGradient (const ColorGradient& other);
	~ColorGradient ();
	
	ColorGradientStopCollection* getStops ();
	bool hasReferences (IColorScheme* scheme) const;

	void setOpacity (float opacity);
	virtual void scale (float sx, float sy) = 0;

	virtual NativeGradient* getNativeGradient () = 0;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IGradient, Object)

protected:
	ColorGradientStopCollection* stops;
	float opacity;
	NativeGradient* nativeGradient;

	void setStops (ColorGradientStopCollection* stops);
	void setPlainStops (const Stop stops[], int stopCount);
	void reset ();
};

//************************************************************************************************
// LinearColorGradient
//************************************************************************************************

class LinearColorGradient: public ColorGradient,
						   public ILinearGradient
{
public:
	DECLARE_CLASS (LinearColorGradient, ColorGradient)

	LinearColorGradient (ColorGradientStopCollection* stops = nullptr,
						 PointFRef startPoint = PointF (),
						 PointFRef endPoint = PointF ());
	LinearColorGradient (const LinearColorGradient& other);

	// ILinearGradient
	tresult CCL_API construct (PointFRef startPoint, PointFRef endPoint, const Stop stops[], int stopCount,
							   IGradient* other = nullptr) override;
	
	// ColorGradient
	void scale (float sx, float sy) override;
	NativeGradient* getNativeGradient () override;

	CLASS_INTERFACE (ILinearGradient, ColorGradient)

protected:
	PointF startPoint;
	PointF endPoint;
};

//************************************************************************************************
// RadialColorGradient
//************************************************************************************************

class RadialColorGradient: public ColorGradient,
						   public IRadialGradient
{
public:
	DECLARE_CLASS (RadialColorGradient, ColorGradient)

	RadialColorGradient (ColorGradientStopCollection* stops = nullptr,
						 PointFRef center = PointF (), float radius = 0.f);
	RadialColorGradient (const RadialColorGradient& other);

	// IRadialGradient
	tresult CCL_API construct (PointFRef center, float radius, const Stop stops[], int stopCount,
							   IGradient* other = nullptr) override;

	// ColorGradient
	void scale (float sx, float sy) override;
	NativeGradient* getNativeGradient () override;

	CLASS_INTERFACE (IRadialGradient, ColorGradient)

protected:
	PointF center;
	float radius;
};

} // namespace CCL

#endif // _ccl_colorgradient_h
