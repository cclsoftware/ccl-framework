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
// Filename    : ccl/gui/views/sprite.cpp
// Description : Sprite
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/views/sprite.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/windows/transparentwindow.h"
#include "ccl/gui/graphics/imaging/offscreen.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/gui/iapplication.h"

using namespace CCL;

//************************************************************************************************
// Sprite
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Sprite, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Sprite* Sprite::createSprite (View* view, IDrawable* drawable, const Rect& size, int options)
{
	if((options & kKeepOnTop) == 0)
		if(NativeGraphicsEngine::instance ().hasGraphicsLayers ())
			return NEW SublayerSprite (view, drawable, size, options);

	return NEW FloatingSprite (view, drawable, size, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Sprite::Sprite (View* _view, IDrawable* _drawable, const Rect& _size, int _options)
: view     (_view),
  drawable (_drawable),
  size     (_size),
  options  (_options & ~kVisible)
{
	if(drawable)
		drawable->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Sprite::~Sprite ()
{
	if(drawable)
		drawable->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Sprite::construct (IView* _view, RectRef _size, IDrawable* _drawable, int _options)
{
	ASSERT (view == nullptr)
	if(view)
		return kResultUnexpected;

	view = unknown_cast<View> (_view);
	ASSERT (view != nullptr)
	if(!view)
		return kResultInvalidArgument;

	size = _size;
	take_shared (drawable, _drawable);
	options = _options & ~kVisible;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Sprite::takeOpacity (IDrawable* drawable)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* Sprite::getView () const
{
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDrawable* Sprite::getDrawable () const
{
	return drawable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Sprite::isVisible () const
{
	return visible ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RectRef CCL_API Sprite::getSize () const
{
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Sprite::show ()
{
	visible (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Sprite::hide ()
{
	visible (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Sprite::move (RectRef _size)
{
	// default behavior: hide -> move -> show
	if(size != _size)
	{
		tbool wasVisible = isVisible ();
		if(wasVisible)
			hide ();

		size = _size;

		if(wasVisible)
			show ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Sprite::moveTo (PointRef position)
{
	Rect rect (getSize ());
	rect.moveTo (position);
	move (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Sprite::scrolled (PointRef delta)
{
	size.offset (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Sprite::refresh ()
{
	hide ();
	show ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Sprite)
	DEFINE_METHOD_NAME ("construct")
	DEFINE_METHOD_NAME ("show")
	DEFINE_METHOD_NAME ("hide")
	DEFINE_METHOD_NAME ("refresh")
END_METHOD_NAMES (Sprite)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Sprite::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "construct")
	{
		UnknownPtr<IView> view (msg[0].asUnknown ());
		Coord width = msg[1].asInt ();
		Coord height = msg[2].asInt ();
		UnknownPtr<IImage> image (msg[3]);
		int options = kKeepOnTop;
		ASSERT (view && image)
		if(view && image)
		{
			AutoPtr<IDrawable> drawable = NEW ImageDrawable (image);
			construct (view, Rect (0, 0, width, height), drawable, options);
		}
		return true;
	}
	else if(msg == "show")
	{
		show ();
		return true;
	}
	else if(msg == "hide")
	{
		hide ();
		return true;
	}
	else if(msg == "refresh")
	{
		refresh ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ManagedSprite
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ManagedSprite, Sprite)

//////////////////////////////////////////////////////////////////////////////////////////////////

ManagedSprite::ManagedSprite (View* view, IDrawable* drawable, const Rect& size, int options)
: Sprite (view, drawable, size, options)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ManagedSprite::show ()
{
	CCL_PRINTF ("ManagedSprite::show (was visible: %d, size %d x %d)\n", visible (), size.getWidth (), size.getHeight ())
	if(!visible ())
	{
		visible (true);
		GUI.addIdleTask (this);
		update (kOnShow);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ManagedSprite::hide ()
{
	if(visible ())
	{
		visible (false);
		GUI.removeIdleTask (this);
		update (kOnHide);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ManagedSprite::move (RectRef newSize)
{
	if(newSize != size)
	{
		CCL_PRINTF ("ManagedSprite::move (visible: %d, size %d x %d)\n", visible (), newSize.getWidth (), newSize.getHeight ())
		size = newSize;
		size.normalize ();
		update (kOnMove);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ManagedSprite::refresh ()
{
	update (kOnRefresh);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ManagedSprite::onTimer (ITimer* timer)
{
	update (kOnIdle);
}

//************************************************************************************************
// FloatingSprite
//************************************************************************************************

DEFINE_CLASS (FloatingSprite, ManagedSprite)
DEFINE_CLASS_UID (FloatingSprite, 0x7da79f66, 0x4676, 0x460e, 0xb1, 0x66, 0x2c, 0xde, 0xf8, 0x74, 0xbd, 0xf4)

//////////////////////////////////////////////////////////////////////////////////////////////////

FloatingSprite::FloatingSprite (View* view, IDrawable* drawable, const Rect& size, int options)
: ManagedSprite (view, drawable, size, options),
  window (nullptr),
  offscreen (nullptr)
{
	oldSize.setReallyEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FloatingSprite::~FloatingSprite ()
{
	removeWindow ();

	safe_release (offscreen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FloatingSprite::takeOpacity (IDrawable* drawable)
{
	drawable->takeOpacity ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap& FloatingSprite::getOffscreen (int width, int height)
{
	if(offscreen == nullptr  || (offscreen && (offscreen->getWidth () != width || offscreen->getHeight () != height)))
	{
		safe_release (offscreen);
		Window* parentWindow = nullptr;
		if(view)
			parentWindow = view->getWindow ();
		offscreen = NEW Offscreen (width, height, Offscreen::kRGBAlpha, false, parentWindow);
	}
	return *offscreen;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TransparentWindow& FloatingSprite::getTransparentWindow ()
{
	if(window == nullptr)
	{
		// try to use app title in case the OS displays it somewhere
		String title (CCLSTR ("Sprite"));
		if(IApplication* app = GUI.getApplication ())
			title = app->getApplicationTitle ();

		ASSERT (view != nullptr)
		window = TransparentWindow::create (view ? view->getWindow () : nullptr, keepOnTop () ? TransparentWindow::kKeepOnTop : 0, title);
	}
	return *window;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FloatingSprite::removeWindow ()
{
	safe_release (window);
	oldSize.setReallyEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FloatingSprite::updateWindow (bool force)
{
	ASSERT (view != nullptr && drawable != nullptr)

	if(!view->hasBeenDrawn ()) // don't do anything before owning view is drawn the first time
		return;
	
	Rect visibleClient;
	view->getVisibleClient (visibleClient);
	Rect visibleSize (size);
	if(keepOnTop () == false)
		visibleSize.bound (visibleClient);

	Point screenOffset;
	view->clientToScreen (screenOffset);
	Rect windowSize (visibleSize);
	windowSize.offset (screenOffset);

	Point position (windowSize.getLeftTop ());
 	Coord width = windowSize.getWidth ();
	Coord height = windowSize.getHeight ();

	bool windowVisible = visible ();
	if(windowSize.isEmpty ())
		windowVisible = false;

	bool wasVisible = window && window->isVisible ();

	bool toggled = wasVisible != windowVisible;
	bool moved = position != oldSize.getLeftTop ();
	bool sized = force || width != oldSize.getWidth () || height != oldSize.getHeight ();
	if(!(toggled || moved || sized))
		return;

	oldSize = windowSize;

	if(windowVisible)
	{
		if(sized)
		{
			// try to access bitmap data directly for per-pixel alpha
			Rect sourceRect;
			Bitmap* sourceBitmap = nullptr;
			if(UnknownPtr<IImageDrawable> imageDrawable = drawable)
				sourceBitmap = Bitmap::getOriginalBitmap (sourceRect, unknown_cast<Image> (imageDrawable->getImage ()));

			if(sourceBitmap)
			{
				if(sourceRect.getSize () == windowSize.getSize ())
					getTransparentWindow ().update (windowSize, *sourceBitmap, sourceRect.getLeftTop (), drawable->getOpacity ());
				else
				{
					// TransparentWindow::update copies pixels without stretching (in logical coordinate space):
					// if the source size does not match the window size, we do the stretching in an offscreen before
					Bitmap& offscreen = getOffscreen (width, height);
					{
						BitmapGraphicsDevice graphics (&offscreen);
						sourceBitmap->draw (graphics, sourceRect, Rect (0, 0, width, height));
					}
					getTransparentWindow ().update (windowSize, offscreen, Point (), drawable->getOpacity ());
				}
			}
			else
			{
				float contentScaleFactor = getTransparentWindow ().getContentScaleFactor ();
				if(!DpiScale::isIntAligned (contentScaleFactor))
				{
					// might need to add one pixel to compensate the truncation of the fractional part
					PixelPointF pixelSizeF (Point (width, height), contentScaleFactor);
					if(!DpiScale::isIntAligned (pixelSizeF.x))
						width++;
					if(!DpiScale::isIntAligned (pixelSizeF.y))
						height++;
				}
				Bitmap& offscreen = getOffscreen (width, height);

				{
					BitmapGraphicsDevice graphics (&offscreen);
					Rect paintRect (0, 0, width, height);
					graphics.clearRect (paintRect);
					
					Point viewOffset = screenOffset - position; // offset from sprite window coords to view coords
					Point clipOffset = visibleSize.getLeftTop () - size.getLeftTop (); // unclipped sprite pos to clipped pos (sprite window pos)

					// draw in coordinate space of view
					graphics.setOrigin (viewOffset);

					// update rect (sprite window area) in view coords
					paintRect.offset (size.getLeftTop () + clipOffset);
					paintRect.expand (1); // ?

					CCL_PRINTF ("sprite (%d, %d, %d, %d)  window (%d, %d, %d, %d) viewOffset %d, clipOffset %d\n", size.left, size.top, size.getWidth (), size.getHeight (), position.x, position.y, width, height, viewOffset.x, clipOffset.x)
					UpdateRgn updateRgn (paintRect);
					drawable->draw (IDrawable::DrawArgs (graphics, size, updateRgn));
				}

				getTransparentWindow ().update (windowSize, offscreen, Point (), drawable->getOpacity ());
			}
		}
		else if(moved)
			getTransparentWindow ().move (position);
	}

	if(toggled)
	{
		if(windowVisible)
			getTransparentWindow ().show ();
		else
			getTransparentWindow ().hide ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FloatingSprite::update (UpdateReason reason)
{
	switch(reason)
	{
	case kOnHide :
		removeWindow ();
		break;
	
	case kOnMove :
		if(window != nullptr)
			updateWindow ();
		break;
	
	case kOnRefresh:
		if(isVisible ())
			updateWindow (true);
		break;
	
	default :
		updateWindow (false);
	}
}

//************************************************************************************************
// SublayerSprite
//************************************************************************************************

DEFINE_CLASS (SublayerSprite, ManagedSprite)
DEFINE_CLASS_UID (SublayerSprite, 0x0876288A, 0xBEB8, 0xF243, 0x94, 0xDF, 0x63, 0xD7, 0xC7, 0xCB, 0x68, 0xD1)

//////////////////////////////////////////////////////////////////////////////////////////////////

SublayerSprite::SublayerSprite (View* _view, IDrawable* _drawable, const Rect& _size, int _options)
: ManagedSprite (_view, _drawable, _size, _options)
{
	oldLayerRect.setReallyEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SublayerSprite::~SublayerSprite ()
{
	ASSERT (subLayer == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SublayerSprite::drawLayer (IGraphics& graphics, const UpdateRgn& updateRgn, PointRef offset)
{
	if(drawable)
	{
		// calc layerRect as in update ()
		Point parentLayerOffset;
		getParentLayer (parentLayerOffset);

		Rect layerRect (size);
		Rect visibleClient;
		view->getVisibleClient (visibleClient);
		layerRect.bound (visibleClient);

		Point clipOffset = layerRect.getLeftTop () - size.getLeftTop (); // unclipped sprite pos to clipped pos
		layerRect.offset (parentLayerOffset);

		Point viewOffset = parentLayerOffset - layerRect.getLeftTop (); // offset from subLayer coords to view coords

		// update rect (subLayer area) in view coords
		Rect paintRect (0, 0, layerRect.getWidth (), layerRect.getHeight ());
		paintRect.offset (size.getLeftTop () + clipOffset);

		// draw in coordinate space of view
		Point totalOffset (offset);
		totalOffset += viewOffset;
		TransformSetter t (graphics, Transform ().translate ((float)totalOffset.x, (float)totalOffset.y));

		CCL_PRINTF ("sprite (%d, %d, %d, %d)  layer (%d, %d, %d, %d) viewOffset %d, clipOffset %d\n", size.left, size.top, size.getWidth (), size.getHeight (), layerRect.left, layerRect.top, layerRect.getWidth (), layerRect.getHeight (), viewOffset.x, clipOffset.x)

		drawable->draw (IDrawable::DrawArgs (graphics, size, UpdateRgn (paintRect)));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayerContent::LayerHint CCL_API SublayerSprite::getLayerHint () const
{
	return kGraphicsContentHintDefault;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* SublayerSprite::getParentLayer (Point& offset) const
{
	if(view->getGraphicsLayer ())
		return view->getGraphicsLayer ();
	else
		return view->getParentLayer (offset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SublayerSprite::update (UpdateReason reason)
{
	Point parentLayerOffset;
	IGraphicsLayer* parentLayer = getParentLayer (parentLayerOffset);
	ASSERT (parentLayer != nullptr)
	if(parentLayer == nullptr)
		return;

	Rect layerRect;
	if(visible ())
	{
		layerRect = size;

		Rect visibleClient;
		view->getVisibleClient (visibleClient);
		layerRect.bound (visibleClient);

		layerRect.offset (parentLayerOffset);
	}
	else
		layerRect.setReallyEmpty ();

	if(layerRect != oldLayerRect || reason == kOnRefresh)
	{
		if(!layerRect.isEmpty ())
		{
			// show it
			if(subLayer == nullptr)
			{
				if(!view->hasBeenDrawn ())  // don't do anything before owning view is drawn the first time
					return;

				subLayer = NativeGraphicsEngine::instance ().createGraphicsLayer (ClassID::GraphicsLayer);
				ASSERT (subLayer != nullptr)

				Rect subRect (0, 0, layerRect.getWidth (), layerRect.getHeight ());
				float contentScaleFactor = view->getWindow ()->getContentScaleFactor ();
				subLayer->construct (this->asUnknown (), subRect, IGraphicsLayer::kClipToBounds, contentScaleFactor);
				subLayer->setOpacity (drawable->getOpacity ());
				subLayer->setOffset (layerRect.getLeftTop ());
				parentLayer->addSublayer (subLayer);

				parentLayer->flush ();
			}
			else
			{
				// size it
				bool sized = layerRect.getWidth () != oldLayerRect.getWidth () || layerRect.getHeight () != oldLayerRect.getHeight ();
				if(sized)
					subLayer->setSize (layerRect.getWidth (), layerRect.getHeight ());

				// move it
				if(layerRect.getLeftTop () != oldLayerRect.getLeftTop ())
					subLayer->setOffset (layerRect.getLeftTop ());

				// update it
				if(sized || reason == kOnRefresh)
					subLayer->setUpdateNeeded ();
			}
		}
		else
		{
			// hide it
			if(subLayer)
			{
				parentLayer->removeSublayer (subLayer);
				subLayer = nullptr;

				//parentLayer->flush ();
			}
		}

		oldLayerRect = layerRect;
	}
}
