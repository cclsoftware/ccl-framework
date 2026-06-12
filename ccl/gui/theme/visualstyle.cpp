//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
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
// StyleConditionContext
//************************************************************************************************

DEFINE_CLASS (StyleConditionContext, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void StyleConditionContext::setCondition (CStringRef condition, bool state)
{
	if(state)
		conditions.addOnce (condition);
	else
		conditions.remove (condition);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StyleConditionContext::matches (StringID conditions) const
{
	bool match = true;
	ForEachCStringToken (conditions, " ", id)
		bool found = false;
		for(CStringRef condition : this->conditions)
		{
			if(condition == id)
			{
				found = true;
				break;
			}
		}

		if(!found)
		{
			match = false;
			break;
		}
	EndFor

	return match;
}

//************************************************************************************************
// VisualStyleData
//************************************************************************************************

DEFINE_CLASS_HIDDEN (VisualStyleData, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyleData::VisualStyleData (StringID name)
: name (name),
  images (0, 2),
  colors (0, 2),
  fonts (0, 1),
  metrics (0, 1),
  strings (0, 1),
  options (0, 1),
  gradients (0, 1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyleData::VisualStyleData (const VisualStyleData& other)
: name (other.name),
  images (0, 2),
  colors (0, 2),
  fonts (0, 1),
  metrics (0, 1),
  strings (0, 1),
  options (0, 1),
  gradients (0, 1)
{
	mergeData (other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyleData::~VisualStyleData ()
{
	removeColorSchemeReferences ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyleData::mergeData (const VisualStyleData& other)
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

void VisualStyleData::removeData ()
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

StringID CCL_API VisualStyleData::getName () const
{
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyleData::setName (StringID _name)
{
	name = _name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyleData::addColorSchemeReference (StringID nameInStyle, ColorScheme& scheme, StringID nameInScheme)
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

void VisualStyleData::removeColorSchemeReferences ()
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

void CCL_API VisualStyleData::notify (ISubject* subject, MessageRef msg)
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

tbool CCL_API VisualStyleData::hasReferences (IColorScheme& scheme) const
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

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorRef CCL_API VisualStyleData::getColor (StringID name, ColorRef defaultColor) const
{
	if(ColorItem* item = lookup (colors, name))
		return item->getColor ();
	else
		return defaultColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyleData::getColor (Color& color, StringID name) const
{
	if(ColorItem* item = lookup (colors, name))
	{
		color = item->getColor ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyleData::setColor (StringID name, ColorRef color)
{
	if(ColorItem* item = lookup (colors, name))
		item->setColor (color);
	else
		colors.add (ColorItem (name, color));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FontRef CCL_API VisualStyleData::getFont (StringID name, FontRef defaultFont) const
{
	if(FontItem* item = lookup (fonts, name))
		return item->getFont ();
	else
		return defaultFont;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyleData::getFont (Font& font, StringID name) const
{
	if(FontItem* item = lookup (fonts, name))
	{
		font = item->getFont ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyleData::setFont (StringID name, FontRef font)
{
	if(FontItem* item = lookup (fonts, name))
		item->setFont (font);
	else
		fonts.add (FontItem (name, font));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyleData::Metric CCL_API VisualStyleData::getMetric (StringID name, Metric defaultValue) const
{
	if(MetricItem* item = lookup (metrics, name))
		return item->getValue ().value;
	else
		return defaultValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyleData::getMetric (Metric& value, StringID name) const
{
	if(MetricItem* item = lookup (metrics, name))
	{
		value = item->getValue ().value;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyleData::setMetric (StringID name, Metric value)
{
	MetricItem* item = lookup (metrics, name);
	if(item)
		item->setValue ({DesignCoord::kCoord, value});
	else
		metrics.add (MetricItem (name, value));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DesignCoord CCL_API VisualStyleData::getDesignMetric (StringID name, DesignCoordRef defaultValue) const
{
	if(MetricItem* item = lookup (metrics, name))
		return item->getValue ();
	else
		return defaultValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyleData::getDesignMetric (DesignCoord& value, StringID name) const
{
	if(MetricItem* item = lookup (metrics, name))
	{
		value = item->getValue ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyleData::setDesignMetric (StringID name, DesignCoordRef value)
{
	MetricItem* item = lookup (metrics, name);
	if(item)
		item->setValue (value);
	else
		metrics.add (MetricItem (name, value));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CString CCL_API VisualStyleData::getString (StringID name, StringID defaultValue) const
{
	if(StringItem* item = lookup (strings, name))
		return item->getValue ();
	else
		return defaultValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyleData::getString (MutableCString& string, StringID name) const
{
	if(StringItem* item = lookup (strings, name))
	{
		string = item->getValue ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyleData::setString (StringID name, StringID value)
{
	if(StringItem* item = lookup (strings, name))
		item->setValue (value);
	else
		strings.add (StringItem (name, value));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle::Options CCL_API VisualStyleData::getOptions (StringID name, Options defaultOptions) const
{
	if(OptionsItem* item = lookup (options, name))
		return item->getOptions ();
	else
		return defaultOptions;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyleData::getOptions (Options& _options, StringID name) const
{
	if(OptionsItem* item = lookup (options, name))
	{
		_options = item->getOptions ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyleData::setOptions (StringID name, Options _options)
{
	if(OptionsItem* item = lookup (options, name))
		item->setOptions (_options);
	else
		options.add (OptionsItem (name, _options));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API VisualStyleData::getImage (StringID name) const
{
	if(ImageItem* item = lookup (images, name))
		return item->getImage ();
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyleData::setImage (StringID name, IImage* image)
{
	ImageItem* item = lookup (images, name);
	if(item)
		item->setImage (image);
	else
		images.add (ImageItem (name, image));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGradient* CCL_API VisualStyleData::getGradient (StringID name) const
{
	if(GradientItem* item = lookup (gradients, name))
		return item->getGradient ();
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyleData::setGradient (StringID name, IGradient* gradient)
{
	if(GradientItem* item = lookup (gradients, name))
		item->setGradient (gradient);
	else
		gradients.add (GradientItem (name, gradient));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyleData::getProperty (Variant& var, MemberID propertyId) const
{
	#define RETURN_PROPERTY(name, vector) \
	if(propertyId == name) { var = const_cast<VisualStyleData*> (this)->vector.asUnknown (); return true; }

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

//************************************************************************************************
// VisualStyle
//************************************************************************************************

DEFINE_CLASS (VisualStyle, VisualStyleData)
DEFINE_CLASS_UID (VisualStyle, 0xc5f60f5b, 0x31b5, 0x47c6, 0x8f, 0x79, 0xdd, 0x18, 0x8a, 0xbc, 0x33, 0xb7)
const VisualStyle VisualStyle::emptyStyle;

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle::VisualStyle (StringID name)
: VisualStyleData (name)
{
	conditions.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle::VisualStyle (const VisualStyle& other)
{
	conditions.objectCleanup (true);

	name = other.name;
	merge (other);

	SOFT_ASSERT (other.trigger == nullptr, "TODO: clone trigger??? - could sometimes be wrong")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyle::merge (const VisualStyle& other)
{
	mergeData (other);
	conditions.add (other.conditions, Container::kClone);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyle::removeAll ()
{
	removeData ();
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

void VisualStyle::addCondition (VisualStyleData* styleData)
{
	ASSERT (styleData)
	conditions.add (styleData);
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

StringID CCL_API VisualStyle::getName () const
{
	return SuperClass::getName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setName (StringID name)
{ 
	SuperClass::setName (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorRef CCL_API VisualStyle::getColor (StringID name, ColorRef defaultColor) const
{
	if(ColorItem* item = lookup (colors, name))
		return item->getColor ();
	if(inherited)
		return inherited->getColor (name, defaultColor);
	return defaultColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::getColor (Color& color, StringID name) const
{
	return getColor (color, name, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::getColor (Color& color, StringID name, IStyleConditionContext* context) const
{
	if(context)
	{
		ArrayForEachReverse (conditions, VisualStyleData, data)
			if(context->matches (data->getName ()))
			{
				if(data->getColor (color, name))
					return true;
			}
		EndFor
	}

	if(SuperClass::getColor (color, name))
		return true;

	if(inherited)
		return inherited->getColor (color, name, context);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setColor (StringID name, ColorRef color)
{
	SuperClass::setColor (name, color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FontRef CCL_API VisualStyle::getFont (StringID name, FontRef defaultFont) const
{
	if(FontItem* item = lookup (fonts, name))
		return item->getFont ();
	if(inherited)
		return inherited->getFont (name, defaultFont);
	return defaultFont;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::getFont (Font& font, StringID name) const
{
	return getFont (font, name, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::getFont (Font& font, StringID name, IStyleConditionContext* context) const
{
	if(context)
	{
		ArrayForEachReverse (conditions, VisualStyleData, data)
			if(context->matches (data->getName ()))
			{
				if(data->getFont (font, name))
					return true;
			}
		EndFor
	}

	if(SuperClass::getFont (font, name))
		return true;

	if(inherited)
		return inherited->getFont (font, name, context);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setFont (StringID name, FontRef font)
{
	SuperClass::setFont (name, font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle::Metric CCL_API VisualStyle::getMetric (StringID name, Metric defaultValue) const
{
	if(MetricItem* item = lookup (metrics, name))
		return item->getValue ().value;
	if(inherited)
		return inherited->getMetric (name, defaultValue);
	return defaultValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::getMetric (Metric& value, StringID name) const
{
	return getMetric (value, name, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::getMetric (Metric& value, StringID name, IStyleConditionContext* context) const
{
	if(context)
	{
		ArrayForEachReverse (conditions, VisualStyleData, data)
			if(context->matches (data->getName ()))
			{
				if(data->getMetric (value, name))
					return true;
			}
		EndFor
	}

	if(SuperClass::getMetric (value, name))
		return true;

	if(inherited)
		return inherited->getMetric (value, name, context);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setMetric (StringID name, Metric value)
{
	SuperClass::setMetric (name, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DesignCoord CCL_API VisualStyle::getDesignMetric (StringID name, DesignCoordRef defaultValue) const
{
	if(MetricItem* item = lookup (metrics, name))
		return item->getValue ();
	if(inherited)
		return inherited->getDesignMetric (name, defaultValue);
	return defaultValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::getDesignMetric (DesignCoord& value, StringID name) const
{
	return getDesignMetric (value, name, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::getDesignMetric (DesignCoord& value, StringID name, IStyleConditionContext* context) const
{
	if(context)
	{
		ArrayForEachReverse (conditions, VisualStyleData, data)
			if(context->matches (data->getName ()))
			{
				if(data->getDesignMetric (value, name))
					return true;
			}
		EndFor
	}

	if(SuperClass::getDesignMetric (value, name))
		return true;
	
	if(inherited)
		return inherited->getDesignMetric (value, name, context);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setDesignMetric (StringID name, DesignCoordRef value)
{
	SuperClass::setDesignMetric (name, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CString CCL_API VisualStyle::getString (StringID name, StringID defaultValue) const
{
	if(StringItem* item = lookup (strings, name))
		return item->getValue ();
	if(inherited)
		return inherited->getString (name, defaultValue);
	return defaultValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::getString (MutableCString& string, StringID name) const
{
	return getString (string, name, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::getString (MutableCString& string, StringID name, IStyleConditionContext* context) const
{
	if(context)
	{
		ArrayForEachReverse (conditions, VisualStyleData, data)
			if(context->matches (data->getName ()))
			{
				if(data->getString (string, name))
					return true;
			}
		EndFor
	}

	if(SuperClass::getString (string, name))
		return true;

	if(inherited)
		return inherited->getString (string, name, context);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setString (StringID name, StringID value)
{
	SuperClass::setString (name, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle::Options CCL_API VisualStyle::getOptions (StringID name, Options defaultOptions) const
{
	if(OptionsItem* item = lookup (options, name))
		return item->getOptions ();
	if(inherited)
		return inherited->getOptions (name, defaultOptions);
	return defaultOptions;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::getOptions (Options& options, StringID name) const
{
	return getOptions (options, name, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::getOptions (Options& options, StringID name, IStyleConditionContext* context) const
{
	if(context)
	{
		ArrayForEachReverse (conditions, VisualStyleData, data)
			if(context->matches (data->getName ()))
			{
				if(data->getOptions (options, name))
					return true;
			}
		EndFor
	}

	if(SuperClass::getOptions (options, name))
		return true;

	if(inherited)
		return inherited->getOptions (options, name, context);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setOptions (StringID name, Options options)
{
	SuperClass::setOptions (name, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API VisualStyle::getImage (StringID name) const
{
	return getImage (name, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API VisualStyle::getImage (StringID name, IStyleConditionContext* context) const
{
	if(context)
	{
		ArrayForEachReverse (conditions, VisualStyleData, data)
			if(context->matches (data->getName ()))
				return data->getImage (name);
		EndFor
	}

	if(ImageItem* item = lookup (images, name))
		return item->getImage ();

	if(inherited)
		return inherited->getImage (name, context);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setImage (StringID name, IImage* image)
{
	SuperClass::setImage (name, image);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGradient* CCL_API VisualStyle::getGradient (StringID name) const
{
	return getGradient (name, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGradient* CCL_API VisualStyle::getGradient (StringID name, IStyleConditionContext* context) const
{
	if(context)
	{
		ArrayForEachReverse (conditions, VisualStyleData, data)
			if(context->matches (data->getName ()))
				return data->getGradient (name);
		EndFor
	}

	if(GradientItem* item = lookup (gradients, name))
		return item->getGradient ();

	if(inherited)
		return inherited->getGradient (name, context);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyle::setGradient (StringID name, IGradient* gradient)
{
	SuperClass::setGradient (name, gradient);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VisualStyle::hasReferences (IColorScheme& scheme) const
{
	if(SuperClass::hasReferences (scheme))
		return true;

	// TOOD: check conditions...

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

const IContainer& CCL_API VisualStyle::getStyleConditions () const
{
	return conditions;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IVisualStyleData* CCL_API VisualStyle::getStyleCondition (StringID name) const
{
	return static_cast<VisualStyleData*> (conditions.findIf ([&] (Object* obj)
		{ return static_cast<VisualStyleData*> (obj)->getName ().compare (name, kStyleCaseSensitive) == Text::kEqual; }));
}
