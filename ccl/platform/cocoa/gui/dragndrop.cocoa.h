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
// Filename    : ccl/platform/cocoa/gui/dragndrop.cocoa.h
// Description : Mac OS Drag-and-Drop
//
//************************************************************************************************

#ifndef _ccl_dragndrop_cocoa_h
#define _ccl_dragndrop_cocoa_h

#include "ccl/gui/system/dragndrop.h"

#include "ccl/base/asyncoperation.h"

@protocol NSDraggingInfo;

namespace CCL {

//************************************************************************************************
// CocoaDragSession
//************************************************************************************************

class CocoaDragSession: public DragSession
{
public:
	DECLARE_CLASS (CocoaDragSession, DragSession)

	CocoaDragSession (IUnknown* source = 0, int inputDevice = kMouseInput);
	CocoaDragSession (id <NSDraggingInfo> dragInfo, int inputDevice = kMouseInput);
	~CocoaDragSession ();

	PROPERTY_VARIABLE (id <NSDraggingInfo>, dragInfo, DragInfo)
	PROPERTY_SHARED_AUTO (AsyncOperation, dragOperation, DragOperation)

	// DragSession
	IAsyncOperation* CCL_API dragAsync () override;
	void showNativeDragImage (bool state) override;
	void onDragFinished (const DragEvent& event) override;

private:
	DragGuard* dragGuard;

	void convertNativeItems ();
};

} // namespace CCL

#endif // _ccl_dragndrop_cocoa_h
