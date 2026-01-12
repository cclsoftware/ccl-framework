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
// Filename    : ccl/platform/cocoa/quartz/engine.h
// Description : Quartz Engine
//
//************************************************************************************************

#include "ccl/gui/graphics/nativegraphics.h"

namespace CCL {
namespace MacOS {
		
//************************************************************************************************
// QuartzEngine
//************************************************************************************************

class QuartzEngine: public NativeGraphicsEngine
{
public:
	// NativeGraphicsEngine
	bool startup ();
	void shutdown ();
	NativeWindowRenderTarget* createRenderTarget (Window* window);
	NativeGraphicsPath* createPath (IGraphicsPath::TypeHint type);
	NativeGradient* createGradient (IGradient::TypeHint type);
	NativeBitmap* createBitmap (int width, int height, IBitmap::PixelFormat pixelFormat, float contentScaleFactor = 1.f);
	NativeBitmap* createOffscreen (int width, int height, IBitmap::PixelFormat pixelFormat, bool global, Window* window = nullptr);
	NativeBitmap* loadBitmap (IStream& stream, const FileType& format);
	bool saveBitmap (IStream& stream, NativeBitmap& bitmap, const FileType& format, const IAttributeList* encoderOptions = nullptr);
	NativeGraphicsDevice* createWindowDevice (Window* window, void* systemDevice = nullptr);
	NativeGraphicsDevice* createBitmapDevice (NativeBitmap* bitmap);
	bool hasGraphicsLayers ();
	IGraphicsLayer* createGraphicsLayer (UIDRef classID);
	ITextLayout* createTextLayout ();
	NativeBitmap* createScreenshotFromWindow (Window* window);
	Object* createPrintJob ();
	IFontTable* collectFonts (int flags);
	INative3DSupport* get3DSupport ();
};

} // namespace MacOS
} // namespace CCL

