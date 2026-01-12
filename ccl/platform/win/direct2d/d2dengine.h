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
// Filename    : ccl/platform/win/direct2d/d2dengine.h
// Description : Direct2D Engine
//
//************************************************************************************************

#ifndef _ccl_direct2d_engine_h
#define _ccl_direct2d_engine_h

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/platform/win/direct2d/d3dsupport.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// Direct2DEngine
//************************************************************************************************

class Direct2DEngine: public NativeGraphicsEngine,
					  public D3DSupport
{
public:
	DECLARE_CLASS_ABSTRACT (Direct2DEngine, NativeGraphicsEngine)

	Direct2DEngine ();

	// NativeGraphicsEngine
	bool startup () override;
	void shutdown () override;
	void recoverFromError () override;
	NativeWindowRenderTarget* createRenderTarget (Window* window) override;
	NativeGraphicsPath* createPath (IGraphicsPath::TypeHint type) override;
	NativeGradient* createGradient (IGradient::TypeHint type) override;
	NativeBitmap* createBitmap (int width, int height, IBitmap::PixelFormat pixelFormat, float contentScaleFactor = 1.f) override;
	NativeBitmap* createOffscreen (int width, int height, IBitmap::PixelFormat pixelFormat, bool global, Window* window = nullptr) override;
	NativeBitmap* loadBitmap (IStream& stream, const FileType& format) override;
	bool saveBitmap (IStream& stream, NativeBitmap& bitmap, const FileType& format, const IAttributeList* encoderOptions = nullptr) override;
	NativeGraphicsDevice* createWindowDevice (Window* window, void* systemDevice = nullptr) override;
	NativeGraphicsDevice* createBitmapDevice (NativeBitmap* bitmap) override;
	NativeBitmap* createScreenshotFromWindow (Window* window) override;
	ITextLayout* createTextLayout () override;
	bool installFontFromMemory (const void* data, uint32 dataSize, StringRef name, int style) override;
	bool beginFontInstallation (bool state) override;
	bool hasGraphicsLayers () override;
	IGraphicsLayer* createGraphicsLayer (UIDRef classID) override;
	Object* createPrintJob () override;
	IFontTable* collectFonts (int flags) override;
	INative3DSupport* get3DSupport () override { return this; }

protected:
	bool startupRequired ();
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_direct2d_engine_h
