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
// Filename    : ccl/gui/graphics/nativegraphics.cpp
// Description : Native Graphics classes
//
//************************************************************************************************

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/gui/windows/window.h"

#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/text/iregexp.h"
#include "ccl/public/gui/graphics/iuivalue.h"
#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/system/cclerror.h"

#include "ccl/base/message.h"
#include "ccl/base/collections/objectlist.h"
#include "ccl/base/storage/configuration.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("GraphicsEngine")
	XSTRING (3DGraphicsSupportRequired, "This application requires 3D graphics support, but no 3D graphics backend could be initialized.")
END_XSTRINGS

//************************************************************************************************
// NativeGraphicsEngine
//************************************************************************************************

const Configuration::BoolValue require3DGraphicsSupport ("CCL.Graphics.3D", "Required", false);

DEFINE_CLASS_ABSTRACT_HIDDEN (NativeGraphicsEngine, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsEngine::NativeGraphicsEngine ()
: suppressErrors (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeGraphicsEngine::addCleanup (IGraphicsCleanup* object)
{
	cleanupList.append (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeGraphicsEngine::shutdown ()
{
	ListForEach (cleanupList, IGraphicsCleanup*, object)
		object->cleanupGraphics ();
	EndFor
	cleanupList.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeWindowRenderTarget* NativeGraphicsEngine::createRenderTarget (Window* window)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* NativeGraphicsEngine::createOffscreen (int width, int height, IBitmap::PixelFormat pixelFormat, bool global, Window* window)
{
	return createBitmap (width, height, pixelFormat, window ? window->getContentScaleFactor () : 1.f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextLayout* NativeGraphicsEngine::createTextLayout ()
{
	return NEW SimpleTextLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NativeGraphicsEngine::verifyFeatureSupport ()
{
	if(require3DGraphicsSupport && get3DSupport () == nullptr)
	{
		ccl_raise (XSTR (3DGraphicsSupportRequired));
		return false;
	}
	return true;
}

//************************************************************************************************
// NativeWindowRenderTarget
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (NativeWindowRenderTarget, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeWindowRenderTarget::NativeWindowRenderTarget (Window& window)
: window (window)
{}

//************************************************************************************************
// NativeGraphicsDevice
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (NativeGraphicsDevice, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::drawRoundRect (RectRef rect, Coord rx, Coord ry, PenRef pen)
{
	AutoPtr<NativeGraphicsPath> path = createPath ();
	path->addRoundRect (rect, rx, ry);
	return path->draw (*this, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::drawRoundRect (RectFRef rect, CoordF rx, CoordF ry, PenRef pen)
{
	AutoPtr<NativeGraphicsPath> path = createPath ();
	path->addRoundRect (rect, rx, ry);
	return path->draw (*this, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::fillRoundRect (RectRef rect, Coord rx, Coord ry, BrushRef brush)
{
	AutoPtr<NativeGraphicsPath> path = createPath ();
	path->addRoundRect (rect, rx, ry);
	return path->fill (*this, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::fillRoundRect (RectFRef rect, CoordF rx, CoordF ry, BrushRef brush)
{
	AutoPtr<NativeGraphicsPath> path = createPath ();
	path->addRoundRect (rect, rx, ry);
	return path->fill (*this, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::drawTriangle (const Point points[3], PenRef pen)
{
	AutoPtr<NativeGraphicsPath> path = createPath ();
	path->addTriangle (points[0], points[1], points[2]);
	return path->draw (*this, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::drawTriangle (const PointF points[3], PenRef pen)
{
	AutoPtr<NativeGraphicsPath> path = createPath ();
	path->addTriangle (points[0], points[1], points[2]);
	return path->draw (*this, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::fillTriangle (const Point points[3], BrushRef brush)
{
	AutoPtr<NativeGraphicsPath> path = createPath ();
	path->addTriangle (points[0], points[1], points[2]);
	return path->fill (*this, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::fillTriangle (const PointF points[3], BrushRef brush)
{
	AutoPtr<NativeGraphicsPath> path = createPath ();
	path->addTriangle (points[0], points[1], points[2]);
	return path->fill (*this, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API NativeGraphicsDevice::getStringWidth (StringRef text, FontRef font)
{
	Rect size;
	measureString (size, text, font); 
	return size.getWidth ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CoordF CCL_API NativeGraphicsDevice::getStringWidthF (StringRef text, FontRef font)
{
	RectF size;
	measureString (size, text, font); 
	return size.getWidth ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::drawTextLayout (PointRef pos, ITextLayout* _textLayout, BrushRef brush, int options)
{
	SimpleTextLayout* textLayout = unknown_cast<SimpleTextLayout> (_textLayout);
	if(!textLayout)
		return kResultInvalidArgument;

	Rect rect (0, 0, textLayout->getWidthInt (), textLayout->getHeightInt ());
	rect.offset (pos);
	if(textLayout->getLineMode () == ITextLayout::kSingleLine)
		return drawString (rect, textLayout->getText (), textLayout->getFont (), brush, textLayout->getFormat ().getAlignment ());
	else
		return drawText (rect, textLayout->getText (), textLayout->getFont (), brush, textLayout->getFormat ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::drawTextLayout (PointFRef pos, ITextLayout* _textLayout, BrushRef brush, int options)
{
	SimpleTextLayout* textLayout = unknown_cast<SimpleTextLayout> (_textLayout);
	if(!textLayout)
		return kResultInvalidArgument;

	RectF rect (0, 0, textLayout->getWidth (), textLayout->getHeight ());
	rect.offset (pos);
	if(textLayout->getLineMode () == ITextLayout::kSingleLine)
		return drawString (rect, textLayout->getText (), textLayout->getFont (), brush, textLayout->getFormat ().getAlignment ());
	else
		return drawText (rect, textLayout->getText (), textLayout->getFont (), brush, textLayout->getFormat ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::drawPath (IGraphicsPath* path, PenRef pen)
{
	CCL_PRINT ("NativeGraphicsDevice::drawPath must not be called!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::fillPath (IGraphicsPath* path, BrushRef brush)
{
	CCL_PRINT ("NativeGraphicsDevice::fillPath must not be called!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::drawImage (IImage* image, PointRef pos, const ImageMode* mode)
{
	CCL_PRINT ("NativeGraphicsDevice::drawImage must not be called!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::drawImage (IImage* image, PointFRef pos, const ImageMode* mode)
{
	CCL_PRINT ("NativeGraphicsDevice::drawImage must not be called!")
	return kResultNotImplemented;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

tresult NativeGraphicsDevice::drawImage (IImage* image, RectRef src, RectRef dst, const ImageMode* mode)
{
	CCL_PRINT ("NativeGraphicsDevice::drawImage must not be called!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::drawImage (IImage* image, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	CCL_PRINT ("NativeGraphicsDevice::drawImage must not be called!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsDevice::setMode (int mode)
{
	CCL_PRINT ("NativeGraphicsDevice::setMode must not be called!")
	return kResultNotImplemented;
}

//************************************************************************************************
// NullGraphicsDevice
//************************************************************************************************

DEFINE_CLASS_HIDDEN (NullGraphicsDevice, NativeGraphicsDevice)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NullGraphicsDevice::saveState () { return kResultOk; }
tresult CCL_API NullGraphicsDevice::restoreState () { return kResultOk; }
tresult CCL_API NullGraphicsDevice::addClip (RectRef rect) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::addClip (RectFRef rect) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::addClip (IGraphicsPath* path) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::addTransform (TransformRef matrix) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::setMode (int mode) { return kResultOk; }
int CCL_API NullGraphicsDevice::getMode () { return 0; }
tresult CCL_API NullGraphicsDevice::clearRect (RectRef rect) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::clearRect (RectFRef rect) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::fillRect (RectRef rect, BrushRef brush) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::fillRect (RectFRef rect, BrushRef brush) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::drawRect (RectRef rect, PenRef pen) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::drawRect (RectFRef rect, PenRef pen) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::drawLine (PointRef p1, PointRef p2, PenRef pen) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::drawLine (PointFRef p1, PointFRef p2, PenRef pen) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::drawEllipse (RectRef rect, PenRef pen) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::drawEllipse (RectFRef rect, PenRef pen) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::fillEllipse (RectRef rect, BrushRef brush) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::fillEllipse (RectFRef rect, BrushRef brush) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::drawString (RectRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::drawString (RectFRef rect, StringRef text, FontRef font, BrushRef brush, AlignmentRef alignment) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::drawString (PointRef point, StringRef text, FontRef font, BrushRef brush, int options) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::drawString (PointFRef point, StringRef text, FontRef font, BrushRef brush, int options) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::drawText (RectRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format) { return kResultOk; }
tresult CCL_API NullGraphicsDevice::drawText (RectFRef rect, StringRef text, FontRef font, BrushRef brush, TextFormatRef format) { return kResultOk; }

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NullGraphicsDevice::measureString (RectF& size, StringRef text, FontRef font)
{
	size.left = size.top = 0;
	size.right = text.length () * font.getSize () * .5f;
	size.bottom = font.getSize () + 2.f;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NullGraphicsDevice::measureText (RectF& size, CoordF lineWidth, StringRef text, FontRef font)
{
	return measureString (size, text, font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NullGraphicsDevice::measureString (Rect& size, StringRef text, FontRef font)
{
	RectF sizeF;	
	measureString (sizeF, text, font);
	size (0, 0, coordFToInt (sizeF.right), coordFToInt (sizeF.bottom));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NullGraphicsDevice::measureText (Rect& size, Coord lineWidth, StringRef text, FontRef font)
{
	return measureString (size, text, font);
}

//************************************************************************************************
// NativeGraphicsPath
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (NativeGraphicsPath, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NativeGraphicsPath::getBounds (RectF& bounds) const
{
	Rect r;
	getBounds (r);
	bounds = rectIntToF (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NativeGraphicsPath::startFigure (PointRef p)
{
	startFigure (pointIntToF (p));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NativeGraphicsPath::lineTo (PointRef p)
{
	lineTo (pointIntToF (p));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NativeGraphicsPath::addRect (RectFRef rect)
{
	addRect (rectFToInt (rect));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NativeGraphicsPath::addRoundRect (RectFRef rect, CoordF rx, CoordF ry)
{
	addRoundRect (rectFToInt (rect), coordFToInt (rx), coordFToInt (ry));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NativeGraphicsPath::addTriangle (PointFRef p1, PointFRef p2, PointFRef p3)
{
	startFigure (p1);
	lineTo (p2);
	lineTo (p3);
	lineTo (p1);
	closeFigure ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NativeGraphicsPath::addTriangle (PointRef p1, PointRef p2, PointRef p3)
{
	addTriangle (pointIntToF (p1), pointIntToF (p2), pointIntToF (p3));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NativeGraphicsPath::addBezier (PointFRef p1, PointFRef c1, PointFRef c2, PointFRef p2)
{
	addBezier (pointFToInt (p1), pointFToInt (c1), pointFToInt (c2), pointFToInt (p2));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NativeGraphicsPath::addArc (RectFRef rect, float startAngle, float sweepAngle)
{
	addArc (rectFToInt (rect), startAngle, sweepAngle);
}

//************************************************************************************************
// NativeBitmap
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (NativeBitmap, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API NativeBitmap::getWidth () const
{
	return DpiScale::pixelToCoord (sizeInPixel.x, contentScaleFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API NativeBitmap::getHeight () const
{
	return DpiScale::pixelToCoord (sizeInPixel.y, contentScaleFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeBitmap::scrollPixelRect (const Rect& rect, const Point& delta)
{
	CCL_NOT_IMPL ("NativeBitmap::scrollPixelRect not implemented!")
	return kResultNotImplemented;
}

//************************************************************************************************
// NativeGradient
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (NativeGradient, Object)

//************************************************************************************************
// NativeGraphicsLayer
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (NativeGraphicsLayer, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsLayer::NativeGraphicsLayer ()
: parentLayer (nullptr),
  deferredRemoval (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsLayer::~NativeGraphicsLayer ()
{
	removeSublayers ();
	removePendingSublayers ();
	signal (Message (kDestroyed));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeGraphicsLayer::setContentScaleFactorDeep (float contentScaleFactor)
{
	setContentScaleFactor (contentScaleFactor);

	ForEach (sublayers, NativeGraphicsLayer, subLayer)
		subLayer->setContentScaleFactorDeep (contentScaleFactor);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeGraphicsLayer::setUpdateNeededRecursive ()
{
	setUpdateNeeded ();

	ForEach (sublayers, NativeGraphicsLayer, subLayer)
		subLayer->setUpdateNeededRecursive ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* CCL_API NativeGraphicsLayer::getParentLayer ()
{
	return parentLayer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* CCL_API NativeGraphicsLayer::getNextSibling (IGraphicsLayer* _layer) const
{
	NativeGraphicsLayer* layer = unknown_cast<NativeGraphicsLayer> (_layer);
	ASSERT (layer != nullptr)
	if(!layer)
		return nullptr;

	int index = sublayers.index (layer);
	if(index < 0 || index == sublayers.count () - 1)
		return nullptr;

	return static_cast<NativeGraphicsLayer*> (sublayers.at (index + 1));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* CCL_API NativeGraphicsLayer::getPreviousSibling (IGraphicsLayer* _layer) const
{
	NativeGraphicsLayer* layer = unknown_cast<NativeGraphicsLayer> (_layer);
	ASSERT (layer != nullptr)
	if(!layer)
		return nullptr;
	
	int index = sublayers.index (layer);
	if(index < 1)
		return nullptr;

	return static_cast<NativeGraphicsLayer*> (sublayers.at (index - 1));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsLayer::addSublayer (IGraphicsLayer* _layer)
{
	NativeGraphicsLayer* layer = unknown_cast<NativeGraphicsLayer> (_layer);
	ASSERT (layer != nullptr)
	if(!layer)
		return kResultInvalidArgument;

	ASSERT (layer->parentLayer == nullptr)
	layer->parentLayer = this;
	layer->retain ();
	sublayers.add (layer);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsLayer::removeSublayer (IGraphicsLayer* _layer)
{
	NativeGraphicsLayer* layer = unknown_cast<NativeGraphicsLayer> (_layer);
	ASSERT (layer != nullptr)
	if(!layer)
		return kResultInvalidArgument;

	ASSERT (layer->parentLayer == this)
	if(!sublayers.remove (layer))
		return kResultFailed;
		
	layer->parentLayer = nullptr;
	if(deferredRemoval)
		removedSublayers.add (layer);
	else
		layer->release ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsLayer::placeAbove (IGraphicsLayer* layer, IGraphicsLayer* sibling)
{
	return moveLayer (layer, sibling, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeGraphicsLayer::placeBelow (IGraphicsLayer* layer, IGraphicsLayer* sibling)
{
	return moveLayer (layer, sibling, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult NativeGraphicsLayer::moveLayer (IGraphicsLayer* _layer, IGraphicsLayer* _sibling, bool above)
{
	NativeGraphicsLayer* layer = unknown_cast<NativeGraphicsLayer> (_layer);
	NativeGraphicsLayer* sibling = unknown_cast<NativeGraphicsLayer> (_sibling);
	ASSERT (layer != nullptr && sibling != nullptr)
	if(!layer || !sibling)
		return kResultInvalidArgument;

	if(layer->getParentLayer () != this || sibling->getParentLayer () != this)
		return kResultInvalidArgument;

	int siblingIndex = sublayers.index (sibling);
	int currentIndex = sublayers.index (layer);
	if(siblingIndex < 0 || currentIndex < 0)
		return kResultFailed;
	
	int insertIndex = siblingIndex;
	if(above)
		insertIndex++;
	if(currentIndex < siblingIndex)
		insertIndex += 1;
	if(currentIndex == insertIndex)
		return kResultOk;

	if(!sublayers.removeAt (currentIndex))
		return kResultFailed;
	if(!sublayers.insertAt (insertIndex, layer))
		return kResultFailed;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeGraphicsLayer::removeSublayers ()
{
	ForEach (sublayers, NativeGraphicsLayer, subLayer)
		removeSublayer (subLayer);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeGraphicsLayer::removePendingSublayers ()
{
	ForEach (removedSublayers, NativeGraphicsLayer, subLayer)
		subLayer->release ();
	EndFor
	removedSublayers.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeGraphicsLayer::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == kOpacity)
	{
		setOpacity (var.asFloat ());
		return true;
	}
	else if(propertyId == kOffsetX)
	{
		setOffsetX (var.asFloat ());
		return true;
	}
	else if(propertyId == kOffsetY)
	{
		setOffsetY (var.asFloat ());
		return true;
	}
	else if(propertyId == kOffset)
	{
		Point p;
		if(IUIValue* value = IUIValue::toValue (var))
			value->toPoint (p);
		setOffset (p);
		return true;
	}
	else if(propertyId == kTransform)
	{
		Transform t;
		if(IUIValue* value = IUIValue::toValue (var))
			value->toTransform (t);
		setTransform (t);
		return true;
	}

	return SuperClass::setProperty (propertyId, var);
}

//************************************************************************************************
// NativeTextLayout
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (NativeTextLayout, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeTextLayout::getWordRange (Range& range, int textIndex) const
{
	range.start = textIndex;
	return getWordOrLineRange (range, RangeMode::kWord);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeTextLayout::getLineRange (Range& range, int textIndex) const
{
	return getExplicitLineRange (range, textIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeTextLayout::getExplicitLineRange (Range& range, int textIndex) const
{
	range.start = textIndex;
	return getWordOrLineRange (range, RangeMode::kLine);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult NativeTextLayout::getWordOrLineRange (Range& range, RangeMode mode, bool tryNonWord) const
{
	range.length = 0;

	AutoPtr<IRegularExpression> regExp = System::CreateRegularExpression ();
	regExp->construct ("(*UCP)\\w");

	auto isSeparator = [mode, tryNonWord, &regExp] (uchar character)
	{
		if(character == '\n')
			return true;

		if(mode == RangeMode::kLine)
			return false;
		else
		{
			String s;
			uchar c[2] = { character, 0 };
			s.append (c);
			return (regExp->isPartialMatch (s) != 0) == tryNonWord;
		}
	};

	StringRef text = getText ();
	for(; range.start > 0; range.start--)
	{
		if(isSeparator (text.at (range.start - 1)))
			break;
	}

	for(; range.start + range.length < text.length (); range.length++)
	{
		if(isSeparator (text.at (range.start + range.length)))
			break;
	}

	if(!tryNonWord && range.length == 0)
		return getWordOrLineRange (range, mode, true);

	return kResultOk;
}

//************************************************************************************************
// SimpleTextLayout
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SimpleTextLayout, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

SimpleTextLayout::SimpleTextLayout ()
: width (0),
  height (0),
  lineMode (kSingleLine)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::construct (StringRef text, Coord width, Coord height, FontRef font, LineMode mode, TextFormatRef format)
{
	return construct (text, (CoordF)width, (CoordF)height, font, mode, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::construct (StringRef text, CoordF width, CoordF height, FontRef font, LineMode mode, TextFormatRef format)
{
	this->text = text;
	setWidth (width);
	setHeight (height);
	setFont (font);
	setLineMode (mode);
	setFormat (format);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API SimpleTextLayout::getText () const
{
	return text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::resize (Coord width, Coord height)
{
	setWidth (width);
	setHeight (height);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::resize (CoordF width, CoordF height)
{
	setWidth (width);
	setHeight (height);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::setFontStyle (const Range& range, int style, tbool state)
{
	// range formatting is not supported
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::setFontSize (const Range& range, float size)
{
	// range formatting is not supported
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::setSpacing (const Range& range, float spacing)
{
	// range formatting is not supported
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::setLineSpacing (const Range& range, float lineSpacing)
{
	// range formatting is not supported
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::setTextColor (const Range& range, Color color)
{
	// range formatting is not supported
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::setBaselineOffset (const Range& range, float offset)
{
	// range formatting is not supported
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::setSuperscript (const Range& range)
{
	// range formatting is not supported
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::setSubscript (const Range& range)
{
	// range formatting is not supported
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::getBounds (Rect& bounds, int flags) const
{
	// provide a very rough estimate of the bounds
	float fontSizePixel = font.getSize () / 72.f * DpiScale::getDpi (1);
	float stringWidthPixel = text.length () * fontSizePixel;
	if(lineMode == kMultiLine)
	{
		bounds.setWidth (ccl_to_int (ccl_round<0, float> (ccl_min<float> (stringWidthPixel, width))));
		int lines = ccl_to_int (stringWidthPixel / width);
		bounds.setHeight (ccl_to_int (ccl_round<0, float> (fontSizePixel * lines * 1.1f)));
	}
	else
	{
		bounds.setWidth (ccl_to_int (ccl_round<0, float> (ccl_min<float> (stringWidthPixel, width))));
		bounds.setHeight (ccl_to_int (ccl_round<0, float> (fontSizePixel)));
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::getBounds (RectF& bounds, int flags) const
{
	// provide a very rough estimate of the bounds
	float fontSizePixel = font.getSize () / 72.f * DpiScale::getDpi (1);
	float stringWidthPixel = text.length () * fontSizePixel;
	if(lineMode == kMultiLine)
	{
		bounds.setWidth (ccl_round<0, float> (ccl_min<float> (stringWidthPixel, width)));
		int lines = ccl_to_int (stringWidthPixel / width);
		bounds.setHeight (ccl_round<0, float> (fontSizePixel * lines * 1.1f));
	}
	else
	{
		bounds.setWidth (ccl_round<0, float> (ccl_min<float> (stringWidthPixel, width)));
		bounds.setHeight (ccl_round<0, float> (fontSizePixel));
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::getImageBounds (RectF& bounds) const
{
	CCL_NOT_IMPL ("SimpleTextLayout::getImageBounds")
	return getBounds (bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::getBaselineOffset (PointF& offset) const
{
	CCL_NOT_IMPL ("SimpleTextLayout::getBaselineOffset")
	offset (0, 0);
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::hitTest (int& textIndex, PointF& position) const
{
	float fontSizePixel = font.getSize () / 72.f * DpiScale::getDpi (1);
	int line = 0;
	if(lineMode == kMultiLine)
		line = static_cast<int> (position.y / fontSizePixel / 1.1f);

	int charactersPerLine = ccl_to_int (width / fontSizePixel);

	textIndex = static_cast<int> (line * charactersPerLine + position.x / fontSizePixel);
	position.x = textIndex * fontSizePixel;
	position.y = line * fontSizePixel * 1.1f;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::getCharacterBounds (RectF& offset, int textIndex) const
{
	float fontSizePixel = font.getSize () / 72.f * DpiScale::getDpi (1);
	float stringWidthPixel = textIndex * fontSizePixel;
	if(lineMode == kMultiLine)
	{
		offset.left = ccl_round<0, float> (stringWidthPixel / width) * width;
		int lines = ccl_to_int (stringWidthPixel / width);
		offset.top = ccl_round<0, float> (fontSizePixel * (lines - 1) * 1.1f);
	}
	else
	{
		offset.left = ccl_round<0, float> (ccl_min<float> (stringWidthPixel, width));
		offset.top = 0;
	}
	offset.setHeight (fontSizePixel * 1.1f);
	offset.setWidth (fontSizePixel);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::getTextBounds (IMutableRegion& bounds, const Range& range) const
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleTextLayout::getLineRange (Range& range, int textIndex) const
{
	return kResultNotImplemented;
}

//************************************************************************************************
// SimpleFontTable
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SimpleFontTable, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void SimpleFontTable::clear ()
{
	fonts.removeAll ();	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SimpleFontTable::addFamily (FontFamily* family)
{	
	fonts.add (family);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SimpleFontTable::addFamilySorted (FontFamily* family)
{
	struct Sorter
	{
		static DEFINE_VECTOR_COMPARE_OBJECT(compare, AutoPtr <FontFamily>, left, right)
			return (*left)->name.compare ((*right)->name);
		}
	};	
	fonts.addSorted (family, Sorter::compare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SimpleFontTable::countFonts ()
{
	return fonts.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleFontTable::getFontName (String& name, int fontIndex)
{
	if(fontIndex < 0 || fontIndex >= fonts.count ())
		return kResultInvalidArgument;

	name = fonts.at (fontIndex)->name;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SimpleFontTable::countFontStyles (int fontIndex)
{
	if(fontIndex < 0 || fontIndex >= fonts.count ())
		return 0;
	
	return fonts.at (fontIndex)->styles.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleFontTable::getFontStyleName (String& name, int fontIndex, int styleIndex)
{
	if(fontIndex < 0 || fontIndex >= fonts.count ())
		return kResultInvalidArgument;

	FontFamily* font = fonts.at (fontIndex);
	if(styleIndex < 0 || styleIndex >= font->styles.count ())
		return kResultInvalidArgument;
	
	name = font->styles.at (styleIndex);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleFontTable::getExampleText (String& text, int fontIndex, int styleIndex)
{
	if(fontIndex < 0 || fontIndex >= fonts.count ())
		return kResultInvalidArgument;

	FontFamily* font = fonts.at (fontIndex);
	if(font->exampleText.isEmpty () == false)
	{
		text = font->exampleText;
		return kResultOk;
	}

	return kResultNotImplemented;
}
