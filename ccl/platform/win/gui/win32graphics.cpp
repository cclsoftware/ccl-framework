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
// Filename    : ccl/platform/win/gui/win32graphics.cpp
// Description : Win32 
//
//************************************************************************************************

#include "ccl/platform/win/gui/win32graphics.h"

#include "ccl/public/gui/graphics/dpiscale.h"

using namespace CCL;
using namespace Win32;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IWin32Graphics, 0x6009fee6, 0x5d13, 0x4e83, 0xb1, 0x6d, 0x8d, 0x44, 0xde, 0x8, 0x7c, 0x77)
DEFINE_IID_ (IWin32Bitmap, 0xb1438c5d, 0x600c, 0x4fd8, 0xba, 0xbb, 0xc6, 0x9a, 0x5f, 0xce, 0xa0, 0x62)

//************************************************************************************************
// GdiClipRegion::RectList
//************************************************************************************************

GdiClipRegion::RectList::RectList (const GdiClipRegion& region)
: rectCount (region.getRects (nullptr, 0))
{
	if(rectCount <= kMaxRectCount)
		rects = fixedRects;
	else
	{
		rectArray.resize (rectCount);
		rects = rectArray.getAddress ();
	}

	region.getRects (rects, rectCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GdiClipRegion::RectList::removeEmptyRects ()
{
	for(int i = rectCount - 1; i >= 0; i--)
	{
		if(rects[i].isEmpty ())
		{
			rectCount--;
			for(int j = i; j < rectCount; j++)
				rects[j] = rects[j+1];
		}	
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GdiClipRegion::RectList::adjustToCoords (float scaleFactor)
{
	if(scaleFactor != 1.f)
	{
		bool fractionalScaling = DpiScale::isIntAligned (scaleFactor) == false;
		for(int i = 0; i < rectCount; i++)
		{
			DpiScale::toCoordRect (rects[i], scaleFactor);
			if(fractionalScaling)
				rects[i].expand (1); 
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GdiClipRegion::RectList::adjustToPixels (float scaleFactor)
{
	if(scaleFactor != 1.f)
		for(int i = 0; i < rectCount; i++)
			DpiScale::toPixelRect (rects[i], scaleFactor);					
}

//************************************************************************************************
// GdiClipRegion
//************************************************************************************************

static inline HRGN CreateNullRegion ()
{
	//return ::CreateRectRgn (kMaxCoord, kMaxCoord, kMinCoord, kMinCoord);
	return ::CreateRectRgn (0, 0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GdiClipRegion::GdiClipRegion ()
: hdc (nullptr),
  hrgn (CreateNullRegion ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

GdiClipRegion::GdiClipRegion (HDC hdc)
: hdc (hdc),
  hrgn (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

GdiClipRegion::GdiClipRegion (HWND hwnd)
: hdc (nullptr),
  hrgn (::CreateRectRgn (0, 0, 0, 0))
{
	int result = ::GetUpdateRgn (hwnd, hrgn, FALSE);

	#if (0 && DEBUG)
	if(result == COMPLEXREGION)
		Debugger::println ("Complex update region!");

	// TEST:
	int numRects = getRects (0, 0);
	Rect rects[100];
	getRects (rects, 100);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GdiClipRegion::~GdiClipRegion ()
{
	if(hrgn)
		::DeleteObject (hrgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GdiClipRegion::rectVisible (RectRef rect) const
{
	RECT r;
	GdiInterop::toSystemRect (r, rect);
	if(hrgn)
		return (tbool)::RectInRegion (hrgn, &r);
	else
		return (tbool)::RectVisible (hdc, &r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GdiClipRegion::isEmpty () const
{
	Rect r;
	getBounds (r);
	return r.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GdiClipRegion::getBounds (Rect& rect) const
{
	RECT r = {0};
	::GetRgnBox (hrgn, &r);
	GdiInterop::fromSystemRect (rect, r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GdiClipRegion::addRegion (const GdiClipRegion& other)
{
	int result = ::CombineRgn (hrgn, hrgn, other.hrgn, RGN_OR);
	ASSERT (result != ERROR)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GdiClipRegion::addRectList (const RectList& rectList)
{
	for(int i = 0; i < rectList.rectCount; i++)
		addRect (rectList.rects[i]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GdiClipRegion::addRect (RectRef rect)
{
	HRGN temp = ::CreateRectRgn (rect.left, rect.top, rect.right, rect.bottom);
	int result = ::CombineRgn (hrgn, hrgn, temp, RGN_OR);
	ASSERT (result != ERROR)
	::DeleteObject (temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GdiClipRegion::setEmpty ()
{
	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect CCL_API GdiClipRegion::getBoundingBox () const
{
	Rect bounds;
	getBounds (bounds);
	return bounds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GdiClipRegion::removeAll ()
{
	::DeleteObject (hrgn);
	hrgn = CreateNullRegion ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int GdiClipRegion::getRects (Rect* rects, int count) const
{
	RGNDATA* regionData = nullptr;
	DWORD byteSize = ::GetRegionData (hrgn, 0, nullptr);

	char* dynamicBuffer = nullptr;
	char staticBuffer[20000];
	if(byteSize <= sizeof(staticBuffer))
		regionData = (RGNDATA*)staticBuffer;
	else
	{
		dynamicBuffer = NEW char[byteSize];
		regionData = (RGNDATA*)dynamicBuffer;
		ASSERT (regionData != nullptr)
		if(regionData == nullptr)
			return -1;
	}

	DWORD result = ::GetRegionData (hrgn, byteSize, regionData);
	ASSERT (result == byteSize)

	RGNDATAHEADER& header = regionData->rdh;
	RECT* rectList = (RECT*)&regionData->Buffer;
	ASSERT (header.iType == RDH_RECTANGLES)

	int numRects = 0;
	if(rects)
	{
		numRects = ccl_min<int> (header.nCount, count);
		for(int i = 0; i < numRects; i++)
			GdiInterop::fromSystemRect (rects[i], rectList[i]);
	}
	else
		numRects = header.nCount;

	if(dynamicBuffer)
		delete [] dynamicBuffer;

	return numRects;
}

//************************************************************************************************
// HBITMAP helper
//************************************************************************************************

HBITMAP Win32::CopyBitmapToDIBSection (HBITMAP bitmap)
{
	// determine source bitmap size + pixel format
	BITMAP bm = {0};
	::GetObject (bitmap, sizeof(BITMAP), &bm);
	int width = bm.bmWidth;
	int height = bm.bmHeight;
	bool hasSourceAlpha = bm.bmBitsPixel == 32;

	// create DIB section
	BITMAPINFO bmInfo = {0};
	BITMAPINFOHEADER& bmHeader = bmInfo.bmiHeader;
	bmHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmHeader.biWidth = width;
	bmHeader.biHeight = height;
	bmHeader.biPlanes = 1;
	bmHeader.biBitCount = hasSourceAlpha ? 32 : 24;
	bmHeader.biCompression = BI_RGB;
	bmHeader.biXPelsPerMeter = bmHeader.biYPelsPerMeter = 72;
	
	void* bitsCopy = nullptr;
	HBITMAP bitmapCopy = ::CreateDIBSection (nullptr, &bmInfo, DIB_RGB_COLORS, &bitsCopy, nullptr, 0);
	ASSERT (bitmapCopy != nullptr)

	// copy bitmap pixels
	HDC hdcSrc = ::CreateCompatibleDC (nullptr);
	HGDIOBJ hOldSrcBmp = ::SelectObject (hdcSrc, bitmap);
	
	HDC hdcDst = ::CreateCompatibleDC (nullptr);
	HGDIOBJ hOldDstBmp = ::SelectObject (hdcDst, bitmapCopy);

	::BitBlt (hdcDst, 0, 0, width, height, hdcSrc, 0, 0, SRCCOPY);

	::SelectObject (hdcSrc, hOldSrcBmp);
	::DeleteDC (hdcSrc);
	
	::SelectObject (hdcDst, hOldDstBmp);
	::DeleteDC (hdcDst);

	return bitmapCopy;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

HBITMAP Win32::CreateScreenshotFromHWND (HWND hwnd)
{
	RECT clientRect = {0};
	::GetClientRect (hwnd, &clientRect);
	int width = clientRect.right;
	int height = clientRect.bottom;

	BITMAPINFO bmInfo = {0};
	BITMAPINFOHEADER& bmHeader = bmInfo.bmiHeader;
	bmHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmHeader.biWidth = width;
	bmHeader.biHeight = height;
	bmHeader.biPlanes = 1;
	bmHeader.biBitCount = 32;
	bmHeader.biCompression = BI_RGB;
	bmHeader.biXPelsPerMeter = bmHeader.biYPelsPerMeter = 72;
	void* bits = nullptr;

	HBITMAP bitmap = ::CreateDIBSection (nullptr, &bmInfo, DIB_RGB_COLORS, &bits, nullptr, 0);
	ASSERT (bitmap)
	if(bitmap)
	{
		HDC hdcBitmap = ::CreateCompatibleDC (NULL);
		HGDIOBJ oldBitmap = ::SelectObject (hdcBitmap, bitmap);

		// copy from screen
		HDC hdcScreen = ::GetDC (NULL);
		POINT offset = {0};
		::ClientToScreen (hwnd, &offset);
		BOOL result = ::BitBlt (hdcBitmap, 0, 0, width, height, hdcScreen, offset.x, offset.y, SRCCOPY);
		ASSERT (result)
		::ReleaseDC (NULL, hdcScreen);

		::SelectObject (hdcBitmap, oldBitmap);
		::DeleteDC (hdcBitmap);

		if(result)
			return bitmap;

		::DeleteObject (bitmap);
	}

	return NULL;
}

//************************************************************************************************
// Brush
//************************************************************************************************

HBRUSH GdiInterop::makeSystemBrush (CCL::BrushRef brush)
{
	const SolidBrush* solidBrush = SolidBrush::castRef (brush);
	ASSERT (solidBrush != nullptr)
	return solidBrush ? makeSystemSolidBrush (*solidBrush) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HBRUSH GdiInterop::makeSystemSolidBrush (CCL::SolidBrushRef solidBrush)
{
	return ::CreateSolidBrush (GdiInterop::toSystemColor (solidBrush.getColor ()));
}

//************************************************************************************************
// Pen
//************************************************************************************************

HPEN GdiInterop::makeSystemPen (CCL::PenRef pen)
{
	ASSERT (pen.getPenType () == Pen::kSolid)
	ASSERT (pen.getLineCap () == Pen::kLineCapButt)
	ASSERT (pen.getLineJoin () == Pen::kLineJoinMiter)
	#if 1
	return ::CreatePen (PS_SOLID, (int)pen.getWidth (), GdiInterop::toSystemColor (pen.getColor ()));
	#else
	LOGBRUSH logBrush = {BS_SOLID, GdiInterop::toSystemColor (pen.getColor ()), 0};
	return ::ExtCreatePen (PS_GEOMETRIC|PS_SOLID|PS_ENDCAP_FLAT, (int)pen.getWidth (), &logBrush, 0, 0);
	#endif
}

//************************************************************************************************
// Font
//************************************************************************************************

Win32::IGdiFontCompatibilityHelper* Win32::theGdiFontHelper = nullptr;

void GdiInterop::fromLogicalFont (CCL::Font& font, const LOGFONT& logFont)
{
	font.setFace (logFont.lfFaceName);
	font.setSize ((float)abs (logFont.lfHeight));

	font.isBold (logFont.lfWeight >= FW_BOLD);
	font.isItalic (logFont.lfItalic == TRUE);
	font.isUnderline (logFont.lfUnderline == TRUE);
	font.isStrikeout (logFont.lfStrikeOut == TRUE);

	font.setMode (Font::kDefault);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HFONT GdiInterop::makeSystemFont (CCL::FontRef font)
{
	if(theGdiFontHelper)
		return theGdiFontHelper->createGdiFont (font);

	return ::CreateFont (-(int)(font.getSize () + .5f), 0, 0, 0,
						(font.getStyle () & Font::kBold) ? FW_BOLD : FW_NORMAL,
						(font.getStyle () & Font::kItalic) != 0,
						(font.getStyle () & Font::kUnderline) != 0,
						(font.getStyle () & Font::kStrikeout) != 0,
						DEFAULT_CHARSET,
						OUT_DEFAULT_PRECIS,
						CLIP_DEFAULT_PRECIS,
						font.getMode () == Font::kNone ? NONANTIALIASED_QUALITY : font.getMode () == Font::kAntiAlias ? ANTIALIASED_QUALITY : CLEARTYPE_QUALITY,
						VARIABLE_PITCH,
						StringChars (font.getFace ()));
}

