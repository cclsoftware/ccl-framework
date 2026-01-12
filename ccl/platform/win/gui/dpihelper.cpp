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
// Filename    : ccl/platform/win/gui/dpihelper.cpp
// Description : DPI-Awareness API Helper
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/win/gui/dpihelper.h"
#include "ccl/platform/win/system/registry.h"

#include "ccl/base/storage/configuration.h"

#include "ccl/public/cclversion.h"

#include <shellscalingapi.h>

#pragma comment(lib, "shcore.lib")

using namespace CCL;
using namespace Win32;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CCL_DPIAWARENESS_ROOT \
	"Software\\" CCL_SETTINGS_NAME "\\DPIAwareness"

static inline DpiAwarenessContext toCCLDpiAwareness (DPI_AWARENESS_CONTEXT win32Context)
{
	if(::AreDpiAwarenessContextsEqual (win32Context, DPI_AWARENESS_CONTEXT_UNAWARE))
		return kDpiContextUnaware;
	if(::AreDpiAwarenessContextsEqual (win32Context, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE))
		return kDpiContextSystemAware;
	return kDpiContextDefault;
}

static inline DPI_AWARENESS_CONTEXT fromCCLDpiAwareness (DpiAwarenessContext cclContext)
{
	switch(cclContext)
	{
	case kDpiContextUnaware : return DPI_AWARENESS_CONTEXT_UNAWARE;
	case kDpiContextSystemAware : return DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
	default :
		return DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE;
	}
}

//************************************************************************************************
// DpiInfo
//************************************************************************************************

