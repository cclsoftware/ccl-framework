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
// Filename    : ccl/platform/android/graphics/androidrendertarget.cpp
// Description : Android Window Render Target
//
//************************************************************************************************

#include "androidrendertarget.h"

#include "ccl/platform/android/gui/window.android.h"

#include "ccl/platform/android/vulkan/vulkanrendertarget.android.h"

using namespace CCL;
using namespace Android;

//************************************************************************************************
// AndroidWindowRenderTarget
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AndroidWindowRenderTarget, NativeWindowRenderTarget)

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidWindowRenderTarget::AndroidWindowRenderTarget (Window& window)
: NativeWindowRenderTarget (window),
  androidWindow (AndroidWindow::cast (&window)),
  size (window.getSize ().getSize (), window.getContentScaleFactor ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidWindowRenderTarget* AndroidWindowRenderTarget::create (Window& window)
{
	if(!VulkanClient::instance ().isSupported ())
		return nullptr;

	if(!AndroidWindow::cast (&window)->getFrameworkView ())
		return nullptr;

	return NEW VulkanWindowRenderTarget (window);
}
