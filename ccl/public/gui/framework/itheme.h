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
// Filename    : ccl/public/gui/framework/itheme.h
// Description : Theme Interface
//
//************************************************************************************************

#ifndef _ccl_itheme_h
#define _ccl_itheme_h

#include "ccl/public/gui/framework/themeelements.h"
#include "ccl/public/gui/framework/ivisualstyle.h"

namespace CCL {

interface IView;
interface IMouseCursor;
interface IThemePainter;
interface IThemeStatics;
interface IAttributeList;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (ThemeStatics, 0x7d5878ad, 0xc251, 0x4c2c, 0xa4, 0x3d, 0x68, 0xf2, 0x3a, 0x18, 0x36, 0xfb)
}

//************************************************************************************************
// ITheme
/**	A theme defines the look and feel of an application.
	It provides resources like images, mouse cursors, styles, etc. and can create 
	views by name. The theme is loaded from a skin package file or folder.
	\ingroup gui_skin */
//************************************************************************************************

interface ITheme: IUnknown
{	
	/** Get theme identifier. */
	virtual StringID CCL_API getThemeID () const = 0;

	/** Get common theme metric. */
	virtual int CCL_API getThemeMetric (ThemeMetricID which) = 0;

	/** Get common theme color. */
	virtual ColorRef CCL_API getThemeColor (ThemeColorID which) = 0;
	
	/** Get common theme font. */
	virtual FontRef CCL_API getThemeFont (ThemeFontID which) = 0;

	/** Get common theme cursor. */
	virtual IMouseCursor* CCL_API getThemeCursor (ThemeCursorID which) = 0;

	/** Get style definition by name. If style isn't present, a default style is returned. */
	virtual const IVisualStyle& CCL_API getStyle (StringID name) = 0;

	/** Get resource by name. */
	virtual IUnknown* CCL_API getResource (StringID name) = 0;

	/** Get gradient by name. */
	virtual IGradient* CCL_API getGradient (StringID name) = 0;

	/** Get image resource by name. */
	virtual IImage* CCL_API getImage (StringID name) = 0;

	/** Get mouse cursor by name. */
	virtual IMouseCursor* CCL_API getCursor (StringID name) = 0;

	/** Get theme painter. */
	virtual IThemePainter& CCL_API getPainter () = 0;

	/** Get theme statics. */
	virtual IThemeStatics& CCL_API getStatics () = 0;

	/** Create view by name with given controller and optional arguments. */
	virtual IView* CCL_API createView (StringID name, IUnknown* controller, IAttributeList* arguments = nullptr) = 0;

	DECLARE_IID (ITheme)
};

DEFINE_IID (ITheme, 0x9d9e7cb6, 0xfe4a, 0x426e, 0x86, 0x41, 0xbd, 0xda, 0x45, 0x84, 0x84, 0x3b)

//************************************************************************************************
// IThemePainter
/** A theme painter provides drawing methods for theme elements.
	\ingroup gui_skin */
//************************************************************************************************

interface IThemePainter: IUnknown
{
	/**	Draw theme element. 
		For matching text color, use ThemeElements::kPushButtonTextColor. */
	virtual tresult CCL_API drawElement (IGraphics& graphics, RectRef rect, ThemeElementID id, ThemeElementState state) = 0;

	/** Draws the frame with the best matching resolution (with contextColor if applicable) from a MultiImage with frames of different sizes. */
	virtual tresult CCL_API drawBestMatchingFrame (IGraphics& graphics, IImage* image, RectRef rect, const ImageMode* mode = nullptr, ColorRef contextColor = 0, tbool scaleAlways = false) const = 0;

	/** Draws current frame of MultiImage centered (with contextColor if applicable) */
	virtual tresult CCL_API drawFrameCentered (IGraphics& graphics, IImage* image, RectRef rect, const ImageMode* mode = nullptr, ColorRef contextColor = 0) const = 0;
	
	DECLARE_IID (IThemePainter)
};

DEFINE_IID (IThemePainter, 0x7404ed97, 0x3cd1, 0x4ff7, 0x96, 0x4a, 0xf4, 0x40, 0xf4, 0xa1, 0xe2, 0x95)

//************************************************************************************************
// IThemeStatics
/**	Access to static members of theme class.
	\ingroup gui_skin */
//************************************************************************************************

interface IThemeStatics: IUnknown
{
	/** Get theme metric name. */
	virtual CStringPtr CCL_API getThemeMetricName (ThemeMetricID which) const = 0;

	/** Get theme color name. */
	virtual CStringPtr CCL_API getThemeColorName (ThemeColorID which) const = 0;

	/** Get theme font name. */
	virtual CStringPtr CCL_API getThemeFontName (ThemeFontID which) const = 0;

	/** Get theme cursor name. */
	virtual CStringPtr CCL_API getThemeCursorName (ThemeCursorID which) const = 0;

	/** Get global visual style. */
	virtual const IVisualStyle& CCL_API getGlobalStyle () const = 0;

	DECLARE_IID (IThemeStatics)

	//////////////////////////////////////////////////////////////////////////////////////////////

	inline FontRef getStandardFont ()
	{
		return getGlobalStyle ().getFont (ThemeNames::kStandardFont);
	}	
};

DEFINE_IID (IThemeStatics, 0xebbe866f, 0xf06a, 0x4445, 0xaa, 0x56, 0x3c, 0x85, 0x8b, 0x30, 0x84, 0x63)

} // namespace CCL

#endif // _ccl_itheme_h
