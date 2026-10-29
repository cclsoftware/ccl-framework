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
#include "ccl/gui/graphics/graphicshelper.h"
#include "ccl/gui/graphics/colorgradient.h"

#include "ccl/gui/system/animation.h"

#include "ccl/base/message.h"

#include "ccl/public/math/mathprimitives.h"

#include "core/public/corestringbuffer.h"

namespace CCL {

//************************************************************************************************
// AtomPropertyName
/** The name a property of an atom has within a visual style, e.g. "background.image". */
//************************************************************************************************

class AtomPropertyName: public Core::CString128
{
public:
	AtomPropertyName (CStringPtr atomName, CStringPtr propertyName)
	{
		append (atomName);
		append ('.');
		append (propertyName);
	}

	operator CString () const { return CString (buffer); }
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// RenderAtom
//************************************************************************************************

DEFINE_TYPELIB_ENTRY (RenderAtom, TypeNames::kRenderAtom, VISUALSTYLES_TYPELIB)
BEGIN_TYPELIB_MEMBERS (RenderAtom)
	ADD_TYPELIB_MEMBER_METRIC (AtomPropertyID::kOffsetX)
	ADD_TYPELIB_MEMBER_METRIC (AtomPropertyID::kOffsetY)
	ADD_TYPELIB_MEMBER_METRIC (AtomPropertyID::kScale)
	ADD_TYPELIB_MEMBER_METRIC (AtomPropertyID::kRotation)
	ADD_TYPELIB_MEMBER_COLOR (AtomPropertyID::kColor)
	ADD_TYPELIB_MEMBER_METRIC (AtomPropertyID::kOpacity)
	ADD_TYPELIB_MEMBER_COLOR (AtomPropertyID::kBorderColor)
	ADD_TYPELIB_MEMBER_METRIC (AtomPropertyID::kBorderWidth)
	ADD_TYPELIB_MEMBER_METRIC (AtomPropertyID::kBorderLeftTopRadius)
	ADD_TYPELIB_MEMBER_METRIC (AtomPropertyID::kBorderLeftBottomRadius)
	ADD_TYPELIB_MEMBER_METRIC (AtomPropertyID::kBorderRightTopRadius)
	ADD_TYPELIB_MEMBER_METRIC (AtomPropertyID::kBorderRightBottomRadius)
	ADD_TYPELIB_MEMBER_COLOR (AtomPropertyID::kContrastColorBright)
	ADD_TYPELIB_MEMBER_COLOR (AtomPropertyID::kContrastColorDark)
	ADD_TYPELIB_MEMBER_COLOR (AtomPropertyID::kContrastColorTransparent)
	ADD_TYPELIB_MEMBER_COLOR (AtomPropertyID::kContrastColorAlphaBlend)
	ADD_TYPELIB_MEMBER_IMAGE (AtomPropertyID::kImage)
	ADD_TYPELIB_MEMBER_METRIC (AtomPropertyID::kImageFrame)
END_TYPELIB_MEMBERS (RenderAtom)

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString RenderAtom::makeId (StringID atomName, StringID propertyId)
{
	return MutableCString (AtomPropertyName (atomName, propertyId).str ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT (RenderAtom, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

RenderAtom::RenderAtom (StringID name, int tag)
: name (name),
  tag (tag),
  brush (SolidBrush (Colors::kTransparentBlack)),
  borderPen (Colors::kTransparentBlack, 0.f),
  opacity (1.f),
  options (0),
  textModel (nullptr),
  minFontSize (-1.f),
  maxFontSize (-1.f),
  offsetX (0.f),
  offsetY (0.f),
  scale (0.f),
  rotation (0.f),
  imageFrame (0),
  imageInterpolation (kImageInterpolationDefault),
  borderLeftTopRadius (0.f),
  borderLeftBottomRadius (0.f),
  borderRightTopRadius (0.f),
  borderRightBottomRadius (0.f),
  textTrimMode (Font::TrimMode::kTrimModeDefault),
  contrastHelper (nullptr),
  client (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

RenderAtom::~RenderAtom ()
{
	if(wasAnimationRunning ())
		AnimationManager::instance ().removeTarget (this);

	delete contrastHelper;
	textLayout.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::setSize (RectFRef _size)
{
	if(size == _size)
		return;

	size = _size;
	needsTextLayoutResize (true);
	updateGradientSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::setWordBreak (bool state)
{
	if(textFormat.isWordBreak () == state)
		return;

	textFormat.isWordBreak (state);
	needsTextLayoutRebuild (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::setTextModel (ITextModel* model)
{
	textModel = model;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::updateTextSize (RectF& _size)
{
	_size.setEmpty ();
	if(textModel == nullptr)
		return;

	updateTextLayout ();
	if(textLayout)
		textLayout->getBounds (_size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::setImage (Image* _image)
{
	image = _image;
	imageFrame = image ? image->getCurrentFrame () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::setImageFrame (int frame)
{
	imageFrame = frame;
	if(image)
		image->setCurrentFrame (frame);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RenderAtom::canRecolorImage () const
{
	return image && (image->getIsTemplate () || image->getIsAdaptive ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::updateTextLayout ()
{
	if(textModel == nullptr || size.isEmpty () || (textLayout && !needsTextLayoutResize () && !needsTextLayoutRebuild () && !needsTextLayoutUpdate ()))
		return;

	CoordF layoutWidth = size.getWidth ();
	CoordF layoutHeight = size.getHeight ();

	if(textLayout && needsTextLayoutResize () && !needsTextLayoutRebuild ())
	{
		if((!isTextMultiline () && isContentFit ()) || textLayout->resize (layoutWidth, layoutHeight) != kResultOk)
			textLayout.release ();

		needsTextLayoutResize (false);
	}

	if(textLayout == nullptr)
		needsTextLayoutRebuild (true);

	if(needsTextLayoutRebuild ())
	{
		Font layoutFont (textFont);
		StringRef displayText = getText ();
		if(displayText.isEmpty ())
			textLayout.release ();
		else
		{
			if(isContentFill ())
			{
				TextScaler scaler;
				if(minFontSize > 0.f)
					scaler.setExplicitMinimalFontSize (minFontSize);
				if(maxFontSize > 0.f && maxFontSize >= minFontSize)
					scaler.setExplicitMaximalFontSize (maxFontSize);

				scaler.scaleTextFont (layoutFont, rectFToInt (RectF (0.f, 0.f, layoutWidth, layoutHeight)), displayText, isTextMarkup () ? TextScaler::kMarkupText : 0);
			}

			textLayout.release ();
			textLayout = NativeGraphicsEngine::instance ().createTextLayout ();
			ITextLayout::LineMode lineMode = isTextMultiline () ? ITextLayout::kMultiLine : ITextLayout::kSingleLine;
			textLayout->construct (displayText, layoutWidth, layoutHeight, layoutFont, lineMode, textFormat);
		}
		needsTextLayoutResize (false);
	}

	if(textLayout)
		textModel->updateLayout (*textLayout);

	needsTextLayoutUpdate (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::draw (IGraphics& graphics, const UpdateRgn& updateRgn)
{
	if(isHidden ())
		return;

	updateTextLayout ();

	RectF contentRect (size);
	ContextSaver saver (graphics);

	if(scale > 0.f && scale != 1.f)
	{
		Transform t;
		PointF center (contentRect.getCenter ());
		t.translate (center.x, center.y);   // 3) move to original position
		t.scale (scale, scale);             // 2) scale
		t.translate (-center.x, -center.y); // 1) move to origin
		graphics.addTransform (t);
	}

	if(offsetX != 0.f || offsetY != 0.f)
	{
		Transform t;
		t.translate (offsetX, offsetY);
		graphics.addTransform (t);
	}

	if(rotation != 0.f)
	{
		Transform t;
		PointF center (contentRect.getCenter ());
		t.translate (center.x, center.y);
		t.rotate (Math::degreesToRad (rotation));
		t.translate (-center.x, -center.y);
		graphics.addTransform (t);
	}

	bool needsClip = borderLeftTopRadius > 0.f || borderRightTopRadius > 0.f || borderRightBottomRadius > 0.f || borderLeftBottomRadius > 0.f;
	if(needsClip)
	{
		graphics.saveState (); // save state before clip to restore it before drawing the outline
		CoordF clipInset = borderPen.isVisible () ? borderPen.getWidth () / 4.f : 0.f; // inset a bit to avoid drawing past the outline
		if(AutoPtr<IGraphicsPath> clipPath = createOutlinePath (contentRect, clipInset))
			graphics.addClip (clipPath);
	}

	if(image)
	{
		if(isBrushBackdrop () && brush.isVisible ())
			graphics.fillRect (contentRect, brush);

		Image* resolvedImage = image;
		int frameToDraw = imageFrame;

		// A MultiImage is used for two purposes:
		//   - frames of equal size represent states: imageFrame selects the frame
		//   - frames of different sizes are resolutions of the same image (icon set): ImageResolutionSelector
		//     picks the best fitting one below, and imageFrame applies to that (it may be a filmstrip itself)
		MultiImage* multiImage = ccl_cast<MultiImage> (resolvedImage);
		if(multiImage && multiImage->getFrameCount () > 0 && multiImage->hasUniformFrameSize ())
		{
			if(Image* frame = multiImage->getFrame (ccl_bound<int> (imageFrame, 0, multiImage->getFrameCount () - 1)))
			{
				resolvedImage = frame;
				frameToDraw = frame->getCurrentFrame (); // imageFrame was consumed by the state selection
			}
		}

		RectF src (resolvedImage->getSizeF ());
		RectF dst (contentRect);

		// An image can fill the atom area in different ways depending on isContentFit and isContentFill:
		//   fit + fill - cover the area, preserving the aspect ratio: the image gets cropped
		//   fit        - fit into the area, preserving the aspect ratio: the area is not fully covered
		//   fill       - fill the area, distorting the image
		//   none       - center the image in the content area, keeping its original dimensions
		bool preserveAspectRatio = isContentFit ();

		ImageResolutionSelector::ScaleMode scaleMode = ImageResolutionSelector::kFitContain;
		if(preserveAspectRatio && isContentFill ())
			scaleMode = ImageResolutionSelector::kFitCover;

		if(src.getWidth () <= 0.f || src.getHeight () <= 0.f)
			src.setSize (dst.getSize ());
		else
		{
			ImageResolutionSelector s (resolvedImage, rectFToInt (contentRect), scaleMode, frameToDraw);
			resolvedImage = s.bestImage;
			src = rectIntToF (s.srcRect);
			if(preserveAspectRatio)
				dst = rectIntToF (s.dstRect);
		}

		if(!isContentFit () && !isContentFill ())
		{
			PointF imageSize (resolvedImage->getSizeF ());
			dst.moveTo (dst.getLeftTop () + ((dst.getSize () - imageSize) * .5f));
			dst.setSize (imageSize);
		}

		if(!isBrushBackdrop ())
			resolvedImage = unknown_cast<Image> (ModifiedImageCache::instance ().lookup (resolvedImage, brush.getColor ()));

		ImageMode mode (opacity, imageInterpolation);
		graphics.drawImage (resolvedImage, src, dst, &mode);
	}
	else if(brush.isVisible ())
	{
		float oldOpacity = -1.f;
		if(opacity < 1.f)
		{
			oldOpacity = GraphicsHelper::instance ().getBrushOpacity (brush);
			GraphicsHelper::instance ().setBrushOpacity (brush, opacity, GraphicsHelper::OpacityMode::kMultiply);
		}

		if(textModel)
		{
			if(textLayout)
			{
				UnknownPtr<ITextBackgroundRenderer> backgroundRenderer (textModel);
				if(backgroundRenderer.isValid ())
					backgroundRenderer->drawTextBackground (graphics, *textLayout, contentRect);

				graphics.drawTextLayout (contentRect.getLeftTop (), textLayout, brush);
			}
		}
		else
		{
			graphics.fillRect (contentRect, brush);
		}

		if(oldOpacity >= 0.f)
			GraphicsHelper::instance ().setBrushOpacity (brush, oldOpacity);
	}

	if(needsClip)
		graphics.restoreState ();

	if(borderPen.isVisible ())
	{
		float oldOpacity = borderPen.getOpacity ();
		borderPen.setOpacity (oldOpacity * opacity);

		if(AutoPtr<IGraphicsPath> outlinePath = createOutlinePath (contentRect, borderPen.getWidth () / 2.f))
			graphics.drawPath (outlinePath, borderPen);

		borderPen.setOpacity (oldOpacity);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::invalidate ()
{
	if(client)
		client->invalidate (*this);
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
	{
		contrastHelper = NEW ColorContrastHelper ();
		contrastHelper->setDefaultColor (getBrush ().getColor ());
	}

	return *contrastHelper;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RenderAtom::hasDefinedContrast () const
{
	if(contrastHelper == nullptr)
		return false;

	return contrastHelper->getContrastColorBright ().isVisible () || contrastHelper->getContrastColorAlphaBlend ().isVisible (); // matches ColorContrastHelper::getForegroundColor ()
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RenderAtom::applyContrastColor (ColorRef backgroundColor)
{
	if(contrastHelper == nullptr)
		return false;

	setBrush (SolidBrush (contrastHelper->getForegroundColor (backgroundColor)));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsPath* RenderAtom::createOutlinePath (RectFRef size, CoordF inset) const
{
	float leftTop = borderLeftTopRadius - inset;
	float rightTop = borderRightTopRadius - inset;
	float rightBottom = borderRightBottomRadius - inset;
	float leftBottom = borderLeftBottomRadius - inset;

	RectF outlineSize (size);
	outlineSize.contract (inset);

	IGraphicsPath* outlinePath = NEW GraphicsPath (IGraphicsPath::kClipPath);
	if(leftTop > 0.f || rightTop > 0.f || rightBottom > 0.f || leftBottom > 0.f)
	{
		float height = outlineSize.getHeight ();
		float width = outlineSize.getWidth ();

		float leftTopRatio = 1.f;
		float rightTopRatio = 1.f;
		float leftBottomRatio = 1.f;
		float rightBottomRatio = 1.f;

		if(leftTop + leftBottom > height)
		{
			float ratio = height / (leftTop + leftBottom);
			leftTopRatio = ccl_min (leftTopRatio, ratio);
			leftBottomRatio = ccl_min (leftBottomRatio, ratio);
		}
		if(leftTop + rightTop > width)
		{
			float ratio = width / (leftTop + rightTop);
			leftTopRatio = ccl_min (leftTopRatio, ratio);
			rightTopRatio = ccl_min (rightTopRatio, ratio);
		}
		if(rightTop + rightBottom > height)
		{
			float ratio = height / (rightTop + rightBottom);
			rightTopRatio = ccl_min (rightTopRatio, ratio);
			rightBottomRatio = ccl_min (rightBottomRatio, ratio);
		}
		if(rightBottom + leftBottom > width)
		{
			float ratio = width / (rightBottom + leftBottom);
			rightBottomRatio = ccl_min (rightBottomRatio, ratio);
			leftBottomRatio = ccl_min (leftBottomRatio, ratio);
		}

		leftTop *= leftTopRatio;
		rightTop *= rightTopRatio;
		leftBottom *= leftBottomRatio;
		rightBottom *= rightBottomRatio;

		if(leftTop > 0.f)
		{
			outlinePath->startFigure (Point (outlineSize.left, outlineSize.top + leftTop));
			outlinePath->addArc (RectF (outlineSize.left, outlineSize.top, PointF (leftTop * 2.f, leftTop * 2.f)), -180, 90);
		}
		else
			outlinePath->startFigure (outlineSize.getLeftTop ());

		if(rightTop > 0.f)
		{
			outlinePath->lineTo (PointF (outlineSize.right - rightTop, outlineSize.top));
			outlinePath->addArc (RectF (outlineSize.right - (rightTop * 2.f), outlineSize.top, outlineSize.right, outlineSize.top + (rightTop * 2.f)), -90, 90);
		}
		else
			outlinePath->lineTo (outlineSize.getRightTop ());

		if(rightBottom > 0.f)
		{
			outlinePath->lineTo (PointF (outlineSize.right, outlineSize.bottom - rightBottom));
			outlinePath->addArc (RectF (outlineSize.right - (rightBottom * 2.f), outlineSize.bottom - (rightBottom * 2.f), outlineSize.right, outlineSize.bottom), 0, 90);
		}
		else
			outlinePath->lineTo (outlineSize.getRightBottom ());

		if(leftBottom > 0.f)
		{
			outlinePath->lineTo (PointF (outlineSize.left + leftBottom, outlineSize.bottom));
			outlinePath->addArc (RectF (outlineSize.left, outlineSize.bottom - (leftBottom * 2.f), outlineSize.left + (leftBottom * 2.f), outlineSize.bottom), 90, 90);
		}
		else
			outlinePath->lineTo (outlineSize.getLeftBottom ());

		outlinePath->closeFigure ();
	}
	else
	{
		outlinePath->startFigure (outlineSize.getLeftTop ());
		outlinePath->lineTo (outlineSize.getRightTop ());
		outlinePath->lineTo (outlineSize.getRightBottom ());
		outlinePath->lineTo (outlineSize.getLeftBottom ());
		outlinePath->closeFigure ();
	}

	return outlinePath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef RenderAtom::getText ()
{
	if(needsTextLayoutRebuild ())
	{
		ASSERT (textModel)
		if(textModel)
			textModel->toDisplayString (displayedText);

		if(!isTextMultiline () && isContentFit ())
		{
			Coord textSpace = size.getWidth ();
			if(isContentFill ())
			{
				static constexpr float kFitTextFactorAfterScaling = 1.16f;
				textSpace = ccl_to_int (textSpace * kFitTextFactorAfterScaling);
			}

			if(textSpace > 0)
				Font::collapseString (displayedText, textSpace, textFont, textTrimMode);
		}

		needsTextLayoutRebuild (false);
	}

	return displayedText;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::updateGradientSize ()
{
	// Adjust gradients to fill the entire area, reacting to isContentFit ()/isContentFill () like images do (see draw ()); a radius can't be
	// distorted into an ellipse, so isContentFill () alone covers the area just like fit + fill. draw () rotates the whole result in place,
	// so this stays in size's own (unrotated) coordinates.
	if(brush.getType () == Brush::kGradient)
	{
		// TODO: add angle attribute to <LinearGradient> to allow setting rotated gradients
		if(LinearColorGradient* linearGradient = unknown_cast<LinearColorGradient> (brush.getGradient ()))
		{
			if(isContentFit () || isContentFill ())
				linearGradient->setSize (size.getLeftTop (), size.getRightTop ());
		}
		else if(RadialColorGradient* radialGradient = unknown_cast<RadialColorGradient> (brush.getGradient ()))
		{
			if(isContentFill ())
				radialGradient->setSize (size.getCenter (), ccl_max (size.getWidth (), size.getHeight ()) / 2);
			else if(isContentFit ())
				radialGradient->setSize (size.getCenter (), ccl_min (size.getWidth (), size.getHeight ()) / 2);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* RenderAtom::getStyleImage (const VisualStyle& style, IStyleConditionContext* context) const
{
	return unknown_cast<Image> (style.getImage (AtomPropertyName (name, AtomPropertyID::kImage), context));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RenderAtom::getStyleColor (Color& styleColor, const VisualStyle& style, StringID propertyId, IStyleConditionContext* context) const
{
	return style.getColor (styleColor, AtomPropertyName (name, propertyId), context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::updateFromVisualStyle (RenderAtomTransaction& transaction, const VisualStyle& style, StyleConditionContext& conditionContext)
{
	IStyleConditionContext* icontext = &conditionContext;

	updateGradientSize ();

	auto addAnimation = [&transaction, &style, icontext, this] (CStringPtr property)
	{
		IAnimation* animation = style.getAnimation (AtomPropertyName (name, property), icontext);
		if(animation == nullptr)
			animation = style.getAnimation (CString::kEmpty, icontext); // animations without name apply to all properties

		if(animation)
		{
			Variant startValue;
			if(getProperty (startValue, property))
			{
				RenderAtomTransaction::PendingAnimation pending (this, property, animation);
				pending.startValue = startValue;
				transaction.animations.add (pending);
				return true;
			}
		}

		return false;
	};

	if(initialized ())
	{
		addAnimation (AtomPropertyID::kOffsetX);
		addAnimation (AtomPropertyID::kOffsetY);
		addAnimation (AtomPropertyID::kColor);
		addAnimation (AtomPropertyID::kBorderColor);
		addAnimation (AtomPropertyID::kBorderWidth);
		addAnimation (AtomPropertyID::kOpacity);
		addAnimation (AtomPropertyID::kBorderLeftTopRadius);
		addAnimation (AtomPropertyID::kBorderLeftBottomRadius);
		addAnimation (AtomPropertyID::kBorderRightTopRadius);
		addAnimation (AtomPropertyID::kBorderRightBottomRadius);
		addAnimation (AtomPropertyID::kScale);
		addAnimation (AtomPropertyID::kRotation);
		addAnimation (AtomPropertyID::kImageFrame);
	}

	auto getStyleBrush = [this, icontext] (const VisualStyle& style, CStringPtr propertyName) -> Brush
	{
		AtomPropertyName atomPropertyName (name, propertyName);
		if(IGradient* gradient = style.getGradient (atomPropertyName, icontext))
			return GradientBrush (gradient);
		else
		{
			Color color = Colors::kTransparentBlack;
			style.getColor (color, atomPropertyName, icontext);
			return SolidBrush (color);
		}
	};

	auto getStyleColor = [this, icontext] (const VisualStyle& style, CStringPtr propertyName)
	{
		Color result = Colors::kTransparentBlack;
		style.getColor (result, AtomPropertyName (name, propertyName), icontext);
		return result;
	};

	setBrush (getStyleBrush (style, AtomPropertyID::kColor));

	Color color = getStyleColor (style, AtomPropertyID::kContrastColorBright);
	if(contrastHelper || color.isVisible ())
		getColorContrastHelper ().setContrastColorBright (color);

	color = getStyleColor (style, AtomPropertyID::kContrastColorDark);
	if(contrastHelper || color.isVisible ())
		getColorContrastHelper ().setContrastColorDark (color);

	color = getStyleColor (style, AtomPropertyID::kContrastColorTransparent);
	if(contrastHelper || color.isVisible ())
		getColorContrastHelper ().setContrastColorTransparent (color);

	color = getStyleColor (style, AtomPropertyID::kContrastColorAlphaBlend);
	if(contrastHelper || color.isVisible ())
		getColorContrastHelper ().setContrastColorAlphaBlend (color);

	if(contrastHelper)
	{
		contrastHelper->setDefaultColor (getBrush ().getColor ());

		if(!contrastHelper->getContrastColorDark ().isVisible ())
			contrastHelper->setContrastColorDark (contrastHelper->getDefaultColor ());
		if(!contrastHelper->getContrastColorTransparent ().isVisible ())
			contrastHelper->setContrastColorTransparent (contrastHelper->getDefaultColor ());
	}

	setImage (getStyleImage (style, icontext));

	borderLeftTopRadius = 0.f;
	style.getMetric (borderLeftTopRadius, AtomPropertyName (name, AtomPropertyID::kBorderLeftTopRadius), icontext);
	borderLeftBottomRadius = 0.f;
	style.getMetric (borderLeftBottomRadius, AtomPropertyName (name, AtomPropertyID::kBorderLeftBottomRadius), icontext);
	borderRightTopRadius = 0.f;
	style.getMetric (borderRightTopRadius, AtomPropertyName (name, AtomPropertyID::kBorderRightTopRadius), icontext);
	borderRightBottomRadius = 0.f;
	style.getMetric (borderRightBottomRadius, AtomPropertyName (name, AtomPropertyID::kBorderRightBottomRadius), icontext);

	Color borderColor (borderPen.getColor ());
	borderColor.setAlphaF (0.f);
	style.getColor (borderColor, AtomPropertyName (name, AtomPropertyID::kBorderColor), icontext);
	borderPen.setColor (borderColor);
	CoordF borderWidth = 1.f;
	style.getMetric (borderWidth, AtomPropertyName (name, AtomPropertyID::kBorderWidth), icontext);
	borderPen.setWidth (borderWidth);

	scale = 0.f;
	style.getMetric (scale, AtomPropertyName (name, AtomPropertyID::kScale), icontext);
	rotation = 0.f;
	style.getMetric (rotation, AtomPropertyName (name, AtomPropertyID::kRotation), icontext);
	opacity = 1.f;
	style.getMetric (opacity, AtomPropertyName (name, AtomPropertyID::kOpacity), icontext);
	offsetX = 0.f;
	style.getMetric (offsetX, AtomPropertyName (name, AtomPropertyID::kOffsetX), icontext);
	offsetY = 0.f;
	style.getMetric (offsetY, AtomPropertyName (name, AtomPropertyID::kOffsetY), icontext);
	imageFrame = 0;
	float frameValue = 0.f;
	style.getMetric (frameValue, AtomPropertyName (name, AtomPropertyID::kImageFrame), icontext);
	imageFrame = int(frameValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RenderAtom::finalizeVisualStyleUpdate (const RenderAtomTransaction& transaction)
{
	updateGradientSize ();

	for(const RenderAtomTransaction::PendingAnimation& pending : transaction.animations)
	{
		if(pending.atom != this)
			continue;

		Variant endValue;
		if(getProperty (endValue, pending.propertyId))
		{
			if(endValue != pending.startValue)
			{
				if(AutoPtr<IBasicAnimation> newAnimation = pending.animation->cloneAs<IBasicAnimation> ())
				{
					newAnimation->setStartValue (pending.startValue);
					newAnimation->setEndValue (endValue);
					AnimationManager::instance ().addAnimation (this, pending.propertyId, newAnimation, AnimationManager::kManualTargetRemoval);
					wasAnimationRunning (true);
					setProperty (pending.propertyId, pending.startValue);
				}
			}
			else if(wasAnimationRunning ())
			{
				AnimationManager::instance ().removeAnimation (this, pending.propertyId);
				setProperty (pending.propertyId, endValue);
			}
		}
	}

	initialized (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RenderAtom::getProperty (Variant& value, MemberID property) const
{
	if(property == AtomPropertyID::kOffsetX)
	{
		value = offsetX;
		return true;
	}
	else if(property == AtomPropertyID::kOffsetY)
	{
		value = offsetY;
		return true;
	}
	else if(property == AtomPropertyID::kColor)
	{
		AutoPtr<UIValue> uiValue = NEW UIValue ();
		uiValue->fromBrush (brush);
		value.takeShared (uiValue->asUnknown ());
		return true;
	}
	else if(property == AtomPropertyID::kBorderColor)
	{
		AutoPtr<UIValue> uiValue = NEW UIValue ();
		uiValue->fromColor (borderPen.getColor ());
		value.takeShared (uiValue->asUnknown ());
		return true;
	}
	else if(property == AtomPropertyID::kBorderWidth)
	{
		value = borderPen.getWidth ();
		return true;
	}
	else if(property == AtomPropertyID::kOpacity)
	{
		value = opacity;
		return true;
	}
	else if(property == AtomPropertyID::kBorderLeftTopRadius)
	{
		value = borderLeftTopRadius;
		return true;
	}
	else if(property == AtomPropertyID::kBorderLeftBottomRadius)
	{
		value = borderLeftBottomRadius;
		return true;
	}
	else if(property == AtomPropertyID::kBorderRightTopRadius)
	{
		value = borderRightTopRadius;
		return true;
	}
	else if(property == AtomPropertyID::kBorderRightBottomRadius)
	{
		value = borderRightBottomRadius;
		return true;
	}
	else if(property == AtomPropertyID::kScale)
	{
		if(scale <= 0.f)
			value = 1.f;
		else
			value = scale;

		return true;
	}
	else if(property == AtomPropertyID::kRotation)
	{
		value = rotation;
		return true;
	}
	else if(property == AtomPropertyID::kImageFrame)
	{
		value = imageFrame;
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RenderAtom::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == AtomPropertyID::kOffsetX)
		offsetX = var.asFloat ();
	else if(propertyId == AtomPropertyID::kOffsetY)
		offsetY = var.asFloat ();
	else if(propertyId == AtomPropertyID::kColor)
	{
		if(IUIValue* uiBrush = IUIValue::toValue (var))
			uiBrush->toBrush (brush);
	}
	else if(propertyId == AtomPropertyID::kBorderColor)
	{
		if(IUIValue* uiColor = IUIValue::toValue (var))
		{
			Color color;
			uiColor->toColor (color);
			borderPen.setColor (color);
		}
	}
	else if(propertyId == AtomPropertyID::kBorderWidth)
		borderPen.setWidth (var.asFloat ());
	else if(propertyId == AtomPropertyID::kOpacity)
		opacity = var.asFloat ();
	else if(propertyId == AtomPropertyID::kBorderLeftTopRadius)
		borderLeftTopRadius = var.asFloat ();
	else if(propertyId == AtomPropertyID::kBorderLeftBottomRadius)
		borderLeftBottomRadius = var.asFloat ();
	else if(propertyId == AtomPropertyID::kBorderRightTopRadius)
		borderRightTopRadius = var.asFloat ();
	else if(propertyId == AtomPropertyID::kBorderRightBottomRadius)
		borderRightBottomRadius = var.asFloat ();
	else if(propertyId == AtomPropertyID::kScale)
		scale = var.asFloat ();
	else if(propertyId == AtomPropertyID::kRotation)
		rotation = var.asFloat ();
	else if(propertyId == AtomPropertyID::kImageFrame)
		imageFrame = var.asInt ();

	invalidate ();
	return true;
}

//************************************************************************************************
// RenderAtom::ColorContrastHelper
//************************************************************************************************

Color RenderAtom::ColorContrastHelper::getForegroundColor (ColorRef backgroundColor) const
{
	if(!backgroundColor.isVisible ())
		return getContrastColorTransparent ();
	else if(getContrastColorBright ().isVisible ())
	{
		if(backgroundColor.getLuminance () < brightColorThreshold)
			return getContrastColorBright ();
		else
			return getContrastColorDark ();
	}
	else if(getContrastColorAlphaBlend ().isVisible ())
	{
		ColorRef alphaColor = getContrastColorAlphaBlend ();
		Color result (backgroundColor);
		result.alphaBlend (alphaColor, alphaColor.getAlphaF ());
		return result;
	}

	return backgroundColor;
}
