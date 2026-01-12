//************************************************************************************************
//
// CCL Spy
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
// Filename    : spymanager.h
// Description : ccl spy gui management: window management, commands, skin reload
//
//************************************************************************************************

#ifndef _spymanager_h
#define _spymanager_h

#include "ccl/app/component.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/iuserinterface.h"

namespace CCL {
class Settings;
interface IMenu; }

namespace Spy {

//************************************************************************************************
// SpyManager
//************************************************************************************************

class SpyManager: public CCL::Component,
				  public CCL::IWindowEventHandler
{
public:
	SpyManager ();
	~SpyManager ();

	bool openWindow ();
	void simulateOrientationChange (CCL::OrientationType orientation);

	// Component
	CCL::tresult CCL_API initialize (IUnknown* context = nullptr) override;
	CCL::tresult CCL_API terminate () override;
	CCL::tbool CCL_API checkCommandCategory (CCL::CStringRef category) const override;
	CCL::tbool CCL_API interpretCommand (const CCL::CommandMsg& msg) override;
	void CCL_API notify (ISubject* subject, CCL::MessageRef msg) override;

	// IWindowEventHandler
	CCL::tbool CCL_API onWindowEvent (CCL::WindowEvent& windowEvent) override;

	CLASS_INTERFACE (IWindowEventHandler, Component)

private:
	CCL::ViewPtr spyView;
	CCL::Settings* settings;
	bool spyMenuAdded;
	CCL::SignalSink debugSink;

	void hookIntoMenuBar ();
	void extendDebugMenu (CCL::IMenu* menu);

	bool reloadSkin (CCL::tbool keepImages);

	typedef Component SuperClass;
};

} //  namespace Spy

#endif // _spymanager_h
