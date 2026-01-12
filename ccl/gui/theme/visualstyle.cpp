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
// Filename    : ccl/gui/theme/visualstyle.cpp
// Description : VisualStyle class
//
//************************************************************************************************

#include "ccl/gui/theme/visualstyle.h"
#include "ccl/gui/theme/colorscheme.h"

#include "ccl/gui/graphics/colorgradient.h"

#include "ccl/base/trigger.h"

using namespace CCL;

//************************************************************************************************
// Boxed::Font
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Boxed::Font, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::Font::Font (CCL::FontRef font)
: font (font)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Boxed::Font::assign (CCL::FontRef font)
{
	this->font = font;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Boxed::Font::copyTo (CCL::Font& font) const
{
	font = this->font;
}

//************************************************************************************************
// VisualStyle
//************************************************************************************************

DEFINE_CLASS (VisualStyle, Object)
DEFINE_CLASS_UID (VisualStyle, 0xc5f60f5b, 0x31b5, 0x47c6, 0x8f, 0x79, 0xdd, 0x18, 0x8a, 0xbc, 0x33, 0xb7)
const VisualStyle VisualStyle::emptyStyle;

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle::VisualStyle (StringID name)
: name (name),
  images (0, 2),
  colors (0, 2),
  fonts (0, 1),
  metrics (0, 1),
  strings (0, 1),
  options (0, 1),
  gradients (0, 1)
{
	#if DEBUG_LOG
	int size = sizeof(ImageItem);
	CCL_PRINTF ("%i\n", size)
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle::VisualStyle (const VisualStyle& other)
: name (other.name),
  images (0, 2),
  colors (0, 2),
  fonts (0, 1),
  metrics (0, 1),
  strings (0, 1),
  options (0, 1),
  gradients (0, 1)
{
	merge (other);

	SOFT_ASSERT (other.trigger == nullptr, "TODO: clone trigger??? - could sometimes be wrong")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle::~VisualStyle ()
{
	removeColorSchemeReferences ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API VisualStyle::getName () const
{
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyle::setName (StringID _name)
{
	name = _name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyle::merge (const VisualStyle& other)
{
	merge (colors, other.colors);
	merge (fonts, other.fonts);
	merge (metrics, other.metrics);
	merge (strings, other.strings);
	merge (options, other.options);
	merge (images, other.images);
	merge (gradients, other.gradients);

	VectorForEach (other.colorSchemeReferences, ColorStyleReference*, reference)
		addColorSchemeReference (reference->nameInStyle, *reference->scheme, reference->nameInScheme);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyle::removeAll ()
{
	colors.removeAll ();
	fonts.removeAll ();
	metrics.removeAll ();
	options.removeAll ();
	images.removeAll ();
	gradients.removeAll ();

	removeColorSchemeReferences ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyle::use (IVisualStyleClient* client)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyle::unuse (IVisualStyleClient* client)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyle::setInherited (VisualStyle* _inherited)
{
	ASSERT (_inherited != this)
	if(_inherited != this)
		inherited = _inherited;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyle::setTrigger (ITriggerPrototype* _trigger)
{
	trigger = _trigger;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITriggerPrototype* VisualStyle::getTrigger (bool deep) const
{
	if(trigger)
		return trigger;
	if(deep && inherited)
		return inherited->getTrigger (true);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyle::addColorSchemeReference (StringID nameInStyle, ColorScheme& scheme, StringID nameInScheme)
{
	if(!colorSchemeObserverList.contains (&scheme))
	{
		scheme.retain ();
		scheme.addObserver (this);
		colorSchemeObserverList.add (&scheme);
	}

	ColorStyleReference* reference = NEW ColorStyleReference;
	reference->scheme = &scheme;
	reference->nameInScheme = nameInScheme;
	reference->nameInStyle = nameInStyle;
	colorSchemeReferences.add (reference);

	setColor (nameInStyle, scheme.getColor (nameInScheme));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyle::removeColorSchemeReferences ()
{
	VectorForEach (colorSchemeObserverList, ColorScheme*, scheme)
		scheme->removeObserver (this);
		scheme->release ();
	EndFor
	colorSchemeObserverList.removeAll ();

	VectorForEach (colorSchemeReferences, ColorStyleReference*, reference)
		delete reference;
	EndFor
	colorSchemeReferences.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
		if(ColorScheme* scheme = unknown_cast<ColorScheme> (subject))
		{
			VectorForEach (colorSchemeReferences, ColorStyleReference*, reference)
				if(reference->scheme == scheme)
					setColor (reference->nameInStyle, scheme->getColor (reference->nameInScheme));
			EndFor
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::hasReferences (IColorScheme& scheme) const
{
	// check colors
	if(colorSchemeObserverList.contains (unknown_cast<ColorScheme> (&scheme)))
		return true;

	// check images
	ImageItem* imageArray = images.getItems ();
	for(int i = 0; i < images.count (); i++)
		if(ColorScheme::hasReferences (imageArray[i].getImage (), scheme))
			return true;

	// check gradients
	GradientItem* gradientArray = gradients.getItems ();
	for(int i = 0; i < gradients.count (); i++)
		if(auto gradient = unknown_cast<ColorGradient> (gradientArray[i].getGradient ()))
			if(gradient->hasReferences (&scheme))
				return true;

	// check parent
	if(inherited)
		return inherited->hasReferences (scheme);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::copyFrom (const IVisualStyle& _other)
{
	if(VisualStyle* other = unknown_cast<VisualStyle> (&_other))
	{
		if(other != this)
		{
			removeAll ();
			merge (*other);
			inherited = other->inherited;
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

const IVisualStyle* CCL_API VisualStyle::getInherited () const
{
	return inherited;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IVisualStyle* CCL_API VisualStyle::getOriginal () const
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorRef CCL_API VisualStyle::getColor (StringID name, ColorRef defaultColor) const
{
	ColorItem* item = lookup (colors, name);
	if(item)
		return item->getColor ();
	if(inherited)
		return inherited->getColor (name, defaultColor);
	return defaultColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setColor (StringID name, ColorRef color)
{
	ColorItem* item = lookup (colors, name);
	if(item)
		item->setColor (color);
	else
		colors.add (ColorItem (name, color));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FontRef CCL_API VisualStyle::getFont (StringID name, FontRef defaultFont) const
{
	FontItem* item = lookup (fonts, name);
	if(item)
		return item->getFont ();
	if(inherited)
		return inherited->getFont (name, defaultFont);
	return defaultFont;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setFont (StringID name, FontRef font)
{
	FontItem* item = lookup (fonts, name);
	if(item)
		item->setFont (Font (font));
	else
		fonts.add (FontItem (name, Font (font)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle::Metric CCL_API VisualStyle::getMetric (StringID name, Metric defaultValue) const
{
	MetricItem* item = lookup (metrics, name);
	if(item)
		return item->getValue ();
	if(inherited)
		return inherited->getMetric (name, defaultValue);
	return defaultValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setMetric (StringID name, Metric value)
{
	MetricItem* item = lookup (metrics, name);
	if(item)
		item->setValue (value);
	else
		metrics.add (MetricItem (name, value));
}

//////////////////////////////////////////////////////////////////////////////////////////////////


CString CCL_API VisualStyle::getString (StringID name, StringID defaultValue) const
{
	StringItem* item = lookup (strings, name);
	if(item)
		return item->getValue ();
	if(inherited)
		return inherited->getString (name, defaultValue);
	return defaultValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setString (StringID name, StringID value)
{
	StringItem* item = lookup (strings, name);
	if(item)
		item->setValue (value);
	else
		strings.add (StringItem (name, value));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle::Options CCL_API VisualStyle::getOptions (StringID name, Options defaultOptions) const
{
	OptionsItem* item = lookup (options, name);
	if(item)
		return item->getOptions ();
	if(inherited)
		return inherited->getOptions (name, defaultOptions);
	return defaultOptions;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setOptions (StringID name, Options _options)
{
	OptionsItem* item = lookup (options, name);
	if(item)
		item->setOptions (_options);
	else
		options.add (OptionsItem (name, _options));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API VisualStyle::getImage (StringID name) const
{
	ImageItem* item = lookup (images, name);
	if(item)
		return item->getImage ();
	if(inherited)
		return inherited->getImage (name);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setImage (StringID name, IImage* image)
{
	ImageItem* item = lookup (images, name);
	if(item)
		item->setImage (image);
	else
		images.add (ImageItem (name, image));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGradient* CCL_API VisualStyle::getGradient (StringID name) const
{
	GradientItem* item = lookup (gradients, name);
	if(item)
		return item->getGradient ();
	if(inherited)
		return inherited->getGradient (name);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setGradient (StringID name, IGradient* gradient)
{
	GradientItem* item = lookup (gradients, name);
	if(item)
		item->setGradient (gradient);
	else
		gradients.add (GradientItem (name, gradient));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::getProperty (Variant& var, MemberID propertyId) const
{
	#define RETURN_PROPERTY(name, vector) \
	if(propertyId == name) { var = const_cast<VisualStyle*> (this)->vector.asUnknown (); return true; }

	RETURN_PROPERTY (kColors, colors)
	RETURN_PROPERTY (kFonts, fonts)
	RETURN_PROPERTY (kMetrics, metrics)
	RETURN_PROPERTY (kStrings, strings)
	RETURN_PROPERTY (kOptions, options)
	RETURN_PROPERTY (kImages, images)
	RETURN_PROPERTY (kGradients, gradients)

	#undef RETURN_PROPERTY

	return SuperClass::getProperty (var, propertyId);
}
