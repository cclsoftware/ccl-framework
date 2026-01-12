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
		mode.setInterpolationMode (ImageMode::kInterpolationHighQuality);
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
	{"fitimage",         Styles::kImageViewAppearanceFitImage},
	{"allowstretch",     Styles::kImageViewAppearanceAllowStretch},
	{"allowzoom",	     Styles::kImageViewAppearanceAllowZoom},
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
	{"centerimage",      Styles::kImageViewAppearanceCenterImage},
	{"disable",          Styles::kImageViewBehaviorDisable},	
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_VISUALSTYLE_CLASS (ImageView, VisualStyle, "ImageViewStyle")
	ADD_VISUALSTYLE_IMAGE ("background")				///< the image to be drawn
	ADD_VISUALSTYLE_METRIC ("border")					///< (set automatically for tiled images) - used to optimize redraws when the ImageView is resized
	ADD_VISUALSTYLE_METRIC ("fill.image")				///< image is resized to fill out the ImageView size (aspectratio is kept) value: ]0-1]
	ADD_VISUALSTYLE_COLOR  ("backcolor")				///< fill the view when "selectname" is off (colorize option set) and no image is available
	ADD_VISUALSTYLE_COLOR  ("forecolor")				///< fill the view when "selectname" is on (colorize option set) and no image is available
	ADD_VISUALSTYLE_COLOR  ("imagecolor")				///< imagecolor is used to colorize the image (colorize option set)
	ADD_VISUALSTYLE_COLOR  ("imagecolor.on")			///< imagecolor.on is used to colorize the image when the "selectname" is on (colorize option set)
	ADD_VISUALSTYLE_COLOR  ("imagecolor.context")		///< imagecolor.context is used (colorize option NOT set) to modify the image automatically when supported - needs to be a template/monochrome image (colorize filter is used) or an adaptive image (lightadapt filter is used)
	ADD_VISUALSTYLE_COLOR  ("imagecolor.transparent")	///< imagecolor.transparent is used (colorize option set) when current colorparam color is transparent
	ADD_VISUALSTYLE_COLOR  ("imagecolor.alphablend")	///< the non-transparent "colorname" color will be alphablended with this color (using "color.alphablend" as fallback) and used as imagecolor
	ADD_VISUALSTYLE_COLOR  ("imagecolor.bright")		///< imagecolor.bright is used when colorize option is set and the current luminance of the "colorname" color is below the "imagecolor.threshold"
	ADD_VISUALSTYLE_METRIC ("imagecolor.threshold")		///< imagecolor.bright is used instead of "imagecolor" if the luminance threshold for the current "colorname" color is below this value - default is 0.35
