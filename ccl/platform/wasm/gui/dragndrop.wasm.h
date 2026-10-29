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
// Filename    : ccl/platform/wasm/gui/dragndrop.wasm.h
// Description : WebAssembly Drag-and-Drop
//
//************************************************************************************************

#ifndef _ccl_dragndrop_wasm_h
#define _ccl_dragndrop_wasm_h

#include "ccl/gui/system/dragndrop.h"

#include "ccl/base/asyncoperation.h"

namespace CCL {

//************************************************************************************************
// WasmDragSession
//************************************************************************************************

class WasmDragSession: public DragSession
{
public:
	DECLARE_CLASS (WasmDragSession, DragSession)

	WasmDragSession (IUnknown* source = 0, int inputDevice = kMouseInput);
	~WasmDragSession ();

	void addData (CStringRef mimeType, StringRef data);

	DECLARE_STRINGID_MEMBER (kUrlListMimeType)
	
	PROPERTY_MUTABLE_CSTRING (preferredMimeType, PreferredMimeType)
	
	// IDragSession
	IAsyncOperation* CCL_API dragAsync () override;
	void onDragFinished (const DragEvent& event) override;
	
protected:
	SharedPtr<AsyncOperation> operation;
	DragGuard* dragGuard;
};

} // namespace CCL

#endif // _ccl_dragndrop_wasm_h
