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
// Filename    : ccl/platform/android/graphics/androidrendertarget.h
// Description : Android Window Render Target
//
//************************************************************************************************

#ifndef _ccl_androidrendertarget_h
#define _ccl_androidrendertarget_h

#include "ccl/gui/windows/window.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/public/gui/graphics/dpiscale.h"

namespace CCL {

class AndroidWindow;

//************************************************************************************************
// AndroidWindowRenderTarget
//************************************************************************************************

class AndroidWindowRenderTarget: public NativeWindowRenderTarget
{
public:
	DECLARE_CLASS_ABSTRACT (AndroidWindowRenderTarget, NativeWindowRenderTarget)
	
	static AndroidWindowRenderTarget* create (Window& window);
	
protected:
	AndroidWindow* androidWindow;
	PixelPoint size;

	AndroidWindowRenderTarget (Window& window);
};

} // namespace CCL

#endif // ccl_androidrendertarget_h
