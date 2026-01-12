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
// Filename    : ccl/app/utilities/imagebuilder.h
// Description : Image Builder
//
//************************************************************************************************

#ifndef _ccl_imagebuilder_h
#define _ccl_imagebuilder_h

#include "ccl/public/gui/graphics/iimage.h"

namespace CCL {

//************************************************************************************************
// ImageBuilder
//************************************************************************************************

class ImageBuilder
{
public:
	enum ThumbnailFlags
	{
		kKeepAspectRatio = (1 << 0)
	};

	/** Check if we are on a high DPI system. */
	static bool isHighResolutionImageNeeded ();

	static IImage* createBitmapCopy (IImage* sourceImage);
	static IImage* createSizedImage (IImage* sourceImage, int width, int height, float scaleFactor);
	static IImage* createBlurredImage (IImage* sourceImage, float blurFactor, int width, int height, bool saturate = false);
	static IImage* createIconSet (IImage* sourceImage, int sizeIDList);

	static const int kThumbnailSize = 96;
	static IImage* createThumbnail (IImage* sourceImage, float scaleFactor = 2.f, int flags = 0);
};

} // namespace CCL

#endif // _ccl_imagebuilder_h
