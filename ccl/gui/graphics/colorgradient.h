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
// GradientStopVector
//************************************************************************************************

typedef FixedSizeVector<IGradient::Stop, IGradient::kMaxStopCount> GradientStopVector;

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
	
	void getPlainStops (GradientStopVector& plainStops, float opacity = 1.f) const;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	FixedSizeVector<ColorGradientStop, IGradient::kMaxStopCount> stops;
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

	float getOpacity () const;
	void setOpacity (float opacity);

	virtual void scale (float sx, float sy) = 0;

	/** Get interpolated color at a normalized position (0..1) along this gradient's stops. */
	Color getColorAt (float position) const;

	/** Blend this gradient into other (weighted t, other counts as t=0, this as t=1): combines and
		color-interpolates the stops of this and other, and interpolates geometry if
		other has a matching geometry type, else geometry is kept as-is. */
	virtual void blendInto (ColorGradient& other, float t) const = 0;

	/** Create a copy of this gradient (same type/geometry) with its stops replaced by a
		two-stop gradient (position 0 and 1) of the given flat color. */
	virtual ColorGradient* cloneWithSolidColor (ColorRef color) const = 0;

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

	/** Combine and color-interpolate the stops of this and other (weighted t), writing the result into other. */
	void blendStopsInto (ColorGradient& other, float t) const;
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

	void setSize (PointFRef startPoint, PointFRef endPoint);

	// ILinearGradient
	tresult CCL_API construct (PointFRef startPoint, PointFRef endPoint, const Stop stops[], int stopCount,
							   IGradient* other = nullptr) override;
	
	// ColorGradient
	void scale (float sx, float sy) override;
	NativeGradient* getNativeGradient () override;
	void blendInto (ColorGradient& other, float t) const override;
	ColorGradient* cloneWithSolidColor (ColorRef color) const override;

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

	void setSize (PointFRef center, float radius);

	// IRadialGradient
	tresult CCL_API construct (PointFRef center, float radius, const Stop stops[], int stopCount,
							   IGradient* other = nullptr) override;

	// ColorGradient
	void scale (float sx, float sy) override;
	NativeGradient* getNativeGradient () override;
	void blendInto (ColorGradient& other, float t) const override;
	ColorGradient* cloneWithSolidColor (ColorRef color) const override;

	CLASS_INTERFACE (IRadialGradient, ColorGradient)

protected:
	PointF center;
	float radius;
};

} // namespace CCL

#endif // _ccl_colorgradient_h
