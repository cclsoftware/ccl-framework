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
// Filename    : ccl/gui/views/imageview.cpp
// Description : Image View class
//
//************************************************************************************************

#include "ccl/gui/views/imageview.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/views/viewanimation.h"

#include "ccl/gui/theme/visualstyle.h"
#include "ccl/gui/theme/legacystyleids.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/touch/touchhandler.h"

#include "ccl/gui/graphics/imaging/multiimage.h"
#include "ccl/gui/graphics/imaging/tiledimage.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/bitmappainter.h"
#include "ccl/gui/graphics/imaging/coloredbitmap.h"
#include "ccl/gui/graphics/imaging/imagecache.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/controlproperties.h"
#include "ccl/public/gui/framework/abstractdraghandler.h"

namespace CCL {

//************************************************************************************************
// ImageViewTouchHandler
//************************************************************************************************

class ImageViewTouchHandler: public TouchHandler
{
public:
	ImageViewTouchHandler (ImageView* imageView)
	: TouchHandler (imageView)
	{
		if(window = imageView->getWindow ())
		{
			addRequiredGesture (GestureEvent::kSwipe, GestureEvent::kPriorityNormal);
			addRequiredGesture (GestureEvent::kLongPress, GestureEvent::kPriorityNormal);
		}
	}
	
	tbool CCL_API onGesture (const GestureEvent& event) override
	{
		if(window)
		{
			Point where (event.where);
			where = window->clientToScreen (where);
			switch(event.getState ())
			{
			case GestureEvent::kBegin:
				{
					Point origin;
					origin = window->clientToScreen (origin);
					origin.offset (-where.x, -where.y);
					startOffset = origin;
				}
				break;
					
			case GestureEvent::kChanged:
				{
					Point newOrigin (startOffset);
					newOrigin.offset (where);
					window->moveWindow (newOrigin);
				}
				break;
					
			case GestureEvent::kEnd:
			case GestureEvent::kFailed:					
				break;
			}
			return true;
		}
		return false;
	}
	
protected:
	Point startOffset;
	Window* window;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// ImageView::ModeSelector
//************************************************************************************************

ImageView::ModeSelector::ModeSelector (const ImageView& imageView)
: modePtr (nullptr)
{
	if(imageView.getStyle ().isCustomStyle (Styles::kImageViewAppearanceHighQuality))
	{
		mode.setInterpolationMode (kImageInterpolationHighQuality);
		modePtr = &mode;
	}
}

//************************************************************************************************
// ImageView::SelectMouseHandler
/** Handles the kImageViewSelectOnClick option. */
//************************************************************************************************

class ImageView::SelectMouseHandler: public MouseHandler
{
public:
	SelectMouseHandler (ImageView* imageView)
	: MouseHandler (imageView)
	{}

