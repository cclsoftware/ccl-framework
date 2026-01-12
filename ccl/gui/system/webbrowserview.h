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
// Filename    : ccl/gui/system/webbrowserview.h
// Description : Web Browser View
//
//************************************************************************************************

#ifndef _ccl_webbrowserview_h
#define _ccl_webbrowserview_h

#include "ccl/gui/views/view.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/gui/inavigator.h"
#include "ccl/public/gui/icommandhandler.h"

namespace CCL {

class NativeWebControl;

//************************************************************************************************
// WebBrowserView
/** A WebView embeds a WebBrowser in a view. */
//************************************************************************************************

class WebBrowserView: public View
{
public:
	DECLARE_CLASS (WebBrowserView, View)

	WebBrowserView (IUnknown* controller = nullptr, const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~WebBrowserView ();

	DECLARE_STYLEDEF (customStyles)

	INavigator* getNavigator () const;

	// View
	IUnknown* CCL_API getController () const override;
	tbool CCL_API setController (IUnknown* controller) override;
	void attached (View* parent) override;
	void removed (View* parent) override;
	bool onFocus (const FocusEvent& event) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	void onSize (const Point& delta) override;
	void onMove (const Point& delta) override;
	void onDisplayPropertiesChanged (const DisplayChangedEvent& event) override;
	bool onContextMenu (const ContextMenuEvent& event) override;

protected:
	SharedPtr<IUnknown> controller;
	NativeWebControl* nativeControl;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// NativeWebControl
//************************************************************************************************

class NativeWebControl: public Object,
						public INavigator,
						public ICommandHandler
{
public:
	static bool isAvailable ();
	static NativeWebControl* createInstance (WebBrowserView& owner);

	StyleRef getOptions () const;
	Rect getSizeInWindow () const;

	virtual void attachView () = 0;		///< attach to owner view
	virtual void detachView () = 0;		///< detach from owner view
	virtual void takeFocus () {}		///< take keyboard focus
	virtual void updateSize () = 0;		///< owner has been resized/moved

	PROPERTY_BOOL (textSelected, TextSelected)
	virtual void copyText () {}			///< copy text to clipboard

	// INavigator (platform-specific methods)
	tresult CCL_API navigate (UrlRef url) override;
	tresult CCL_API refresh () override;
	tresult CCL_API goBack () override;
	tresult CCL_API goForward () override;

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	CLASS_INTERFACE2 (INavigator, ICommandHandler, Object)

protected:
	NativeWebControl (WebBrowserView& owner);
	~NativeWebControl ();

	WebBrowserView& owner;
	Url currentUrl;
	String currentTitle;
	int commandState;

	enum CommandStates { kCanBack = 1<<0, kCanForward = 1<<1 };
	PROPERTY_FLAG (commandState, kCanBack, flagCanBack)
	PROPERTY_FLAG (commandState, kCanForward, flagCanForward)

	// INavigator (do not overwrite)
	tresult CCL_API navigateDeferred (UrlRef url) override;
	UrlRef CCL_API getCurrentUrl () const override;
	StringRef CCL_API getCurrentTitle () const override;
	tbool CCL_API canGoBack () const override;
	tbool CCL_API canGoForward () const override;
	tresult CCL_API goHome () override;
	UrlRef CCL_API getHomeUrl () const override;
};

} // namespace CCL

#endif // _ccl_webbrowserview_h
