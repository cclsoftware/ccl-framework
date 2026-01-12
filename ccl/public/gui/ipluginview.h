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
// Filename    : ccl/public/gui/ipluginview.h
// Description : Plug-In View
//
//************************************************************************************************

#ifndef _ccl_ipluginview_h
#define _ccl_ipluginview_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/gui/graphics/rect.h"

namespace CCL {

struct GUIEvent;
struct UpdateRgn;
interface IPlugInView;
interface IPlugInViewFrame;
interface IPlugInViewManagement;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (PlugInViewStatics, 0xb6035e5e, 0xfe1b, 0x4f61, 0x95, 0x2a, 0xe3, 0x5e, 0x7d, 0x15, 0xa4, 0xf0)
}

//************************************************************************************************
// IPlugInViewFactory
/** Factory interface for plug-in views. 
	\ingroup gui_view */
//************************************************************************************************

interface IPlugInViewFactory: IUnknown
{
	/** Create plug-in view by name. */
	virtual tresult CCL_API createPlugInView (IPlugInView*& view, StringID name) = 0;
	
	DECLARE_IID (IPlugInViewFactory)
};

DEFINE_IID (IPlugInViewFactory, 0x765b662d, 0x5015, 0x403b, 0x81, 0x4d, 0xdb, 0xf3, 0x99, 0x7e, 0xff, 0x58)

//************************************************************************************************
// IPlugInView
/** View interface for interaction with foreign GUI toolkits. 
	\ingroup gui_view */
//************************************************************************************************

interface IPlugInView: IUnknown
{
	enum ViewStyles
	{
		kSizable = 1<<0,			///< view is sizable within given limits
		kCanScale = 1<<1,			///< view supports content scaling
		kUnitIsPixels = 1<<2,		///< view coordinates are in pixels instead of points
		kSystemScalingAware = 1<<3,	///< view is ready for system scaling
		kWantsExtendedInput = 1<<4,	///< view wants extended input events via onViewEvent()
	};
	
	/** Get plug-in instance owning this view. */
	virtual IUnknown* CCL_API getOwner () const = 0;
	
	/** Get style. */
	virtual int CCL_API getStyle () const = 0;

	/** Get current size. */
	virtual void CCL_API getSize (Rect& size) const = 0;

	/** Get size limits (optional, only if kSizable). */
	virtual void CCL_API getSizeLimits (SizeLimit& sizeLimits) const = 0;

	/** Assign frame object provided by host. */
	virtual void CCL_API setFrame (IPlugInViewFrame* frame) = 0;

	/**	View has been attached to host window
		nativeParent is:
		HWND on Windows,
		NSView on macOS,
		WindowContext on Linux (defined in ilinuxspecifics.h),
		or null when using IPlugInViewRenderer. 
		Not implemented for iOS and Android. */
	virtual void CCL_API attached (void* nativeParent) = 0;
	
	/** View has been removed from host window. */
	virtual void CCL_API removed () = 0;

	/** Called periodically by host. */
	virtual void CCL_API onIdle () = 0;
	
	/** Resize notification. */
	virtual void CCL_API onSize (const Rect& newSize) = 0;

	/** Handle view event. */
	virtual tbool CCL_API onViewEvent (const GUIEvent& event) = 0;

	/** Adjust the proposed size to a size supported by the plug-in (if necessary). */
	virtual void CCL_API constrainSize (Rect& size) = 0;

	DECLARE_IID (IPlugInView)
};

DEFINE_IID (IPlugInView, 0x25b661c7, 0x5609, 0x4fe2, 0x81, 0xa7, 0xce, 0x90, 0x1, 0x28, 0xb0, 0xca)

//************************************************************************************************
// IPlugInViewFrame
/** Host-side of plug-in view. 
	\ingroup gui_view */
//************************************************************************************************

interface IPlugInViewFrame: IUnknown
{
	/** Call to request new size. */
	virtual void CCL_API setFrameSize (const Rect& size) = 0;
	
	/** Call to get current size. */
	virtual void CCL_API getFrameSize (Rect& size) const = 0;

	/** Call to give idle time back to host. */
	virtual void CCL_API onIdle () = 0;

	/** Call if native parent windows must be protected from manipulations of naughty plug-ins. 
	    The plug-in-frame then temporarily disconnects the plug-in view from its parents. */
	virtual void CCL_API enableParentProtection (tbool state, void*& protectedData) = 0;

	/** Call to notify the host about activation of the native plug-in view. */
	virtual void CCL_API onPluginViewActivated () = 0;

	/** Query additional window interfaces. */
	virtual tresult CCL_API queryWindowInterface (UIDRef iid, void** ptr) = 0;

	DECLARE_IID (IPlugInViewFrame)
};

DEFINE_IID (IPlugInViewFrame, 0xed85bd70, 0x455f, 0x41b1, 0x8c, 0x81, 0x49, 0xc, 0x9, 0x58, 0xa2, 0x95)

