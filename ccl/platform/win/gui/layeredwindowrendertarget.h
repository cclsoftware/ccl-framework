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
// Filename    : ccl/platform/win/gui/layeredwindowrendertarget.h
// Description : Layered window render target (WS_EX_LAYERED)
//
//************************************************************************************************

#ifndef _ccl_win32_layeredwindowrendertarget_h
#define _ccl_win32_layeredwindowrendertarget_h

#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/imaging/offscreen.h"

#include "ccl/platform/win/gui/win32graphics.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// LayeredWindowRenderTarget
//************************************************************************************************

class LayeredWindowRenderTarget: public NativeWindowRenderTarget
{
public:
	DECLARE_CLASS_ABSTRACT (LayeredWindowRenderTarget, NativeWindowRenderTarget)

	LayeredWindowRenderTarget (Window& window);
	~LayeredWindowRenderTarget ();

	// NativeWindowRenderTarget
	bool shouldCollectUpdates () override;
	IMutableRegion* getUpdateRegion () override;
	void onRender () override;
	void onSize () override;
	void onScroll (RectRef rect, PointRef delta) override;

protected:
	AutoPtr<Bitmap> offscreen;
	
	float getContentScaleFactor () const;
	void render (const GdiClipRegion::RectList& rectList);
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_win32_layeredwindowrendertarget_h
