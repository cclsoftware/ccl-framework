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
// Filename    : ccl/platform/linux/gui/transparentwindow.linux.h
// Description : Transparent Window
//
//************************************************************************************************

#ifndef _ccl_transparentwindow_linux_h
#define _ccl_transparentwindow_linux_h

#include "ccl/platform/linux/wayland/subsurface.h"
#include "ccl/platform/linux/wayland/waylandbuffer.h"

#include "ccl/gui/windows/transparentwindow.h"

#include "ccl/public/collections/vector.h"

namespace CCL {

//************************************************************************************************
// LinuxTransparentWindow
//************************************************************************************************

class LinuxTransparentWindow: public TransparentWindow,
							  public Linux::SubSurface<>
{
public:
	LinuxTransparentWindow (Window* parentWindow, int options, StringRef title);
	~LinuxTransparentWindow ();

	// TransparentWindow
	void show () override;
	void hide () override;
	bool isVisible () const override;
	void update (RectRef size, Bitmap& bitmap, PointRef offset = Point (), float opacity = 1.f) override;
	void move (PointRef position) override;
	
protected:
	Linux::WaylandBuffer buffers[2];
	Rect size;
	bool initialized;
    bool visible;
    bool suspended;
};

} // namespace CCL

#endif // _ccl_transparentwindow_linux_h
