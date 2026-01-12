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
// Filename    : ccl/gui/skin/skinattributes.cpp
// Description : Skin Attributes
//
//************************************************************************************************

#include "ccl/gui/skin/skinattributes.h"
#include "ccl/gui/skin/skinwizard.h"

#include "ccl/gui/views/view.h"

using namespace CCL;

//************************************************************************************************
// SkinAttributes
//************************************************************************************************

const String SkinAttributes::strTrue = CCLSTR ("true");
const String SkinAttributes::strFalse = CCLSTR ("false");

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::scanRect (Rect& r, StringRef string)
{
	if(!string.isEmpty ())
	{
		int left = 0, top = 0, right = 0, bottom = 0;
		if(::sscanf (MutableCString (string), "%d ,%d ,%d ,%d", &left, &top, &right, &bottom) > 0)
		{
			r (left, top, right, bottom);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::scanSize (Rect& r, StringRef string)
{
	if(!string.isEmpty ())
	{
		int left = 0, top = 0, width = 0, height = 0;
		if(::sscanf (MutableCString (string), "%d ,%d ,%d ,%d", &left, &top, &width, &height) > 0)
		{
			r (left, top, left + width, top + height);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::scanDesignRect (DesignSize& ds, StringRef string)
{
	if(!scanDesignSize (ds, string))
		return false;
	
	ds.width -= ds.left;
	ds.height -= ds.top;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::scanDesignSize (DesignSize& ds, StringRef string)
{
	if(string.isEmpty ())
		return false;

	DesignCoord* coordinates[4] = {&ds.left, &ds.top, &ds.width, &ds.height};
	
	int i = 0;
	ForEachStringToken (string, ",", token)
		if(i > 3) // More than 4 coordinates are not supported
			return false;
	
		token.trimWhitespace ();
		scanDesignCoord (*coordinates[i++], token);
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinAttributes::scanDesignCoord (DesignCoord& dc, StringRef string)
{
	if(string == DesignCoord::kStrUndefined || string.isEmpty ())
		dc.unit = DesignCoord::kUndefined;
	else if(string == DesignCoord::kStrAuto)
		dc.unit = DesignCoord::kAuto;
	else
	{
		dc.unit = string.endsWith (DesignCoord::kStrPercent) ? DesignCoord::kPercent : DesignCoord::kCoord;
		string.getIntValue (dc.value);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinAttributes::getDesignCoord (DesignCoord& dc, StringID name) const
{
	if(!exists (name))
		return;
	
	String string = getString (name);
	scanDesignCoord (dc, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinAttributes::setDesignCoord (StringID name, const DesignCoord& dc)
{
	if(dc.isAuto ())
		setString (name, DesignCoord::kStrAuto);
	else if(dc.isUndefined ())
		setString (name, DesignCoord::kStrUndefined);
	else
	{
		String value;
		value.appendIntValue (dc.value);
		if(dc.isPercent ())
			value.append (DesignCoord::kStrPercent);
		setString (name, value);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::exists (StringID name) const
{
	return !getString (name).isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::setString (StringID name, CStringRef value)
{
	return setString (name, String (value));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString SkinAttributes::getCString (StringID name) const
{
	return MutableCString (getString (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::getRect (Rect& r, StringID name) const
{
	String string = getString (name);
	return scanRect (r, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::getSize (Rect& r, StringID name) const
{
	String string = getString (name);
	return scanSize (r, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::getPoint (Point& point, StringID name) const
{
	String string = getString (name);
	if(!string.isEmpty ())
	{
		int x = 0, y = 0;
		if(::sscanf (MutableCString (string), "%d ,%d", &x, &y) > 0)
		{
			point (x, y);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::getPointF (PointF& point, StringID name) const
{
	String string = getString (name);
	if(!string.isEmpty ())
	{
		float x = 0.f, y = 0.f;
		if(::sscanf (MutableCString (string), "%f ,%f", &x, &y) > 0)
		{
			point (x, y);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::getPointF3D (PointF3D& point, StringID name) const
{
	String string = getString (name);
	if(!string.isEmpty ())
	{
		float x = 0.f, y = 0.f, z = 0.f;
		if(::sscanf (MutableCString (string), "%f ,%f, %f", &x, &y, &z) > 0)
		{
			point = {x, y, z};
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::setRect (StringID name, const Rect& rect)
{
	MutableCString temp;
	temp.appendFormat ("%d ,%d ,%d ,%d", rect.left, rect.top, rect.right, rect.bottom);
	return setString (name, String (temp));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::setSize (StringID name, const Rect& size)
{
	MutableCString temp;
	temp.appendFormat ("%d ,%d ,%d ,%d", size.left, size.top, size.getWidth (), size.getHeight ());
	return setString (name, String (temp));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::setPoint (StringID name, const Point& point)
{
	MutableCString temp;
	temp.appendFormat ("%d ,%d", point.x, point.y);
	return setString (name, String (temp));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::setPointF (StringID name, const PointF& point)
{
	MutableCString temp;
	temp.appendFormat ("%f ,%f", point.x, point.y);
	return setString (name, String (temp));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::setPointF3D (StringID name, const PointF3D& point)
{
	MutableCString temp;
	temp.appendFormat ("%f ,%f, %f", point.x, point.y, point.z);
	return setString (name, String (temp));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SkinAttributes::getInt (StringID name, int def) const
{
	int value = 0;
	String string = getString (name);
	if(!string.isEmpty () && string.getIntValue (value))
		return value;
	return def;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::setInt (StringID name, int value)
{
	String string;
	string.appendIntValue (value);
	return setString (name, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float SkinAttributes::getFloat (StringID name, float def) const
{
	double value = 0.;
	String string = getString (name);
	if(!string.isEmpty () && string.getFloatValue (value))
		return (float)value;
	return def;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::setFloat (StringID name, float value)
{
	String string;
	string.appendFloatValue (value);
	return setString (name, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::getBool (StringID name, bool def) const
{
	bool value = def;
	String string = getString (name);
	if(!string.isEmpty ())
	{
		int64 temp = 0;
		if(string.compare (strTrue, false) == Text::kEqual)
			value = true;
		else if(string.getIntValue (temp) && temp != 0)
			value = true;
		else
			value = false;
	}
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::setBool (StringID name, bool value)
{
	return setString (name, value ? strTrue : strFalse);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SkinAttributes::getOptions (StringID name, const StyleDef* style, bool exclusive, int def) const
{
	return parseOptions (getString (name), style, exclusive, def);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SkinAttributes::parseOptions (StringRef optionsString, const StyleDef* style, bool exclusive, int def)
{
	int result = def;
	if(!optionsString.isEmpty ())
	{
		MutableCString cString (optionsString);
		if(exclusive)
			result = Core::EnumInfo::parseOne (cString, style, def);
		else
			result = Core::EnumInfo::parseMultiple (cString, style);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinAttributes::makeOptionsString (String& string, int value, const StyleDef* style, bool exclusive)
{
	MutableCString cString;
	if(exclusive)
		Core::EnumInfo::printOne (cString, value, style);
	else
		Core::EnumInfo::printMultiple (cString, value, style);
	
	string.empty ();
	string.appendASCII (cString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::setOptions (StringID name, int value, const StyleDef* style, bool exclusive)
{
	String string;
	makeOptionsString (string, value, style, exclusive);
	return setString (name, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StyleFlags& SkinAttributes::getOptions (StyleFlags& style, StringID name, const StyleDef* customStyleDef) const
{
	style.common = getOptions (name, View::commonStyles);
	if(customStyleDef)
		style.custom = getOptions (name, customStyleDef);
	return style;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::setOptions (StringID name, const StyleFlags& style, const StyleDef* customStyleDef)
{
	String string;
	makeOptionsString (string, style.common, View::commonStyles);
	if(customStyleDef)
		makeOptionsString (string, style.custom, customStyleDef);
	return setString (name, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::getColorCode (Color& color, StringID name) const
{
	String string = getString (name);
	return !string.isEmpty () ? Colors::fromString (color, string) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinAttributes::setColor (StringID name, const Color& color)
{
	String string;
	Colors::toString (color, string);
	return setString (name, string);
}

//************************************************************************************************
// MutableSkinAttributes
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MutableSkinAttributes, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

String MutableSkinAttributes::getString (StringID name) const
{
	if(kAttrCaseSensitive == false)
	{
		Variant value;
		for(int i = 0, count = attributes.countAttributes (); i < count; i++)
		{
			MutableCString attrName;
			attributes.getAttributeName (attrName, i);
			if(isEqual (attrName, name))
			{
				attributes.getAttributeValue (value, i);
				break;
			}
		}
		return value.toString ();
	}
	else
		return attributes.getString (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MutableSkinAttributes::setString (StringID name, StringRef value)
{
	return attributes.set (name, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MutableSkinAttributes::count () const
{
	return attributes.countAttributes ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString MutableSkinAttributes::getNameAt (int index) const
{
	MutableCString name;
	attributes.getAttributeName (name, index);
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String MutableSkinAttributes::getStringAt (int index) const
{
	String string;
	Variant value;
	if(attributes.getAttributeValue (value, index))
		value.toString (string);
	return string;
}

//************************************************************************************************
// ResolvedSkinAttributes
//************************************************************************************************

ResolvedSkinAttributes::ResolvedSkinAttributes (const SkinAttributes& attributes, const SkinWizard& wizard)
: attributes (attributes),
  wizard (wizard)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ResolvedSkinAttributes::getString (StringID name) const
{
	String string (attributes.getString (name));

	if(string.contains (SkinVariable::prefix))
		return wizard.resolveTitle (string);

	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ResolvedSkinAttributes::setString (StringID name, StringRef value)
{
	CCL_NOT_IMPL ("ResolvedSkinAttributes::setString: only read access allowed")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ResolvedSkinAttributes::count () const
{
	return attributes.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString ResolvedSkinAttributes::getNameAt (int index) const
{
	return attributes.getNameAt (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ResolvedSkinAttributes::getStringAt (int index) const
{
	return attributes.getStringAt (index);
}
