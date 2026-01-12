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
// Filename    : ccl/platform/shared/opengles/openglesimage.h
// Description : OpenGLES Image
//
//************************************************************************************************

#ifndef _openglesimage_h
#define _openglesimage_h

#include "ccl/public/gui/graphics/point.h"
#include "ccl/public/base/cclmacros.h"

namespace CCL {

//************************************************************************************************
// OpenGLESImage
//************************************************************************************************

class OpenGLESImage
{
public:
	OpenGLESImage ();
	~OpenGLESImage ();

	PROPERTY_VARIABLE (Point, size, Size)
	PROPERTY_VARIABLE (uint32, sampleCount, SampleCount)
	PROPERTY_VARIABLE (uint32, textureId, TextureID)
	PROPERTY_VARIABLE (uint32, framebufferId, FramebufferID)
	PROPERTY_VARIABLE (uint32, format, Format)
	PROPERTY_VARIABLE (int, wrapMode, WrapMode)
	PROPERTY_VARIABLE (int, mipLevels, MipLevels)
	
	bool create (const void* initialData = nullptr);
	void destroy ();
	
	bool generateFramebuffer ();
	bool generateMipmaps ();
};

} // namespace CCL

#endif // _openglesimage_h

