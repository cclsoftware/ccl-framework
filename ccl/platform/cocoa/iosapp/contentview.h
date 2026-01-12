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
// Filename    : ccl/platform/cocoa/iosapp/contentview.h
// Description : iOS View to CCL form bridge
//
//************************************************************************************************

#ifndef _ccl_ios_contentview_h
#define _ccl_ios_contentview_h

#include "ccl/gui/windows/window.h"

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

class GestureRecognizerManager;

//************************************************************************************************
// ContentView
//************************************************************************************************

@interface CCL_ISOLATED (ContentView): UIView<UIGestureRecognizerDelegate, UIPencilInteractionDelegate, CALayerDelegate>
{
    CCL::Window* window;
	GestureRecognizerManager* recognizerManager;
}

+ (Class)layerClass;

- (void)setWindow:(CCL::Window*)window;
- (id)initWithFrame:(CGRect)frame;
- (void)dealloc;
- (void)drawRect:(CGRect)rect;
- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event;
- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event;
- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event;
- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event;
- (void)pressesBegan:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event;
- (void)pressesEnded:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event;
- (void)pencilInteractionDidTap:(UIPencilInteraction*)interaction API_AVAILABLE(ios(12.1));
- (void)layoutSublayersOfLayer:(CALayer*)layer;

@end

#include "ccl/public/base/ccldefpop.h"

#endif // _ccl_ios_contentview_h
