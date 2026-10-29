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
// Filename    : ccl/gui/theme/renderer/bivrenderer.cpp
// Description : Background-Icon-Value (BIV) Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/bivrenderer.h"

#include "ccl/gui/views/view.h"
#include "ccl/gui/windows/window.h"

using namespace CCL;

//************************************************************************************************
// BIVRenderer
//************************************************************************************************

DEFINE_TYPELIB_ENTRY (BIV, "BIVStyle", VISUALSTYLES_TYPELIB)
BEGIN_TYPELIB_MEMBERS (BIV)
	ADD_TYPELIB_MEMBER_RENDERATOM (AtomName::kBackground)
	ADD_TYPELIB_MEMBER_RENDERATOM (AtomName::kIcon)
	ADD_TYPELIB_MEMBER_RENDERATOM (AtomName::kValue)
	ADD_TYPELIB_MEMBER_METRIC (StyleID::BIV::kIconValueSpacing) ///< distance between the icon and the value
	ADD_TYPELIB_MEMBER_METRIC (StyleID::BIV::kIconFillFactor) ///< size of the icon relative to the content area, aspect ratio is kept - more than 1 reaches beyond that area
	ADD_TYPELIB_MEMBER_METRIC (StyleID::BIV::kIconLeading) ///< icon is drawn before the value instead of after it
	ADD_TYPELIB_MEMBER_METRIC (StyleID::BIV::kIconOverlay) ///< the icon overlays the value instead of taking away space from it
	ADD_TYPELIB_MEMBER_ALIGN  (StyleID::BIV::kIconAlignment) ///< vertical alignment of the icon within the content area (default: vcenter) - "left" or "right" additionally align an icon that has no value next to it
	ADD_TYPELIB_MEMBER_ALIGN  (StyleID::BIV::kValueAlignment) ///< vertical alignment of the value within the content area, and horizontal alignment of the whole content (default: "textalign")
	ADD_TYPELIB_MEMBER_STRING (StyleID::kPadding) ///< combined padding (left, right, top, bottom) 
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kPaddingLeft) ///< left padding
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kPaddingTop) ///< top padding
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kPaddingRight) ///< right padding
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kPaddingBottom) ///< bottom padding
END_TYPELIB_MEMBERS (BIV)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (BIVRenderer, ControlRenderer)

//////////////////////////////////////////////////////////////////////////////////////////////////

