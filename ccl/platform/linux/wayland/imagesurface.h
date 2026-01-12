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
// Filename    : ccl/platform/linux/wayland/imagesurface.h
// Description : Wayland Image Surface
//
//************************************************************************************************

#ifndef _ccl_linux_imagesurface_h
#define _ccl_linux_imagesurface_h

#include "ccl/platform/linux/wayland/waylandbuffer.h"
#include "ccl/platform/linux/wayland/surface.h"

#include "ccl/gui/graphics/imaging/image.h"

namespace CCL {
namespace Linux {

//************************************************************************************************
// ImageSurface
//************************************************************************************************

class ImageSurface: public Object,
					public Linux::Surface
{
public:
	ImageSurface ();

	void setImage (Image* image);

	// Surface
	void createSurface () override;
	void setScaleFactor (int scaleFactor) override;

private:
	SharedPtr<Image> image;
	WaylandBuffer buffer;
	int scaleFactor;

	void render ();
};

} // namespace Linux
} // namespace CCL

#endif // _ccl_linux_imagesurface_h
