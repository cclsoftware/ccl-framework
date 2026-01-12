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
// Filename    : ccl/app/editing/scale.h
// Description : Scale
//
//************************************************************************************************

#ifndef _ccl_scale_h
#define _ccl_scale_h

#include "ccl/base/object.h"
#include "ccl/public/gui/iparamobserver.h"

#include "ccl/app/editing/iscale.h"

namespace CCL {

class Attributes;
class ScrollParam;
class FloatParam;
struct MouseWheelEvent;
interface IFormatter;

//************************************************************************************************
// Scale
/** A Scale is used to scale data for display on a pixel-oriented canvas with scroll and zoom support. */
//************************************************************************************************

class Scale: public Object,
			 public IScale,
			 public IParamObserver
{
public:
	DECLARE_CLASS (Scale, Object)

	enum Orientation
	{
		kVertical,
		kHorizontal
	};

	Scale (Unit numUnits = 100,
		   double unitsPerPixel = 1.,
		   Coord visibleLength = 1,
		   Coord offset = 0,
		   bool reversed = false,
		   Orientation orientation = kVertical);
	~Scale ();

	PROPERTY_VARIABLE (Orientation, orientation, Orientation)
	PROPERTY_BOOL (independentResolution, IndependentResolution)  ///< true if resolution should not be bound to visible length
	PROPERTY_VARIABLE (float, minZoom, MinZoom)   ///< min zoom in unit per pixels
	PROPERTY_VARIABLE (float, maxZoom, MaxZoom)   ///< max zoom in unit per pixels

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Resolution
	//////////////////////////////////////////////////////////////////////////////////////////////

	void setNumUnits (int units);
	void setReversed (bool _reversed) { reversed = _reversed; }

	void setTotalLength (Coord newLength);				///< make full data range visible
	Coord getTotalLength () const;						///< returns full length in pixels

	void setOffset (Coord newOffset);					///< offset is bound to its maximum
	Coord getOffset () const;							///< returns current offset in pixels
	Coord getMaxOffset () const;						///< returns maximum offset in pixels
	float getOffsetNormalized () const;					///< returns normalized offset (0..1)
	void setOffsetNormalized (float newNormOffset);		///< set offset from normalized float

	void setVisibleLength (Coord newLength);			///< offset and/or resolution are adjusted if necessary
	Coord getVisibleLength () const;					///< returns currently visible length in pixels
	void getVisibleUnits (Unit& start, Unit& end);      ///< returns currently visible range in units
	void setVisibleStartUnit (Unit start);              ///< make visible range start at given unit
	void setVisibleEndUnit (Unit end);                  ///< make visible range end at given unit
	void makeUnitVisible (Unit unit);

	void setUnitsPerPixel (double newUnitsPerPixel);	///< set new resolution
	double getUnitsPerPixel () const;					///< returns current resolution
	void setPixelPerUnit (Coord pixelPerUnit);			///< inverse of setUnitsPerPixel
	Coord getPixelPerUnit () const;						///< inverse of getUnitsPerPixel

	void getMinMaxUnitsPerPixel (double& minUnitsPerPixel, double& maxUnitsPerPixel) const; ///< unitsPerPixel for zoom 0 and zoom 1

	void setZoomFactor (float newZoom);					///< zoom is normalized (0..1)
	float getZoomFactor () const;						///< returns normalized zoom factor

	void center (int unit, Coord pixelOffset = 0);		///< center the scale around the supplied unit, optional adding pixels for exact placement
	int getCenter () const;

	void applyMouseWheel (const MouseWheelEvent& event); ///< view implementation helper

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Parameters
	//////////////////////////////////////////////////////////////////////////////////////////////

	IParameter* getScrollParam ();
	IParameter* getZoomParam ();

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Data Unit <-> Pixel Conversion (IScale)
	//////////////////////////////////////////////////////////////////////////////////////////////

	Coord unitToPixel (Unit value) const override;
	Unit pixelToUnit (Coord position) const override;
	void getExtent (Unit start, Unit end, Coord& startCoord, Coord& endCoord) const override;
	Unit getNumUnits () const override;
	bool isReversed () const override { return reversed; }
	IFormatter* createFormatter () const override;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Storage
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual void storeSettings (Attributes& attr) const;
	virtual void restoreSettings (Attributes& attr);

	CLASS_INTERFACE2 (IScale, IParamObserver, Object)

protected:
	Unit numUnits;					///< number of total data units
	double unitsPerPixel;			///< data units per pixel
	Coord visibleLength;			///< visible length in pixels
	Coord offset;					///< offset in pixels
	ScrollParam* scrollParam;		///< scroll parameter
	FloatParam* zoomParam;			///< zoom parameter
	bool reversed;					///< scale is reversed
	double previousScrollPosition;	///< used to keep scroll position stable when numUnits changes

	Coord boundOffset (Coord newOffset) const;
};

//************************************************************************************************
// ScaleZoomer
/** Calculates zooming and scrolling with a zoomLock position. */
//************************************************************************************************

class ScaleZoomer
{
public:
	ScaleZoomer (Scale* scale);

	// set zoom lock coord
	PROPERTY_VARIABLE (Coord, zoomLock, ZoomLock)		///< the pixel that should stay fix
	void setZoomLock (PointRef where);					///< selects coord based on scale orientation

	// set zoom lock from pixel range
	bool setZoomLock (Coord start, Coord end);			///< the pixel range that should stay visible
	bool setZoomLock (RectRef rect);					///< selects range based on scale orientation

	void zoom (float deltaZoom, Coord deltaScroll = 0);	/// deltaZoom, deltaScroll are base on the initial state
	void setZoomFactor (float zoom, Coord deltaScroll = 0);
	void setUnitsPerPixel (double newUnitsPerPixel);

private:
	Scale* scale;
	int startOffset;
	float startZoom;
	double startUnitsPerPixel;
};

} // namespace CCL

#endif // _ccl_scale_h
