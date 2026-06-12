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

#include "ccl/public/gui/framework/designsize.h"
#include "ccl/public/gui/graphics/types.h"

#include "ccl/public/text/cstring.h"

namespace CCL {

interface IContainer;
interface IColorScheme;
interface IStyleConditionContext;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (VisualStyle, 0xc5f60f5b, 0x31b5, 0x47c6, 0x8f, 0x79, 0xdd, 0x18, 0x8a, 0xbc, 0x33, 0xb7)
}

/** Visual style metric type. */
typedef float VisualStyleMetric;

/** Visual style options type. */
typedef int VisualStyleOptions;

//************************************************************************************************
// IVisualStyleData
/** Visual style data holds colors, fonts, metrics, etc.
	\ingroup gui */
//************************************************************************************************

interface IVisualStyleData: IUnknown
{
	using Metric = VisualStyleMetric;
	using Options = VisualStyleOptions;

	/** Get name of style data. */
	virtual StringID CCL_API getName () const = 0;

	/** Set name of style data. */
	virtual void CCL_API setName (StringID name) = 0;

	/** Get color by name, returns default color if not found. */
	virtual ColorRef CCL_API getColor (StringID name, ColorRef defaultColor = Colors::kBlack) const = 0;

	/** Get color by name, returns false if not found. */
	virtual tbool CCL_API getColor (Color& color, StringID name) const = 0;

	/** Set color by name. */
	virtual void CCL_API setColor (StringID name, ColorRef color) = 0;

	/** Get font by name, returns default font if not found. */
	virtual FontRef CCL_API getFont (StringID name, FontRef defaultFont = Font::getDefaultFont ()) const = 0;

	/** Get font by name, returns false if not found. */
	virtual tbool CCL_API getFont (Font& font, StringID name) const = 0;

	/** Set font by name. */
	virtual void CCL_API setFont (StringID name, FontRef font) = 0;

	/** Get metric by name (float), returns default value if not found. */
	virtual Metric CCL_API getMetric (StringID name, Metric defaultValue = 0) const = 0;

	/** Get metric by name (float), returns false if not found. */
	virtual tbool CCL_API getMetric (Metric& value, StringID name) const = 0;

	/** Set metric by name (float). */
	virtual void CCL_API setMetric (StringID name, Metric value) = 0;

	/** Get metric by name (DesignCoord), returns default value if not found. */
	virtual DesignCoord CCL_API getDesignMetric (StringID name, DesignCoordRef defaultValue = DesignCoord::kUndefined) const = 0;

	/** Get metric by name (DesignCoord), returns false if not found. */
	virtual tbool CCL_API getDesignMetric (DesignCoord& value, StringID name) const = 0;

	/** Set metric by name (DesignCoord). */
	virtual void CCL_API setDesignMetric (StringID name, DesignCoordRef value) = 0;

	/** Get string by name, returns default value if not found. */
	virtual CString CCL_API getString (StringID name, StringID defaultValue = CString::kEmpty) const = 0;

	/** Get string by name, returns false if not found. */
	virtual tbool CCL_API getString (MutableCString& string, StringID name) const = 0;

	/** Set string by name. */
	virtual void CCL_API setString (StringID name, StringID value) = 0;

	/** Get options by name, returns default value if not found. */
	virtual Options CCL_API getOptions (StringID name, Options defaultOptions = 0) const = 0;

	/** Get options by name, returns false if not found. */
	virtual tbool CCL_API getOptions (Options& options, StringID name) const = 0;

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

	/** Check if style data has references to given color scheme. */
	virtual tbool CCL_API hasReferences (IColorScheme& scheme) const = 0;

	DECLARE_IID (IVisualStyleData)

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
};

DEFINE_IID (IVisualStyleData, 0x8600e5f1, 0xb98c, 0x444d, 0xa2, 0x5a, 0x71, 0x21, 0x2d, 0xdb, 0x3e, 0x90)
DEFINE_STRINGID_MEMBER (IVisualStyleData, kColors, "colors")
DEFINE_STRINGID_MEMBER (IVisualStyleData, kFonts, "fonts")
DEFINE_STRINGID_MEMBER (IVisualStyleData, kMetrics, "metrics")
DEFINE_STRINGID_MEMBER (IVisualStyleData, kStrings, "strings")
DEFINE_STRINGID_MEMBER (IVisualStyleData, kOptions, "options")
DEFINE_STRINGID_MEMBER (IVisualStyleData, kImages, "images")
DEFINE_STRINGID_MEMBER (IVisualStyleData, kGradients, "gradients")

//************************************************************************************************
// StyleDataReader
/** Read common attributes from visual style data.
	\ingroup gui */
//************************************************************************************************

class StyleDataReader
{
public:
	StyleDataReader (const IVisualStyleData& data);

