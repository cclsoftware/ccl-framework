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
// Filename    : ccl/platform/win/direct2d/d2dtextlayout.h
// Description : Direct2D Text Layout
//
//************************************************************************************************

#ifndef _ccl_direct2d_textlayout_h
#define _ccl_direct2d_textlayout_h

#include "ccl/platform/win/direct2d/dwriteengine.h"

namespace CCL {
namespace Win32 {

class D2DRenderTarget;
	
//************************************************************************************************
// D2DTextLayout
//************************************************************************************************
	
class D2DTextLayout: public DWTextLayout
{
public:
	// DWTextLayout
	tresult CCL_API setTextColor (const Range& range, Color color) override;
	tresult CCL_API setBaselineOffset (const Range& range, float offset) override;
	tresult CCL_API setSuperscript (const Range& range) override;
	tresult CCL_API setSubscript (const Range& range) override;

private:
	tresult setSuperscript (const Range& range, float sizeFactor, float baselineFactor);
	template<class T> tresult setEffect (const Range& range, T function);
};

//************************************************************************************************
// D2DTextRenderer
//************************************************************************************************
	
class D2DTextRenderer: public Object,
					   public IDWriteTextRenderer
{
public:
	D2DTextRenderer (D2DRenderTarget& target);

	// IUnknown
	DELEGATE_COM_IUNKNOWN
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;

	// IDWriteTextRenderer
	HRESULT STDMETHODCALLTYPE DrawGlyphRun (void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, DWRITE_MEASURING_MODE measuringMode, 
		const DWRITE_GLYPH_RUN* glyphRun, const DWRITE_GLYPH_RUN_DESCRIPTION* glyphRunDescription, ::IUnknown* clientDrawingEffect) override;
	HRESULT STDMETHODCALLTYPE DrawUnderline (void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, const DWRITE_UNDERLINE* underline, ::IUnknown* clientDrawingEffect) override;
	HRESULT STDMETHODCALLTYPE DrawStrikethrough (void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, const DWRITE_STRIKETHROUGH* strikethrough, ::IUnknown* clientDrawingEffect) override;
	HRESULT STDMETHODCALLTYPE DrawInlineObject (void* clientDrawingContext, FLOAT originX, FLOAT originY, IDWriteInlineObject* inlineObject, BOOL isSideways, BOOL isRightToLeft, ::IUnknown* clientDrawingEffect) override;
	HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled (void* clientDrawingContext, BOOL* isDisabled) override;
	HRESULT STDMETHODCALLTYPE GetCurrentTransform (void* clientDrawingContext, DWRITE_MATRIX* transform) override;
	HRESULT STDMETHODCALLTYPE GetPixelsPerDip (void* clientDrawingContext, FLOAT* pixelsPerDip) override;

private:
	D2DRenderTarget& target;

	HRESULT drawRect (const D2D1_RECT_F& rect, void* clientDrawingContext, ::IUnknown* clientDrawingEffect) const;
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_direct2d_textlayout_h
