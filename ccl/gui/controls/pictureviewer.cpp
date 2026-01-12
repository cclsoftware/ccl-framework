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
// Filename    : ccl/gui/controls/pictureviewer.cpp
// Description : Picture Viewer
//
//************************************************************************************************

#include "ccl/gui/controls/pictureviewer.h"

#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/dialogs/dialogbuilder.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/multiimage.h"

#include "ccl/gui/theme/thememanager.h"

using namespace CCL;

//************************************************************************************************
// PictureViewer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PictureViewer, ImageView)

//////////////////////////////////////////////////////////////////////////////////////////////////

PictureViewer::PictureViewer (IImage* background, const Rect& size, StyleRef style, StringRef title)
: ImageView (background, size, style, title),
  hasMagnifier (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* PictureViewer::getLargestFrame () const
{
	if(MultiImage* multiImage = unknown_cast<MultiImage> (background))
	{
		Image* largest = nullptr;
		for(int i = 0; i < multiImage->getFrameCount (); i++)
			if(Image* frame = multiImage->getFrame (i))
			{
				if(largest == nullptr)
					largest = frame;
				else if(frame->getWidth () > largest->getWidth ())
					largest = frame;
			}

		if(largest)
			return largest;
	}
	return background;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PictureViewer::canPopup () const
{
	IImage* image = getLargestFrame ();
	if(image == nullptr)
		return false;

	Coord w = image->getWidth ();
	Coord h = image->getHeight ();
	bool larger = w > getWidth () || h > getHeight ();

	return larger;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PictureViewer::popupPicture ()
{
	IImage* image = getLargestFrame ();
	ASSERT (image != nullptr)

	Rect rect (0, 0, image->getWidth (), image->getHeight ());
	Rect maxRect (0, 0, 1024, 1024);
	if(rect.getWidth () > maxRect.getWidth () || rect.getHeight () > maxRect.getHeight ())
		rect.fitProportionally (maxRect);

	DialogBuilder builder;
	builder.setTheme (getTheme ());

	ImageView* view = NEW ImageView (image, rect, StyleFlags (0, Styles::kImageViewBehaviorWindowMovable|Styles::kImageViewAppearanceHighQuality));
	view->setName (CCLSTR ("PictureViewer"));
	builder.runDialog (view, StyleFlags (0, Styles::kWindowBehaviorRestoreCenter|Styles::kWindowBehaviorCenter), Styles::kCloseButton);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseCursor* PictureViewer::getMagnifierCursor () const
{
	IMouseCursor* cursor = getTheme ().getCursor ("MagnifierCursor");
	if(cursor == nullptr)
	{
		// try application theme
		if(ITheme* appTheme = ThemeManager::instance ().getApplicationTheme ())
			if(&getTheme () != appTheme)
				cursor = appTheme->getCursor ("MagnifierCursor");

		// fallback to system cursor
		// TODO: check if platforms provide a magnifier...
		if(cursor == nullptr)
			cursor = getTheme ().getThemeCursor (ThemeElements::kPointhandCursor);
	}
	return unknown_cast<MouseCursor> (cursor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PictureViewer::updateCursor (const Point& where)
{
	// don't show magnifier when over other view (could be a sibling of any parent, etc.)
	View* viewUnderMouse = nullptr;
	if(Window* window = getWindow ())
	{
		Point p (where);
		clientToWindow (p);
		viewUnderMouse = window->findView (p, true);
		if(ccl_strict_cast<ImageView> (viewUnderMouse)) // ignore decorations (ImageViews) 
			viewUnderMouse = this;
	}

	bool showMagnifier = viewUnderMouse == this;
	if(showMagnifier != hasMagnifier)
	{
		if(showMagnifier)
		{
			setCursor (getMagnifierCursor ());
			hasMagnifier = true;
		}
		else
			resetCursor ();		
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PictureViewer::resetCursor (bool mouseLeaving)
{
	if(mouseLeaving == false)
		setCursor ((MouseCursor*)nullptr);

	hasMagnifier = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PictureViewer::onMouseEnter (const MouseEvent& event)
{
	if(canPopup ())
	{
		updateCursor (event.where);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PictureViewer::onMouseMove  (const MouseEvent& event)
{
	updateCursor (event.where);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PictureViewer::onMouseLeave (const MouseEvent& event)
{
	resetCursor (true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PictureViewer::onMouseDown (const MouseEvent& event)
{
	if(SuperClass::onMouseDown (event))
		return true;

	if(background && detectDrag (event))
	{
		AutoPtr<DragSession> session (DragSession::create (this->asUnknown ()));
		Color backColor (getVisualStyle ().getBackColor ());

		// scale down drag image
		const Coord kMaxSize = 80;
		if(background->getWidth () > kMaxSize || background->getHeight () > kMaxSize)
		{
			AutoPtr<Bitmap> bitmap = NEW Bitmap (kMaxSize, kMaxSize);
			{
				BitmapGraphicsDevice device (bitmap);
				Rect r (0, 0, kMaxSize, kMaxSize);
				device.fillRect (r, SolidBrush (backColor));
				ImageResolutionSelector::draw (device, background, r);
			}
			session->setDragImage (bitmap, backColor);
		}
		else
			session->setDragImage (unknown_cast<Image> (background), backColor);

		resetCursor ();
		session->getItems ().add (background, true);
		session->drag ();
		return true;
	}
	else if(canPopup ())
	{
		popupPicture ();
		return true;
	}

	return false;
}

