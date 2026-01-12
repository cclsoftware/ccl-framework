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
// Filename    : ccl/platform/cocoa/quartz/device.h
// Description : Quartz Device
//
//************************************************************************************************

#ifndef _ccl_quartz_device_h
#define _ccl_quartz_device_h

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/platform/cocoa/quartz/quartzrendertarget.h"

#include "ccl/public/base/ccldefpush.h"
#include <CoreGraphics/CGContext.h>
#include "ccl/public/base/ccldefpop.h"

#if CCL_PLATFORM_MAC
#define COLORTYPE NSColor
#elif CCL_PLATFORM_IOS
#define COLORTYPE UIColor
#endif

namespace CCL {
namespace MacOS {

//************************************************************************************************
// QuartzDeviceState
//************************************************************************************************

class QuartzDeviceState
{
public:
	QuartzDeviceState ();

	void init (CGContextRef context);
	void save ();
	void restore ();
	CGContextRef getContext () { return context; }

	enum DirtyFlags
	{
		kBrushDirty = 1<<0,			
		kPenDirty   = 1<<1,
		kFontDirty  = 1<<2,
		kContextDirty   = kBrushDirty|kPenDirty|kFontDirty	
	};

	PROPERTY_FLAG (dirtyFlags, kBrushDirty, isBrushDirty)
	PROPERTY_FLAG (dirtyFlags, kPenDirty, isPenDirty)
	PROPERTY_FLAG (dirtyFlags, kFontDirty, isFontDirty)
	void setDirty (); ///< Context state was potentially damaged

	void setPen (PenRef pen);
	void setBrush (BrushRef brush);
	void setFont (FontRef font);
	void setTextBrush (BrushRef brush);
	void resetObjects ();
	
protected:
	CGContextRef context;
	int dirtyFlags;

	Pen currentPen;
	SolidBrush currentBrush;
	Font currentFont;
};

//************************************************************************************************
// QuartzDevice
//************************************************************************************************

class QuartzDevice: public NativeGraphicsDevice
{
public:
	DECLARE_CLASS_ABSTRACT (QuartzDevice, NativeGraphicsDevice)

	QuartzDevice (QuartzRenderTarget& _target);
	~QuartzDevice ();

	QuartzRenderTarget& getTarget () const { return target; }
	QuartzDeviceState& getState () { return state; }

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
	QuartzRenderTarget& target;
	bool antiAlias;
	QuartzDeviceState state;
	
	void initialize ();
};

//************************************************************************************************
// QuartzScopedGraphicsDevice
//************************************************************************************************

class QuartzScopedGraphicsDevice: public QuartzDevice
{
public:
	DECLARE_CLASS_ABSTRACT (QuartzScopedGraphicsDevice, QuartzDevice)

	QuartzScopedGraphicsDevice (QuartzRenderTarget& target, IUnknown& targetUnknown);
	~QuartzScopedGraphicsDevice ();

protected:
	IUnknown& targetUnknown;
};

} // namespace MacOS
} // namespace CCL

#endif // _ccl_quartz_device_h
