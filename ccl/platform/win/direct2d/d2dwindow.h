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
// Filename    : ccl/platform/win/direct2d/d2dwindow.h
// Description : Direct2D Window Render Target
//
//************************************************************************************************

#ifndef _ccl_direct2d_window_h
#define _ccl_direct2d_window_h

#include "ccl/platform/win/direct2d/d2dbase.h"
#include "ccl/platform/win/direct2d/dxgiengine.h"

#include "ccl/gui/graphics/nativegraphics.h"

namespace CCL {
namespace Win32 {

class D3DSurface;
class GdiClipRegion;

//************************************************************************************************
// D2DWindowRenderTarget
//************************************************************************************************

class D2DWindowRenderTarget: public NativeWindowRenderTarget,
							 public D2DRenderTarget
{
public:
	DECLARE_CLASS_ABSTRACT (D2DWindowRenderTarget, NativeWindowRenderTarget)

	D2DWindowRenderTarget (Window& window);
	~D2DWindowRenderTarget ();

	IDXGISwapChain1* getSwapChain () { return swapChain; }
	void discardSwapChain ();

	PROPERTY_BOOL (flushNeeded, FlushNeeded)
	void flush ();
	void invalidate ();

	// NativeWindowRenderTarget
	bool shouldCollectUpdates () override;
	IMutableRegion* getUpdateRegion () override;
	void onRender () override;
	void onSize () override;
	void onScroll (RectRef rect, PointRef delta) override;
	void add3DSurface (Native3DSurface* surface) override;
	void remove3DSurface (Native3DSurface* surface) override;

	// D2DRenderTarget
	bool isAlphChannelUsed () const override;
	float getContentScaleFactor () const override;

protected:
	ComPtr<IDXGISwapChain1> swapChain;
	ComPtr<ID2D1Bitmap1> swapChainBitmap;

	GdiClipRegion* updateRegion;
	Rect scrollRect;
	Point scrollOffset;
	Vector<SharedPtr<D3DSurface>> surfaces;

	Point getPixelSize () const;
	bool isDirectUpdateEnabled () const;
	void handleError (CStringPtr message, HRESULT hr);
	bool makeSwapChainBitmap (PointRef sizeInPixel);
	void present (const DXGI_PRESENT_PARAMETERS* params = nullptr);
	void render (GdiClipRegion& renderRegion);
	void render3DContent ();
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_direct2d_window_h
