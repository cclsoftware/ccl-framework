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
// Filename    : ccl/gui/skin/skinattributes.h
// Description : Skin Attributes
//
//************************************************************************************************

#ifndef _ccl_skinattributes_h
#define _ccl_skinattributes_h

#include "ccl/base/storage/attributes.h"

#include "ccl/public/gui/graphics/rect.h"
#include "ccl/public/gui/graphics/color.h"
#include "ccl/public/gui/graphics/3d/point3d.h"

#include "ccl/public/gui/framework/designsize.h"
#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

class SkinWizard;

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ForEachSkinAttribute(a,name,value) \
{ for(int __index = 0; __index < (a).count (); __index++) \
{ CCL::MutableCString name ((a).getNameAt (__index)); \
  CCL::String value ((a).getStringAt (__index));

//************************************************************************************************
// SkinAttributes
//************************************************************************************************

class SkinAttributes
{
public:
	SkinAttributes (): verbose (false) {}
	virtual ~SkinAttributes () {}

	PROPERTY_BOOL (verbose, Verbose)

	constexpr static bool kAttrCaseSensitive = false; ///< case sensitivity of skin attributes
	
	template <class StringType> 
	static bool isEqual (const StringType& lhs, CStringPtr rhs) 
	{ return lhs.compare (rhs, kAttrCaseSensitive) == Text::kEqual; }

	static bool scanRect (Rect& r, StringRef string);
	static bool scanSize (Rect& r, StringRef string);
	
	static bool scanDesignRect (DesignSize& ds, StringRef string);
	static bool scanDesignSize (DesignSize& ds, StringRef string);
	static void scanDesignCoord (DesignCoord& dc, StringRef string);

	void getDesignCoord (DesignCoord& dc, StringID name) const;
	void setDesignCoord (StringID name, const DesignCoord& dc);

	virtual String getString (StringID name) const = 0;
	virtual bool setString (StringID name, StringRef value) = 0;
	virtual int count () const = 0;
	virtual MutableCString getNameAt (int index) const = 0;
	virtual String getStringAt (int index) const = 0;

	bool setString (StringID name, CStringRef value);
	MutableCString getCString (StringID name) const;

	bool exists (StringID name) const;

	bool getRect (Rect& rect, StringID name) const;	///< "left, top, right, bottom"
	bool getSize (Rect& size, StringID name) const;	///< "left, top, width, height"
	bool getPoint (Point& point, StringID name) const;
	bool getPointF (PointF& point, StringID name) const;
	bool getPointF3D (PointF3D& point, StringID name) const;

	bool setRect (StringID name, const Rect& rect);
	bool setSize (StringID name, const Rect& size);
	bool setPoint (StringID name, const Point& point);
	bool setPointF (StringID name, const PointF& point);
	bool setPointF3D (StringID name, const PointF3D& point);

	int getInt (StringID name, int def = 0) const;
	bool setInt (StringID name, int value);

	float getFloat (StringID name, float def = 0.f) const;
	bool setFloat (StringID name, float value);

	bool getBool (StringID name, bool def = false) const;
	bool setBool (StringID name, bool value);

	int getOptions (StringID name, const StyleDef* styleDef, bool exclusive = false, int def = 0) const;
	bool setOptions (StringID name, int value, const StyleDef* styleDef, bool exclusive = false);

	static void makeOptionsString (String& string, int value, const StyleDef* styleDef, bool exclusive = false);
	static int parseOptions (StringRef optionsString, const StyleDef* style, bool exclusive = false, int def = 0);

	StyleFlags& getOptions (StyleFlags& style, StringID name, const StyleDef* customStyleDef = nullptr) const;
	bool setOptions(StringID name, const StyleFlags& style, const StyleDef* customStyleDef = nullptr);

	bool getColorCode (Color& color, StringID name) const; ///< RGB/HSL/V only, doesn't resolve symbolic colors defined in skin model!
	bool setColor (StringID name, const Color& color);

protected:
	static const String strTrue;
	static const String strFalse;
};

//************************************************************************************************
// MutableSkinAttributes
//************************************************************************************************

class MutableSkinAttributes: public Object,
							 public SkinAttributes
{
public:
	DECLARE_CLASS (MutableSkinAttributes, Object)

	Attributes& getAttributes ()				{ return attributes; }
	const Attributes& getAttributes () const	{ return attributes; }

	// SkinAttributes
	String getString (StringID name) const override;
	bool setString (StringID name, StringRef value) override;
	int count () const override;
	MutableCString getNameAt (int index) const override;
	String getStringAt (int index) const override;

protected:
	Attributes attributes;
};

//************************************************************************************************
// ResolvedSkinAttributes
/** Resolves skin variables in attribute strings before parsing values. */
//************************************************************************************************

class ResolvedSkinAttributes: public SkinAttributes
{
public:
	ResolvedSkinAttributes (const SkinAttributes& attributes, const SkinWizard& wizard);

	// SkinAttributes
	String getString (StringID name) const override;
	bool setString (StringID name, StringRef value) override;
	int count () const override;
	MutableCString getNameAt (int index) const override;
	String getStringAt (int index) const override;

private:
	const SkinAttributes& attributes;
	const SkinWizard& wizard;

	using SuperClass = SkinAttributes;
};

} // namespace CCL

#endif // _ccl_skinattributes_h
