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
// Filename    : ccl/platform/android/gui/graphicslayer.android.h
// Description : Android Graphixs Layer implementation
//
//************************************************************************************************

#ifndef _ccl_graphicslayer_android_h
#define _ccl_graphicslayer_android_h

#include "ccl/gui/graphics/nativegraphics.h"

#include "core/public/gui/corerectlist.h"

#include "ccl/platform/android/cclandroidjni.h"

#define USE_LAYER_DIRTY_REGION 0

#if USE_LAYER_DIRTY_REGION
#include "core/public/gui/corerectlist.h"
#endif

namespace CCL {
namespace Android {

class FrameworkGraphics;

//************************************************************************************************
// AndroidGraphicsLayer
//************************************************************************************************

class AndroidGraphicsLayer: public NativeGraphicsLayer,
							public JniCast<AndroidGraphicsLayer>
{
public:
	DECLARE_CLASS (AndroidGraphicsLayer, NativeGraphicsLayer)
	
	AndroidGraphicsLayer ();
	~AndroidGraphicsLayer ();
	
	// NativeGraphicsLayer
	tresult CCL_API construct (IUnknown* content, RectRef bounds = Rect (), int mode = 0, float contentScaleFactor = 1.f) override;
	tresult CCL_API setContent (IUnknown* content) override;
	void CCL_API setOffset (PointRef offset) override;
	void CCL_API setOffsetX (float offsetX) override;
	void CCL_API setOffsetY (float offsetY) override;
	void CCL_API setSize (Coord width, Coord height) override;
	void CCL_API setMode (int mode) override;
	void CCL_API setOpacity (float opacity) override;
	void CCL_API setTransform (TransformRef transform) override;
	void CCL_API setContentScaleFactor (float factor) override;
	void CCL_API setUpdateNeeded () override;
	void CCL_API setUpdateNeeded (RectRef rect) override;
	tresult CCL_API addSublayer (IGraphicsLayer* layer) override;
	tresult CCL_API removeSublayer (IGraphicsLayer* layer) override;
	tresult CCL_API addAnimation (StringID propertyId, const IAnimation* animation) override;
	tresult CCL_API removeAnimation (StringID propertyId) override;
	tbool CCL_API getPresentationProperty (Variant& value, StringID propertyId) const override;
	tresult CCL_API flush () override;
	void CCL_API suspendTiling (tbool suspend, const Rect* visibleRect) override;

	void setGraphics (FrameworkGraphics* graphics);
	void redraw ();

	void isSprite (bool state);

	#if USE_LAYER_DIRTY_REGION
	Core::RectList& getDirtyRegion () { return dirtyRegion; }
	#endif

	class AnimationListener;
	class AnimationHelper;

protected:
	Android::JniObject layerView;
	FrameworkGraphics* graphics;
	SharedPtr<IUnknown> content;
	Rect size;
	float contentScaleFactor;

	#if USE_LAYER_DIRTY_REGION
	Core::RectList dirtyRegion;
	#endif
};

//************************************************************************************************
// AndroidGraphicsLayer
//************************************************************************************************

class AndroidRootLayer: public AndroidGraphicsLayer
{
public:
	AndroidRootLayer ();
	~AndroidRootLayer ();
	
	// NativeGraphicsLayer
	tresult CCL_API construct (IUnknown* content, RectRef bounds = Rect (), int mode = 0, float contentScaleFactor = 1.f) override;
	void CCL_API setOffset (PointRef offset) override;
	void CCL_API setOffsetX (float offsetX) override;
	void CCL_API setOffsetY (float offsetY) override;
	void CCL_API setSize (Coord width, Coord height) override;
	void CCL_API setUpdateNeeded () override;
	void CCL_API setUpdateNeeded (RectRef rect) override;
};

} // namespace Android
} // namespace CCL

#endif // _ccl_graphicslayer_android_h
