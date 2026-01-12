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
// Filename    : ccl/platform/win/gui/win32graphics.h
// Description : Win32 Graphics Helpers
//
//************************************************************************************************

#ifndef _ccl_win32graphics_h
#define _ccl_win32graphics_h

#include "ccl/platform/win/interfaces/iwin32graphics.h"

#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/gui/graphics/updatergn.h"

#include "ccl/public/base/buffer.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// GdiClipRegion
//************************************************************************************************

class GdiClipRegion: public Unknown,
					 public IMutableRegion
{
public:
	struct RectList;

	GdiClipRegion ();
	GdiClipRegion (HDC hdc);
	GdiClipRegion (HWND hwnd);
	~GdiClipRegion ();

	bool isEmpty () const;
	void getBounds (Rect& rect) const;
	void removeAll ();

	void addRegion (const GdiClipRegion& other);
	void addRectList (const RectList& rectList);

	/** Get rectangles in region (pass null to determine count). */
	int getRects (Rect* rects, int count) const;

	// IMutableRegion
	tbool CCL_API rectVisible (RectRef rect) const override;
	void CCL_API addRect (RectRef rect) override;
	void CCL_API setEmpty () override;
	Rect CCL_API getBoundingBox () const override;

	CLASS_INTERFACE2 (IMutableRegion, IUpdateRegion, Unknown)

protected:
	HDC hdc;
	HRGN hrgn;
};

//************************************************************************************************
// GdiClipRegion::RectList
//************************************************************************************************

struct GdiClipRegion::RectList
{
	int rectCount;
	Rect* rects;

	RectList (const GdiClipRegion& region);

	void removeEmptyRects ();
	void adjustToCoords (float scaleFactor);
	void adjustToPixels (float scaleFactor);

private:
	static const int kMaxRectCount = 8;
	Rect fixedRects[kMaxRectCount];
	Array<Rect> rectArray;
};

//************************************************************************************************
// HBITMAP helper
//************************************************************************************************

HBITMAP CopyBitmapToDIBSection (HBITMAP bitmap);
HBITMAP CreateScreenshotFromHWND (HWND hwnd);

//************************************************************************************************
// IGdiFontCompatibilityHelper
//************************************************************************************************

interface IGdiFontCompatibilityHelper
{
	virtual HFONT createGdiFont (FontRef font) const = 0;
};

extern IGdiFontCompatibilityHelper* theGdiFontHelper;

namespace GdiInterop {

//************************************************************************************************
// Brush
//************************************************************************************************

HBRUSH makeSystemBrush (CCL::BrushRef brush);
HBRUSH makeSystemSolidBrush (CCL::SolidBrushRef solidBrush);

//************************************************************************************************
// Pen
//************************************************************************************************

HPEN makeSystemPen (CCL::PenRef pen);

//************************************************************************************************
// Font
//************************************************************************************************

void fromLogicalFont (CCL::Font& font, const LOGFONT& logFont);
HFONT makeSystemFont (CCL::FontRef font);

//************************************************************************************************
// Point
//************************************************************************************************

inline POINT& toSystemPoint (POINT& dst, CCL::PointRef src)
{ 
	dst.x = src.x;
	dst.y = src.y; 
	return dst; 
}

//************************************************************************************************
// Rect
//************************************************************************************************

inline CCL::Rect& fromSystemRect (CCL::Rect& dst, const RECT& src)
{
	return dst (src.left, src.top, src.right, src.bottom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline RECT& toSystemRect (RECT& dst, const CCL::Rect& src)
{
	dst.left = src.left;
	dst.top = src.top;
	dst.right = src.right;
	dst.bottom = src.bottom;
	return dst;
}

//************************************************************************************************
// Transform
//************************************************************************************************

inline XFORM& toSystemTransform (XFORM& dst, CCL::TransformRef src)
{
	XFORM temp = {src.a0, src.a1, src.b0, src.b1, src.t0, src.t1};
	dst = temp;
	return dst;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline CCL::Transform& fromSystemTransform (CCL::Transform& dst, const XFORM& src)
{
	dst (src.eM11, src.eM12, src.eM21, src.eM22, src.eDx, src.eDy);
	return dst;
}

//************************************************************************************************
// Color
//************************************************************************************************

inline COLORREF toSystemColor (CCL::Color color)
{
	return RGB (color.red, color.green, color.blue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline CCL::Color fromSystemColor (COLORREF color)
{
	return CCL::Color (GetRValue (color), GetGValue (color), GetBValue (color), 0xFF);
}

} // namespace GdiInterop

} // namespace Win32
} // namespace CCL

#endif // _ccl_win32graphics_h
