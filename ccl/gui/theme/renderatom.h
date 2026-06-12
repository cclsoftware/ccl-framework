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
// Filename    : ccl/gui/theme/renderatom.h
// Description : Render Atom
//
//************************************************************************************************

#ifndef _ccl_renderatom_h
#define _ccl_renderatom_h

#include "ccl/gui/theme/visualstyle.h"

#include "ccl/public/gui/framework/itextmodel.h"
#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

struct UpdateRgn;

//************************************************************************************************
// RenderAtom
//************************************************************************************************

class RenderAtom: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (RenderAtom, Object)

	RenderAtom (CStringRef name, RectFRef size);
	~RenderAtom ();

	PROPERTY_MUTABLE_CSTRING (name, Name)
	RectFRef getSize () const { return size; }
	void setSize (RectFRef size);

	// Basic
	PROPERTY_OBJECT (Brush, brush, Brush)
	PROPERTY_VARIABLE (float, alpha, Alpha)
	PROPERTY_VARIABLE (float, borderLeftTopRadius, BorderLeftTopRadius)
	PROPERTY_VARIABLE (float, borderLeftBottomRadius, BorderLeftBottomRadius)
	PROPERTY_VARIABLE (float, borderRightTopRadius, BorderRightTopRadius)
	PROPERTY_VARIABLE (float, borderRightBottomRadius, BorderRightBottomRadius)

	void setBorderRadius (float radius);

	// Text
	PROPERTY_OBJECT (Font, textFont, TextFont)
	PROPERTY_OBJECT (TextFormat, textFormat, TextFormat)
	PROPERTY_VARIABLE (Font::TrimMode, textTrimMode, TextTrimMode)

	void setTextModel (ITextModel* model);

	// Image
	PROPERTY_SHARED_POINTER (IImage, image, Image)
	PROPERTY_VARIABLE (float, imageZoom, ImageZoom)

	// Options
	PROPERTY_FLAG (options, 1<<0, isTextVertical)
	PROPERTY_FLAG (options, 1<<1, isTextFit)
	PROPERTY_FLAG (options, 1<<2, isTextScale)
	PROPERTY_FLAG (options, 1<<3, isTextMultiline)
	PROPERTY_FLAG (options, 1<<4, isTextMarkup)

	PROPERTY_FLAG (options, 1<<5, isImageFit)
	PROPERTY_FLAG (options, 1<<6, isImageFitAllowStretch)
	PROPERTY_FLAG (options, 1<<7, isImageFitAllowZoom)
	PROPERTY_FLAG (options, 1<<8, isImageCentered)

	void draw (IGraphics& graphics, const UpdateRgn& updateRgn);

	struct ColorContrastHelper
	{
		PROPERTY_VARIABLE (Color, defaultColor, DefaultColor)
		PROPERTY_VARIABLE (Color, contrastBrightColor, ContrastBrightColor)
		PROPERTY_VARIABLE (Color, contrastDarkColor, ContrastDarkColor)
		PROPERTY_VARIABLE (Color, contrastTransparentColor, ContrastTransparentColor)

		Color getForegroundColor (float brightColorThreshold = .35f, const Color* customBackcolor = nullptr) const;
	};
	ColorContrastHelper& getColorContrastHelper ();

	void setStyleCondition (CStringRef condition, bool state);

protected:
	RectF size;
	ITextModel* textModel;
	ITextLayout* textLayout;
	int options;
	String displayedText; // possibly truncated to fit into the box and has the labelParam text prepended

	StyleConditionContext conditionContext;
	ColorContrastHelper* contrastHelper;

	PROPERTY_FLAG (options, 1<<20, hasTextChanged)
	PROPERTY_FLAG (options, 1<<21, hasSizeChanged)

	void clipBorderRadius (IGraphics& graphics);
	StringRef getText ();

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

//************************************************************************************************
// AtomName
//************************************************************************************************

namespace AtomName
{
	const CStringPtr kBackground = "background";
}

} // namespace CCL

#endif // _ccl_renderatom_h
