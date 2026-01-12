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
// Filename    : ccl/platform/android/gui/desktop.android.cpp
// Description : Desktop Management
//
//************************************************************************************************

#include "ccl/gui/windows/desktop.h"

#include "ccl/public/gui/graphics/dpiscale.h"

#include "ccl/platform/android/gui/frameworkactivity.h"
#include "ccl/platform/android/gui/frameworkview.h"

namespace CCL {

//************************************************************************************************
// AndroidDesktopManager
//************************************************************************************************

class AndroidDesktopManager: public DesktopManager
{
public:
	// DesktopManager
	IWindow* CCL_API findWindow (PointRef screenPos, int flags = 0) override;
	tbool CCL_API getMonitorSize (Rect& rect, int index, tbool useWorkArea) const override;
	float CCL_API getMonitorScaleFactor (int index) const override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* CCL_API AndroidDesktopManager::findWindow (PointRef screenPos, int flags)
{
	for(int layer = kNumWindowLayers - 1; layer >= 0; layer--)
		for(Window* window : windows[layer])
			if(window->getSize ().pointInside (screenPos))
				return window;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidDesktopManager androidDesktop;
DesktopManager& Desktop = androidDesktop;

} // namespace CCL

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// AndroidDesktopManager
//************************************************************************************************

tbool CCL_API AndroidDesktopManager::getMonitorSize (Rect& rect, int index, tbool useWorkArea) const
{
	if(index == 0)
	{
		FrameworkActivity* activity = FrameworkActivity::getCurrentActivity ();

		if(useWorkArea)
			// note: when we hide the status bar / navigation bar with the additional flag SYSTEM_UI_FLAG_LAYOUT_STABLE,
			// this work area will always be the full size, regardless of whether these controls are currently hidden or not
			rect = activity->getWorkArea ();
		else
			rect = Rect (0, 0, activity->getScreenSize ());

		DpiScale::toCoordRect (rect, activity->getDensityFactor ());
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API AndroidDesktopManager::getMonitorScaleFactor (int index) const
{
	if(index == 0)
		return FrameworkActivity::getCurrentActivity ()->getDensityFactor ();
	else
		return 1.f;
}
