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
// Filename    : ccl/platform/wasm/filemanager.wasm.h
// Description : WebAssembly file manager
//
//************************************************************************************************

#ifndef _ccl_filemanager_wasm_h
#define _ccl_filemanager_wasm_h

#include "ccl/system/filemanager.h"

namespace CCL {

//************************************************************************************************
// WasmFileManager
//************************************************************************************************

class WasmFileManager: public FileManager
{
public:
	WasmFileManager ();
	~WasmFileManager ();

	// FileManager
	void CCL_API terminate () override;
	
protected:
	// FileManager
	tresult startWatching (UrlRef url, int flags) override;
	tresult stopWatching (UrlRef url) override;
};

} // namespace CCL

#endif // _ccl_filemanager_wasm_h