//************************************************************************************************
// IPlugInViewRenderer
/** Interface for plug-in view rendering.
	\ingroup gui_view */
//************************************************************************************************

interface IPlugInViewRenderer: IUnknown
{
	/**	Check if given rendering type and format are supported.
		Default is ccl_iid<IBitmap> and IBitmap::kRGBAlpha for software rendering. */
	virtual tbool CCL_API isRenderingTypeSupported (UIDRef typeId, int format) = 0;
	
	/** Draw view content to target, default target is IBitmap. */
	virtual tresult CCL_API draw (IUnknown* target, const UpdateRgn& updateRgn) = 0;

	DECLARE_IID (IPlugInViewRenderer)
};

DEFINE_IID (IPlugInViewRenderer, 0xfc7de23c, 0x9519, 0x42c4, 0xbd, 0x53, 0x84, 0xac, 0x80, 0x37, 0x41, 0x93)

//************************************************************************************************
// IPlugInViewRendererFrame
/** Host-side interface when using rendering.
	\ingroup gui_view */
//************************************************************************************************

interface IPlugInViewRendererFrame: IUnknown
{
	/** Invalidate given rectangle. */
	virtual void CCL_API invalidateFrame (const Rect& rect) = 0;

	DECLARE_IID (IPlugInViewRendererFrame)
};

DEFINE_IID (IPlugInViewRendererFrame, 0x61580142, 0x4b26, 0x431c, 0x97, 0x20, 0xeb, 0x36, 0x6c, 0x88, 0x46, 0xae)

//************************************************************************************************
// IPlugInViewRepair
/**
	\ingroup gui_view */
//************************************************************************************************

interface IPlugInViewRepair: IUnknown
{
	/** Detect current size. */
	virtual tbool CCL_API detectSize (Rect& size) const = 0;

	/** Repair size. */
	virtual tbool CCL_API repairSize (RectRef size) = 0;

	DECLARE_IID (IPlugInViewRepair)
};

DEFINE_IID (IPlugInViewRepair, 0xe274d912, 0xaae9, 0x4e66, 0x85, 0x9, 0xb2, 0x72, 0x45, 0x7a, 0x99, 0xd9)

//************************************************************************************************
// IPlugInViewParamFinder
/** Extension to IPlugInView to identify parameters in a foreign view's client area.
	\ingroup gui_view */
//************************************************************************************************

interface IPlugInViewParamFinder: IUnknown
{
	/** Create parameter identity at position (can be null, must be released otherwise). */
	virtual IUnknown* CCL_API createParameterIdentity (const Point& p) = 0;

	DECLARE_IID (IPlugInViewParamFinder)
};

DEFINE_IID (IPlugInViewParamFinder, 0x5f34eb7, 0xdc49, 0x49f8, 0x86, 0x54, 0xf6, 0x55, 0xb8, 0x8f, 0x9a, 0xac)

//************************************************************************************************
// IPlugInViewStatics
/** Interface to static members of plug-in view host-side.
	\ingroup gui_view */
//************************************************************************************************

interface IPlugInViewStatics: IUnknown
{
	/** Check if system scaling is supported by current OS version (Windows only). */
	virtual tbool CCL_API isSystemScalingAvailable () const = 0;

	/** Assign management interface. */
	virtual tresult CCL_API setManagementInterface (IPlugInViewManagement* plugInViewManagement) = 0;

	DECLARE_IID (IPlugInViewStatics)
};

DEFINE_IID (IPlugInViewStatics, 0x8c693638, 0x6fc9, 0x4fc1, 0x81, 0xc9, 0x42, 0xee, 0xdd, 0xfd, 0x74, 0xe7)

//************************************************************************************************
// IPlugInViewManagement
/** Interface to manage plug-in view behavior from the application.
	\ingroup gui_view */
//************************************************************************************************

interface IPlugInViewManagement: IUnknown
{
	/** Check if system DPI scaling should be applied to foreign view of given class (Windows only). */
	virtual tbool CCL_API isSystemScalingEnabled (UIDRef cid) const = 0;

	DECLARE_IID (IPlugInViewManagement)
};

DEFINE_IID (IPlugInViewManagement, 0xfc08e61e, 0x8c58, 0x43ab, 0x85, 0x9f, 0x68, 0x6c, 0x1c, 0xed, 0xcc, 0xa2)

//************************************************************************************************
// PlugViewParentProtector
/** Helper to engage parent protection in scope.
	\ingroup gui_view */
//************************************************************************************************

struct PlugViewParentProtector
{
	IPlugInViewFrame* frame;
	void* protectedData;

	PlugViewParentProtector (IPlugInViewFrame* _frame)
	: frame (_frame),
	  protectedData (nullptr)
	{
		if(frame) frame->enableParentProtection (true, protectedData);
	}

	~PlugViewParentProtector ()
	{
		if(frame) frame->enableParentProtection (false, protectedData);
	}
};

} // namespace CCL

#endif // _ccl_ipluginview_h
