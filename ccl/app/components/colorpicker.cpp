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
// Filename    : ccl/app/components/colorpicker.cpp
// Description : Color Picker Component
//
//************************************************************************************************

#include "ccl/app/components/colorpicker.h"

#include "ccl/app/params.h"
#include "ccl/app/controls/usercontrol.h"

#include "ccl/app/presets/simplepreset.h"
#include "ccl/app/presets/presetcomponent.h"
#include "ccl/public/app/presetmetainfo.h"

#include "ccl/base/storage/storage.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/math/mathprimitives.h"

#include "ccl/public/gui/framework/iitemmodel.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/themeelements.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/cclversion.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (ColorPalette, "Color Palette")
END_XSTRINGS

//************************************************************************************************
// ColorPaletteFile
//************************************************************************************************

class ColorPaletteFile: public JsonStorableObject
{
public:
	DECLARE_CLASS (ColorPaletteFile, JsonStorableObject)

	ColorPaletteFile (IColorPalette* _palette = nullptr) 
	: palette (_palette) 
	{}

	// StorableObject
	tbool CCL_API getFormat (FileType& format) const override
	{
		format = CustomColorPresets::getFileType ();
		return true;
	}
	
	bool load (const Storage& storage) override
	{
		palette->removeAll ();
		
		Attributes& a = storage.getAttributes ();
		
		IterForEach (a.newQueueIterator ("colors", ccl_typeid<Attribute> ()), Attribute, attr)
			int64 hexValue = 0;
			attr->getValue ().asString ().getHexValue (hexValue);
			palette->appendColor (Color::fromInt ((uint32)hexValue));
		EndFor		
		return true;
	}
	
	bool save (const Storage& storage) const override
	{
		Attributes& a = storage.getAttributes ();
		
		AttributeQueue* colors = NEW AttributeQueue;
		for(int i = 0; i < palette->getCount (); i++)
		{
			String temp;
			temp.appendHexValue ((int64)palette->getColorAt (i), 8);
			colors->addValue (temp);
		}
		
		a.set ("colors", colors, Attributes::kOwns);
		return true;
	}
	
protected:
	SharedPtr<IColorPalette> palette;
};

DEFINE_CLASS_HIDDEN (ColorPaletteFile, JsonStorableObject)

//************************************************************************************************
// ColorPicker::HSLColorWheel
//************************************************************************************************

class ColorPicker::HSLColorWheel: public UserControl
{
public:
	DECLARE_CLASS (ColorPicker::HSLColorWheel, UserControl)
	
	HSLColorWheel (IParameter* _hslH = nullptr, IParameter* _hslS = nullptr, IParameter* _hslL = nullptr, RectRef size = Rect (), StyleRef customStyle = 0, StringRef title = nullptr);
	~HSLColorWheel ();
	
	PROPERTY_VARIABLE (Color, pickerBackcolor, PickerBackcolor)
	PROPERTY_VARIABLE (Color, pickerBorderColor, PickerBorderColor)
	
	void setHSLValue (float h, float s = -1, float l = -1, PointRef _exactSLHandlePosition = Point ());
	PointF getDefaultTrianglePoint (int index) const;
	void resetExactHandlePosition ();
	
	// UserControl
	void attached (IView* parent) override;
	void draw (const DrawEvent& event) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	IMouseHandler* CCL_API createMouseHandler (const MouseEvent& event) override;
	
protected:
	IParameter* hslH;
	IParameter* hslS;
	IParameter* hslL;
	AutoPtr<IImage> colorColorWheelImage;
	AutoPtr<IImage> triangleShapeImage;
	AutoPtr<IImage> triangleBitmap;
	Coord colorWheelOffset;
	Coord rOffset;
	Coord gOffset;
	Coord bOffset;
	float colorWheelRatio;
	float borderPenWidth;
	float bezierRadius;
	Coord handleSize;
	float fullColorLuminance;
	bool triangleNeedUpdate;
	Vector<PointF> trianglePoints;
	ColorHSL hslColor;
	Point exactSLHandlePosition;
	bool initialized;
	
	enum PartCode
	{
		kColorWheel = 0,
		kColorTriangle
	};
	
	Rect getPartSize (PartCode which) const;	
	Rect getPartRect (PartCode which) const;
	
	enum RadiusCode
	{
		kInner = 0,
		kOuter,
		kMiddle
	};
	
	float getColorWheelRadius (RadiusCode which) const;
	bool isInsideColorWheel (PointRef where) const;
	bool isInsideColorTriangle (PointRef where) const;
	
	void initialize ();
	void makeColorWheel ();
	void updateTriangle ();

	Point getWheelPosition () const;
	Point getTrianglePosition () const;
	void drawColorWheelHandle (IGraphics& graphics);
	void drawColorTriangleHandle (IGraphics& graphics);
};

//************************************************************************************************
// ColorPicker::RGBSlider
//************************************************************************************************

class ColorPicker::RGBSlider: public UserControl
{
public:
	DECLARE_CLASS (ColorPicker::RGBSlider, UserControl)
	
	RGBSlider (IParameter* _mainParameter = nullptr, IParameter* _leftParameter = nullptr, IParameter* _rightParameter = nullptr, RectRef size = Rect (), int channelTag = 0, StyleRef customStyle = 0, StringRef title = nullptr);
	~RGBSlider ();
	
	// UserControl
	void attached (IView* parent) override;
	void draw (const DrawEvent& event) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	
protected:
	IParameter* mainParameter;
	IParameter* leftParameter;
	IParameter* rightParameter;
	
	AutoPtr<IImage> gradientImage;

	Coord backRadius;
	Coord margin;
	bool gradientNeedsUpdate;
	bool initialized;
	int channelTag;
	
	void initialize ();
	void updateGradient ();
};
 
//************************************************************************************************
// ColorPickerHueMouseHandler
//************************************************************************************************

class ColorPicker::ColorPickerHueMouseHandler: public UserControl::MouseHandler
{
public:
	ColorPickerHueMouseHandler (HSLColorWheel* control = nullptr, RectRef wheelRect = Rect ())
	: control (control),
	  wheelRect (wheelRect)
	{}

	// MouseHandler
	void onBegin () override
	{
		control->setHSLValue (translatePositionToHue (first.where));
	}

	bool onMove (int moveFlags) override
	{
		control->setHSLValue (translatePositionToHue (current.where));
		return true;
	}

	void onRelease (bool canceled) override
	{
		UserControl::MouseHandler::onRelease (canceled);
	}
	
protected:
	HSLColorWheel* control;
	Rect wheelRect;
	
	float translatePositionToHue (PointRef where)
	{
		Point centerWhere (where);
		Point center (wheelRect.getCenter ());
		centerWhere.offset (center * -1);

		double theta, radius;
		Math::cartesianToPolar (theta, radius, (double)centerWhere.x, (double)centerWhere.y);

		float degree = float(Math::radToDegrees (theta));
		if(degree >= 360)
			degree -= 360;
		if(degree < 0)
			degree += 360;
		return degree;
	}
};

//************************************************************************************************
// ColorPickerSLMouseHandler
//************************************************************************************************

class ColorPicker::ColorPickerSLMouseHandler: public UserControl::MouseHandler
{
public:
	ColorPickerSLMouseHandler (HSLColorWheel* control = nullptr, RectRef satLumRect = Rect (), float hueValue = 0)
	: control (control),
	  frameOffset (satLumRect.getLeftTop ()),
	  triangleSize (satLumRect),
	  hueValue (hueValue)
	{
		triangleSize.moveTo (Point ());
		PointF offsetP (control->getDefaultTrianglePoint (0) * -1);
		discreteTrianglePointOffset = Point (ccl_to_int (offsetP.x), ccl_to_int (offsetP.y));
		
		dtp.add (Point ());
		
		PointF second (control->getDefaultTrianglePoint (1));
		dtp.add (Point (ccl_to_int (second.x), ccl_to_int (second.y)));
		
		PointF third (control->getDefaultTrianglePoint (2));
		dtp.add (Point (ccl_to_int (third.x), ccl_to_int (third.y)));
		
		dtp[1].offset (discreteTrianglePointOffset);
		dtp[2].offset (discreteTrianglePointOffset);
	}

