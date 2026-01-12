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
// Filename    : ccl/platform/shared/skia/skiadevice.cpp
// Description : Skia Device
//
//************************************************************************************************

#define DEBUG_LOG 0

#define HIGHLIGHT_CLIP 0 && DEBUG

#include "ccl/platform/shared/skia/skiadevice.h"
#include "ccl/platform/shared/skia/skiapath.h"
#include "ccl/platform/shared/skia/skiabitmap.h"
#include "ccl/platform/shared/skia/skiagradient.h"
#include "ccl/platform/shared/skia/skiatextlayout.h"

#include "ccl/public/gui/graphics/brush.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaDeviceState::SkiaDeviceState ()
: canvas (nullptr)
{}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaDeviceState::init (SkCanvas* _canvas)
{
	canvas = _canvas;
	save ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaDeviceState::save ()
{
	SOFT_ASSERT (canvas, "Trying to save device state without a valid canvas.")
	if(canvas)
		canvas->save ();
	savedState = paint;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaDeviceState::restore ()
{
	SOFT_ASSERT (canvas, "Trying to restore device state without a valid canvas.")
	if(canvas)
		canvas->restore ();
	paint = savedState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkiaDeviceState::isAntiAlias () const
{
	return paint.isAntiAlias ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaDeviceState::setAntiAlias (bool state)
{
	paint.setAntiAlias (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaDeviceState::setPen (PenRef pen)
{
	const Color& cclColor = pen.getColor ();
	SkColor4f color;
	color.fR = cclColor.getRedF ();
	color.fG = cclColor.getGreenF ();
	color.fB = cclColor.getBlueF ();
	color.fA = cclColor.getAlphaF ();

	paint.setColor (color);
	paint.setShader (nullptr);
	paint.setStrokeWidth (pen.getWidth ());
	
	SkPaint::Cap cap = SkPaint::kDefault_Cap;
	switch(pen.getLineCap ())
	{
  		case Pen::kLineCapButt:
			cap = SkPaint::kButt_Cap;
	    break;
  		case Pen::kLineCapSquare:
			cap = SkPaint::kSquare_Cap;
  		break;
  		case Pen::kLineCapRound:
			cap = SkPaint::kRound_Cap;
  		break;
	}
	paint.setStrokeCap (cap);
	
	SkPaint::Join join = SkPaint::kDefault_Join;
	switch(pen.getLineJoin ())
	{
  		case Pen::kLineJoinBevel:
			join = SkPaint::kBevel_Join;
	    break;
  		case Pen::kLineJoinMiter:
			join = SkPaint::kMiter_Join;
  		break;
  		case Pen::kLineJoinRound:
			join = SkPaint::kRound_Join;
  		break;
	}
	paint.setStrokeJoin (join);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaDeviceState::setBrush (BrushRef brush)
{
	if(brush.getType () == Brush::kGradient)
	{
		if(auto gradient = NativeGradient::resolveTo<SkiaGradient> (brush.getGradient ()))
		{
			paint.setColor (SK_ColorBLACK);
			paint.setShader (gradient->getGradientShader ());
		}
	}
	else if(brush.getType () == Brush::kSolid)
	{
		const Color& cclColor = brush.getColor ();
		SkColor4f color;
		color.fR = cclColor.getRedF ();
		color.fG = cclColor.getGreenF ();
		color.fB = cclColor.getBlueF ();
		color.fA = cclColor.getAlphaF ();
		
		paint.setColor (color);
		paint.setShader (nullptr);
	}
	else
	{
		ASSERT (false)
	}
}

//************************************************************************************************
// SkiaDevice
//************************************************************************************************

// ensure that CCL coordinate limits don't exceed these limits for Skia
static constexpr SkScalar kMinScalar = (SkScalar)(-0x1000000);
static constexpr SkScalar kMaxScalar = (SkScalar)( 0x0FFF000);

static_assert (kMaxCoord <= kMaxScalar, "kMaxCoord exceeds skia limit");
static_assert (kMinCoord >= kMinScalar, "kMinCoord exceeds skia limit");

//////////////////////////////////////////////////////////////////////////////////////////////////

SkRect& SkiaDevice::toSkRect (SkRect& dst, const Rect& src)
{
	return toSkRect (dst, rectIntToF (src));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkRect& SkiaDevice::toSkRect (SkRect& dst, const RectF& src)
{
	dst.setLTRB (src.left, src.top, src.right, src.bottom);
    return dst;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkPoint& SkiaDevice::toSkPoint (SkPoint& dst, const Point& src)
{
	return toSkPoint (dst, pointIntToF (src));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkPoint& SkiaDevice::toSkPoint (SkPoint& dst, const PointF& src)
{
	dst.set (src.x, src.y);
	return dst;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::Rect& SkiaDevice::fromSkRect (CCL::Rect& dst, const SkRect& src)
{
	dst = CCL::Rect (coordFToInt (src.fLeft), coordFToInt (src.fTop), coordFToInt (src.fRight), coordFToInt (src.fBottom));
	return  dst;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RectF& SkiaDevice::fromSkRect (RectF& dst, const SkRect& src)
{
	dst = CCL::RectF (src.fLeft, src.fTop, src.fRight, src.fBottom);
	return dst;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (SkiaDevice, NativeGraphicsDevice)

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaDevice::initialize ()
{
	SkCanvas* canvas = getCanvas ();
	//ASSERT(canvas)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaDevice::setOrigin (PointRef point)
{
	SkCanvas* canvas = getCanvas ();
	if(!canvas)
		return;
		
	canvas->translate ((SkScalar)(point.x - origin.x), (SkScalar)(point.y - origin .y));
	NativeGraphicsDevice::setOrigin (point);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaDevice::flushStock ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::saveState ()
{
	state.save ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::restoreState ()
{
	state.restore ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::addClip (RectRef rect)
{
	return addClip (rectIntToF (rect));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
tresult CCL_API SkiaDevice::addClip (RectFRef rect)
{
	SkCanvas* canvas = getCanvas ();
	if(!canvas)
		return kResultFailed;
		
	const SkRect& clipRect = SkRect::MakeLTRB (rect.left, rect.top, rect.right, rect.bottom);
	canvas->clipRect (clipRect, true);
	
	#if HIGHLIGHT_CLIP
	Pen redPen (Colors::kRed);
	drawRect (rect, redPen);
	#endif
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::addClip (IGraphicsPath* path)
{
	SkiaPath* skiaPath = unknown_cast<SkiaPath> (path);
	if(!skiaPath)
		return kResultUnexpected;

	SkCanvas* canvas = getCanvas ();
	if(!canvas)
		return kResultFailed;
		
	canvas->clipPath (skiaPath->getSkPath (), true);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::addTransform (TransformRef t)
{
	SkCanvas* canvas = getCanvas ();
	if(!canvas)
		return kResultFailed;
		
	canvas->concat (SkMatrix::MakeAll (t.a0, t.b0, t.t0, t.a1, t.b1, t.t1 , 0, 0, 1));

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::setMode (int mode)
{
	state.setAntiAlias (mode & kAntiAlias);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SkiaDevice::getMode ()
{
	return state.isAntiAlias () ? kAntiAlias : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::clearRect (RectRef rect)
 {
	return clearRect (rectIntToF (rect));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::clearRect (RectFRef rect)
{
	SkCanvas* canvas = getCanvas ();
	if(!canvas)
		return kResultFailed;
		
	SkPaint paint = state.getPaint ();
	paint.setStyle (SkPaint::kFill_Style);
	paint.setBlendMode (SkBlendMode::kClear);
	SkRect skRect;
	canvas->drawRect (toSkRect (skRect, rect), paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::fillRect (RectRef rect, BrushRef brush)
{
	return fillRect (rectIntToF (rect), brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::fillRect (RectFRef rect, BrushRef brush)
{
	SkCanvas* canvas = getCanvas ();
	if(!canvas)
		return kResultFailed;
		
	state.setBrush (brush);
	SkPaint paint = state.getPaint ();
	paint.setStyle (SkPaint::kFill_Style);
	SkRect skRect;
	toSkRect (skRect, rect);
	canvas->drawRect (skRect, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::drawRect (RectRef rect, PenRef pen)
{
	return drawRect (rectIntToF (rect), pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::drawRect (RectFRef rect, PenRef pen)
{
	SkCanvas* canvas = getCanvas ();
	if(!canvas)
		return kResultFailed;
		
	state.setPen (pen);
	SkPaint paint = state.getPaint ();
	paint.setStyle (SkPaint::kStroke_Style);

	SkRect skRect;
	toSkRect (skRect, rect);
	skRect.inset (0.5f, 0.5f);
	
	canvas->drawRect (skRect, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::drawLine (PointRef p1, PointRef p2, PenRef pen)
{
	return drawLine (pointIntToF (p1), pointIntToF (p2), pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::drawLine (PointFRef p1, PointFRef p2, PenRef pen)
{
	SkCanvas* canvas = getCanvas ();
	if(!canvas)
		return kResultFailed;
		
	state.setPen (pen);
	SkPaint paint = state.getPaint ();

	SkPoint sP1;
	toSkPoint(sP1, p1);
	SkPoint sP2;
	toSkPoint(sP2, p2);
	
	bool isVertical = sP2.x () == sP1.x ();
	bool isHorizontal = sP2.y () == sP1.y ();

	if(state.isAntiAlias ())
	{
		if(isHorizontal)
		{
			sP1.offset (0.0f, 0.5f);
			sP2.offset (0.0f, 0.5f);
		}
		else if(isVertical)
		{
			sP1.offset (0.5f, 0.0f);
			sP2.offset (0.5f, 0.0f);
		}
		else
		{
			sP1.offset (0.5f, 0.5f);
			sP2.offset (0.5f, 0.5f);
		}
	}
	else
	{
		if(isHorizontal)
		{
			sP1.offset (0.0f, 0.5f);
			sP2.offset (0.5f, 0.5f);
		}
		else if(isVertical)
		{
			sP1.offset (0.5f, 0.0f);
			sP2.offset (0.5f, 0.5f);
		}
		else
		{
			sP1.offset (0.5f, 0.5f);
			sP2.offset (0.5f, 0.5f);
		}
	}

	canvas->drawLine (sP1, sP2, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::drawEllipse (RectRef rect, PenRef pen)
{
	return drawEllipse (rectIntToF (rect), pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::drawEllipse (RectFRef rect, PenRef pen)
{
	SkCanvas* canvas = getCanvas ();
	if(!canvas)
		return kResultFailed;
		
	state.setPen (pen);
	AntiAliasSetter smoother (*this); // enable anti-aliasing
	SkPaint paint = state.getPaint ();
	paint.setStyle (SkPaint::kStroke_Style);
	SkRect skRect;
	toSkRect (skRect, rect);

	canvas->drawOval (skRect, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::fillEllipse (RectRef rect, BrushRef brush)
{
	return fillEllipse (rectIntToF (rect), brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::fillEllipse (RectFRef rect, BrushRef brush)
{
	SkCanvas* canvas = getCanvas ();
	if(!canvas)
		return kResultFailed;
		
	state.setBrush (brush);
	AntiAliasSetter smoother (*this); // enable anti-aliasing
	SkPaint paint = state.getPaint ();
	paint.setStyle (SkPaint::kFill_Style);
	SkRect skRect;
	toSkRect (skRect, rect);

	canvas->drawOval (skRect, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::drawString (RectRef rect, StringRef string, FontRef font, BrushRef brush, AlignmentRef alignment)
{
    SkiaTextLayout layout;
    layout.construct (string, rect.getWidth (), rect.getHeight (), font, ITextLayout::kSingleLine, TextFormat (alignment));
    
    ClipSetter cs (*this, rect);
	tresult result = drawTextLayout (pointIntToF (rect.getLeftTop ()), &layout, brush);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::drawString (RectFRef rect, StringRef string, FontRef font, BrushRef brush, AlignmentRef alignment)
{
	SkiaTextLayout layout;
    layout.construct (string, rect.getWidth (), rect.getHeight (), font, ITextLayout::kSingleLine, TextFormat (alignment));

    ClipSetter cs (*this, rect);
    tresult result = drawTextLayout (rect.getLeftTop (), &layout, brush);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::drawString (PointRef point, StringRef string, FontRef font, BrushRef brush, int options)
{
    return drawString (pointIntToF (point), string, font, brush, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::drawString (PointFRef point, StringRef string, FontRef font, BrushRef brush, int options)
{
    SkiaTextLayout layout;
    layout.construct (string, 0, 0, font, ITextLayout::kSingleLine, TextFormat (Alignment::kLeftTop));

    return drawTextLayout (point, &layout, brush, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::drawText (RectRef rect, StringRef string, FontRef font, BrushRef brush, TextFormatRef format)
{
    SkiaTextLayout layout;
    layout.construct (string, rect.getWidth (), rect.getHeight (), font, ITextLayout::kMultiLine, format);
  
	ClipSetter cs (*this, rect);
	tresult result = drawTextLayout (rect.getLeftTop (), &layout, brush);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::drawText (RectFRef rect, StringRef string, FontRef font, BrushRef brush, TextFormatRef format)
{
    SkiaTextLayout layout;
    layout.construct (string, rect.getWidth (), rect.getHeight (), font, ITextLayout::kMultiLine, format);
    
	ClipSetter cs (*this, rect);
	tresult result = drawTextLayout (rect.getLeftTop (), &layout, brush);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::drawTextLayout (PointRef pos, ITextLayout* textLayout, BrushRef brush, int options)
{
    return drawTextLayout (pointIntToF (pos), textLayout, brush, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::drawTextLayout (PointFRef pos, ITextLayout* textLayout, BrushRef brush, int options)
{
    SkCanvas* canvas = getCanvas ();
	if(!canvas)
		return kResultFailed;
		
	SkiaTextLayout* layout = unknown_cast<SkiaTextLayout> (textLayout);
	if(!layout)
		return SuperClass::drawTextLayout (pos, textLayout, brush, options);
	
	Color textColor;
	if(const SolidBrush* solidBrush = SolidBrush::castRef (brush))
		textColor = solidBrush->getColor ();
 
	PointF position (pos);
	if(options & kDrawAtBaseline)
	{
		PointF baseLine;
		layout->getBaselineOffset (baseLine);
		position -= baseLine;
	}
		
    layout->draw (*canvas, position, textColor);
    
    return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::measureText (Rect& size, Coord lineWidth, StringRef string, FontRef font)
{
    SkiaTextLayout layout;
	TextFormat format (Alignment::kLeftTop);
    format.isWordBreak (true);
    layout.construct (string, lineWidth, kMaxCoord, font, ITextLayout::kMultiLine, format);
            
    return layout.getBounds (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::measureText (RectF& size, CoordF lineWidth, StringRef string, FontRef font)
{
    SkiaTextLayout layout;
    TextFormat format (Alignment::kLeftTop);
    format.isWordBreak (true);
    layout.construct (string, lineWidth, (CoordF)kMaxCoord, font, ITextLayout::kMultiLine, format);
            
    return layout.getBounds (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::measureString (Rect& size, StringRef text, FontRef font)
{
    SkiaTextLayout layout;
    TextFormat format (Alignment::kLeftTop);
    layout.construct (text, kMaxCoord, kMaxCoord, font, ITextLayout::kSingleLine, format);
    
    return layout.getBounds (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaDevice::measureString (RectF& size, StringRef text, FontRef font)
{
    SkiaTextLayout layout;
    TextFormat format (Alignment::kLeftTop);
    layout.construct (text, (CoordF)kMaxCoord, (CoordF)kMaxCoord, font, ITextLayout::kSingleLine, format);
    
    return layout.getBounds (size);
}

//************************************************************************************************
// SkiaScopedGraphicsDevice
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkiaScopedGraphicsDevice, SkiaDevice)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaScopedGraphicsDevice::SkiaScopedGraphicsDevice (SkiaRenderTarget& _target, IUnknown& targetUnknown)
: target (_target),
  targetUnknown (targetUnknown)
{
	targetUnknown.retain ();
	
	SkCanvas* canvas = getCanvas ();
	state.init (canvas);

	initialize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaScopedGraphicsDevice::~SkiaScopedGraphicsDevice ()
{
	state.restore ();
	
	targetUnknown.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* SkiaScopedGraphicsDevice::getCanvas () const
{
	SkCanvas* canvas = target.getCanvas ();
	SOFT_ASSERT (canvas, "Invalid canvas.")
	return canvas;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API SkiaScopedGraphicsDevice::getContentScaleFactor () const
{
	return target.getContentScaleFactor ();
}