END_VISUALSTYLE_CLASS (ImageView)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (ImageView, View)
DEFINE_CLASS_UID (ImageView, 0xb3973fca, 0x38a, 0x441c, 0xa3, 0xbd, 0x91, 0x3e, 0x7, 0x8b, 0xc3, 0x8f)

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageView::ImageView (IImage* _background, const Rect& size, StyleRef style, StringRef title)
: View (size, style, title),
  background (nullptr),
  frame (0),
  border (0),
  selectParam (nullptr),
  imageProvider (nullptr),
  transitionType (Styles::kTransitionNone),
  imageFillSize (0.f),
  brightColorThreshold (0.35f),
  useModifiedImage (false),
  drawAsTemplate (true),
  initialized (false)
{
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

void ImageView::initialize ()
{
	const IVisualStyle& vs = getVisualStyle ();

	if(background == nullptr)
	{
		background = vs.getImage ("background");
		hasBackgroundFromVisualStyle (true);
	}
	border = vs.getMetric<int> ("border", 0);
	
	imageFillSize = vs.getMetric<float> ("fill.image", 0.f);
	
	alphaBlendColor = vs.getColor ("imagecolor.alphablend", vs.getColor ("color.alphablend", Colors::kTransparentBlack));
	imageColor = vs.getColor ("imagecolor", alphaBlendColor);
	imageColorOn = vs.getColor ("imagecolor.on", imageColor);
	imageContrastTransparentColor = vs.getColor ("imagecolor.transparent", imageColor);
	imageContrastBrightColor = vs.getColor ("imagecolor.bright", Colors::kTransparentBlack);
	imageContextColor = vs.getColor ("imagecolor.context", vs.getColor ("contextcolor", Colors::kTransparentBlack));
		
	brightColorThreshold = vs.getMetric<float> ("imagecolor.threshold", brightColorThreshold);
	
	bool colorizeImage = vs.getMetric<bool> ("colorize.image", style.isCustomStyle (Styles::kImageViewAppearanceColorize));
	bool lightAdaptImage = vs.getMetric<bool> ("lightadapt.image", false);
	drawAsTemplate = !lightAdaptImage;
	
	if(border == 0)
	{
		if(TiledImage* tiledImage = unknown_cast<TiledImage> (background))
		{
			// if no border specified for a tiled image: use largest margin
			Rect margins = tiledImage->getMargins ();
			Coord b = ccl_max (margins.left, margins.right);
			ccl_lower_limit (b, ccl_max (margins.top, margins.bottom));
			border = b;
		}
	}
		
	if(selectParam)
		determineFrameIndex ();
	
	// compatibility check: colorize when imagecolor is set or use colorparam color when no backcolor is set
	
	Color fillBackColor = vs.getColor (StyleID::kBackColor, Colors::kTransparentBlack);
	if(colorizeImage || lightAdaptImage)
	{
		// compatibility check:
		if(imageColor != Colors::kTransparentBlack)
		{
			useModifiedImage = true;
		}
		else if(fillBackColor == Colors::kTransparentBlack)
		{
			if(UnknownPtr<IColorParam> colorParam = selectParam)
				useModifiedImage = true;
		}
	}
	
	initialized = true;
}

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
		if(!style.isCustomStyle (Styles::kImageViewAppearanceColorize))
			return kGraphicsContentEmpty;
			
		if(selectParam && selectParam->getValue ())
		{
			if(getVisualStyle ().getForeColor ().isTranslucent ())
				return kGraphicsContentTranslucent;
		}
		else
		{
			if(getVisualStyle ().getBackColor ().isTranslucent ())
				return kGraphicsContentTranslucent;			
		}
		return kGraphicsContentOpaque;
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

	if(background)
	{
		GraphicsPort port (this);
		Rect src (0, 0, background->getWidth (), background->getHeight ());
		Rect dst;
		getClientRect (dst);
		
		ModeSelector selector (*this);
		
		IImage* modifiedBackground = background;
		
		if(style.isCustomStyle (Styles::kImageViewAppearanceFitImage))
		{
			int flags = 0;
			if(style.isCustomStyle (Styles::kImageViewAppearanceAllowStretch))
				flags = ImageResolutionSelector::kAllowStretch;
			else if(style.isCustomStyle (Styles::kImageViewAppearanceAllowZoom))
				flags = ImageResolutionSelector::kAllowZoom;
			
			Image* drawable = unknown_cast<Image> (background);
			
			ImageResolutionSelector s (drawable, dst, flags, frame);
			modifiedBackground = s.bestImage;
			src = s.srcRect;
			dst = s.dstRect;
			
			Point imageSize (src.getWidth (), src.getHeight ());
			Point imagePos;
						
			if(imageFillSize > 0)
			{
				float imageResize = imageFillSize * ccl_min (dst.getHeight (), dst.getWidth ());
				float resizeRatio = (imageResize > 0.f) ? (imageResize / ccl_max (imageSize.x, imageSize.y)) : 1;
				imageSize *= resizeRatio;
				imagePos = dst.getLeftTop () + ((dst.getSize () - imageSize) * .5f);
			
				dst.setSize (imageSize);
				dst.moveTo (imagePos);
			}
		}
		else if(style.isCustomStyle (Styles::kImageViewAppearanceCenterImage))
		{
			modifiedBackground->setCurrentFrame (frame);
			Point imageSize (modifiedBackground->getWidth (), modifiedBackground->getHeight ());
			dst.moveTo (dst.getLeftTop () + ((dst.getSize () - imageSize) * .5f));
			dst.setSize (imageSize);
		}
		else
			background->setCurrentFrame (frame);
		
		if(modifiedBackground)
		{
			Color color;
			if(hasModifyBackgroundColor (color))
				modifiedBackground = ModifiedImageCache::instance ().lookup (modifiedBackground, color, drawAsTemplate);
			
			port.drawImage (modifiedBackground, src, dst, selector.modePtr);
		}
	}
	else 
	{
		if(style.isCustomStyle (Styles::kImageViewAppearanceColorize) && !useModifiedImage)
		{
			GraphicsPort port (this);
			if(selectParam && selectParam->getValue ())
				port.fillRect (updateRgn.bounds, getVisualStyle ().getForeBrush ());
			else
				port.fillRect (updateRgn.bounds, getVisualStyle ().getBackBrush ());
		}
	}

	SuperClass::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImageView::canDrawControlBackground () const
{
	return getBackground () || getStyle ().isCustomStyle (Styles::kImageViewAppearanceColorize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageView::hasModifyBackgroundColor (Color& color) const
{
	if(Image* drawable = unknown_cast<Image> (background))
	{
		if(useModifiedImage)
		{
			if(UnknownPtr<IColorParam> colorParam = selectParam)
			{
				colorParam->getColor (color);
			
				if(color.getAlphaF () == 0)
				{
					color = imageContrastTransparentColor;
				}
				else if(imageContrastBrightColor != Colors::kTransparentBlack)
				{
					if(color.getLuminance () < brightColorThreshold)
						color = imageContrastBrightColor;
					else
						color = imageColor;
				}
				else if(alphaBlendColor.getAlphaF () != 0)
				{
					color.alphaBlend (alphaBlendColor, alphaBlendColor.getAlphaF ());
				}
			}
			else
			{
				bool isOn = selectParam ? selectParam->getValue ().asBool () : false;
				color = isOn ? imageColorOn : imageColor;
			}
			
			return true;
		}
		else
		{
			color = imageContextColor;
			if(color.getAlphaF () > 0)
				if(drawable->getIsAdaptive () || drawable->getIsTemplate ())
					return true;
		}
	}
		
	return false;
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
		if(style.isCustomStyle (Styles::kImageViewAppearanceColorize))
			graphics.fillRect (dst, getVisualStyle ().getBackBrush ());
	}
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ImageView::notify (ISubject* s, MessageRef msg)
{
	if(msg == kChanged)
	{
		if(selectParam && isEqualUnknown (selectParam, s))
		{
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
	if(propertyId == "dataTarget")
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
	if(propertyId == "frame")
	{
		frame = var.asInt ();
		invalidate ();
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}
