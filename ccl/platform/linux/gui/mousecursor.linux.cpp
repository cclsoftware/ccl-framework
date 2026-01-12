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
// Filename    : ccl/platform/linux/gui/mousecursor.linux.cpp
// Description : Platform-specific Cursor implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/linux/wayland/waylandbuffer.h"
#include "ccl/platform/linux/wayland/inputhandler.h"

#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/bitmapfilter.h"
#include "ccl/gui/graphics/imaging/bitmappainter.h"
#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/theme/theme.h"

#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/gui/framework/themeelements.h"
#include "ccl/public/systemservices.h"

#include <wayland-cursor.h>

namespace CCL {

//************************************************************************************************
// LinuxMouseCursor
//************************************************************************************************

class LinuxMouseCursor: public MouseCursor,
						public IdleClient,
						public Linux::WaylandObject
{
public:
	LinuxMouseCursor (wl_cursor* cursor, bool ownCursor, int x = 0, int y = 0);
	~LinuxMouseCursor ();

	// MouseCursor
	void makeCurrent () override;
	
	// IdleClient
	void CCL_API onTimer (ITimer*) override;
	
	// WaylandObject
	void onCompositorDisconnected () override;
	void onCompositorConnected () override;
	
	CLASS_INTERFACE (ITimerTask, MouseCursor)
	
protected:
	static LinuxMouseCursor* currentCursor;
	
	wl_cursor* cursor;
	int currentImageIndex;
	wl_surface* surface;
	int x;
	int y;
	
	virtual void updateCursorImage ();
};

//************************************************************************************************
// LinuxSystemCursor
//************************************************************************************************

class LinuxThemeCursor: public LinuxMouseCursor
{
public:
	LinuxThemeCursor (int themeCursorId, wl_cursor* cursor);
	~LinuxThemeCursor ();
	
	// LinuxMouseCursor
	void onCompositorConnected () override;
	
protected:
	int themeCursorId;
};

//************************************************************************************************
// LinuxBitmapCursor
//************************************************************************************************

class LinuxBitmapCursor: public LinuxMouseCursor
{
public:
	LinuxBitmapCursor (Image& image, int x = 0, int y = 0);
	
protected:
	SharedPtr<Image> image;
	Linux::WaylandBuffer buffer;
	
	// LinuxMouseCursor
	void updateCursorImage () override;
};

//************************************************************************************************
// LinuxCursorFactory
//************************************************************************************************

class LinuxCursorFactory: public MouseCursorFactory,
						  public Linux::WaylandObject
{
public:
	LinuxCursorFactory ();
	~LinuxCursorFactory ();
	
	wl_cursor* createThemeCursor (int themeCursorId);
	void unloadTheme ();
	
	// MouseCursorFactory
	MouseCursor* createCursor (int themeCursorId) override;
	MouseCursor* createCursor (Image& image, PointRef hotspot) override;
	
	// WaylandObject
	void onCompositorDisconnected () override;
	
protected:
	wl_cursor_theme* theme;
	int useCount;
};

} // namespace CCL

using namespace CCL;
using namespace Linux;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (LinuxMouseCursor, kFrameworkLevelFirst)
{
	static LinuxCursorFactory theFactory;
	MouseCursor::setFactory (&theFactory);
	return true;
}

//************************************************************************************************
// LinuxCursorFactory
//************************************************************************************************

