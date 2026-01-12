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
// Filename    : ccl/public/gui/framework/iwin32specifics.h
// Description : Interfaces specific to Windows (Win32)
//
//************************************************************************************************

#ifndef _ccl_iwin32specifics_h
#define _ccl_iwin32specifics_h

#include "ccl/public/gui/graphics/point.h"

namespace CCL {

interface IWindow;
interface IProgressNotify;

namespace Win32 {

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (TaskBar, 0x6c0c1c5b, 0x6a4f, 0x46d0, 0x91, 0xe8, 0x9b, 0x78, 0x6a, 0x2f, 0x57, 0x68)
	DEFINE_CID (DpiInfo, 0xe1655bdb, 0x7b, 0x48dd, 0x93, 0x9b, 0x70, 0x7b, 0xa8, 0x68, 0xe6, 0x96)
	DEFINE_CID (ScreenInfo, 0x2fb1afb6, 0x56ce, 0x43ae, 0xba, 0x91, 0x23, 0xc7, 0xd, 0xc, 0x87, 0xd7)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** DPI awareness context (available since Windows 10 1607). */
DEFINE_ENUM (DpiAwarenessContext)
{
	kDpiContextDefault, 
	kDpiContextUnaware,
	kDpiContextSystemAware
};

/** DPI hosting behavior (available since Windows 10 1803). */
DEFINE_ENUM (DpiHostingBehavior)
{
	kDpiHostingDefault, 
	kDpiHostingMixed 
};

//************************************************************************************************
// ITaskBar
/** 
	\ingroup gui */
//************************************************************************************************

interface ITaskBar: IUnknown
{
	/** Get the associated progress interface (Windows 7 and later). */
	virtual IProgressNotify* CCL_API getProgressBar (IWindow* window) = 0;

	DECLARE_IID (ITaskBar)
};

DEFINE_IID (ITaskBar, 0x7624a455, 0x543c, 0x414e, 0x8d, 0x6a, 0xd8, 0x28, 0x13, 0xa4, 0x83, 0xa6)

//************************************************************************************************
// IDpiInfo
/** 
	\ingroup gui */
//************************************************************************************************

interface IDpiInfo: IUnknown
{
	/** Check if high-DPI mode is currently active. */
	virtual tbool CCL_API isDpiAware () const = 0;

	/** Turn high-DPI mode on or off (requires application restart). */
	virtual void CCL_API setDpiAwarenessEnabled (tbool state) = 0;

	/** Check if high-DPI mode is turned on. */
	virtual tbool CCL_API isDpiAwarenessEnabled () const = 0;

	virtual tbool CCL_API canSwitchDpiAwarenessContext () const = 0;
	virtual tbool CCL_API switchToDpiAwarenessContext (DpiAwarenessContext which) = 0;
	virtual DpiAwarenessContext CCL_API getCurrentDpiAwarenessContext () const = 0;
	
	virtual tbool CCL_API canSwitchDpiHostingBehavior () = 0;
	virtual tbool CCL_API switchToDpiHostingBehavior (DpiHostingBehavior which) = 0;

	DECLARE_IID (IDpiInfo)

	//////////////////////////////////////////////////////////////////////////////////////////////

	bool isThreadDpiUnaware () const { return getCurrentDpiAwarenessContext () == kDpiContextUnaware; }
};

DEFINE_IID (IDpiInfo, 0x1cfc3769, 0x2c73, 0x4e10, 0xa6, 0x50, 0x10, 0x1d, 0xf9, 0xb8, 0xec, 0x79)

//************************************************************************************************
// DpiAwarenessScope
/** 
	\ingroup gui */
//************************************************************************************************

struct DpiAwarenessScope
{
	IDpiInfo& dpiInfo;
	DpiAwarenessContext oldContext;
	bool changed;

	DpiAwarenessScope (IDpiInfo& dpiInfo, DpiAwarenessContext newContext)
	: dpiInfo (dpiInfo),
	  oldContext (dpiInfo.getCurrentDpiAwarenessContext ()),
	  changed (false)
	{
		if(newContext != oldContext)
			changed = dpiInfo.switchToDpiAwarenessContext (newContext);
	}	

	~DpiAwarenessScope ()
	{
		if(changed)
		{
			dpiInfo.switchToDpiAwarenessContext (oldContext);
			changed = false;
		}
	}
};

//************************************************************************************************
// IScreenInfo
/** 
	\ingroup gui */
//************************************************************************************************

interface IScreenInfo: IUnknown
{
	/** Convert physical screen pixel position to global coordinate. */
	virtual void CCL_API screenPixelToGlobalCoord (Point& p) const = 0;

	DECLARE_IID (IScreenInfo)
};

DEFINE_IID (IScreenInfo, 0xdca12d53, 0x42b4, 0x4a0f, 0x83, 0xcd, 0xe1, 0x3f, 0xf3, 0xcb, 0xb8, 0xbe)

} // namespace Win32
} // namespace CCL

#endif // _ccl_iwin32specifics_h
