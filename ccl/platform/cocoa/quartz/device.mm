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
// Filename    : ccl/platform/cocoa/quartz/device.mm
// Description : Quartz Device
//
//************************************************************************************************

#define DEBUG_LOG 0

#define HIGHLIGHT_CLIP 0 && DEBUG

#include "ccl/platform/cocoa/quartz/device.h"
#include "ccl/platform/cocoa/quartz/path.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"
#include "ccl/platform/cocoa/quartz/quartztextlayout.h"
#include "ccl/platform/cocoa/quartz/fontcache.h"
#include "ccl/platform/cocoa/quartz/gradient.h"

#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/gui/window.mac.h"

#include "ccl/public/gui/graphics/brush.h"

using namespace CCL;
using namespace MacOS;
	
//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzDeviceState::QuartzDeviceState ()
: context (NULL)
{
	setDirty ();
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzDeviceState::init (CGContextRef _context)
{
	context = _context;
	save ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzDeviceState::save ()
{
	ASSERT (context)
	if(context)
		::CGContextSaveGState (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzDeviceState::restore ()
{
	ASSERT (context)
	if(context)
		::CGContextRestoreGState (context);
	setDirty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzDeviceState::setDirty ()
{
	dirtyFlags = -1;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzDeviceState::setPen (PenRef pen)
{
	if(isPenDirty () || currentPen.getColor () != pen.getColor ())
	{
		const Color& color = pen.getColor ();
		CGColorRef cgColor = [COLORTYPE colorWithRed:color.getRedF () green:color.getGreenF () blue:color.getBlueF () alpha:color.getAlphaF ()].CGColor;
		::CGContextSetStrokeColorWithColor (context, cgColor);
	}
	
	if(isPenDirty () || currentPen.getWidth () != pen.getWidth ())
		::CGContextSetLineWidth (context, pen.getWidth ());

	if(isPenDirty () || currentPen.getLineCap () != pen.getLineCap ())
	{
		::CGLineCap lineCap = ::kCGLineCapButt;
		switch(pen.getLineCap ())
		{
			case Pen::kLineCapButt:	  lineCap = ::kCGLineCapButt; break;
			case Pen::kLineCapSquare: lineCap = ::kCGLineCapSquare; break;
			case Pen::kLineCapRound:  lineCap = ::kCGLineCapRound; break;
		}
		::CGContextSetLineCap (context, lineCap);
	}

	if(isPenDirty () || currentPen.getLineJoin () != pen.getLineJoin ())
	{
		::CGLineJoin lineJoin = ::kCGLineJoinMiter;
		switch(pen.getLineJoin ())
		{
			case Pen::kLineJoinMiter: lineJoin = ::kCGLineJoinMiter; break;
			case Pen::kLineJoinBevel: lineJoin = ::kCGLineJoinBevel; break;
			case Pen::kLineJoinRound: lineJoin = ::kCGLineJoinRound; break;
		}
		::CGContextSetLineJoin (context, lineJoin);
	}

	isPenDirty (false);
	currentPen = pen;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzDeviceState::setBrush (BrushRef brush)
{
	const SolidBrush* solidBrush = SolidBrush::castRef (brush);
	SOFT_ASSERT (solidBrush, "gradient brushes are only supported for fillRect and fillEllipse")

	if(solidBrush && (isBrushDirty () || currentBrush.getColor () != solidBrush->getColor ()))
	{
		const Color& color = solidBrush->getColor ();
		CGColorRef cgColor = [COLORTYPE colorWithRed:color.getRedF () green:color.getGreenF () blue:color.getBlueF () alpha:color.getAlphaF ()].CGColor;
		::CGContextSetFillColorWithColor (context, cgColor);

		currentBrush = *solidBrush;
		isBrushDirty (false);
	}
}

//************************************************************************************************
// QuartzDevice
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (QuartzDevice, NativeGraphicsDevice)

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzDevice::QuartzDevice (QuartzRenderTarget& _target)
: target (_target)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzDevice::~QuartzDevice ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzDevice::initialize ()
{
	CGContextRef context = getTarget ().getContext ();
	ASSERT (context)
	::CGContextSetShouldAntialias (context, false);
	::CGContextSetInterpolationQuality(context, kCGInterpolationMedium);
	CGContextSetTextMatrix (context, CGAffineTransformIdentity);
	antiAlias = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzDevice::setOrigin (PointRef point)
{
	CGContextRef context = getTarget ().getContext ();
	::CGContextTranslateCTM (context, (float)(point.x - origin.x), (float)(point.y - origin .y));
	NativeGraphicsDevice::setOrigin (point);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API QuartzDevice::getContentScaleFactor () const
{
	return getTarget ().getContentScaleFactor ();	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::saveState ()
{
	state.save ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::restoreState ()
{
	state.restore ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::addClip (RectRef rect)
{
	return addClip (rectIntToF (rect));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
tresult CCL_API QuartzDevice::addClip (RectFRef rect)
{
	CCL_PRINTF ("addClip    %f %f %f %f\n", rect.left, rect.top, rect.getWidth (), rect.getHeight ())

	CGRect rect2;
	toCGRect (rect2, rect);
	CGContextRef context = getTarget ().getContext ();
	::CGContextClipToRect (context, rect2);
	#if HIGHLIGHT_CLIP
	Pen redPen (Colors::kRed);
	drawRect (rect, redPen);
	#endif
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::addClip (IGraphicsPath* path)
{
	QuartzPath* qPath = unknown_cast<QuartzPath> (path);
	if(!qPath)
		return kResultUnexpected;

	CGContextRef context = getTarget ().getContext ();
	::CGContextBeginPath (context);
	::CGContextAddPath (context, qPath->getCGPath ());
	::CGContextClosePath (context);
	::CGContextClip (context);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::addTransform (TransformRef t)
{
	CGContextRef context = getTarget ().getContext ();
	ASSERT(context)
	::CGAffineTransform tx1 = {t.a0, t.a1, t.b0, t.b1, t.t0, t.t1};
	::CGContextConcatCTM (context, tx1);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::setMode (int mode)
{
	antiAlias = (mode & kAntiAlias) != 0;
	CGContextRef context = getTarget ().getContext ();
	::CGContextSetShouldAntialias (context, antiAlias);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API QuartzDevice::getMode ()
{
	return antiAlias ? kAntiAlias : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::clearRect (RectRef rect)
{
	return clearRect (rectIntToF (rect));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::clearRect (RectFRef rect)
{
	CGRect rect2;
	toCGRect (rect2, rect);
	CGContextRef context = getTarget ().getContext ();
	::CGContextClearRect (context, rect2);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::fillRect (RectRef rect, BrushRef brush)
{
	return fillRect (rectIntToF (rect), brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::fillRect (RectFRef rect, BrushRef brush)
{
	CGContextRef context = getTarget ().getContext ();

	if(brush.getType () == Brush::kGradient)
	{
		if(auto gradient = NativeGradient::resolveTo<QuartzGradient> (brush.getGradient ()))
		{
			saveState ();
			addClip (rect);
			gradient->draw (context);
			restoreState ();
		}
	}
	else if(brush.getType () == Brush::kSolid)
	{
		state.setBrush (brush);
		
		CGRect rect2;
		toCGRect (rect2, rect);
		::CGContextFillRect (context, rect2);
	}
	else
		return kResultNotImplemented;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::drawRect (RectRef rect, PenRef pen)
{
	state.setPen (pen);
	int penWidth = ccl_to_int (pen.getWidth ());
	bool isPenEven = (penWidth % 2) == 0;
	bool antiAliasMode = getMode () == kAntiAlias;
		
	CGRect rect2;
	toCGRect (rect2, rect);
	rect2.origin.y += 0.5f;
	rect2.origin.x += 0.5f;
	rect2.size.height -= 1;
	rect2.size.width -= 1;
	CGContextRef context = getTarget ().getContext ();
	
	if(antiAliasMode || getContentScaleFactor () >= 2)
	{
		if(penWidth == 1)
		{
			CGPoint rectangle[8];
			rectangle[0].x = rect2.origin.x  - 0.5f;
			rectangle[0].y = rect2.origin.y;
			rectangle[1].x = rect2.origin.x + rect2.size.width + 0.5f;
			rectangle[1].y = rect2.origin.y;
			
			rectangle[2].x = rect2.origin.x + rect2.size.width;
			rectangle[2].y = rect2.origin.y - 0.5f;
			rectangle[3].x = rect2.origin.x + rect2.size.width;
			rectangle[3].y = rect2.origin.y + rect2.size.height + 0.5f;
			
			rectangle[4].x = rect2.origin.x + rect2.size.width + 0.5f;
			rectangle[4].y = rect2.origin.y + rect2.size.height;
			rectangle[5].x = rect2.origin.x - 0.5f;
			rectangle[5].y = rect2.origin.y + rect2.size.height;
			
			rectangle[6].x = rect2.origin.x;
			rectangle[6].y = rect2.origin.y + rect2.size.height + 0.5f;
			rectangle[7].x = rect2.origin.x;
			rectangle[7].y = rect2.origin.y - 0.5f;
			
			::CGContextStrokeLineSegments (context, rectangle, 8);
			return kResultOk;
		}
		else if(penWidth > 3)
		{
			rect2.size.height += 1;
			rect2.size.width += 1;			
		}
	}
	if(!antiAliasMode && penWidth > 1 && isPenEven)
	{

		rect2.origin.y += 0.5f;
		rect2.origin.x += 0.5f;
		rect2.size.height -= 1;
		rect2.size.width -= 1;
	}

	::CGContextStrokeRectWithWidth (context, rect2, pen.getWidth ());

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::drawRect (RectFRef rect, PenRef pen)
{
	state.setPen (pen);
		
	CGRect rect2;
	toCGRect (rect2, rect);

	CGContextRef context = getTarget ().getContext ();
	
	// todo: check if any of the adjustments for integer coords above are required here

	::CGContextStrokeRectWithWidth (context, rect2, pen.getWidth ());

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::drawLine (PointRef p1, PointRef p2, PenRef pen)
{
	state.setPen (pen);
	CGFloat x1 = static_cast<CGFloat>(p1.x);
	CGFloat y1 = static_cast<CGFloat>(p1.y);
	CGFloat x2 = static_cast<CGFloat>(p2.x);
	CGFloat y2 = static_cast<CGFloat>(p2.y);

	int penWidth = ccl_to_int (pen.getWidth ());
	bool isPenEven = (penWidth % 2) == 0;
	bool isHorizontal = y2 == y1;
	bool isVertical = x2 == x1;
	
	if(isHorizontal)
	{
		y1 += 0.5;
		y2 += 0.5;
	}
	else if(isVertical)
	{
		x1 += 0.5;
		x2 += 0.5;
	}
	else
	{
		x1 += 0.5;
		y1 += 0.5;
		x2 += 0.5;
		y2 += 0.5;			
	}
	
	if(pen.getWidth () > 1.f)
	{
		
		bool antiAliasMode = getMode () == kAntiAlias;
		
		if(isHorizontal)
		{
			if(!antiAliasMode && isPenEven)
			{
				y1 += 0.5;
				y2 += 0.5;
			}
		}
		else if(isVertical)
		{
			if(!antiAliasMode && isPenEven)
			{				
				x1 += 0.5;
				x2 += 0.5;
			}				
		}
	}
	
	CGContextRef context = getTarget ().getContext ();
	CGPoint points[2] = {{x1, y1}, {x2, y2}};
	::CGContextStrokeLineSegments (context, points, 2);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::drawLine (PointFRef p1, PointFRef p2, PenRef pen)
{
	state.setPen (pen);
	CGFloat x1 = p1.x;
	CGFloat y1 = p1.y;
	CGFloat x2 = p2.x;
	CGFloat y2 = p2.y;

	// todo: check if any of the adjustments for integer coords above are required here

	CGContextRef context = getTarget ().getContext ();
	CGPoint points[2] = {{x1, y1}, {x2, y2}};
	::CGContextStrokeLineSegments (context, points, 2);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::drawEllipse (RectRef rect, PenRef pen)
{
	AntiAliasSetter smoother (*this); // enable anti-aliasing
	
	state.setPen (pen);

	::CGRect rect2;
	toCGRect (rect2, rect);
	rect2.origin.y += 0.5f;
	rect2.origin.x += 0.5f;
	rect2.size.height -= 1;
	rect2.size.width -= 1;
	CGContextRef context = getTarget ().getContext ();
	::CGContextStrokeEllipseInRect (context, rect2);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::drawEllipse (RectFRef rect, PenRef pen)
{
	AntiAliasSetter smoother (*this); // enable anti-aliasing
	
	state.setPen (pen);

	::CGRect rect2;
	toCGRect (rect2, rect);
	
	// todo: check if any of the adjustments for integer coords above are required here
	CGContextRef context = getTarget ().getContext ();
	::CGContextStrokeEllipseInRect (context, rect2);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::fillEllipse (RectRef rect, BrushRef brush)
{
	return fillEllipse (rectIntToF (rect), brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::fillEllipse (RectFRef rect, BrushRef brush)
{
	AntiAliasSetter smoother (*this); // enable anti-aliasing

	CGContextRef context = getTarget ().getContext ();

	if(brush.getType () == Brush::kGradient)
	{
		if(auto gradient = NativeGradient::resolveTo<QuartzGradient> (brush.getGradient ()))
		{
			saveState ();
			QuartzPath path;
			path.addArc (rect, 0, 360);
			addClip (&path);
			gradient->draw (context);
			restoreState ();
		}
	}
	else if(brush.getType () == Brush::kSolid)
	{
		state.setBrush (brush);

		::CGRect rect2;
		toCGRect (rect2, rect);

		CGContextRef context = getTarget ().getContext ();
		::CGContextFillEllipseInRect (context, rect2);
	}
	else
		return kResultNotImplemented;
		
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzDevice::flushStock ()
{
	// remove cached fontRefs
	FontCache::instance ().removeAll ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::drawString (RectRef rect, StringRef string, FontRef font, BrushRef brush, AlignmentRef alignment)
{
	QuartzTextLayout layout;
	layout.construct (string, rect.getWidth (), rect.getHeight (), font, ITextLayout::kSingleLine, TextFormat (alignment));
	
	return drawTextLayout (pointIntToF (rect.getLeftTop ()), &layout, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::drawString (RectFRef rect, StringRef string, FontRef font, BrushRef brush, AlignmentRef alignment)
{
	QuartzTextLayout layout;
	layout.construct (string, rect.getWidth (), rect.getHeight (), font, ITextLayout::kSingleLine, TextFormat (alignment));
	
	return drawTextLayout (rect.getLeftTop (), &layout, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::drawString (PointRef point, StringRef string, FontRef font, BrushRef brush, int options)
{
	return drawString (pointIntToF (point), string, font, brush, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::drawString (PointFRef point, StringRef string, FontRef font, BrushRef brush, int options)
{
	QuartzTextLayout layout;
	layout.construct (string, 0, 0, font, ITextLayout::kSingleLine, TextFormat (Alignment::kLeftTop));

	return drawTextLayout (point, &layout, brush, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::drawText (RectRef rect, StringRef string, FontRef font, BrushRef brush, TextFormatRef format)
{
	QuartzTextLayout layout;
	layout.construct (string, rect.getWidth (), rect.getHeight (), font, ITextLayout::kMultiLine, format);
	
	return drawTextLayout (pointIntToF (rect.getLeftTop ()), &layout, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::drawText (RectFRef rect, StringRef string, FontRef font, BrushRef brush, TextFormatRef format)
{
	QuartzTextLayout layout;
	layout.construct (string, rect.getWidth (), rect.getHeight (), font, ITextLayout::kMultiLine, format);
	
	return drawTextLayout (rect.getLeftTop (), &layout, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::drawTextLayout (PointRef pos, ITextLayout* textLayout, BrushRef brush, int options)
{
	return drawTextLayout (pointIntToF (pos), textLayout, brush, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::drawTextLayout (PointFRef _pos, ITextLayout* _textLayout, BrushRef brush, int options)
{
	QuartzTextLayout* layout = unknown_cast<QuartzTextLayout>(_textLayout);
	if(!layout)
		return SuperClass::drawTextLayout (_pos, _textLayout, brush, options);
	
	Color textColor;
	if(const SolidBrush* solidBrush = SolidBrush::castRef (brush))
		textColor = solidBrush->getColor ();

	PointF position (_pos);
	if(options & kDrawAtBaseline)
	{
		PointF baseLine;
		layout->getBaselineOffset (baseLine);
		position -= baseLine;
	}
	
	AntiAliasSetter smoother (*this);
	CGContextRef context = target.getContext ();
	layout->draw (context, position, textColor);
	state.isPenDirty (true);
	state.isBrushDirty (true);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::measureText (Rect& size, Coord lineWidth, StringRef string, FontRef font)
{
	QuartzTextLayout layout;
	TextFormat format;
	format.isWordBreak (true);
	layout.construct (string, lineWidth, kMaxCoord, font, ITextLayout::kMultiLine, format);
			
	return layout.getBounds (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::measureText (RectF& size, CoordF lineWidth, StringRef string, FontRef font)
{
	QuartzTextLayout layout;
	TextFormat format;
	format.isWordBreak (true);
	layout.construct (string, lineWidth, (CoordF)kMaxCoord, font, ITextLayout::kMultiLine, format);
			
	return layout.getBounds (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::measureString (Rect& size, StringRef text, FontRef font)
{
	QuartzTextLayout layout;
	TextFormat format;
	layout.construct (text, kMaxCoord, kMaxCoord, font, ITextLayout::kSingleLine, format);
	
	return layout.getBounds (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzDevice::measureString (RectF& size, StringRef text, FontRef font)
{
	QuartzTextLayout layout;
	TextFormat format;
	layout.construct (text, (CoordF)kMaxCoord, (CoordF)kMaxCoord, font, ITextLayout::kSingleLine, format);
	
	return layout.getBounds (size);
}


//************************************************************************************************
// QuartzScopedGraphicsDevice
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (QuartzScopedGraphicsDevice, QuartzDevice)

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzScopedGraphicsDevice::QuartzScopedGraphicsDevice (QuartzRenderTarget& target, IUnknown& targetUnknown)
: QuartzDevice (target),
  targetUnknown (targetUnknown)
{
	targetUnknown.retain ();
	
	initialize ();
	state.init (target.getContext ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzScopedGraphicsDevice::~QuartzScopedGraphicsDevice ()
{
	target.flush ();
	state.restore ();
	
	targetUnknown.release ();
}