LinuxCursorFactory::LinuxCursorFactory ()
: theme (nullptr),
  useCount (0)
{
	WaylandClient::instance ().registerObject (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxCursorFactory::~LinuxCursorFactory ()
{	
	WaylandClient::instance ().unregisterObject (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
MouseCursor* LinuxCursorFactory::createCursor (Image& image, PointRef hotspot)
{
	return NEW LinuxBitmapCursor (image, hotspot.x, hotspot.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxCursorFactory::unloadTheme ()
{
	ASSERT (useCount > 0)
	useCount--;
	if(useCount == 0 && theme != nullptr && WaylandClient::instance ().isInitialized ())
	{
		wl_cursor_theme_destroy (theme);
		theme = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

wl_cursor* LinuxCursorFactory::createThemeCursor (int themeCursorId)
{
	wl_shm* sharedMemory = WaylandClient::instance ().getSharedMemory ();
	if(sharedMemory == nullptr)
		return nullptr;
	
	const char* themeName = ::getenv ("XCURSOR_THEME");
	const char* cursorSize = ::getenv ("XCURSOR_SIZE");
	
	String cursorSizeString;
	if(cursorSize != nullptr)
		cursorSizeString.appendCString (Text::kSystemEncoding, cursorSize);
	int size = 0;
	if(!cursorSizeString.getIntValue (size) || size <= 0)
		size = 32;
	
	if(theme == nullptr)
		theme = wl_cursor_theme_load (themeName, size, sharedMemory);
	
	ASSERT (theme)
	if(theme == nullptr)
		return nullptr;
	
	wl_cursor* nativeCursor = nullptr;
	
	// https://www.freedesktop.org/wiki/Specifications/cursor-spec/
	// https://bbs.archlinux.org/viewtopic.php?id=59039
	// https://www.w3.org/TR/css-ui-3/#cursor

	switch(themeCursorId)
	{
	case ThemeElements::kArrowCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "default");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "left_ptr");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "X_cursor");
		break;
	case ThemeElements::kWaitCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "wait");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "watch");
		break;
	case ThemeElements::kCrosshairCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "crosshair");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "cross");
		break;
	case ThemeElements::kPointhandCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "pointer");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "hand");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "hand1");
		break;
	case ThemeElements::kSizeHorizontalCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "col-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "ew-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "h_double_arrow");
		break;
	case ThemeElements::kSizeVerticalCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "row-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "ns-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "v_double_arrow");
		break;
	case ThemeElements::kSizeLeftCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "w-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "left_side");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "ew-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "h_double_arrow");
		break;
	case ThemeElements::kSizeRightCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "e-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "right_side");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "ew-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "h_double_arrow");
		break;
	case ThemeElements::kSizeUpCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "n-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "top_side");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "ns-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "v_double_arrow");
		break;
	case ThemeElements::kSizeDownCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "s-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "bottom_side");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "ns-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "v_double_arrow");
		break;
	case ThemeElements::kSizeLeftUpCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "top_left_corner");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "nw-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "nwse-resize");
		break;
	case ThemeElements::kSizeLeftDownCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "bottom_left_corner");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "sw-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "nesw-resize");
		break;
	case ThemeElements::kSizeRightUpCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "top_right_corner");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "ne-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "nesw-resize");
		break;
	case ThemeElements::kSizeRightDownCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "bottom_right_corner");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "se-resize");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "nwse-resize");
		break;
	case ThemeElements::kSizeLeftUpRightDownCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "nwse-resize");
		break;
	case ThemeElements::kSizeLeftDownRightUpCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "nesw-resize");
		break;
	case ThemeElements::kTextCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "ibeam");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "text");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "xterm");
		break;
	case ThemeElements::kCopyCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "dnd-copy");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "copy");
		break;
	case ThemeElements::kGrabCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "grab");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "openhand");
		break;
	case ThemeElements::kGrabbingCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "dnd-move");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "grabbing");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "closedhand");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "move");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "alias");
		break;
	case ThemeElements::kNoDropCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "dnd-no-drop");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "no-drop");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "not-allowed");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "forbidden");
		if(nativeCursor == nullptr)
			nativeCursor = wl_cursor_theme_get_cursor (theme, "crossed_circle");
		break;
	case ThemeElements::kZoomInCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "zoom-in");
		break;
	case ThemeElements::kZoomOutCursor :
		nativeCursor = wl_cursor_theme_get_cursor (theme, "zoom-out");
		break;
	}
	
	return nativeCursor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseCursor* LinuxCursorFactory::createCursor (int themeCursorId)
{
	ASSERT (themeCursorId >= 0 && themeCursorId < ThemeElements::kNumCursors)

	wl_cursor* nativeCursor = createThemeCursor (themeCursorId);
	SOFT_ASSERT (nativeCursor, "Failed to load cursor")
	
	if(nativeCursor == nullptr || nativeCursor->image_count == 0)
	{
		if(themeCursorId == ThemeElements::kArrowCursor)
			return nullptr;
		else
			return createCursor (ThemeElements::kArrowCursor);
	}
	
	useCount++;
	return NEW LinuxThemeCursor (themeCursorId, nativeCursor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxCursorFactory::onCompositorDisconnected ()
{
	theme = nullptr;
}

//************************************************************************************************
// LinuxMouseCursor
//************************************************************************************************

LinuxMouseCursor* LinuxMouseCursor::currentCursor = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxMouseCursor::LinuxMouseCursor (wl_cursor* cursor, bool ownCursor, int x, int y)
: MouseCursor (ownCursor),
  cursor (cursor),
  currentImageIndex (-1),
  surface (nullptr),
  x (x),
  y (y)
{
	WaylandClient& client = WaylandClient::instance ();
	client.registerObject (*this);
	if(client.getCompositor ())
		surface = wl_compositor_create_surface (client.getCompositor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxMouseCursor::~LinuxMouseCursor ()
{
	wl_pointer* pointer = InputHandler::instance ().getPointer ();
	
	WaylandClient& client = WaylandClient::instance ();
	client.unregisterObject (*this);
	if(pointer && client.getCompositor ())
	{
		if(currentCursor == this)
		{
			uint32_t serial = WaylandClient::instance ().getSerial ();
			wl_pointer_set_cursor (pointer, serial, nullptr, 0, 0);
		}
		
		if(surface && WaylandClient::instance ().isInitialized ())
			wl_surface_destroy (surface);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxMouseCursor::makeCurrent ()
{
	wl_pointer* pointer = InputHandler::instance ().getPointer ();
	if(pointer == nullptr)
		return;
	
	if(currentCursor != this)
	{
		currentCursor = this;
		updateCursorImage ();
	}
	
	wl_pointer_set_cursor (pointer, WaylandClient::instance ().getEnterSerial (), surface, x, y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxMouseCursor::onTimer (ITimer*)
{
	updateCursorImage ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxMouseCursor::updateCursorImage ()
{
	 if(!WaylandClient::instance ().isInitialized ())
		return;
	
	if(currentCursor != this)
	{
		stopTimer ();
		return;
	}
	
	ASSERT (cursor != nullptr)
	if(cursor == nullptr)
		return;
	
	wl_cursor_image* cursorImage = nullptr;
	
	if(cursor->image_count > 1)
	{
		currentImageIndex = (currentImageIndex + 1) % cursor->image_count;
		cursorImage = cursor->images[currentImageIndex];
		ASSERT (cursorImage != nullptr)
		startTimer (cursorImage->delay, false);
	}
	else
		cursorImage = cursor->images[0];
	
	x = cursorImage->hotspot_x;
	y = cursorImage->hotspot_y;
	
	wl_buffer* buffer = wl_cursor_image_get_buffer (cursorImage);
	if(buffer != nullptr)
	{
		wl_surface_attach (surface, buffer, 0, 0);
		wl_surface_damage_buffer (surface, 0, 0, cursorImage->width, cursorImage->height);
		wl_surface_commit (surface);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxMouseCursor::onCompositorDisconnected ()
{
	surface = nullptr;
	cursor = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxMouseCursor::onCompositorConnected ()
{
	WaylandClient& client = WaylandClient::instance ();
	if(client.getCompositor ())
		surface = wl_compositor_create_surface (client.getCompositor ());
}

//************************************************************************************************
// LinuxThemeCursor
//************************************************************************************************

LinuxThemeCursor::LinuxThemeCursor (int themeCursorId, wl_cursor* cursor)
: LinuxMouseCursor (cursor, false),
  themeCursorId (themeCursorId)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxThemeCursor::~LinuxThemeCursor ()
{
	static_cast<LinuxCursorFactory*> (factory)->unloadTheme ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxThemeCursor::onCompositorConnected ()
{
	LinuxMouseCursor::onCompositorConnected ();
	if(cursor == nullptr)
		cursor = static_cast<LinuxCursorFactory*> (factory)->createThemeCursor (themeCursorId);
}

//************************************************************************************************
// LinuxBitmapCursor
//************************************************************************************************

LinuxBitmapCursor::LinuxBitmapCursor (Image& image, int x, int y)
: LinuxMouseCursor (nullptr, true, x, y),
  image (&image)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxBitmapCursor::updateCursorImage ()
{
	 if(!WaylandClient::instance ().isInitialized ())
		return;
	
	if(currentCursor != this)
	{
		stopTimer ();
		return;
	}
	
	if(image == nullptr)
		return;
	
	if(image->getFrameCount () > 1)
	{
		currentImageIndex = (currentImageIndex + 1) % image->getFrameCount ();
		if(!isTimerEnabled ())
			startTimer ();
	}
	else
		currentImageIndex = 0;
	
	image->setCurrentFrame (currentImageIndex);
	
	BitmapProcessor processor;
	const Point size (image->getSize ());
	processor.setup (image, Colors::kWhite, 0, &size);
	BitmapFilterList copyFilter; // no filtering, just copy
	processor.process (static_cast<IBitmapFilterList&> (copyFilter));
	CCL::Bitmap* bitmap = unknown_cast<Bitmap> (processor.getOutput ());
	
	ASSERT (bitmap != nullptr)
	if(bitmap)
	{
		buffer.fromBitmap (*bitmap);
		buffer.attach (surface);
	}
}