	Color getForeColor () const;				///< Get foreground color
	Color getBackColor () const;				///< Get background color
	Color getHiliteColor () const;				///< Get hilite color
	Color getTextColor () const;				///< Get text color
	VisualStyleMetric getStrokeWidth () const;	///< Get stroke width
	Pen getForePen () const;					///< Get foreground pen
	Pen getBackPen () const;					///< Get background pen
	Brush getForeBrush () const;				///< Get foreground brush (solid or gradient)
	Brush getBackBrush () const;				///< Get background brush (solid or gradient)
	Brush getTextBrush () const;				///< Get text brush (solid or gradient)
	Font getTextFont () const;					///< Get text font
	Alignment getTextAlignment () const;		///< Get text alignment
	VisualStyleOptions getTextOptions () const;	///< Get text options
	TextFormat getTextFormat () const;			///< Get text format
	IImage* getBackgroundImage () const;		///< Get background image
	void getPadding (Rect& padding) const;		///< Get padding

	template <typename T> T getMetric (StringID name, T defaultValue) const;

protected:
	const IVisualStyleData& data;
};

//************************************************************************************************
// IVisualStyle
/** A visual style describes the appearance of an UI element.
	\ingroup gui */
//************************************************************************************************

interface IVisualStyle: IVisualStyleData
{
	/** Copy columns from other style. */
	virtual tbool CCL_API copyFrom (const IVisualStyle& other) = 0;

	/** Get inherited visual style. */
	virtual const IVisualStyle* CCL_API getInherited () const = 0;

	/** Get original visual style (usually this, or a source style this one delegates to, e.g. for a <styleselector>). */
	virtual const IVisualStyle* CCL_API getOriginal () const = 0;

	/** Get container with nested visual style conditions. */
	virtual const IContainer& CCL_API getStyleConditions () const = 0;

	/** Get visual style data for given named condition. */
	virtual const IVisualStyleData* CCL_API getStyleCondition (StringID name) const = 0;

	/** Get color with conditon context, returns false if not found. */
	virtual tbool CCL_API getColor (Color& color, StringID name, IStyleConditionContext* context) const = 0;

	/** Get font with conditon context, returns false if not found. */
	virtual tbool CCL_API getFont (Font& font, StringID name, IStyleConditionContext* context) const = 0;

	/** Get metric with conditon context (float), returns false if not found. */
	virtual tbool CCL_API getMetric (Metric& value, StringID name, IStyleConditionContext* context) const = 0;

	/** Get metric with conditon context (DesignCoord), returns false if not found. */
	virtual tbool CCL_API getDesignMetric (DesignCoord& value, StringID name, IStyleConditionContext* context) const = 0;

	/** Get string with conditon context, returns false if not found. */
	virtual tbool CCL_API getString (MutableCString& string, StringID name, IStyleConditionContext* context) const = 0;

	/** Get options with conditon context, returns false if not found. */
	virtual tbool CCL_API getOptions (Options& options, StringID name, IStyleConditionContext* context) const = 0;

	/** Get image with conditon context. */
	virtual IImage* CCL_API getImage (StringID name, IStyleConditionContext* context) const = 0;

	/** Get gradient with conditon context. */
	virtual IGradient* CCL_API getGradient (StringID name, IStyleConditionContext* context) const = 0;

	using IVisualStyleData::getColor;
	using IVisualStyleData::getFont;
	using IVisualStyleData::getMetric;
	using IVisualStyleData::getDesignMetric;
	using IVisualStyleData::getString;
	using IVisualStyleData::getOptions;
	using IVisualStyleData::getImage;
	using IVisualStyleData::getGradient;

	DECLARE_IID (IVisualStyle)

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Common attributes
	//////////////////////////////////////////////////////////////////////////////////////////////

	Color getForeColor () const;
	Color getBackColor () const;
	Color getHiliteColor () const;
	Color getTextColor () const;
	Metric getStrokeWidth () const;
	Pen getForePen () const;
	Pen getBackPen () const;
	Brush getForeBrush () const;
	Brush getBackBrush () const;
	Brush getTextBrush () const;
	Font getTextFont () const;
	Alignment getTextAlignment () const;
	Options getTextOptions () const;
	TextFormat getTextFormat () const;
	IImage* getBackgroundImage () const;
	void getPadding (Rect& padding) const;

	template <typename T> T getMetric (StringID name, T defaultValue) const;
};

DEFINE_IID (IVisualStyle, 0xb5b3485e, 0x1549, 0x483e, 0xb7, 0x58, 0xce, 0x62, 0x16, 0xfd, 0x7e, 0x58)

//************************************************************************************************
// IStyleConditionContext
/** Filter interface to access visual style items based on named conditions.
	\ingroup gui */
//************************************************************************************************

interface IStyleConditionContext: IUnknown
{
	/** Check if given conditions are matching. */
	virtual tbool CCL_API matches (StringID conditions) const = 0;

	DECLARE_IID (IStyleConditionContext)
};

DEFINE_IID (IStyleConditionContext, 0xb3834881, 0x23c9, 0x4734, 0xb3, 0xcb, 0xeb, 0xd8, 0xe0, 0xb8, 0xec, 0xb2)

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
// StyleDataReader inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline StyleDataReader::StyleDataReader (const IVisualStyleData& data)
: data (data) 
{}

inline Color StyleDataReader::getForeColor () const
{ return data.getColor (StyleID::kForeColor, Colors::kBlack); }

