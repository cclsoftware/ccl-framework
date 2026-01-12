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
// Filename    : ccl/platform/shared/skia/skiarendertarget.h
// Description : Skia Engine
//
//************************************************************************************************

#ifndef _ccl_skiarendertarget_h
#define _ccl_skiarendertarget_h

#include "ccl/gui/windows/window.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/gui/graphics/dpiscale.h"

#include "ccl/platform/shared/skia/skiaglue.h"

class SkCanvas;

namespace CCL {

//************************************************************************************************
// SkiaRenderTarget
//************************************************************************************************

class SkiaRenderTarget
{
public:
	virtual SkCanvas* getCanvas ();
	virtual float getContentScaleFactor () const { return 1; }
	virtual	void onSize () {};
	
	sk_sp<SkSurface> surface;
};

//************************************************************************************************
// SkiaWindowRenderTarget
//************************************************************************************************

class SkiaWindowRenderTarget: public NativeWindowRenderTarget,
							  public SkiaRenderTarget
{
public:
	DECLARE_CLASS_ABSTRACT (SkiaWindowRenderTarget, NativeWindowRenderTarget)
	
	static SkiaWindowRenderTarget* create (Window& window);
	
	// SkiaRenderTarget
	float getContentScaleFactor () const override;
	
protected:
	PixelPoint size;

	SkiaWindowRenderTarget (Window& window);
};

//************************************************************************************************
// SkiaPDFRenderTarget
//************************************************************************************************

class SkiaPDFRenderTarget: public Object,
						   public SkiaRenderTarget
{
public:
	DECLARE_CLASS_ABSTRACT (SkiaPDFRenderTarget, Object)
	
	SkiaPDFRenderTarget (IStream& stream, float width, float height);
	~SkiaPDFRenderTarget ();

	void nextPage ();
	
	// SkiaRenderTarget
	SkCanvas* getCanvas () override;
	
protected:
	class Writer : public SkWStream
	{
	public:
		Writer (IStream& _stream) : stream (_stream) {}
		
		// SkWStream
		bool write (const void* buffer, size_t size) { return stream.write (buffer, static_cast<int> (size)) == size; }
		size_t bytesWritten () const { return stream.tell (); }

	protected:
		IStream& stream;
	};
	
	sk_sp<SkDocument> document;
	Writer* writer;
	SkPDF::Metadata pdfMetaData;
	SkCanvas* canvas;
	float width;
	float height;
};

} // namespace CCL

#endif // ccl_skiarendertarget_h
