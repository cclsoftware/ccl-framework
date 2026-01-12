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
// Filename    : ccl/app/navigation/navigator.h
// Description : Navigator Component
//
//************************************************************************************************

#ifndef _ccl_navigator_h
#define _ccl_navigator_h

#include "ccl/app/navigation/navigatorbase.h"

#include "ccl/public/gui/iviewstate.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/graphics/iimage.h"

namespace CCL {

//************************************************************************************************
// Navigator
//************************************************************************************************

class Navigator: public NavigatorBase2,
				 public ComponentSingleton<Navigator>,
				 public IViewStateHandler
{
public:
	DECLARE_CLASS (Navigator, NavigatorBase2)

	Navigator (StringRef name = nullptr, StringRef title = nullptr);
	~Navigator ();

	class CommandLink;
	CommandLink* addCommandLink (StringRef name, StringRef title, UrlRef url, IImage* icon = nullptr, int index = -1);

	PROPERTY_BOOL (autoShow, AutoShow)
	PROPERTY_BOOL (autoHide, AutoHide)
	PROPERTY_BOOL (autoHome, AutoHome)
	PROPERTY_BOOL (trackingEnabled, TrackingEnabled)
	PROPERTY_BOOL (dispatchCommandsToContentComponent, DispatchCommandsToContentComponent)
	PROPERTY_POINTER (IUnknown, defaultContentComponent, DefaultContentComponent)

	bool canOpenWindow () const;
	bool openWindow ();
	bool closeWindow ();

	void setVisibilityParam (StringRef linkName, IParameter* param); // (shared)

	// INavigator
	tresult CCL_API navigate (UrlRef url) override;
	tresult CCL_API navigateDeferred (UrlRef url) override;
	tresult CCL_API refresh () override;
	
	// IViewStateHandler
	tbool CCL_API saveViewState (StringID viewID, StringID viewName, IAttributeList& attributes, const CCL::IViewState* state) const override;
	tbool CCL_API loadViewState (StringID viewID, StringID viewName, const IAttributeList& attributes, CCL::IViewState* state) override;

	// Component
	IObjectNode* CCL_API findChild (StringRef id) const override;
	tbool CCL_API paramChanged (IParameter* param) override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override;
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	CLASS_INTERFACE (IViewStateHandler, NavigatorBase2)

protected:
	ObjectList commandLinks;
	bool restoreUrlSuspended;

	void updateNavigation ();
	void updateCommandLinks ();
	CommandLink* findCommandLink (StringRef name) const;
	IUnknown* getContentComponent () const;

	// Navigation Events
	void onNavigated () override;

	// Command Methods
	bool onHomeCmd (CmdArgs) override;
	bool onRefreshCmd (CmdArgs) override;
};

//************************************************************************************************
// Navigator::CommandLink
//************************************************************************************************

class Navigator::CommandLink: public Object
{
public:
	DECLARE_CLASS (CommandLink, Object)

	CommandLink (StringRef name = nullptr);
	CommandLink (StringRef name, StringRef title, UrlRef url);

	PROPERTY_STRING (name, Name)
	PROPERTY_STRING (title, Title)
	PROPERTY_OBJECT (Url, url, Url)
	PROPERTY_SHARED_AUTO (IImage, icon, Icon)
	PROPERTY_POINTER (IParameter, parameter, Parameter)
	PROPERTY_SHARED_AUTO (IParameter, visibilityParam, VisibilityParam)

	bool isVisible () const;

	// Object
	bool equals (const Object& obj) const override;
	bool toString (String& string, int flags = 0) const override;
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;
};

} // namespace CCL

#endif // _ccl_navigator_h
