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
// Filename    : ccl/platform/win/direct2d/d2ddevice.h
// Description : Direct2D Graphics Device
//
//************************************************************************************************

#ifndef _ccl_direct2d_device_h
#define _ccl_direct2d_device_h

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/platform/win/direct2d/d2dclipper.h"
#include "ccl/platform/win/direct2d/d2dtextlayout.h"
#include "ccl/platform/win/interfaces/iwin32graphics.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// D2DGraphicsDevice
//************************************************************************************************

class D2DGraphicsDevice: public NativeGraphicsDevice,
						 public IWin32Graphics
{
public:
	DECLARE_CLASS_ABSTRACT (D2DGraphicsDevice, NativeGraphicsDevice)

	D2DGraphicsDevice (D2DRenderTarget& target);
	~D2DGraphicsDevice ();

	D2DRenderTarget& getTarget () { return target; }

	// NativeGraphicsDevice
	void setOrigin (PointRef point) override;
	void flushStock () override;
	float CCL_API getContentScaleFactor () const override;
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
	tresult CCL_API fillTriangle (const Point points[3], BrushRef brush) override;
	tresult CCL_API fillTriangle (const PointF points[3], BrushRef brush) override;
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

	CLASS_INTERFACES (NativeGraphicsDevice)

protected:
	friend struct D2DTextAntialiasModeSetter;

	D2DRenderTarget& target;
	D2DClipper clipper;
	D2DTextRenderer textRenderer;

	void initialize ();

	tresult drawDirectWrite (RectRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format, bool multiline);
	tresult drawDirectWrite (RectFRef _rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format, bool multiline);
	tresult drawDirectWrite (PointFRef point, StringRef text, FontRef font, BrushRef brush, int options);
	tresult measureDirectWrite (Rect& size, Coord lineWidth, StringRef text, FontRef font, bool multiline);
	tresult measureDirectWrite (RectF& size, CoordF lineWidth, StringRef text, FontRef font, bool multiline);

	struct DrawRectHelper;
	struct Line;
	void convertLine (Line& line, int penWidth, PointRef p1, PointRef p2);

	// IWin32Graphics
	HDC CCL_API getHDC () override;
	void CCL_API releaseHDC (HDC hdc, const RECT* rect = nullptr) override;
};

//************************************************************************************************
// D2DScopedGraphicsDevice
//************************************************************************************************

class D2DScopedGraphicsDevice: public D2DGraphicsDevice,
							   public D2DClientRenderDevice
{
public:
	DECLARE_CLASS_ABSTRACT (D2DScopedGraphicsDevice, D2DGraphicsDevice)

	D2DScopedGraphicsDevice (D2DRenderTarget& target, IUnknown* targetUnknown);
	~D2DScopedGraphicsDevice ();

protected:
	IUnknown* targetUnknown;

	// D2DClientRenderDevice
	void suspend (bool state) override;
};

//************************************************************************************************
// D2DTextAntialiasModeSetter
//************************************************************************************************

struct D2DTextAntialiasModeSetter
{
	D2DTextAntialiasModeSetter (D2DGraphicsDevice& device, int fontMode)
	: device (device),
	  oldMode (device.getTarget ()->GetTextAntialiasMode ()),
	  newMode (getMode (fontMode))
	{
		if(newMode != oldMode)
			device.getTarget ()->SetTextAntialiasMode (newMode);
	}

	~D2DTextAntialiasModeSetter ()
	{
		if(oldMode != newMode)
			device.getTarget ()->SetTextAntialiasMode (oldMode);
	}

	D2D1_TEXT_ANTIALIAS_MODE getMode (int mode) const
	{
		return	mode == Font::kNone ? D2D1_TEXT_ANTIALIAS_MODE_ALIASED :
				device.getTarget ().getDefaultTextAntialiasMode ();
	}

	D2DGraphicsDevice& device;
	D2D1_TEXT_ANTIALIAS_MODE oldMode;
	D2D1_TEXT_ANTIALIAS_MODE newMode;
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_direct2d_device_h