	void onBegin () override
	{
		setSLFromPosition (first.where);
	}

	bool onMove (int moveFlags) override
	{
		Point delta = first.where - current.where;
		setSLFromPosition (current.where);
		
		return true;
	}
	
	void onRelease (bool canceled) override
	{
		UserControl::MouseHandler::onRelease (canceled);
		control->resetExactHandlePosition ();
	}
	
protected:
	HSLColorWheel* control;
	Point frameOffset;
	Rect triangleSize;
	Vector<Point> dtp;
	Point discreteTrianglePointOffset;
	float hueValue;
	
	void setSLFromPosition (PointRef where)
	{
		// transform where
		Point currenWhere (where);
		currenWhere.offset (frameOffset * -1);

		Transform matrix;
		matrix.translate (triangleSize.getCenter ().x, triangleSize.getCenter ().y);
		matrix.rotate (Math::degreesToRad (-hueValue));
		matrix.translate (-triangleSize.getCenter ().x, -triangleSize.getCenter ().y);
		matrix.transform (currenWhere);
		
		currenWhere.offset (discreteTrianglePointOffset);
	
		float s = 0.f;
		float l = 0.f;
		Point exactSLHandlePosition;
		
		if(currenWhere.x <= 0) // outer left 
		{
			s = 0.f;
			
			if(currenWhere.y <= 0)
				l = 1;
			else if((currenWhere.y > dtp[0].y) && (currenWhere.y < dtp[1].y))
				l = 1 - (currenWhere.y / (float)dtp[1].y);
			else
				l = 0;
		}
		else if(currenWhere.x > dtp[2].x) // outer right
		{
			s = 1;
			l = 0.5f;
		}
		else // center
		{
			Point s1 (0,0);
			Point s2 (0, dtp[1].y);
			Point s1_old (0,0);
			Point s2_old (0, dtp[1].y);
			Point c1 (currenWhere.x, 0);
			Point c2 (currenWhere.x, dtp[1].y);
			
			getIntersectionPoint (s1, dtp[0], dtp[2], c1, c2);
			getIntersectionPoint (s2, dtp[1], dtp[2], c1, c2);
		
			if(s1.y > currenWhere.y) // above
			{			      
				s = 1;
				l = 1 - (s1.y / (float)dtp[1].y);	
			}
			else if(s2.y < currenWhere.y) // below
			{			      
				s = 1;
				l = 1 - (s2.y / (float)dtp[1].y);
			}
			else // inside
			{
				exactSLHandlePosition.x = currenWhere.x; 
				exactSLHandlePosition.y = currenWhere.y;
				
				Point s1 (dtp[2].x, 0);
				Point s2 (dtp[2].x, dtp[1].y);
				Point c1 (0, currenWhere.y);
				Point c2 (dtp[2].x, currenWhere.y);
				
				getIntersectionPoint (s1, dtp[0], dtp[2], c1, c2);
				getIntersectionPoint (s2, dtp[1], dtp[2], c1, c2);
				
				l = 1 - (currenWhere.y / (float)dtp[1].y);
				
				if(s1.x < dtp[2].x) // upper half
				{
					s = currenWhere.x / (float)s1.x;
				}
				else if(s2.x <= dtp[2].x) // lower half
				{			      
					s = currenWhere.x / (float)s2.x;
				}
				else
				{
					ASSERT (false); // cannot happen (tm) 
				}
			}
		}
		
		if(exactSLHandlePosition.isNull ())
		{
			//derive from s and l values
			float xFactor = 1;
			if(l <= 0.5f)
			 	xFactor = l * 2;
			else
				xFactor = (1 - (l - 0.5f) * 2);
			
			exactSLHandlePosition.x = ccl_to_int (s * (dtp[2].x * xFactor));
			exactSLHandlePosition.y = ccl_to_int ((1 - l) * dtp[1].y);
		}
	
		exactSLHandlePosition.offset (discreteTrianglePointOffset * -1);
		
		Transform invMatrix;
		invMatrix.translate (triangleSize.getCenter ().x, triangleSize.getCenter ().y);
		invMatrix.rotate (Math::degreesToRad (hueValue));
		invMatrix.translate (-triangleSize.getCenter ().x, -triangleSize.getCenter ().y);
		invMatrix.transform (exactSLHandlePosition);
		
		exactSLHandlePosition.offset (frameOffset);
		
		control->setHSLValue (hueValue, s, l, exactSLHandlePosition);
	}	
};

//************************************************************************************************
// ColorPicker::HSLColorWheel
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ColorPicker::HSLColorWheel, UserControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorPicker::HSLColorWheel::HSLColorWheel (IParameter* _hslH, IParameter* _hslS, IParameter* _hslL, RectRef size, StyleRef customStyle, StringRef title)
: UserControl (size, customStyle, title),
  hslH (nullptr),
  hslS (nullptr),
  hslL (nullptr),
  triangleNeedUpdate (true),
  colorWheelOffset (0),
  rOffset (0),
  gOffset (0),
  bOffset (0),
  colorWheelRatio (0),
  borderPenWidth (1),
  fullColorLuminance (0.5f), 
  bezierRadius (10),
  handleSize (10),
  pickerBackcolor (Colors::kBlack),
  pickerBorderColor (Colors::kGray),
  initialized (false)
{
	share_and_observe_unknown (this, hslH, _hslH);
	share_and_observe_unknown (this, hslS, _hslS);
	share_and_observe_unknown (this, hslL, _hslL);

	hslColor.h = hslH->getValue ().asInt (); 
	hslColor.s = hslS->getValue ().asFloat ();
	hslColor.l = hslL->getValue ().asFloat ();
}

////////////////////////////////////////////////////////////////////////////

ColorPicker::HSLColorWheel::~HSLColorWheel ()
{
	share_and_observe_unknown<IParameter> (this, hslH, nullptr);
	share_and_observe_unknown<IParameter> (this, hslS, nullptr);
	share_and_observe_unknown<IParameter> (this, hslL, nullptr);
}

////////////////////////////////////////////////////////////////////////////

void ColorPicker::HSLColorWheel::attached (IView* parent)
{
	initialize ();
}

////////////////////////////////////////////////////////////////////////////

void ColorPicker::HSLColorWheel::initialize ()
{
	const IVisualStyle& vs = getVisualStyle ();
	colorWheelOffset = vs.getMetric ("colorwheel.offset", 0);
	rOffset = vs.getMetric ("r.offset", 0);
	gOffset = vs.getMetric ("g.offset", 0);
	bOffset = vs.getMetric ("b.offset", 0);
	
	colorWheelRatio = vs.getMetric ("colorwheel.ratio", 0.2f); // percentage of the available space for the colorWheel
	borderPenWidth = vs.getMetric ("borderpenwidth", borderPenWidth);
	setPickerBackcolor (vs.getBackColor ());
	setPickerBorderColor (vs.getColor ("bordercolor"));
	fullColorLuminance = vs.getMetric ("fullcolor.luminance", 50) / 100.f;
	
	bezierRadius = vs.getMetric ("bezierradius", bezierRadius);
	handleSize = vs.getMetric ("handlesize", handleSize);

	colorColorWheelImage = GraphicsFactory::createShapeImage ();
	triangleShapeImage = GraphicsFactory::createShapeImage ();
	
	Rect triangleSize (getPartSize (kColorTriangle));
	triangleBitmap = GraphicsFactory::createSolidBitmap (Colors::kTransparentBlack, triangleSize.getWidth (), triangleSize.getHeight (), IBitmap::kRGBAlpha, 2.f);
	
	makeColorWheel ();
	triangleNeedUpdate = true;
	initialized = true;
}