inline Color StyleDataReader::getHiliteColor () const
{ return data.getColor (StyleID::kHiliteColor, Colors::kGray); }

inline Color StyleDataReader::getBackColor () const
{ return data.getColor (StyleID::kBackColor, Colors::kWhite); }

inline Color StyleDataReader::getTextColor () const
{ return data.getColor (StyleID::kTextColor, Colors::kBlack); }

inline VisualStyleMetric StyleDataReader::getStrokeWidth () const
{ return data.getMetric (StyleID::kStrokeWidth, 1.f); }

inline Pen StyleDataReader::getForePen () const
{ return Pen (getForeColor (), getStrokeWidth ()); }

inline Pen StyleDataReader::getBackPen () const
{ return Pen (getBackColor (), getStrokeWidth ()); }

inline Brush StyleDataReader::getForeBrush () const
{ 
	if(auto* gradient = data.getGradient (StyleID::kForeColor))
		return GradientBrush (gradient);
	else	
		return SolidBrush (getForeColor ()); 
}

inline Brush StyleDataReader::getBackBrush () const
{
	if(auto* gradient = data.getGradient (StyleID::kBackColor))
		return GradientBrush (gradient);
	else	
		return SolidBrush (getBackColor ()); 
}

inline Brush StyleDataReader::getTextBrush () const
{
	if(auto* gradient = data.getGradient (StyleID::kTextColor))
		return GradientBrush (gradient);
	else	
		return SolidBrush (getTextColor ()); 
}

inline Font StyleDataReader::getTextFont () const
{ return data.getFont (StyleID::kTextFont); }

inline Alignment StyleDataReader::getTextAlignment () const
{ return Alignment (data.getOptions (StyleID::kTextAlign)); }

inline VisualStyleOptions StyleDataReader::getTextOptions () const
{ return data.getOptions (StyleID::kTextOptions); }

inline TextFormat StyleDataReader::getTextFormat () const
{ return TextFormat (getTextAlignment (), getTextOptions ()); }

inline IImage* StyleDataReader::getBackgroundImage () const
{ return data.getImage (StyleID::kBackground); }

template <typename T> 
T StyleDataReader::getMetric (StringID name, T defaultValue) const
{ return (T)data.getMetric (name, (VisualStyleMetric)defaultValue); }

template <> 
inline bool StyleDataReader::getMetric (StringID name, bool defaultValue) const
{ return getMetric (name, (VisualStyleMetric)defaultValue) != 0; }

inline void StyleDataReader::getPadding (Rect& padding) const
{
	Coord p = getMetric<Coord> (StyleID::kPadding, 0); // fallback value
	padding.left = getMetric<Coord> (StyleID::kPaddingLeft, p);
	padding.top = getMetric<Coord> (StyleID::kPaddingTop, p);
	padding.right = getMetric<Coord> (StyleID::kPaddingRight, p);
	padding.bottom = getMetric<Coord> (StyleID::kPaddingBottom, p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IVisualStyle inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Color IVisualStyle::getForeColor () const
{ return StyleDataReader (*this).getForeColor (); }

inline Color IVisualStyle::getHiliteColor () const
{ return StyleDataReader (*this).getHiliteColor (); }

inline Color IVisualStyle::getBackColor () const
{ return StyleDataReader (*this).getBackColor (); }

inline Color IVisualStyle::getTextColor () const
{ return StyleDataReader (*this).getTextColor (); }

inline IVisualStyle::Metric IVisualStyle::getStrokeWidth () const
{ return StyleDataReader (*this).getStrokeWidth (); }

inline Pen IVisualStyle::getForePen () const
{ return StyleDataReader (*this).getForePen (); }

inline Pen IVisualStyle::getBackPen () const
{ return StyleDataReader (*this).getBackPen (); }

inline Brush IVisualStyle::getForeBrush () const
{ return StyleDataReader (*this).getForeBrush (); }

inline Brush IVisualStyle::getBackBrush () const
{ return StyleDataReader (*this).getBackBrush (); }

inline Brush IVisualStyle::getTextBrush () const
{ return StyleDataReader (*this).getTextBrush (); }

inline Font IVisualStyle::getTextFont () const
{ return StyleDataReader (*this).getTextFont (); }

inline Alignment IVisualStyle::getTextAlignment () const
{ return StyleDataReader (*this).getTextAlignment (); }

inline IVisualStyle::Options IVisualStyle::getTextOptions () const
{ return StyleDataReader (*this).getTextOptions (); }

inline TextFormat IVisualStyle::getTextFormat () const
{ return StyleDataReader (*this).getTextFormat (); }

inline IImage* IVisualStyle::getBackgroundImage () const
{ return StyleDataReader (*this).getBackgroundImage (); }

template <typename T> 
T IVisualStyle::getMetric (StringID name, T defaultValue) const
{ return StyleDataReader (*this).getMetric<T> (name, defaultValue); }

inline void IVisualStyle::getPadding (Rect& padding) const
{ StyleDataReader (*this).getPadding (padding); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_ivisualstyle_h
