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
// Filename    : ccl/platform/shared/skia/skiaengine.h
// Description : Skia Engine
//
//************************************************************************************************

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/platform/shared/skia/skiaglue.h"

namespace CCL {

class SkiaRenderTarget;

//************************************************************************************************
// SkiaEngine
//************************************************************************************************

class SkiaEngine: public NativeGraphicsEngine
{
public:
	DECLARE_CLASS_ABSTRACT (SkiaEngine, NativeGraphicsEngine)
	
	static SkiaEngine* getInstance ();

	virtual const SkShaper& getShaper ();
	virtual GrRecordingContext* getGPUContext ();

	// NativeGraphicsEngine
	bool startup () override;
	NativeWindowRenderTarget* createRenderTarget (Window* window) override;
	NativeGraphicsPath* createPath (IGraphicsPath::TypeHint type) override;
	NativeGradient* createGradient (IGradient::TypeHint type) override;
	virtual NativeBitmap* createBitmap (int width, int height, IBitmap::PixelFormat pixelFormat, float contentScaleFactor = 1.f) override;
	virtual NativeBitmap* loadBitmap (IStream& stream, const FileType& format) override;
	bool saveBitmap (IStream& stream, NativeBitmap& bitmap, const FileType& format, const IAttributeList* encoderOptions = nullptr) override;
	NativeGraphicsDevice* createWindowDevice (Window* window, void* systemDevice = nullptr) override;
	NativeGraphicsDevice* createBitmapDevice (NativeBitmap* bitmap) override;
	virtual NativeBitmap* createScreenshotFromWindow (Window* window) override;
	ITextLayout* createTextLayout () override;
	virtual bool hasGraphicsLayers () override;
	virtual IGraphicsLayer* createGraphicsLayer (UIDRef classID) override;
	IFontTable* collectFonts (int flags) override;

protected:
	std::unique_ptr<SkShaper> shaper;
	
	virtual NativeGraphicsDevice* createdScopedDevice (SkiaRenderTarget* target, IUnknown& targetUnknown);
};

} // namespace CCL
