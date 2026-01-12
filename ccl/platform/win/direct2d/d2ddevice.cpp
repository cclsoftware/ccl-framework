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
// Filename    : ccl/platform/win/direct2d/d2ddevice.cpp
// Description : Direct2D Graphics Device
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/win/direct2d/d2ddevice.h"
#include "ccl/platform/win/direct2d/d2dpath.h"

#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/platform/win/gui/win32graphics.h"

/*
	Direct2D on MSDN:

	Transforms Overview
	http://msdn.microsoft.com/en-us/library/dd756655%28VS.85%29.aspx

	ID2D1RenderTarget Interface
	http://msdn.microsoft.com/en-us/library/dd371766%28VS.85%29.aspx

	See also: <d2derr.h> for list of error codes
*/

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// D2DScopedGraphicsDevice
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (D2DScopedGraphicsDevice, D2DGraphicsDevice)

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DScopedGraphicsDevice::D2DScopedGraphicsDevice (D2DRenderTarget& target, IUnknown* targetUnknown)
: D2DGraphicsDevice (target),
  targetUnknown (targetUnknown)
{
	CCL_PRINTLN ("\n\n### [Direct2D BEGIN DRAW] ###")

	if(targetUnknown)
		targetUnknown->retain ();

	target.setActive (*this, true);
	target.beginDraw ();

	initialize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DScopedGraphicsDevice::~D2DScopedGraphicsDevice ()
{
	CCL_PRINTLN (" ### [Direct2D END DRAW] ### ")

	clipper.removeClip (target); // ensure clipping is off

	target.endDraw ();
	target.setActive (*this, false);

	if(targetUnknown)
		targetUnknown->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DScopedGraphicsDevice::suspend (bool state)
{
	// called only when this device is temporarily suspended by another device
	clipper.suspendClip (target, state);
}

//************************************************************************************************
// D2DGraphicsDevice
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (D2DGraphicsDevice, NativeGraphicsDevice)

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DGraphicsDevice::D2DGraphicsDevice (D2DRenderTarget& target)
: target (target),
  textRenderer (target)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DGraphicsDevice::~D2DGraphicsDevice ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DGraphicsDevice::initialize ()
{
	SOFT_ASSERT (target.isValid (), "D2D target invalid!!!\n")
	if(target.isValid ())
	{
		#if (0 && DEBUG)
		D2D1_ANTIALIAS_MODE initialMode = target->GetAntialiasMode ();
		D2D1_TEXT_ANTIALIAS_MODE initialTextMode = target->GetTextAntialiasMode ();
		#endif

		target->SetTransform (D2D1::Matrix3x2F::Identity ());
		target->SetAntialiasMode (D2D1_ANTIALIAS_MODE_ALIASED); // turn off anti-aliasing by default!
		target->SetTextAntialiasMode (target.getDefaultTextAntialiasMode ());
		target->SetTextRenderingParams (DWriteEngine::instance ().getCachedDefaultRenderingParams ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::queryInterface (UIDRef iid, void** ptr)
{
	if(iid == ccl_iid<IWin32Graphics> ())
	{
		if(!target.isGdiCompatible ())
		{
			*ptr = nullptr;
			return kResultNoInterface;
		}
		QUERY_INTERFACE (IWin32Graphics)
	}
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HDC CCL_API D2DGraphicsDevice::getHDC ()
{
	ASSERT (target.isGdiCompatible ())

	CCL_PRINTLN ("[Direct2D get HDC]")

	// temporarily turn off clipping, otherwise GetDC() fails
	clipper.suspendClip (target, true);

	HDC hdc = nullptr;
	HRESULT hr = target.getGdiTarget ()->GetDC (D2D1_DC_INITIALIZE_MODE_COPY, &hdc);
	ASSERT (SUCCEEDED (hr))

	// init HDC transform/clipping
	if(hdc)
	{
		Rect clipRectInPoint;
		Transform transformInPoint; // not used!
		clipper.getState (transformInPoint, clipRectInPoint, true);

		int oldMode = ::SetGraphicsMode (hdc, GM_ADVANCED);
		ASSERT (oldMode == GM_COMPATIBLE)

		Transform gdiTransform;
		PixelPoint pixelOrigin (origin, getContentScaleFactor ());
		gdiTransform.translate ((float)pixelOrigin.x, (float)pixelOrigin.y);

		XFORM xform = {0};
		GdiInterop::toSystemTransform (xform, gdiTransform);
		BOOL result = ::SetWorldTransform (hdc, &xform);
		ASSERT (result)

		#if 1
		// Note: SelectClipRgn() uses device coordinates!
		PixelRect pixelClipRect (clipRectInPoint, getContentScaleFactor ());
		HRGN hRgn = ::CreateRectRgn (pixelClipRect.left, pixelClipRect.top, pixelClipRect.right, pixelClipRect.bottom);
		::ExtSelectClipRgn (hdc, hRgn, RGN_COPY);
		::DeleteObject (hRgn);
		#endif

		::SetBkMode (hdc, TRANSPARENT);
		::SetStretchBltMode (hdc, HALFTONE);
	}

	return hdc;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DGraphicsDevice::releaseHDC (HDC hdc, const RECT* rect)
{
	CCL_PRINTLN ("[Direct2D release HDC]")

	const RECT* updateRectPtr = nullptr;

	#if 1 // seems to be the right one ;-)
	if(rect)
	{
		Rect clipRect;
		Transform transform;
		clipper.getState (transform, clipRect, false);

		Rect r;
		GdiInterop::fromSystemRect (r, *rect);
		r.bound (clipRect); // bound before transformation!
		transform.transform (r);

		static RECT tempRect = {0};
		GdiInterop::toSystemRect (tempRect, r);
		updateRectPtr = &tempRect;
	}
	#elif 0
	updateRectPtr = rect;
	#endif

	// reset HDC transform/clipping
	::ExtSelectClipRgn (hdc, NULL, RGN_COPY);

	XFORM identity;
	GdiInterop::toSystemTransform (identity, Transform ());
	::SetWorldTransform (hdc, &identity);

	::SetGraphicsMode (hdc, GM_COMPATIBLE);

	HRESULT hr = target.getGdiTarget ()->ReleaseDC (updateRectPtr);
	ASSERT (SUCCEEDED (hr))

	// restore clipping
	clipper.suspendClip (target, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DGraphicsDevice::setOrigin (PointRef point)
{
	CCL_PRINTF ("[Direct2D setOrigin %d,%d]\n", point.x, point.y)

	clipper.setOrigin (target, point);

	SuperClass::setOrigin (point);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DGraphicsDevice::flushStock ()
{
	SuperClass::flushStock ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API D2DGraphicsDevice::getContentScaleFactor () const
{
	return target.getContentScaleFactor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::saveState ()
{
	CCL_PRINTLN ("[Direct2D saveState]")

	// TODO: check SaveDrawingState()/RestoreDrawingState()???

	clipper.saveState (target);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::restoreState ()
{
	CCL_PRINTLN ("[Direct2D restoreState]")

	return clipper.restoreState (target) ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::addClip (RectRef rect)
{
	return addClip (rectIntToF (rect));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::addClip (RectFRef rect)
{
	#if DEBUG_LOG
	dumpRect (rect, "[Direct2D addClip]");
	#endif

	clipper.addClip (target, rect);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::addClip (IGraphicsPath* path)
{
	CCL_PRINTLN ("[Direct2D addClip (IGraphicsPath)]")

	clipper.addClip (target, path);	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::addTransform (TransformRef matrix)
{
	CCL_PRINTLN ("[Direct2D addTransform]")

	clipper.addTransform (target, matrix);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::setMode (int mode)
{
	CCL_PRINTLN ("[Direct2D setMode]")

	bool antiAlias = (mode & kAntiAlias) != 0;
	target->SetAntialiasMode (antiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API D2DGraphicsDevice::getMode ()
{
	D2D1_ANTIALIAS_MODE mode = target->GetAntialiasMode ();
	if(mode == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE)
		return kAntiAlias;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::clearRect (RectRef rect)
{
	return clearRect (rectIntToF (rect));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::clearRect (RectFRef rect)
{
	target->PushAxisAlignedClip (D2DInterop::fromCCL (rect), D2D1_ANTIALIAS_MODE_ALIASED);
	target->Clear ();
	target->PopAxisAlignedClip ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::fillRect (RectRef rect, BrushRef brush)
{
	return fillRect (rectIntToF (rect), brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::fillRect (RectFRef rect, BrushRef brush)
{
	//CCL_PRINTLN ("[Direct2D fillRect]")

	ID2D1Brush* d2dBrush = target.getUnderlyingBrush (brush);
	target->FillRectangle (D2DInterop::fromCCL (rect), d2dBrush);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

struct D2DGraphicsDevice::DrawRectHelper
{
	ID2D1Brush* primaryBrush;
	ID2D1StrokeStyle* strokeStyle;
	Pen::Size penWidth;
	float scaleFactor;
	bool fractionalScaling;
	bool antiAliasMode;
	AntiAliasSetter antialiasSetter;

	DrawRectHelper (D2DGraphicsDevice& device, PenRef pen)
	: primaryBrush (device.target.getBrushForPen (pen)),
	  strokeStyle (device.target.getStyleForPen (pen)),
	  penWidth (pen.getWidth ()),
	  scaleFactor (device.getContentScaleFactor ()),
	  fractionalScaling (!DpiScale::isIntAligned (scaleFactor)),
	  antiAliasMode (fractionalScaling || device.getMode () == kAntiAlias), // switch to antialias mode if scale is not an integer
	  antialiasSetter (device, antiAliasMode)
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::drawRect (RectRef _r, PenRef pen)
{
	//CCL_PRINTLN ("[Direct2D drawRect]")
	DrawRectHelper helper (*this, pen);

	int penWidthInt = ccl_to_int (helper.penWidth);

	// adjust rect
	RectF r = CCL::rectIntToF (_r);
	float startShift = 0.f;
	float endShift = 0.f;

	if(helper.scaleFactor > 1.0)
	{
		float penShift = (penWidthInt % 2) == 0 ? 0.5f : 0.0f;
		startShift = 0.5f + penShift;
		endShift = -(0.5f + penShift);
		if(penShift == 0 && helper.fractionalScaling)
		{
			if(_r.left % 2 != 0)
				r.left -= 0.5f;
			if(_r.top % 2 != 0)
				r.top -= 0.5f;
		}
	}
	else if(helper.antiAliasMode)
	{
		startShift = 0.5f;
		endShift = (penWidthInt < 4) ? -0.5f : 0.5f;
	}
	else
	{
		startShift = 1.f;
		endShift = (penWidthInt % 2) == 0 ? -0.5f : 0.0f;
	}

	// finally draw it
	D2D1_RECT_F d2dRect = D2D1::RectF (r.left + startShift, r.top + startShift, r.right + endShift, r.bottom + endShift);
	target->DrawRectangle (d2dRect, helper.primaryBrush, helper.penWidth, helper.strokeStyle);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::drawRect (RectFRef rect, PenRef pen)
{
	//CCL_PRINTLN ("[Direct2D drawRect]")
	DrawRectHelper helper (*this, pen);

	// todo: check if any of the adjustments for integer coords above are required here

	D2D1_RECT_F d2dRect = D2DInterop::fromCCL (rect);
	target->DrawRectangle (d2dRect, helper.primaryBrush, helper.penWidth, helper.strokeStyle);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

struct D2DGraphicsDevice::Line
{
	D2D1_POINT_2F start;
	D2D1_POINT_2F end;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void D2DGraphicsDevice::convertLine (Line& line, int32 penWidth, PointRef _p1, PointRef _p2)
{
	Point p1 (_p1), p2 (_p2);
	bool horizontalLine = p1.y == p2.y; // a horiztontal line ---
	bool verticalLine = p1.x == p2.x;   // a vertical line  |
	bool isRightToLeft = p1.x > p2.x;
	bool isBottomUp = p1.y > p2.y;

	if(isRightToLeft && horizontalLine)
		ccl_swap (p1, p2);
	else if(isBottomUp && verticalLine)
		ccl_swap (p1, p2);

	line.start.x = (float)p1.x;
	line.start.y = (float)p1.y;
	line.end.x = (float)p2.x;
	line.end.y = (float)p2.y;

	bool antiAliasMode = getMode () == kAntiAlias;

	if(horizontalLine)
	{
		line.start.y += 0.5f;
		line.end.y += 0.5f;
	}
	else if(verticalLine)
	{
		line.start.x += 0.5f;
		line.end.x += 0.5f;
	}
	else
	{
		if(antiAliasMode)
		{
			line.start.y += 0.5f;
			line.start.x += 0.5f;
			line.end.y += 0.5f;
			line.end.x += 0.5f;
		}
		else
		{
			if(isRightToLeft)
				line.start.x += 1.f;
			else
				line.end.x += 1.f;

			if(isBottomUp)
				line.start.y += 1.f;
			else
				line.end.y += 1.f;
		}
	}

	if(antiAliasMode == false)
	{
		if(horizontalLine || verticalLine)
		{
			if(penWidth % 2 == 0)
			{
				float penWidthHalf = float (penWidth / 2);
				if(horizontalLine)
				{
					line.start.y += penWidthHalf;
					line.end.y += penWidthHalf;
				}
				else
				{
					line.start.x += penWidthHalf;
					line.end.x += penWidthHalf;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::drawLine (PointRef p1, PointRef p2, PenRef pen)
{
	//CCL_PRINTLN ("[Direct2D drawLine]")

	ID2D1Brush* brush = target.getBrushForPen (pen);
	ID2D1StrokeStyle* strokeStyle = target.getStyleForPen (pen);

	bool fractionalScaling = DpiScale::isIntAligned (getContentScaleFactor ()) == false;
	AntiAliasSetter setter (*this, fractionalScaling || getMode () == kAntiAlias);

	Line line;
	convertLine (line, ccl_to_int (pen.getWidth ()), p1, p2);

	target->DrawLine (line.start, line.end, brush, pen.getWidth (), strokeStyle);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::drawLine (PointFRef p1, PointFRef p2, PenRef pen)
{
	//CCL_PRINTLN ("[Direct2D drawLine]")

	ID2D1Brush* brush = target.getBrushForPen (pen);
	ID2D1StrokeStyle* strokeStyle = target.getStyleForPen (pen);

	// todo: check if anything from convertLine (integer coords) is required here

	target->DrawLine (D2DInterop::fromCCL (p1), D2DInterop::fromCCL (p2), brush, pen.getWidth (), strokeStyle);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::drawEllipse (RectRef rect, PenRef pen)
{
	return drawEllipse (rectIntToF (rect), pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::drawEllipse (RectFRef rect, PenRef pen)
{
	//CCL_PRINTLN ("[Direct2D drawEllipse]")

	AntiAliasSetter smoother (*this); // enable anti-aliasing

	ID2D1Brush* brush = target.getBrushForPen (pen);
	D2D1_ELLIPSE ellipse = D2DInterop::toEllipse (rect);
	ID2D1StrokeStyle* strokeStyle = target.getStyleForPen (pen);
	target->DrawEllipse (ellipse, brush, pen.getWidth (), strokeStyle);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::fillEllipse (RectRef rect, BrushRef brush)
{
	return fillEllipse (rectIntToF (rect), brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::fillEllipse (RectFRef rect, BrushRef brush)
{
	AntiAliasSetter smoother (*this); // enable anti-aliasing

	ID2D1Brush* d2dBrush = target.getUnderlyingBrush (brush);
	D2D1_ELLIPSE ellipse = D2DInterop::toEllipse (rect);
	target->FillEllipse (ellipse, d2dBrush);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::fillTriangle (const Point points[3], BrushRef brush)
{
	D2DPathGeometry path;
	path.addTriangle (points[0], points[1], points[2]);
	path.closeFigure ();
	path.fill (*this, brush);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::fillTriangle (const PointF points[3], BrushRef brush)
{
	D2DPathGeometry path;
	path.addTriangle (points[0], points[1], points[2]);
	path.closeFigure ();
	path.fill (*this, brush);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::drawString (RectRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment)
{
	return drawDirectWrite (rect, text, font, brush, TextFormat (alignment), false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::drawString (RectFRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment)
{
	return drawDirectWrite (rect, text, font, brush, TextFormat (alignment), false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::drawString (PointRef point, StringRef text, FontRef font, BrushRef brush, int options)
{
	return drawDirectWrite (pointIntToF (point), text, font, brush, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::drawString (PointFRef point, StringRef text, FontRef font, BrushRef brush, int options)
{
	return drawDirectWrite (point, text, font, brush, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::measureString (Rect& size, StringRef text, FontRef font)
{
	return measureDirectWrite (size, kMaxCoord, text, font, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::measureString (RectF& size, StringRef text, FontRef font)
{
	return measureDirectWrite (size, kMaxCoord, text, font, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::measureText (Rect& size, Coord lineWidth, StringRef text, FontRef font)
{
	return measureDirectWrite (size, lineWidth, text, font, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::measureText (RectF& size, CoordF lineWidth, StringRef text, FontRef font)
{
	return measureDirectWrite (size, lineWidth, text, font, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::drawText (RectRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format)
{
	return drawDirectWrite (rect, text, font, brush, format, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::drawText (RectFRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format)
{
	return drawDirectWrite (rect, text, font, brush, format, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult D2DGraphicsDevice::drawDirectWrite (RectRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format, bool multiline)
{
	return drawDirectWrite (rectIntToF (rect), text, font, brush, format, multiline);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult D2DGraphicsDevice::drawDirectWrite (RectFRef _rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format, bool multiline)
{
	ComPtr<IDWriteTextFormat> textFormat = DWriteEngine::instance ().createCachedTextFormat (font);
	if(textFormat == 0)
		return kResultFailed;

	DWInterop::applyAlignment (textFormat, format.getAlignment ());
	DWInterop::setWordWrapping (textFormat, multiline && format.isWordBreak ());
	//if(multiline == false)
	//	DWInterop::setCharacterTrimming (textFormat, true);

	// configure brush
	ID2D1Brush* d2dBrush = target.getUnderlyingBrush (brush);

	// configure anti-aliasing
	D2DTextAntialiasModeSetter smoother (*this, font.getMode ());

	RectF rect (_rect);
	DWInterop::adjustLayoutPosition (rect, format.getAlignment ());
	if(rect.isEmpty ()) // text layout creation would fail if rect is empty
		return kResultOk;

	// draw text
	if(multiline || font.isUnderline () || font.isStrikeout () || font.getSpacing () != 0.f)
	{
		ComPtr<IDWriteTextLayout> textLayout = DWriteEngine::instance ().createTextLayoutWithFontAttributes (text, textFormat, (float)rect.getWidth (), (float)rect.getHeight (), font);
		if(textLayout)
		{
			if(multiline)
				DWInterop::adjustTabStops (textLayout);
			target->DrawTextLayout (D2DInterop::fromCCL (rect.getLeftTop ()), textLayout, d2dBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
		}
	}
	else
	{
		StringChars textChars (text);
		UINT textLength = text.length ();
		D2D1_RECT_F layoutRect = D2DInterop::fromCCL (rect);
		target->DrawText (textChars, textLength, textFormat, layoutRect, d2dBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult D2DGraphicsDevice::drawDirectWrite (PointFRef point, StringRef text, FontRef font, BrushRef brush, int options)
{
	ComPtr<IDWriteTextFormat> textFormat = DWriteEngine::instance ().createCachedTextFormat (font);
	if(textFormat == 0)
		return kResultFailed;

	textFormat->SetTextAlignment (DWRITE_TEXT_ALIGNMENT_LEADING);
	textFormat->SetParagraphAlignment (DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

	// configure brush
	ID2D1Brush* d2dBrush = target.getUnderlyingBrush (brush);

	// configure anti-aliasing
	D2DTextAntialiasModeSetter smoother (*this, font.getMode ());

	PointF pos (point);

	static const CoordF kRectSize = kMaxCoord;
	bool atBaseline = (options & kDrawAtBaseline);
	
	// draw text
	if(atBaseline || font.isUnderline () || font.isStrikeout () || font.getSpacing () != 0.f)
	{
		ComPtr<IDWriteTextLayout> textLayout = DWriteEngine::instance ().createTextLayoutWithFontAttributes (text, textFormat, kRectSize, kRectSize, font);
		if(atBaseline)
			pos.y -= DWTextLayout::getBaseline (textLayout);
		if(textLayout)
			target->DrawTextLayout (D2DInterop::fromCCL (pos), textLayout, d2dBrush);
	}
	else
	{
		StringChars textChars (text);
		UINT textLength = text.length ();
		RectF rect (pos.x, pos.y, pos.x + kRectSize, pos.y + kRectSize);
		target->DrawText (textChars, textLength, textFormat, D2DInterop::fromCCL (rect), d2dBrush);
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult D2DGraphicsDevice::measureDirectWrite (Rect& size, Coord lineWidth, StringRef text, FontRef font, bool multiline)
{
	RectF sizeF;
	if(measureDirectWrite (sizeF, (float)lineWidth, text, font, multiline) != kResultOk)
		return kResultFailed;

	size = rectFToInt (sizeF);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult D2DGraphicsDevice::measureDirectWrite (RectF& size, CoordF lineWidth, StringRef text, FontRef font, bool multiline)
{
	ComPtr<IDWriteTextFormat> textFormat = DWriteEngine::instance ().createCachedTextFormat (font);
	if(textFormat == 0)
		return kResultFailed;

	DWInterop::applyAlignment (textFormat, Alignment::kLeftTop);
	DWInterop::setWordWrapping (textFormat, multiline);

	ComPtr<IDWriteTextLayout> textLayout = DWriteEngine::instance ().createTextLayoutWithFontAttributes (text, textFormat, lineWidth, (float)kMaxCoord, font);
	if(!textLayout)
		return kResultFailed;

	if(multiline)
		DWInterop::adjustTabStops (textLayout);

	DWInterop::getTextMetrics (size, textLayout);
	DWInterop::adjustTextMetrics (size);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::drawTextLayout (PointRef pos, ITextLayout* textLayout, BrushRef brush, int options)
{
	return drawTextLayout (pointIntToF (pos), textLayout, brush, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DGraphicsDevice::drawTextLayout (PointFRef _pos, ITextLayout* _textLayout, BrushRef brush, int options)
{
	DWTextLayout* textLayout = unknown_cast<DWTextLayout> (_textLayout);
	if(textLayout == nullptr)
		return SuperClass::drawTextLayout (_pos, _textLayout, brush, options); // fallback to simple text layout

	ASSERT (textLayout->getLayout ())
	if(textLayout->getLayout ())
	{
		// configure brush
		ID2D1Brush* d2dBrush = target.getUnderlyingBrush (brush);

		// configure anti-aliasing
		D2DTextAntialiasModeSetter smoother (*this, textLayout->getFont ().getMode ());

		PointF pos (_pos);
		if(options & kDrawAtBaseline)
		{
			PointF offset;
			textLayout->getBaselineOffset (offset);
			pos -= offset;
		}
		else
			DWInterop::adjustLayoutPos (pos, textLayout->getAlignment ());

		ASSERT ((options & (~kDrawAtBaseline)) == 0) // no other options implemented
		textLayout->getLayout ()->Draw (d2dBrush, &textRenderer, pos.x, pos.y);
	}
	return kResultOk;
}