DpiInfo CCL::Win32::gDpiInfo;
DEFINE_SINGLETON_CLASS (DpiInfo, Object)
DEFINE_CLASS_UID (DpiInfo, 0xe1655bdb, 0x7b, 0x48dd, 0x93, 0x9b, 0x70, 0x7b, 0xa8, 0x68, 0xe6, 0x96)

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* DpiInfo::__createSingleton ()
{
	return return_shared (&gDpiInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DpiInfo::DpiInfo ()
: active (false),
  perMonitorDpi (false),
  systemDpiFactor (1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DpiInfo::init (InitMode mode)
{
	// Don't use any high DPI APIs when system scaling override is enabled
	// This can be done by user via "Properties -> Compatibility -> Override high DPI scaling behavior -> System"
	if(isSystemScalingOverrideEnabled ())
		return;

	bool dpiAware = true;
	Configuration::Registry::instance ().getValue (dpiAware, "CCL.Win32", "DpiAware");

	if(dpiAware || isDpiAwarenessEnabled ())
	{
		active = true;

		if(mode == kSetProcessDpiAwareness)
		{
			::SetProcessDpiAwareness (PROCESS_PER_MONITOR_DPI_AWARE);
			perMonitorDpi = true;
		}
		else if(mode == kUseProcessDpiAwareness)
		{
			PROCESS_DPI_AWARENESS processDpiAwareness = PROCESS_DPI_UNAWARE;
			::GetProcessDpiAwareness (NULL, &processDpiAwareness);

			if(processDpiAwareness == PROCESS_PER_MONITOR_DPI_AWARE)
				perMonitorDpi = true;
			else if(processDpiAwareness == PROCESS_DPI_UNAWARE)
				active = false; // we are running in a host process that isn't DPI aware
		}

		if(active)
		{
			// PLEASE NOTE: LOGPIXELSX (system DPI) changes upon user log on only.
			// On Windows 8.1 or later the primary monitor DPI can be different from
			// the value reported by GDI if settings are changed by the user "on the fly".
			// LATER TODO: use GetDpiForSystem()!
			HDC hdc = ::GetDC (NULL);
			if(hdc)
			{
				int dpiX = ::GetDeviceCaps (hdc, LOGPIXELSX);
				::ReleaseDC (NULL, hdc);
				systemDpiFactor = DpiScale::getFactor (dpiX);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DpiInfo::isDpiAware () const
{
	return active;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float DpiInfo::getDpiFactorForMonitor (void* hMonitor)
{
	UINT dpiX = 0, dpiY = 0;
	HRESULT hr = ::GetDpiForMonitor ((HMONITOR)hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
	ASSERT (SUCCEEDED (hr))
	return SUCCEEDED (hr) ? DpiScale::getFactor (dpiX) : 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DpiInfo::enableNonClientDpiScaling (void* hwnd)
{
	::EnableNonClientDpiScaling ((HWND)hwnd);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DpiInfo::adjustWindowRectForDpiFactor (void* rect, uint32 wstyle, bool hasMenu, uint32 xstyle, float dpiFactor)
{
	UINT dpi = (UINT)DpiScale::getDpi (dpiFactor);	
	::AdjustWindowRectExForDpi ((LPRECT)rect, wstyle, hasMenu, xstyle, dpi);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DpiInfo::logicalToPhysicalPoint (void* hwnd, void* point) const
{
	BOOL result = ::LogicalToPhysicalPointForPerMonitorDPI ((HWND)hwnd, (LPPOINT)point);
	ASSERT (result)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DpiInfo::physicalToLogicalPoint (void* hwnd, void* point) const
{
	BOOL result = ::PhysicalToLogicalPointForPerMonitorDPI ((HWND)hwnd, (LPPOINT)point);
	ASSERT (result)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DpiInfo::canSwitchDpiAwarenessContext () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DpiInfo::switchToDpiAwarenessContext (DpiAwarenessContext which)
{
	CCL_PRINTF ("*** Switch to DPI Awareness Context %s ***\n", 
				which == kDpiContextUnaware ? "Unaware" : 
				which == kDpiContextSystemAware ? "System" :
				"Default")

	DPI_AWARENESS_CONTEXT newContext = fromCCLDpiAwareness (which);
	DPI_AWARENESS_CONTEXT oldContext = ::SetThreadDpiAwarenessContext (newContext);
	ASSERT (oldContext != NULL)
	return oldContext != NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DpiAwarenessContext CCL_API DpiInfo::getCurrentDpiAwarenessContext () const
{
	DPI_AWARENESS_CONTEXT context = ::GetThreadDpiAwarenessContext ();
	return toCCLDpiAwareness (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DpiAwarenessContext CCL_API DpiInfo::getWindowDpiAwarenessContext (void* hwnd) const
{
	DPI_AWARENESS_CONTEXT context = ::GetWindowDpiAwarenessContext ((HWND)hwnd);
	return toCCLDpiAwareness (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DpiInfo::canSwitchDpiHostingBehavior ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DpiInfo::switchToDpiHostingBehavior (DpiHostingBehavior which)
{
	CCL_PRINTF ("*** Switch to DPI Hosting Behavior %s ***\n", which == kDpiHostingMixed ? "Mixed" : "Default")

	DPI_HOSTING_BEHAVIOR newBehavior = which == kDpiHostingMixed ? DPI_HOSTING_BEHAVIOR_MIXED :
									   DPI_HOSTING_BEHAVIOR_DEFAULT;
	DPI_HOSTING_BEHAVIOR oldBehavior = ::SetThreadDpiHostingBehavior (newBehavior);
	ASSERT (oldBehavior != DPI_HOSTING_BEHAVIOR_INVALID)
	return oldBehavior != DPI_HOSTING_BEHAVIOR_INVALID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DpiInfo::setDpiAwarenessEnabled (tbool state)
{
	Registry::Accessor accessor (Registry::kKeyCurrentUser, CCL_DPIAWARENESS_ROOT);
	String name (Registry::AppValueName ());
	accessor.writeDWORD (state ? 1 : 0, nullptr, name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DpiInfo::isDpiAwarenessEnabled () const
{
	Registry::Accessor accessor (Registry::kKeyCurrentUser, CCL_DPIAWARENESS_ROOT);
	String name (Registry::AppValueName ());
	uint32 value = 0;
	if(accessor.readDWORD (value, nullptr, name))
		return value != 0;
	else
	{
		// fall back to default
		bool dpiAwarenessDefault = false;
		Configuration::Registry::instance ().getValue (dpiAwarenessDefault, "CCL.Win32", "DpiAwarenessDefault");
		return dpiAwarenessDefault;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DpiInfo::isSystemScalingOverrideEnabled () const
{
	#define APP_COMPATIBILITY_FLAGS_LAYERS \
		"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"

	#define SYSTEM_SCALING_OVERRIDE "DPIUNAWARE"

	// check current user and all user setting
	Registry::RegKey keys[2] = {Registry::kKeyCurrentUser, Registry::kKeyLocalMachine};
	String name (Registry::AppValueName ());
	for(auto key : keys)
	{
		Registry::Accessor accessor (key, APP_COMPATIBILITY_FLAGS_LAYERS);
		String stringValue;
		if(accessor.readString (stringValue, String::kEmpty, name))
			if(stringValue.contains (SYSTEM_SCALING_OVERRIDE))
				return true;
	}

	return false;
}
