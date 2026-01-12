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
// Filename    : ccl/platform/cocoa/gui/nativeview.ios.h
// Description : Wrapped UIView
//
//************************************************************************************************

#ifndef _ccl_nativeviewios_h
#define _ccl_nativeviewios_h

#include "ccl/base/object.h"
#include "ccl/platform/cocoa/iosapp/contentview.h"

#include "ccl/public/base/ccldefpush.h"
#include <UIKit/UIKit.h>
#include "ccl/public/base/ccldefpop.h"

namespace CCL {

namespace MacOS {

//************************************************************************************************
// NativeView
//************************************************************************************************

class NativeView: public Object
{
public:
	NativeView (CCL_ISOLATED (ContentView)* _view)
	: view (_view)
	{
		[view retain];
	}
	
	~NativeView ()
	{
		[view release];
	}
	
	CCL_ISOLATED (ContentView)* getView () const { return view; }
	CALayer* getLayer () const { return view.layer; }
	
protected:
	CCL_ISOLATED (ContentView)* view;
};

} // namespace MacOS
} // namespace CCL

#endif // _ccl_nativeviewios_h

