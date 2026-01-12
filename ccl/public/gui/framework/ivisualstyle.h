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
// Filename    : ccl/public/gui/framework/ivisualstyle.h
// Description : Visual Style Interface
//
//************************************************************************************************

#ifndef _ccl_ivisualstyle_h
#define _ccl_ivisualstyle_h

#include "ccl/public/gui/graphics/types.h"

#include "ccl/public/text/cstring.h"

namespace CCL {

interface IColorScheme;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (VisualStyle, 0xc5f60f5b, 0x31b5, 0x47c6, 0x8f, 0x79, 0xdd, 0x18, 0x8a, 0xbc, 0x33, 0xb7)
}

//************************************************************************************************
// IVisualStyle
/** A visual style holds colors, fonts, metrics, etc. describing the appearance of an UI element.
	\ingroup gui */
//************************************************************************************************

interface IVisualStyle: IUnknown
{	
	/** Metric type. */
	typedef float Metric;

	/** Options type. */
	typedef int Options;

	/** Get name of visual style. */
	virtual StringID CCL_API getName () const = 0;

	/** Get color by name. If color does not exist, the default color is returned. */
	virtual ColorRef CCL_API getColor (StringID name, ColorRef defaultColor = Colors::kBlack) const = 0;

	/** Set color by name. */
	virtual void CCL_API setColor (StringID name, ColorRef color) = 0;

	/** Get font by name. If font does not exist, the default font is returned. */
	virtual FontRef CCL_API getFont (StringID name, FontRef defaultFont = Font::getDefaultFont ()) const = 0;

	/** Set font by name. */
	virtual void CCL_API setFont (StringID name, FontRef font) = 0;

	/** Get metric by name. If it does not exist, the default value is returned. */
	virtual Metric CCL_API getMetric (StringID name, Metric defaultValue = 0) const = 0;

	/** Set metric by name. */
	virtual void CCL_API setMetric (StringID name, Metric value) = 0;

	/** Get string by name. If it does not exist, the default value is returned. */
	virtual CString CCL_API getString (StringID name, StringID defaultValue = CString::kEmpty) const = 0;

	/** Set string by name. */
	virtual void CCL_API setString (StringID name, StringID value) = 0;

	/** Get options by name. If it does not exist, the default options are returned. */
	virtual Options CCL_API getOptions (StringID name, Options defaultOptions = 0) const = 0;

	/** Set options by name. */
	virtual void CCL_API setOptions (StringID name, Options options) = 0;

	/** Get image by name. */
	virtual IImage* CCL_API getImage (StringID name) const = 0;

	/** Set image by name. */
	virtual void CCL_API setImage (StringID name, IImage* image) = 0;

	/** Get gradient by name. */
	virtual IGradient* CCL_API getGradient (StringID name) const = 0;

	/** Set gradient by name. */
	virtual void CCL_API setGradient (StringID name, IGradient* gradient) = 0;

	/** Check if style has references to given color scheme. */
	virtual tbool CCL_API hasReferences (IColorScheme& scheme) const = 0;

	/** Copy columns from other style. */
	virtual tbool CCL_API copyFrom (const IVisualStyle& other) = 0;

	/** Get inherited visual style. */
	virtual const IVisualStyle* CCL_API getInherited () const = 0;

	/** Get original visual style (usually this, or a source style this one delegates to, e.g. for a <styleselector>). */
	virtual const IVisualStyle* CCL_API getOriginal () const = 0;

	DECLARE_IID (IVisualStyle)

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Additional properties (IObject)
	//////////////////////////////////////////////////////////////////////////////////////////////

	DECLARE_STRINGID_MEMBER (kColors)		///< colors [IArrayObject]
	DECLARE_STRINGID_MEMBER (kFonts)		///< fonts [IArrayObject]
	DECLARE_STRINGID_MEMBER (kMetrics)		///< metrics [IArrayObject]
	DECLARE_STRINGID_MEMBER (kStrings)		///< strings [IArrayObject]
	DECLARE_STRINGID_MEMBER (kOptions)		///< options [IArrayObject]
	DECLARE_STRINGID_MEMBER (kImages)		///< images [IArrayObject]
	DECLARE_STRINGID_MEMBER (kGradients)	///< gradients [IArrayObject]

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Common style attributes
	//////////////////////////////////////////////////////////////////////////////////////////////

	Color getForeColor () const;			///< Get foreground color
	Color getBackColor () const;			///< Get background color
	Color getHiliteColor () const;			///< Get hilite color
	Color getTextColor () const;			///< Get text color
	Metric getStrokeWidth () const;			///< Get stroke width
	Pen getForePen () const;				///< Get foreground pen
	Pen getBackPen () const;				///< Get background pen
	Brush getForeBrush () const;			///< Get foreground brush (solid or gradient)
	Brush getBackBrush () const;			///< Get background brush (solid or gradient)
	Brush getTextBrush () const;			///< Get text brush (solid or gradient)
	Font getTextFont () const;				///< Get text font
	Alignment getTextAlignment () const;	///< Get text alignment
	Options getTextOptions () const;		///< Get text options
	TextFormat getTextFormat () const;		///< Get text format
	IImage* getBackgroundImage () const;	///< Get background image
	void getPadding (Rect& padding) const;	///< Get padding
	
	template <typename T> T getMetric (StringID name, T defaultValue) const;
};

