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
// Filename    : ccl/platform/shared/skia/skiadevice.h
// Description : Skia Device
//
//************************************************************************************************

#ifndef _ccl_skia_device_h
#define _ccl_skia_device_h

#include "ccl/platform/shared/skia/skiaglue.h"

#include "ccl/platform/shared/skia/skiarendertarget.h"

#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/base/collections/objectarray.h"

namespace CCL {

//************************************************************************************************
// SkiaDeviceState
//************************************************************************************************

class SkiaDeviceState
{
public:
	SkiaDeviceState ();

	void init (SkCanvas* canvas);
	void save ();
	void restore ();
	bool isAntiAlias () const;
	void setAntiAlias (bool state);
	const SkPaint& getPaint () const { return paint; }
	void setPen (PenRef pen);
	void setBrush (BrushRef brush);
	
protected:
	SkCanvas* canvas;
	SkPaint paint;
	SkPaint savedState;
};

//************************************************************************************************
// SkiaDevice
//************************************************************************************************

class SkiaDevice: public NativeGraphicsDevice
{
public:
	DECLARE_CLASS_ABSTRACT (SkiaDevice, NativeGraphicsDevice)

    static SkPoint& toSkPoint (SkPoint& dst, const PointF& src);
	static SkPoint& toSkPoint (SkPoint& dst, const Point& src);
	static SkRect& toSkRect (SkRect& dst, const RectF& src);
	static SkRect& toSkRect (SkRect& dst, const Rect& src);
	static RectF& fromSkRect (RectF& dst, const SkRect& src);
	static Rect& fromSkRect (Rect& dst, const SkRect& src);

	SkiaDeviceState& getState () { return state; }
	virtual SkCanvas* getCanvas () const = 0;

	// NativeGraphicsDevice
	void setOrigin (PointRef point) override;
	void flushStock () override;
	tresult CCL_API saveState () override;
	tresult CCL_API restoreState () override;
	tresult CCL_API addClip (RectRef rect) override;
	tresult CCL_API addClip (RectFRef rect) override;
	tresult CCL_API addClip (IGraphicsPath* path) override;
	tresult CCL_API addTransform (TransformRef matrix) override;
	tresult CCL_API setMode (int mode) override;
	int CCL_API getMode () override;
	tresult CCL_API clearRect (RectRef rect) override;
	tresult CCL_API clearRect (RectFRef rect) override;
	tresult CCL_API fillRect (RectRef rect, BrushRef brush) override;
	tresult CCL_API fillRect (RectFRef rect, BrushRef brush) override;
	tresult CCL_API drawRect (RectRef rect, PenRef pen) override;
	tresult CCL_API drawRect (RectFRef rect, PenRef pen) override;
	tresult CCL_API drawLine (PointRef p1, PointRef p2, PenRef pen) override;
	tresult CCL_API drawLine (PointFRef p1, PointFRef p2, PenRef pen) override;
	tresult CCL_API drawEllipse (RectRef rect, PenRef pen) override;
	tresult CCL_API drawEllipse (RectFRef rect, PenRef pen) override;
	tresult CCL_API fillEllipse (RectRef rect, BrushRef brush) override;
	tresult CCL_API fillEllipse (RectFRef rect, BrushRef brush) override;
	tresult CCL_API drawString (RectRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment = Alignment ()) override;
	tresult CCL_API drawString (RectFRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment = Alignment ()) override;
	tresult CCL_API drawString (PointRef point, StringRef text, FontRef font, BrushRef brush, int options = 0) override;
	tresult CCL_API drawString (PointFRef point, StringRef text, FontRef font, BrushRef brush, int options = 0) override;
	tresult CCL_API measureString (Rect& size, StringRef text, FontRef font) override;
	tresult CCL_API measureString (RectF& size, StringRef text, FontRef font) override;
	tresult CCL_API measureText (Rect& size, Coord lineWidth, StringRef text, FontRef font) override;
	tresult CCL_API measureText (RectF& size, CoordF lineWidth, StringRef text, FontRef font) override;
	tresult CCL_API drawText (RectRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format = TextFormat ()) override;
	tresult CCL_API drawText (RectFRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format = TextFormat ()) override;
	tresult CCL_API drawTextLayout (PointRef pos, ITextLayout* textLayout, BrushRef brush, int options = 0) override;
	tresult CCL_API drawTextLayout (PointFRef pos, ITextLayout* textLayout, BrushRef brush, int options = 0) override;

protected:
    void initialize ();

	SkiaDeviceState state;
};

//************************************************************************************************
// SkiaScopedGraphicsDevice
//************************************************************************************************

class SkiaScopedGraphicsDevice: public SkiaDevice
{
public:
	DECLARE_CLASS_ABSTRACT (SkiaScopedGraphicsDevice, SkiaDevice)

	SkiaScopedGraphicsDevice (SkiaRenderTarget& target, IUnknown& targetUnknown);
	~SkiaScopedGraphicsDevice ();

	// SkiaDevice
	SkCanvas* getCanvas () const override;
	float CCL_API getContentScaleFactor () const override;

protected:
	SkiaRenderTarget& target;
	IUnknown& targetUnknown;
};

} // namespace CCL

#endif // _ccl_skia_device_h
