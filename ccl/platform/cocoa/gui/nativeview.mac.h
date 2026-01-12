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
// Filename    : ccl/platform/cocoa/gui/nativeview.mac.h
// Description : Customized NSView, wrapped
//
//************************************************************************************************

#ifndef _ccl_nativeviewmac_h
#define _ccl_nativeviewmac_h

#include "ccl/base/object.h"
#include "ccl/public/gui/graphics/rect.h"
#include "ccl/public/gui/framework/iwindow.h"

@class NSWindow;
@class NSView;
@class CALayer;

namespace CCL {

class OSXWindow;

namespace MacOS {

//************************************************************************************************
// NSWindow Helper
//************************************************************************************************

NSWindow* toNSWindow (const IWindow* window);

//************************************************************************************************
// NativeView
//************************************************************************************************

class NativeView: public Object
{
public:
	NativeView (NSView* view, OSXWindow* window = nullptr);
	~NativeView ();

	NSView* getView () const { return view; }
	void setLayer (CALayer* layer);
    CALayer* getLayer () const;
	
protected:
	OSXWindow* window;
	NSView* view;
};

//************************************************************************************************
// CustomView
//************************************************************************************************

class CustomView: public NativeView
{
public:
	CustomView (OSXWindow* window, const Rect& size);
	~CustomView ();

	bool embedInto (NSWindow* parent);
	bool embedInto (NSView* parent);
};

} // namespace MacOS
} // namespace CCL

#endif // _ccl_nativeviewmac_h

