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
// Filename    : ccl/platform/linux/opengles/openglesclient.linux.cpp
// Description : OpenGL ES Client Context using Wayland
//
//************************************************************************************************

#include "ccl/platform/shared/opengles/openglesclient.h"

#include "ccl/platform/linux/wayland/waylandclient.h"

namespace CCL {
namespace Linux {
    
//************************************************************************************************
// LinuxOpenGLESClient
//************************************************************************************************

class LinuxOpenGLESClient: public OpenGLESClient,
						   public WaylandObject
{
protected:
	// WaylandObject
	void onCompositorDisconnected () override;
	void onCompositorConnected () override;

	// OpenGLESClient
	void initializePlatform () override;
	void terminatePlatform () override;
};

} // namespace Linux
} // namespace CCL

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// LinuxOpenGLESClient
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (OpenGLESClient, LinuxOpenGLESClient)

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxOpenGLESClient::initializePlatform ()
{
	// try to initialize with a wayland display
	wl_display* waylandDisplay = WaylandClient::instance ().getDisplay ();
	if(waylandDisplay != nullptr)
		initialize (waylandDisplay);
	WaylandClient::instance ().registerObject (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxOpenGLESClient::terminatePlatform ()
{
	WaylandClient::instance ().unregisterObject (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxOpenGLESClient::onCompositorDisconnected ()
{
	display = EGL_NO_DISPLAY;
	context = EGL_NO_CONTEXT;
	terminatePlatform ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxOpenGLESClient::onCompositorConnected ()
{
	initializePlatform ();
}
