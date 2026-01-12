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
// Filename    : ccl/platform/linux/platformintegration/guiintegration.h
// Description : CCL GUI Platform Integration
//
//************************************************************************************************

#ifndef _ccl_guiintegration_h
#define _ccl_guiintegration_h

#include "ccl/platform/shared/interfaces/platformgui.h"

namespace CCL {
namespace PlatformIntegration {

//************************************************************************************************
// PlatformGUIBase
//************************************************************************************************

class PlatformGUIBase: public IPlatformGUI
{
public:
	bool isRunning () const { return started; }

	// IPlatformGUI
	void startup (CStringPtr applicationId) override { started = true; }
	void release () override { started = false; }
	void setProperty (const Core::Property& value) override {}
	void getProperty (Core::Property& value) override {}

private:
	bool started;
};

//************************************************************************************************
// PlatformGUIFactory
/** Class factory for classes based on PlatformGUIBase. */
//************************************************************************************************

template <class UIClass>
struct PlatformGUIFactory
{
	static void* createInstance (Core::InterfaceID iid = IPlatformGUI::kIID)
	{ 
		if(iid == IPlatformGUI::kIID)
			return &UIClass::instance ();
		return nullptr;
	}
};

//************************************************************************************************
// GUIClassFactory
/** Class factory ensuring that a class can only be instantiated 
 * 		if a corresponding IPlatformGUI implementation is being used. */
//************************************************************************************************

template <class UIClass, class BaseFactory>
struct GUIClassFactory
{
	static void* createInstance (Core::InterfaceID iid)
	{
		if(!UIClass::instance ().isRunning ())
			return nullptr;
		return BaseFactory::createInstance (iid);
	}
};

} // namespace PlatformIntegration
} // namespace CCL

#endif // _ccl_guiintegration_h
