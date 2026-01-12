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
// Filename    : ccl/platform/win/gui/mousecursor.win.cpp
// Description : Platform-specific Cursor implementation
//
//************************************************************************************************

#include "ccl/gui/system/mousecursor.h"

#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/imaging/multiimage.h"

#include "ccl/public/gui/framework/themeelements.h"

#include "ccl/platform/win/interfaces/iwin32graphics.h"
#include "ccl/platform/win/gui/dpihelper.h"
#include "ccl/platform/win/gui/screenscaling.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// PlatformCursor
//************************************************************************************************

struct PlatformCursor
{
	HCURSOR normalCursor;
	HCURSOR hiResCursor;

	PlatformCursor (HCURSOR normalCursor = NULL, HCURSOR hiResCursor = NULL)
	: normalCursor (normalCursor),
	  hiResCursor (hiResCursor)
	{}

	bool isValid () const
	{
		return normalCursor || hiResCursor;
	}

	HCURSOR selectCursor (float dpiFactor) const
	{
		if(hiResCursor && Bitmap::isHighResolutionScaling (dpiFactor))
			return hiResCursor;
		else
			return normalCursor;
	}

	void release (bool ownCursor)
	{
		if(ownCursor)
		{
			if(normalCursor)
				::DestroyCursor (normalCursor);
			if(hiResCursor)
				::DestroyCursor (hiResCursor);
		}

		delete this;
	}
};

} // namespace Win32
} // namespace CCL

namespace CCL {

//************************************************************************************************
// WindowsMouseCursor
//************************************************************************************************

class WindowsMouseCursor: public MouseCursor
{
public:
	WindowsMouseCursor (Win32::PlatformCursor* nativeCursor, bool ownCursor);
	~WindowsMouseCursor ();

