//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/shared/skia/skialayer.h
// Description : Graphics Layer for Skia content
//
//************************************************************************************************

#ifndef _skialayer_h
#define _skialayer_h

#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/imaging/bitmap.h"

#include "ccl/platform/shared/skia/skiaglue.h"

namespace CCL {

//************************************************************************************************
// SkiaLayer
//************************************************************************************************

class SkiaLayer: public NativeGraphicsLayer
{
public:
	DECLARE_CLASS_ABSTRACT (SkiaLayer, NativeGraphicsLayer)

	SkiaLayer ();

	PROPERTY_OBJECT (Rect, contentRect, ContentRect)
	PROPERTY_OBJECT (Rect, size, LayerSize)
	PROPERTY_OBJECT (Point, offset, LayerOffset)
	PROPERTY_OBJECT (Color, backColor, LayerBackColor)
	PROPERTY_OBJECT (SkMatrix, transformMatrix, TransformMatrix)
	PROPERTY_VARIABLE (float, opacity, LayerOpacity)
	PROPERTY_VARIABLE (int, mode, LayerMode)

	virtual void onFrame (SkSurface* targetSurface) {}

	virtual void resetGPUState () {}

	virtual Rect getClipRect ();

	// NativeGraphicsLayer
	tresult CCL_API construct (IUnknown* content, RectRef bounds = Rect (), int mode = 0, float contentScaleFactor = 1.f) override;
	tresult CCL_API setContent (IUnknown* content) override;
	void CCL_API setSize (Coord width, Coord height) override;
	void CCL_API setContentScaleFactor (float factor) override;
	void CCL_API setUpdateNeeded () override;
	void CCL_API setUpdateNeeded (RectRef rect) override;
	void CCL_API suspendTiling (tbool suspend, const Rect* visibleRect) override;
	void CCL_API setOffset (PointRef offset) override;
	void CCL_API setOffsetX (float offsetX) override;
	void CCL_API setOffsetY (float offsetY) override;
	void CCL_API setMode (int mode) override;
	void CCL_API setOpacity (float opacity) override;
	void CCL_API setTransform (TransformRef transform) override;
	tresult CCL_API addAnimation (StringID propertyId, const IAnimation* animation) override;
	tresult CCL_API removeAnimation (StringID propertyId) override;
	tbool CCL_API getPresentationProperty (Variant& value, StringID propertyId) const override;
	void CCL_API setBackColor (const Color& color) override;

protected:
	SharedPtr<IUnknown> content;

	AutoPtr<Bitmap> contentBitmap;

	Rect dirtyRect;
	float contentScaleFactor;
	bool needCanvasUpdate;

	void requestCanvasUpdate ();
	virtual void checkSize () {}
	virtual void updatePosition () {}
	virtual void requestFrame (bool deep = false) {}
};

} // namespace CCL

#endif // _skialayer_h
 