	void onBegin () override
	{
		ImageView* imageView = (ImageView*)view;
		if(IParameter* selectParam = imageView->getSelectParam ())
			selectParam->setValue (selectParam->getValue () != selectParam->getMax () ? selectParam->getMax () : selectParam->getMin (), true);
	} 
};

//************************************************************************************************
// ImageView::InsertDataDragHandler
//************************************************************************************************

class ImageView::InsertDataDragHandler: public Unknown,
										public AbstractDragHandler
{
public:
	InsertDataDragHandler (ImageView& view);

	// IDragHandler
	tbool CCL_API afterDrop (const DragEvent& event) override;

	CLASS_INTERFACE (IDragHandler, Unknown)

private:
	ImageView& view;
};

//************************************************************************************************
// ImageView::InsertDataDragHandler
//************************************************************************************************

ImageView::InsertDataDragHandler::InsertDataDragHandler (ImageView& view)
: view (view)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImageView::InsertDataDragHandler::afterDrop (const DragEvent& event)
{
	bool dropped = false;

	if(IDataTarget* dataTarget = view.getDataTarget ())
	{
		dropped = dataTarget->insertData (event.session.getItems (), &event.session) != 0;
		if(dropped)
		{
			if(event.session.getResult () == DragSession::kDropNone)
				event.session.setResult (DragSession::kDropCopyReal);
		}
	}

	AbstractDragHandler::afterDrop (event);
	return dropped;
}

//************************************************************************************************
// ImageView
//************************************************************************************************

BEGIN_STYLEDEF (ImageView::customStyles)
	{"colorize",         Styles::kImageViewAppearanceColorize},
	{"contentfit",       Styles::kImageViewAppearanceContentFit},
	{"contentfill",      Styles::kImageViewAppearanceContentFill},
	{"selectonclick",    Styles::kImageViewBehaviorSelectOnClick},
	{"framesbyname",     Styles::kImageViewBehaviorFramesByName},
	{"windowsizable",    Styles::kImageViewBehaviorWindowSizable},
	{"windowmovable",    Styles::kImageViewBehaviorWindowMovable},
	{"windowmaximize",   Styles::kImageViewBehaviorWindowMaximize},
	{"insertdata",       Styles::kImageViewBehaviorInsertData},
	{"ignoreimagesize",	 Styles::kImageViewLayoutIgnoreImageSize},
	{"swallow-mouseclick", Styles::kImageViewBehaviorSwallowMouseClick},
	{"swallow-mousewheel", Styles::kImageViewBehaviorSwallowMouseWheel},
	{"swallowmouse",	 Styles::kImageViewBehaviorSwallowMouseClick|Styles::kImageViewBehaviorSwallowMouseWheel},
	{"backgroundlayer",	 Styles::kImageViewBehaviorBackgroundLayer},
	{"highquality",		 Styles::kImageViewAppearanceHighQuality},
	{"hfitimagesize",	 Styles::kImageViewLayoutHFitImageSize},
	{"disable",          Styles::kImageViewBehaviorDisable},
	#if LEGACY_STYLES_SUPPORTED
	// replaced by kImageViewAppearanceContentFit and kImageViewAppearanceContentFill
	{"fitimage",         Styles::kImageViewAppearanceFitImageLegacy},
	{"allowstretch",     Styles::kImageViewAppearanceAllowStretchLegacy},
	{"allowzoom",	     Styles::kImageViewAppearanceAllowZoomLegacy},
	{"centerimage",      Styles::kImageViewAppearanceCenterImageLegacy},
	#endif
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_TYPELIB_ENTRY_WITH_PARENT (ImageView, VisualStyle, "ImageViewStyle", VISUALSTYLES_TYPELIB)
BEGIN_TYPELIB_MEMBERS (ImageView)
	ADD_TYPELIB_MEMBER_RENDERATOM (AtomName::kBackground)			///< draws the image - its "color" colorizes a template or an adaptive image, and fills the view when there is no image ("colorize" option)
END_TYPELIB_MEMBERS (ImageView)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (ImageView, View)
DEFINE_CLASS_UID (ImageView, 0xb3973fca, 0x38a, 0x441c, 0xa3, 0xbd, 0x91, 0x3e, 0x7, 0x8b, 0xc3, 0x8f)

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageView::ImageView (IImage* _background, const Rect& size, StyleRef style, StringRef title)
: View (size, style, title),
  background (nullptr),
  backgroundAtom (AtomName::kBackground),
  frame (0),
  border (0),
  selectParam (nullptr),
  imageProvider (nullptr),
  transitionType (Styles::kTransitionNone),
  initialized (false)
  #if LEGACY_STYLES_SUPPORTED
  , imageColor (Colors::kTransparentBlack),
  contextColor (Colors::kTransparentBlack),
  useModifiedImage (false),
  modifiedImageAllowed (false),
  growImageAllowed (true)
  #endif
{
	backgroundAtom.setClient (this);
	setBackground (_background);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageView::~ImageView ()
{
	setSelectParam (nullptr);
	setImageProvider (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::onSize (const Point& delta)
{
	if(border != 0)
	{
		Rect rect;
		getClientRect (rect);

		Rect h (rect);
		if(delta.x > 0)
		{
			h.right = h.right - delta.x;
			h.left = h.right - border;
			invalidate (h);
		}
		else if(delta.x < 0)
		{
			h.left = h.right - border;
			invalidate (h);
		}

		if(delta.y > 0)
		{
			rect.bottom = rect.bottom - delta.y;
			rect.top = rect.bottom - border;
			invalidate (rect);
		}
		else if(delta.y < 0)
		{
			rect.top = rect.bottom - border;
			invalidate (rect);
		}
	}
	else
		invalidate ();

	if(backgroundLayer)
		backgroundLayer->setSize (size.getWidth (), size.getHeight ());

	SuperClass::onSize (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::onMove (const Point& delta)
{
	if(backgroundLayer)
	{
		Point offset;
		getParentLayer (offset);
		backgroundLayer->setOffset (offset);
	}
	SuperClass::onMove (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ImageView::getBackground () const 
{ 
	return background; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::setBackground (IImage* newBackground)
{
	if(background != newBackground)
	{
		SharedPtr<IImage> oldBackground = background;
		background = newBackground;
		hasBackgroundFromVisualStyle (false);

		// adjust size
		if(background && !getStyle ().isCustomStyle (Styles::kImageViewLayoutIgnoreImageSize) && (getSize ().isEmpty () || (style.isCustomStyle (Styles::kImageViewLayoutHFitImageSize))))
		{
			Rect r (getSize ());
			if(style.isCustomStyle (Styles::kImageViewLayoutHFitImageSize))
			{
				float ratio = (r.getHeight () / (float) background->getHeight ());
				r.setWidth (ccl_min (ccl_to_int (ratio * background->getWidth ()), background->getWidth ()));
			}
			else
			{
				r.setWidth (background->getWidth ());
				r.setHeight (background->getHeight ());
			}
			
			setSize (r);
		}
		
		if(selectParam)
			determineFrameIndex ();
		else if(background)
		{
			frame = background->getFrameIndex (ThemeNames::kNormal);
			background->setCurrentFrame (frame);
		}

		if(backgroundLayer && background)
		{
			background->setCurrentFrame (frame);
			backgroundLayer->setContent (background);
		}
		else
			invalidate ();

		// transition
		if(oldBackground && newBackground)
			if(AutoPtr<ViewAnimator> animator = ViewAnimator::create (this, transitionType))
			{
				animator->setFromImage (oldBackground);
				animator->setToImage (newBackground);
				animator->setIgnoreAlpha (false); // images might contain transparency (we don't know)
				animator->makeTransition ();
			}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::determineFrameIndex ()
{
	if(background)
	{
		if(style.isCustomStyle (Styles::kImageViewBehaviorFramesByName))
		{
			String frameName;
			selectParam->toString (frameName);
			frame = background->getFrameIndex (MutableCString (frameName));
		}
		else
		{
			frame = background->getFrameIndex (selectParam->getValue () ? ThemeNames::kPressed : ThemeNames::kNormal);
			if(frame < 0)
				frame = selectParam->getValue ().asInt ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::attached (View* parent)
{
	initialize ();
		
	#if 1 // first step to get rid of this extra option...
	// this is an optimization to draw background bitmaps directly into a layer
	if(isLayerBackingEnabled () && unknown_cast<Bitmap> (background))
		style.setCustomStyle (Styles::kImageViewBehaviorBackgroundLayer);
	#endif

	if(getStyle ().isCustomStyle (Styles::kImageViewBehaviorDisable))
		enable (false);
	
	if(getStyle ().isCustomStyle (Styles::kImageViewBehaviorBackgroundLayer))
	{
		if(background && !backgroundLayer)
		{
			background->setCurrentFrame (frame);
			backgroundLayer = addGraphicsSublayer (background);
			
			// layer for subviews
			if(!views.isEmpty ())
			{
				style.setCommonStyle (Styles::kTranslucent);
				privateFlags |= kLayerBacking;
			}
		}
	}

	SuperClass::attached (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::removed (View* parent)
{
	SuperClass::removed (parent);

	if(backgroundLayer)
	{
		if(IGraphicsLayer* parentLayer = backgroundLayer->getParentLayer ())
			parentLayer->removeSublayer (backgroundLayer);
		backgroundLayer.release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* CCL_API ImageView::getParentLayer (Point& offset) const
{
	#if 0 // TODO: use background layer for subviews?
	if(backgroundLayer)
		return backgroundLayer;
	else
	#endif
		return SuperClass::getParentLayer (offset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageView::isSelected () const
{
	return selectParam != nullptr && selectParam->getValue ().asBool ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::updateAtomStyle ()
{
	// a select parameter that is on is the "on" style condition, so that the atom properties can
	// have an "on" variant
	conditionContext.setCondition (ThemeNames::kOn, isSelected ());

	RenderAtomTransaction transaction;
	VisualStyle* visualStyle = getVisualStyleDirect ();
	if(visualStyle)
		backgroundAtom.updateFromVisualStyle (transaction, *visualStyle, conditionContext);
	else
		backgroundAtom.setBrush (SolidBrush (Colors::kTransparentBlack));

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	applyLegacyVisualStyle (); // needs the end values - finalizeVisualStyleUpdate () resets animated properties to their start
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	if(visualStyle)
		backgroundAtom.finalizeVisualStyleUpdate (transaction);

	// "background.image" can have an "on" variant - an image from the style follows the atom
	if(hasBackgroundFromVisualStyle ())
		if(Image* atomImage = backgroundAtom.getImage ())
			background = atomImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::initialize ()
{
	const IVisualStyle& vs = getVisualStyle ();

	updateAtomStyle ();

	if(background == nullptr)
	{
		background = backgroundAtom.getImage (); // "background.image"
		hasBackgroundFromVisualStyle (background != nullptr);
	}

	if(TiledImage* tiledImage = unknown_cast<TiledImage> (background))
	{
		// use largest margin for border
		Rect margins = tiledImage->getMargins ();
		Coord b = ccl_max (margins.left, margins.right);
		ccl_lower_limit (b, ccl_max (margins.top, margins.bottom));
		border = b;
	}
	else
		border = 0;

	if(selectParam)
		determineFrameIndex ();

	// The image is stretched over the whole view by default. "contentfit" scales it into the view
	// preserving its aspect ratio, "contentfill" makes it cover the view, and both together crop it.
	// Without either of them the image keeps its own size in the center of the view.
	bool contentFit = style.isCustomStyle (Styles::kImageViewAppearanceContentFit);
	bool contentFill = style.isCustomStyle (Styles::kImageViewAppearanceContentFill);

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	bool lightAdaptImage = vs.getMetric<bool> (LegacyStyleID::kLightAdaptImage, false);
	if(lightAdaptImage || vs.getMetric<bool> (LegacyStyleID::kColorizeImage, false))
	{
		if(Image* image = unknown_cast<Image> (background))
		{
			image->setIsTemplate (!lightAdaptImage); // "lightadapt.image" wins
			image->setIsAdaptive (lightAdaptImage);
		}
	}

	if(style.isCustomStyle (Styles::kImageViewAppearanceFitImageLegacy))
	{
		bool allowStretch = style.isCustomStyle (Styles::kImageViewAppearanceAllowStretchLegacy);
		contentFit = true;
		contentFill = style.isCustomStyle (Styles::kImageViewAppearanceAllowZoomLegacy) && !allowStretch;
		growImageAllowed = contentFill || allowStretch;
	}
	else if(style.isCustomStyle (Styles::kImageViewAppearanceCenterImageLegacy))
	{
		contentFit = false;
		contentFill = false;
	}
	else if(!contentFit && !contentFill)
		contentFill = true;
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif // LEGACY_STYLES_SUPPORTED

	backgroundAtom.isContentFit (contentFit);
	backgroundAtom.isContentFill (contentFill);
	backgroundAtom.setImageInterpolation (style.isCustomStyle (Styles::kImageViewAppearanceHighQuality) ?
										  kImageInterpolationHighQuality : kImageInterpolationDefault);

	initialized = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#if LEGACY_STYLES_SUPPORTED
//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::applyLegacyVisualStyle ()
{
	const IVisualStyle& vs = getVisualStyle ();

	if(IImage* legacyBackground = vs.getBackgroundImage ())
		backgroundAtom.setImage (unknown_cast<Image> (legacyBackground));

	Color atomColor = backgroundAtom.getBrush ().getColor (); // "background.color"

	RenderAtom::ColorContrastHelper& contrastHelper = backgroundAtom.getColorContrastHelper ();
	Color alphaBlendColor = vs.getColor (LegacyStyleID::kImageColorAlphaBlend, vs.getColor (LegacyStyleID::kColorAlphaBlend, Colors::kTransparentBlack));
	if(alphaBlendColor.isVisible ())
		contrastHelper.setContrastColorAlphaBlend (alphaBlendColor);

	Color baseImageColor (alphaBlendColor);
	bool hasLegacyImageColor = vs.getColor (baseImageColor, LegacyStyleID::kImageColor) || alphaBlendColor.isVisible ();
	if(!hasLegacyImageColor)
		baseImageColor = atomColor;

	imageColor = baseImageColor;
	if(isSelected ())
		vs.getColor (imageColor, LegacyStyleID::kImageColorOn);

	contextColor = vs.getColor (LegacyStyleID::kImageColorContext, vs.getColor (LegacyStyleID::kContextColor, atomColor));

	contrastHelper.setContrastColorBright (vs.getColor (LegacyStyleID::kImageColorBright, contrastHelper.getContrastColorBright ()));

	// the dark and the transparent contrast color follow "imagecolor"
	if(hasLegacyImageColor)
		contrastHelper.setContrastColorDark (baseImageColor);

	Color transparentColor (baseImageColor);
	if(vs.getColor (transparentColor, LegacyStyleID::kImageColorTransparent) || hasLegacyImageColor)
		contrastHelper.setContrastColorTransparent (transparentColor);

	contrastHelper.setBrightColorThreshold (vs.getMetric<float> (LegacyStyleID::kImageColorThreshold, contrastHelper.getBrightColorThreshold ()));

	modifiedImageAllowed = baseImageColor != Colors::kTransparentBlack;
	if(!modifiedImageAllowed && vs.getColor (StyleID::kBackColor, Colors::kTransparentBlack) == Colors::kTransparentBlack)
		modifiedImageAllowed = UnknownPtr<IColorParam> (selectParam).isValid ();

	useModifiedImage = style.isCustomStyle (Styles::kImageViewAppearanceColorize) && modifiedImageAllowed;
	if(vs.getMetric<bool> (LegacyStyleID::kColorizeImage, false) || vs.getMetric<bool> (LegacyStyleID::kLightAdaptImage, false))
		useModifiedImage = modifiedImageAllowed;

	backgroundAtom.setScale (vs.getMetric<float> (LegacyStyleID::kImageFillFactor, backgroundAtom.getScale ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#endif // LEGACY_STYLES_SUPPORTED
//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::onVisualStyleChanged ()
{
	if(hasBackgroundFromVisualStyle ())
	{
		// discard background when visual style changes
		if(backgroundLayer)
		{
			if(IGraphicsLayer* parentLayer = backgroundLayer->getParentLayer ())
				parentLayer->removeSublayer (backgroundLayer);
			backgroundLayer.release ();
		}
		background.release ();
	}

	initialized = false;

	SuperClass::onVisualStyleChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	if(getVisualStyle ().hasReferences (event.scheme))
		initialized = false;
	
	SuperClass::onColorSchemeChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayerContent::LayerHint CCL_API ImageView::getLayerHint () const
{
	if(isEmpty () && !background)
	{
		// Until draw () has read the style again (see onVisualStyleChanged ()), what it draws is not known yet.
		// An empty layer would not be drawn at all, so the style would never be read.
		if(!initialized)
			return kGraphicsContentTranslucent;

		// the brush draw () fills the view with - the atom can still leave parts uncovered (radius, opacity, scale)
		return getFillBrush ().isVisible () ? kGraphicsContentTranslucent : kGraphicsContentEmpty;
	}
	return kGraphicsContentHintDefault;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::draw (const UpdateRgn& updateRgn)
{
	if(backgroundLayer && !View::isRendering ())
	{
		SuperClass::draw (updateRgn);
		return;
	}

	if(initialized == false)
		initialize ();

	Image* image = unknown_cast<Image> (background);
	backgroundAtom.setImage (image);

	// The atom brush is "background.color" (an animation of it changes the brush directly), draw () only
	// borrows the atom for the brush it actually draws with and gives it back afterwards.
	Brush styleBrush (backgroundAtom.getBrush ());

	// The atom color colorizes a template or an adaptive image (via the ModifiedImageCache in
	// RenderAtom::draw ()). Without an image it would fill the whole view, which only "colorize" asks for.
	Brush brush (image ? SolidBrush (getImageColor ()) : getFillBrush ());
	backgroundAtom.setBrush (brush);

	if(image || brush.isVisible ())
	{
		if(image)
			backgroundAtom.setImageFrame (frame);

		Rect dst;
		getClientRect (dst);
		RectF dstF (rectIntToF (dst));

		#if LEGACY_STYLES_SUPPORTED
		BEGIN_LEGACY_STYLES_ENABLED_BLOCK
		if(image && !growImageAllowed && backgroundAtom.isContentFit ())
		{
			// if the image is not allowed to grow but the content size is larger than the image in both dimensions,
			// set the atom size to the image size so that it can't grow
			RectF imageSize (image->getSizeF ());
			CoordF xSpace = dstF.getWidth () - imageSize.getWidth ();
			CoordF ySpace = dstF.getHeight () - imageSize.getHeight ();
			if(xSpace > 0.f && ySpace > 0.f)
			{
				dstF.setWidth (dstF.getWidth () - xSpace);
				dstF.setHeight (dstF.getHeight () - ySpace);
				dstF.offset (xSpace / 2.f, ySpace / 2.f);
			}
		}
		END_LEGACY_STYLES_ENABLED_BLOCK
		#endif // LEGACY_STYLES_SUPPORTED

		backgroundAtom.setSize (dstF);

		GraphicsPort port (this);
		backgroundAtom.draw (port, updateRgn);
	}

	backgroundAtom.setBrush (styleBrush);

	SuperClass::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::invalidate (const RenderAtom& atom)
{
	invalidate (rectFToEnclosingInt (atom.getSize ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImageView::canDrawControlBackground () const
{
	return getBackground () || getStyle ().isCustomStyle (Styles::kImageViewAppearanceColorize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Color ImageView::getImageColor ()
{
	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	// "imagecolor" needed the "colorize" option - an adaptive image was colorized without it
	// (adaptive="true" at the resource, formerly the "lightadapt.image" metric)
	Image* image = unknown_cast<Image> (background);
	if(!useModifiedImage && !(modifiedImageAllowed && image && image->getIsAdaptive ()))
		return contextColor;
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	// a color parameter provides the color, kept readable on a dark background by the contrast colors
	if(UnknownPtr<IColorParam> colorParam = selectParam)
	{
		Color colorParamColor;
		colorParam->getColor (colorParamColor);
		return backgroundAtom.getColorContrastHelper ().getForegroundColor (colorParamColor);
	}

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	return imageColor; // "imagecolor" / "imagecolor.on", falling back to the atom color
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	return backgroundAtom.getBrush ().getColor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Brush ImageView::getFillBrush () const
{
	// Without an image the atom color fills the view, it colorizes an image otherwise.
	if(!style.isCustomStyle (Styles::kImageViewAppearanceColorize))
		return SolidBrush (Colors::kTransparentBlack);

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	// "backcolor" (or "forecolor" when the select parameter is on) used to fill the view - but only
	// when its color is not meant for an image: a view that takes its image from a provider must not
	// show a color while that image is missing, and neither must one that colorizes its image.
	if(imageProvider == nullptr && !useModifiedImage)
	{
		const IVisualStyle& vs = getVisualStyle ();
		CStringPtr colorName = StyleID::kBackColor;
		if(isSelected ())
			colorName = StyleID::kForeColor;

		Color legacyColor;
		bool hasLegacyColor = vs.getGradient (colorName) || vs.getColor (legacyColor, colorName);
		if(hasLegacyColor || !backgroundAtom.getBrush ().isVisible ())
		{
			if(isSelected ())
				return vs.getForeBrush ();

			return vs.getBackBrush ();
		}
	}
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	if(backgroundAtom.getBrush ().isVisible ())
		return backgroundAtom.getBrush ();

	return SolidBrush (Colors::kTransparentBlack);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ImageView::drawControlBackground (IGraphics& graphics, RectRef src, PointRef offset)
{
	Rect dst (src);
	dst.offset (offset);

	if(background)
	{
		ModeSelector selector (*this);
		background->setCurrentFrame (frame);
		if(TiledImage* tiledImage = unknown_cast<TiledImage> (background))
		{
			Rect sourceRect;
			tiledImage->getSize (sourceRect);
			dst = sourceRect;
			dst.offset (offset);
			graphics.drawImage (background, sourceRect, dst, selector.modePtr);
		}
		else
		{	
			graphics.drawImage (background, src, dst, selector.modePtr);
		}
	}
	else
	{
		Brush fillBrush (getFillBrush ());
		if(fillBrush.isVisible ())
			graphics.fillRect (dst, fillBrush);
	}
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ImageView::notify (ISubject* s, MessageRef msg)
{
	if(msg == kChanged)
	{
		if(selectParam && isEqualUnknown (selectParam, s))
		{
			if(initialized)
				updateAtomStyle (); // the "on" style condition follows the select parameter
			determineFrameIndex ();
			if(backgroundLayer)
			{
				background->setCurrentFrame (frame);
				backgroundLayer->setContent (background);
			}
			else
			{
				invalidate ();
				//updateClient (); // maybe later as an option if required somewhere
			}
		}
		else if(imageProvider && isEqualUnknown (imageProvider, s))
		{
			setBackground (imageProvider->getImage ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* ImageView::createMouseHandler (const MouseEvent& event)
{
	if(selectParam && getStyle().isCustomStyle (Styles::kImageViewBehaviorSelectOnClick))
		return NEW SelectMouseHandler (this);

	SharedPtr<Object> holder (this); // view might get removed below during resize, maximize, etc.

	if(Window* window = getWindow ())
	{
		if(getStyle().isCustomStyle (Styles::kImageViewBehaviorWindowSizable)
			&& window->getStyle ().isCustomStyle (Styles::kWindowBehaviorSizable))
		{
			Rect client;
			getClientRect (client);
			client.left = client.right - 16;
			client.top = client.bottom - 16;
			if(client.pointInside (event.where))
			{
				if(event.dragged != 0)
					window->resizeWindow ();
				return NEW NullMouseHandler (this); // swallow mouse click
			}
		}

		if(getStyle().isCustomStyle (Styles::kImageViewBehaviorWindowMaximize)
			&& window->getStyle ().isCustomStyle (Styles::kWindowBehaviorSizable)
			&& detectDoubleClick (event))
		{
			window->tryMaximize (!window->isMaximized ());
			return NEW NullMouseHandler (this); // swallow mouse click
		}

		if(getStyle().isCustomStyle (Styles::kImageViewBehaviorWindowMovable))
		{
			if(event.dragged != 0)
				window->moveWindow ();
			return NEW NullMouseHandler (this); // swallow mouse click
		}
	}

	if(getStyle().isCustomStyle (Styles::kImageViewBehaviorSwallowMouseClick))
		return NEW NullMouseHandler (this); // swallow mouse click

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageView::onMouseWheel (const MouseWheelEvent& event)
{
	bool result = SuperClass::onMouseWheel (event);

	if(result == false && getStyle().isCustomStyle (Styles::kImageViewBehaviorSwallowMouseWheel))
		return true;
	
	return result;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* ImageView::createTouchHandler (const TouchEvent& event)
{
	if(getStyle().isCustomStyle (Styles::kImageViewBehaviorWindowMovable))
		return NEW ImageViewTouchHandler (this);
	else
		return SuperClass::createTouchHandler (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageView::onDragEnter (const DragEvent& event)
{
	if(dataTarget)
	{
		if(dataTarget->canInsertData (event.session.getItems (), &event.session, this))
		{
			IDragHandler* dragHandler = event.session.getDragHandler ();
			if(dragHandler)
				dragHandler->dragEnter (event);

			if(!dragHandler || getStyle ().isCustomStyle (Styles::kImageViewBehaviorInsertData))
			{
				if(event.session.getResult () == IDragSession::kDropNone)
					event.session.setResult (IDragSession::kDropCopyReal);
				event.session.setDragHandler (AutoPtr<IDragHandler> (NEW InsertDataDragHandler (*this)));
			}

			return true;
		}
	}
	return SuperClass::onDragEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::setSelectParam (IParameter* _selectParam)
{
	if(selectParam != _selectParam)
	{
		if(selectParam)
		{
			ISubject::removeObserver (selectParam, this);
			selectParam->release ();
		}
		selectParam = _selectParam;
		if(selectParam)
		{
			ISubject::addObserver (selectParam, this);
			selectParam->retain ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ImageView::getSelectParam () const
{
	return selectParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageView::setImageProvider (IImageProvider* provider)
{
	UnknownPtr<ISubject> subject (imageProvider);
	if(subject)
		subject->removeObserver (this);
	take_shared (imageProvider, provider);
	subject = imageProvider;
	if(subject)
		subject->addObserver (this);

	if(imageProvider)
		setBackground (imageProvider->getImage ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImageView::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == kImageViewBackground)
	{
		var = background;
		return true;
	}
	if(propertyId == kImageViewDataTarget)
	{
		var = dataTarget;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImageView::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == kImageViewBackground)
	{
		Image* image = unknown_cast<Image> (var);
		setBackground (image);
		return true;
	}
	if(propertyId == kImageViewFrame)
	{
		frame = var.asInt ();
		invalidate ();
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}
