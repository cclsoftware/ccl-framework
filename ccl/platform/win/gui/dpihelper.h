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
// Filename    : ccl/platform/win/gui/dpihelper.h
// Description : DPI-Awareness API Helper
//
//************************************************************************************************

#ifndef _ccl_dpihelper_h
#define _ccl_dpihelper_h

#include "ccl/base/singleton.h"

#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/gui/framework/iwin32specifics.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// DpiInfo
//************************************************************************************************

class DpiInfo: public Object,
			   public IDpiInfo
{
public:
	DECLARE_CLASS (DpiInfo, Object)

	DpiInfo ();

	enum InitMode
	{
		kSetProcessDpiAwareness,
		kUseProcessDpiAwareness
	};

	void init (InitMode mode);
	tbool CCL_API isDpiAware () const override; // IDpiInfo
	
	float getSystemDpiFactor () const;
	bool isPerMonitorDpi () const;
	float getDpiFactorForMonitor (void* hMonitor);

	void enableNonClientDpiScaling (void* hwnd);
	bool adjustWindowRectForDpiFactor (void* rect, uint32 wstyle, bool hasMenu, uint32 xstyle, float dpiFactor);
	void logicalToPhysicalPoint (void* hwnd, void* point) const;
	void physicalToLogicalPoint (void* hwnd, void* point) const;

	// IDpiInfo
	tbool CCL_API canSwitchDpiAwarenessContext () const override;
	tbool CCL_API switchToDpiAwarenessContext (DpiAwarenessContext which) override;
	DpiAwarenessContext CCL_API getCurrentDpiAwarenessContext () const override;
	DpiAwarenessContext CCL_API getWindowDpiAwarenessContext (void* hwnd) const; // (not in interface)
	tbool CCL_API canSwitchDpiHostingBehavior () override;
	tbool CCL_API switchToDpiHostingBehavior (DpiHostingBehavior which) override;
	
	CLASS_INTERFACE (IDpiInfo, Object)

private:
	bool active;
	bool perMonitorDpi;
	float systemDpiFactor;

	static Object* __createSingleton (); // used by meta class

	bool isSystemScalingOverrideEnabled () const;

	// IDpiInfo
	void CCL_API setDpiAwarenessEnabled (tbool state) override;
	tbool CCL_API isDpiAwarenessEnabled () const override;
};

extern DpiInfo gDpiInfo;

//////////////////////////////////////////////////////////////////////////////////////////////////

inline float DpiInfo::getSystemDpiFactor () const { return systemDpiFactor; }
inline bool DpiInfo::isPerMonitorDpi () const { return perMonitorDpi; }

} // namespace Win32
} // namespace CCL

#endif // _ccl_dpihelper_h
