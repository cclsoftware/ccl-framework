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
// Filename    : ccl/platform/cocoa/skia/skiarendertarget.ios.mm
// Description : Skia Render Target for iOS using Metal
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/skia/skiarendertarget.ios.h"

#include "ccl/platform/cocoa/gui/window.ios.h"
#include "ccl/platform/cocoa/gui/nativeview.ios.h"
#include "ccl/platform/cocoa/metal/metalclient.h"

#include "ccl/public/base/ccldefpush.h"
#import <UIKit/UIKit.h>
#include <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>
#include "ccl/public/base/ccldefpop.h"

using namespace CCL;

//************************************************************************************************
// SkiaWindowRenderTarget
//************************************************************************************************

SkiaWindowRenderTarget* SkiaWindowRenderTarget::create (Window& window)
{
	SkiaWindowRenderTarget* result = nullptr;

	if(MetalClient::instance ().isSupported ())
		result = NEW MetalIOSWindowRenderTarget (window);

	return result;
}

//************************************************************************************************
// MetalIOSWindowRenderTarget
//************************************************************************************************

MetalIOSWindowRenderTarget::MetalIOSWindowRenderTarget (Window& window)
: MetalWindowRenderTarget (window),
  hostView (nullptr)
{
	initialize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetalIOSWindowRenderTarget::~MetalIOSWindowRenderTarget ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalIOSWindowRenderTarget::initialize ()
{
	IOSWindow* iosWindow = IOSWindow::cast (&window);
	NativeView* view = iosWindow->getNativeView ();
	ASSERT (view)
	if(!view)
		return;
	hostView = view->getView ();
	ASSERT (hostView)
	if(!hostView)
		return;
	metalLayer = static_cast<CAMetalLayer*> (hostView.layer);
	metalLayer.framebufferOnly = NO;
	[metalLayer retain];
	
	MetalWindowRenderTarget::initialize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalIOSWindowRenderTarget::addMetal3DSurface (Native3DSurface* surface)
{
	if(Metal3DSurface* metal3DSurface = ccl_cast<Metal3DSurface> (surface))
	{
		[hostView addSubview:metal3DSurface->getView ()];
		MetalWindowRenderTarget::addMetal3DSurface (metal3DSurface);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalIOSWindowRenderTarget::removeMetal3DSurface (Native3DSurface* surface)
{
	if(Metal3DSurface* metal3DSurface = ccl_cast<Metal3DSurface> (surface))
	{
		MetalWindowRenderTarget::removeMetal3DSurface (metal3DSurface);
		[metal3DSurface->getView () removeFromSuperview];
	}
}
