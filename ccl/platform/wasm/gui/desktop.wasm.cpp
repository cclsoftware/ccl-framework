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
// Filename    : ccl/platform/wasm/gui/desktop.wasm.cpp
// Description : Desktop Management
//
//************************************************************************************************

#include "ccl/gui/windows/desktop.h"

namespace CCL {

namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (int, getViewportWidth, (), 
{
	return Math.max (document.documentElement.clientWidth || 0, window.innerWidth || 0);
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (int, getViewportHeight, (), 
{
	return Math.max (document.documentElement.clientHeight || 0, window.innerHeight || 0);
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, findElement, (char* cString, int cStringSize, int x, int y), 
{
	const element = document.elementFromPoint (x, y);
	if(!element || !element.id) 
	{
		if(cStringSize > 0) 
			cString[0] = 0;
		return false;
	}
	stringToUTF8 (element.id, cString, cStringSize);
	return true;
});

} // namespace Wasm

//************************************************************************************************
// WasmDesktopManager
//************************************************************************************************

class WasmDesktopManager: public DesktopManager
{
public:
	// DesktopManager
	IWindow* CCL_API findWindow (PointRef screenPos, int flags = 0) override;
	tbool CCL_API getMonitorSize (Rect& rect, int index, tbool useWorkArea) const override;
	float CCL_API getMonitorScaleFactor (int index) const override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmDesktopManager wasmDesktop;
DesktopManager& Desktop = wasmDesktop;

} // namespace CCL

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// WasmDesktopManager
//************************************************************************************************

IWindow* CCL_API WasmDesktopManager::findWindow (PointRef screenPos, int flags)
{
	char windowId[64] = {};
	if(findElement (windowId, ARRAY_COUNT (windowId), screenPos.x, screenPos.y))
	{
		CString id (windowId);
		for(int i = countWindows () - 1; i >= 0; i--)
		{
			IWindow* window = getWindow (i);
			if(window && id.contains (reinterpret_cast<CStringPtr> (window->getSystemWindow ())))
				return window;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WasmDesktopManager::getMonitorSize (Rect& rect, int index, tbool useWorkArea) const
{
	if(index != 0)
		return false;
	
	rect.setSize ({getViewportWidth (), getViewportHeight ()});
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API WasmDesktopManager::getMonitorScaleFactor (int index) const
{
	return emscripten_get_device_pixel_ratio ();
}