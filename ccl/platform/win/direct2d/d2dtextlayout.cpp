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
// Filename    : ccl/platform/win/direct2d/d2dtextlayout.cpp
// Description : Direct2D Text Layout
//
//************************************************************************************************

#include "ccl/platform/win/direct2d/d2dtextlayout.h"
#include "ccl/platform/win/direct2d/d2dbase.h"
#include "ccl/platform/win/direct2d/dxgiengine.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// D2DTextEffect
//************************************************************************************************

class D2DTextEffect: public Object,
					 public ::IUnknown
{
public:
	D2DTextEffect ();

	void copyFrom (const D2DTextEffect& other);

	HRESULT setTextColor (const Color& color);
	PROPERTY_VARIABLE (float, baselineOffset, BaselineOffset);

	ID2D1SolidColorBrush* getBrush () const { return brush; }

	DELEGATE_COM_IUNKNOWN

private:
	ComPtr<ID2D1SolidColorBrush> brush;
};

} // namespace Win32
} // namespace CCL

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// D2DTextLayout
//************************************************************************************************
	
template<class T> 
tresult D2DTextLayout::setEffect (const Range& range, T function)
{
	ASSERT (layout != 0)
	if(!layout)
		return kResultUnexpected;
	
	AutoPtr<D2DTextEffect> effect = NEW D2DTextEffect;

	ComPtr<::IUnknown> existingEffect = nullptr;
	DWRITE_TEXT_RANGE effectRange = {0};
	int rangeEnd = range.start + range.length;
	for(int start = range.start; start < rangeEnd; start++)
	{
		HRESULT hr = layout->GetDrawingEffect (start, &existingEffect, &effectRange);
		if(SUCCEEDED (hr) && existingEffect.isValid ())
		{
			int effectRangeEnd = effectRange.startPosition + effectRange.length;
		
			if(effectRange.startPosition > range.start)
			{
				// Effect changes in the requested range. Split range and try again.
				tresult result = setEffect ({ range.start, int(effectRange.startPosition) - range.start }, function);
				if(result == kResultOk)
					result = setEffect ({ int(effectRange.startPosition), rangeEnd - int(effectRange.startPosition) }, function);
				return result;
			}

			if(effectRangeEnd > 0 && effectRangeEnd < rangeEnd)
			{
				// Effect changes in the requested range. Split range and try again.
				tresult result = setEffect ({ range.start, effectRangeEnd - range.start }, function);
				if(result == kResultOk)
					result = setEffect ({ effectRangeEnd, rangeEnd - effectRangeEnd }, function);
				return result;
			}

			if(effectRange.startPosition < start)
				layout->SetDrawingEffect (existingEffect, { effectRange.startPosition, start - effectRange.startPosition });
			if(effectRangeEnd > rangeEnd)
				layout->SetDrawingEffect (existingEffect, { uint32(rangeEnd), uint32(effectRangeEnd - rangeEnd) });

			effect->copyFrom (*static_cast<D2DTextEffect*> (static_cast<::IUnknown*> (existingEffect)));

			start = effectRangeEnd;
		}
	}

	HRESULT hr = function (*effect);
	if(SUCCEEDED (hr))
		hr = layout->SetDrawingEffect (effect, { uint32(range.start), uint32(range.length) });

	return static_cast<tresult> (hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DTextLayout::setTextColor (const Range& range, Color color)
{
	return setEffect (range, [&] (D2DTextEffect& effect) 
	{
		return effect.setTextColor (color);
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DTextLayout::setBaselineOffset (const Range& range, float offset)
{
	return setEffect (range, [&] (D2DTextEffect& effect) 
	{
		effect.setBaselineOffset (offset);
		return S_OK;
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult D2DTextLayout::setSuperscript (const Range& range, float sizeFactor, float baselineFactor)
{
	ASSERT (layout != 0)
	if(!layout)
		return kResultUnexpected;

	float fontSize = 0;
	DWRITE_TEXT_RANGE fontSizeRange = {0};
	int rangeEnd = range.start + range.length;
	for(int start = range.start; start < rangeEnd; start++)
	{
		HRESULT hr = layout->GetFontSize (start, &fontSize, &fontSizeRange);
		if(FAILED (hr))
			return static_cast<tresult> (hr);

		int fontSizeRangeEnd = fontSizeRange.startPosition + fontSizeRange.length;
		
		if(fontSizeRange.startPosition > range.start)
		{
			// Font size changes in the requested range. Split range and try again.
			tresult result = setSuperscript ({ range.start, int(fontSizeRange.startPosition) - range.start }, sizeFactor, baselineFactor);
			if(result == kResultOk)
				result = setSuperscript ({ int(fontSizeRange.startPosition), rangeEnd - int(fontSizeRange.startPosition) }, sizeFactor, baselineFactor);
			return result;
		}

		if(fontSizeRangeEnd > 0 && fontSizeRangeEnd < rangeEnd)
		{
			// Font size changes in the requested range. Split range and try again.
			tresult result = setSuperscript ({ range.start, fontSizeRangeEnd - range.start }, sizeFactor, baselineFactor);
			if(result == kResultOk)
				result = setSuperscript ({ fontSizeRangeEnd, rangeEnd - fontSizeRangeEnd }, sizeFactor, baselineFactor);
			return result;
		}
	}

	tresult result = setEffect (range, [&] (D2DTextEffect& effect) 
	{
		effect.setBaselineOffset (effect.getBaselineOffset () + baselineFactor * fontSize);
		return S_OK;
	});
	if(result == kResultOk)
		result = setFontSize (range, fontSize * sizeFactor);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
tresult CCL_API D2DTextLayout::setSuperscript (const Range& range)
{
	return setSuperscript (range, kSuperscriptSizeFactor, -kSuperscriptBaselineFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DTextLayout::setSubscript (const Range& range)
{	
	return setSuperscript (range, kSubscriptSizeFactor, kSubscriptBaselineFactor);
}

//************************************************************************************************
// D2DTextRenderer
//************************************************************************************************

D2DTextRenderer::D2DTextRenderer (D2DRenderTarget& target)
: target (target)
{}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE D2DTextRenderer::DrawGlyphRun (void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, DWRITE_MEASURING_MODE measuringMode, 
	const DWRITE_GLYPH_RUN* glyphRun, const DWRITE_GLYPH_RUN_DESCRIPTION* glyphRunDescription, ::IUnknown* clientDrawingEffect)
{
	ID2D1Brush* brush = static_cast<ID2D1Brush*> (clientDrawingContext);
	D2D1_POINT_2F position = { baselineOriginX, baselineOriginY };

	D2DTextEffect* effect = nullptr;
	if(clientDrawingEffect)
		effect = static_cast<D2DTextEffect*> (clientDrawingEffect);

	if(effect)
	{
		if(effect->getBrush ())
			brush = effect->getBrush ();

		position.y += effect->getBaselineOffset ();
	}

	target->DrawGlyphRun (position, glyphRun, brush, measuringMode);

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT D2DTextRenderer::drawRect (const D2D1_RECT_F& _rect, void* clientDrawingContext, ::IUnknown* clientDrawingEffect) const
{
	ID2D1Brush* brush = static_cast<ID2D1Brush*> (clientDrawingContext);
	
	D2D1_RECT_F rect = _rect;
	
	D2DTextEffect* effect = nullptr;
	if(clientDrawingEffect)
		effect = static_cast<D2DTextEffect*> (clientDrawingEffect);
	
	if(effect)
	{
		if(effect->getBrush ())
			brush = effect->getBrush ();

		rect.top += effect->getBaselineOffset ();
		rect.bottom += effect->getBaselineOffset ();
	}

	target->FillRectangle (rect, brush);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE D2DTextRenderer::DrawUnderline (void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, const DWRITE_UNDERLINE* underline, ::IUnknown* clientDrawingEffect)
{
	ASSERT (underline->readingDirection == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT || underline->readingDirection == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT)

	D2D1_RECT_F rect = {0};
	rect.left = baselineOriginX - (underline->readingDirection == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT ? 0 : underline->width);
	rect.top = baselineOriginY + underline->offset;
	rect.right = rect.left + (underline->readingDirection == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT ? underline->width : 0);
	rect.bottom = rect.top + underline->thickness;

	return drawRect (rect, clientDrawingContext, clientDrawingEffect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE D2DTextRenderer::DrawStrikethrough (void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, const DWRITE_STRIKETHROUGH* strikethrough, ::IUnknown* clientDrawingEffect)
{
	ASSERT (strikethrough->readingDirection == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT || strikethrough->readingDirection == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT)

	D2D1_RECT_F rect = {0};
	rect.left = baselineOriginX - (strikethrough->readingDirection == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT ? 0 : strikethrough->width);
	rect.top = baselineOriginY + strikethrough->offset;
	rect.right = rect.left + (strikethrough->readingDirection == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT ? strikethrough->width : 0);
	rect.bottom = rect.top + strikethrough->thickness;

	return drawRect (rect, clientDrawingContext, clientDrawingEffect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE D2DTextRenderer::DrawInlineObject (void* clientDrawingContext, FLOAT originX, FLOAT originY, IDWriteInlineObject* inlineObject, BOOL isSideways, BOOL isRightToLeft, ::IUnknown* clientDrawingEffect)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE D2DTextRenderer::IsPixelSnappingDisabled (void* clientDrawingContext, BOOL* isDisabled)
{
    *isDisabled = FALSE;
    return S_OK;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE D2DTextRenderer::GetCurrentTransform (void* clientDrawingContext, DWRITE_MATRIX* transform)
{
    //forward the render target's transform
	target->GetTransform (reinterpret_cast<D2D1_MATRIX_3X2_F*> (transform));
    return S_OK;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE D2DTextRenderer::GetPixelsPerDip (void* clientDrawingContext, FLOAT* pixelsPerDip)
{
	*pixelsPerDip = target.getContentScaleFactor ();
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DTextRenderer::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_COM_INTERFACE (IDWritePixelSnapping)
	QUERY_COM_INTERFACE (IDWriteTextRenderer)
	return Object::queryInterface (iid, ptr);
}

//************************************************************************************************
// D2DTextEffect
//************************************************************************************************

D2DTextEffect::D2DTextEffect ()
: baselineOffset (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DTextEffect::copyFrom (const D2DTextEffect& other)
{
	setBaselineOffset (other.getBaselineOffset ());
	brush = other.brush;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT D2DTextEffect::setTextColor (const Color& color)
{
	brush.release ();
	return DXGIEngine::instance ().getDirect2dDeviceContext ()->CreateSolidColorBrush (D2DInterop::toColorF (color), brush);
}