DEFINE_IID (IVisualStyle, 0xb5b3485e, 0x1549, 0x483e, 0xb7, 0x58, 0xce, 0x62, 0x16, 0xfd, 0x7e, 0x58)
DEFINE_STRINGID_MEMBER (IVisualStyle, kColors, "colors")
DEFINE_STRINGID_MEMBER (IVisualStyle, kFonts, "fonts")
DEFINE_STRINGID_MEMBER (IVisualStyle, kMetrics, "metrics")
DEFINE_STRINGID_MEMBER (IVisualStyle, kStrings, "strings")
DEFINE_STRINGID_MEMBER (IVisualStyle, kOptions, "options")
DEFINE_STRINGID_MEMBER (IVisualStyle, kImages, "images")
DEFINE_STRINGID_MEMBER (IVisualStyle, kGradients, "gradients")

//************************************************************************************************
// IVisualStyleItem
/**	Named item in a visual style (color, font, metric, etc.). Access via IArrayObject properties
	like IVisualStyle::kColors.
	\ingroup gui */
//************************************************************************************************

interface IVisualStyleItem: IUnknown
{
	/** Get item name. */
	virtual StringID CCL_API getItemName () const = 0;

	/** Get item value, can be integer, string (including colors), IFont, IImage, IGradient. */
	virtual void CCL_API getItemValue (Variant& value) const = 0;

	DECLARE_IID (IVisualStyleItem)
};

DEFINE_IID (IVisualStyleItem, 0x60E771DF, 0x299B, 0x4B4D, 0x88, 0x6C, 0x25, 0x4B, 0x5A, 0xB8, 0x7D, 0xCA)

//////////////////////////////////////////////////////////////////////////////////////////////////
// Common Style Attribute Identifier
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace StyleID
{
	DEFINE_STRINGID (kForeColor, "forecolor")
	DEFINE_STRINGID (kHiliteColor, "hilitecolor")
	DEFINE_STRINGID (kBackColor, "backcolor")
	DEFINE_STRINGID (kTextColor, "textcolor")
	DEFINE_STRINGID (kStrokeWidth, "strokewidth")
	DEFINE_STRINGID (kTextFont, "textfont")
	DEFINE_STRINGID (kTextAlign, "textalign")
	DEFINE_STRINGID (kTextOptions, "textoptions")		
	DEFINE_STRINGID (kBackground, "background")
	DEFINE_STRINGID (kPadding, "padding")		
	DEFINE_STRINGID (kPaddingLeft, "padding.left")		
	DEFINE_STRINGID (kPaddingTop, "padding.top")		
	DEFINE_STRINGID (kPaddingRight, "padding.right")		
	DEFINE_STRINGID (kPaddingBottom, "padding.bottom")		
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IVisualStyle inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Color IVisualStyle::getForeColor () const
{ return getColor (StyleID::kForeColor, Colors::kBlack); }

inline Color IVisualStyle::getHiliteColor () const
{ return getColor (StyleID::kHiliteColor, Colors::kGray); }

inline Color IVisualStyle::getBackColor () const
{ return getColor (StyleID::kBackColor, Colors::kWhite); }

inline Color IVisualStyle::getTextColor () const
{ return getColor (StyleID::kTextColor, Colors::kBlack); }

inline IVisualStyle::Metric IVisualStyle::getStrokeWidth () const
{ return getMetric (StyleID::kStrokeWidth, 1.f); }

inline Pen IVisualStyle::getForePen () const
{ return Pen (getForeColor (), getStrokeWidth ()); }

inline Pen IVisualStyle::getBackPen () const
{ return Pen (getBackColor (), getStrokeWidth ()); }

inline Brush IVisualStyle::getForeBrush () const
{ 
	if(auto gradient = getGradient (StyleID::kForeColor))
		return GradientBrush (gradient);
	else	
		return SolidBrush (getForeColor ()); 
}

inline Brush IVisualStyle::getBackBrush () const
{
	if(auto gradient = getGradient (StyleID::kBackColor))
		return GradientBrush (gradient);
	else	
		return SolidBrush (getBackColor ()); 
}

inline Brush IVisualStyle::getTextBrush () const
{
	if(auto gradient = getGradient (StyleID::kTextColor))
		return GradientBrush (gradient);
	else	
		return SolidBrush (getTextColor ()); 
}

inline Font IVisualStyle::getTextFont () const
{ return getFont (StyleID::kTextFont); }

inline Alignment IVisualStyle::getTextAlignment () const
{ return Alignment (getOptions (StyleID::kTextAlign)); }

inline IVisualStyle::Options IVisualStyle::getTextOptions () const
{ return getOptions (StyleID::kTextOptions); }

inline TextFormat IVisualStyle::getTextFormat () const
{ return TextFormat (getTextAlignment (), getTextOptions ()); }

inline IImage* IVisualStyle::getBackgroundImage () const
{ return getImage (StyleID::kBackground); }

template <typename T> 
T IVisualStyle::getMetric (StringID name, T defaultValue) const
{ return (T)getMetric (name, (Metric)defaultValue); }

template <> 
inline bool IVisualStyle::getMetric (StringID name, bool defaultValue) const
{ return getMetric (name, (Metric)defaultValue) != 0; }

inline void IVisualStyle::getPadding (Rect& padding) const
{
	Coord p = getMetric<Coord> (StyleID::kPadding, 0); // fallback value
	padding.left = getMetric<Coord> (StyleID::kPaddingLeft, p);
	padding.top = getMetric<Coord> (StyleID::kPaddingTop, p);
	padding.right = getMetric<Coord> (StyleID::kPaddingRight, p);
	padding.bottom = getMetric<Coord> (StyleID::kPaddingBottom, p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_ivisualstyle_h
