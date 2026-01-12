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
// Filename    : ccl/platform/win/gui/dragndrop.win.h
// Description : Windows-specific Drag-and-Drop
//
//************************************************************************************************

#ifndef _ccl_dragndrop_win_h
#define _ccl_dragndrop_win_h

#include "ccl/gui/system/dragndrop.h"

struct IDataObject;

namespace CCL {

//************************************************************************************************
// WindowsDragSession
//************************************************************************************************

class WindowsDragSession: public DragSession
{
public:
	DECLARE_CLASS (WindowsDragSession, DragSession)

	WindowsDragSession (IUnknown* source = nullptr, int inputDevice = kMouseInput);
	WindowsDragSession (IDataObject* dataObject, int inputDevice = kMouseInput);

	PROPERTY_POINTER (::IDataObject, dataObject, DataObject)

	// IDragSession
	IAsyncOperation* CCL_API dragAsync () override;

private:
	void convertNativeItems ();
};

} // namespace CCL

#endif // _ccl_dragndrop_win_h
