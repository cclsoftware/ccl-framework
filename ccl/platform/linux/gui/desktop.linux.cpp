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
// Filename    : ccl/platform/linux/gui/desktop.linux.cpp
// Description : Desktop Management
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/childwindow.h"
#include "ccl/gui/gui.h"

#include "ccl/platform/linux/wayland/monitorhelper.h"
#include "ccl/platform/linux/gui/window.linux.h"

namespace CCL {

//************************************************************************************************
// LinuxDesktopManager
//************************************************************************************************

class LinuxDesktopManager: public DesktopManager
{
public:
	// DesktopManager
	IWindow* CCL_API findWindowUnderCursor (int flags = 0) override;
	int CCL_API countMonitors () const override;
	int CCL_API getMainMonitor () const override;
	int CCL_API findMonitor (PointRef where, tbool fallbackToDefault) const override;
	tbool CCL_API getMonitorSize (Rect& rect, int index, tbool useWorkArea) const override;
	float CCL_API getMonitorScaleFactor (int index) const override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxDesktopManager linuxDesktop;
DesktopManager& Desktop = linuxDesktop;

} // namespace CCL

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// LinuxDesktopManager
//************************************************************************************************

IWindow* CCL_API LinuxDesktopManager::findWindowUnderCursor (int flags)
{
	return LinuxWindow::getMouseWindow ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API LinuxDesktopManager::countMonitors () const
{
	return MonitorHelper::instance ().countOutputs ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API LinuxDesktopManager::getMainMonitor () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API LinuxDesktopManager::findMonitor (PointRef where, tbool defaultToPrimary) const
{
	const MonitorHelper& monitorHelper = MonitorHelper::instance ();
	for(int i = 0; i < monitorHelper.countOutputs (); i++)
	{
		if(monitorHelper.getOutput (i).logicalSize.pointInside (where))
			return i;
	}
	return defaultToPrimary ? 0 : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxDesktopManager::getMonitorSize (Rect& size, int index, tbool useWorkArea) const
{
	const MonitorHelper& monitorHelper = MonitorHelper::instance ();
	const WaylandOutput& output = monitorHelper.getOutput (index);
	if(output.handle == nullptr)
		return false;

	size = output.logicalSize.isEmpty () ? Rect (output.x, output.y, output.width, output.height) : output.logicalSize;
	if(useWorkArea)
	{
		if(monitorHelper.getWorkAreaSize ().isNull ())
		{
			// We don't to know the work area size. Apply a factor to account for task bars etc.
			static const float kWorkAreaSize = .85f;
			size.zoom ((useWorkArea ? kWorkAreaSize : 1.f) / output.scaleFactor);
		}
		else
			size = monitorHelper.getWorkAreaSize ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API LinuxDesktopManager::getMonitorScaleFactor (int index) const
{
	return MonitorHelper::instance ().getOutput (index).scaleFactor;
}