	// MouseCursor
	void makeCurrent () override;

private:
	Win32::PlatformCursor* nativeCursor;
};

//************************************************************************************************
// WindowsCursorFactory
//************************************************************************************************

class WindowsCursorFactory: public MouseCursorFactory
{
public:
	// MouseCursorFactory
	MouseCursor* createCursor (int themeCursorId) override;
	MouseCursor* createCursor (Image& image, PointRef hotspot) override;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

HICON CreateIconIndirectFromImage (Image& image, const Point& _hotspot, const Point& sizeInPixel, float scaleFactor, BOOL isIcon)
{
	Point sizeInPoint;
	sizeInPoint.x = DpiScale::pixelToCoord (sizeInPixel.x, scaleFactor);
	sizeInPoint.y = DpiScale::pixelToCoord (sizeInPixel.y, scaleFactor);

	AutoPtr<Bitmap> tempBitmap = NEW Bitmap (sizeInPoint.x, sizeInPoint.y, Bitmap::kRGBAlpha, scaleFactor);
	{
		BitmapGraphicsDevice device (tempBitmap);
		if(device.isNullDevice ()) // give caller a chance for additional error handling
			return NULL;
		ImageResolutionSelector::draw (device, &image, Rect (sizeInPoint));
	}

	Point hotspot (_hotspot);
	if(!isIcon) // cursor image is centered, hotspot needs to be corrected
	{
		Point hotspotOffset;
		hotspotOffset.x = (sizeInPoint.x - image.getWidth ()) / 2;
		hotspotOffset.y = (sizeInPoint.y - image.getHeight ()) / 2;
		hotspot.offset (hotspotOffset);
	}

	UnknownPtr<Win32::IWin32Bitmap> gdiBitmap = ccl_as_unknown (tempBitmap->getNativeBitmap ());
	ASSERT (gdiBitmap != nullptr)
	if(!gdiBitmap)
		return nullptr;

	HBITMAP hbmColor = gdiBitmap->detachHBITMAP ();
	if(!hbmColor)
		return nullptr;

	Point hotspotInPixel;
	hotspotInPixel.x = DpiScale::coordToPixel (hotspot.x, scaleFactor);
	hotspotInPixel.y = DpiScale::coordToPixel (hotspot.y, scaleFactor);

	ICONINFO iconInfo = {0};
	iconInfo.fIcon = isIcon;
	iconInfo.xHotspot = hotspotInPixel.x;
	iconInfo.yHotspot = hotspotInPixel.y;
	iconInfo.hbmMask = ::CreateBitmap (sizeInPixel.x, sizeInPixel.y, 1, 1, nullptr);
	iconInfo.hbmColor = hbmColor;

	HICON hIcon = ::CreateIconIndirect (&iconInfo);
	ASSERT (hIcon != nullptr);

	::DeleteObject (iconInfo.hbmMask);
	::DeleteObject (iconInfo.hbmColor);

	return hIcon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (WindowsMouseCursor, kFrameworkLevelFirst)
{
	static WindowsCursorFactory theFactory;
	MouseCursor::setFactory (&theFactory);
	return true;
}

//************************************************************************************************
// WindowsCursorFactory
//************************************************************************************************

MouseCursor* WindowsCursorFactory::createCursor (Image& image, PointRef hotspot)
{
	// Note: Windows creates cursors of a certain size only!
	int cursorW = ::GetSystemMetrics (SM_CXCURSOR);
	int cursorH = ::GetSystemMetrics (SM_CYCURSOR);
	float scaleFactor = (float)cursorW / 32.f; // 32px on standard low-dpi systems

	// create multiple representations for per-monitor DPI
	Win32::PlatformCursor* platformCursor = NEW Win32::PlatformCursor;
	if(Win32::gDpiInfo.isPerMonitorDpi () && Bitmap::isHighResolutionScaling (scaleFactor))
	{
		platformCursor->normalCursor = CreateIconIndirectFromImage (image, hotspot, Point (cursorW, cursorH), 1.f, FALSE);
		platformCursor->hiResCursor = CreateIconIndirectFromImage (image, hotspot, Point (cursorW, cursorH), scaleFactor, FALSE);
	}
	else
	{
		platformCursor->normalCursor = CreateIconIndirectFromImage (image, hotspot, Point (cursorW, cursorH), scaleFactor, FALSE);
	}

	if(platformCursor->isValid ())
		return NEW WindowsMouseCursor (platformCursor, true);
	else
	{
		platformCursor->release (true);
		return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseCursor* WindowsCursorFactory::createCursor (int themeCursorId)
{
	ASSERT (themeCursorId >= 0 && themeCursorId < ThemeElements::kNumCursors)

	const uchar* platformId = nullptr;
	switch(themeCursorId)
	{
	case ThemeElements::kArrowCursor :
		platformId = IDC_ARROW;
		break;
	case ThemeElements::kWaitCursor :
		platformId = IDC_WAIT;
		break;
	case ThemeElements::kCrosshairCursor :
		platformId = IDC_CROSS;
		break;
	case ThemeElements::kPointhandCursor :
		platformId = IDC_HAND;
		break;
	case ThemeElements::kSizeHorizontalCursor :
	case ThemeElements::kSizeLeftCursor :
	case ThemeElements::kSizeRightCursor :
		platformId = IDC_SIZEWE;
		break;
	case ThemeElements::kSizeVerticalCursor :
	case ThemeElements::kSizeUpCursor :
	case ThemeElements::kSizeDownCursor :
		platformId = IDC_SIZENS;
		break;
	case ThemeElements::kSizeLeftUpRightDownCursor :
	case ThemeElements::kSizeLeftUpCursor :
	case ThemeElements::kSizeRightDownCursor :
		platformId = IDC_SIZENWSE;
		break;
	case ThemeElements::kSizeLeftDownRightUpCursor :
	case ThemeElements::kSizeLeftDownCursor :
	case ThemeElements::kSizeRightUpCursor :
		platformId = IDC_SIZENESW;
		break;
	case ThemeElements::kTextCursor :
		platformId = IDC_IBEAM;
		break;
	case ThemeElements::kNoDropCursor :
		platformId = IDC_NO;
		break;
	}

	if(platformId == nullptr)
		return nullptr;

	Win32::PlatformCursor* nativeCursor = NEW Win32::PlatformCursor (::LoadCursor (nullptr, platformId));
	return NEW WindowsMouseCursor (nativeCursor, false);
}

//************************************************************************************************
// WindowsMouseCursor
//************************************************************************************************

WindowsMouseCursor::WindowsMouseCursor (Win32::PlatformCursor* nativeCursor, bool ownCursor)
: MouseCursor (ownCursor),
  nativeCursor (nativeCursor)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsMouseCursor::~WindowsMouseCursor ()
{
	if(nativeCursor)
		nativeCursor->release (ownCursor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsMouseCursor::makeCurrent ()
{
	HCURSOR hCursor = NULL;
	if(nativeCursor)
	{
		// quick fix: check DPI of monitor under mouse
		if(Win32::gDpiInfo.isPerMonitorDpi ())
		{
			POINT p = {0};
			::GetCursorPos (&p);
			const Win32::ScreenInformation& screen = Win32::gScreens.screenForPixel (Point (p.x, p.y));
			hCursor = nativeCursor->selectCursor (screen.scaleFactor);
		}
		else
			hCursor = nativeCursor->normalCursor;
	}

	if(!hCursor)
		hCursor = ::LoadCursor (nullptr, IDC_ARROW);
	::SetCursor (hCursor);
}
