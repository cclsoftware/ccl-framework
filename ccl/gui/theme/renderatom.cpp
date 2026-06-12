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
// Filename    : ccl/gui/theme/renderatom.cpp
// Description : Render Atom
//
//************************************************************************************************

#include "ccl/gui/theme/renderatom.h"
#include "ccl/gui/theme/textscaler.h"

#include "ccl/gui/graphics/imaging/imagecache.h"
#include "ccl/gui/graphics/imaging/multiimage.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/graphicspath.h"

using namespace CCL;

//************************************************************************************************
// RenderAtom
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (RenderAtom, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

RenderAtom::RenderAtom (CStringRef name, RectFRef size)
: name (name),
  size (size),
  brush (SolidBrush (Colors::kTransparentBlack)),
  alpha (1.f),
  options (0),
  textModel (nullptr),
  textLayout (nullptr),
  imageZoom (0.f),
  borderLeftTopRadius (0.f),
  borderLeftBottomRadius (0.f),
  borderRightTopRadius (0.f),
  borderRightBottomRadius (0.f),
  textTrimMode (Font::TrimMode::kTrimModeDefault),
  contrastHelper (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

RenderAtom::~RenderAtom ()
{
	if(contrastHelper)
		delete contrastHelper;

	safe_release (textLayout);
	if(textModel)
		share_and_observe_unknown<ITextModel> (this, textModel, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::setSize (RectFRef _size)
{
	size = _size;
	hasSizeChanged (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::setTextModel (ITextModel* model)
{
	share_and_observe_unknown<ITextModel> (this, textModel, model);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::draw (IGraphics& graphics, const UpdateRgn& updateRgn)
{
	ContextSaver saver (graphics);

	clipBorderRadius (graphics);

	if(image)
	{
		ImageMode mode (alpha, ImageMode::kInterpolationHighQuality);
		RectF src (0.f, 0.f, image->getWidth (), image->getHeight ());
		RectF dst (size);
		IImage* drawImage = image;
		if(isImageFit ())
		{
			int flags = 0;
			if(isImageFitAllowStretch ())
				flags = ImageResolutionSelector::kAllowStretch;
			else if(isImageFitAllowZoom ())
				flags = ImageResolutionSelector::kAllowZoom;
			
			Image* drawable = unknown_cast<Image> (drawImage);
			ImageResolutionSelector s (drawable, rectFToInt (size), flags, drawable->getCurrentFrame ());
			drawImage = s.bestImage;
			src = rectIntToF (s.srcRect);
			dst = rectIntToF (s.dstRect);
			
			PointF imageSize (src.getWidth (), src.getHeight ());
			PointF imagePos;

			if(imageZoom > 0.f)
			{
				float imageResize = imageZoom * ccl_min (dst.getHeight (), dst.getWidth ());
				float resizeRatio = (imageResize > 0.f) ? (imageResize / ccl_max (imageSize.x, imageSize.y)) : 1;
				imageSize *= resizeRatio;
				imagePos = dst.getLeftTop () + ((dst.getSize () - imageSize) * .5f);
			
				dst.setSize (imageSize);
				dst.moveTo (imagePos);
			}
		}
		else if(isImageCentered ())
		{
			PointF imageSize (drawImage->getWidth (), drawImage->getHeight ());
			dst.moveTo (dst.getLeftTop () + ((dst.getSize () - imageSize) * .5f));
			dst.setSize (imageSize);
		}

		if(brush.getColor ().getAlphaF () > 0.f)
			drawImage = ModifiedImageCache::instance ().lookup (image, brush.getColor (), true);

		graphics.drawImage (drawImage, src, dst, &mode);
	}
	else if(brush.getColor ().getAlphaF () > 0.f)
	{
		if(textModel)
		{
			if(hasTextChanged () || hasSizeChanged () || textLayout == nullptr)
			{
				Font drawFont (textFont);
				String displayText (getText ());
				if(isTextScale ())
					TextScaler ().scaleTextFont (drawFont, rectFToInt (size), displayText, isTextMarkup () ? TextScaler::kMarkupText : 0);

				safe_release (textLayout);
				textLayout = NativeGraphicsEngine::instance ().createTextLayout ();
				textLayout->construct (displayText, size.getWidth (), size.getHeight (), drawFont, isTextMultiline () ? ITextLayout::kMultiLine : ITextLayout::kSingleLine, textFormat);
				hasSizeChanged (false);
			}

			textModel->updateLayout (*textLayout);

			graphics.drawTextLayout (size.getLeftTop (), textLayout, brush);
		}
		else
			graphics.fillRect (size, brush);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::setBorderRadius (float radius)
{
	borderLeftTopRadius = radius;
	borderRightTopRadius = radius;
	borderLeftBottomRadius = radius;
	borderRightBottomRadius = radius;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RenderAtom::ColorContrastHelper& RenderAtom::getColorContrastHelper ()
{
	if(contrastHelper == nullptr)
		contrastHelper = NEW ColorContrastHelper ();

	return *contrastHelper;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::clipBorderRadius (IGraphics& graphics)
{
	if(borderLeftTopRadius > 0.f || borderLeftBottomRadius > 0.f || borderRightTopRadius > 0.f || borderRightBottomRadius > 0.f)
	{
		float maxRadius = ccl_min (size.getWidth (), size.getHeight ()) / 2.f;
		float leftTop = ccl_min (borderLeftTopRadius, maxRadius);
		float rightTop = ccl_min (borderRightTopRadius, maxRadius);
		float rightBottom = ccl_min (borderRightBottomRadius, maxRadius);
		float leftBottom = ccl_min (borderLeftBottomRadius, maxRadius);

		AutoPtr<IGraphicsPath> clipPath = NEW GraphicsPath (IGraphicsPath::kClipPath);
		if(leftTop > 0.f)
		{
			clipPath->startFigure (Point (size.left, size.top + leftTop));
			clipPath->addArc (RectF (size.getLeftTop (), PointF (leftTop * 2.f, leftTop * 2.f)), -180, -90);
		}
		else
			clipPath->startFigure (size.getLeftTop ());

		if(rightTop > 0.f)
		{
			clipPath->lineTo (PointF (size.right - rightTop, size.top));
			clipPath->addArc (RectF (size.right - (rightTop * 2.f), size.top, size.right, size.top + (rightTop * 2.f)), -90, 0);
		}
		else
			clipPath->lineTo (size.getRightTop ());

		if(rightBottom > 0.f)
		{
			clipPath->lineTo (PointF (size.right, size.bottom - rightBottom));
			clipPath->addArc (RectF (size.right - (rightBottom * 2.f), size.bottom - (rightBottom * 2.f), size.right, size.bottom), 0, 90);
		}
		else
			clipPath->lineTo (size.getRightBottom ());

		if(leftBottom > 0.f)
		{
			clipPath->lineTo (PointF (size.left + leftBottom, size.bottom));
			clipPath->addArc (RectF (size.left, size.bottom - (leftBottom * 2.f), size.left + (leftBottom * 2.f), size.bottom), 90, 180);
		}
		else
			clipPath->lineTo (size.getLeftBottom ());

		clipPath->closeFigure ();
		graphics.addClip (clipPath);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef RenderAtom::getText ()
{
	if(hasTextChanged ())
	{
		ASSERT (textModel)
		if(textModel)
			textModel->toDisplayString (displayedText);

		if(!isTextMultiline () && isTextFit ())
		{
			Coord textSpace = isTextVertical () ? size.getHeight () : size.getWidth ();
			if(isTextScale ())
			{
				static constexpr float kFitTextFactorAfterScaling = 1.16f;
				textSpace = ccl_to_int (textSpace * kFitTextFactorAfterScaling);
			}

			if(textSpace > 0)
				Font::collapseString (displayedText, textSpace, textFont, textTrimMode);
		}

		hasTextChanged (false);
	}

	return displayedText;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API RenderAtom::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && isEqualUnknown (subject, textModel))
		hasTextChanged (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::setStyleCondition (CStringRef condition, bool state)
{
	conditionContext.setCondition (condition, state);
}

//************************************************************************************************
// RenderAtom::ColorContrastHelper
//************************************************************************************************

Color RenderAtom::ColorContrastHelper::getForegroundColor (float brightColorThreshold, const Color* customBackcolor) const
{
	Color result = getDefaultColor ();
	if(customBackcolor)
	{
		if(customBackcolor->getAlphaF () == 0.f)
			return getContrastTransparentColor ();
		else if(getContrastBrightColor () != Colors::kTransparentBlack)
		{
			if(customBackcolor->getLuminance () < brightColorThreshold)
				return getContrastBrightColor ();
			else
				return getContrastDarkColor ();
		}
		else if(result.getAlphaF () != 0)
			result.renderAlpha (*customBackcolor);
	}

	return result;
}
