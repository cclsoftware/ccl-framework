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
// Filename    : ccl/platform/wasm/gui/mousecursor.wasm.cpp
// Description : Platform-specific Cursor implementation
//
//************************************************************************************************

#include "ccl/platform/wasm/gui/window.wasm.h"

#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/theme/theme.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/bitmapfilter.h"
#include "ccl/gui/graphics/imaging/bitmappainter.h"
#include "ccl/gui/graphics/graphicsdevice.h"

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, setCursor, (CStringPtr id, CStringPtr cursorId),
{
	var element = CCL.getFrameElement (UTF8ToString (id));
	if(element)
		element.style.cursor = UTF8ToString (cursorId);
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, setImageCursor, (CStringPtr id, const uint8* imageData, int width, int height, int hotspotX, int hotspotY),
{
	try
	{
		var canvas = CCL.createImageCanvas (imageData, width, height);
		var dataUrl = canvas.toDataURL ('image/png');
		var cursorStr = 'url(' + dataUrl + ') ' + hotspotX + ' ' + hotspotY + ', auto';

		var element = CCL.getFrameElement (UTF8ToString (id));
		if(element)
			element.style.cursor = cursorStr;
	}
	catch(e)
	{}
});

} // namespace Wasm

//************************************************************************************************
// WasmMouseCursor
//************************************************************************************************

class WasmMouseCursor: public MouseCursor
{
public:
	WasmMouseCursor (int themeCursorId);

	// MouseCursor
	void makeCurrent () override;

protected:
	int themeCursorId;
};

//************************************************************************************************
// WasmImageCursor
//************************************************************************************************

class WasmImageCursor: public MouseCursor
{
public:
	WasmImageCursor (Image& image, PointRef hotspot);

	// MouseCursor
	void makeCurrent () override;

protected:
	SharedPtr<Image> image;
	Point hotspot;
};

//************************************************************************************************
// WasmCursorFactory
//************************************************************************************************

class WasmCursorFactory: public MouseCursorFactory
{
public:
	// MouseCursorFactory
	MouseCursor* createCursor (int themeCursorId) override;
	MouseCursor* createCursor (Image& image, PointRef hotspot) override;
};

} // namespace CCL

using namespace CCL;
using namespace Wasm;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (LinuxMouseCursor, kFrameworkLevelFirst)
{
	static WasmCursorFactory theFactory;
	MouseCursor::setFactory (&theFactory);
	return true;
}

//************************************************************************************************
// WasmCursorFactory
//************************************************************************************************

MouseCursor* WasmCursorFactory::createCursor (int themeCursorId)
{
	return NEW WasmMouseCursor (themeCursorId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseCursor* WasmCursorFactory::createCursor (Image& image, PointRef hotspot)
{
	return NEW WasmImageCursor (image, hotspot);
}

//************************************************************************************************
// LinuxMouseCursor
//************************************************************************************************

WasmMouseCursor::WasmMouseCursor (int themeCursorId)
: themeCursorId (themeCursorId)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmMouseCursor::makeCurrent ()
{
	WasmWindow* mouseWindow = nullptr;
	IWindow* window = Desktop.findWindowUnderCursor ();

	if(window == nullptr)
		return;

	CStringPtr cursorId = CSTR ("auto");
	switch(themeCursorId)
	{
	case ThemeElements::kArrowCursor :
		cursorId = CSTR ("default");
		break;
	case ThemeElements::kWaitCursor :
		cursorId = CSTR ("wait");
		break;
	case ThemeElements::kCrosshairCursor :
		cursorId = CSTR ("crosshair");
		break;
	case ThemeElements::kPointhandCursor :
		cursorId = CSTR ("pointer");
		break;
	case ThemeElements::kSizeHorizontalCursor :
		cursorId = CSTR ("col-resize");
		break;
	case ThemeElements::kSizeVerticalCursor :
		cursorId = CSTR ("row-resize");
		break;
	case ThemeElements::kSizeLeftCursor :
		cursorId = CSTR ("w-resize");
		break;
	case ThemeElements::kSizeRightCursor :
		cursorId = CSTR ("e-resize");
		break;
	case ThemeElements::kSizeUpCursor :
		cursorId = CSTR ("n-resize");
		break;
	case ThemeElements::kSizeDownCursor :
		cursorId = CSTR ("s-resize");
		break;
	case ThemeElements::kSizeLeftUpCursor :
		cursorId = CSTR ("nw-resize");
		break;
	case ThemeElements::kSizeLeftDownCursor :
		cursorId = CSTR ("sw-resize");
		break;
	case ThemeElements::kSizeRightUpCursor :
		cursorId = CSTR ("ne-resize");
		break;
	case ThemeElements::kSizeRightDownCursor :
		cursorId = CSTR ("se-resize");
		break;
	case ThemeElements::kSizeLeftUpRightDownCursor :
		cursorId = CSTR ("nwse-resize");
		break;
	case ThemeElements::kSizeLeftDownRightUpCursor :
		cursorId = CSTR ("nesw-resize");
		break;
	case ThemeElements::kTextCursor :
		cursorId = CSTR ("text");
		break;
	case ThemeElements::kCopyCursor :
		cursorId = CSTR ("copy");
		break;
	case ThemeElements::kGrabCursor :
		cursorId = CSTR ("grab");
		break;
	case ThemeElements::kGrabbingCursor :
		cursorId = CSTR ("grabbing");
		break;
	case ThemeElements::kNoDropCursor :
		cursorId = CSTR ("no-drop");
		break;
	case ThemeElements::kZoomInCursor :
		cursorId = CSTR ("zoom-in");
		break;
	case ThemeElements::kZoomOutCursor :
		cursorId = CSTR ("zoom-out");
		break;
	}

	setCursor (reinterpret_cast<CStringPtr> (window->getSystemWindow ()), cursorId);
}

//************************************************************************************************
// WasmImageCursor
//************************************************************************************************

WasmImageCursor::WasmImageCursor (Image& image, PointRef hotspot)
: MouseCursor (true),
  image (&image),
  hotspot (hotspot)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmImageCursor::makeCurrent ()
{
	IWindow* window = Desktop.findWindowUnderCursor ();
	if(window == nullptr || image == nullptr)
		return;

	BitmapProcessor processor;
	const Point size (image->getSize ());
	processor.setup (image, Colors::kWhite, 0, &size);
	BitmapFilterList copyFilter;
	processor.process (static_cast<IBitmapFilterList&> (copyFilter));
	Bitmap* bitmap = unknown_cast<Bitmap> (processor.getOutput ());

	if(bitmap == nullptr)
		return;

	BitmapDataLocker locker (bitmap, IBitmap::kRGBAlpha, IBitmap::kLockRead);
	if(locker.result != kResultOk || locker.data.scan0 == nullptr)
		return;

	setImageCursor (reinterpret_cast<CStringPtr> (window->getSystemWindow ()),
		static_cast<const uint8*> (locker.data.scan0),
		locker.data.width, locker.data.height,
		hotspot.x, hotspot.y
	);
}

