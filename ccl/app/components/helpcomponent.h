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
// Filename    : ccl/app/components/helpcomponent.h
// Description : Help Component
//
//************************************************************************************************

#ifndef _ccl_helpcomponent_h
#define _ccl_helpcomponent_h

#include "ccl/app/component.h"

#include "ccl/public/gui/framework/ipresentable.h"
#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/itimer.h"

namespace CCL {

interface IMenu;

//************************************************************************************************
// HelpCatalogComponent
//************************************************************************************************

class HelpCatalogComponent: public Component
{
public:
	HelpCatalogComponent ();

	void makeMainMenu (IMenu& menu, StringID category);

	void appendCatalogMenu (IMenu& menu, StringID category);

	// Component
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

protected:
	static const CString kCatalogPrefix;
	static const CString kLocationPrefix;
	static const int kCommandIndexStart;

	int commandIndex;

	void makeMenu (IMenu& menu, ICommandHandler* handler, StringID category);
};

//************************************************************************************************
// HelpTutorialComponent
//************************************************************************************************

class HelpTutorialComponent: public Component
{
public:
	HelpTutorialComponent ();

	static void makeMenu (IMenu& menu, CStringRef categoryFilter = IHelpTutorial::kGlobal);

	// Component
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

protected:
	static const CString kTutorialPrefix;
	static const int kCommandIndexStart;
};

//************************************************************************************************
// HelpInfoViewComponent
//************************************************************************************************

class HelpInfoViewComponent: public Component,
							 public IHelpInfoViewer,
							 public IUIEventHandler,
							 public ITimerTask
{
public:
	DECLARE_CLASS (HelpInfoViewComponent, Component)

	HelpInfoViewComponent ();
	~HelpInfoViewComponent ();

	// IHelpInfoViewer
	void CCL_API updateHelpInfo (IPresentable* info) override;

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	CLASS_INTERFACE3 (IHelpInfoViewer, IUIEventHandler, ITimerTask, Component)

protected:
	bool active;
	int viewCount;
	ViewPtr infoView;
	SharedPtr<IPresentable> currentInfo;
	uint32 lastModifiers;

	void updateInfoView ();

	// IUIEventHandler
	tbool CCL_API handleEvent (IWindow* window, const GUIEvent& event) override;

	// ITimerTask
	void CCL_API onTimer (ITimer* timer) override;

	friend class HelpInfoControl;
	void setActive (bool state);
	void viewAttached ();
	void viewDetached ();
};

} // namespace CCL

#endif // _ccl_helpcomponent_h
