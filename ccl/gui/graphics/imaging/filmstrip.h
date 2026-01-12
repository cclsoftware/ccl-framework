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
// Filename    : ccl/gui/graphics/imaging/filmstrip.h
// Description : Filmstrip
//
//************************************************************************************************

#ifndef _ccl_filmstrip_h
#define _ccl_filmstrip_h

#include "ccl/gui/graphics/imaging/image.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/text/cstring.h"

namespace CCL {

//************************************************************************************************
// Filmstrip
/** Image containing multiple frames. */
//************************************************************************************************

class Filmstrip: public Image
{
public:
	DECLARE_CLASS (Filmstrip, Image)

	enum FrameMode { kHorizontalMode, kVerticalMode, kTableMode };

	Filmstrip (Image* sourceImage = nullptr, int frames = 1, FrameMode mode = kVerticalMode);
	~Filmstrip ();
	
	PROPERTY_VARIABLE (double, duration, Duration) ///< optional duration in seconds

	FrameMode getFrameMode () const {return frameMode;}
	bool parseFrameNames (StringRef string);
	StringID getFrameName (int index) const;
	void setFrameName (int index, StringID name);

	Image* getSubFrame (StringID name); ///< get frame as image (owned by filmstrip)

	// Image
	ImageType CCL_API getType () const override;
	int CCL_API getFrameCount () const override;
	int CCL_API getCurrentFrame () const override;
	void CCL_API setCurrentFrame (int frameIndex) override;
	int CCL_API getFrameIndex (StringID name) const override;
	Image* getOriginalImage (Rect& originalRect, bool deep = false) override;
	tresult draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode = nullptr) override;
	tresult tile (GraphicsDevice& graphics, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins) override;

protected:

	Image* sourceImage;
	FrameMode frameMode;
	int frameCount;
	int currentFrame;
	int tableRowCount;
	int tableColumnCount;
	Vector<MutableCString> frameNames;
	ObjectArray subImages;

	void setFrameCount (int frames);
	void getFrameRect (Rect& frameRect, int frameIndex) const;
	void getFrameRect (RectF& frameRect, int frameIndex) const;
};

} // namespace CCL

#endif // _ccl_filmstrip_h
