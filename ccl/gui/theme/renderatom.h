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
#include "ccl/gui/theme/visualstyleclass.h"

#include "ccl/public/gui/framework/itextmodel.h"
#include "ccl/public/gui/framework/styleflags.h"

#include "ccl/gui/graphics/imaging/image.h"

namespace CCL {

struct UpdateRgn;
class RenderAtom;

//************************************************************************************************
// IRenderAtomClient
//************************************************************************************************

interface IRenderAtomClient: IUnknown
{
	virtual void invalidate (const RenderAtom& atom) = 0;

	DECLARE_IID (IRenderAtomClient)
};

//************************************************************************************************
// RenderAtomTransaction
//************************************************************************************************

struct RenderAtomTransaction
{
	struct PendingAnimation
	{
		RenderAtom* atom;
		CString propertyId;
		SharedPtr<IAnimation> animation;
		Variant startValue;

		PendingAnimation (RenderAtom* atom = nullptr, StringID propertyId = nullptr, IAnimation* animation = nullptr)
		: atom (atom),
		  propertyId (propertyId),
		  animation (animation)
		{}

		void operator = (const PendingAnimation& other)
		{
			atom = other.atom;
			propertyId = other.propertyId;
			animation = other.animation;
			startValue = other.startValue;
		}

		bool operator == (const PendingAnimation& other) const
		{
			return atom == other.atom && propertyId == other.propertyId;
		}
	};

	Vector<PendingAnimation> animations;
};

//************************************************************************************************
// RenderAtom
//************************************************************************************************

class RenderAtom: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (RenderAtom, Object)

	RenderAtom (StringID name, int tag = -1);
	~RenderAtom ();

	static MutableCString makeId (StringID atomName, StringID propertyId);

	PROPERTY_MUTABLE_CSTRING (name, Name)
	PROPERTY_VARIABLE (int, tag, Tag)
	PROPERTY_POINTER (IRenderAtomClient, client, Client)

	// Basic
	RectFRef getSize () const { return size; }
	void setSize (RectFRef size);
	PROPERTY_VARIABLE (float, offsetX, OffsetX)
	PROPERTY_VARIABLE (float, offsetY, OffsetY)
	PROPERTY_VARIABLE (float, opacity, Opacity)
	PROPERTY_VARIABLE (float, scale, Scale)
	PROPERTY_VARIABLE (float, rotation, Rotation)
	PROPERTY_OBJECT (Brush, brush, Brush)
	PROPERTY_OBJECT (Pen, borderPen, BorderPen)
	PROPERTY_VARIABLE (float, borderLeftTopRadius, BorderLeftTopRadius)
	PROPERTY_VARIABLE (float, borderLeftBottomRadius, BorderLeftBottomRadius)
	PROPERTY_VARIABLE (float, borderRightTopRadius, BorderRightTopRadius)
	PROPERTY_VARIABLE (float, borderRightBottomRadius, BorderRightBottomRadius)

	void setBorderRadius (float radius);

	// Text
	PROPERTY_OBJECT (Font, textFont, TextFont)
	PROPERTY_OBJECT (TextFormat, textFormat, TextFormat)
	PROPERTY_VARIABLE (Font::TrimMode, textTrimMode, TextTrimMode)
	PROPERTY_VARIABLE (float, minFontSize, MinFontSize)
	PROPERTY_VARIABLE (float, maxFontSize, MaxFontSize)
	void setWordBreak (bool state);

	ITextModel* getTextModel () { return textModel; }
	void setTextModel (ITextModel* model);

	void updateTextSize (RectF& size);
	void updateTextLayout ();

	const ITextLayout* getTextLayout () const { return textLayout; }

	// Image
	Image* getImage () { return image; }
	const Image* getImage () const { return image; }
	void setImage (Image* image);
	int getImageFrame () const { return imageFrame; }
	void setImageFrame (int frame);
	bool canRecolorImage () const;
	PROPERTY_VARIABLE (ImageInterpolationMode, imageInterpolation, ImageInterpolation)

	// Options
	PROPERTY_FLAG (options, 1<<0, isHidden)
	PROPERTY_FLAG (options, 1<<1, isContentFit)
	PROPERTY_FLAG (options, 1<<2, isContentFill)

	PROPERTY_FLAG (options, 1<<3, isTextMultiline)
	PROPERTY_FLAG (options, 1<<4, isTextMarkup)

