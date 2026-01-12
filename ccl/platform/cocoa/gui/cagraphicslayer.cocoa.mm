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
// Filename    : ccl/platform/cocoa/gui/cagraphicslayer.cocoa.mm
// Description : CoreAnimation Graphics Layer Base Class
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/gui/cagraphicslayer.cocoa.h"

#include "ccl/base/message.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/graphics/graphicshelper.h"
#include "ccl/gui/system/animation.h"

#include "ccl/platform/cocoa/quartz/cghelper.h"
#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

#include <QuartzCore/CAAnimation.h>
#include <QuartzCore/CAMediaTimingFunction.h>

using namespace CCL;
using namespace MacOS;

#if CCL_PLATFORM_MAC
#define COLORTYPE NSColor
#elif CCL_PLATFORM_IOS
#define COLORTYPE UIColor
#endif

#define LOG_INVALIDATE	 0 && DEBUG_LOG
#define LOG_DRAW		 0 && DEBUG_LOG
#define HIGHLIGHT_LAYERS 0 && DEBUG

inline ::CATransform3D toCATransform (CCL::TransformRef t)
{
	::CGAffineTransform cgt = {t.a0, t.b0, t.a1, t.b1, t.t0, t.t1};
	::CATransform3D cat = CATransform3DMakeAffineTransform (cgt);
	return cat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline ::NSValue* valueWithPoint (CCL::PointRef p)
{
	#if CCL_PLATFORM_IOS
	CGPoint point ({ (float)p.x, (float)p.y });
	return [NSValue valueWithCGPoint:point];
	#else
	NSPoint point ({ (float)p.x, (float)p.y });
	return [NSValue valueWithPoint:point];
	#endif
}

//************************************************************************************************
// CoreAnimationHandler
//************************************************************************************************

@interface CCL_ISOLATED (CoreAnimationHandler) : NSObject<CAAnimationDelegate>
{
	SharedPtr<IAnimationCompletionHandler> _completionHandler;
}

- (id)initWithCompletionHandler:(IAnimationCompletionHandler*) animation;
- (void)animationDidStop:(CAAnimation*)theAnimation finished:(BOOL)flag;

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (CoreAnimationHandler)

- (id)initWithCompletionHandler:(IAnimationCompletionHandler*)completionHandler
{
	self = [super init];
	if(self)
		_completionHandler = completionHandler;
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)animationDidStop:(CAAnimation*)theAnimation finished:(BOOL)flag
{
	if(_completionHandler)
		_completionHandler->onAnimationFinished ();
}

@end


//************************************************************************************************
// CoreAnimationLayer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CoreAnimationLayer, NativeGraphicsLayer)
	
//////////////////////////////////////////////////////////////////////////////////////////////////

CoreAnimationLayer::CoreAnimationLayer ()
: nativeLayer (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CoreAnimationLayer::~CoreAnimationLayer ()
{
	if(nativeLayer)
		[nativeLayer release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CoreAnimationLayer::getSize (Coord& width, Coord& height) const
{
	width = Coord(nativeLayer.frame.size.width);
	height = Coord(nativeLayer.frame.size.height);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CALayer* CoreAnimationLayer::createNativeLayer ()
{
	CALayer* layer = [[CALayer alloc] init]; 
	#if HIGHLIGHT_LAYERS
	[layer setBorderColor:[[COLORTYPE blueColor] CGColor]];
	[layer setBorderWidth:1.0];
	#endif	
	
	return layer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreAnimationLayer::setOffset (PointRef offset)
{
	CGRect frame = nativeLayer.frame;
	frame.origin = CGPointMake (offset.x, offset.y);
	nativeLayer.frame = frame;
	
	CCL_PRINTF ("layer \"%s\" ", name.str ()) PRINT_CGRECT ("setOffset ", nativeLayer.frame)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreAnimationLayer::setOffsetX (float offsetX)
{
	CGPoint position = nativeLayer.position;
	position.x = offsetX;
	nativeLayer.position = position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreAnimationLayer::setOffsetY (float offsetY)
{
	CGPoint position = nativeLayer.position;
	position.y = offsetY;
	nativeLayer.position = position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreAnimationLayer::setSize (Coord width, Coord height)
{
	CGRect frame = nativeLayer.frame;
	frame.size = CGSizeMake (width, height);
	nativeLayer.frame = frame;
	CCL_PRINTF ("layer \"%s\" ", name.str ()) PRINT_CGRECT ("setSize ", nativeLayer.frame)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreAnimationLayer::setMode (int mode)
{
	nativeLayer.masksToBounds = (mode & kClipToBounds) ? YES : NO;    // clip sublayers
	nativeLayer.opaque = (mode & kIgnoreAlpha) ? YES : NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreAnimationLayer::setOpacity (float opacity)
{
	nativeLayer.opacity = opacity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreAnimationLayer::setTransform (TransformRef transform)
{
	CATransform3D cat = toCATransform (transform);
	nativeLayer.transform = cat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreAnimationLayer::setContentScaleFactor (float factor)
{
	if(nativeLayer)
		if(factor != getContentScaleFactor ())
		{
			nativeLayer.contentsScale = factor;
			setContent (content);
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CoreAnimationLayer::getContentScaleFactor () const
{
	if(nativeLayer)
		return (float)nativeLayer.contentsScale;

	return 1.0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreAnimationLayer::addSublayer (IGraphicsLayer* layer)
{
	tresult tr = SuperClass::addSublayer (layer);
	if(tr == kResultOk)
	{
		CoreAnimationLayer* caLayer = unknown_cast<CoreAnimationLayer> (layer);
		
		CGRect frame = caLayer->nativeLayer.frame;
		caLayer->nativeLayer.frame = frame;
		
		[nativeLayer addSublayer:caLayer->nativeLayer];
		
		CCL_PRINTF ("addSublayer \"%s\" to \"%s\"\n", caLayer->getName ().str (), name.str ());
		PRINT_CGRECT ("  parent frame: ", nativeLayer.frame)
		PRINT_CGRECT ("  child frame:  ", caLayer->nativeLayer.frame)
		PRINT_CGRECT ("  child bounds: ", caLayer->nativeLayer.bounds)
		PRINT_CGPOINT("  position", caLayer->nativeLayer.position)
		CCL_PRINTF   ("  anchorPoint: %f, %f\n", caLayer->nativeLayer.anchorPoint.x, caLayer->nativeLayer.anchorPoint.y)
	}
	return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreAnimationLayer::removeSublayer (IGraphicsLayer* layer)
{
	tresult tr = SuperClass::removeSublayer (layer);

	if(tr == kResultOk && !deferredRemoval)
	{
		CoreAnimationLayer* caLayer = unknown_cast<CoreAnimationLayer> (layer);
		CCL_PRINTF ("removeSublayer \"%s\" from \"%s\"\n", caLayer->getName ().str (), name.str ());
		[caLayer->nativeLayer removeFromSuperlayer];
	}
	return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CoreAnimationLayer::removePendingSublayersFromParent ()
{
	ForEach (removedSublayers, NativeGraphicsLayer, subLayer)
		CoreAnimationLayer* caLayer = ccl_cast<CoreAnimationLayer> (subLayer);
		CCL_PRINTF ("removeSublayer \"%s\" from \"%s\"\n", caLayer->getName ().str (), name.str ());
		[caLayer->nativeLayer removeFromSuperlayer];
	EndFor

	removePendingSublayers ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NSString* CoreAnimationLayer::getNativePropertyPath (StringID propertyId)
{
	if(propertyId == kOffsetX)
		return @"position.x";
	
	if(propertyId == kOffsetY)
		return @"position.y";
	
	if(propertyId == kOffset)
		return @"position";

	if(propertyId == kOpacity)
		return @"opacity";
	
	if(propertyId == kTransform)
		return @"transform";
	
	return nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CAMediaTimingFunction* CoreAnimationLayer::getNativeTimingFunction (AnimationTimingType functionId, const AnimationControlPoints& values)
{
	if(functionId == kTimingEaseInOut)
		return [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
	if(functionId == kTimingEaseIn)
		return [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseIn];
	if(functionId == kTimingEaseOut)
		return [CAMediaTimingFunction functionWithControlPoints:0 : 0 : 0.125 : 1];
	if(functionId == kTimingLinear)
		return [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionLinear];
	if(functionId == kTimingCubicBezier)
		return [CAMediaTimingFunction functionWithControlPoints:(float)values.c1x: (float)values.c1y: (float)values.c2x: (float)values.c2y];
	
	return nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreAnimationLayer::addAnimation (StringID propertyId, const IAnimation* _animation)
{
	const Animation* animation = Animation::cast<Animation> (_animation);
	if(!animation)
		return kResultInvalidArgument;
	
	NSString* keyPath = getNativePropertyPath (propertyId);
	if(!keyPath)
		return kResultInvalidArgument;
	
	CAMediaTimingFunction* timingFunction = getNativeTimingFunction (animation->getTimingType (), animation->getControlPoints ());
	if(!timingFunction)
		return kResultInvalidArgument;
	
	NSValue* fromValue = nil;
	NSValue* toValue = nil;
	if(const BasicAnimation* basicAnimation = ccl_cast<BasicAnimation> (animation))
	{
		if(basicAnimation->getValueType () == UIValue::kNil) // scalar value
		{
			float start = basicAnimation->getStartValue ();
			float end = basicAnimation->getEndValue ();
			fromValue = [NSNumber numberWithFloat:start];
			toValue	= [NSNumber numberWithFloat:end];
		}
		else if(propertyId == kOffset)
		{
			Point startPoint, endPoint;
			if(IUIValue* value = IUIValue::toValue (basicAnimation->getStartValue ()))
				value->toPoint (startPoint);

			if(IUIValue* value = IUIValue::toValue (basicAnimation->getEndValue ()))
				value->toPoint (endPoint);
			
			fromValue = valueWithPoint (startPoint);
			toValue = valueWithPoint (endPoint);
		}
	}
	else if(const TransformAnimation* transfromAnimation = ccl_cast<TransformAnimation> (animation))
	{
		Transform t1, t2;
		transfromAnimation->getStartTransform (t1);
		transfromAnimation->getEndTransform (t2);
		
		fromValue = [NSValue valueWithCATransform3D:toCATransform (t1)];
		toValue = [NSValue valueWithCATransform3D:toCATransform (t2)];
	}
	
	if(fromValue && toValue)
	{		
		CABasicAnimation* nativeAnimation = [CABasicAnimation animation];
		nativeAnimation.keyPath = keyPath;
		nativeAnimation.fromValue = fromValue;
		nativeAnimation.toValue = toValue;
		nativeAnimation.duration = animation->getDuration ();
		nativeAnimation.autoreverses = animation->isAutoReverse ();
		nativeAnimation.repeatCount = animation->getRepeatCount ();
		nativeAnimation.timingFunction = timingFunction;
		nativeAnimation.fillMode = kCAFillModeForwards;
		
		if(animation->getCompletionHandler ())
		{		
			CCL_ISOLATED (CoreAnimationHandler)* completionHandler = [[CCL_ISOLATED (CoreAnimationHandler) alloc] initWithCompletionHandler:animation->getCompletionHandler ()];
			nativeAnimation.delegate = completionHandler;
			[completionHandler release]; // retained by CAAnimation (exception to usual memory management rules)
		}
		
		NSString* key = [NSString stringWithCString:propertyId encoding:(NSASCIIStringEncoding)];
		[nativeLayer addAnimation:nativeAnimation forKey:key];
		[nativeLayer setValue:nativeAnimation.toValue forKeyPath:keyPath];
		return kResultOk;
	}
	else
		return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreAnimationLayer::removeAnimation (StringID propertyId)
{
	NSString* key = [NSString stringWithCString:propertyId encoding:(NSASCIIStringEncoding)];
	[nativeLayer removeAnimationForKey:key];
	
	#if DEBUG_LOG
	if(nativeLayer.animationKeys.count > 0)
	{
		NSLog(@"removeAnimation \"%s\"; remaining:\n", propertyId.str ());
		for(NSString* obj in nativeLayer.animationKeys)
			NSLog(@"\"%@\"", obj);
	}
	#endif
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreAnimationLayer::getPresentationProperty (Variant& value, StringID propertyId) const
{
	CALayer* presentationLayer = (CALayer*)nativeLayer.presentationLayer;
	if(presentationLayer)
	{
		if(propertyId == kOffsetX)
		{
			value = presentationLayer.position.x;
			return true;
		}
		else if(propertyId == kOffsetY)
		{
			value = presentationLayer.position.y;
			return true;
		}
		else if(propertyId == kOpacity)
		{
			value = presentationLayer.opacity;
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreAnimationLayer::setBackColor (const Color& color)
{
	CGColorRef cgColor = [COLORTYPE colorWithRed:color.getRedF () green:color.getGreenF () blue:color.getBlueF () alpha:color.getAlphaF ()].CGColor;
	nativeLayer.backgroundColor = cgColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreAnimationLayer::placeAbove (IGraphicsLayer* _layer, IGraphicsLayer* _sibling)
{
	tresult result = SuperClass::placeAbove (_layer, _sibling);
	if(result == kResultOk)
	{
		CoreAnimationLayer* layer = unknown_cast<CoreAnimationLayer> (_layer);
		CoreAnimationLayer* sibling = unknown_cast<CoreAnimationLayer> (_sibling);
		if(layer == nullptr || sibling == nullptr)
			return kResultInvalidArgument;

		if([[nativeLayer sublayers] indexOfObject:sibling->nativeLayer] == NSNotFound)
			return kResultInvalidArgument;

		[layer->nativeLayer removeFromSuperlayer];
		[nativeLayer insertSublayer:layer->nativeLayer above:sibling->nativeLayer];
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreAnimationLayer::placeBelow (IGraphicsLayer* _layer, IGraphicsLayer* _sibling)
{
	tresult result = SuperClass::placeBelow (_layer, _sibling);
	if(result == kResultOk)
	{
		CoreAnimationLayer* layer = unknown_cast<CoreAnimationLayer> (_layer);
		CoreAnimationLayer* sibling = unknown_cast<CoreAnimationLayer> (_sibling);
		if(layer == nullptr || sibling == nullptr)
			return kResultInvalidArgument;

		if([[nativeLayer sublayers] indexOfObject:sibling->nativeLayer] == NSNotFound)
			return kResultInvalidArgument;

		[layer->nativeLayer removeFromSuperlayer];
		[nativeLayer insertSublayer:layer->nativeLayer below:sibling->nativeLayer];
	}
	return result;
}