////////////////////////////////////////////////////////////////////////////

void ColorPicker::HSLColorWheel::makeColorWheel ()
{
	Rect colorWheelSize (getPartSize (kColorWheel));
	Rect arcSquare (colorWheelSize);
	float penWidth = colorWheelSize.getWidth () * colorWheelRatio;
	arcSquare.contract (ccl_to_int(penWidth * 0.5f));

	AutoPtr<IGraphics> g = GraphicsFactory::createShapeBuilder (colorColorWheelImage);	
	g->fillEllipse (colorWheelSize, SolidBrush (pickerBackcolor));
	
	int i = 0;
	
	while(i < 360)
	{
		ColorHSL hsl (i, 1, fullColorLuminance);
		Color color;
		hsl.toColor (color);
		if(AutoPtr<IGraphicsPath> path = GraphicsFactory::createPath ())
		{
			path->addArc (arcSquare, i, 2);
			g->drawPath (path, Pen (color, penWidth));
		}
		i++;
	}
	
	if(AutoPtr<IGraphicsPath> path = GraphicsFactory::createPath ())
	{
		ColorHSL hsl (0, 1, fullColorLuminance);
		Color color;
		hsl.toColor (color);
		path->addArc (arcSquare, 0, 1);
		g->drawPath (path, Pen (color, penWidth));
	}
	
	g->drawEllipse (colorWheelSize, Pen (pickerBorderColor, borderPenWidth));
	g->drawEllipse (arcSquare, Pen (pickerBorderColor, borderPenWidth));
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////
	
void CCL_API ColorPicker::HSLColorWheel::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		if(isEqualUnknown (subject, hslH))
		{
			hslColor.h = hslH->getValue ().asInt ();
			triangleNeedUpdate = true;
		}	
		else if(isEqualUnknown (subject, hslS))
		{
			hslColor.s = hslS->getValue ().asFloat ();
		}	
		else if(isEqualUnknown (subject, hslL))
		{
			hslColor.l = hslL->getValue ().asFloat ();
		}
		
		invalidate ();
	}
	else
	{
		SuperClass::notify (subject, msg);
	}
}

////////////////////////////////////////////////////////////////////////////

void ColorPicker::HSLColorWheel::updateTriangle ()
{
 	// calculate main triangle points : (top left), (bottom left), (right center)
	trianglePoints = {getDefaultTrianglePoint (0), getDefaultTrianglePoint (1), getDefaultTrianglePoint (2)};
	
	float hueValue = hslColor.h;
	if(hueValue != 0.f)
	{
		Rect triangleSize (getPartSize (kColorTriangle));
		Transform matrix;
		matrix.translate (triangleSize.getCenter ().x, triangleSize.getCenter ().y);
		matrix.rotate (Math::degreesToRad (hueValue));
		matrix.translate (-triangleSize.getCenter ().x, -triangleSize.getCenter ().y);
	
		for(int i = 0; i < 3 ; i++)
			matrix.transform (trianglePoints[i]);
	}
	
	float p0AX = bezierRadius * cos (Math::degreesToRad (hueValue - 330)) + trianglePoints[0].x;
	float p0AY = bezierRadius * sin (Math::degreesToRad (hueValue - 330)) + trianglePoints[0].y;
	float p0BX = bezierRadius * cos (Math::degreesToRad (hueValue - 270)) + trianglePoints[0].x;
	float p0BY = bezierRadius * sin (Math::degreesToRad (hueValue - 270)) + trianglePoints[0].y;
	
	PointF p0A (p0AX, p0AY);
	PointF p0B (p0BX, p0BY);

	float p1AX = bezierRadius * cos (Math::degreesToRad (hueValue - 90)) + trianglePoints[1].x;
	float p1AY = bezierRadius * sin (Math::degreesToRad (hueValue - 90)) + trianglePoints[1].y;
	float p1BX = bezierRadius * cos (Math::degreesToRad (hueValue - 30)) + trianglePoints[1].x;
	float p1BY = bezierRadius * sin (Math::degreesToRad (hueValue - 30)) + trianglePoints[1].y;
	
	PointF p1A (p1AX, p1AY);
	PointF p1B (p1BX, p1BY);
	
	float p2AX = bezierRadius * cos (Math::degreesToRad (hueValue - 210)) + trianglePoints[2].x;
	float p2AY = bezierRadius * sin (Math::degreesToRad (hueValue - 210)) + trianglePoints[2].y;
	float p2BX = bezierRadius * cos (Math::degreesToRad (hueValue - 150)) + trianglePoints[2].x;
	float p2BY = bezierRadius * sin (Math::degreesToRad (hueValue - 150)) + trianglePoints[2].y;
		
	PointF p2A (p2AX, p2AY);
	PointF p2B (p2BX, p2BY);
	
	if(AutoPtr<IGraphicsPath> path = GraphicsFactory::createPath ())
	{
		path->startFigure (p0A);
		path->addBezier (p0A, trianglePoints[0], trianglePoints[0], p0B); 
		path->lineTo (p1A);
		path->addBezier (p1A, trianglePoints[1], trianglePoints[1], p1B); 
		path->lineTo (p2A);
		path->addBezier (p2A, trianglePoints[2], trianglePoints[2], p2B); 
		path->closeFigure ();
		
		Rect backSize (getPartSize (kColorTriangle));
	
		if(AutoPtr<IGraphics> g = GraphicsFactory::createShapeBuilder (triangleShapeImage))
		{	
			g->fillEllipse (getPartSize (kColorTriangle), SolidBrush (pickerBackcolor));
			g->fillPath (path, SolidBrush (Colors::kGray));
			
			PointF wStart (trianglePoints[0].x, trianglePoints[0].y);
			PointF wEnd ((trianglePoints[1].x + trianglePoints[2].x) * 0.5f, (trianglePoints[1].y + trianglePoints[2].y) * 0.5f);
			Color whiteFade (0xFF, 0xFF, 0xFF, 0x01);

			g->fillRect (backSize, LinearGradientBrush (wStart, wEnd, Colors::kWhite, whiteFade));
						
			PointF cStart (trianglePoints[2].x, trianglePoints[2].y);
			PointF cEnd (((trianglePoints[0].x + trianglePoints[1].x) * 0.5f) + (trianglePoints[2].x * 0.1f), (trianglePoints[0].y + trianglePoints[1].y) * 0.5f);
			ColorHSL hsl (hueValue, 1, fullColorLuminance);
			Color color;
			hsl.toColor (color);
			Color colorFade (color);
			colorFade.setAlphaF (0.01f);
			
			g->fillRect (backSize, LinearGradientBrush (cStart, cEnd, color, colorFade));
			
			PointF bStart (trianglePoints[1].x, trianglePoints[1].y);
			PointF bEnd ((trianglePoints[2].x + trianglePoints[0].x) * 0.5f, (trianglePoints[2].y + trianglePoints[0].y) * 0.5f);
			PointF bFadeEnd (bStart + ((bEnd - bStart) * 0.9f));
			Color blackFade (0, 0, 0, 0x01);
			
			g->fillRect (backSize, LinearGradientBrush (bStart, bFadeEnd, Colors::kBlack, blackFade));
		}
		
		// draw shape on clipped bitmap
		if(AutoPtr<IGraphics> g = GraphicsFactory::createBitmapGraphics (triangleBitmap))
		{
			g->fillEllipse (backSize, SolidBrush (pickerBackcolor));
			g->addClip (path);
			g->drawImage (triangleShapeImage, backSize, backSize);
		}
	}
	
	triangleNeedUpdate = false;
}

