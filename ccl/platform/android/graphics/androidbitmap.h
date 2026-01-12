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
// Filename    : ccl/platform/android/graphics/androidbitmap.h
// Description : Android Bitmap
//
//************************************************************************************************

#ifndef _ccl_androidbitmap_h
#define _ccl_androidbitmap_h

#include "ccl/platform/android/cclandroidjni.h"

#include "ccl/gui/graphics/nativegraphics.h"

#include "core/public/corestream.h"
#include "core/public/corebuffer.h"

namespace CCL {

interface IMemoryStream;

namespace Android {

class FrameworkGraphics;

//************************************************************************************************
// JavaBitmap
//************************************************************************************************

class JavaBitmap: public JniObject
{
public:
	JavaBitmap (JNIEnv* jni, jobject object);
	JavaBitmap ();

	void drawDirect (FrameworkGraphics& device, RectRef src, float scaleFactor);

	tresult draw (FrameworkGraphics& device, PointFRef pos, const ImageMode* mode, float scaleFactor);
	tresult draw (FrameworkGraphics& device, RectRef src, RectRef dst, const ImageMode* mode, float scaleFactor);
	tresult tile (FrameworkGraphics& device, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins, float scaleFactor);
	tresult lockBits (BitmapLockData& data, IBitmap::PixelFormat format, int mode);
	tresult unlockBits (BitmapLockData& data);
	tresult scrollPixelRect (const Rect& rect, const Point& delta);
};

//************************************************************************************************
// AndroidBitmap
//************************************************************************************************

class AndroidBitmap: public NativeBitmap
{
public:
	DECLARE_CLASS_ABSTRACT (AndroidBitmap, NativeBitmap)

	AndroidBitmap (IBitmapDecoder* customDecoder);
	AndroidBitmap (JNIEnv* jni, jobject object);
	~AndroidBitmap ();

	void drawDirect (FrameworkGraphics& device, RectRef src);
	JavaBitmap* getJavaBitmap ();

	// NativeBitmap
	tresult draw (NativeGraphicsDevice& device, PointRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (NativeGraphicsDevice& device, PointFRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (NativeGraphicsDevice& device, RectRef src, RectRef dst, const ImageMode* mode = nullptr) override;
	tresult draw (NativeGraphicsDevice& device, RectFRef src, RectFRef dst, const ImageMode* mode = nullptr) override;
	tresult tile (NativeGraphicsDevice& device, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins) override;
	PixelFormat CCL_API getPixelFormat () const override;
	tresult CCL_API lockBits (BitmapLockData& data, IBitmap::PixelFormat format, int mode) override;
	tresult CCL_API unlockBits (BitmapLockData& data) override;
	tresult CCL_API scrollPixelRect (const Rect& rect, const Point& delta) override;

private:
	AutoPtr<IBitmapDecoder> customDecoder;
	Core::IO::Buffer pixelBuffer;			// decoded pixels in native memory
	JavaBitmap javaBitmap;					// android Bitmap living on the java side
};

} // namespace Android
} // namespace CCL

#endif // _ccl_androidbitmap_h