	PROPERTY_FLAG (options, 1<<5, isBrushBackdrop)

	// State flags
	PROPERTY_FLAG (options, 1<<20, needsTextLayoutRebuild)
	PROPERTY_FLAG (options, 1<<21, needsTextLayoutResize)
	PROPERTY_FLAG (options, 1<<22, needsTextLayoutUpdate)

	struct ColorContrastHelper
	{
		PROPERTY_VARIABLE (float, brightColorThreshold, BrightColorThreshold)
		PROPERTY_VARIABLE (Color, defaultColor, DefaultColor)
		PROPERTY_VARIABLE (Color, contrastColorBright, ContrastColorBright)
		PROPERTY_VARIABLE (Color, contrastColorDark, ContrastColorDark)
		PROPERTY_VARIABLE (Color, contrastColorTransparent, ContrastColorTransparent)
		PROPERTY_VARIABLE (Color, contrastColorAlphaBlend, ContrastColorAlphaBlend)

		ColorContrastHelper ()
		: brightColorThreshold (.35f),
		  defaultColor (Colors::kTransparentBlack),
		  contrastColorBright (Colors::kTransparentBlack),
		  contrastColorDark (Colors::kTransparentBlack),
		  contrastColorTransparent (Colors::kTransparentBlack),
		  contrastColorAlphaBlend (Colors::kTransparentBlack)
		{}

		Color getForegroundColor (ColorRef backgroundColor) const;
	};

	ColorContrastHelper& getColorContrastHelper ();
	bool hasDefinedContrast () const;
	bool applyContrastColor (ColorRef backgroundColor);

	Image* getStyleImage (const VisualStyle& style, IStyleConditionContext* context = nullptr) const;
	bool getStyleColor (Color& styleColor, const VisualStyle& style, StringID propertyId, IStyleConditionContext* context = nullptr) const;

	void updateFromVisualStyle (RenderAtomTransaction& transaction, const VisualStyle& style, StyleConditionContext& conditionContext);
	void finalizeVisualStyleUpdate (const RenderAtomTransaction& transaction); // needs to be called after all atom updates have been applied, but before draw

	void draw (IGraphics& graphics, const UpdateRgn& updateRgn);
	void invalidate ();

	// Object
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;

protected:
	RectF size;
	ITextModel* textModel;
	AutoPtr<ITextLayout> textLayout;
	SharedPtr<Image> image;
	int imageFrame; // imageFrame needed per atom because image is shared between atoms
	int options;
	String displayedText;
	ColorContrastHelper* contrastHelper;

	// Private flags
	PROPERTY_FLAG (options, 1<<25, initialized)
	PROPERTY_FLAG (options, 1<<26, wasAnimationRunning)

	IGraphicsPath* createOutlinePath (RectFRef size, CoordF inset = 0.f) const;
	StringRef getText ();
	void updateGradientSize ();
};

DECLARE_TYPELIB_ENTRY (RenderAtom)

//************************************************************************************************
// AtomName
//************************************************************************************************

namespace AtomName
{
	const CStringPtr kBackground = "background";
}

//************************************************************************************************
// AtomPropertyID
//************************************************************************************************

namespace AtomPropertyID
{
	const CStringPtr kOffsetX = "offsetx";
	const CStringPtr kOffsetY = "offsety";
	const CStringPtr kScale = "scale";
	const CStringPtr kRotation = "rotation";
	const CStringPtr kColor = "color";
	const CStringPtr kOpacity = "opacity";
	const CStringPtr kBorderColor = "bordercolor";
	const CStringPtr kBorderWidth = "borderwidth";
	const CStringPtr kBorderLeftTopRadius = "borderlefttopradius";
	const CStringPtr kBorderLeftBottomRadius = "borderleftbottomradius";
	const CStringPtr kBorderRightTopRadius = "borderrighttopradius";
	const CStringPtr kBorderRightBottomRadius = "borderrightbottomradius";
	const CStringPtr kContrastColorBright = "contrastcolorbright";
	const CStringPtr kContrastColorDark = "contrastcolordark";
	const CStringPtr kContrastColorTransparent = "contrastcolortransparent";
	const CStringPtr kContrastColorAlphaBlend = "contrastcoloralphablend";
	const CStringPtr kImage = "image";
	const CStringPtr kImageFrame = "imageframe";
}

} // namespace CCL

#endif // _ccl_renderatom_h