BIVRenderer::BIVRenderer (VisualStyle* visualStyle, View* owner)
: ControlRenderer (visualStyle, owner),
  iconAlignment (Alignment::kCenter),
  valueAlignment (Alignment::kLeftCenter),
  backgroundAtom (AtomName::kBackground, kBackgroundAtom),
  iconAtom (AtomName::kIcon, kIconAtom),
  valueAtom (AtomName::kValue, kValueAtom),
  observedTextModel (nullptr),
  order (kLeadingIcon),
  iconValueSpacing (0),
  iconFillFactor (0.f),
  iconOverlay (false),
  contentScaleFactor (1.f)
{
	addAtom (backgroundAtom);
	addAtom (iconAtom);
	addAtom (valueAtom);

	backgroundAtom.isContentFill (true);

	if(Window* window = owner->getWindow ())
		contentScaleFactor = window->getContentScaleFactor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BIVRenderer::~BIVRenderer ()
{
	if(observedTextModel)
		share_and_observe_unknown<ITextModel> (this, observedTextModel, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ITextLayout* BIVRenderer::getTextLayout () const
{
	return valueAtom.getTextLayout ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BIVRenderer::updateTextModel (ITextModel* textModel)
{
	if(observedTextModel == textModel)
		return false;

	share_and_observe_unknown<ITextModel> (this, observedTextModel, textModel);
	valueAtom.setTextModel (textModel);
	valueAtom.needsTextLayoutRebuild (true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BIVRenderer::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && observedTextModel && isEqualUnknown (subject, observedTextModel))
	{
		valueAtom.needsTextLayoutRebuild (true);
		owner->invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/ void BIVRenderer::alignVertically (RectF& rect, RectFRef contentRect, AlignmentRef alignment)
{
	CoordF top = contentRect.top + (contentRect.getHeight () - rect.getHeight ()) / 2.f;

	int alignV = alignment.getAlignV ();
	if(alignV == Alignment::kTop)
		top = contentRect.top;
	else if(alignV == Alignment::kBottom)
		top = contentRect.bottom - rect.getHeight ();

	rect.moveTo (PointF (rect.left, top));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BIVRenderer::snapToPixelGrid (RectF& rect, RectFRef area) const
{
	CoordF left = area.left + CoordF(int((rect.left - area.left) * contentScaleFactor)) / contentScaleFactor;
	CoordF top  = area.top + CoordF(int((rect.top - area.top) * contentScaleFactor)) / contentScaleFactor;
	rect.moveTo (PointF (left, top));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BIVRenderer::setContentScaleFactor (float factor)
{
	contentScaleFactor = factor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BIVRenderer::updateAtoms ()
{
	SuperClass::updateAtoms ();

	Rect totalSizeInt;
	owner->getClientRect (totalSizeInt);
	RectF totalSize (rectIntToF (totalSizeInt));
	RectF foregroundSize (totalSize);
	foregroundSize.left   += padding.left;
	foregroundSize.right  -= padding.right;
	foregroundSize.top    += padding.top;
	foregroundSize.bottom -= padding.bottom;

	RectF contentRect (foregroundSize);

	RectF iconRect;
	RectF valueRect;
	bool useMinSize = totalSize.isEmpty ();
	if(useMinSize && !valueAtom.isContentFit () && !valueAtom.isContentFill () && !valueAtom.isTextMultiline ())
		valueAtom.setSize (RectF (0.f, 0.f, kMaxCoord, 1));
	else
		valueAtom.setSize (contentRect);

	valueAtom.updateTextSize (valueRect);

	// In the minimum size pass the content area is empty, so the icon comes out at its own size here
	getImageSize (iconRect, iconAtom.getImage (), foregroundSize, iconFillFactor);

	// An overlay icon is drawn on top of the value, so it neither takes away space from it nor keeps
	// a distance to it. Without a value there is nothing to overlay, so the icon is placed as usual.
	bool overlayIcon = iconOverlay && !valueRect.isEmpty ();
	CoordF iconWidth = overlayIcon ? 0.f : iconRect.getWidth ();
	Coord currentIVSpacing = (!iconRect.isEmpty () && !overlayIcon) ? iconValueSpacing : 0;

	if(useMinSize)
	{
		// The content area the icon is measured against does not exist yet, so establish it first -
		// with the icon contributing its own size to it like any other content. Apply iconFillFactor afterwards.
		CoordF contentHeight = ccl_max (valueRect.getHeight (), iconRect.getHeight ());
		if(iconFillFactor > 0.f)
		{
			getImageSize (iconRect, iconAtom.getImage (), RectF (0.f, 0.f, PointF (kMaxCoord, contentHeight)), iconFillFactor);
			iconWidth = overlayIcon ? 0.f : iconRect.getWidth ();
		}

		contentRect = RectF (padding.left, padding.top,	PointF (valueRect.getWidth () + currentIVSpacing + iconWidth, contentHeight));
	}

	contentRect.setWidth (valueRect.getWidth () + iconWidth);
	if(!valueRect.isEmpty () && !iconRect.isEmpty ())
		contentRect.setWidth (contentRect.getWidth () + currentIVSpacing);

	if(useMinSize)
	{
		foregroundSize = contentRect;
		totalSize = foregroundSize;
		totalSize.left -= padding.left;
		totalSize.top -= padding.top;
		totalSize.right += padding.right;
		totalSize.bottom += padding.bottom;
	}

	// The content is aligned horizontally as a whole, icon and value are aligned vertically within it.
	// Without a value the icon is the whole content, so a left or right icon alignment aligns it then.
	int alignH = valueAlignment.getAlignH ();
	if(valueRect.isEmpty () && iconAlignment.getAlignH () != Alignment::kHCenter)
		alignH = iconAlignment.getAlignH ();

	if(alignH == Alignment::kHCenter)
		contentRect.moveTo (PointF (padding.left + (foregroundSize.getWidth () - contentRect.getWidth ()) / 2.f, contentRect.top));
	else if(alignH == Alignment::kRight)
		contentRect.moveTo (PointF (padding.left + foregroundSize.getWidth () - contentRect.getWidth (), contentRect.top));

	alignVertically (valueRect, contentRect, valueAlignment);
	alignVertically (iconRect, contentRect, iconAlignment);

	if(order == kLeadingIcon)
	{
		iconRect.moveTo (PointF (contentRect.left, iconRect.top));
		valueRect.moveTo (PointF (contentRect.left + iconWidth + currentIVSpacing, valueRect.top));
	}
	else if(order == kTrailingIcon)
	{
		iconRect.moveTo (PointF (contentRect.right - iconRect.getWidth (), iconRect.top));
		valueRect.moveTo (PointF (contentRect.right - iconWidth - currentIVSpacing - valueRect.getWidth (), valueRect.top));
	}

	// the alignment above can leave the icon between two pixels, which makes it blurry
	snapToPixelGrid (iconRect, foregroundSize);

	iconAtom.setSize (iconRect);
	valueAtom.setSize (valueRect);
	backgroundAtom.setSize (totalSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BIVRenderer::applyVisualStyle (RenderAtomTransaction& transaction)
{
	SuperClass::applyVisualStyle (transaction);

	if(Image* icon = getIcon ())
		iconAtom.setImage (icon); // SuperClass::applyVisualStyle resets the icon

	StyleRef style = owner->getStyle ();

	// Renderer properties
	visualStyle->getPadding (padding);

	// Atom properties
	backgroundAtom.isHidden (!style.isOpaque ());

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	// the defaults of the legacy renderer
	iconValueSpacing = 3;
	iconFillFactor = 0.f;
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	iconValueSpacing = visualStyle->getMetric<Coord> (StyleID::BIV::kIconValueSpacing, iconValueSpacing);
	iconFillFactor = visualStyle->getMetric<float> (StyleID::BIV::kIconFillFactor, iconFillFactor);

	#if LEGACY_STYLES_SUPPORTED
	// Legacy atom properties
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	applyLegacyVisualStyle ();
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	// the element order set by the derived renderer is the default here
	order = visualStyle->getMetric<bool> (StyleID::BIV::kIconLeading, order == kLeadingIcon) ? kLeadingIcon : kTrailingIcon;
	iconOverlay = visualStyle->getMetric<bool> (StyleID::BIV::kIconOverlay, false);

	iconAtom.isContentFit (iconFillFactor > 0.f); // the image is scaled to fit into the rect

	TextFormat textFormat (visualStyle->getTextFormat ());
	valueAtom.setTextFont (visualStyle->getTextFont ().zoom (owner->getZoomFactor ()));
	valueAtom.setTextFormat (textFormat);

	// "textalign" is the default for the value alignment - horizontally only, because the vertical
	// position has always been centered
	Alignment defaultValueAlignment (textFormat.getAlignment ().getAlignH () | Alignment::kVCenter);
	setValueAlignment (visualStyle->getOptions (StyleID::BIV::kValueAlignment, defaultValueAlignment.align));

	// the icon alignment set by the derived renderer is the default here
	setIconAlignment (visualStyle->getOptions (StyleID::BIV::kIconAlignment, iconAlignment.align));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#if LEGACY_STYLES_SUPPORTED
//////////////////////////////////////////////////////////////////////////////////////////////////

Brush BIVRenderer::getValueBrush () const
{
	static const LegacyStyleID::StateColorIDs kTextColors = {StyleID::kTextColor, LegacyStyleID::kTextColorOn,
		LegacyStyleID::kTextColorPressed, LegacyStyleID::kTextColorPressedOn, LegacyStyleID::kTextColorMouseover,
		LegacyStyleID::kTextColorMouseoverOn, LegacyStyleID::kTextColorDisabled, LegacyStyleID::kTextColorDisabledOn};

	Color valueColor (Colors::kBlack);
	if(getStateColor (valueColor, kTextColors) || !valueAtom.getBrush ().isVisible ())
		return SolidBrush (valueColor);

	return valueAtom.getBrush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Brush BIVRenderer::getIconBrush () const
{
	static const LegacyStyleID::StateColorIDs kIconColors = {LegacyStyleID::kIconColor, LegacyStyleID::kIconColorOn,
		LegacyStyleID::kIconColorPressed, LegacyStyleID::kIconColorPressedOn, LegacyStyleID::kIconColorMouseover,
		LegacyStyleID::kIconColorMouseoverOn, LegacyStyleID::kIconColorDisabled, LegacyStyleID::kIconColorDisabledOn};

	Color iconColor;
	if(getStateColor (iconColor, kIconColors))
		return SolidBrush (iconColor);

	if(iconAtom.getBrush ().isVisible ())
		return iconAtom.getBrush ();

	return getValueBrush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BIVRenderer::getStateColor (Color& color, const LegacyStyleID::StateColorIDs& ids) const
{
	int state = owner->getThemeElementState () % ThemeElements::kNumElementStates;
	bool isOn = conditionContext.matches (ThemeNames::kOn);

	// each defined color replaces the one looked up before it
	bool hasColor = false;
	auto lookup = [&] (CStringPtr name)
	{
		if(visualStyle->getColor (color, name))
			hasColor = true;
	};

	lookup (ids.normal);
	if(isOn)
		lookup (ids.on);

	if(state == ThemeElements::kPressed)
	{
		lookup (ids.pressed);
		if(isOn)
			lookup (ids.pressedOn);
	}
	else if(state == ThemeElements::kMouseOver)
	{
		if(isOn)
			lookup (ids.mouseoverOn);
		else
			lookup (ids.mouseover);
	}
	else if(state == ThemeElements::kDisabled)
	{
		lookup (ids.disabled);
		if(isOn)
			lookup (ids.disabledOn);
	}

	return hasColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Brush BIVRenderer::getBackgroundBrush () const
{
	Color backgroundColor (Colors::kTransparentBlack);
	bool isOn = conditionContext.matches (ThemeNames::kOn);
	if(!isOn && visualStyle->getColor (backgroundColor, LegacyStyleID::kBackcolorOff) && backgroundColor.isVisible ())
		return SolidBrush (backgroundColor);

	CStringPtr colorName = StyleID::kBackColor;
	if(isOn)
	{
		if(backgroundAtom.getImage ())
			colorName = LegacyStyleID::kBackcolorOn;
		else
			colorName = StyleID::kForeColor;
	}
	else if(backgroundAtom.getImage ())
		colorName = LegacyStyleID::kBackcolorTransparent;

	if(visualStyle->getColor (backgroundColor, colorName))
		return SolidBrush (backgroundColor);

	return backgroundAtom.getBrush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BIVRenderer::applyLegacyVisualStyle ()
{
	StyleRef style = owner->getStyle ();

	IImage* image = nullptr;
	if(style.isCommonStyle (Styles::kMiddle))
		image = visualStyle->getImage (LegacyStyleID::kMiddleImage);
	if(style.isCommonStyle (Styles::kLeft) && style.isCommonStyle (Styles::kRight))
		image = visualStyle->getImage (LegacyStyleID::kSingleImage);
	else if(style.isCommonStyle (Styles::kLeft))
		image = visualStyle->getImage (LegacyStyleID::kLeftImage);
	else if(style.isCommonStyle (Styles::kRight))
		image = visualStyle->getImage (LegacyStyleID::kRightImage);

	if(image == nullptr)
		image = visualStyle->getBackgroundImage ();

	if(image)
		backgroundAtom.setImage (unknown_cast<Image> (image));

	valueAtom.setBrush (getValueBrush ());

	bool isOn = conditionContext.matches (ThemeNames::kOn);

	RenderAtom::ColorContrastHelper& valueContrastHelper = valueAtom.getColorContrastHelper ();
	valueContrastHelper.setDefaultColor (valueAtom.getBrush ().getColor ());

	float brightColorThreshold = visualStyle->getMetric<float> (LegacyStyleID::kTextColorThreshold, valueContrastHelper.getBrightColorThreshold ());
	valueContrastHelper.setBrightColorThreshold (brightColorThreshold);
	static const LegacyStyleID::ContrastColorIDs kTextContrastColors = {LegacyStyleID::kTextColorBright, LegacyStyleID::kTextColorDark,
		LegacyStyleID::kTextColorTransparent, nullptr, LegacyStyleID::kTextColorAlphaBlend};
	static const LegacyStyleID::ContrastColorIDs kTextContrastColorsOn = {LegacyStyleID::kTextColorBrightOn, LegacyStyleID::kTextColorDarkOn,
		LegacyStyleID::kTextColorTransparentOn, nullptr, LegacyStyleID::kTextColorAlphaBlend};
	if(isOn)
		applyLegacyContrastColors (valueAtom, kTextContrastColorsOn);
	else
		applyLegacyContrastColors (valueAtom, kTextContrastColors);

	// "colorize.icon" used to colorize the icon in "iconcolor" no matter whether the image was marked as
	// a template. The ModifiedImageCache decides by that flag alone by now, so it is applied to the image here.
	if(visualStyle->getMetric<bool> (LegacyStyleID::kColorizeIcon, false))
	{
		Image* icon = iconAtom.getImage ();
		if(icon == nullptr)
			icon = unknown_cast<Image> (visualStyle->getImage (LegacyStyleID::kIcon)); // the atom gets the image later (see ButtonRenderer::applyVisualStyle)

		if(icon)
			icon->setIsTemplate (true);
	}

	RenderAtom::ColorContrastHelper& iconContrastHelper = iconAtom.getColorContrastHelper ();
	iconContrastHelper.setDefaultColor (getIconBrush ().getColor ());
	iconContrastHelper.setBrightColorThreshold (brightColorThreshold);
	static const LegacyStyleID::ContrastColorIDs kIconContrastColors = {LegacyStyleID::kIconColorBright, LegacyStyleID::kIconColorDark,
		LegacyStyleID::kIconColorTransparent, nullptr, LegacyStyleID::kIconColorAlphaBlend};
	static const LegacyStyleID::ContrastColorIDs kIconContrastColorsOn = {LegacyStyleID::kIconColorBrightOn, LegacyStyleID::kIconColorDarkOn,
		LegacyStyleID::kIconColorTransparentOn, nullptr, LegacyStyleID::kIconColorAlphaBlend};
	if(isOn)
		applyLegacyContrastColors (iconAtom, kIconContrastColorsOn);
	else
		applyLegacyContrastColors (iconAtom, kIconContrastColors);

	float backgroundRadius = 0.f;
	if(visualStyle->getMetric (backgroundRadius, LegacyStyleID::kBackgroundRadius))
		backgroundAtom.setBorderRadius (backgroundRadius);

	// "fill.icon" sizes the icon rather than the drawing of it, so it does not go into the atom zoom
	iconFillFactor = visualStyle->getMetric<float> (LegacyStyleID::kIconFillFactor, iconFillFactor);
	iconValueSpacing = visualStyle->getMetric<Coord> (LegacyStyleID::kSpacingIcon, iconValueSpacing);

	valueAtom.setMaxFontSize (visualStyle->getMetric<float> (LegacyStyleID::kScaleTextMaxFont, 100));
	valueAtom.setMinFontSize (visualStyle->getMetric<float> (LegacyStyleID::kScaleTextMinFont, 6));

	backgroundAtom.setBrush (getBackgroundBrush ());
}

#endif // LEGACY_STYLES_SUPPORTED