////////////////////////////////////////////////////////////////////////////

PointF ColorPicker::HSLColorWheel::getDefaultTrianglePoint (int index) const
{
	Rect triangleSize (getPartSize (kColorTriangle));
	PointF p (triangleSize.right, triangleSize.getCenter ().y);
			
	if(index < 2)
	{
		Transform matrix;
		matrix.translate (triangleSize.getCenter ().x, triangleSize.getCenter ().y);
		matrix.rotate (Math::degreesToRad ((index == 0) ? -120.f : 120.f));
		matrix.translate (-triangleSize.getCenter ().x, -triangleSize.getCenter ().y);
		matrix.transform (p);
	}
	
	return p;
}

////////////////////////////////////////////////////////////////////////////

void ColorPicker::HSLColorWheel::resetExactHandlePosition ()
{
	exactSLHandlePosition.x = 0;
	exactSLHandlePosition.y = 0;
}

////////////////////////////////////////////////////////////////////////////

Rect ColorPicker::HSLColorWheel::getPartSize (PartCode which) const
{
	switch(which)
	{
	case kColorWheel:
		{
			if(colorColorWheelImage && colorColorWheelImage->getWidth () != 0)
				return Rect (0, 0, colorColorWheelImage->getWidth (), colorColorWheelImage->getHeight ());
		
			Rect rect;
			getClientRect (rect);
			Coord edge (ccl_min (rect.getWidth (), rect.getHeight ()) - (2 * colorWheelOffset));
			
			return Rect (0, 0, edge, edge);			
		}
	case kColorTriangle:
		{
			if(triangleShapeImage && triangleShapeImage->getWidth () != 0)
				return Rect (0, 0, triangleShapeImage->getWidth (), triangleShapeImage->getHeight ());
			
			Rect rect;
			getClientRect (rect);
			Coord edge (ccl_min (rect.getWidth (), rect.getHeight ()) - (2 * colorWheelOffset));
			edge = ccl_to_int(edge * (1 - colorWheelRatio));
		
			return Rect (0, 0, edge, edge);
		}
	}
	
	return Rect ();
}
////////////////////////////////////////////////////////////////////////////

Rect ColorPicker::HSLColorWheel::getPartRect (PartCode which) const
{
	Rect rect;
	getClientRect (rect);
	Rect colorWheelRect (getPartSize (which));
	colorWheelRect.center (rect);
	return colorWheelRect; 
}

////////////////////////////////////////////////////////////////////////////

void ColorPicker::HSLColorWheel::draw (const DrawEvent& event)
{
	if(!initialized)
		initialize ();
		
	if(triangleNeedUpdate)
		updateTriangle ();
		
	Rect triangleSize (getPartSize (kColorTriangle));
	Rect triangleRect (getPartRect (kColorTriangle));
	
	event.graphics.drawImage (colorColorWheelImage, getPartSize (kColorWheel), getPartRect (kColorWheel));
	drawColorWheelHandle (event.graphics);
	event.graphics.drawImage (triangleBitmap, triangleSize, triangleRect);
	drawColorTriangleHandle (event.graphics);
}

////////////////////////////////////////////////////////////////////////////

void ColorPicker::HSLColorWheel::drawColorWheelHandle (IGraphics& graphics)
{	
	Rect handleRect (0, 0, handleSize, handleSize);
	Point handleOffset (handleRect.getRightBottom () * -0.5);
	Point wheelPosition (getWheelPosition ());
	
	wheelPosition.offset (handleOffset);
	handleRect.offset (wheelPosition);
	
	Color color;
	Color border;
	ColorHSL hsl (hslColor.h, 1, 0.5f);
	hsl.toColor (color);
		
	graphics.drawEllipse (handleRect, Pen (Colors::kBlack, 1.5));
	handleRect.contract (1);
	graphics.fillEllipse (handleRect, SolidBrush (color));
	graphics.drawEllipse (handleRect, Pen (Colors::kWhite, 1));
}

////////////////////////////////////////////////////////////////////////////

void ColorPicker::HSLColorWheel::drawColorTriangleHandle (IGraphics& graphics)
{	
	Rect handleRect (0, 0, handleSize, handleSize);
	Point handleOffset (handleRect.getRightBottom () * -0.5);
	Point trianglePosition (getTrianglePosition ());

	trianglePosition.offset (handleOffset);
	handleRect.offset (trianglePosition);
	
	Color color;
	Color border;
	hslColor.toColor (color);
		
	graphics.drawEllipse (handleRect, Pen (Colors::kBlack, 1.5));
	handleRect.contract (1);
	graphics.fillEllipse (handleRect, SolidBrush (color));
	graphics.drawEllipse (handleRect, Pen (Colors::kWhite, 1));
}

////////////////////////////////////////////////////////////////////////////

