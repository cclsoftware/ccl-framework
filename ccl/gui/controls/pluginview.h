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
// Filename    : ccl/gui/controls/pluginview.h
// Description : Plugin View
//
//************************************************************************************************

#ifndef _ccl_pluginview_h
#define _ccl_pluginview_h

#include "ccl/gui/views/view.h"

#include "ccl/gui/graphics/imaging/bitmap.h"

#include "ccl/public/gui/ipluginview.h"
#include "ccl/public/gui/framework/itimer.h"

namespace CCL {

class ChildWindow;

//************************************************************************************************
// PlugInView
//************************************************************************************************

class PlugInView: public View,
				  public IPlugInViewFrame,
				  public IPlugInViewRendererFrame,
				  public IPlugInViewParamFinder,
				  public ITimerTask
{
public:
	DECLARE_CLASS (PlugInView, View)
	
	DECLARE_STYLEDEF (customStyles)

	PlugInView (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~PlugInView ();

	static PlugInView* createPlugInView (IUnknown* plug, StringID name);	
	
	static bool isSystemScalingAvailable ();
	static bool setManagementInterface (IPlugInViewManagement* plugInViewManagement);
	static PlugInView* getAttachingView ();

	enum HostingMode
	{
		kDefaultHosting,
		kPixelUnitHosting,
		kSystemScaledHosting
	};

	void setHostingMode (HostingMode mode);
	HostingMode getHostingMode () const;

	void setView (IPlugInView* plugView);
	IPlugInView* getView () const;

	// IPlugInViewFrame
	void CCL_API setFrameSize (const Rect& size) override;
	void CCL_API getFrameSize (Rect& size) const override;
	void CCL_API onIdle () override;
	void CCL_API enableParentProtection (tbool state, void*& protectedData) override;
	void CCL_API onPluginViewActivated () override;
	tresult CCL_API queryWindowInterface (UIDRef iid, void** ptr) override;

	// IPlugInViewRendererFrame
	void CCL_API invalidateFrame (const Rect& rect) override;

	// IPlugInViewParamFinder
	IUnknown* CCL_API createParameterIdentity (const Point& p) override;

	// View
	void attached (View* parent) override;
	void removed (View* parent) override;
	void draw (const UpdateRgn& updateRgn) override;
	void onSize (const Point& delta) override;
	void onMove (const Point& delta) override;
	void calcSizeLimits () override;
	void constrainSize (Rect& rect) const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	void onDisplayPropertiesChanged (const DisplayChangedEvent& event) override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool onMouseUp (const MouseEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	bool onFocus (const FocusEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onKeyUp (const KeyEvent& event) override;
	void onActivate (bool state) override;
	tbool CCL_API delegateEvent (const GUIEvent& event) override;

	CLASS_INTERFACES (View)

protected:
	static constexpr IBitmap::PixelFormat kDefaultBitmapFormat = IBitmap::kRGBAlpha;
	static IPlugInViewManagement* plugInViewManagement;
	static PlugInView* attachingView;

	struct PlugInCallScope;

	HostingMode hostingMode;
	IPlugInView* plugView;
	UnknownPtr<IPlugInViewRenderer> renderer;
	AutoPtr<Bitmap> renderBitmap;
	ChildWindow* childWindow;
	float initialScaleFactor;
	bool plugViewOnSizePending;	
	int childWindowSizeChanging;
	#if DEBUG
	bool insideScalingChanged;
	#endif

	// ITimerTask
	void CCL_API onTimer (ITimer* timer) override;

	bool isSizable () const;
	bool wantsExtendedInput () const;
	float getContentScaleFactor () const;
	Point& toPlugInPoint (Point& p) const;
	Rect& toPlugInRect (Rect& r) const;
	Rect& fromPlugInRect (Rect& r) const;
	void calcPlugViewRect (Rect& r) const;
	void repairPlugViewSize ();
	void attach ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline 
//////////////////////////////////////////////////////////////////////////////////////////////////

inline PlugInView::HostingMode PlugInView::getHostingMode () const
{ return hostingMode; }

} // namespace CCL

#endif // _ccl_pluginview_h
