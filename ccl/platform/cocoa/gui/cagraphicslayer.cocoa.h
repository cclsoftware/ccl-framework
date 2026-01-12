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
// Filename    : ccl/platform/cocoa/gui/cagraphicslayer.cocoa.h
// Description : CoreAnimation Graphics Layer Base Class
//
//************************************************************************************************

#ifndef _cagraphicslayer_h
#define _cagraphicslayer_h

#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/public/gui/framework/ianimation.h"
#include "ccl/public/text/cstring.h"

@class CALayer;
@class NSString;
@class CAMediaTimingFunction;

namespace CCL {
namespace MacOS {

//************************************************************************************************
// CoreAnimationLayer
//************************************************************************************************

class CoreAnimationLayer: public NativeGraphicsLayer
{
public:
	DECLARE_CLASS_ABSTRACT (CoreAnimationLayer, NativeGraphicsLayer)
	
	CoreAnimationLayer ();
	~CoreAnimationLayer ();

	virtual void getSize (Coord& width, Coord& height) const;

	void removePendingSublayersFromParent ();

	// NativeGraphicsLayer
	void CCL_API setOffset (PointRef offset) override;
	void CCL_API setOffsetX (float offsetX) override;
	void CCL_API setOffsetY (float offsetY) override;
	virtual void CCL_API setSize (Coord width, Coord height) override;
	void CCL_API setMode (int mode) override;
	void CCL_API setOpacity (float opacity) override;
	void CCL_API setTransform (TransformRef transform) override;
	virtual void CCL_API setContentScaleFactor (float factor) override;
	tresult CCL_API addSublayer (IGraphicsLayer* layer) override;
	tresult CCL_API removeSublayer (IGraphicsLayer* layer) override;
	tresult CCL_API addAnimation (StringID propertyId, const IAnimation* animation) override;
	tresult CCL_API removeAnimation (StringID propertyId) override;
	tbool CCL_API getPresentationProperty (Variant& value, StringID propertyId) const override;
	void CCL_API setBackColor (const Color& color) override;
	tresult CCL_API placeAbove (IGraphicsLayer* layer, IGraphicsLayer* sibling) override;
	tresult CCL_API placeBelow (IGraphicsLayer* layer, IGraphicsLayer* sibling) override;

	#if DEBUG
	PROPERTY_MUTABLE_CSTRING (name, Name)
	#endif

protected:
	SharedPtr<IUnknown> content;
	CALayer* nativeLayer;
		
	virtual CALayer* createNativeLayer ();
	float getContentScaleFactor () const;
	
	static NSString* getNativePropertyPath (StringID propertyId);
	static CAMediaTimingFunction* getNativeTimingFunction (AnimationTimingType functionId, const AnimationControlPoints& values);
};

} // namespace MacOS
} // namespace CCL

#endif // _cagraphicslayer_h