void ColorPicker::HSLColorWheel::setHSLValue (float h, float s, float l, PointRef _exactSLHandlePosition)
{
	exactSLHandlePosition = _exactSLHandlePosition;
	
	bool shouldReset = false;
	
	if(hslColor.h != h)
	{
		hslColor.h = h;
		triangleNeedUpdate = true;
	}
		
	if(l >= 0)
		hslColor.l = l;
	else if((hslColor.l < 0.05f) || (hslColor.l > 0.95f))
		shouldReset = true;
		
	if(s >= 0)
		hslColor.s = s;
	else if(hslColor.s < 0.02f)
		shouldReset = true;
		
	if(shouldReset)
	{
		hslColor.s = 1;
		hslColor.l = 0.5f;	
	}
	
	Color color;
	hslColor.toColor (color);
	
	hslH->setValue (hslColor.h, true);
	hslS->setValue (hslColor.s, true);
	hslL->setValue (hslColor.l, true);
	
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float ColorPicker::HSLColorWheel::getColorWheelRadius (RadiusCode which) const
{
	switch(which)
	{
	case kInner:
		return getPartSize (kColorWheel).getWidth () * ((1 - colorWheelRatio) / 2.f);
	case kOuter:
		return getPartSize (kColorWheel).getWidth () / 2.f;
	case kMiddle:
		return getPartSize (kColorWheel).getWidth () * ((1 - (colorWheelRatio / 2)) / 2.f);
	default:
		return 0.f; // cannot happen (just to please the compiler)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point ColorPicker::HSLColorWheel::getWheelPosition () const
{
	float theta = Math::degreesToRad (hslColor.h);
	CoordF x, y;	
	Math::polarToCartesian (x, y, theta, getColorWheelRadius (kMiddle));
	Rect rect (getPartRect (kColorWheel));
	return Point (rect.getCenter ().x + ccl_to_int (x), rect.getCenter ().y + ccl_to_int (y));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point ColorPicker::HSLColorWheel::getTrianglePosition () const
{
	if(!exactSLHandlePosition.isNull ())
		return exactSLHandlePosition;
		
	// derive position from hslColor
	Rect triangleSize (getPartRect (kColorTriangle));	
	Point frameOffset (triangleSize.getLeftTop ());
	
	triangleSize.moveTo (Point ());
	
	float hueValue = hslColor.h;
	
	Vector<Point> dtp;
	
	PointF offsetP (getDefaultTrianglePoint (0) * -1);
	Point dtpOffset (ccl_to_int (offsetP.x), ccl_to_int (offsetP.y));
	
	dtp.add (Point ());
	
	PointF second (getDefaultTrianglePoint (1));
	dtp.add (Point (ccl_to_int (second.x), ccl_to_int (second.y)));
	
	PointF third (getDefaultTrianglePoint (2));
	dtp.add (Point (ccl_to_int (third.x), ccl_to_int (third.y)));
	
	dtp[1].offset (dtpOffset);
	dtp[2].offset (dtpOffset);

	float xFactor = 1;
	if(hslColor.l <= 0.5f)
	 	xFactor = hslColor.l * 2;
	else
		xFactor = (1 - (hslColor.l - 0.5f) * 2);
	
	Point cp (ccl_to_int (hslColor.s * (dtp[2].x * xFactor)), ccl_to_int ((1 - hslColor.l) * dtp[1].y));
	cp.offset (dtpOffset * -1);
		
	Transform matrix;
	matrix.translate (triangleSize.getCenter ().x, triangleSize.getCenter ().y);
	matrix.rotate (Math::degreesToRad (hueValue));
	matrix.translate (-triangleSize.getCenter ().x, -triangleSize.getCenter ().y);
	matrix.transform (cp);
	
	cp.offset (frameOffset);
	
	return cp;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorPicker::HSLColorWheel::isInsideColorWheel (PointRef where) const
{
	Point centerWhere (where);
	Rect rect;
	getClientRect (rect);
	Point center (rect.getCenter ());
	centerWhere.offset (center * -1);

	double theta, radius;
	Math::cartesianToPolar (theta, radius, (double)centerWhere.x, (double)centerWhere.y);

	if((radius > getColorWheelRadius (kInner)) && (radius < getColorWheelRadius (kOuter)))
		return true;	

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMouseHandler* ColorPicker::HSLColorWheel::createMouseHandler (const MouseEvent& event)
{
	if(isInsideColorWheel (event.where))
		return NEW ColorPickerHueMouseHandler (this, getPartRect (kColorWheel));
	else if(isInsideColorTriangle (event.where))
		return NEW ColorPickerSLMouseHandler (this, getPartRect (kColorTriangle), hslColor.h);
		
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorPicker::HSLColorWheel::isInsideColorTriangle (PointRef where) const
{	
	Rect triangleRect (getPartRect (kColorTriangle));
	Point offsetWhere (where);
	offsetWhere.offset (triangleRect.getLeftTop () * -1);
	
	float cross0 = ((trianglePoints[1].y - trianglePoints[0].y) * (offsetWhere.x - trianglePoints[0].x)) - ((trianglePoints[1].x - trianglePoints[0].x) * (offsetWhere.y - trianglePoints[0].y));
	float cross1 = ((trianglePoints[2].y - trianglePoints[1].y) * (offsetWhere.x - trianglePoints[1].x)) - ((trianglePoints[2].x - trianglePoints[1].x) * (offsetWhere.y - trianglePoints[1].y));
	float cross2 = ((trianglePoints[0].y - trianglePoints[2].y) * (offsetWhere.x - trianglePoints[2].x)) - ((trianglePoints[0].x - trianglePoints[2].x) * (offsetWhere.y - trianglePoints[2].y));
	
	return ((cross0 > 0) && (cross1 > 0) && (cross2 > 0)) ? true : false; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum ColorPickerTags
	{
		kPickerMode,
		kAddColor,
		kRemoveColor,
		kResetColors,
		kRestorePreset,
		kHex,
		kRedChannel,
		kGreenChannel,
		kBlueChannel,
		kHue,
		kSaturation,
		kLuminance,
		kHSLWheelMode
	};
}

//************************************************************************************************
// ColorPicker::RGBSlider
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ColorPicker::RGBSlider, UserControl)

////////////////////////////////////////////////////////////////////////////

ColorPicker::RGBSlider::RGBSlider (IParameter* _mainParameter, IParameter* _leftParameter, IParameter* _rightParameter, RectRef size, int channelTag, StyleRef customStyle, StringRef title)
: UserControl (size, customStyle, title),
  channelTag (channelTag),
  mainParameter (nullptr),
  leftParameter (nullptr),
  rightParameter (nullptr),
  backRadius (0),
  margin (0),
  gradientNeedsUpdate (false)
{
	share_and_observe_unknown (this, mainParameter, _mainParameter);
	share_and_observe_unknown (this, leftParameter, _leftParameter);
	share_and_observe_unknown (this, rightParameter, _rightParameter);
}

////////////////////////////////////////////////////////////////////////////

ColorPicker::RGBSlider::~RGBSlider ()
{
	share_and_observe_unknown<IParameter> (this, mainParameter, nullptr);
	share_and_observe_unknown<IParameter> (this, leftParameter, nullptr);
	share_and_observe_unknown<IParameter> (this, rightParameter, nullptr);
}

////////////////////////////////////////////////////////////////////////////

void ColorPicker::RGBSlider::attached (IView* parent)
{
	initialize ();
}

////////////////////////////////////////////////////////////////////////////

void ColorPicker::RGBSlider::initialize ()
{
	const IVisualStyle& vs = getVisualStyle ();

	margin = vs.getMetric ("margin", margin);
	backRadius = vs.getMetric ("backradius", backRadius);
	
	// create background
	gradientImage = GraphicsFactory::createShapeImage ();
		
	// create slider
	ITheme* theme = RootComponent::instance ().getTheme ();
	ASSERT (theme != nullptr)
	if(theme)
	{
		Rect rect (getSize ());
		rect.moveTo (Point ());
		ControlBox rgbSlider (ClassID::Slider, mainParameter, rect, StyleFlags (Styles::kTransparent));
		rgbSlider.setVisualStyle (vs);
		rgbSlider.setSizeMode (IView::kAttachAll);
		getChildren ().add (rgbSlider);
		initialized = true;
		gradientNeedsUpdate = true;
	}
}

////////////////////////////////////////////////////////////////////////////

void ColorPicker::RGBSlider::draw (const DrawEvent& event)
{
	if(!initialized)
		initialize ();
		
	if(gradientNeedsUpdate)
		updateGradient ();
	
	// draw the background
	Rect rect (getSize ());
	rect.moveTo (Point ());
	event.graphics.drawImage (gradientImage, rect, rect);
	
	 // draw the slider
	SuperClass::draw (event);
}

////////////////////////////////////////////////////////////////////////////

void ColorPicker::RGBSlider::updateGradient ()
{
	AutoPtr<IGraphics> g = GraphicsFactory::createShapeBuilder (gradientImage);	
	
	PointF start (getSize ().left, getSize ().top);
	PointF end (getSize ().right, getSize ().top);
	
	Color startColor;
	Color endColor;
	
	switch(channelTag)
	{
	default :
		break;
		
	case Tag::kRedChannel :
		{
			startColor = Color (0, (uint8)leftParameter->getValue ().asInt (), (uint8)rightParameter->getValue ().asInt ());
			endColor = Color (255, (uint8)leftParameter->getValue ().asInt (), (uint8)rightParameter->getValue ().asInt ());
		}
		break;		
			
	case Tag::kGreenChannel :
		{
			startColor = Color ((uint8)leftParameter->getValue ().asInt (), 0, (uint8)rightParameter->getValue ().asInt ());
			endColor = Color ((uint8)leftParameter->getValue ().asInt (), 255, (uint8)rightParameter->getValue ().asInt ());
		}
		break;		
			
	case Tag::kBlueChannel :
		{
			startColor = Color ((uint8)leftParameter->getValue ().asInt (), (uint8)rightParameter->getValue ().asInt (), 0);
			endColor = Color ((uint8)leftParameter->getValue ().asInt (), (uint8)rightParameter->getValue ().asInt (), 255);
		}
		break;		
	}
	
	Rect rect (getSize ());
	rect.moveTo (Point ());
	rect.contract (margin);
	g->fillRoundRect (rect, backRadius, backRadius, LinearGradientBrush (start, end, startColor, endColor));
	
	gradientNeedsUpdate = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
void CCL_API ColorPicker::RGBSlider::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
		if(isEqualUnknown (subject, leftParameter) || isEqualUnknown (subject, rightParameter))
		{
			gradientNeedsUpdate = true;
			invalidate ();
		}
}

//************************************************************************************************
// CustomColorPresets
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CustomColorPresets, Object)
DEFINE_SINGLETON (CustomColorPresets)

////////////////////////////////////////////////////////////////////////////

CustomColorPresets::CustomColorPresets ()
: palette (ccl_new<IColorPalette> (ClassID::ColorPalette)),
  paletteInitialized (false),
  presetCategory (CCLSTR ("ColorPalette")),
  presetClassName (getFileType ().getDescription ())
{	
	ASSERT (palette != nullptr)
	paletteFile = NEW ColorPaletteFile (palette);	

	SimplePresetHandler* handler = NEW SimplePresetHandler (getFileType ());
	handler->setPresetFolderName (CCLSTR ("Color Palettes"));
	handler->setPresetCategory (presetCategory);
	handler->setPresetClassName (presetClassName);
	handler->registerSelf ();
}

////////////////////////////////////////////////////////////////////////////

void CustomColorPresets::initializePalette (bool loadDefault)
{
	ITheme* theme = RootComponent::instance ().getTheme ();
	if(theme && palette)
	{
		if(!loadDefault)
			loadDefault = (palette->getCount () == 0) ? true : false;
		
		if(loadDefault)
		{
			palette->removeAll ();
			palette->fromStyle (theme->getStyle ("Standard.ColorPickerPalette"));
			palette->appendColor (Colors::kTransparentBlack);
		}
		else
		{
			palette->fromStyle (theme->getStyle ("Standard.ColorPickerMetrics"));
		}
			
		paletteInitialized = true;
	}
}

////////////////////////////////////////////////////////////////////////////

IColorPalette* CustomColorPresets::getPalette () const
{
	if(!paletteInitialized)
		const_cast<CustomColorPresets*> (this)->initializePalette ();

	return palette;
}

////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API CustomColorPresets::getPresetTarget ()
{
	return paletteFile; 
}

////////////////////////////////////////////////////////////////////////////

tbool CCL_API CustomColorPresets::getPresetMetaInfo (IAttributeList& metaInfo)
{
	PresetMetaAttributes attributes (metaInfo);
	attributes.setCategory (presetCategory);
	attributes.setClassName (presetClassName);
	return true;
}

////////////////////////////////////////////////////////////////////////////

const FileType& CustomColorPresets::getFileType ()
{
	static FileType fileType (nullptr, "colorpalette", CCL_MIME_TYPE "-colorpalette+json");
	return FileTypes::init (fileType, XSTR (ColorPalette));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomColorPresets::getUserPresetPath (Url& userPath) const
{
	FileType fileType;
	paletteFile->getFormat (fileType);

	System::GetSystem ().getLocation (userPath, System::kAppSettingsFolder);
	userPath.descend ("user");
	userPath.setFileType (fileType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomColorPresets::restoreUserPreset ()
{
	Url userPath;
	getUserPresetPath (userPath);
	
	if(System::GetFileSystem ().fileExists (userPath))
		if(AutoPtr<IStream> stream = System::GetFileSystem ().openStream (userPath))
			if(paletteFile->load (*stream))
				return;
		
	// fallback
	initializePalette (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomColorPresets::storeUserPreset ()
{
	Url userPath;
	getUserPresetPath (userPath);

	if(AutoPtr<IStream> stream = System::GetFileSystem ().openStream (userPath, IStream::kCreateMode))
		paletteFile->save (*stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomColorPresets::restoreLastPreset (UrlRef presetPath)
{		
	if(System::GetFileSystem ().fileExists (presetPath))
		if(AutoPtr<IStream> stream = System::GetFileSystem ().openStream (presetPath))
			if(paletteFile->load (*stream))
				return true;

	// fallback
	restoreUserPreset ();
	return false;
}

//************************************************************************************************
// ColorPicker
//************************************************************************************************
 
DEFINE_CLASS (ColorPicker, Component)
DEFINE_CLASS_UID (ColorPicker, 0x97A35F04, 0xA7AE, 0x4ACC, 0x88, 0xF9, 0xF4, 0x81, 0x4F, 0xDD, 0x78, 0x2A)
DEFINE_CLASS_NAMESPACE (ColorPicker, "Host")
bool ColorPicker::hslWheelMode = true;

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorPicker::ColorPicker ()
: Component (CCLSTR ("ColorPicker")),
  paletteModel (nullptr),
  pickerPalette (nullptr),
  parameter (nullptr),
  hslDirty (false),
  deferAcceptOnMouseUp (false),
  shouldEndPreview (false),
  colorWasChangedInPickerMode (false),
  presetComponent (nullptr),
  currentPaletteCount (0)
{
	paramList.addParam ("pickerMode", Tag::kPickerMode); // paletteOnlyMode or pickerMode
	paramList.addParam ("addColor", Tag::kAddColor);
	paramList.addParam ("removeColor", Tag::kRemoveColor);
	paramList.addParam ("resetColors", Tag::kResetColors);
	paramList.addParam ("restorePreset", Tag::kRestorePreset);
	paramList.addString ("hex", Tag::kHex);
	paramList.addInteger (0, 255, "red", Tag::kRedChannel);
	paramList.addInteger (0, 255, "green", Tag::kGreenChannel);
	paramList.addInteger (0, 255, "blue", Tag::kBlueChannel);
	paramList.addInteger (0, 359, "hue", Tag::kHue);
	paramList.addFloat (0, 1, "saturation", Tag::kSaturation)->setFormatter (AutoPtr<IFormatter> (NEW Format::Percent));
	paramList.addFloat (0, 1, "luminance", Tag::kLuminance)->setFormatter (AutoPtr<IFormatter> (NEW Format::Percent));
	paramList.addParam ("hslWheelMode", Tag::kHSLWheelMode)->setValue (hslWheelMode);

	paletteModel = ccl_new<IColorPaletteModel> (ClassID::ColorPaletteModel);
		
	ISubject::addObserver (paletteModel, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorPicker::ColorPicker (IParameter* _parameter, bool applyPresetPalette)
: ColorPicker ()
{
	construct (_parameter, applyPresetPalette);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorPicker::~ColorPicker ()
{
	ISubject::removeObserver (paletteModel, this);
	safe_release (paletteModel);
	
	if(pickerPalette)
		ISubject::removeObserver (pickerPalette, this);
	
	share_and_observe_unknown<IParameter> (this, parameter, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorPicker::construct (IParameter* _parameter, bool applyPresetPalette)
{
	share_and_observe_unknown (this, parameter, _parameter);
		
	UnknownPtr<IPaletteProvider> provider (parameter);	
	
	if(applyPresetPalette) 
	{
		// create optional preset component when shared palette from CustomColorPresets is used
		presetComponent = NEW PresetComponent (&CustomColorPresets::instance ());
		presetComponent->setPresetType (MutableCString (CustomColorPresets::getFileType ().getMimeType ()));
		presetComponent->setOptions (0);
		presetComponent->setCurrentPresetName (nullptr);
		addComponent (presetComponent);
		
		pickerPalette = CustomColorPresets::instance ().getPalette ();
		if(provider) //assign shared palette to parameter 
			provider->setPalette (pickerPalette);
	}
	else
	{
		pickerPalette = provider ? provider->getPalette () : nullptr;
	}
	
	ISubject::addObserver (pickerPalette, this);
	
	ASSERT (pickerPalette) // parameter has no palette associated
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorPicker::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		if(isEqualUnknown (subject, parameter))
		{
			if(UnknownPtr<IColorParam> colorParam = parameter)
			{
				Color color;
				colorParam->getColor (color);
				syncParametersFromColor (color);

				if(isInPickerMode ())
				{	
					paletteModel->setFocusColor (color);
					colorWasChangedInPickerMode = true;					
				}
			}
		}
		else if(isEqualUnknown (subject, pickerPalette))
		{
			int newPaletteCount = pickerPalette->getCount ();
			if(currentPaletteCount != newPaletteCount)
			{
				currentPaletteCount = newPaletteCount;
				paramList.byTag (Tag::kRemoveColor)->enable (currentPaletteCount > kMinColors);
				paramList.byTag (Tag::kAddColor)->enable (currentPaletteCount < kMaxColors);
			}
		}
	}
	else if(msg == IColorPaletteModel::kFocusColorChanged)
	{	
		syncParametersFromColor (paletteModel->getFocusColor ());
		
		shouldEndPreview = true;
		setHSLDirty (true);
	}
	else
	{
		SuperClass::notify (subject, msg);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorPicker::syncParametersFromColor (ColorRef color)
{
	String s; 
	Colors::toString (color, s);
	String simplified (s.subString (1).truncate (6));
	paramList.byTag (Tag::kHex)->fromString (simplified);
	
	paramList.byTag (Tag::kRedChannel)->setValue (color.red);
	paramList.byTag (Tag::kGreenChannel)->setValue (color.green);
	paramList.byTag (Tag::kBlueChannel)->setValue (color.blue);
	
	if(isHSLDirty ())
	{
		ColorHSL hslColor;
		hslColor.fromColor (color);
					
		paramList.byTag (Tag::kHue)->setValue (hslColor.h);
		paramList.byTag (Tag::kSaturation)->setValue (hslColor.s);
		paramList.byTag (Tag::kLuminance)->setValue (hslColor.l);
		
		setHSLDirty (false);
	}
}
		
//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorPicker::isInPickerMode () const
{
	return paramList.byTag (Tag::kPickerMode)->getValue ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorPicker::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kPickerMode :
		{
			if(UnknownPtr<IPaletteItemModel> paletteItemModel = paletteModel)
			{				
				if(ViewBox boxed = paletteItemModel->getItemView ())
				{
					ViewBox::StyleModifier (boxed).setCustomStyle (Styles::kItemViewBehaviorAutoSelect, !param->getValue ());
					
					if(param->getValue ())
					{
						// enter picker mode should end preview
						if(shouldEndPreview) 
						{
							paletteItemModel->finishPreview ();
							Variant value (parameter->getValue ());
							paletteItemModel->setFocusIndex (pickerPalette->getIndex (value));

							shouldEndPreview = false;
						}
												
						acceptOnMouseUp (false); // Don't close popup in picker mode
						
						int oldIndex = paletteItemModel->getFocusIndex ();
						int transparentIndex = pickerPalette->getCount () - 1;
						paletteModel->removeColor (transparentIndex); // don't show last transparent color in picker mode 
						if(oldIndex != transparentIndex)
							paletteItemModel->setFocusIndex (oldIndex);
					}
					else
					{
						deferAcceptOnMouseUp = true; // Accept mouse up in palette mode - after changing the mode (not right now) 
						acceptOnDoubleClick (false);
						
						int oldIndex = paletteItemModel->getFocusIndex ();
						paletteModel->addColor (Colors::kTransparentBlack); // add new transparent color
						paletteItemModel->setFocusIndex (oldIndex);
					}
				}
			}
		}
		return true;
	
	case Tag::kAddColor :
		{
			if(param->getValue ())
				addCurrentColor ();
		}
		return true;	
	
	case Tag::kRemoveColor :
		{
			if(param->getValue ())
				removeSelectedColor ();
		}
		return true;
		
	case Tag::kResetColors :
		{
			if(param->getValue ())
			{
				resetColors ();
				setHSLDirty (true);
				if(UnknownPtr<IColorParam> colorParam = parameter)
					colorParam->setColor (paletteModel->getFocusColor (), true);
			}
		}
		return true;
		
	case Tag::kRestorePreset :
		{
			if(param->getValue ())
			{
				restorePreset ();
				setHSLDirty (true);
				if(UnknownPtr<IColorParam> colorParam = parameter)
					colorParam->setColor (paletteModel->getFocusColor (), true);
			}
		}
		return true;
		
	case Tag::kHex :
		{
			setHSLDirty (true);
			Color color;
			if(getColorFromHexString (color))
				if(UnknownPtr<IColorParam> colorParam = parameter)
					colorParam->setColor (color);
		}
		return true;
		 
		 
	case Tag::kRedChannel :
	case Tag::kGreenChannel :
	case Tag::kBlueChannel :
		{
			setHSLDirty (true);
			uint8 value = (uint8)param->getValue ().asInt ();
			if(UnknownPtr<IColorParam> colorParam = parameter)
			{
				Color color;
				colorParam->getColor (color);
				switch(param->getTag ())
				{
				case Tag::kRedChannel:
					color.red = value;
					break;
				case Tag::kGreenChannel:
					color.green = value;
					break;
				case Tag::kBlueChannel:
					color.blue = value;
					break;
				default: 
					break;
				}
				
				colorParam->setColor (color);
			}
		}
		return true;
	
	case Tag::kHue :
	case Tag::kSaturation :
	case Tag::kLuminance :
		{
			Color color;
			ColorHSL hslColor;
			hslColor.h = paramList.byTag (Tag::kHue)->getValue ().asInt ();
			hslColor.s = paramList.byTag (Tag::kSaturation)->getValue ().asFloat ();
			hslColor.l = paramList.byTag (Tag::kLuminance)->getValue ().asFloat ();
			hslColor.toColor (color);
			
			if(UnknownPtr<IColorParam> colorParam = parameter)
				colorParam->setColor (color);
		}
		return true;
	
	case Tag::kHSLWheelMode :
		{
			hslWheelMode = param->getValue ().asBool ();
		}
		return true;
		
	default:
		return SuperClass::paramChanged (param);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorPicker::addCurrentColor ()
{
	if(pickerPalette->getCount () < kMaxColors)
	{
		if(UnknownPtr<IColorParam> colorParam = parameter)
		{
			Color color;
			paletteModel->addColor (colorParam->getColor (color));
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorPicker::removeSelectedColor ()
{
	if(pickerPalette->getCount () > kMinColors)
	{
		paletteModel->removeColor ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorPicker::resetColors ()
{
	if(hasPresetPalette ()) // reset shared palette
	{
		CustomColorPresets::instance ().initializePalette (true);  // true: load default palette 
		
		if(isInPickerMode ()) // no trailing transparent color shown in picker mode -> remove it
			UnknownPtr<IColorPalette> (pickerPalette)->removeColors (pickerPalette->getCount () - 1);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorPicker::restorePreset ()
{
	if(presetComponent)
	{
		bool success = CustomColorPresets::instance ().restoreLastPreset (presetComponent->getCurrentPresetUrl ());
		// returns true when restore was successful
		// returns false when user preset restored as fallback 
		
		// remove transparent color from user or init-preset in picker mode
		if(!success && isInPickerMode ()) 
			UnknownPtr<IColorPalette> (pickerPalette)->removeColors (pickerPalette->getCount () - 1);
	}
	else
	{
		resetColors ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorPicker::getColorFromHexString (Color& color) const
{
	String hexCode;
	paramList.byTag (Tag::kHex)->toString (hexCode);
	MutableCString s ("#");
	s.append (hexCode);
	return (Colors::fromCString (color, s)) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorPicker::hasPresetPalette () const
{
	return (pickerPalette == CustomColorPresets::instance ().getPalette ()) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorPicker::hasPresets () const
{
	return presetComponent ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ColorPicker::getObject (StringID name, UIDRef classID)
{
	if(name == "paletteModel")
		return paletteModel;
	
	return SuperClass::getObject (name, classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API ColorPicker::createPopupView (SizeLimit& limits)
{
	initializePopup ();
	ITheme* theme = getTheme ();
	IView* view = theme->createView ("Standard.ColorPickerPopup", asUnknown ());
	if(!view)
	{
		ITheme* theme2 = System::GetThemeManager ().getApplicationTheme ();
		if(theme2 && theme2 != theme)
			view = theme2->createView ("Standard.ColorPickerPopup", asUnknown ());
	}
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorPicker::initializePopup ()
{	
	// initModel: set previewHandler
	if(UnknownPtr<IPaletteItemModel> paletteItemModel = paletteModel)
	{
		UnknownPtr<IParamPreviewHandler> previewHandler (parameter->getController ());
		paletteItemModel->initModel (pickerPalette, parameter, previewHandler);
		
		Variant value (parameter->getValue ());
		paletteItemModel->setFocusIndex (pickerPalette->getIndex (value));
	}

	// update picker control mode (hslWheel or rgbSliders)
	paramList.byTag (Tag::kHSLWheelMode)->setValue (hslWheelMode);
	
	// init flags 
	acceptAfterSwipe (true);
	acceptOnMouseUp (true);
	acceptOnMouseDown (false);
	acceptOnDoubleClick (false);
	shouldEndPreview = false;
	colorWasChangedInPickerMode = false;
}		
		
//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API ColorPicker::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "HSLColorWheel")
		return *NEW HSLColorWheel (paramList.byTag (Tag::kHue), paramList.byTag (Tag::kSaturation), paramList.byTag (Tag::kLuminance), bounds); 
	if(name == "RGBSliderRed")
		return *NEW RGBSlider (paramList.byTag (Tag::kRedChannel), paramList.byTag (Tag::kGreenChannel), paramList.byTag (Tag::kBlueChannel), bounds, Tag::kRedChannel); 
	if(name == "RGBSliderGreen")
		return *NEW RGBSlider (paramList.byTag (Tag::kGreenChannel), paramList.byTag (Tag::kRedChannel), paramList.byTag (Tag::kBlueChannel), bounds, Tag::kGreenChannel); 
	if(name == "RGBSliderBlue")
		return *NEW RGBSlider (paramList.byTag (Tag::kBlueChannel), paramList.byTag (Tag::kRedChannel), paramList.byTag (Tag::kGreenChannel), bounds, Tag::kBlueChannel); 
	
	return SuperClass::createView (name, data, bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API ColorPicker::onMouseDown (const MouseEvent& event, IWindow& popupWindow)
{	
	// check if acceptOnDoubleClick makes sense (accept double click on palette when in picker mode)
	if(isInPickerMode ())
	{
		if(UnknownPtr<IPaletteItemModel> paletteItemModel = paletteModel)
		{
			if(UnknownPtr<IView> view = paletteItemModel->getItemView ())
			{
				Point bottomRight (view->getSize ().getRightBottom ());
				
				bool onPalette = (event.where.y > view->clientToWindow (bottomRight).y) ? false : true;
				acceptOnDoubleClick (onPalette);
			}
		}
	}
	
	// select current focusColor (onPopupClosed), and close the popup afterwards (when acceptOnMouseUp is true === not in picker mode)
	Result result = PopupSelectorClient::onMouseDown (event, popupWindow);
	
	if(deferAcceptOnMouseUp)
	{
		acceptOnMouseUp (true);
		deferAcceptOnMouseUp = false;
	}
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API ColorPicker::onMouseUp (const MouseEvent& event, IWindow& popupWindow)
{
	UnknownPtr<IView> view (&popupWindow);
	if(view)
	{
		Rect clientRect;
		view->getVisibleClient (clientRect);
		clientRect.contract (2); // drop mouse up events at edge of popupWindow (might happen when opening on double click)
		if(!clientRect.pointInside (event.where))
			return kIgnore;
	}
	return PopupSelectorClient::onMouseUp (event, popupWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API ColorPicker::onEventProcessed (const GUIEvent& event, IWindow& popupWindow, IView* view)
{
	// accept on single tap in palette list view (close, no preview on touch input)
	if(auto gestureEvent = event.as<GestureEvent> ())
	{
		if(gestureEvent->getState () == GestureEvent::kBegin && gestureEvent->getType () == GestureEvent::kSingleTap)
		{
			UnknownPtr<IItemView> listView (view);
			if(listView && isEqualUnknown (listView->getModel (), paletteModel))
				return IPopupSelectorClient::kOkay;
		}
	}
	return PopupSelectorClient::onEventProcessed (event, popupWindow, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorPicker::onPopupClosed (Result result)
{
	if(shouldEndPreview)
	{
		if(UnknownPtr<IPaletteItemModel> paletteItemModel = paletteModel)
		{
			paletteItemModel->finishPreview ();
			paletteItemModel->initModel (pickerPalette, parameter, nullptr); // avoid following preview calls 
		}
		shouldEndPreview = false;
	}
		
	if(result == IPopupSelectorClient::kOkay || colorWasChangedInPickerMode)
	{
		if(UnknownPtr<IPaletteItemModel> paletteItemModel = paletteModel)
		{
			Variant value (pickerPalette->getAt (paletteItemModel->getFocusIndex ()));
			parameter->beginEdit ();
			parameter->setValue (value, false);
			parameter->performUpdate (); // trigger update even if color has not changed
			parameter->endEdit ();
		}
	}
	
	// reset to paletteOnlyMode
	paramList.byTag (Tag::kPickerMode)->setValue (false, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorPicker::popup (IVisualStyle* popupStyle, bool useMousePos)
{
	AutoPtr<IPopupSelector> popupSelector (ccl_new<IPopupSelector> (ClassID::PopupSelector));
	ASSERT (popupSelector)

	int placementFlags = useMousePos ? PopupSizeInfo::kHMouse | PopupSizeInfo::kVMouse : PopupSizeInfo::kHCenter|PopupSizeInfo::kVCenter;
	PopupSizeInfo sizeInfo (nullptr, placementFlags);

	popupSelector->setTheme (getTheme ());
	popupSelector->setVisualStyle (popupStyle);
	return popupSelector->popup (this, sizeInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ColorPicker)
	DEFINE_METHOD_ARGS ("construct", "colorParam: Parameter, applyPresetPalette: bool = true")
	DEFINE_METHOD_ARGR ("popup", "popupStyle: VisualStyle = null, useMousePos : bool = false", "bool")
END_METHOD_NAMES (ColorPicker)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorPicker::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "construct")
	{
		UnknownPtr<IParameter> parameter (msg[0].asUnknown ());
		bool applyPresetPalette = msg.getArgCount () > 1 ? msg[1].asBool () : true;
		ASSERT (parameter)

		construct (parameter, applyPresetPalette);
		return true;
	}
	else if(msg == "popup")
	{
		UnknownPtr<IVisualStyle> popupStyle = msg.getArgCount () > 0 ? msg[0].asUnknown () : nullptr;
		bool useMousePos = msg.getArgCount () > 1 && msg[1].asBool ();

		returnValue = popup (popupStyle, useMousePos);
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorPicker::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "hasPresets")
	{
		var = hasPresets ();
		return true;
	}
	if(propertyId == "hasPresetPalette")
	{
		var = hasPresetPalette ();
		return true;
	}	
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// ColorPickerDialog
//************************************************************************************************

bool ColorPickerDialog::run (Color& color)
{
	#if CCL_PLATFORM_DESKTOP
	AutoPtr<ColorParam> param = NEW ColorParam;
	param->setColor (color);
	if(ColorPicker (param).popup ())
	{
		param->getColor (color);
		return true;
	}
	#endif
	return false;
}