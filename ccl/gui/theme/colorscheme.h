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
// Filename    : ccl/gui/theme/colorscheme.h
// Description : Color Scheme
//
//************************************************************************************************

#ifndef _ccl_colorscheme_h
#define _ccl_colorscheme_h

#include "ccl/public/gui/framework/icolorscheme.h"
#include "ccl/public/text/cstring.h"

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/base/storage/storableobject.h"

namespace CCL {

interface IImage;
	
//************************************************************************************************
// ColorScheme
//************************************************************************************************

class ColorScheme: public Object,
				   public IColorScheme
{
public:
	DECLARE_CLASS (ColorScheme, Object)

	ColorScheme (StringID name = nullptr);

	PROPERTY_VARIABLE (float, hueLevel, HueLevel)
	PROPERTY_VARIABLE (float, saturationLevel, SaturationLevel)
	PROPERTY_VARIABLE (float, luminanceLevel, LuminanceLevel)
	PROPERTY_VARIABLE (float, contrastLevel, ContrastLevel)
	PROPERTY_BOOL (colorInverted, ColorInverted)
	PROPERTY_BOOL (mainSchemeDependent, MainSchemeDependent)
	
	void restore ();
	void store () const;

	class Item;

	Item& getItemMutable (StringID name);

	// IColorScheme
	StringID CCL_API getName () const override { return name; }
	float CCL_API getLevel (StringID id) const override;
	void CCL_API setLevel (StringID id, float value, int updateMode = kDetect) override;
	Color CCL_API getColor (StringID name, ColorRef defaultColor = Colors::kBlack) const override;
	void CCL_API setDefaultLevel (StringID id, float value) override;
	void CCL_API resetToDefaults () override;

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (IColorScheme, Object)

	// Helpers:
	static bool hasReferences (IImage* image, IColorScheme& scheme);

protected:
	MutableCString name;
	ObjectArray items;
	
	float defaultHueLevel;
	float defaultSaturationLevel;
	float defaultLuminanceLevel;
	float defaultContrastLevel;
	bool defaultColorInverted;
	bool defaultMainSchemeDependent;
	
	const Item* findItem (StringID name) const;
	void updateAll ();
};
	
//************************************************************************************************
// ColorScheme::Item
//************************************************************************************************

class ColorScheme::Item: public Object
{
public:
	Item (StringID name = nullptr);
	
	PROPERTY_MUTABLE_CSTRING (name, Name)
	PROPERTY_VARIABLE (Color, currentColor, CurrentColor)
	PROPERTY_VARIABLE (Color, baseColor, BaseColor)
	PROPERTY_VARIABLE (Color, invertedColor, InvertedColor)
	PROPERTY_BOOL (hueFixed, HueFixed)
	PROPERTY_BOOL (invertible, Invertible)
	PROPERTY_VARIABLE (int, invertedValue, InvertedValue)
	PROPERTY_BOOL (slCombined, SLCombined)

	void setSaturationSegments (StringRef description);
	void setLuminanceSegments (StringRef description);
	void setContrastSegments (StringRef description);
	void updateCurrentColor (float hueLevel, float luminanceLevel, float saturationLevel, float contrastLevel, bool useColorInverted);
	
private:

	struct SegmentDescription
	{
		int offsetPosition = -1;
		bool offsetForColorInversionOnly = false;
		bool offsetNotForColorInversion = false;
		double offset = 0.;
		Vector<double> segments;
				
		void parseSegments (StringID string);
		bool useOffsetIndex (bool useColorInverted) const;
	};
	
	SegmentDescription lowerSaturationSegments;
	SegmentDescription lowerLuminanceSegments;
	SegmentDescription lowerContrastSegments;
	SegmentDescription upperSaturationSegments;
	SegmentDescription upperLuminanceSegments;
	SegmentDescription upperContrastSegments;

	void setSegments (SegmentDescription& lowerPart, SegmentDescription& upperPart, StringRef description);
	float hslShift (float level, const SegmentDescription& lowerPart, const SegmentDescription& upperPart, bool useColorInverted);

	void adjustHue (float& hue, float hueLevel);
	void adjustSaturation (float& sat, float saturationLevel, float contrastLevel);
	void adjustLuminance (float& lum, float luminanceLevel, float contrastLevel, bool useColorInverted);
	
	bool luminanceShiftHadOffsetJump (float level, bool useColorInverted) const;
	bool isAutoInvertColor () const { return (getInvertedValue () == -1) ? true : false; }
	bool isDarkInvertedContrastColor () const { return (getInvertedValue () >= 0 && getInvertedValue () < 50) ? true : false; }	
};

//************************************************************************************************
// ColorSchemes
//************************************************************************************************

class ColorSchemes: public Object,
					public IColorSchemes,
					public Singleton<ColorSchemes>
{
public:
	DECLARE_CLASS (ColorSchemes, Object)

	ColorSchemes ();

	static const FileType& getFileType ();

	ColorScheme* find (StringID name) const;
	ColorScheme& get (StringID name);

	void schemeChanged (ColorScheme& scheme);
	
	enum AppearanceMode
	{
		kFollowOSAppearance,
		kLightAppearance,
		kDarkAppearance
	};
	
	AppearanceMode getMainAppearanceMode () { return appearanceMode; }
	void setMainAppearanceMode (AppearanceMode mode);
	void setMainAppearanceModeFromString (StringRef modeString);
	
	// IColorSchemes
	IColorScheme* CCL_API getScheme (StringID name, tbool create = false) override;
	const FileType& CCL_API getSchemeFileType () const override;
	IStorable* CCL_API createSchemeFile (IColorScheme* schemes[], int count,
										 IColorSchemeImporter* importer = nullptr, 
										 int revision = 0) override;

	CLASS_INTERFACE (IColorSchemes, Object)

protected:
	ObjectArray schemes;
	AppearanceMode appearanceMode;
};

//************************************************************************************************
// ColorSchemeFile
//************************************************************************************************

class ColorSchemeFile: public StorableObject,
					   public IContainer
{
public:
	DECLARE_CLASS (ColorSchemeFile, StorableObject)

	ColorSchemeFile ();

	PROPERTY_VARIABLE (int, revision, Revision)
	PROPERTY_SHARED_AUTO (IColorSchemeImporter, importer, Importer)

	void addShared (ColorScheme* scheme);

	// StorableObject
	tbool CCL_API getFormat (FileType& format) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACES (StorableObject)

protected:
	ObjectArray schemes;

	// IContainer
	IUnknownIterator* CCL_API createIterator () const override;
};

} // namespace CCL

#endif // _ccl_colorscheme_h
