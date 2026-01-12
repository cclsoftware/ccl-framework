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
// Filename    : ccl/gui/theme/colorscheme.cpp
// Description : Color Scheme
//
//************************************************************************************************

#include "ccl/gui/theme/colorscheme.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/graphics/shapes/shapeimage.h"
#include "ccl/gui/graphics/imaging/coloredbitmap.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/public/gui/framework/controlsignals.h"

#include "ccl/public/storage/filetype.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/cclversion.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/configuration.h"
#include "ccl/base/storage/storage.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (ColorScheme, "Color Scheme")
END_XSTRINGS

//************************************************************************************************
// ColorScheme
//************************************************************************************************

bool ColorScheme::hasReferences (IImage* _image, IColorScheme& scheme)
{
	Image* image = unknown_cast<Image> (_image);
	if(image)
	{
		Rect unused;
		image = image->getOriginalImage (unused);
	}

	Rect originalRect;
	if(ShapeImage* shapeImage = ccl_cast<ShapeImage> (image))
		return shapeImage->hasReferences (scheme);
	else if(ColoredSchemeBitmap* coloredSchemeBitmap = ccl_cast<ColoredSchemeBitmap> (image))
		return coloredSchemeBitmap->hasReferences (scheme);
	else if(ColoredSchemeBitmap* originalSchemeBitmap = ccl_cast<ColoredSchemeBitmap> (image->getOriginalImage (originalRect)))
		return originalSchemeBitmap->hasReferences (scheme);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ColorScheme, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorScheme::ColorScheme (StringID name)
: name (name),
  hueLevel (0.f),
  saturationLevel (0.5f),
  luminanceLevel (0.5f),
  contrastLevel (0.5f),
  colorInverted (false),
  mainSchemeDependent (false),
  defaultHueLevel (0),
  defaultSaturationLevel (0.5f),
  defaultLuminanceLevel (0.5f),
  defaultContrastLevel (0.5f),
  defaultColorInverted (false),
  defaultMainSchemeDependent (false)
{
	items.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorScheme::restore ()
{
	Configuration::Registry& registry = Configuration::Registry::instance ();
	MutableCString section (kPersistentPrefix);
	section += name;

	Variant value;
	if(registry.getValue (value, section, kHueLevel))
		setHueLevel (value);
	if(registry.getValue (value, section, kSaturationLevel))
		setSaturationLevel (value);
	if(registry.getValue (value, section, kLuminanceLevel))
		setLuminanceLevel (value);
	if(registry.getValue (value, section, kContrastLevel))
		setContrastLevel (value);
	if(registry.getValue (value, section, kColorInversion))
		setColorInverted (value);
	if(registry.getValue (value, section, kMainSchemeDependent))
		setMainSchemeDependent (value);
	updateAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorScheme::store () const
{
	Configuration::Registry& registry = Configuration::Registry::instance ();
	MutableCString section (kPersistentPrefix);
	section += name;

	registry.setValue (section, kHueLevel, getHueLevel ());
	registry.setValue (section, kSaturationLevel, getSaturationLevel ());
	registry.setValue (section, kLuminanceLevel, getLuminanceLevel ());
	registry.setValue (section, kContrastLevel, getContrastLevel ());
	registry.setValue (section, kColorInversion, isColorInverted ());
	registry.setValue (section, kMainSchemeDependent, isMainSchemeDependent ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorScheme::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	setHueLevel ((float)a.getFloat (kHueLevel));
	setSaturationLevel ((float)a.getFloat (kSaturationLevel));
	setLuminanceLevel ((float)a.getFloat (kLuminanceLevel));
	setContrastLevel ((float)a.getFloat (kContrastLevel));
	setColorInverted (a.getBool (kColorInversion));
	setMainSchemeDependent (a.getBool (kMainSchemeDependent));
	
	store ();
	updateAll ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorScheme::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set (kHueLevel, getHueLevel ());
	a.set (kSaturationLevel, getSaturationLevel ());
	a.set (kLuminanceLevel, getLuminanceLevel ());
	a.set (kContrastLevel, getContrastLevel ());
	a.set (kColorInversion, isColorInverted ());
	if(isMainSchemeDependent ()) // optional attribute
		a.set (kMainSchemeDependent, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API ColorScheme::getLevel (StringID id) const
{
	if(id == kSaturationLevel)
		return getSaturationLevel ();
	if(id == kLuminanceLevel)
		return getLuminanceLevel ();
	if(id == kHueLevel)
		return getHueLevel ();
	if(id == kContrastLevel)
		return getContrastLevel ();
	if(id == kColorInversion)
		return isColorInverted () ? 1.f : 0.f;
	if(id == kMainSchemeDependent)
		return isMainSchemeDependent () ? 1.f : 0.f;
	return 0.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorScheme::setLevel (StringID id, float value, int updateMode)
{
	bool changed = false;

	if(id == kHueLevel)
	{
		changed = value != getHueLevel ();
		setHueLevel (value);
	}
	else if(id == kSaturationLevel)
	{
		changed = value != getSaturationLevel ();
		setSaturationLevel (value);
	}
	else if(id == kLuminanceLevel)
	{
		changed = value != getLuminanceLevel ();
		setLuminanceLevel (value);
	}
	else if(id == kContrastLevel)
	{
		changed = value != getContrastLevel ();
		setContrastLevel (value);
	}
	else if(id == kColorInversion)
	{
		changed = value != (isColorInverted () ? 1.f : 0.f);
		setColorInverted (value != 0.f);
	}
	else if(id == kMainSchemeDependent)
	{
		changed = value != (isMainSchemeDependent () ? 1.f : 0.f);
		setMainSchemeDependent (value != 0.f);
	}

	if((changed && updateMode == kDetect) || updateMode == kForce)
	{
		store ();
		updateAll ();
		ColorSchemes::instance ().schemeChanged (*this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ColorScheme::Item* ColorScheme::findItem (StringID name) const
{
	ArrayForEachFast (items, Item, item)
		if(item->getName ().compare (name, false) == 0)
			return item;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorScheme::Item& ColorScheme::getItemMutable (StringID name)
{
	Item* item = const_cast<Item*> (findItem (name));
	if(item == nullptr)
		items.add (item = NEW Item (name));
	return *item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Color CCL_API ColorScheme::getColor (StringID name, ColorRef defaultColor) const
{
	if(const Item* item = findItem (name))
		return item->getCurrentColor ();
	else
		return defaultColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorScheme::setDefaultLevel (StringID id, float value)
{
	if(id == kHueLevel)
		defaultHueLevel = value;
	else if(id == kSaturationLevel)
		defaultSaturationLevel = value;
	else if(id == kLuminanceLevel)
		defaultLuminanceLevel = value;
	else if(id == kContrastLevel)
		defaultContrastLevel = value;
	else if(id == kColorInversion)
		defaultColorInverted = (value != 0.f) ? true : false;
	else if(id == kMainSchemeDependent)
		defaultMainSchemeDependent  = (value != 0.f) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorScheme::resetToDefaults ()
{
	setLevel (kHueLevel, defaultHueLevel, kIgnore);
	setLevel (kSaturationLevel, defaultSaturationLevel, kIgnore);
	setLevel (kLuminanceLevel, defaultLuminanceLevel, kIgnore);
	setLevel (kContrastLevel, defaultContrastLevel, kIgnore);
	setLevel (kColorInversion, defaultColorInverted ? 1.f : 0.f, kIgnore);
	setLevel (kMainSchemeDependent, defaultMainSchemeDependent ? 1.f : 0.f, kForce);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorScheme::updateAll ()
{
	ArrayForEachFast (items, Item, item)
		item->updateCurrentColor (getHueLevel (), getLuminanceLevel (), getSaturationLevel (), getContrastLevel (), isColorInverted ());
	EndFor
	
	signal (Message (kChanged));
}

//************************************************************************************************
// ColorScheme::Item
//************************************************************************************************

ColorScheme::Item::Item (StringID name)
: name (name),
  hueFixed (false),
  invertible (true),
  invertedValue (-1),
  slCombined (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorScheme::Item::updateCurrentColor (float hueLevel, float luminanceLevel, float saturationLevel, float contrastLevel, bool useColorInverted)
{
	Color c (useColorInverted ? getInvertedColor () : getBaseColor ());
	
	ColorHSL hsl (c);

	adjustHue (hsl.h, hueLevel);
//	if(isSLCombined () && luminanceLevel > 0.5f)
//		adjustSaturation (hsl.s, luminanceLevel, 0.f);
	if(useColorInverted && isSLCombined ())
		hsl.s = ccl_bound (hsl.s * 1.5f);
	
	adjustSaturation (hsl.s, saturationLevel, contrastLevel);
	adjustLuminance (hsl.l, luminanceLevel, contrastLevel, useColorInverted && isInvertible ());
	
	hsl.toColor (c);
	
	setCurrentColor (c);
}
//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorScheme::Item::adjustHue (float& hue, float hueLevel)
{
	if(isHueFixed () == false)
		hue += hueLevel * 360;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorScheme::Item::adjustSaturation (float& sat, float saturationLevel, float contrastLevel)
{
	sat += hslShift ((saturationLevel - 0.5f) * 2.f, lowerSaturationSegments, upperSaturationSegments, false);
	
	// add saturation for high contrast in relation to the saturation level
	float saturationContrastLevel = contrastLevel;
	if(saturationContrastLevel > 0.75f)
	{
		saturationContrastLevel -= 0.75f;
		saturationContrastLevel *= 4;
	}
	else
	{
		saturationContrastLevel = 0;
	}
	
	sat += hslShift (saturationContrastLevel * saturationLevel, lowerContrastSegments, upperContrastSegments, false);
	sat = ccl_bound (sat);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorScheme::Item::adjustLuminance (float& lum, float luminanceLevel, float contrastLevel, bool useColorInverted)
{
	if(useColorInverted)
	{
		if(isAutoInvertColor ())
		{
			luminanceLevel = 1 - luminanceLevel;
			contrastLevel = 1 - contrastLevel;
		}
		else
		{
			float invertLum = getInvertedValue () * 0.01f;
			
			if(contrastLevel < 0.5f)
				if(invertLum > 0.5f)
					contrastLevel = 0.25f + (contrastLevel * 0.5f);
			
			bool invertContrastLevel = true;	// balance contrastAdjustmentSign
			if(lum < 0.5f)
				if(invertLum > 0.5f)
					invertContrastLevel = false;
			
			if(lum > 0.5f)
				if(invertLum < 0.5f)
					invertContrastLevel = false;
			
			if(invertContrastLevel)
				contrastLevel = 1 - contrastLevel;
			
			lum = invertLum;
		}
	}

	lum += hslShift ((luminanceLevel - 0.5f) * 2.f, lowerLuminanceSegments, upperLuminanceSegments, useColorInverted);
	int contrastAdjustmentSign = luminanceShiftHadOffsetJump ((luminanceLevel - 0.5f) * 2.f, useColorInverted) ? -1 : 1;
	lum += hslShift ((contrastLevel - 0.5f) * 2.f, lowerContrastSegments, upperContrastSegments, useColorInverted) * contrastAdjustmentSign;
	lum = ccl_bound (lum);
	
	if(useColorInverted && isAutoInvertColor ())
		lum = 1 - lum;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorScheme::Item::luminanceShiftHadOffsetJump (float level, bool useColorInverted) const
{
	bool result = useColorInverted;

	if(level == 0.0)
		return result;
	
	const SegmentDescription& desc = (level < 0.0) ? lowerLuminanceSegments : upperLuminanceSegments;
	
	if(level < 0.0)
		level *= -1;
		
	double step = 1.f / desc.segments.count ();
	double accu = 0;
	double i = 0;
	int j = 0;

	for (i = step; i < level; i += step)
	{
		accu += step * desc.segments.at (j);
		j++;
		
		if(j == desc.offsetPosition)
		{
			if(desc.useOffsetIndex (useColorInverted))
				if(accu * desc.offset < 0)
					return !result;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
float ColorScheme::Item::hslShift (float level, const SegmentDescription& lowerPart, const SegmentDescription& upperPart, bool useColorInverted)
{
	if(level == 0.0)
		return 0.f;
	
	const SegmentDescription& desc = (level < 0.0) ? lowerPart : upperPart;
	
	int sign = 1;
	
	if(level < 0.0)
	{
		sign = -1;
		level *= sign;
	}

	double step = 1.f / desc.segments.count ();
	double accu = 0;
	double i = 0;
	int j = 0;

	for (i = step; i < level; i += step)
	{
		accu += step * desc.segments.at (j);
		j++;
		
		if(j == desc.offsetPosition)
		{
			if(desc.useOffsetIndex (useColorInverted))
				accu += desc.offset;
		}
	}

	double remainder = level - (i - step);
	if(remainder > 0.0)
		accu += remainder * desc.segments.at (j);

	float divider = 100.f * sign;
	
	if(useColorInverted && isDarkInvertedContrastColor ())
		if(level > 0.0)
			divider *= 2.f; // don't lighten dark inverted colors too much
	
	return (float)accu / divider;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorScheme::Item::setSegments (SegmentDescription& lowerPart, SegmentDescription& upperPart, StringRef string)
{
	lowerPart.offsetPosition = -1;
	lowerPart.offset = 0;
	lowerPart.segments.removeAll ();
	upperPart.offsetPosition = -1;
	upperPart.offset = 0;
	upperPart.segments.removeAll ();
	
	int splitIndex = string.index (":");
	int legacyIndex = string.index ("-");
	
	if(splitIndex > 0)
	{
		MutableCString subString = string.subString (0, splitIndex);
		lowerPart.parseSegments (subString);
		
		subString = string.subString (splitIndex+1);
		upperPart.parseSegments (subString);
	}
	else if(legacyIndex > 0)
	{
		double entry;
		MutableCString subString = string.subString (0, legacyIndex);
		subString.getFloatValue (entry);
		lowerPart.segments.add (entry);
		subString = string.subString (legacyIndex+1);
		subString.getFloatValue (entry);
		upperPart.segments.add (entry);
	}
	else
	{
		double entry;
		string.getFloatValue (entry);
		lowerPart.segments.add (entry);
		upperPart.segments.add (entry);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorScheme::Item::SegmentDescription::parseSegments (StringID string)
{	
	int segmentNum = 0;
	
	ForEachCStringToken (string, "/", token)
		segmentNum++;
		
		MutableCString stringToken = token;
		if(offsetPosition == -1)
		{
			int offsetIndex = stringToken.index ("|");
			
			int inversionOffsetIndex = stringToken.index ("I");
			offsetForColorInversionOnly = (inversionOffsetIndex > 0) ? true : false;
			if(offsetForColorInversionOnly)
				offsetIndex = inversionOffsetIndex;
			
			int exclusiveOffsetIndex = stringToken.index ("X");
			offsetNotForColorInversion = (exclusiveOffsetIndex > 0) ? true : false;
			if(offsetNotForColorInversion)
				offsetIndex = exclusiveOffsetIndex;

			if(offsetIndex > 0)
			{
				MutableCString offsetString = stringToken.subString (offsetIndex+1);
				offsetString.getFloatValue (offset);
				offsetPosition = segmentNum;
				stringToken = stringToken.subString (0, offsetIndex);
			}
			
		}
		
		double entry = 0.f;
		stringToken.getFloatValue (entry);
		segments.add (entry);
	EndFor
};


//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorScheme::Item::SegmentDescription::useOffsetIndex (bool useColorInversion) const
{
	bool useOffset = true;
	
	if(useColorInversion && offsetNotForColorInversion)
		useOffset = false;
	
	if(offsetForColorInversionOnly)
		useOffset = useColorInversion;
	
	return useOffset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorScheme::Item::setSaturationSegments (StringRef description)
{
	setSegments (lowerSaturationSegments, upperSaturationSegments, description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorScheme::Item::setLuminanceSegments (StringRef description)
{
	setSegments (lowerLuminanceSegments, upperLuminanceSegments, description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorScheme::Item::setContrastSegments (StringRef description)
{
	setSegments (lowerContrastSegments, upperContrastSegments, description);
}


//************************************************************************************************
// ColorSchemes
//************************************************************************************************

const FileType& ColorSchemes::getFileType ()
{
	static FileType fileType (nullptr, "colorscheme", CCL_MIME_TYPE "-colorscheme+xml");
	return FileTypes::init (fileType, XSTR (ColorScheme));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SINGLETON_CLASS (ColorSchemes, Object)
DEFINE_CLASS_UID (ColorSchemes, 0xe7e2611a, 0xdd45, 0x4fe1, 0xaf, 0x76, 0x6f, 0x78, 0x0, 0x28, 0x80, 0x1e)
DEFINE_SINGLETON (ColorSchemes)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorSchemes::ColorSchemes ()
: appearanceMode (kDarkAppearance)
{
	schemes.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorScheme* ColorSchemes::find (StringID name) const
{
	ArrayForEachFast (schemes, ColorScheme, scheme)
		if(scheme->getName ().compare (name, false) == 0)
			return scheme;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorScheme& ColorSchemes::get (StringID name)
{
	ColorScheme* scheme = find (name);
	if(scheme == nullptr)
		schemes.add (scheme = NEW ColorScheme (name));
	return *scheme;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IColorScheme* CCL_API ColorSchemes::getScheme (StringID name, tbool create)
{
	ColorScheme* scheme = find (name);
	if(scheme == nullptr && create)
	{
		scheme = &get (name);
		scheme->restore ();
	}
	return scheme;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API ColorSchemes::getSchemeFileType () const
{
	return getFileType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStorable* CCL_API ColorSchemes::createSchemeFile (IColorScheme* schemes[], int count, IColorSchemeImporter* importer, int revision)
{
	ColorSchemeFile* file = NEW ColorSchemeFile;
	file->setImporter (importer);
	file->setRevision (revision);
	for(int i = 0; i < count; i++)
		if(ColorScheme* scheme = unknown_cast<ColorScheme> (schemes[i]))
			file->addShared (scheme);
	return file;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorSchemes::schemeChanged (ColorScheme& scheme)
{
	// notify application
	SignalSource (Signals::kGUI).signal (Message (Signals::kColorSchemeChanged, scheme.asUnknown ()));
	
	// notify views
	for(int i = 0, count = Desktop.countWindows (); i < count; i++)
		if(Window* window = unknown_cast<Window> (Desktop.getWindow (i)))
		{
			window->onColorSchemeChanged (ColorSchemeEvent (scheme));
			window->invalidate ();

			// additionally, make sure all layers are updated
			if(NativeGraphicsLayer* rootLayer = unknown_cast<NativeGraphicsLayer> (window->getGraphicsLayer ()))
				rootLayer->setUpdateNeededRecursive ();
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorSchemes::setMainAppearanceMode (AppearanceMode mode)
{
	appearanceMode = mode;
	
	if(IColorScheme* mainScheme = getScheme (ThemeNames::kMain, true))
		mainScheme->setLevel (IColorScheme::kColorInversion, (appearanceMode == kLightAppearance) ? true : false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorSchemes::setMainAppearanceModeFromString (StringRef _modeString)
{
	MutableCString modeString (_modeString);
	setMainAppearanceMode (modeString == "followOS" ? kFollowOSAppearance :
						   modeString == "light" ? kLightAppearance : 
						   kDarkAppearance);
}

//************************************************************************************************
// ColorSchemeFile
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ColorSchemeFile, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorSchemeFile::ColorSchemeFile ()
: revision (0)
{
	schemes.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ColorSchemeFile::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IContainer)

	// make importer accessible
	if(iid == ccl_iid<IColorSchemeImporter> () && importer)
		return importer->queryInterface (iid, ptr);

	return SuperClass::queryInterface (iid, ptr);
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorSchemeFile::addShared (ColorScheme* scheme)
{
	ASSERT (scheme != nullptr)
	schemes.add (return_shared (scheme));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API ColorSchemeFile::createIterator () const
{
	return static_cast<const IContainer&> (schemes).createIterator (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorSchemeFile::getFormat (FileType& format) const
{
	format = ColorSchemes::getFileType ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorSchemeFile::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	ForEach (schemes, ColorScheme, scheme)
		if(Attributes* a2 = a.getAttributes (scheme->getName ()))
		{
			// adjust older revision via importer
			int savedRevision = a.getInt ("revision");
			if(importer && savedRevision != revision)
				importer->adjustScheme (scheme->getName (), *a2, savedRevision);

			scheme->load (Storage (*a2, storage));
			ColorSchemes::instance ().schemeChanged (*scheme);
		}
	EndFor
	return true;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorSchemeFile::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	ForEach (schemes, ColorScheme, scheme)
		Attributes* a2 = NEW Attributes;
		scheme->save (Storage (*a2, storage));
		if(revision > 0)
			a.set ("revision", revision);
		a.set (scheme->getName (), a2, Attributes::kOwns);
	EndFor
	return true;
}
