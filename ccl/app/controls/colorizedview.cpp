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
// Filename    : ccl/app/controls/colorizedview.cpp
// Description : Colorized View
//
//************************************************************************************************

#include "ccl/app/controls/colorizedview.h"

#include "ccl/base/collections/objectlist.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/graphics/brush.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/framework/themeelements.h"

using namespace CCL;

//************************************************************************************************
// ColorManipulator
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ColorManipulator, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorManipulator::ColorManipulator (const IVisualStyle* vs)
: luminanceWeight (.5f),
  luminanceWeightSelected (.5f),
  saturationWeight (.5f),
  saturationWeightSelected (.5f),
  saturation (.5f),
  saturationSelected (.5f),
  brightness (.5f),
  brightnessSelected (.5f),
  opacity (1.f),
  opacitySelected (1.f),
  visualStyle (vs)
{
	if(vs)
		updateSettings (*vs);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorManipulator::updateSettings (const IVisualStyle& vs)
{
	saturationWeight = vs.getMetric ("saturationWeight", 0.f);
	saturationWeightSelected = vs.getMetric ("saturationWeightSelected", 0.f);
	luminanceWeight = vs.getMetric ("luminanceWeight", .5f);
	luminanceWeightSelected = vs.getMetric ("luminanceWeightSelected", .5f);
	opacity = vs.getMetric ("opacity", 1.f);
	opacitySelected = vs.getMetric ("opacitySelected", 1.f);
	
	ColorHSV referenceColor = vs.getColor ("userReferenceColor", Colors::kGray);
	ColorHSV referenceColorSelected = vs.getColor ("userReferenceColorSelected", vs.getColor ("userReferenceColor", Colors::kGray));
	saturation = referenceColor.s;
	saturationSelected = referenceColorSelected.s;
	brightness = referenceColor.v;
	brightnessSelected = referenceColorSelected.v;
	
	transparentBackColor = vs.getColor ("backcolor.transparent", vs.getBackColor ());
	transparentForeColor = vs.getColor ("forecolor.transparent", vs.getForeColor ().addBrightness (0.05f));
	
	selectedCache.removeAll ();
	normalCache.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorManipulator::adjustColor (Color& color, bool selected)
{
	ColorCache& cache = selected ? selectedCache : normalCache;
	
	uint32 key (color);
	
	if(key == 0)
	{
		color = selected ? transparentForeColor : transparentBackColor;
	}
	else if(cache.lookupColor (color, key) == false)
	{
		if(selected)
			adjustColor (color, saturationSelected, brightnessSelected, saturationWeightSelected, luminanceWeightSelected, opacitySelected);
		else
			adjustColor (color, saturation, brightness, saturationWeight, luminanceWeight, opacity);
		
		cache.addColor (color, key);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorManipulator::adjustColor (Color& color, float referenceSaturation, float referenceBrightness, float referenceSaturationWeight, float referenceLuminanceWeight, float fixedOpacity)
{
	color.setAlphaF (fixedOpacity);
	ColorHSV hsv (color);
	
	if(hsv.s > 0)
		hsv.s = (hsv.s * (1 - referenceSaturationWeight)) + (referenceSaturation * referenceSaturationWeight);
	
	float currentWeight = referenceLuminanceWeight;
	
	if(currentWeight > 0)
		if(hsv.v > currentWeight)
			currentWeight = currentWeight + ((hsv.v - currentWeight) / 2.f);
	
	hsv.v = (hsv.v * (1 - currentWeight)) + (referenceBrightness * currentWeight);
	
	hsv.toColor (color);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorManipulator::ColorCache::lookupColor (Color& color, uint32 colorKey)
{
	ListForEach (cache, CacheEntry, entry)
		if(entry.colorKey == colorKey)
		{
			color (entry.cachedColor);
			return true;
		}
	EndFor

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorManipulator::ColorCache::addColor (const Color& cachedColor, uint32 colorKey)
{
	cache.append (CacheEntry (colorKey, cachedColor));
}

//************************************************************************************************
// ColorizedView
//************************************************************************************************

static ObjectList colorizedViews;

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorizedView::applyConfigurationTo (MetaClassRef typeId)
{
	ForEach (colorizedViews, ColorizedView, view)
		if(view->canCast (typeId))
			view->configurationChanged ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (ColorizedView, UserControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorizedView::ColorizedView (IColorParam* colorParam, IParameter* selectParam, RectRef size)
: UserControl (size),
  selectParam (selectParam),
  colorParam (colorParam),
  colorsNeedUpdate (true),
  colorizeStyle (true),
  mask (nullptr),
  radius (0.f),
  useGradient (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorizedView::~ColorizedView ()
{
	disposeManipulator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorizedView::configurationChanged ()
{
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorizedView::isColorizeEnabled () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorizedView::attached (IView* parent)
{
	enableUpdates (true);

	const IVisualStyle& vs = getVisualStyle ();

	mask = vs.getImage ("mask");
	radius = vs.getMetric ("radius", 0.f);
	clipRect.left   = vs.getMetric<Coord> ("clip.left", 0);
	clipRect.top    = vs.getMetric<Coord> ("clip.top", 0);
	clipRect.right  = vs.getMetric<Coord> ("clip.right", 0);
	clipRect.bottom = vs.getMetric<Coord> ("clip.bottom", 0);


	gradientBorderPen = vs.getColor ("gradientBorderColor", Colors::kTransparentBlack);
	useGradient = vs.getMetric<bool> ("gradient", useGradient);
	colorizeStyle = vs.getMetric<bool> ("colorize", colorizeStyle);

	colorsNeedUpdate = true;
	if(colorizeStyle)
		getManipulator ().updateSettings ();

	SuperClass::attached (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorizedView::removed (IView* parent)
{
	enableUpdates (false);

	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorizedView::enableUpdates (bool state)
{
	if(state)
	{
		colorizedViews.add (this);
		ISubject::addObserver (selectParam, this);
		ISubject::addObserver (colorParam, this);
	}
	else
	{
		ISubject::removeObserver (selectParam, this);
		ISubject::removeObserver (colorParam, this);
		colorizedViews.remove (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorizedView::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		if(isEqualUnknown (subject, colorParam))
			colorsNeedUpdate = true;
			
		invalidate ();
	}

	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorizedView::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	if(colorizeStyle)
		getManipulator ().updateSettings ();
	colorsNeedUpdate = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorizedView::draw (const DrawEvent& event)
{
	drawBackground (event.graphics, event.updateRgn.bounds);

	SuperClass::draw (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorManipulator& ColorizedView::getManipulator ()
{
	if(!manipulator)
		manipulator = acquireManipulator ();

	return *manipulator;
}



//////////////////////////////////////////////////////////////////////////////////////////////////

static ObjectList manipulators;

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorizedView::disposeManipulator ()
{
	ColorManipulator* cached = nullptr;
	ForEach (manipulators, ColorManipulator, manipulator)
	if(&manipulator->getVisualStyle () == &getVisualStyle ())
	{
		cached = manipulator;
		break;
	}
	EndFor
	
	if(!cached)
		return;
	
	if(cached->getRetainCount () == 2) // only one reference left -> remove from list
	{
		manipulators.remove (cached);
		cached->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorManipulator* ColorizedView::acquireManipulator ()
{
	ForEach (manipulators, ColorManipulator, manipulator)
		if(&manipulator->getVisualStyle () == &getVisualStyle ())
			return manipulator;
	EndFor

	const IVisualStyle& vs = getVisualStyle ();
	
	ColorManipulator* newManipulator = NEW ColorManipulator (&vs);
	manipulators.add (newManipulator);
	return newManipulator;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorizedView::drawBackground (IGraphics& graphics, const Rect& updateRect)
{
	bool selected = selectParam ? selectParam->getValue ().asBool () : true;
	Rect clientRect;
	getClientRect (clientRect);
	bool useUpdateRect = true;
	if(radius > 0.f)
	{
		Rect fillRect (clientRect);
		clientRect.left -= clipRect.left;
		clientRect.right += clipRect.right;
		clientRect.top -= clipRect.top;
		clientRect.bottom += clipRect.bottom;
		
		fillRect.contract (ccl_to_int (radius));
		useUpdateRect = fillRect.rectInside (updateRect);
	}

	if(isColorizeEnabled () && colorizeStyle)
	{
		if(colorsNeedUpdate)
		{
			colorsNeedUpdate = false;

			colorParam->getColor (color);
			getManipulator ().adjustColor (color, false);
			color.renderAlpha (getVisualStyle ().getBackColor ());

			colorParam->getColor (selectedColor);
			getManipulator ().adjustColor (selectedColor, true);
			selectedColor.renderAlpha (getVisualStyle ().getBackColor ());
		}

		if(radius > 0.f && (!useUpdateRect || useGradient))
		{
			if(useGradient)
			{
				LinearGradientBrush gradientBrush (pointIntToF (clientRect.getLeftTop ()), pointIntToF (clientRect.getLeftBottom ()), selectedColor, color);
				graphics.fillRoundRect (clientRect, ccl_to_int (radius), ccl_to_int (radius), gradientBrush);
				graphics.drawRoundRect (clientRect, ccl_to_int (radius), ccl_to_int (radius), gradientBorderPen);
			}
			else
			{
				graphics.fillRoundRect (clientRect, ccl_to_int (radius), ccl_to_int (radius), selected ? SolidBrush (selectedColor) : SolidBrush (color));
				graphics.drawRoundRect (clientRect, ccl_to_int (radius), ccl_to_int (radius), selected ? Pen (selectedColor) : Pen (color));
			}
		}
		else
		{
			if(useGradient)
			{
				LinearGradientBrush gradientBrush (pointIntToF (clientRect.getLeftTop ()), pointIntToF (clientRect.getLeftBottom ()), selectedColor, color);
				graphics.fillRect (clientRect, gradientBrush);
				graphics.drawRect (clientRect, gradientBorderPen);
			}
			else
			{
				graphics.fillRect (updateRect, selected ? SolidBrush (selectedColor) : SolidBrush (color));
			}
		}
	}
	else
	{
		if(radius > 0.f && (!useUpdateRect || useGradient))
		{
			if(useGradient)
			{
				LinearGradientBrush gradientBrush (pointIntToF (clientRect.getLeftTop ()), pointIntToF (clientRect.getLeftBottom ()), getVisualStyle ().getForeColor (), getVisualStyle ().getBackColor ());
				graphics.fillRoundRect (clientRect, ccl_to_int (radius), ccl_to_int (radius), gradientBrush);
				graphics.drawRoundRect (clientRect, ccl_to_int (radius), ccl_to_int (radius), gradientBorderPen);
			}
			else
			{
				graphics.fillRoundRect (clientRect, ccl_to_int (radius), ccl_to_int (radius), selected ? getVisualStyle ().getForeBrush () : getVisualStyle ().getBackBrush ());
				graphics.drawRoundRect (clientRect, ccl_to_int (radius), ccl_to_int (radius), selected ? getVisualStyle ().getForePen () : getVisualStyle ().getBackPen ());
			}
		}
		else
		{
			if(useGradient)
			{
				LinearGradientBrush gradientBrush (pointIntToF (clientRect.getLeftTop ()), pointIntToF (clientRect.getLeftBottom ()), getVisualStyle ().getForeColor (), getVisualStyle ().getBackColor ());
				graphics.fillRect (clientRect, gradientBrush);
				graphics.drawRect (clientRect, gradientBorderPen);
			}
			else
			{
				graphics.fillRect (updateRect, selected ? getVisualStyle ().getForeBrush () : getVisualStyle ().getBackBrush ());
			}
		}
	}
	
	if(mask)
	{
		Rect src (0, 0, mask->getWidth (), mask->getHeight ());
		graphics.drawImage (mask, src, clientRect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorizedView::canDrawControlBackground () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorizedView::drawControlBackground (IGraphics& graphics, RectRef r, PointRef offset)
{
	TransformSetter t (graphics, Transform ().translate ((float)offset.x, (float)offset.y));
	drawBackground (graphics, r);
}
